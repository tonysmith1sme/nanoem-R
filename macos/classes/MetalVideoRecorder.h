/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#ifndef NANOEM_EMAPP_MACOS_METALVIDEORECORDER_H_
#define NANOEM_EMAPP_MACOS_METALVIDEORECORDER_H_

#import "BaseVideoRecorder.h"

#import <Metal/MTLPixelFormat.h>

@protocol MTLCommandQueue;
@protocol MTLDevice;
@protocol MTLTexture;

namespace nanoem {
namespace macos {

class MetalVideoRecorder final : public BaseVideoRecorder {
public:
    MetalVideoRecorder(Project *projectPtr, id<MTLDevice> device, id<MTLCommandQueue> commandQueue);
    ~MetalVideoRecorder() noexcept;

    bool capture(nanoem_frame_index_t frameIndex) override;

private:
    static MTLPixelFormat metalPixelFormat(OSType format) noexcept;
    id<MTLTexture> createTexture(IOSurfaceRef surface, sg_image_desc &ide);
    void updateDepthImage(IOSurfaceRef surface);
    bool appendPendingFrame();

    id<MTLDevice> m_device = nil;
    id<MTLCommandQueue> m_commandQueue = nil;
    id<MTLTexture> m_texture = nil;
    CVPixelBufferRef m_pendingPixelBuffer = nullptr;
    nanoem_frame_index_t m_pendingFrameIndex = 0;
    bool m_pendingFrameSubmitted = false;
};

} /* namespace macos */
} /* namespace nanoem */

#endif /* NANOEM_EMAPP_MACOS_METALVIDEORECORDER_H_ */
