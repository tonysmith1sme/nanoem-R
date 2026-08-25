/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

/// effect_corpus - batch MME effect compiler for compatibility reporting.
///
/// Walks a directory (or list file) of .fx/.fxsub effects, compiles each through the
/// production fx9 compiler plugin path, and writes a machine readable JSONL report:
///
///   {"path":"...","status":"ok","language":"glsl","numPasses":2,"numCompiled":2,...}
///
/// Options:
///   --language <glsl|essl|hlsl|msl|spirv>  target backend language (default: glsl)
///   --output <file.jsonl>                  report destination (default: stdout)
///   --cache <file.jsonl>                   reuse entries matching path+size+mtime+language
///   --manifest <file>                      "<status> <path>" lines; exits 2 on regressions
///   --optimize / --no-optimize             SPIR-V optimizer toggle (default: off)
///   --validate / --no-validate             translated source validation toggle (default: off)
///   --verbose                              print full sinks per failing effect
///
/// Exit codes: 0 = completed, 2 = manifest regressions found.

#include "fx9next/Compiler.h"

#include <sys/stat.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <string>
#include <vector>

using namespace fx9next;

namespace {

const char *kLanguageNames[] = { "glsl", "essl", "hlsl", "msl", "spirv" };
const Compiler::LanguageType kLanguageTypes[] = { Compiler::kLanguageTypeGLSL, Compiler::kLanguageTypeESSL,
    Compiler::kLanguageTypeHLSL, Compiler::kLanguageTypeMSL, Compiler::kLanguageTypeSPIRV };

void
writeJSONString(FILE *output, const std::string &value)
{
    fputc('"', output);
    for (auto it = value.begin(), end = value.end(); it != end; ++it) {
        const char c = *it;
        if (c == '"' || c == '\\') {
            fputc('\\', output);
            fputc(c, output);
        }
        else if (c == '\n') {
            fputs("\\n", output);
        }
        else if (c == '\r') {
            fputs("\\r", output);
        }
        else if (c == '\t') {
            fputs("\\t", output);
        }
        else if (static_cast<unsigned char>(c) < 0x20) {
            fprintf(output, "\\u%04x", c);
        }
        else {
            fputc(c, output);
        }
    }
    fputc('"', output);
}

std::string
firstErrorLine(const std::string &source)
{
    size_t offset = 0;
    while (offset < source.size()) {
        size_t next = source.find('\n', offset);
        std::string line = source.substr(offset, next == std::string::npos ? std::string::npos : next - offset);
        offset = next == std::string::npos ? source.size() : next + 1;
        if (line.find("ERROR") != std::string::npos) {
            return line;
        }
    }
    return source.substr(0, 256);
}

bool
hasExtension(const std::string &path, const char *extension)
{
    const size_t offset = path.size() - strlen(extension);
    if (path.size() < strlen(extension)) {
        return false;
    }
    for (size_t i = 0; i < strlen(extension); i++) {
        if (tolower(path[offset + i]) != extension[i]) {
            return false;
        }
    }
    return path[offset - 1] == '.';
}

void
collectEffectFiles(const std::string &path, std::vector<std::string> &effects)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return;
    }
    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        const std::string command = "find " + path + " -type f \\( -iname '*.fx' -o -iname '*.fxsub' \\) | sort";
        if (FILE *pipe = popen(command.c_str(), "r")) {
            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string found(buffer);
                while (!found.empty() && (found.back() == '\n' || found.back() == '\r')) {
                    found.pop_back();
                }
                if (!found.empty()) {
                    effects.push_back(found);
                }
            }
            pclose(pipe);
        }
    }
    else if (hasExtension(path, "fx") || hasExtension(path, "fxsub")) {
        effects.push_back(path);
    }
    else {
        /* list file: one path per line, '#' comments allowed */
        if (FILE *fp = fopen(path.c_str(), "r")) {
            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), fp)) {
                std::string line(buffer);
                while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
                    line.pop_back();
                }
                if (!line.empty() && line[0] != '#') {
                    collectEffectFiles(line, effects);
                }
            }
            fclose(fp);
        }
    }
}

