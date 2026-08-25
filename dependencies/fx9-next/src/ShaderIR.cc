/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/ShaderIR.h"

#include <sstream>

namespace fx9next {

std::string
ShaderModuleIR::canonicalDump() const
{
    std::ostringstream stream;
    stream << (stage == kShaderStageVertex ? "vertex" : "pixel") << " " << entryPoint << "\n";
    for (std::vector<ShaderParameterIR>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        stream << "in " << it->type.toString() << " " << it->name << " : " << it->semantic << "\n";
    }
    for (std::vector<ShaderParameterIR>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        stream << "out " << it->type.toString() << " " << it->name << " : " << it->semantic << "\n";
    }
    for (std::vector<ShaderFunctionIR>::const_iterator it = functions.begin(); it != functions.end(); ++it) {
        stream << "fn " << it->returnType.toString() << " " << it->name << "(";
        for (size_t i = 0; i < it->parameters.size(); i++) {
            if (i > 0) {
                stream << ",";
            }
            stream << it->parameters[i].type.toString() << " " << it->parameters[i].name;
        }
        stream << ")\n";
    }
    return stream.str();
}

} /* namespace fx9next */
