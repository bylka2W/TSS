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

/// @defgroup DX12Backend DX12 Backend
/// TSS SDK native backend implementation for DirectX 12.
/// 
/// @ingroup Backends

/// @defgroup DX12FrameInterpolation DX12 FrameInterpolation
/// TSS SDK native frame interpolation implementation for DirectX 12 backend.
/// 
/// @ingroup DX12Backend

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include "../../api/internal/tss_interface.h"
#include "../../api/include/dx12/tss_api_dx12.h"

#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

/// Query how much memory is required for the DirectX 12 backend's scratch buffer.
/// 
/// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
///                                         (Note that some effects contain internal contexts which count towards this maximum)
///
/// @returns
/// The size (in bytes) of the required scratch memory buffer for the DX12 backend.
/// @ingroup DX12Backend
FFX_API size_t tssGetScratchMemorySizeDX12(size_t maxContexts);

/// Create a <c><i>TssDevice</i></c> from a <c><i>ID3D12Device</i></c>.
///
/// @param [in] device                      A pointer to the DirectX12 device.
///
/// @returns
/// An abstract TSS device.
///
/// @ingroup DX12Backend
FFX_API TssDevice tssGetDeviceDX12(ID3D12Device* device);

/// Populate an interface with pointers for the DX12 backend.
///
/// @param [out] backendInterface           A pointer to a <c><i>TssInterface</i></c> structure to populate with pointers.
/// @param [in] device                      A pointer to the DirectX12 device.
/// @param [in] scratchBuffer               A pointer to a buffer of memory which can be used by the DirectX(R)12 backend.
/// @param [in] scratchBufferSize           The size (in bytes) of the buffer pointed to by <c><i>scratchBuffer</i></c>.
/// @param [in] maxContexts                 The maximum number of simultaneous effect contexts that will share the backend.
///                                         (Note that some effects contain internal contexts which count towards this maximum)
///
/// @retval
/// FFX_OK                                  The operation completed successfully.
/// @retval
/// FFX_ERROR_CODE_INVALID_POINTER          The <c><i>interface</i></c> pointer was <c><i>NULL</i></c>.
///
/// @ingroup DX12Backend
FFX_API TssErrorCode tssGetInterfaceDX12(
    TssInterface* backendInterface,
    TssDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize, 
    size_t maxContexts);

/// Create a <c><i>TssCommandList</i></c> from a <c><i>ID3D12CommandList</i></c>.
///
/// @param [in] cmdList                     A pointer to the DirectX12 command list.
///
/// @returns
/// An abstract TSS command list.
///
/// @ingroup DX12Backend
FFX_API TssCommandList tssGetCommandListDX12(ID3D12CommandList* cmdList);

/// Create a <c><i>TssPipeline</i></c> from a <c><i>ID3D12PipelineState</i></c>.
///
/// @param [in] pipelineState               A pointer to the DirectX12 pipeline state.
///
/// @returns
/// An abstract TSS pipeline.
///
/// @ingroup DX12Backend
FFX_API TssPipeline tssGetPipelineDX12(ID3D12PipelineState* pipelineState);

/// Fetch a <c><i>TssApiResource</i></c> from a <c><i>GPUResource</i></c>.
///
/// @param [in] dx12Resource                A pointer to the DX12 resource.
/// @param [in] ffxResDescription           An <c><i>TssApiResourceDescription</i></c> for the resource representation.
/// @param [in] ffxResName                  (optional) A name string to identify the resource in debug mode.
/// @param [in] state                       The state the resource is currently in.
///
/// @returns
/// An abstract TSS resources.
///
/// @ingroup DX12Backend
FFX_API TssApiResource tssGetResourceDX12(const ID3D12Resource*  dx12Resource,
                                       TssApiResourceDescription ffxResDescription,
                                       const wchar_t*         ffxResName,
                                       uint32_t state = TSS_API_RESOURCE_STATE_COMPUTE_READ);

/// Loads TSS AGS runtime dll to allow SDK calls to show up in Radeon GPU Profiler.
///
/// @param [in] agsDllPath                  The path to the DLL to load.
///
/// @retval
/// FFX_OK                                  The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_PATH                  Could not load the DLL using the provided path.
/// @retval
/// FFX_ERROR_BACKEND_API_ERROR             Could not get proc addresses for
///                                         agsDriverExtensionsDX12_PushMarker and/or
///                                         agsDriverExtensionsDX12_PopMarker and/or
///                                         agsDriverExtensionsDX12_SetMarker.
///
/// @ingroup DX12Backend
FFX_API TssErrorCode tssLoadAgsDll(const wchar_t* agsDllPath);

