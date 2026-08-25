/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Preprocessor.h"

#include "fx9next/Encoding.h"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace fx9next {
namespace {

std::string
trim(const std::string &s)
{
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) {
        b++;
    }
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) {
        e--;
    }
    return s.substr(b, e - b);
}

std::string
dirName(const std::string &path)
{
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return std::string();
    }
    return path.substr(0, slash);
}

std::string
normalizePathSeparators(const std::string &path)
{
    std::string s = path;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\\') {
            s[i] = '/';
        }
    }
    return s;
}

std::string
baseName(const std::string &path)
{
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

std::string
joinPath(const std::string &dir, const std::string &name)
{
    if (dir.empty()) {
        return name;
    }
    char last = dir[dir.size() - 1];
    if (last == '/' || last == '\\') {
        return dir + name;
    }
#ifdef _WIN32
    return dir + "\\" + name;
#else
    return dir + "/" + name;
#endif
}

bool
readFile(const std::string &path, std::string &out)
{
#ifdef _WIN32
    int fd = _open(path.c_str(), _O_RDONLY | _O_BINARY);
#else
    int fd = open(path.c_str(), O_RDONLY);
#endif
    if (fd < 0) {
        return false;
    }
#ifdef _WIN32
    long size = _lseek(fd, 0, SEEK_END);
    _lseek(fd, 0, SEEK_SET);
#else
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
#endif
    if (size < 0) {
#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif
        return false;
    }
    std::string buffer(static_cast<size_t>(size), '\0');
#ifdef _WIN32
    int n = _read(fd, &buffer[0], static_cast<unsigned int>(size));
    _close(fd);
#else
    ssize_t n = read(fd, &buffer[0], static_cast<size_t>(size));
    close(fd);
#endif
    if (n < 0) {
        return false;
    }
    buffer.resize(static_cast<size_t>(n));
    out = decodeTextSource(buffer.data(), buffer.size());
    return true;
}

bool
isIdentChar(unsigned char c)
{
    return std::isalnum(c) || c == '_';
}

} /* namespace anonymous */

void
Preprocessor::setMacro(const std::string &key, const std::string &value)
{
    Macro macro;
    macro.functionLike = false;
    macro.body = value;
    m_macros[key] = macro;
}

void
Preprocessor::removeMacro(const std::string &key)
{
    m_macros.erase(key);
}

bool
Preprocessor::containsMacro(const std::string &key) const
{
    return m_macros.find(key) != m_macros.end();
}

const Preprocessor::MacroMap &
Preprocessor::macros() const
{
    static MacroMap empty;
    return empty;
}

void
Preprocessor::addIncludeSource(const std::string &filePath, const std::string &sourceData)
{
    std::string norm = normalizePathSeparators(filePath);
    m_includes[norm] = sourceData;
    if (norm != filePath) {
        m_includes[filePath] = sourceData;
    }
}

void
Preprocessor::clearIncludeSources()
{
    m_includes.clear();
}

void
Preprocessor::clearIncludedPaths()
{
    m_includedPaths.clear();
}

const Preprocessor::IncludeMap &
Preprocessor::includeSources() const
{
    return m_includes;
}

const std::vector<std::string> &
Preprocessor::includedPaths() const
{
    return m_includedPaths;
}

bool
Preprocessor::process(const std::string &source, const std::string &filename, std::string &output, std::string &error)
{
    m_includedPaths.clear();
    return processInternal(source, filename, 0, output, error);
}