struct CacheEntry {
    std::string path;
    std::string status;
    std::string language;
    std::string line;
    long long size = -1;
    long long mtime = -1;
};

bool
readReport(const std::string &path, std::map<std::string, CacheEntry> &entries)
{
    FILE *fp = path.empty() || path == "-" ? stdin : fopen(path.c_str(), "r");
    if (!fp) {
        return false;
    }
    char buffer[65536];
    while (fgets(buffer, sizeof(buffer), fp)) {
        CacheEntry entry;
        entry.line = buffer;
        /* minimal JSON field extraction without a parser dependency */
        auto extract = [&entry](const char *key, std::string &value) {
            const std::string needle = std::string("\"") + key + "\":";
            size_t offset = entry.line.find(needle);
            if (offset == std::string::npos) {
                return;
            }
            offset += needle.size();
            while (offset < entry.line.size() && entry.line[offset] == ' ') {
                offset++;
            }
            if (offset < entry.line.size() && entry.line[offset] == '"') {
                std::string decoded;
                for (size_t i = offset + 1; i < entry.line.size() && entry.line[i] != '"'; i++) {
                    if (entry.line[i] == '\\' && i + 1 < entry.line.size()) {
                        char escaped = entry.line[++i];
                        switch (escaped) {
                        case 'n':
                            decoded += '\n';
                            break;
                        case 'r':
                            decoded += '\r';
                            break;
                        case 't':
                            decoded += '\t';
                            break;
                        default:
                            decoded += escaped;
                            break;
                        }
                    }
                    else {
                        decoded += entry.line[i];
                    }
                }
                value = decoded;
            }
            else {
                value = entry.line.substr(offset, entry.line.find_first_of(",}", offset) - offset);
            }
        };
        extract("path", entry.path);
        extract("status", entry.status);
        extract("language", entry.language);
        std::string sizeValue, mtimeValue;
        extract("size", sizeValue);
        extract("mtime", mtimeValue);
        entry.size = sizeValue.empty() ? -1 : atoll(sizeValue.c_str());
        entry.mtime = mtimeValue.empty() ? -1 : atoll(mtimeValue.c_str());
        if (!entry.path.empty()) {
            entries[entry.path] = entry;
        }
    }
    if (fp != stdin) {
        fclose(fp);
    }
    return true;
}

void
appendSinkField(FILE *output, const char *key, const std::string &value, bool last)
{
    if (value.empty()) {
        return;
    }
    fputs(",\"", output);
    fputs(key, output);
    fputs("\":", output);
    writeJSONString(output, value.substr(0, 2048));
    if (last) {
        /* nothing */
    }
}

} /* namespace anonymous */

