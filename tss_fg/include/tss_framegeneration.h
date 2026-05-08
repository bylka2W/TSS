// This file is part of the TSS SDK.
//
// Copyright (C) 2026 CapGames.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "../../api/include/tss_api.h"
#include "tss_framegeneration_api_types.h"

#define TSS_FRAMEGENERATION_VERSION_MAJOR 4
#define TSS_FRAMEGENERATION_VERSION_MINOR 0
#define TSS_FRAMEGENERATION_VERSION_PATCH 0

#define TSS_FRAMEGENERATION_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define TSS_FRAMEGENERATION_VERSION                           TSS_FRAMEGENERATION_MAKE_VERSION(TSS_FRAMEGENERATION_VERSION_MAJOR, TSS_FRAMEGENERATION_VERSION_MINOR, TSS_FRAMEGENERATION_VERSION_PATCH)

#if defined(__cplusplus)
extern "C" {
#endif
enum TssApiCreateContextFramegenerationFlags
{
    TSS_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT             = (1 << 0),  ///< A bit indicating that async compute workloads should be supported. Enables generation work on async compute queues.
    TSS_FRAMEGENERATION_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS  = (1 << 1),  ///< A bit indicating if the motion vectors are rendered at display resolution.
    TSS_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION = (1 << 2),  ///< A bit indicating that the motion vectors have the jittering pattern applied to them.
    TSS_FRAMEGENERATION_ENABLE_DEPTH_INVERTED                     = (1 << 3),  ///< A bit indicating that the input depth buffer data provided is inverted [1..0].
    TSS_FRAMEGENERATION_ENABLE_DEPTH_INFINITE                     = (1 << 4),  ///< A bit indicating that the input depth buffer data provided is using an infinite far plane.
    TSS_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE                 = (1 << 5),  ///< A bit indicating if the input color data provided to all inputs is using a high-dynamic range.
    TSS_FRAMEGENERATION_ENABLE_DEBUG_CHECKING                     = (1 << 6),  ///< A bit indicating that the runtime should check some API values and report issues.
};

enum TssApiDispatchFramegenerationFlags
{
    TSS_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES       = (1 << 0),  ///< A bit indicating that the debug tear lines will be drawn to the generated output.
    TSS_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS = (1 << 1),  ///< A bit indicating that the debug reset indicators will be drawn to the generated output.
    TSS_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW             = (1 << 2),  ///< A bit indicating that the generated output resource will contain debug views with relevant information.
    TSS_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY = (1 << 3),  ///< A bit indicating that the context should only run frame interpolation and not modify the swapchain.
    TSS_FRAMEGENERATION_FLAG_DRAW_DEBUG_PACING_LINES     = (1 << 4),  ///< A bit indicating that the debug pacing lines will be drawn to the generated output.
    TSS_FRAMEGENERATION_FLAG_RESERVED_1                  = (1 << 5),  ///< Reserved for future use.
    TSS_FRAMEGENERATION_FLAG_RESERVED_2                  = (1 << 6),  ///< Reserved for future use.
};

enum TssApiUiCompositionFlags
{
    TSS_FRAMEGENERATION_UI_COMPOSITION_FLAG_USE_PREMUL_ALPHA                    = (1 << 0),  ///< A bit indicating that we use premultiplied alpha for UI composition.
    TSS_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING = (1 << 1),  ///< A bit indicating that the swapchain should doublebuffer the UI resource.
};

#define TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x01)
struct tssCreateContextDescFrameGeneration
{
    tssCreateContextDescHeader header;            ///< Description header for frame generation context creation.
    uint32_t                   flags;             ///< A combination of zero or more values from TssApiCreateContextFramegenerationFlags.
    struct TssApiDimensions2D  displaySize;       ///< The resolution at which both rendered and generated frames will be displayed.
    struct TssApiDimensions2D  maxRenderSize;     ///< The maximum rendering resolution.
    uint32_t                   backBufferFormat;  ///< The surface format for the backbuffer. One of the values from TssApiSurfaceFormat.
};

#define TSS_API_CALLBACK_DESC_TYPE_FRAMEGENERATION_PRESENT TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x05)
typedef struct tssCallbackDescFrameGenerationPresent
{
    tssDispatchDescHeader header;                 ///< Description header for present callback dispatch.
    void*                 device;                 ///< The GPU device (ID3D12Device for DX12, VkDevice for Vulkan) passed during context creation.
    void*                 commandList;            ///< A command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) that will be executed before presentation.
    struct TssApiResource currentBackBuffer;      ///< Backbuffer image either rendered or generated.
    struct TssApiResource currentUI;              ///< UI image for composition if passed. Otherwise empty.
    struct TssApiResource outputSwapChainBuffer;  ///< Output image that will be presented.
    bool                  isGeneratedFrame;       ///< true if this frame is generated, false if rendered.
    uint64_t              frameID;                ///< Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
} tssCallbackDescFrameGenerationPresent;

#define TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x03)
typedef struct tssDispatchDescFrameGeneration
{
    tssDispatchDescHeader header;                      ///< Description header for frame generation dispatch.
    void*                 commandList;                 ///< The command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) to record frame generation commands.
    struct TssApiResource presentColor;                ///< The current presentation color buffer to be used as source for frame generation.
    struct TssApiResource outputs[4];                  ///< Output destination targets for generated frames (1 for each frame in numGeneratedFrames).
    uint32_t              numGeneratedFrames;          ///< The number of frames to generate from the input color target.
    bool                  reset;                       ///< A boolean value which when set to true, indicates the camera has moved discontinuously and frame generation should reset.
    uint32_t              backbufferTransferFunction;  ///< The transfer function used to convert frame generation source color data to linear RGB. One of the values from TssApiBackbufferTransferFunction.
    float                 minMaxLuminance[2];          ///< Min and max luminance values (in nits), used when converting HDR colors to linear RGB.
    struct TssApiRect2D   generationRect;              ///< The area of the backbuffer that should be used for frame generation in case only a part of the screen is active (e.g. due to movie bars).
    uint64_t              frameID;                     ///< Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
} tssDispatchDescFrameGeneration;

typedef tssReturnCode_t (*TssApiPresentCallbackFunc)(tssCallbackDescFrameGenerationPresent* params, void* pUserCtx);
typedef tssReturnCode_t (*TssApiFrameGenerationDispatchFunc)(tssDispatchDescFrameGeneration* params, void* pUserCtx);

#define TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x02)
struct tssConfigureDescFrameGeneration
{
    tssConfigureDescHeader            header;                              ///< Description header for frame generation configuration.
    void*                             swapChain;                           ///< The swapchain to use with frame generation (IDXGISwapChain for DX12, VkSwapchainKHR for Vulkan).
    TssApiPresentCallbackFunc         presentCallback;                     ///< Callback function called when finalizing the frame image for presentation and UI composition.
    void*                             presentCallbackUserContext;          ///< User context pointer to be passed to the present callback function.
    TssApiFrameGenerationDispatchFunc frameGenerationCallback;             ///< Callback function called to dispatch frame generation work and generate interpolated frames.
    void*                             frameGenerationCallbackUserContext;  ///< User context pointer to be passed to the frame generation callback function.
    bool                              frameGenerationEnabled;              ///< Sets the state of frame generation. Set to false to disable frame generation.
    bool                              allowAsyncWorkloads;                 ///< Sets the state of async workloads. Set to true to enable generation work on async compute.
    struct TssApiResource             HUDLessColor;                        ///< The hudless back buffer image to use for UI extraction from backbuffer resource. May be empty.
    uint32_t                          flags;                               ///< Zero or combination of flags from TssApiDispatchFramegenerationFlags.
    bool                              onlyPresentGenerated;                ///< Set to true to only present generated frames.
    struct TssApiRect2D               generationRect;                      ///< The area of the backbuffer that should be used for generation in case only a part of the screen is used e.g. due to movie bars
    uint64_t                          frameID;                             ///< Identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
};

#pragma TSS_PRAGMA_WARNING_PUSH
#pragma TSS_PRAGMA_WARNING_DISABLE_DEPRECATIONS

#define TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x04)
struct TSS_DEPRECATION("tssDispatchDescFrameGenerationPrepare is deprecated, use tssDispatchDescFrameGenerationPrepareV2") tssDispatchDescFrameGenerationPrepare
{
    tssDispatchDescHeader      header;             ///< Description header for frame generation prepare dispatch.
    uint64_t                   frameID;            ///< Frame identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
    uint32_t                   flags;              ///< Zero or combination of values from TssApiDispatchFramegenerationFlags.
    void*                      commandList;        ///< The command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) to record frame generation prepare commands.
    struct TssApiDimensions2D  renderSize;         ///< The dimensions used to render game content, dilatedDepth, dilatedMotionVectors are expected to be of ths size.
    struct TssApiFloatCoords2D jitterOffset;       ///< The subpixel jitter offset applied to the camera.
    struct TssApiFloatCoords2D motionVectorScale;  ///< The scale factor to convert motion vectors to UV space. Set to (1.0, 1.0) if motion vectors are already in pixel space, or to renderSize dimensions if motion vectors are in UV space.

    float                 frameTimeDelta;           ///< Time elapsed in milliseconds since the last frame.
    bool                  unused_reset;             ///< A (currently unused) boolean value which when set to true, indicates FrameGeneration will be called in reset mode
    float                 cameraNear;               ///< The distance to the near plane of the camera.
    float                 cameraFar;                ///< The distance to the far plane of the camera. This is used only used in case of non infinite depth.
    float                 cameraFovAngleVertical;   ///< The camera angle field of view in the vertical direction (expressed in radians).
    float                 viewSpaceToMetersFactor;  ///< The scale factor to convert view space units to meters
    struct TssApiResource depth;                    ///< The depth buffer data for the current frame.
    struct TssApiResource motionVectors;            ///< The motion vector data for the current frame.
};

#pragma TSS_PRAGMA_WARNING_POP

#define TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_KEYVALUE TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x06)
struct tssConfigureDescFrameGenerationKeyValue
{
    tssConfigureDescHeader header;  ///< Description header for frame generation key-value configuration.
    uint64_t               key;     ///< Configuration key, member of the TssApiConfigureFrameGenerationKey enumeration.
    uint64_t               u64;     ///< Integer value or enum value to set.
    void*                  ptr;     ///< Pointer value to set or pointer to value to set.
};

enum TssApiConfigureFrameGenerationKey
{
    TSS_API_CONFIGURE_FRAMEGENERATION_KEY_DEBUG_VIEW_MODE       = 0,  ///< A key to set a debug view mode of display.
    TSS_API_CONFIGURE_FRAMEGENERATION_KEY_DEBUG_VIEW_FLOW_SCALE = 1,  ///< A key to set a debug view scale for flow resources.
};

#define TSS_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x07)
struct tssQueryDescFrameGenerationGetGPUMemoryUsage
{
    tssQueryDescHeader              header;                         ///< Description header for GPU memory usage query.
    struct TssApiEffectMemoryUsage* gpuMemoryUsageFrameGeneration;  ///< Output pointer to receive GPU memory usage information for frame generation.
};

#define TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_REGISTERDISTORTIONRESOURCE TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x08)
//Pass this optional linked struct after tssConfigureDescFrameGeneration
struct tssConfigureDescFrameGenerationRegisterDistortionFieldResource
{
    tssConfigureDescHeader header;           ///< Description header for distortion field resource configuration.
    struct TssApiResource  distortionField;  ///< A resource containing distortion offset data. Needs to be 2-component (ie. RG). Read by FG shaders via Sample. Pixel value encodes [UV coordinate of pixel after lens distortion effect - UV coordinate of pixel before lens distortion].
};

#define TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_HUDLESS TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x09)
//Pass this optional linked struct at FG context creation to enable app to use different hudlessBackBufferformat (IE.RGBA8_UNORM) from backBufferFormat (IE. BGRA8_UNORM)
struct tssCreateContextDescFrameGenerationHudless
{
    tssCreateContextDescHeader header;                   ///< Description header for hudless context creation.
    uint32_t                   hudlessBackBufferFormat;  ///< The surface format for the hudless back buffer. One of the values from TssApiSurfaceFormat.
};

#pragma TSS_PRAGMA_WARNING_PUSH
#pragma TSS_PRAGMA_WARNING_DISABLE_DEPRECATIONS

#define TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_CAMERAINFO TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x0a)
// Link this struct after tssDispatchDescFrameGenerationPrepare. This is a required input to TSS 3.1.4 and TSS 3.1.5.
// These fields are now embedded in tssDispatchDescFrameGenerationPrepareV2.
struct TSS_DEPRECATION("tssDispatchDescFrameGenerationPrepareCameraInfo is deprecated, use tssDispatchDescFrameGenerationPrepareV2 instead") tssDispatchDescFrameGenerationPrepareCameraInfo
{
    tssConfigureDescHeader header;             ///< Description header for camera info configuration.
    float                  cameraPosition[3];  ///< The camera position in world space.
    float                  cameraUp[3];        ///< The camera up normalized vector in world space.
    float                  cameraRight[3];     ///< The camera right normalized vector in world space.
    float                  cameraForward[3];   ///< The camera forward normalized vector in world space.
};

#pragma TSS_PRAGMA_WARNING_POP

#define TSS_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE_V2 TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x0b)
struct tssQueryDescFrameGenerationGetGPUMemoryUsageV2
{
    tssQueryDescHeader              header;                         ///< Description header for GPU memory usage query.
    void*                           device;                         ///< The GPU device. For DX12: pointer to ID3D12Device. For VK: pointer to VkDevice. App needs to fill out before Query() call.
    struct TssApiDimensions2D       maxRenderSize;                  ///< The maximum rendering resolution. App needs to fill out before Query() call.
    struct TssApiDimensions2D       displaySize;                    ///< The resolution at which both rendered and generated frames will be displayed. App needs to fill out before Query() call.
    uint32_t                        createFlags;                    ///< Context creation flags. A combination of zero or more values from TssApiCreateContextFramegenerationFlags. App needs to fill out before Query() call.
    uint32_t                        dispatchFlags;                  ///< Dispatch flags. A combination of zero or more values from TssApiDispatchFramegenerationFlags. App needs to fill out before Query() call.
    uint32_t                        backBufferFormat;               ///< The surface format for the backbuffer. One of the values from TssApiSurfaceFormat. App needs to fill out before Query() call.
    uint32_t                        hudlessBackBufferFormat;        ///< The surface format for HUDLessColor resource if used. One of the values from TssApiSurfaceFormat. Otherwise set value to TSS_API_SURFACE_FORMAT_UNKNOWN(0). App needs to fill out before Query() call.
    struct TssApiEffectMemoryUsage* gpuMemoryUsageFrameGeneration;  ///< Output pointer to receive GPU memory usage information for frame generation. Populated by Query() call.
};

#define TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2 TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x0c)
struct tssDispatchDescFrameGenerationPrepareV2
{
    tssDispatchDescHeader      header;             ///< Description header for frame generation prepare dispatch.
    uint64_t                   frameID;            ///< Frame identifier used to select internal resources when async support is enabled. Must increment by exactly one (1) for each frame. Any non-exactly-one difference will reset the frame generation logic.
    uint32_t                   flags;              ///< Zero or combination of values from TssApiDispatchFramegenerationFlags.
    void*                      commandList;        ///< The command list (ID3D12GraphicsCommandList for DX12, VkCommandBuffer for Vulkan) to record frame generation prepare commands.
    struct TssApiDimensions2D  renderSize;         ///< The dimensions used to render game content. dilatedDepth and dilatedMotionVectors are expected to be of this size.
    struct TssApiFloatCoords2D jitterOffset;       ///< The subpixel jitter offset applied to the camera.
    struct TssApiFloatCoords2D motionVectorScale;  ///< The scale factor to convert motion vectors to UV space. Set to (1.0, 1.0) if motion vectors are already in pixel space, or to renderSize dimensions if motion vectors are in UV space.

    float                 frameTimeDelta;           ///< Time elapsed in milliseconds since the last frame.
    bool                  reset;                    ///< A boolean value which when set to true, indicates the camera has moved discontinuously and frame generation should reset its internal state.
    float                 cameraNear;               ///< The distance to the near plane of the camera.
    float                 cameraFar;                ///< The distance to the far plane of the camera. This is used only in case of non-infinite depth.
    float                 cameraFovAngleVertical;   ///< The camera angle field of view in the vertical direction (expressed in radians).
    float                 viewSpaceToMetersFactor;  ///< The scale factor to convert view space units to meters.
    struct TssApiResource depth;                    ///< The depth buffer data for the current frame.
    struct TssApiResource motionVectors;            ///< The motion vector data for the current frame.

    float cameraPosition[3];  ///< The camera position in world space.
    float cameraUp[3];        ///< The camera up normalized vector in world space.
    float cameraRight[3];     ///< The camera right normalized vector in world space.
    float cameraForward[3];   ///< The camera forward normalized vector in world space.
};

#define TSS_API_CALLBACK_DESC_TYPE_FRAMEGENERATION_PRESENT_PREMUL_ALPHA TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x0d)
// This is a structure that is attached to header.pNext of tssCallbackDescFrameGenerationPresent
// Provides whether to use premultiplied alpha blending or not for the UI
typedef struct tssCallbackDescFrameGenerationPresentPremulAlpha
{
    tssApiHeader header;          ///< Header for versioning & ABI stability.
    bool         usePremulAlpha;  ///< Toggles whether UI gets premultiplied alpha blending or not.
} tssCallbackDescFrameGenerationPresentPremulAlpha;

/// Callback function for waiting on a named fence to reach a specific value.
/// @param fenceName The name of the fence to wait on.
/// @param fenceValueToWaitFor The fence value to wait for.
/// @return 0 on success, non-zero error code on failure.
typedef int32_t (*TssWaitCallbackFunc)(wchar_t* fenceName, uint64_t fenceValueToWaitFor);

#define TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x0e)
// Pass this linked struct at FG context creation to enable new API features - omitting this will prevent new APIs from functioning.
struct tssCreateContextDescFrameGenerationVersion
{
    tssCreateContextDescHeader header;   ///< Description header for frame generation version context creation.
    uint32_t                   version;  ///< The version of the API the application was built against. This must be set to TSS_FRAMEGENERATION_VERSION.
};

#define TSS_API_FRAME_GENERATION_CONFIG TSS_API_MAKE_EFFECT_SUB_ID(TSS_API_EFFECT_ID_FRAMEGENERATION, 0x0f)
/// A structure representing the configuration options to pass to FrameInterpolationSwapChain
///
/// @ingroup TssInterface
typedef struct TssFrameGenerationConfig
{
    tssApiHeader                      header;                          ///< Header for versioning & ABI stability.
    void*                             swapChain;                       ///< The <c><i>TssSwapchain</i></c> to use with frame interpolation.
    TssApiPresentCallbackFunc         presentCallback;                 ///< A UI composition callback to call when finalizing the frame image.
    void*                             presentCallbackContext;          ///< A pointer to be passed to the UI composition callback.
    TssApiFrameGenerationDispatchFunc frameGenerationCallback;         ///< The frame generation callback to use to generate the interpolated frame.
    void*                             frameGenerationCallbackContext;  ///< A pointer to be passed to the frame generation callback.
    bool                              frameGenerationEnabled;          ///< Sets the state of frame generation. Set to false to disable frame generation.
    bool                              allowAsyncWorkloads;             ///< Sets the state of async workloads. Set to true to enable interpolation work on async compute.
    bool                              allowAsyncPresent;               ///< Sets the state of async presentation (console only). Set to true to enable present from async command queue.
    struct TssApiResource             HUDLessColor;                    ///< The hudless back buffer image to use for UI extraction from backbuffer resource.
    uint32_t                          flags;                           ///< Zero or combination of flags from TssApiDispatchFramegenerationFlags.
    bool                              onlyPresentInterpolated;         ///< Set to true to only present interpolated frame.
    struct TssApiRect2D               interpolationRect;               ///< Set the area in the backbuffer that will be interpolated.
    uint64_t                          frameID;                         ///< A frame identifier used to synchronize resource usage in workloads.
    bool                              drawDebugPacingLines;            ///< Sets the state of pacing debug lines. Set to true to display debug lines.
} TssFrameGenerationConfig;

#if defined(__cplusplus)
}  // extern "C"
#endif