bool
Preprocessor::processInternal(
    const std::string &source, const std::string &filename, int depth, std::string &output, std::string &error)
{
    if (depth > 32) {
        error = "include depth exceeded in " + filename;
        return false;
    }
    std::istringstream in(source);
    std::string line;
    bool emitting = true;
    std::vector<int> ifStack;
    while (std::getline(in, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.resize(line.size() - 1);
        }
        while (!line.empty() && line[line.size() - 1] == '\\') {
            line.resize(line.size() - 1);
            std::string more;
            if (!std::getline(in, more)) {
                break;
            }
            if (!more.empty() && more[more.size() - 1] == '\r') {
                more.resize(more.size() - 1);
            }
            line += more;
        }
        std::string stripped = trim(line);
        size_t comment = std::string::npos;
        bool inStr = false;
        for (size_t ci = 0; ci + 1 < stripped.size(); ci++) {
            if (stripped[ci] == '"') {
                inStr = !inStr;
            }
            if (!inStr && stripped[ci] == '/' && stripped[ci + 1] == '/') {
                comment = ci;
                break;
            }
        }
        if (comment != std::string::npos) {
            stripped = trim(stripped.substr(0, comment));
            line = line.substr(0, line.find("//"));
        }
        if (!stripped.empty() && stripped[0] == '#') {
            if (!handleDirective(stripped, filename, depth, emitting, ifStack, output, error)) {
                return false;
            }
            output.push_back('\n');
            continue;
        }
        if (emitting) {
            output += expand(line, 0);
        }
        output.push_back('\n');
    }
    if (!ifStack.empty()) {
        error = filename + ": unterminated #if";
        return false;
    }
    return true;
}

bool
Preprocessor::handleDirective(const std::string &line, const std::string &filename, int depth, bool &emitting,
    std::vector<int> &ifStack, std::string &output, std::string &error)
{
    std::string body = trim(line.substr(1));
    std::string cmd;
    size_t i = 0;
    while (i < body.size() && isIdentChar(static_cast<unsigned char>(body[i]))) {
        cmd.push_back(body[i]);
        i++;
    }
    std::string rest = trim(body.substr(i));
    if (cmd == "define") {
        if (!emitting) {
            return true;
        }
        size_t p = 0;
        std::string name;
        while (p < rest.size() && isIdentChar(static_cast<unsigned char>(rest[p]))) {
            name.push_back(rest[p]);
            p++;
        }
        if (name.empty()) {
            error = filename + ": invalid #define";
            return false;
        }
        Macro macro;
        macro.functionLike = false;
        if (p < rest.size() && rest[p] == '(') {
            macro.functionLike = true;
            p++;
            std::string param;
            for (; p < rest.size() && rest[p] != ')'; p++) {
                if (rest[p] == ',') {
                    macro.params.push_back(trim(param));
                    param.clear();
                }
                else {
                    param.push_back(rest[p]);
                }
            }
            if (!param.empty() || !macro.params.empty()) {
                std::string t = trim(param);
                if (!t.empty()) {
                    macro.params.push_back(t);
                }
            }
            if (p < rest.size() && rest[p] == ')') {
                p++;
            }
            macro.body = trim(rest.substr(p));
        }
        else {
            macro.body = trim(rest.substr(p));
        }
        m_macros[name] = macro;
        return true;
    }
    if (cmd == "undef") {
        if (emitting) {
            m_macros.erase(trim(rest));
        }
        return true;
    }
    if (cmd == "include") {
        if (!emitting) {
            return true;
        }
        if (rest.size() < 2) {
            error = filename + ": invalid #include";
            return false;
        }
        bool quoted = rest[0] == '"';
        char close = quoted ? '"' : '>';
        if (!quoted && rest[0] != '<') {
            error = filename + ": invalid #include";
            return false;
        }
        size_t end = rest.find(close, 1);
        if (end == std::string::npos) {
            error = filename + ": invalid #include";
            return false;
        }
        std::string header = rest.substr(1, end - 1);
        std::string path, contents;
        if (!loadInclude(header, filename, quoted, path, contents, error)) {
            return false;
        }
        m_includedPaths.push_back(normalizePathSeparators(header));
        std::string included;
        if (!processInternal(contents, path, depth + 1, included, error)) {
            return false;
        }
        output += included;
        return true;
    }
    if (cmd == "ifdef" || cmd == "ifndef") {
        bool present = containsMacro(trim(rest));
        bool cond = cmd == "ifdef" ? present : !present;
        ifStack.push_back(emitting ? (cond ? 1 : 0) : 2);
        emitting = emitting && cond;
        return true;
    }
    if (cmd == "if") {
        bool cond = false;
        if (emitting) {
            if (!evalIf(rest, cond, error)) {
                return false;
            }
        }
        ifStack.push_back(emitting ? (cond ? 1 : 0) : 2);
        emitting = emitting && cond;
        return true;
    }
    if (cmd == "elif") {
        if (ifStack.empty()) {
            error = filename + ": #elif without #if";
            return false;
        }
        int state = ifStack.back();
        if (state == 2) {
            emitting = false;
            return true;
        }
        if (state == 1) {
            ifStack.back() = 3;
            emitting = false;
            return true;
        }
        if (state == 3) {
            emitting = false;
            return true;
        }
        bool cond = false;
        if (!evalIf(rest, cond, error)) {
            return false;
        }
        ifStack.back() = cond ? 1 : 0;
        emitting = cond;
        return true;
    }
    if (cmd == "else") {
        if (ifStack.empty()) {
            error = filename + ": #else without #if";
            return false;
        }
        int state = ifStack.back();
        if (state == 2 || state == 3) {
            emitting = false;
        }
        else {
            emitting = state == 0;
            ifStack.back() = emitting ? 1 : 3;
        }
        return true;
    }
    if (cmd == "endif") {
        if (ifStack.empty()) {
            error = filename + ": #endif without #if";
            return false;
        }
        ifStack.pop_back();
        emitting = true;
        for (size_t n = 0; n < ifStack.size(); n++) {
            if (ifStack[n] != 1) {
                emitting = false;
                break;
            }
        }
        return true;
    }
    if (cmd == "pragma" || cmd == "line" || cmd == "error" || cmd == "warning") {
        return true;
    }
    return true;
}

