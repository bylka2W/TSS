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
#include "../include/tss_framegeneration.h"

/// Legacy declaration for SDK1 resource structure.
///
/// @ingroup SDKTypes
#if defined(__cplusplus)
typedef struct TssResourceSDK1 : public TssApiResource
{
    wchar_t name[FFX_RESOURCE_NAME_SIZE];
    TssResourceSDK1& operator=(TssApiResource const& other)
    {
        if (this != &other)
        {
            this->description = other.description;
            this->resource = other.resource;
            this->state = other.state;
        }
        return *this;
    }
} TssResourceSDK1;
#else
typedef struct TssResourceSDK1
{
    TssApiResource header;
    wchar_t name[FFX_RESOURCE_NAME_SIZE];
} TssResourceSDK1;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct FfxPresentCallbackDescriptionSDK2
{
    TssDevice       device;                    ///< The active device
    TssCommandList  commandList;               ///< The command list on which to register render commands
    TssApiResource  currentBackBuffer;         ///< The backbuffer resource with scene information
    TssApiResource  currentUI;                 ///< Optional UI texture (when doing backbuffer + ui blend)
    TssApiResource  outputSwapChainBuffer;     ///< The swapchain target into which to render ui composition
    bool            isInterpolatedFrame;       ///< Whether this is an interpolated or real frame
    bool            usePremulAlpha;            ///< Toggles whether UI gets premultiplied alpha blending or not
    uint64_t        frameID;
} FfxPresentCallbackDescriptionSDK2;

typedef struct FfxPresentCallbackDescriptionSDK1
{
    TssDevice       device;                    ///< The active device
    TssCommandList  commandList;               ///< The command list on which to register render commands
    TssResourceSDK1  currentBackBuffer;         ///< The backbuffer resource with scene information
    TssResourceSDK1  currentUI;                 ///< Optional UI texture (when doing backbuffer + ui blend)
    TssResourceSDK1  outputSwapChainBuffer;     ///< The swapchain target into which to render ui composition
    bool            isInterpolatedFrame;       ///< Whether this is an interpolated or real frame
    bool            usePremulAlpha;            ///< Toggles whether UI gets premultiplied alpha blending or not
    uint64_t        frameID;
} FfxPresentCallbackDescriptionSDK1;

typedef struct TssFrameGenerationDispatchDescriptionSDK2 {
    TssCommandList                  commandList;                    ///< The command list on which to register render commands
    TssApiResource                  presentColor;                   ///< The current presentation color, this will be used as interpolation source data.
    TssApiResource                  outputs[4];                     ///< Interpolation destination targets (1 for each frame in numInterpolatedFrames)
    uint32_t                        numInterpolatedFrames;          ///< The number of frames to interpolate from the passed in color target
    bool                            reset;                          ///< A boolean value which when set to true, indicates the camera has moved discontinuously.
    TssApiBackbufferTransferFunction   backBufferTransferFunction;     ///< The transfer function use to convert interpolation source color data to linear RGB.
    float                           minMaxLuminance[2];             ///< Min and max luminance values, used when converting HDR colors to linear RGB
    TssApiRect2D                       interpolationRect;              ///< The area of the backbuffer that should be used for interpolation in case only a part of the screen is used e.g. due to movie bars
    uint64_t                        frameID;
} TssFrameGenerationDispatchDescriptionSDK2;

typedef struct TssFrameGenerationDispatchDescriptionSDK1 {
    TssCommandList                  commandList;                    ///< The command list on which to register render commands
    TssResourceSDK1                  presentColor;                   ///< The current presentation color, this will be used as interpolation source data.
    TssResourceSDK1                  outputs[4];                     ///< Interpolation destination targets (1 for each frame in numInterpolatedFrames)
    uint32_t                        numInterpolatedFrames;          ///< The number of frames to interpolate from the passed in color target
    bool                            reset;                          ///< A boolean value which when set to true, indicates the camera has moved discontinuously.
    TssApiBackbufferTransferFunction   backBufferTransferFunction;     ///< The transfer function use to convert interpolation source color data to linear RGB.
    float                           minMaxLuminance[2];             ///< Min and max luminance values, used when converting HDR colors to linear RGB
    TssApiRect2D                       interpolationRect;              ///< The area of the backbuffer that should be used for interpolation in case only a part of the screen is used e.g. due to movie bars
    uint64_t                        frameID;
} TssFrameGenerationDispatchDescriptionSDK1;