/// Loads PIX runtime dll to allow SDK calls to show up in Microsoft PIX.
///
/// @param [in] pixDllPath                  The path to the DLL to load.
///
/// @retval
/// FFX_OK                                  The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_PATH                  Could not load the DLL using the provided path.
/// @retval
/// FFX_ERROR_BACKEND_API_ERROR             Could not get proc addresses for
///                                         PIXBeginEvent and/or
///                                         PIXEndEvent and/or
///                                         PIXSetMarker
///
/// @ingroup DX12Backend
FFX_API TssErrorCode tssLoadPixDll(const wchar_t* pixDllPath);

/// Toggles debugging.
/// 
/// @param [in]  backendInterface                    A pointer to the backend interface.
/// @param [in]  effectContextId                     The context space to be used for the effect in question.
/// @param [in]  flag                                Flag indicating whether debugging should be enabled.
/// 
/// @ingroup DX12Backend
FFX_API void tssToggleDebuggingDX12(TssInterface* backendInterface, FfxUInt32 effectContextId, bool flag);

/// Fetch a <c><i>TssApiSurfaceFormat</i></c> from a DXGI_FORMAT.
///
/// @param [in] format              The DXGI_FORMAT to convert to <c><i>TssApiSurfaceFormat</i></c>.
///
/// @returns
/// An <c><i>TssApiSurfaceFormat</i></c>.
///
/// @ingroup DX12Backend
FFX_API TssApiSurfaceFormat tssGetSurfaceFormatDX12(DXGI_FORMAT format);

/// Fetch a DXGI_FORMAT from a <c><i>TssApiSurfaceFormat</i></c>.
///
/// @param [in] surfaceFormat       The <c><i>TssApiSurfaceFormat</i></c> to convert to DXGI_FORMAT.
///
/// @returns
/// An DXGI_FORMAT.
///
/// @ingroup DX12Backend
FFX_API DXGI_FORMAT tssGetDX12FormatFromSurfaceFormat(TssApiSurfaceFormat surfaceFormat);

/// Fetch a <c><i>TssApiResourceDescription</i></c> from an existing ID3D12Resource.
///
/// @param [in] pResource           The ID3D12Resource resource to create a <c><i>TssApiResourceDescription</i></c> for.
/// @param [in] additionalUsages    Optional <c><i>TssApiResourceUsage</i></c> flags needed for select resource mapping.
///
/// @returns
/// An <c><i>TssApiResourceDescription</i></c>.
///
/// @ingroup DX12Backend
FFX_API TssApiResourceDescription tssGetResourceDescriptionDX12(const ID3D12Resource* pResource, TssApiResourceUsage additionalUsages = TSS_API_RESOURCE_USAGE_READ_ONLY);

/// Fetch a <c><i>TssCommandQueue</i></c> from an existing ID3D12CommandQueue.
///
/// @param [in] pCommandQueue       The ID3D12CommandQueue to create a <c><i>TssCommandQueue</i></c> from.
///
/// @returns
/// An <c><i>TssCommandQueue</i></c>.
///
/// @ingroup DX12Backend
FFX_API TssCommandQueue tssGetCommandQueueDX12(ID3D12CommandQueue* pCommandQueue);

/// Fetch a <c><i>TssSwapchain</i></c> from an existing IDXGISwapChain4.
///
/// @param [in] pSwapchain          The IDXGISwapChain4 to create a <c><i>TssSwapchain</i></c> from.
///
/// @returns
/// An <c><i>TssSwapchain</i></c>.
///
/// @ingroup DX12Backend
FFX_API TssSwapchain tssGetSwapchainDX12(IDXGISwapChain4* pSwapchain);

/// Fetch a IDXGISwapChain4 from an existing <c><i>TssSwapchain</i></c>.
///
/// @param [in] TssSwapchain          The <c><i>TssSwapchain</i></c> to fetch an IDXGISwapChain4 from.
///
/// @returns
/// An IDXGISwapChain4 object.
///
/// @ingroup DX12Backend
FFX_API IDXGISwapChain4* tssGetDX12SwapchainPtr(TssSwapchain TssSwapchain);

/// Replaces the current swapchain with the provided <c><i>TssSwapchain</i></c>.
///
/// @param [in] gameQueue               The <c><i>TssCommandQueue</i></c> presentation will occur on.
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c> to use for frame interpolation presentation.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          One of the parameters is invalid.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssReplaceSwapchainForFrameinterpolationDX12(TssCommandQueue gameQueue, TssSwapchain& gameSwapChain);