bool
Preprocessor::evalIf(const std::string &expr, bool &result, std::string &error) const
{
    std::string e = expand(expr, 0);
    std::string replaced;
    size_t i = 0;
    while (i < e.size()) {
        if (e.compare(i, 8, "defined(") == 0 || (e.compare(i, 7, "defined") == 0 && i + 7 < e.size() &&
                                                    std::isspace(static_cast<unsigned char>(e[i + 7])))) {
            size_t p = i + 7;
            while (p < e.size() && std::isspace(static_cast<unsigned char>(e[p]))) {
                p++;
            }
            bool paren = p < e.size() && e[p] == '(';
            if (paren) {
                p++;
            }
            while (p < e.size() && std::isspace(static_cast<unsigned char>(e[p]))) {
                p++;
            }
            std::string name;
            while (p < e.size() && isIdentChar(static_cast<unsigned char>(e[p]))) {
                name.push_back(e[p]);
                p++;
            }
            while (p < e.size() && std::isspace(static_cast<unsigned char>(e[p]))) {
                p++;
            }
            if (paren && p < e.size() && e[p] == ')') {
                p++;
            }
            replaced += containsMacro(name) ? "1" : "0";
            i = p;
            continue;
        }
        replaced.push_back(e[i]);
        i++;
    }
    std::string tokens;
    for (size_t n = 0; n < replaced.size(); n++) {
        unsigned char c = static_cast<unsigned char>(replaced[n]);
        if (isIdentChar(c) && !std::isdigit(c)) {
            size_t s = n;
            while (n < replaced.size() && isIdentChar(static_cast<unsigned char>(replaced[n]))) {
                n++;
            }
            std::string ident = replaced.substr(s, n - s);
            if (m_macros.find(ident) == m_macros.end()) {
                tokens += "0";
            }
            else {
                tokens += expand(ident, 0);
            }
            n--;
        }
        else {
            tokens.push_back(replaced[n]);
        }
    }
    long value = 0;
    std::string expr2;
    for (size_t n = 0; n < tokens.size(); n++) {
        expr2.push_back(tokens[n]);
    }
    const char *p = expr2.c_str();
    struct Parser {
        const char *p;
        long parseOr()
        {
            long l = parseAnd();
            while (*p) {
                while (std::isspace(static_cast<unsigned char>(*p))) {
                    p++;
                }
                if (p[0] == '|' && p[1] == '|') {
                    p += 2;
                    long r = parseAnd();
                    l = (l || r) ? 1 : 0;
                }
                else {
                    break;
                }
            }
            return l;
        }
        long parseAnd()
        {
            long l = parseEq();
            while (*p) {
                while (std::isspace(static_cast<unsigned char>(*p))) {
                    p++;
                }
                if (p[0] == '&' && p[1] == '&') {
                    p += 2;
                    long r = parseEq();
                    l = (l && r) ? 1 : 0;
                }
                else {
                    break;
                }
            }
            return l;
        }
        long parseEq()
        {
            long l = parseRel();
            while (*p) {
                while (std::isspace(static_cast<unsigned char>(*p))) {
                    p++;
                }
                if (p[0] == '=' && p[1] == '=') {
                    p += 2;
                    l = (l == parseRel()) ? 1 : 0;
                }
                else if (p[0] == '!' && p[1] == '=') {
                    p += 2;
                    l = (l != parseRel()) ? 1 : 0;
                }
                else {
                    break;
                }
            }
            return l;
        }
        long parseRel()
        {
            long l = parseAdd();
            while (*p) {
                while (std::isspace(static_cast<unsigned char>(*p))) {
                    p++;
                }
                if (p[0] == '<' && p[1] == '=') {
                    p += 2;
                    l = (l <= parseAdd()) ? 1 : 0;
                }
                else if (p[0] == '>' && p[1] == '=') {
                    p += 2;
                    l = (l >= parseAdd()) ? 1 : 0;
                }
                else if (p[0] == '<') {
                    p++;
                    l = (l < parseAdd()) ? 1 : 0;
                }
                else if (p[0] == '>') {
                    p++;
                    l = (l > parseAdd()) ? 1 : 0;
                }
                else {
                    break;
                }
            }
            return l;
        }
        long parseAdd()
        {
            long l = parseMul();
            while (*p) {
                while (std::isspace(static_cast<unsigned char>(*p))) {
                    p++;
                }
                if (*p == '+') {
                    p++;
                    l += parseMul();
                }
                else if (*p == '-') {
                    p++;
                    l -= parseMul();
                }
                else {
                    break;
                }
            }
            return l;
        }
        long parseMul()
        {
            long l = parseUnary();
            while (*p) {
                while (std::isspace(static_cast<unsigned char>(*p))) {
                    p++;
                }
                if (*p == '*') {
                    p++;
                    l *= parseUnary();
                }
                else if (*p == '/') {
                    p++;
                    long r = parseUnary();
                    l = r ? l / r : 0;
                }
                else if (*p == '%') {
                    p++;
                    long r = parseUnary();
                    l = r ? l % r : 0;
                }
                else {
                    break;
                }
            }
            return l;
        }
        long parseUnary()
        {
            while (std::isspace(static_cast<unsigned char>(*p))) {
                p++;
            }
            if (*p == '!') {
                p++;
                return parseUnary() ? 0 : 1;
            }
            if (*p == '-') {
                p++;
                return -parseUnary();
            }
            if (*p == '+') {
                p++;
                return parseUnary();
            }
            if (*p == '(') {
                p++;
                long v = parseOr();
                while (std::isspace(static_cast<unsigned char>(*p))) {
                    p++;
                }
                if (*p == ')') {
                    p++;
                }
                return v;
            }
            if (std::isdigit(static_cast<unsigned char>(*p))) {
                char *end = nullptr;
                long v = std::strtol(p, &end, 0);
                p = end;
                return v;
            }
            while (*p && !std::isspace(static_cast<unsigned char>(*p)) && *p != ')' && *p != '&' && *p != '|') {
                p++;
            }
            return 0;
        }
    } parser;
    parser.p = p;
    value = parser.parseOr();
    result = value != 0;
    (void) error;
    return true;
}

