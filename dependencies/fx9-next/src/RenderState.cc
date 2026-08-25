/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/RenderState.h"

#include <cctype>
#include <cstdlib>
#include <unordered_map>

namespace fx9next {
namespace {

std::string
upper(const std::string &s)
{
    std::string o(s);
    for (size_t i = 0; i < o.size(); i++) {
        o[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(o[i])));
    }
    return o;
}

const std::unordered_map<std::string, uint32_t> &
stateKeys()
{
    static const std::unordered_map<std::string, uint32_t> k = { { "ZENABLE", 7 }, { "FILLMODE", 8 }, { "SHADEMODE", 9 },
        { "ZWRITEENABLE", 14 }, { "ALPHATESTENABLE", 15 }, { "LASTPIXEL", 16 }, { "SRCBLEND", 19 }, { "DESTBLEND", 20 },
        { "CULLMODE", 22 }, { "ZFUNC", 23 }, { "ALPHAREF", 24 }, { "ALPHAFUNC", 25 }, { "DITHERENABLE", 26 },
        { "ALPHABLENDENABLE", 27 }, { "FOGENABLE", 28 }, { "SPECULARENABLE", 29 }, { "STENCILENABLE", 52 },
        { "STENCILFAIL", 53 }, { "STENCILZFAIL", 54 }, { "STENCILPASS", 55 }, { "STENCILFUNC", 56 }, { "STENCILREF", 57 },
        { "STENCILMASK", 58 }, { "STENCILWRITEMASK", 59 }, { "COLORWRITEENABLE", 168 }, { "BLENDOP", 171 },
        { "SCISSORTESTENABLE", 174 }, { "TWOSIDEDSTENCILMODE", 185 }, { "CCW_STENCILFAIL", 186 },
        { "CCW_STENCILZFAIL", 187 }, { "CCW_STENCILPASS", 188 }, { "CCW_STENCILFUNC", 189 }, { "SRGBWRITEENABLE", 194 },
        { "SEPARATEALPHABLENDENABLE", 206 }, { "SRCBLENDALPHA", 207 }, { "DESTBLENDALPHA", 208 },
        { "BLENDOPALPHA", 209 } };
    return k;
}

} /* namespace anonymous */

bool
lookupRenderStateKey(const std::string &name, uint32_t &key)
{
    auto it = stateKeys().find(upper(name));
    if (it == stateKeys().end()) {
        return false;
    }
    key = it->second;
    return true;
}

bool
lookupRenderStateValue(const std::string & /*stateName*/, const std::string &value, uint32_t &out)
{
    const std::string u = upper(value);
    if (u == "TRUE" || u == "ENABLE") {
        out = 1;
        return true;
    }
    if (u == "FALSE" || u == "DISABLE" || u == "NONE") {
        out = 0;
        return true;
    }
    static const std::unordered_map<std::string, uint32_t> values = { { "ZERO", 1 }, { "ONE", 2 }, { "SRCCOLOR", 3 },
        { "INVSRCCOLOR", 4 }, { "SRCALPHA", 5 }, { "INVSRCALPHA", 6 }, { "DESTALPHA", 7 }, { "INVDESTALPHA", 8 },
        { "DESTCOLOR", 9 }, { "INVDESTCOLOR", 10 }, { "SRCALPHASAT", 11 }, { "BOTHSRCALPHA", 12 },
        { "BOTHINVSRCALPHA", 13 }, { "BLENDFACTOR", 14 }, { "INVBLENDFACTOR", 15 }, { "ADD", 1 }, { "SUBTRACT", 2 },
        { "REVSUBTRACT", 3 }, { "MIN", 4 }, { "MAX", 5 }, { "NEVER", 1 }, { "LESS", 2 }, { "EQUAL", 3 },
        { "LESSEQUAL", 4 }, { "GREATER", 5 }, { "NOTEQUAL", 6 }, { "GREATEREQUAL", 7 }, { "ALWAYS", 8 }, { "KEEP", 1 },
        { "REPLACE", 3 }, { "INCRSAT", 4 }, { "DECRSAT", 5 }, { "INVERT", 6 }, { "INCR", 7 }, { "DECR", 8 },
        { "CW", 2 }, { "CCW", 3 }, { "SOLID", 3 }, { "WIREFRAME", 2 }, { "POINT", 1 }, { "FLAT", 1 }, { "GOURAUD", 2 } };
    auto it = values.find(u);
    if (it != values.end()) {
        out = it->second;
        return true;
    }
    char *end = nullptr;
    unsigned long parsed = std::strtoul(value.c_str(), &end, 0);
    if (end != value.c_str()) {
        out = static_cast<uint32_t>(parsed);
        return true;
    }
    return false;
}

} /* namespace fx9next */
