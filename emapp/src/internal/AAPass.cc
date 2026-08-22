/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "emapp/internal/AAPass.h"

#include "emapp/Project.h"

#include "emapp/private/CommonInclude.h"

namespace nanoem {
namespace internal {
namespace {
#include "emapp/private/shaders/fxaa_fs_glsl_core33.h"
#include "emapp/private/shaders/fxaa_fs_glsl_es3.h"
#include "emapp/private/shaders/fxaa_fs_msl_macos.h"
#include "emapp/private/shaders/fxaa_fs_spirv.h"
#if defined(_WIN32)
#include "emapp/private/shaders/fxaa_ps_dxbc.h"
#include "emapp/private/shaders/blit_vs_dxbc.h"
#endif /* _WIN32 */
#include "emapp/private/shaders/blit_vs_glsl_core33.h"
#include "emapp/private/shaders/blit_vs_glsl_es3.h"
#include "emapp/private/shaders/blit_vs_msl_macos.h"
#include "emapp/private/shaders/blit_vs_spirv.h"
} /* namespace anonymous */

AAPass::AAPass(Project *project)
    : BasePass(project)
{
    m_vertices[0].m_texcoord = Vector4(0, 0, 0, 0);
    m_vertices[1].m_texcoord = Vector4(1, 0, 0, 0);
    m_vertices[2].m_texcoord = Vector4(0, 1, 0, 0);
    m_vertices[3].m_texcoord = Vector4(1, 1, 0, 0);
}

AAPass::~AAPass() NANOEM_DECL_NOEXCEPT
{
}

void
AAPass::draw(sg::PassBlock::IDrawQueue *drawQueue, sg_pass dest, sg_image source, const Vector2UI16 &sourceSize,
    const PixelFormat &format)
{
    nanoem_parameter_assert(
        sg::query_image_state(source) == SG_RESOURCESTATE_VALID, "source image must NOT be null");
    nanoem_parameter_assert(sg::query_pass_state(dest) == SG_RESOURCESTATE_VALID, "destination pass must be valid");
    SG_INSERT_MARKERF("AAPass::draw(destHandle=%d, sourceHandle=%d, width=%d, height=%d)", dest.id, source.id,
        sourceSize.x, sourceSize.y);
    setupVertexBuffer();
    sg_pipeline pipeline = { SG_INVALID_ID };
    setupPipeline(format, pipeline);
    sg_pass_action pa;
    Inline::clearZeroMemory(pa);
    pa.colors[0].action = pa.depth.action = pa.stencil.action = SG_ACTION_LOAD;
    sg_bindings bindings(m_bindings);
    bindings.fs_images[0] = source;
    sg::PassBlock pb(drawQueue, dest, pa);
    const Vector4 texelSize(1.0f / sourceSize.x, 1.0f / sourceSize.y, sourceSize.x, sourceSize.y);
    pb.applyUniformBlock(SG_SHADERSTAGE_FS, &texelSize, sizeof(texelSize));
    pb.applyPipelineBindings(pipeline, bindings);
    pb.draw(0, 4);
}

void
AAPass::setupVertexBuffer()
{
    sg_buffer &vb = m_bindings.vertex_buffers[0];
    const nanoem_f32_t px = -1, py = sg::query_features().origin_top_left ? 1 : -1, pw = 2.0f, ph = -2.0f;
    m_vertices[0].m_position = Vector4(px, py, 0, 0);
    m_vertices[1].m_position = Vector4(pw + px, py, 0, 0);
    m_vertices[2].m_position = Vector4(px, ph + py, 0, 0);
    m_vertices[3].m_position = Vector4(pw + px, ph + py, 0, 0);
    if (!sg::is_valid(vb)) {
        sg_buffer_desc vbd;
        Inline::clearZeroMemory(vbd);
        vbd.usage = SG_USAGE_STREAM;
        vbd.size = sizeof(m_vertices);
        if (Inline::isDebugLabelEnabled()) {
            vbd.label = "@nanoem/AAPass/Vertices";
        }
        vb = sg::make_buffer(&vbd);
        nanoem_assert(sg::query_buffer_state(vb) == SG_RESOURCESTATE_VALID, "vertex buffer must be valid");
        SG_LABEL_BUFFER(vb, vbd.label);
    }
    sg::update_buffer(vb, m_vertices, sizeof(m_vertices));
}

void
AAPass::setupShaderDescription(sg_shader_desc &desc)
{
    const sg_backend backend = sg::query_backend();
#if defined(_WIN32)
    if (backend == SG_BACKEND_D3D11) {
        desc.fs.bytecode.ptr = g_nanoem_fxaa_ps_dxbc_data;
        desc.fs.bytecode.size = g_nanoem_fxaa_ps_dxbc_size;
        desc.vs.bytecode.ptr = g_nanoem_blit_vs_dxbc_data;
        desc.vs.bytecode.size = g_nanoem_blit_vs_dxbc_size;
        desc.fs.images[0] = sg_shader_image_desc { nullptr, SG_IMAGETYPE_2D, SG_SAMPLERTYPE_FLOAT };
    }
    else
#endif /* _WIN32 */
    if (sg::is_backend_metal(backend)) {
        desc.fs.bytecode.ptr = g_nanoem_fxaa_fs_msl_macos_data;
        desc.fs.bytecode.size = g_nanoem_fxaa_fs_msl_macos_size;
        desc.vs.bytecode.ptr = g_nanoem_blit_vs_msl_macos_data;
        desc.vs.bytecode.size = g_nanoem_blit_vs_msl_macos_size;
        desc.fs.images[0] = sg_shader_image_desc { nullptr, SG_IMAGETYPE_2D, SG_SAMPLERTYPE_FLOAT };
    }
    else if (backend == SG_BACKEND_GLCORE33) {
        desc.fs.source = reinterpret_cast<const char *>(g_nanoem_fxaa_fs_glsl_core33_data);
        desc.vs.source = reinterpret_cast<const char *>(g_nanoem_blit_vs_glsl_core33_data);
#if defined(NANOEM_ENABLE_SHADER_OPTIMIZED)
        desc.fs.images[0] = sg_shader_image_desc { "SPIRV_Cross_Combined", SG_IMAGETYPE_2D, SG_SAMPLERTYPE_FLOAT };
#else
        desc.fs.images[0] = sg_shader_image_desc { "SPIRV_Cross_Combinedu_textureu_textureSampler", SG_IMAGETYPE_2D,
            SG_SAMPLERTYPE_FLOAT };
#endif /* NANOEM_ENABLE_SHADER_OPTIMIZED */
    }
    else if (backend == SG_BACKEND_GLES3) {
        desc.fs.source = reinterpret_cast<const char *>(g_nanoem_fxaa_fs_glsl_es3_data);
        desc.vs.source = reinterpret_cast<const char *>(g_nanoem_blit_vs_glsl_es3_data);
#if defined(NANOEM_ENABLE_SHADER_OPTIMIZED)
        desc.fs.images[0] = sg_shader_image_desc { "SPIRV_Cross_Combined", SG_IMAGETYPE_2D, SG_SAMPLERTYPE_FLOAT };
#else
        desc.fs.images[0] = sg_shader_image_desc { "SPIRV_Cross_Combinedu_textureu_textureSampler", SG_IMAGETYPE_2D,
            SG_SAMPLERTYPE_FLOAT };
#endif /* NANOEM_ENABLE_SHADER_OPTIMIZED */
    }
    else if (backend == SG_BACKEND_WGPU) {
        desc.fs.bytecode.ptr = g_nanoem_fxaa_fs_spirv_data;
        desc.fs.bytecode.size = g_nanoem_fxaa_fs_spirv_size;
        desc.vs.bytecode.ptr = g_nanoem_blit_vs_spirv_data;
        desc.vs.bytecode.size = g_nanoem_blit_vs_spirv_size;
        desc.fs.images[0] = sg_shader_image_desc { nullptr, SG_IMAGETYPE_2D, SG_SAMPLERTYPE_FLOAT };
    }
    desc.fs.uniform_blocks[0].size = sizeof(Vector4);
#if defined(NANOEM_ENABLE_SHADER_OPTIMIZED)
    desc.fs.uniform_blocks[0].uniforms[0] = sg_shader_uniform_desc { "_25", SG_UNIFORMTYPE_FLOAT4, 1 };
#else
    desc.fs.uniform_blocks[0].uniforms[0] = sg_shader_uniform_desc { "fxaa_parameters_t", SG_UNIFORMTYPE_FLOAT4, 1 };
#endif /* NANOEM_ENABLE_SHADER_OPTIMIZED */
    desc.vs.entry = "nanoemVSMain";
    desc.fs.entry = "nanoemPSMain";
    desc.attrs[0] = sg_shader_attr_desc { "a_position", "SV_POSITION", 0 };
    desc.attrs[1] = sg_shader_attr_desc { "a_normal", "NORMAL", 0 };
    desc.attrs[2] = sg_shader_attr_desc { "a_texcoord0", "TEXCOORD", 0 };
#if defined(_WIN32)
    if (backend == SG_BACKEND_D3D11) {
        desc.attrs[3] = sg_shader_attr_desc { "a_texcoord1", "TEXCOORD", 1 };
        desc.attrs[4] = sg_shader_attr_desc { "a_texcoord2", "TEXCOORD", 2 };
        desc.attrs[5] = sg_shader_attr_desc { "a_texcoord3", "TEXCOORD", 3 };
        desc.attrs[6] = sg_shader_attr_desc { "a_texcoord4", "TEXCOORD", 4 };
        desc.attrs[7] = sg_shader_attr_desc { "a_color0", "COLOR", 0 };
    }
#endif /* _WIN32 */
}

void
AAPass::setupPipelineDescription(sg_pipeline_desc &desc)
{
    desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
    desc.face_winding = SG_FACEWINDING_CW;
    desc.cull_mode = SG_CULLMODE_BACK;
    sg_layout_desc &ld = desc.layout;
    ld.buffers[0].stride = sizeof(sg::QuadVertexUnit);
    ld.attrs[0] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_position), SG_VERTEXFORMAT_FLOAT3 };
    ld.attrs[1] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_position), SG_VERTEXFORMAT_FLOAT3 };
    ld.attrs[2] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_texcoord), SG_VERTEXFORMAT_FLOAT2 };
#if defined(_WIN32)
    if (sg::query_backend() == SG_BACKEND_D3D11) {
        ld.attrs[3] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_texcoord), SG_VERTEXFORMAT_FLOAT4 };
        ld.attrs[4] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_texcoord), SG_VERTEXFORMAT_FLOAT4 };
        ld.attrs[5] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_texcoord), SG_VERTEXFORMAT_FLOAT4 };
        ld.attrs[6] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_texcoord), SG_VERTEXFORMAT_FLOAT4 };
        ld.attrs[7] = sg_vertex_attr_desc { 0, offsetof(sg::QuadVertexUnit, m_position), SG_VERTEXFORMAT_FLOAT4 };
    }
#endif /* _WIN32 */
}

const char *
AAPass::name() const NANOEM_DECL_NOEXCEPT
{
    return "AAPass";
}

} /* namespace internal */
} /* namespace nanoem */