std::string
Preprocessor::expand(const std::string &text, int depth) const
{
    if (depth > 32) {
        return text;
    }
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (isIdentChar(c) && !std::isdigit(c)) {
            size_t s = i;
            while (i < text.size() && isIdentChar(static_cast<unsigned char>(text[i]))) {
                i++;
            }
            std::string ident = text.substr(s, i - s);
            auto it = m_macros.find(ident);
            if (it == m_macros.end()) {
                out += ident;
                continue;
            }
            const Macro &macro = it->second;
            if (!macro.functionLike) {
                std::string body = macro.body;
                size_t paste = body.find("##");
                while (paste != std::string::npos) {
                    body.erase(paste, 2);
                    paste = body.find("##");
                }
                out += expand(body, depth + 1);
                continue;
            }
            size_t j = i;
            while (j < text.size() && std::isspace(static_cast<unsigned char>(text[j]))) {
                j++;
            }
            if (j >= text.size() || text[j] != '(') {
                out += ident;
                continue;
            }
            j++;
            std::vector<std::string> args;
            std::string arg;
            int paren = 1;
            for (; j < text.size() && paren > 0; j++) {
                if (text[j] == '(') {
                    paren++;
                    arg.push_back(text[j]);
                }
                else if (text[j] == ')') {
                    paren--;
                    if (paren > 0) {
                        arg.push_back(text[j]);
                    }
                }
                else if (text[j] == ',' && paren == 1) {
                    args.push_back(trim(arg));
                    arg.clear();
                }
                else {
                    arg.push_back(text[j]);
                }
            }
            if (!arg.empty() || !args.empty()) {
                args.push_back(trim(arg));
            }
            i = j;
            std::string body = macro.body;
            std::string replaced;
            size_t b = 0;
            while (b < body.size()) {
                if (b + 1 < body.size() && body[b] == '#' && body[b + 1] != '#') {
                    b++;
                    std::string pname;
                    while (b < body.size() && isIdentChar(static_cast<unsigned char>(body[b]))) {
                        pname.push_back(body[b]);
                        b++;
                    }
                    for (size_t n = 0; n < macro.params.size() && n < args.size(); n++) {
                        if (macro.params[n] == pname) {
                            replaced.push_back('"');
                            replaced += args[n];
                            replaced.push_back('"');
                            pname.clear();
                            break;
                        }
                    }
                    replaced += pname;
                    continue;
                }
                if (isIdentChar(static_cast<unsigned char>(body[b])) && !std::isdigit(static_cast<unsigned char>(body[b]))) {
                    size_t s2 = b;
                    while (b < body.size() && isIdentChar(static_cast<unsigned char>(body[b]))) {
                        b++;
                    }
                    std::string pname = body.substr(s2, b - s2);
                    bool found = false;
                    for (size_t n = 0; n < macro.params.size() && n < args.size(); n++) {
                        if (macro.params[n] == pname) {
                            replaced += args[n];
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        replaced += pname;
                    }
                    continue;
                }
                if (b + 1 < body.size() && body[b] == '#' && body[b + 1] == '#') {
                    b += 2;
                    continue;
                }
                replaced.push_back(body[b]);
                b++;
            }
            out += expand(replaced, depth + 1);
            continue;
        }
        out.push_back(text[i]);
        i++;
    }
    return out;
}

