/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#ifndef NANOEM_EMAPP_INTERNAL_AAPASS_H_
#define NANOEM_EMAPP_INTERNAL_AAPASS_H_

#include "emapp/internal/BasePass.h"

namespace nanoem {
namespace internal {

/* Fullscreen FXAA pass driven by the MSAA resolve chain: samples the 1x resolved
   viewport image and writes the anti-aliased result into the destination pass. */
class AAPass NANOEM_DECL_SEALED : public BasePass {
public:
    AAPass(Project *project);
    ~AAPass() NANOEM_DECL_NOEXCEPT;

    void draw(sg::PassBlock::IDrawQueue *drawQueue, sg_pass dest, sg_image source, const Vector2UI16 &sourceSize,
        const PixelFormat &format);

private:
    void setupVertexBuffer();
    void setupShaderDescription(sg_shader_desc &desc) NANOEM_DECL_OVERRIDE;
    void setupPipelineDescription(sg_pipeline_desc &desc) NANOEM_DECL_OVERRIDE;
    const char *name() const NANOEM_DECL_NOEXCEPT_OVERRIDE;

    sg::QuadVertexUnit m_vertices[4];
};

} /* namespace internal */
} /* namespace nanoem */

#endif /* NANOEM_EMAPP_INTERNAL_AAPASS_H_ */
