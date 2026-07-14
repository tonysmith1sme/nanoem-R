/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "WebGPUBootstrap.h"

#include "emapp/private/CommonInclude.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#if defined(NANOEM_HAS_WGPU)
#if defined(__has_include)
#if __has_include(<webgpu/webgpu.h>)
#include <webgpu/webgpu.h>
#elif __has_include(<dawn/webgpu.h>)
#include <dawn/webgpu.h>
#else
#include <webgpu.h>
#endif
#else
#include <webgpu/webgpu.h>
#endif
#endif /* NANOEM_HAS_WGPU */

namespace nanoem {
namespace macos {
namespace {

#if defined(NANOEM_HAS_WGPU)
static void
requestAdapterCallback(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char * /* message */, void *userdata)
{
    if (status == WGPURequestAdapterStatus_Success) {
        *static_cast<WGPUAdapter *>(userdata) = adapter;
    }
}

static void
requestDeviceCallback(WGPURequestDeviceStatus status, WGPUDevice device, const char * /* message */, void *userdata)
{
    if (status == WGPURequestDeviceStatus_Success) {
        *static_cast<WGPUDevice *>(userdata) = device;
    }
}
#endif /* NANOEM_HAS_WGPU */

} /* namespace anonymous */

bool
isWebGPUAvailable() NANOEM_DECL_NOEXCEPT
{
#if defined(NANOEM_HAS_WGPU)
    return true;
#else
    return false;
#endif
}

WebGPUBootstrap::~WebGPUBootstrap()
{
    destroy();
}

bool
WebGPUBootstrap::create(NSView *view, int width, int height, int sampleCount)
{
    destroy();
#if defined(NANOEM_HAS_WGPU)
    if (!view || width <= 0 || height <= 0) {
        return false;
    }
    const WGPUInstanceDescriptor instanceDesc = {};
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
    if (!instance) {
        return false;
    }
    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
    WGPUAdapter adapter = nullptr;
    wgpuInstanceRequestAdapter(instance, &adapterOptions, requestAdapterCallback, &adapter);
    if (!adapter) {
        wgpuInstanceRelease(instance);
        return false;
    }
    WGPUDeviceDescriptor deviceDesc = {};
    WGPUDevice device = nullptr;
    wgpuAdapterRequestDevice(adapter, &deviceDesc, requestDeviceCallback, &device);
    if (!device) {
        wgpuAdapterRelease(adapter);
        wgpuInstanceRelease(instance);
        return false;
    }
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    if (![view.layer isKindOfClass:[CAMetalLayer class]]) {
        view.wantsLayer = YES;
        view.layer = [CAMetalLayer layer];
    }
    WGPUSurfaceDescriptorFromMetalLayer metalLayerDesc = {};
    metalLayerDesc.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;
    metalLayerDesc.layer = (__bridge void *) view.layer;
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&metalLayerDesc);
    WGPUSurface surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
    if (!surface) {
        wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter);
        wgpuInstanceRelease(instance);
        return false;
    }
    m_instance = instance;
    m_adapter = adapter;
    m_device = device;
    m_queue = queue;
    m_surface = surface;
    m_renderFormat = static_cast<int>(WGPUTextureFormat_BGRA8Unorm);
    m_valid = true;
    resize(width, height, sampleCount);
    return m_valid && m_swapChain != nullptr;
#else
    (void)view; (void)width; (void)height; (void)sampleCount;
    return false;
#endif
}

void
WebGPUBootstrap::destroy()
{
    releaseFrameViews();
    releaseAttachments();
#if defined(NANOEM_HAS_WGPU)
    if (m_surface) {
        wgpuSurfaceRelease(static_cast<WGPUSurface>(m_surface));
        m_surface = nullptr;
    }
    if (m_device) {
        wgpuDeviceRelease(static_cast<WGPUDevice>(m_device));
        m_device = nullptr;
    }
    if (m_adapter) {
        wgpuAdapterRelease(static_cast<WGPUAdapter>(m_adapter));
        m_adapter = nullptr;
    }
    if (m_instance) {
        wgpuInstanceRelease(static_cast<WGPUInstance>(m_instance));
        m_instance = nullptr;
    }
    m_queue = nullptr;
#endif
    m_valid = false;
    m_width = 0;
    m_height = 0;
    m_sampleCount = 1;
}

