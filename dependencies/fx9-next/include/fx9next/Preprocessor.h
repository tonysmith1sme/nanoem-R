/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_PREPROCESSOR_H_
#define FX9NEXT_PREPROCESSOR_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace fx9next {

class Preprocessor {
public:
    typedef std::unordered_map<std::string, std::string> MacroMap;
    typedef std::unordered_map<std::string, std::string> IncludeMap;

    void setMacro(const std::string &key, const std::string &value);
    void removeMacro(const std::string &key);
    bool containsMacro(const std::string &key) const;
    const MacroMap &macros() const;

    void addIncludeSource(const std::string &filePath, const std::string &sourceData);
    const IncludeMap &includeSources() const;
    const std::vector<std::string> &includedPaths() const;

    bool process(const std::string &source, const std::string &filename, std::string &output, std::string &error);

private:
    struct Macro {
        std::vector<std::string> params;
        std::string body;
        bool functionLike;
    };

    bool processInternal(const std::string &source, const std::string &filename, int depth, std::string &output,
        std::string &error);
    bool handleDirective(const std::string &line, const std::string &filename, int depth, bool &emitting,
        std::vector<int> &ifStack, std::string &output, std::string &error);
    bool evalIf(const std::string &expr, bool &result, std::string &error) const;
    std::string expand(const std::string &text, int depth) const;
    bool loadInclude(const std::string &header, const std::string &fromFile, bool quoted, std::string &resolvedPath,
        std::string &contents, std::string &error) const;

    std::unordered_map<std::string, Macro> m_macros;
    IncludeMap m_includes;
    std::vector<std::string> m_includedPaths;
};

} /* namespace fx9next */

#endif /* FX9NEXT_PREPROCESSOR_H_ */
