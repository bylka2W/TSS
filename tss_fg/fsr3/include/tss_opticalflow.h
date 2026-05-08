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

// @defgroup OpticalFlow

#pragma once

// Include the interface for the backend of the OpticalFlow API.
#include "../../../api/internal/tss_interface.h"

/// TSS Optical Flow context count
///
/// Defines the number of internal effect contexts required by Optical Flow
///
/// @ingroup tssOpticalFlow
#define TSS_OPTICALFLOW_CONTEXT_COUNT (1)

/// The size of the context specified in 32bit size units.
///
/// @ingroup tssOpticalFlow
#define TSS_OPTICALFLOW_CONTEXT_SIZE (FFX_SDK_DEFAULT_CONTEXT_SIZE)

#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

/// An enumeration of all the passes which constitute the OpticalFlow algorithm.
///
/// @ingroup tssOpticalFlow
typedef enum tssOpticalFlowPass
{
    TSS_OPTICALFLOW_PASS_PREPARE_LUMA = 0,
    TSS_OPTICALFLOW_PASS_GENERATE_OPTICAL_FLOW_INPUT_PYRAMID,
    TSS_OPTICALFLOW_PASS_GENERATE_SCD_HISTOGRAM,
    TSS_OPTICALFLOW_PASS_COMPUTE_SCD_DIVERGENCE,
    TSS_OPTICALFLOW_PASS_COMPUTE_OPTICAL_FLOW_ADVANCED_V5,
    TSS_OPTICALFLOW_PASS_FILTER_OPTICAL_FLOW_V5,
    TSS_OPTICALFLOW_PASS_SCALE_OPTICAL_FLOW_ADVANCED_V5,

    TSS_OPTICALFLOW_PASS_COUNT
} tssOpticalFlowPass;

/// An enumeration of bit flags used when creating a
/// <c><i>tssOpticalFlowContext</i></c>. See <c><i>tssOpticalFlowDispatchDescription</i></c>.
///
/// @ingroup tssOpticalFlow
typedef enum tssOpticalFlowInitializationFlagBits
{
    TSS_OPTICALFLOW_ENABLE_TEXTURE1D_USAGE = (1 << 0),

} tssOpticalFlowInitializationFlagBits;

/// A structure encapsulating the parameters required to initialize 
/// TSS OpticalFlow.
///
/// @ingroup tssOpticalFlow
typedef struct tssOpticalFlowContextDescription {

    TssInterface                backendInterface;       ///< A set of pointers to the backend implementation for TSS SDK
    uint32_t                    flags;                  ///< A collection of <c><i>tssOpticalFlowInitializationFlagBits</i></c>.
    TssApiDimensions2D             resolution;
} tssOpticalFlowContextDescription;

/// A structure encapsulating the parameters for dispatching the various passes
/// of TSS Opticalflow.
///
/// @ingroup tssOpticalFlow
typedef struct tssOpticalFlowDispatchDescription
{
    TssCommandList   commandList;       ///< The <c><i>TssCommandList</i></c> to record rendering commands into.
    TssApiResource      color;             ///< A <c><i>TssApiResource</i></c> containing the input color buffer 
    TssApiResource      opticalFlowVector; ///< A <c><i>TssApiResource</i></c> containing the output motion buffer 
    TssApiResource      opticalFlowSCD;    ///< A <c><i>TssApiResource</i></c> containing the output scene change detection buffer 
    bool             reset;             ///< A boolean value which when set to true, indicates the camera has moved discontinuously.
    int              backbufferTransferFunction;
    TssApiFloatCoords2D minMaxLuminance;
} tssOpticalFlowDispatchDescription;

/// A structure encapsulating the resource descriptions for internal resources for this effect.
///
/// @ingroup tssOpticalFlow