void
WebGPUBootstrap::resize(int width, int height, int sampleCount)
{
#if defined(NANOEM_HAS_WGPU)
    if (!m_valid || !m_device || !m_surface || width <= 0 || height <= 0) {
        return;
    }
    releaseFrameViews();
    releaseAttachments();
    m_width = width;
    m_height = height;
    m_sampleCount = glm::max(sampleCount, 1);
    WGPUSwapChainDescriptor swapDesc = {};
    swapDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapDesc.format = static_cast<WGPUTextureFormat>(m_renderFormat);
    swapDesc.width = static_cast<uint32_t>(width);
    swapDesc.height = static_cast<uint32_t>(height);
    swapDesc.presentMode = WGPUPresentMode_Fifo;
    m_swapChain =
        wgpuDeviceCreateSwapChain(static_cast<WGPUDevice>(m_device), static_cast<WGPUSurface>(m_surface), &swapDesc);
    WGPUTextureDescriptor depthDesc = {};
    depthDesc.usage = WGPUTextureUsage_RenderAttachment;
    depthDesc.dimension = WGPUTextureDimension_2D;
    depthDesc.size.width = static_cast<uint32_t>(width);
    depthDesc.size.height = static_cast<uint32_t>(height);
    depthDesc.size.depthOrArrayLayers = 1;
    depthDesc.format = WGPUTextureFormat_Depth24PlusStencil8;
    depthDesc.mipLevelCount = 1;
    depthDesc.sampleCount = static_cast<uint32_t>(m_sampleCount);
    m_depthTexture = wgpuDeviceCreateTexture(static_cast<WGPUDevice>(m_device), &depthDesc);
    m_depthView = wgpuTextureCreateView(static_cast<WGPUTexture>(m_depthTexture), nullptr);
    if (m_sampleCount > 1) {
        WGPUTextureDescriptor msaaDesc = depthDesc;
        msaaDesc.format = static_cast<WGPUTextureFormat>(m_renderFormat);
        m_msaaTexture = wgpuDeviceCreateTexture(static_cast<WGPUDevice>(m_device), &msaaDesc);
        m_msaaView = wgpuTextureCreateView(static_cast<WGPUTexture>(m_msaaTexture), nullptr);
    }
#else
    (void)width; (void)height; (void)sampleCount;
#endif
}

void
WebGPUBootstrap::beginFrame()
{
#if defined(NANOEM_HAS_WGPU)
    if (!m_valid || !m_swapChain) {
        return;
    }
    if (!m_swapchainView) {
        m_swapchainView = wgpuSwapChainGetCurrentTextureView(static_cast<WGPUSwapChain>(m_swapChain));
    }
#endif
}

void
WebGPUBootstrap::endFrame()
{
#if defined(NANOEM_HAS_WGPU)
    if (!m_valid || !m_swapChain) {
        return;
    }
    wgpuSwapChainPresent(static_cast<WGPUSwapChain>(m_swapChain));
    releaseFrameViews();
#endif
}

void
WebGPUBootstrap::releaseFrameViews()
{
#if defined(NANOEM_HAS_WGPU)
    if (m_swapchainView) {
        wgpuTextureViewRelease(static_cast<WGPUTextureView>(m_swapchainView));
        m_swapchainView = nullptr;
    }
#endif
}

void
WebGPUBootstrap::releaseAttachments()
{
#if defined(NANOEM_HAS_WGPU)
    if (m_msaaView) {
        wgpuTextureViewRelease(static_cast<WGPUTextureView>(m_msaaView));
        m_msaaView = nullptr;
    }
    if (m_msaaTexture) {
        wgpuTextureRelease(static_cast<WGPUTexture>(m_msaaTexture));
        m_msaaTexture = nullptr;
    }
    if (m_depthView) {
        wgpuTextureViewRelease(static_cast<WGPUTextureView>(m_depthView));
        m_depthView = nullptr;
    }
    if (m_depthTexture) {
        wgpuTextureRelease(static_cast<WGPUTexture>(m_depthTexture));
        m_depthTexture = nullptr;
    }
    if (m_swapChain) {
        wgpuSwapChainRelease(static_cast<WGPUSwapChain>(m_swapChain));
        m_swapChain = nullptr;
    }
#endif
}

bool
WebGPUBootstrap::isValid() const NANOEM_DECL_NOEXCEPT
{
    return m_valid;
}

void *
WebGPUBootstrap::device() const NANOEM_DECL_NOEXCEPT
{
    return m_device;
}

void *
WebGPUBootstrap::renderView() const NANOEM_DECL_NOEXCEPT
{
    if (m_sampleCount > 1 && m_msaaView) {
        return m_msaaView;
    }
    return m_swapchainView;
}

void *
WebGPUBootstrap::resolveView() const NANOEM_DECL_NOEXCEPT
{
    if (m_sampleCount > 1) {
        return m_swapchainView;
    }
    return nullptr;
}

void *
WebGPUBootstrap::depthStencilView() const NANOEM_DECL_NOEXCEPT
{
    return m_depthView;
}

int
WebGPUBootstrap::renderFormat() const NANOEM_DECL_NOEXCEPT
{
    return m_renderFormat;
}

int
WebGPUBootstrap::sampleCount() const NANOEM_DECL_NOEXCEPT
{
    return m_sampleCount;
}

} /* namespace macos */
} /* namespace nanoem */
