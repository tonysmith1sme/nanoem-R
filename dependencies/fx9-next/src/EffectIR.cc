/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/EffectIR.h"

#include <sstream>

namespace fx9next {

std::string
EffectModuleIR::canonicalDump() const
{
    std::ostringstream stream;
    for (std::vector<EffectResourceIR>::const_iterator it = resources.begin(); it != resources.end(); ++it) {
        stream << "resource " << it->type << " " << it->name << " " << it->format << " " << it->mipLevels << " "
               << (it->shared ? "shared" : "local") << "\n";
    }
    for (std::vector<EffectBindingIR>::const_iterator it = bindings.begin(); it != bindings.end(); ++it) {
        stream << "binding " << it->registerSet << " " << it->registerIndex << " " << it->registerCount << " "
               << it->type.toString() << " " << it->name << "\n";
    }
    for (std::vector<EffectTechniqueIR>::const_iterator technique = techniques.begin(); technique != techniques.end();
         ++technique) {
        stream << "technique " << technique->name << "\n";
        for (std::vector<EffectScriptCommandIR>::const_iterator command = technique->script.begin();
             command != technique->script.end(); ++command) {
            stream << "script " << command->type << " " << command->name << " " << command->value << " "
                   << command->index << "\n";
        }
        for (std::vector<EffectPassIR>::const_iterator pass = technique->passes.begin(); pass != technique->passes.end();
             ++pass) {
            stream << "pass " << pass->name << "\n";
            for (std::vector<std::pair<std::string, std::string> >::const_iterator state = pass->renderStates.begin();
                 state != pass->renderStates.end(); ++state) {
                stream << "state " << state->first << "=" << state->second << "\n";
            }
        }
    }
    return stream.str();
}

} /* namespace fx9next */