typedef struct tssOpticalFlowInternalResourceDescriptions
{
    FfxCreateResourceDescription opticalFlowInput1;
    FfxCreateResourceDescription opticalFlowInput1Level1;
    FfxCreateResourceDescription opticalFlowInput1Level2;
    FfxCreateResourceDescription opticalFlowInput1Level3;
    FfxCreateResourceDescription opticalFlowInput1Level4;
    FfxCreateResourceDescription opticalFlowInput1Level5;
    FfxCreateResourceDescription opticalFlowInput1Level6;
    FfxCreateResourceDescription opticalFlowInput2;
    FfxCreateResourceDescription opticalFlowInput2Level1;
    FfxCreateResourceDescription opticalFlowInput2Level2;
    FfxCreateResourceDescription opticalFlowInput2Level3;
    FfxCreateResourceDescription opticalFlowInput2Level4;
    FfxCreateResourceDescription opticalFlowInput2Level5;
    FfxCreateResourceDescription opticalFlowInput2Level6;
    FfxCreateResourceDescription opticalFlow1;
    FfxCreateResourceDescription opticalFlow1Level1;
    FfxCreateResourceDescription opticalFlow1Level2;
    FfxCreateResourceDescription opticalFlow1Level3;
    FfxCreateResourceDescription opticalFlow1Level4;
    FfxCreateResourceDescription opticalFlow1Level5;
    FfxCreateResourceDescription opticalFlow1Level6;
    FfxCreateResourceDescription opticalFlow2;
    FfxCreateResourceDescription opticalFlow2Level1;
    FfxCreateResourceDescription opticalFlow2Level2;
    FfxCreateResourceDescription opticalFlow2Level3;
    FfxCreateResourceDescription opticalFlow2Level4;
    FfxCreateResourceDescription opticalFlow2Level5;
    FfxCreateResourceDescription opticalFlow2Level6;
    FfxCreateResourceDescription opticalFlowSCDHistogram;
    FfxCreateResourceDescription opticalFlowSCDPreviousHistogram;
    FfxCreateResourceDescription opticalFlowSCDTemp;
} tssOpticalFlowInternalResourceDescriptions;

/// A structure encapsulating the resource descriptions for shared resources for this effect.
///
/// @ingroup tssOpticalFlow
typedef struct tssOpticalFlowSharedResourceDescriptions {

    FfxCreateResourceDescription opticalFlowVector;
    FfxCreateResourceDescription opticalFlowSCD;

} tssOpticalFlowSharedResourceDescriptions;

/// A structure encapsulating the TSS OpticalFlow context.
///
/// This sets up an object which contains all persistent internal data and
/// resources that are required by OpticalFlow.
///
/// The <c><i>tssOpticalFlowContext</i></c> object should have a lifetime matching
/// your use of OpticalFlow. Before destroying the OpticalFlow context care should be taken
/// to ensure the GPU is not accessing the resources created or used by OpticalFlow.
/// It is therefore recommended that the GPU is idle before destroying OpticalFlow
/// OpticalFlow context.
///
/// @ingroup tssOpticalFlow
typedef struct tssOpticalFlowContext
{
    uint32_t data[TSS_OPTICALFLOW_CONTEXT_SIZE];  ///< An opaque set of <c>uint32_t</c> which contain the data for the context.
} tssOpticalFlowContext;


