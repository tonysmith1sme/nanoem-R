/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Diagnostics.h"

#include <sstream>

namespace fx9next {

SourceLocation::SourceLocation()
    : line(0)
    , column(0)
{
}

SourceLocation::SourceLocation(const std::string &path_, int line_, int column_)
    : path(path_)
    , line(line_)
    , column(column_)
{
}

void
DiagnosticSink::clear()
{
    m_diagnostics.clear();
}

void
DiagnosticSink::add(DiagnosticStage stage, DiagnosticSeverity severity, const std::string &code,
    const SourceLocation &location, const std::string &entity, const std::string &message)
{
    Diagnostic value;
    value.stage = stage;
    value.severity = severity;
    value.code = code;
    value.location = location;
    value.entity = entity;
    value.message = message;
    m_diagnostics.push_back(value);
}

bool
DiagnosticSink::hasErrors() const
{
    for (std::vector<Diagnostic>::const_iterator it = m_diagnostics.begin(); it != m_diagnostics.end(); ++it) {
        if (it->severity == kDiagnosticError) {
            return true;
        }
    }
    return false;
}

const std::vector<Diagnostic> &
DiagnosticSink::diagnostics() const
{
    return m_diagnostics;
}

std::string
DiagnosticSink::format() const
{
    std::ostringstream stream;
    for (std::vector<Diagnostic>::const_iterator it = m_diagnostics.begin(); it != m_diagnostics.end(); ++it) {
        stream << diagnosticSeverityName(it->severity) << " " << diagnosticStageName(it->stage) << " " << it->code;
        if (!it->location.path.empty()) {
            stream << " " << it->location.path << ":" << it->location.line << ":" << it->location.column;
        }
        if (!it->entity.empty()) {
            stream << " [" << it->entity << "]";
        }
        stream << ": " << it->message << "\n";
    }
    return stream.str();
}

const char *
diagnosticStageName(DiagnosticStage value)
{
    switch (value) {
    case kDiagnosticEncoding: return "encoding";
    case kDiagnosticPreprocess: return "preprocess";
    case kDiagnosticLex: return "lex";
    case kDiagnosticParse: return "parse";
    case kDiagnosticType: return "type";
    case kDiagnosticEffect: return "effect";
    case kDiagnosticSPIRV: return "spirv";
    case kDiagnosticProduct: return "product";
    case kDiagnosticRuntime: return "runtime";
    default: return "unknown";
    }
}

const char *
diagnosticSeverityName(DiagnosticSeverity value)
{
    switch (value) {
    case kDiagnosticNote: return "note";
    case kDiagnosticWarning: return "warning";
    case kDiagnosticError: return "error";
    default: return "unknown";
    }
}

} /* namespace fx9next */
