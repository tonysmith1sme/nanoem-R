/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

/// Resolve a dx9rt StateVector into sokol pipeline / image descriptions.
///
/// Unlike the legacy incremental conversion (emapp's PipelineDescriptor m_has* flags on
/// top of sokol defaults), the resolver applies the authoritative D3D9 default table and
/// reports an explicit disposition for every state: implemented, approximated, deferred
/// to shader generation, or ignored. Nothing is dropped silently.

#ifndef DX9RT_RESOLVER_H_
#define DX9RT_RESOLVER_H_

#include "dx9rt/StateVector.h"

#include "emapp/Forward.h"

namespace dx9rt {

enum DispositionType {
    kDispositionImplemented = 0, /* maps to a sokol field with D3D9 semantics */
    kDispositionApproximated,    /* nearest equivalent, minor visual divergence possible */
    kDispositionShaderLevel,     /* must be baked into shader generation (alpha test, sRGB, fog) */
    kDispositionRuntimeLevel,    /* applied by the renderer at draw time (scissor) */
    kDispositionIgnored,         /* no modern equivalent, no MME-visible effect */
    kDispositionUnknown,         /* not a D3D9 render state key */
};

/// One diagnostic row collected while resolving.
struct ResolveNote {
    uint32_t key;
    DispositionType disposition;
    const char *note;
};

/// Fixed capacity diagnostics (a pass can touch at most every known state once).
struct ResolveDiagnostics {
    static const int kMaxNotes = 128;

    ResolveNote notes[kMaxNotes];
    int numNotes;

    ResolveDiagnostics()
        : numNotes(0)
    {
    }
    void add(uint32_t key, DispositionType disposition, const char *note)
    {
        if (numNotes < kMaxNotes) {
            notes[numNotes].key = key;
            notes[numNotes].disposition = disposition;
            notes[numNotes].note = note;
            numNotes++;
        }
    }
};

/// States that do not live in sg_pipeline_desc: they are consumed by shader generation
/// (alpha test discard, sRGB encode) or by the renderer at draw time (scissor rect).
struct ResolvedExtraStates {
    bool alphaTestEnabled;
    uint8_t alphaTestReference; /* 0..255, D3D space */
    int alphaTestCompareFunc;   /* dx9rt CompareFuncType */
    bool srgbWriteEnabled;
    bool scissorTestEnabled;

    ResolvedExtraStates();
};

/// Fill a sg_pipeline_desc from the whole state vector (D3D9 defaults included).
/// Base desc fields not owned by D3D9 (shader, layout, colors count, sample count,
/// pixel formats) are left untouched - callers initialize them before resolving.
void resolvePipeline(const StateVector &states, nanoem::sg_pipeline_desc &desc, ResolvedExtraStates &extra,
    ResolveDiagnostics *diagnostics);

/// Fill the sampler-owned fields of a sg_image_desc (old-generation sokol keeps
/// sampler state on the image) for one sampler stage.
void resolveSamplerImage(const StateVector &states, int stage, nanoem::sg_image_desc &desc,
    ResolveDiagnostics *diagnostics);

/// The disposition of a render state key without resolving anything (for tests/docs).
DispositionType renderStateDisposition(uint32_t key);

} /* namespace dx9rt */

#endif /* DX9RT_RESOLVER_H_ */