/// Create a TSS OpticalFlow context from the parameters
/// programmed to the <c><i>tssOpticalFlowContextDescription</i></c> structure.
///
/// The context structure is the main object used to interact with the OpticalFlow
/// API, and is responsible for the management of the internal resources used
/// by the OpticalFlow algorithm. When this API is called, multiple calls will be
/// made via the pointers contained in the <c><i>callbacks</i></c> structure.
/// These callbacks will attempt to retreive the device capabilities, and
/// create the internal resources, and pipelines required by OpticalFlow's
/// frame-to-frame function. Depending on the precise configuration used when
/// creating the <c><i>tssOpticalFlowContext</i></c> a different set of resources and
/// pipelines might be requested via the callback functions.
///
/// The flags included in the <c><i>flags</i></c> field of
/// <c><i>tssOpticalFlowContext</i></c> how match the configuration of your
/// application as well as the intended use of OpticalFlow. It is important that these
/// flags are set correctly (as well as a correct programmed
/// <c><i>tssOpticalFlowContextDescription</i></c>) to ensure correct operation. It is
/// recommended to consult the overview documentation for further details on
/// how OpticalFlow should be integerated into an application.
///
/// When the <c><i>tssOpticalFlowContext</i></c> is created, you should use the
/// <c><i>tssOpticalFlowContextDispatch</i></c> function each frame where TSS
/// Frame Interpolation should be applied. See the documentation of
/// <c><i>tssOpticalFlowContextDispatch</i></c> for more details.
///
/// The <c><i>tssOpticalFlowContext</i></c> should be destroyed when use of it is
/// completed, typically when an application is unloaded or OpticalFlow is
/// disabled by a user. To destroy the OpticalFlow context you should call
/// <c><i>tssOpticalFlowContextDestroy</i></c>.
///
/// @param [out] context                A pointer to a <c><i>tssOpticalFlowContext</i></c> structure to populate.
/// @param [in]  contextDescription     A pointer to a <c><i>tssOpticalFlowContextDescription</i></c> structure.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> or <c><i>contextDescription</i></c> was <c><i>NULL</i></c>.
/// @retval
/// FFX_ERROR_INCOMPLETE_INTERFACE      The operation failed because the <c><i>tssOpticalFlowContextDescription.callbacks</i></c>  was not fully specified.
/// @retval
/// FFX_ERROR_BACKEND_API_ERROR         The operation failed because of an error returned from the backend.
///
/// @ingroup tssOpticalFlow
FFX_API TssErrorCode tssOpticalFlowContextCreate(tssOpticalFlowContext* context, tssOpticalFlowContextDescription* contextDescription);

/// Get GPU memory usage of the TSS Frame Interpolation's Shared context.
///
/// @param [in]  pContext                A pointer to a <c><i>TssInterface</i></c> structure.
/// @param [out] pVramUsage              A pointer to a <c><i>TssApiEffectMemoryUsage</i></c> structure.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> or <c><i>vramUsage</i></c> were <c><i>NULL</i></c>.
///
/// @ingroup tssOpticalFlow
FFX_API TssErrorCode tssOpticalFlowContextGetGpuMemoryUsage(tssOpticalFlowContext* pContext, TssApiEffectMemoryUsage* vramUsage);

/// Get GPU memory usage of the TSS OpticalFlow.
///
/// @param [in]  device                  A <c><i>TssDevice</i></c>.
/// @param [in]  displaySize             A pointer to a <c><i>TssApiDimensions2D</i></c> structure.
/// @param [out] pVramUsage              A pointer to a <c><i>TssApiEffectMemoryUsage</i></c> structure.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> or <c><i>vramUsage</i></c> were <c><i>NULL</i></c>.
///
/// @ingroup tssOpticalFlow
FFX_API TssErrorCode tssOpticalFlowGetGpuMemoryUsage(TssDevice device, TssApiDimensions2D* displaySize, TssApiEffectMemoryUsage* pVramUsage);

/// Provides the descriptions for shared resources that must be allocated for this effect.
///
/// @param [in] context                    A pointer to a <c><i>tssOpticalFlowContext</i></c> structure.
/// @param [out] sharedResources           A pointer to a <c><i>tssOpticalFlowSharedResourceDescriptions</i></c> to populate.
///
/// @returns
/// FFX_OK                                The operation completed successfully.
/// @returns
/// Anything else                        The operation failed.
///
/// @ingroup tssFrameInterpolation
FFX_API TssErrorCode tssOpticalFlowGetSharedResourceDescriptions(tssOpticalFlowContext* context, tssOpticalFlowSharedResourceDescriptions* sharedResources);

FFX_API TssErrorCode tssOpticalFlowContextDispatch(tssOpticalFlowContext* context, const tssOpticalFlowDispatchDescription* dispatchDescription);

/// Destroy the TSS OpticalFlow context.
///
/// @param [out] context                A pointer to a <c><i>tssOpticalFlowContext</i></c> structure to destroy.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_NULL_POINTER         The operation failed because either <c><i>context</i></c> was <c><i>NULL</i></c>.
///
/// @ingroup tssOpticalFlow
FFX_API TssErrorCode tssOpticalFlowContextDestroy(tssOpticalFlowContext* context);

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)