int
main(int argc, char *argv[])
{
    Compiler::LanguageType language = Compiler::kLanguageTypeGLSL;
    std::string outputPath = "-", cachePath, manifestPath;
    bool optimize = false, validate = false, verbose = false;
    std::vector<std::string> inputs;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        auto value = [&argc, &argv, &i]() -> std::string {
            return i + 1 < argc ? argv[++i] : std::string();
        };
        if (arg == "--language") {
            const std::string name = value();
            bool found = false;
            for (int j = 0; j < 5; j++) {
                if (name == kLanguageNames[j]) {
                    language = kLanguageTypes[j];
                    found = true;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "unknown language: %s\n", name.c_str());
                return 1;
            }
        }
        else if (arg == "--output") {
            outputPath = value();
        }
        else if (arg == "--cache") {
            cachePath = value();
        }
        else if (arg == "--manifest") {
            manifestPath = value();
        }
        else if (arg == "--optimize") {
            optimize = true;
        }
        else if (arg == "--no-optimize") {
            optimize = false;
        }
        else if (arg == "--validate") {
            validate = true;
        }
        else if (arg == "--no-validate") {
            validate = false;
        }
        else if (arg == "--verbose") {
            verbose = true;
        }
        else if (!arg.empty() && arg[0] == '-') {
            fprintf(stderr, "unknown option: %s\n", arg.c_str());
            return 1;
        }
        else {
            inputs.push_back(arg);
        }
    }
    if (inputs.empty()) {
        fprintf(stderr, "usage: effect_corpus [--language glsl|essl|hlsl|msl|spirv] [--output report.jsonl]"
                        " [--cache report.jsonl] [--manifest manifest.txt] [--optimize] [--validate] [--verbose]"
                        " <directory-or-list-file>...\n");
        return 1;
    }
    std::vector<std::string> effects;
    for (auto it = inputs.begin(), end = inputs.end(); it != end; ++it) {
        collectEffectFiles(*it, effects);
    }
    std::sort(effects.begin(), effects.end());
    effects.erase(std::unique(effects.begin(), effects.end()), effects.end());

    std::map<std::string, CacheEntry> cache;
    if (!cachePath.empty()) {
        readReport(cachePath, cache);
        fprintf(stderr, "cache: %zu entries from %s\n", cache.size(), cachePath.c_str());
    }
    std::map<std::string, std::string> manifest;
    if (!manifestPath.empty()) {
        if (FILE *fp = fopen(manifestPath.c_str(), "r")) {
            char buffer[8192];
            while (fgets(buffer, sizeof(buffer), fp)) {
                std::string line(buffer);
                while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
                    line.pop_back();
                }
                const size_t offset = line.find(' ');
                if (offset != std::string::npos) {
                    manifest[line.substr(offset + 1)] = line.substr(0, offset);
                }
            }
            fclose(fp);
        }
        else {
            fprintf(stderr, "manifest: cannot open %s\n", manifestPath.c_str());
            return 1;
        }
    }

    FILE *output = outputPath == "-" ? stdout : fopen(outputPath.c_str(), "w");
    if (!output) {
        fprintf(stderr, "cannot open output %s\n", outputPath.c_str());
        return 1;
    }
    const char *languageName = "glsl";
    for (int j = 0; j < 5; j++) {
        if (language == kLanguageTypes[j]) {
            languageName = kLanguageNames[j];
        }
    }

    Compiler::initialize();
    size_t numOK = 0, numFail = 0, numCached = 0, numFragments = 0, numRegressions = 0;
    for (auto it = effects.begin(), end = effects.end(); it != end; ++it) {
        const std::string &path = *it;
        struct stat st;
        const bool exists = stat(path.c_str(), &st) == 0;
        const long long size = exists ? static_cast<long long>(st.st_size) : -1;
        const long long mtime = exists ? static_cast<long long>(st.st_mtime) : -1;
        auto cacheIt = cache.find(path);
        if (exists && cacheIt != cache.end() && cacheIt->second.size == size && cacheIt->second.mtime == mtime &&
            cacheIt->second.language == languageName) {
            fputs(cacheIt->second.line.c_str(), output);
            if (!cacheIt->second.line.empty() && cacheIt->second.line.back() != '\n') {
                fputc('\n', output);
            }
            numCached++;
            numOK += cacheIt->second.status == "ok";
            numFail += cacheIt->second.status == "fail";
            numFragments += cacheIt->second.status == "fragment";
            continue;
        }
        std::string status = "fail", reason, bucket;
        size_t numPasses = 0, numCompiled = 0, numValidated = 0;
        std::string sinkInfo, sinkValidator, sinkBuilder, sinkTranslator, sinkOptimizer;
        {
            Compiler compiler;
            compiler.setTargetLanguage(language);
            compiler.setOptimizeEnabled(optimize);
            compiler.setValidationEnabled(validate);
            compiler.setDefineMacro("NANOEM", "1");
            compiler.setDefineMacro("NANOEM_OUTPUT_SHADER_LANGUAGE_GLSL",
                language == Compiler::kLanguageTypeGLSL ? "1" : "0");
            compiler.setDefineMacro("NANOEM_OUTPUT_SHADER_LANGUAGE_ESSL",
                language == Compiler::kLanguageTypeESSL ? "1" : "0");
            compiler.setDefineMacro(
                "NANOEM_OUTPUT_SHADER_LANGUAGE_HLSL", language == Compiler::kLanguageTypeHLSL ? "1" : "0");
            compiler.setDefineMacro(
                "NANOEM_OUTPUT_SHADER_LANGUAGE_MSL", language == Compiler::kLanguageTypeMSL ? "1" : "0");
            compiler.setDefineMacro(
                "NANOEM_OUTPUT_SHADER_LANGUAGE_SPIRV", language == Compiler::kLanguageTypeSPIRV ? "1" : "0");
            Compiler::EffectProduct product;
            const bool compiled = compiler.compile(path.c_str(), product);
            numPasses = product.numPasses;
            numCompiled = product.numCompiledPasses;
            numValidated = product.numValidatedPasses;
            status = compiled ? "ok" : "fail";
            if (!compiled) {
                if (!product.sink.translator.empty()) {
                    bucket = "translator";
                    reason = *product.sink.translator.begin();
                }
                else if (!product.sink.builder.empty()) {
                    bucket = "builder";
                    reason = firstErrorLine(product.sink.builder);
                }
                else if (!product.sink.optimizer.empty()) {
                    bucket = "optimizer";
                    reason = *product.sink.optimizer.begin();
                }
                else if (!product.sink.validator.empty()) {
                    bucket = "validator";
                    reason = firstErrorLine(product.sink.validator);
                }
                else if (!product.sink.info.empty()) {
                    bucket = "info";
                    reason = firstErrorLine(product.sink.info);
                }
                else {
                    bucket = "unknown";
                    reason = "no diagnostic recorded";
                }
            }
            if (verbose) {
                sinkInfo = product.sink.info;
                sinkValidator = product.sink.validator;
                sinkBuilder = product.sink.builder;
                sinkOptimizer.clear();
                for (auto entry : product.sink.optimizer) {
                    sinkOptimizer += entry;
                    sinkOptimizer += "\n";
                }
                sinkTranslator.clear();
                for (auto entry : product.sink.translator) {
                    sinkTranslator += entry;
                    sinkTranslator += "\n";
                }
            }
        }
        /* .fxsub files are include fragments: failing to compile standalone is expected
           because they reference symbols provided by their includer, so classify them
           separately instead of polluting the failure statistics */
        const bool isFragment = status != "ok" && path.size() > 6 && path.compare(path.size() - 6, 6, ".fxsub") == 0;
        if (isFragment) {
            status = "fragment";
        }
        numOK += status == "ok";
        numFail += status == "fail";
        numFragments += isFragment;
        fputc('{', output);
        fputs("\"path\":", output);
        writeJSONString(output, path);
        fprintf(output, ",\"status\":\"%s\",\"language\":\"%s\",\"numPasses\":%zu,\"numCompiled\":%zu"
                        ",\"numValidated\":%zu,\"size\":%lld,\"mtime\":%lld",
            status.c_str(), languageName, numPasses, numCompiled, numValidated, size, mtime);
        if (!reason.empty()) {
            fputs(",\"bucket\":\"", output);
            fputs(bucket.c_str(), output);
            fputs("\",\"reason\":", output);
            writeJSONString(output, reason);
        }
        appendSinkField(output, "info", sinkInfo, false);
        appendSinkField(output, "validator", sinkValidator, false);
        appendSinkField(output, "builder", sinkBuilder, false);
        appendSinkField(output, "translator", sinkTranslator, false);
        fputs("}\n", output);
        if (!manifestPath.empty()) {
            auto manifestIt = manifest.find(path);
            const std::string expected = manifestIt != manifest.end() ? manifestIt->second : std::string();
            if (expected == "ok" && status != "ok") {
                fprintf(stderr, "REGRESSION: %s (expected ok, got %s: %s)\n", path.c_str(), status.c_str(),
                    reason.c_str());
                numRegressions++;
            }
        }
        fflush(output);
    }
    Compiler::terminate();
    if (output != stdout) {
        fclose(output);
    }
    fprintf(stderr,
        "effect_corpus: %zu effects (%zu ok, %zu fail, %zu fragments, %zu cached) language=%s%s\n", effects.size(),
        numOK, numFail, numFragments, numCached, languageName, numRegressions > 0 ? " - REGRESSIONS DETECTED" : "");
    return numRegressions > 0 ? 2 : 0;
}
