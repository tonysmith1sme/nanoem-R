/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_DIAGNOSTICS_H_
#define FX9NEXT_DIAGNOSTICS_H_

#include <string>
#include <vector>

namespace fx9next {

enum DiagnosticStage {
    kDiagnosticEncoding,
    kDiagnosticPreprocess,
    kDiagnosticLex,
    kDiagnosticParse,
    kDiagnosticType,
    kDiagnosticEffect,
    kDiagnosticSPIRV,
    kDiagnosticProduct,
    kDiagnosticRuntime
};

enum DiagnosticSeverity {
    kDiagnosticNote,
    kDiagnosticWarning,
    kDiagnosticError
};

struct SourceLocation {
    SourceLocation();
    SourceLocation(const std::string &path, int line, int column);

    std::string path;
    int line;
    int column;
};

struct Diagnostic {
    DiagnosticStage stage;
    DiagnosticSeverity severity;
    std::string code;
    SourceLocation location;
    std::string entity;
    std::string message;
};

class DiagnosticSink {
public:
    void clear();
    void add(DiagnosticStage stage, DiagnosticSeverity severity, const std::string &code,
        const SourceLocation &location, const std::string &entity, const std::string &message);
    bool hasErrors() const;
    const std::vector<Diagnostic> &diagnostics() const;
    std::string format() const;

private:
    std::vector<Diagnostic> m_diagnostics;
};

const char *diagnosticStageName(DiagnosticStage value);
const char *diagnosticSeverityName(DiagnosticSeverity value);

} /* namespace fx9next */

#endif /* FX9NEXT_DIAGNOSTICS_H_ */