bool
Preprocessor::loadInclude(const std::string &header, const std::string &fromFile, bool quoted,
    std::string &resolvedPath, std::string &contents, std::string &error) const
{
    const std::string normHeader = normalizePathSeparators(header);
    std::vector<std::string> candidates;
    if (quoted && !fromFile.empty()) {
        candidates.push_back(joinPath(dirName(fromFile), normHeader));
    }
    candidates.push_back(normHeader);
    candidates.push_back(header);
    for (size_t c = 0; c < candidates.size(); c++) {
        const std::string &target = candidates[c];
        for (auto it = m_includes.begin(); it != m_includes.end(); ++it) {
            std::string incKey = normalizePathSeparators(it->first);
            if (incKey == target || it->first == target) {
                resolvedPath = it->first;
                contents = it->second;
                return true;
            }
        }
    }
    for (size_t c = 0; c < candidates.size(); c++) {
        const std::string &target = candidates[c];
        for (auto it = m_includes.begin(); it != m_includes.end(); ++it) {
            std::string incKey = normalizePathSeparators(it->first);
            if (incKey.size() >= target.size() &&
                incKey.compare(incKey.size() - target.size(), target.size(), target) == 0 &&
                (incKey.size() == target.size() || incKey[incKey.size() - target.size() - 1] == '/')) {
                resolvedPath = it->first;
                contents = it->second;
                return true;
            }
        }
    }
    std::string baseHeader = baseName(normHeader);
    for (auto it = m_includes.begin(); it != m_includes.end(); ++it) {
        std::string incKey = normalizePathSeparators(it->first);
        if (baseName(incKey) == baseHeader) {
            resolvedPath = it->first;
            contents = it->second;
            return true;
        }
    }
    for (size_t i = 0; i < candidates.size(); i++) {
        if (readFile(candidates[i], contents)) {
            resolvedPath = candidates[i];
            return true;
        }
    }
    error = fromFile + ": cannot open include '" + header + "'";
    return false;
}

} /* namespace fx9next */