/// Creates a <c><i>TssSwapchain</i></c> from passed in parameters.
///
/// @param [in] desc                    The DXGI_SWAP_CHAIN_DESC describing the swapchain creation parameters from the calling application.
/// @param [in] queue                   The ID3D12CommandQueue to use for frame interpolation presentation.
/// @param [in] dxgiFactory             The IDXGIFactory to use for DX12 swapchain creation.
/// @param [out] outGameSwapChain       The created <c><i>TssSwapchain</i></c>.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          One of the parameters is invalid.
/// FFX_ERROR_OUT_OF_MEMORY             Insufficient memory available to allocate <c><i>TssSwapchain</i></c> or underlying component.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssCreateFrameinterpolationSwapchainDX12(const DXGI_SWAP_CHAIN_DESC* desc,
                                                      ID3D12CommandQueue* queue,
                                                      IDXGIFactory* dxgiFactory,
                                                      TssSwapchain& outGameSwapChain);

/// Creates a <c><i>TssSwapchain</i></c> from passed in parameters.
///
/// @param [in] hWnd                    The HWND handle for the calling application.
/// @param [in] desc1                   The DXGI_SWAP_CHAIN_DESC1 describing the swapchain creation parameters from the calling application.
/// @param [in] fullscreenDesc          The DXGI_SWAP_CHAIN_FULLSCREEN_DESC describing the full screen swapchain creation parameters from the calling application.
/// @param [in] queue                   The ID3D12CommandQueue to use for frame interpolation presentation.
/// @param [in] dxgiFactory             The IDXGIFactory to use for DX12 swapchain creation.
/// @param [out] outGameSwapChain       The created <c><i>TssSwapchain</i></c>.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          One of the parameters is invalid.
/// FFX_ERROR_OUT_OF_MEMORY             Insufficient memory available to allocate <c><i>TssSwapchain</i></c> or underlying component.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssCreateFrameinterpolationSwapchainForHwndDX12(HWND hWnd,
                                                             const DXGI_SWAP_CHAIN_DESC1* desc1,
                                                             const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc,
                                                             ID3D12CommandQueue* queue,
                                                             IDXGIFactory* dxgiFactory,
                                                             TssSwapchain& outGameSwapChain);

/// Waits for the <c><i>TssSwapchain</i></c> to complete presentation.
///
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c> to wait on.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssWaitForPresents(TssSwapchain gameSwapChain);

/// Registers a <c><i>TssApiResource</i></c> to use for UI with the provided <c><i>TssSwapchain</i></c>.
///
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c> to to register the UI resource with.
/// @param [in] uiResource              The <c><i>TssApiResource</i></c> representing the UI resource.
/// @param [in] flags                   A set of <c><i>FfxUiCompositionFlags</i></c>.
/// 
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssRegisterFrameinterpolationUiResourceDX12(TssSwapchain gameSwapChain, TssApiResource uiResource, uint32_t flags);

/// Fetches a <c><i>TssCommandList</i></c> from the <c><i>TssSwapchain</i></c>.
///
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c> to get a <c><i>TssCommandList</i></c> from.
/// @param [out] gameCommandlist        The <c><i>TssCommandList</i></c> from the provided <c><i>TssSwapchain</i></c>.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssGetFrameinterpolationCommandlistDX12(TssSwapchain gameSwapChain, TssCommandList& gameCommandlist);

/// Fetches a <c><i>TssApiResource</i></c>  representing the backbuffer from the <c><i>TssSwapchain</i></c>.
///
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c> to get a <c><i>TssApiResource</i></c> backbuffer from.
///
/// @returns
/// An abstract TSS resources for the swapchain backbuffer.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssApiResource tssGetFrameinterpolationTextureDX12(TssSwapchain gameSwapChain);

/// Sets a <c><i>TssFrameGenerationConfig</i></c> to the internal FrameInterpolationSwapChain (in the backend).
///
/// @param [in] config                  The <c><i>TssFrameGenerationConfig</i></c> to set.
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssSetFrameGenerationConfigToSwapchainDX12(TssFrameGenerationConfig const* config);

//enum value matches enum TssApiConfigureFrameGenerationSwapChainKeyDX12
typedef enum TssFrameInterpolationSwapchainConfigureKey
{
    TSS_FI_SWAPCHAIN_CONFIGURE_KEY_WAITCALLBACK = 0,
    TSS_FI_SWAPCHAIN_CONFIGURE_KEY_FRAMEPACINGTUNING = 2,
} TssFrameInterpolationSwapchainConfigureKey;

/// Configures <c><i>TssSwapchain</i></c> via KeyValue API post <c><i>TssSwapchain</i></c> context creation
///
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c> to configure via KeyValue API
/// @param [in] key                     The <c><i>TssFrameInterpolationSwapchainConfigureKey</i></c> is key
/// @param [in] valuePtr                The <c><i><void *></i></c> pointer to value. What this pointer deference to depends on key.
/// 
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssConfigureFrameInterpolationSwapchainDX12(TssSwapchain gameSwapChain, TssFrameInterpolationSwapchainConfigureKey key, void* valuePtr);

