/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

/// Immutable snapshot of the full D3D9 state vector: every render state, every sampler
/// state of 16 stages, and which of them were set explicitly (vs. left at the D3D9
/// default). The vector is hashable so pipelines can be cached per unique state.

#ifndef DX9RT_STATEVECTOR_H_
#define DX9RT_STATEVECTOR_H_

#include "dx9rt/Types.h"

#include <stddef.h>

namespace dx9rt {

class StateVector {
public:
    StateVector();

    /// Reset every state to its D3D9 documented default and clear all explicit flags.
    void resetToDefaults();

    /// Apply one render state; returns false when the key is unknown (caller decides
    /// whether to warn - dx9rt never drops states silently).
    bool setRenderState(uint32_t key, uint32_t value);
    /// Apply one sampler state of stage [0, 16); returns false when key or stage is invalid.
    bool setSamplerState(int samplerIndex, uint32_t key, uint32_t value);

    uint32_t renderState(uint32_t key) const;
    uint32_t renderStateByIndex(int index) const;
    /// Whether setRenderState was called for this state since reset (not whether the
    /// value differs from the default - explicit == is authoritative in D3D).
    bool isRenderStateExplicit(uint32_t key) const;

    uint32_t samplerState(int samplerIndex, uint32_t key) const;
    bool isSamplerStateExplicit(int samplerIndex, uint32_t key) const;

    /// 32-bit hash over the whole vector (values and explicit flags).
    uint32_t hash() const;

    bool operator==(const StateVector &other) const;
    bool operator!=(const StateVector &other) const;

private:
    static uint32_t murmur2a(const uint32_t *data, size_t size, uint32_t seed);

    uint32_t m_renderStateValues[kRenderStateIndexMaxEnum];
    uint32_t m_renderStateExplicitMasks[(kRenderStateIndexMaxEnum + 31) / 32];
    uint32_t m_samplerStateValues[kMaxSamplerCount][kSamplerStateIndexMaxEnum];
    uint32_t m_samplerStateExplicitMasks[kMaxSamplerCount][(kSamplerStateIndexMaxEnum + 31) / 32];
};

} /* namespace dx9rt */

#endif /* DX9RT_STATEVECTOR_H_ */
