/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Encoding.h"

#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <errno.h>
#include <iconv.h>
#endif

namespace fx9next {
namespace {

#ifdef _WIN32
std::string
sjisToUtf8(const char *bytes, size_t size)
{
    if (size == 0) {
        return std::string();
    }
    int wideCount = MultiByteToWideChar(932, 0, bytes, static_cast<int>(size), nullptr, 0);
    if (wideCount <= 0) {
        return std::string(bytes, size);
    }
    std::wstring wide(static_cast<size_t>(wideCount), L'\0');
    MultiByteToWideChar(932, 0, bytes, static_cast<int>(size), &wide[0], wideCount);
    int utf8Count = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideCount, nullptr, 0, nullptr, nullptr);
    if (utf8Count <= 0) {
        return std::string(bytes, size);
    }
    std::string utf8(static_cast<size_t>(utf8Count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideCount, &utf8[0], utf8Count, nullptr, nullptr);
    return utf8;
}
#else
std::string
sjisToUtf8(const char *bytes, size_t size)
{
    if (size == 0) {
        return std::string();
    }
    iconv_t cd = iconv_open("UTF-8", "CP932");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        cd = iconv_open("UTF-8", "SHIFT_JIS");
    }
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return std::string(bytes, size);
    }
    size_t inLeft = size;
    char *inBuf = const_cast<char *>(bytes);
    std::string out;
    out.resize(size * 4 + 4);
    char *outBuf = &out[0];
    size_t outLeft = out.size();
    size_t result = iconv(cd, &inBuf, &inLeft, &outBuf, &outLeft);
    iconv_close(cd);
    if (result == static_cast<size_t>(-1)) {
        return std::string(bytes, size);
    }
    out.resize(out.size() - outLeft);
    return out;
}
#endif

} /* namespace anonymous */

bool
isLikelyUtf8(const char *bytes, size_t size)
{
    size_t i = 0;
    while (i < size) {
        unsigned char c = static_cast<unsigned char>(bytes[i]);
        if (c <= 0x7F) {
            i++;
            continue;
        }
        int need = 0;
        if ((c & 0xE0) == 0xC0) {
            need = 1;
        }
        else if ((c & 0xF0) == 0xE0) {
            need = 2;
        }
        else if ((c & 0xF8) == 0xF0) {
            need = 3;
        }
        else {
            return false;
        }
        if (i + static_cast<size_t>(need) >= size) {
            return false;
        }
        for (int n = 1; n <= need; n++) {
            unsigned char next = static_cast<unsigned char>(bytes[i + static_cast<size_t>(n)]);
            if ((next & 0xC0) != 0x80) {
                return false;
            }
        }
        i += static_cast<size_t>(need) + 1;
    }
    return true;
}

std::string
decodeTextSource(const void *bytes, size_t size)
{
    const char *data = static_cast<const char *>(bytes);
    if (!data || size == 0) {
        return std::string();
    }
    if (size >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        return std::string(data + 3, size - 3);
    }
    if (isLikelyUtf8(data, size)) {
        return std::string(data, size);
    }
    return sjisToUtf8(data, size);
}

} /* namespace fx9next */