/// Query how much GPU memory created by <c><i>TssSwapchain</i></c>. This excludes GPU memory created by DXGI (ie. size of DXGI swapchaim backbuffers).
///
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c>
/// @param [in out] vramUsage           The <c><i>TssApiEffectMemoryUsage</i></c> is the GPU memory created by FrameInterpolationSwapchain
/// 
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssFrameInterpolationSwapchainGetGpuMemoryUsageDX12(TssSwapchain gameSwapChain, TssApiEffectMemoryUsage* vramUsage);

/// Query how much GPU memory created by <c><i>TssSwapchain</i></c>. This excludes GPU memory created by DXGI (ie. size of DXGI swapchaim backbuffers).
///
/// @param [in] device                  The <c><i>TssDevice</i></c>
/// @param [in] displaySize             A pointer to a <c><i>TssApiDimensions2D</i></c> structure.
/// @param [in] backbufferFormat        The <c><i>TssApiSurfaceFormat</i></c> structure.
/// @param [in] backbufferCount         The <c><i>uint32_t</i></c>.
/// @param [in] uiResourceSize          A pointer to a <c><i>TssApiDimensions2D</i></c> structure.
/// @param [in] uiResourceFormat        The <c><i>TssApiSurfaceFormat</i></c> structure.
/// @param [in] flags                   The <c><i>uint32_t</i></c> value
/// @param [in out] vramUsage           The <c><i>TssApiEffectMemoryUsage</i></c> is the GPU memory created by FrameInterpolationSwapchain
/// 
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query the interface for the frame interpolation swap chain.
///
/// @ingroup DX12FrameInterpolation
FFX_API TssErrorCode tssFrameInterpolationSwapchainGetGpuMemoryUsageDX12V2(TssDevice device, TssApiDimensions2D* displaySize, TssApiSurfaceFormat backbufferFormat, uint32_t backbufferCount, TssApiDimensions2D* uiResourceSize, TssApiSurfaceFormat uiResourceFormat, uint32_t flags, TssApiEffectMemoryUsage* vramUsage);


struct TssFrameInterpolationContext;
typedef TssErrorCode (*TssCreateFiSwapchain)(TssFrameInterpolationContext* fiContext, TssDevice device, TssCommandQueue gameQueue, TssSwapchain& swapchain);
typedef TssErrorCode (*TssReleaseFiSwapchain)(TssFrameInterpolationContext* fiContext, TssSwapchain* outRealSwapchain);

/// Query estimated GPU memory size of a resource description
///
/// @param [in] device                  The <c><i>TssDevice</i></c>
/// @param [in] createResourceDescription The <c><i>FfxCreateResourceDescription</i></c>
/// @param [out] sizeInBytes            The <c><i>uint64_t</i></c>
///
/// @retval
/// FFX_OK                              The operation completed successfully.
/// @retval
/// FFX_ERROR_INVALID_ARGUMENT          Could not query <c><i>ID3D12Device</i></c> from <c><i>TssDevice</i></c> to call 
///
/// @ingroup DX12Backend
FFX_API TssErrorCode tssGetResourceSizeFromDescriptionDX12(TssDevice device, const FfxCreateResourceDescription* createResourceDescription, uint64_t* sizeInBytes, uint64_t* alignment = nullptr);

/// Query The ABI version used by the swapchain
///
/// @param [in] gameSwapChain           The <c><i>TssSwapchain</i></c>
///
/// @retval
/// TSS_ABI_INVALID The ABI is Invalid or an error occurred
/// @retval
/// TSS_ABI_OLD The ABI predates TSS 3.1.4 and only supports upscaler replacement
/// @retval
/// TSS_ABI_1_1_4 The TSS 3.1.4 release - first one to support frame-gen replacement
/// @retval
/// TSS_ABI_1_1_5 The ABI is for a patch release for specific titles
/// @retval
/// TSS_ABI_2_0_0 The FSR4 release
/// @retval
/// TSS_ABI_VALID The latest stable ABI, currently a synonym for TSS_ABI_2_0_0
///
/// @ingroup DX12Backend
FFX_API TssABIVersion tssGetSwapchainABIDX12(TssSwapchain gameSwapChain);

FFX_API void tssRegisterConstantBufferAllocatorDX12(TssApiConstantBufferAllocator fpConstantAllocator);
FFX_API void tssRegisterResourceAllocatorDX12(PfnTssResourceAllocatorFunc fpResourceAllocator);
FFX_API void tssRegisterResourceDeallocatorDX12(PfnTssResourceDeallocatorFunc fpResourceDeallocator);
FFX_API void tssRegisterHeapAllocatorDX12(PfnFfxHeapAllocatorFunc fpHeapAllocator);
FFX_API void tssRegisterHeapDeallocatorDX12(PfnFfxHeapDeallocatorFunc fpHeapDeallocator);

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)
