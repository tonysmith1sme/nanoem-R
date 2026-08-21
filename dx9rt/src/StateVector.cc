/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "dx9rt/StateVector.h"

#include <string.h>

namespace dx9rt {

StateVector::StateVector()
{
    resetToDefaults();
}

void
StateVector::resetToDefaults()
{
    for (int i = 0; i < kRenderStateIndexMaxEnum; i++) {
        m_renderStateValues[i] = renderStateDefaultValue(i);
    }
    for (size_t i = 0; i < sizeof(m_renderStateExplicitMasks) / sizeof(m_renderStateExplicitMasks[0]); i++) {
        m_renderStateExplicitMasks[i] = 0;
    }
    for (int s = 0; s < kMaxSamplerCount; s++) {
        for (int i = 0; i < kSamplerStateIndexMaxEnum; i++) {
            m_samplerStateValues[s][i] = samplerStateDefaultValue(i);
        }
        for (size_t i = 0; i < sizeof(m_samplerStateExplicitMasks[0]) / sizeof(m_samplerStateExplicitMasks[0][0]); i++) {
            m_samplerStateExplicitMasks[s][i] = 0;
        }
    }
}

bool
StateVector::setRenderState(uint32_t key, uint32_t value)
{
    const int index = renderStateIndexFromKey(key);
    if (index < 0) {
        return false;
    }
    m_renderStateValues[index] = value;
    m_renderStateExplicitMasks[index >> 5] |= 1u << (index & 31);
    return true;
}

bool
StateVector::setSamplerState(int samplerIndex, uint32_t key, uint32_t value)
{
    if (samplerIndex < 0 || samplerIndex >= kMaxSamplerCount) {
        return false;
    }
    const int index = samplerStateIndexFromKey(key);
    if (index < 0) {
        return false;
    }
    m_samplerStateValues[samplerIndex][index] = value;
    m_samplerStateExplicitMasks[samplerIndex][index >> 5] |= 1u << (index & 31);
    return true;
}

uint32_t
StateVector::renderState(uint32_t key) const
{
    const int index = renderStateIndexFromKey(key);
    return index >= 0 ? m_renderStateValues[index] : 0;
}

uint32_t
StateVector::renderStateByIndex(int index) const
{
    return index >= 0 && index < kRenderStateIndexMaxEnum ? m_renderStateValues[index] : 0;
}

bool
StateVector::isRenderStateExplicit(uint32_t key) const
{
    const int index = renderStateIndexFromKey(key);
    return index >= 0 && (m_renderStateExplicitMasks[index >> 5] & (1u << (index & 31))) != 0;
}

uint32_t
StateVector::samplerState(int samplerIndex, uint32_t key) const
{
    if (samplerIndex < 0 || samplerIndex >= kMaxSamplerCount) {
        return 0;
    }
    const int index = samplerStateIndexFromKey(key);
    return index >= 0 ? m_samplerStateValues[samplerIndex][index] : 0;
}

bool
StateVector::isSamplerStateExplicit(int samplerIndex, uint32_t key) const
{
    if (samplerIndex < 0 || samplerIndex >= kMaxSamplerCount) {
        return false;
    }
    const int index = samplerStateIndexFromKey(key);
    return index >= 0 && (m_samplerStateExplicitMasks[samplerIndex][index >> 5] & (1u << (index & 31))) != 0;
}

uint32_t
StateVector::murmur2a(const uint32_t *data, size_t size, uint32_t seed)
{
    /* MurmurHash2A over 32-bit words, mirroring the pipeline cache key convention */
    const uint32_t m = 0x5bd1e995;
    uint32_t h = seed ^ uint32_t(size * 4);
    for (size_t i = 0; i < size; i++) {
        uint32_t k = data[i];
        k *= m;
        k ^= k >> 24;
        k *= m;
        h *= m;
        h ^= k;
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

uint32_t
StateVector::hash() const
{
    uint32_t value = murmur2a(m_renderStateValues, kRenderStateIndexMaxEnum, 0x9747b28c);
    value = murmur2a(m_renderStateExplicitMasks, sizeof(m_renderStateExplicitMasks) / sizeof(uint32_t), value);
    value = murmur2a(&m_samplerStateValues[0][0],
        kMaxSamplerCount * kSamplerStateIndexMaxEnum, value);
    value = murmur2a(&m_samplerStateExplicitMasks[0][0],
        kMaxSamplerCount * ((kSamplerStateIndexMaxEnum + 31) / 32), value);
    return value;
}

bool
StateVector::operator==(const StateVector &other) const
{
    return memcmp(m_renderStateValues, other.m_renderStateValues, sizeof(m_renderStateValues)) == 0 &&
        memcmp(m_renderStateExplicitMasks, other.m_renderStateExplicitMasks, sizeof(m_renderStateExplicitMasks)) == 0 &&
        memcmp(m_samplerStateValues, other.m_samplerStateValues, sizeof(m_samplerStateValues)) == 0 &&
        memcmp(m_samplerStateExplicitMasks, other.m_samplerStateExplicitMasks,
            sizeof(m_samplerStateExplicitMasks)) == 0;
}

bool
StateVector::operator!=(const StateVector &other) const
{
    return !(*this == other);
}

} /* namespace dx9rt */