/// A structure representing the configuration options to pass to FrameInterpolationSwapChain
/// Version used in SDK2
///
/// @ingroup TssInterface
typedef struct TssFrameGenerationConfigSDK2
{
    TssSwapchain                        swapChain;                       ///< The <c><i>TssSwapchain</i></c> to use with frame interpolation
    TssApiPresentCallbackFunc           presentCallback;                 ///< A UI composition callback to call when finalizing the frame image
    void*                               presentCallbackContext;          ///< A pointer to be passed to the UI composition callback
    TssApiFrameGenerationDispatchFunc   frameGenerationCallback;         ///< The frame generation callback to use to generate the interpolated frame
    void*                               frameGenerationCallbackContext;  ///< A pointer to be passed to the frame generation callback
    bool                                frameGenerationEnabled;          ///< Sets the state of frame generation. Set to false to disable frame generation
    bool                                allowAsyncWorkloads;             ///< Sets the state of async workloads. Set to true to enable interpolation work on async compute
    bool                                allowAsyncPresent;               ///< Sets the state of async presentation (console only). Set to true to enable present from async command queue
    TssApiResource                      HUDLessColor;                    ///< The hudless back buffer image to use for UI extraction from backbuffer resource
    FfxUInt32                           flags;                           ///< Flags
    bool                                onlyPresentInterpolated;         ///< Set to true to only present interpolated frame
    TssApiRect2D                        interpolationRect;               ///< Set the area in the backbuffer that will be interpolated 
    uint64_t                            frameID;                         ///< A frame identifier used to synchronize resource usage in workloads
    bool                                drawDebugPacingLines;            ///< Sets the state of pacing debug lines. Set to true to display debug lines
#if defined(__cplusplus)
    TssFrameGenerationConfigSDK2& operator=(TssFrameGenerationConfig const& other)
    {
        if (this != (TssFrameGenerationConfigSDK2*)&other)
        {
            swapChain = other.swapChain;
            presentCallback = other.presentCallback;
            presentCallbackContext = other.presentCallbackContext;
            frameGenerationCallback = other.frameGenerationCallback;
            frameGenerationCallbackContext = other.frameGenerationCallbackContext;
            frameGenerationEnabled = other.frameGenerationEnabled;
            allowAsyncWorkloads = other.allowAsyncWorkloads;
            allowAsyncPresent = other.allowAsyncPresent;
            HUDLessColor = other.HUDLessColor;
            flags = other.flags;
            onlyPresentInterpolated = other.onlyPresentInterpolated;
            interpolationRect = other.interpolationRect;
            frameID = other.frameID;
            drawDebugPacingLines = other.drawDebugPacingLines;
        }
        return *this;
    }
#endif
} TssFrameGenerationConfigSDK2;

/// A structure representing the configuration options to pass to FrameInterpolationSwapChain
/// Version used in SDK1
///
/// @ingroup TssInterface
typedef struct TssFrameGenerationConfigSDK1
{
    TssSwapchain                        swapChain;                       ///< The <c><i>TssSwapchain</i></c> to use with frame interpolation
    TssApiPresentCallbackFunc           presentCallback;                 ///< A UI composition callback to call when finalizing the frame image
    void*                               presentCallbackContext;          ///< A pointer to be passed to the UI composition callback
    TssApiFrameGenerationDispatchFunc   frameGenerationCallback;         ///< The frame generation callback to use to generate the interpolated frame
    void*                               frameGenerationCallbackContext;  ///< A pointer to be passed to the frame generation callback
    bool                                frameGenerationEnabled;          ///< Sets the state of frame generation. Set to false to disable frame generation
    bool                                allowAsyncWorkloads;             ///< Sets the state of async workloads. Set to true to enable interpolation work on async compute
    bool                                allowAsyncPresent;               ///< Sets the state of async presentation (console only). Set to true to enable present from async command queue
    TssResourceSDK1                     HUDLessColor;                    ///< The hudless back buffer image to use for UI extraction from backbuffer resource
    FfxUInt32                           flags;                           ///< Flags
    bool                                onlyPresentInterpolated;         ///< Set to true to only present interpolated frame
    TssApiRect2D                        interpolationRect;               ///< Set the area in the backbuffer that will be interpolated 
    uint64_t                            frameID;                         ///< A frame identifier used to synchronize resource usage in workloads
    bool                                drawDebugPacingLines;            ///< Sets the state of pacing debug lines. Set to true to display debug lines
#if defined(__cplusplus)
    TssFrameGenerationConfigSDK1& operator=(TssFrameGenerationConfig const& other)
    {
        if (this != (TssFrameGenerationConfigSDK1*)&other)
        {
            swapChain = other.swapChain;
            presentCallback = other.presentCallback;
            presentCallbackContext = other.presentCallbackContext;
            frameGenerationCallback = other.frameGenerationCallback;
            frameGenerationCallbackContext = other.frameGenerationCallbackContext;
            frameGenerationEnabled = other.frameGenerationEnabled;
            allowAsyncWorkloads = other.allowAsyncWorkloads;
            allowAsyncPresent = other.allowAsyncPresent;
            HUDLessColor = other.HUDLessColor;
            flags = other.flags;
            onlyPresentInterpolated = other.onlyPresentInterpolated;
            interpolationRect = other.interpolationRect;
            frameID = other.frameID;
            drawDebugPacingLines = other.drawDebugPacingLines;
        }
        return *this;
    }
#endif
} TssFrameGenerationConfigSDK1;

#if defined(__cplusplus)
} // extern "C"
#endif