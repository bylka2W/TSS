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

#include "../include/gpu/frameinterpolation/tss_frameinterpolation_resources.h"

/// An enumeration of all the permutations that can be passed to the TSS Frame Interpolation algorithm.
///
/// TSS Frame Interpolation features are organized through a set of pre-defined compile
/// permutation options that need to be specified. Which shader blob
/// is returned for pipeline creation will be determined by what combination
/// of shader permutations are enabled.
///
/// @ingroup FRAMEINTERPOLATION
typedef enum FrameInterpolationShaderPermutationOptions
{
    FRAMEINTERPOLATION_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS = (1 << 0),
    FRAMEINTERPOLATION_SHADER_PERMUTATION_JITTER_MOTION_VECTORS  = (1 << 1),
    FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED         = (1 << 2),  ///< Indicates input resources were generated with inverted depth
    FRAMEINTERPOLATION_SHADER_PERMUTATION_FORCE_WAVE64           = (1 << 3),  ///< doesn't map to a define, selects different table
    FRAMEINTERPOLATION_SHADER_PERMUTATION_ALLOW_FP16             = (1 << 4),  ///< Enables fast math computations where possible
} FrameInterpolationShaderPermutationOptions;

typedef struct FrameInterpolationConstants
{
    int32_t renderSize[2];
    int32_t displaySize[2];

    float   displaySizeRcp[2];
    float   cameraNear;
    float   cameraFar;
    
    int32_t upscalerTargetSize[2];  // how is that different from display size?
    int     Mode;
    int     Reset;

    float   deviceToViewDepth[4];

    float   deltaTime;
    int     HUDLessAttachedFactor;
    int32_t distortionFieldSize[2];

    float   opticalFlowScale[2];
    int32_t opticalFlowBlockSize;
    uint32_t dispatchFlags;

    int32_t maxRenderSize[2];
    int     opticalFlowHalfResMode;
    int     numInstances;

    int32_t interpolationRectBase[2];
    int32_t interpolationRectSize[2];

    float   debugBarColor[3];
    uint32_t backBufferTransferFunction;

    float    minMaxLuminance[2];
    float    fTanHalfFOV;
    float   _pad1;

    float   jitter[2];
    float   motionVectorScale[2];
} FrameInterpolationConstants;

typedef struct InpaintingPyramidConstants {

    uint32_t                    mips;
    uint32_t                    numworkGroups;
    uint32_t                    workGroupOffset[2];
} InpaintingPyramidConstants;

struct TssDeviceCapabilities;
struct TssPipelineState;
struct TssApiResource;

typedef struct TssFrameInterpolationRenderDescription
{
    TssApiDimensions2D renderSize;
    TssApiDimensions2D upscaleSize;

    float cameraNear;
    float cameraFar;
    float cameraFovAngleVertical;
    float viewSpaceToMetersFactor;

    TssApiFloatCoords2D motionVectorScale;  ///< The scale factor to apply to motion vectors.
} TssFrameInterpolationRenderDescription;

// TssUpscaleContext_Private
// The private implementation of the TSS Frame Interpolation context.
typedef struct TssFrameInterpolationContext_Private {

    TssFrameInterpolationContextDescription     contextDescription;
    FfxUInt32                                   effectContextId;
    TssFrameInterpolationRenderDescription      renderDescription;
    FrameInterpolationConstants                 constants;
    InpaintingPyramidConstants                  inpaintingPyramidContants;
    TssDevice                                   device;
    TssDeviceCapabilities                       deviceCapabilities;

    // FrameInterpolation Pipelines
    TssPipelineState                            pipelineFiReconstructAndDilate;
    TssPipelineState                            pipelineFiSetup;
    TssPipelineState                            pipelineFiReconstructPreviousDepth;
    TssPipelineState                            pipelineFiGameMotionVectorField;
    TssPipelineState                            pipelineFiOpticalFlowVectorField;
    TssPipelineState                            pipelineFiDisocclusionMask;
    TssPipelineState                            pipelineFiScfi;
    TssPipelineState                            pipelineInpaintingPyramid;
    TssPipelineState                            pipelineInpainting;
    TssPipelineState                            pipelineGameVectorFieldInpaintingPyramid;
    TssPipelineState                            pipelineDebugView;

    FfxConstantBuffer                           constantBuffers[FFX_FRAMEINTERPOLATION_CONSTANTBUFFER_COUNT];

    // 2 arrays of resources, as e.g. TSS_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_LOCK_STATUS will use different resources when bound as SRV vs when bound as UAV
    TssResourceInternal                         srvResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNT];
    TssResourceInternal                         uavResources[FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_COUNT];

    bool                                        firstExecution;
    bool                                        refreshPipelineStates;

    bool                                        asyncSupported;
    uint64_t                                    previousFrameID;
    uint64_t                                    dispatchCount;

} TssFrameInterpolationContext_Private;
