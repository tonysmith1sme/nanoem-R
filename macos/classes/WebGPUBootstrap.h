/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#ifndef NANOEM_EMAPP_MACOS_WEBGPUBOOTSTRAP_H_
#define NANOEM_EMAPP_MACOS_WEBGPUBOOTSTRAP_H_

#include "emapp/Forward.h"

@class NSView;

namespace nanoem {
namespace macos {

/**
 * Optional WebGPU host bootstrap used by the "Vulkan" preference path.
 * When NANOEM_HAS_WGPU (Dawn) is available this owns instance/device/surface/views;
 * otherwise create() fails and the app falls back to Metal/OpenGL.
 *
 * Frame lifecycle mirrors sokol_app emscripten WGPU:
 *   beginFrame()  - acquire swapchain texture view
 *   render/resolve/depth callbacks return cached views
 *   endFrame()    - present + release transient swapchain views
 */
class WebGPUBootstrap {
public:
    WebGPUBootstrap() = default;
    ~WebGPUBootstrap();

    bool create(NSView *view, int width, int height, int sampleCount);
    void destroy();
    void resize(int width, int height, int sampleCount);
    void beginFrame();
    void endFrame();
    bool isValid() const NANOEM_DECL_NOEXCEPT;

    void *device() const NANOEM_DECL_NOEXCEPT;
    void *renderView() const NANOEM_DECL_NOEXCEPT;
    void *resolveView() const NANOEM_DECL_NOEXCEPT;
    void *depthStencilView() const NANOEM_DECL_NOEXCEPT;
    int renderFormat() const NANOEM_DECL_NOEXCEPT;
    int sampleCount() const NANOEM_DECL_NOEXCEPT;

private:
    void releaseFrameViews();
    void releaseAttachments();

    void *m_instance = nullptr;
    void *m_adapter = nullptr;
    void *m_device = nullptr;
    void *m_queue = nullptr;
    void *m_surface = nullptr;
    void *m_swapChain = nullptr;
    void *m_depthTexture = nullptr;
    void *m_depthView = nullptr;
    void *m_msaaTexture = nullptr;
    void *m_msaaView = nullptr;
    void *m_swapchainView = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_sampleCount = 1;
    int m_renderFormat = 0;
    bool m_valid = false;
};

bool isWebGPUAvailable() NANOEM_DECL_NOEXCEPT;

} /* namespace macos */
} /* namespace nanoem */

#endif /* NANOEM_EMAPP_MACOS_WEBGPUBOOTSTRAP_H_ */
