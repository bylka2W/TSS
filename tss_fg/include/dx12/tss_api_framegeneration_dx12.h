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
#include "../../../api/include/dx12/tss_api_dx12.h"

#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MAJOR 3
#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MINOR 1
#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_PATCH 6

#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION                           FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MAJOR, FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MINOR, FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_PATCH)

#define TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x01)
struct tssCreateContextDescFrameGenerationSwapChainWrapDX12
{
    tssCreateContextDescHeader header;     ///< Description header for frame generation swapchain wrapping context creation.
    IDXGISwapChain4**          swapchain;  ///< Input swap chain to wrap with frame interpolation. On return, contains the wrapped frame interpolation swapchain.
    ID3D12CommandQueue*        gameQueue;  ///< The command queue to be used for presentation.
};

#define TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x05)
struct tssCreateContextDescFrameGenerationSwapChainNewDX12
{
    tssCreateContextDescHeader header;       ///< Description header for frame generation swapchain creation.
    IDXGISwapChain4**          swapchain;    ///< Output pointer to receive the created frame interpolation swapchain.
    DXGI_SWAP_CHAIN_DESC*      desc;         ///< Swap chain creation parameters.
    IDXGIFactory*              dxgiFactory;  ///< The DXGI factory to use for DX12 swapchain creation.
    ID3D12CommandQueue*        gameQueue;    ///< The command queue to be used for presentation.
};

#define TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x06)
struct tssCreateContextDescFrameGenerationSwapChainForHwndDX12
{
    tssCreateContextDescHeader       header;          ///< Description header for frame generation swapchain creation for HWND.
    IDXGISwapChain4**                swapchain;       ///< Output pointer to receive the created frame interpolation swapchain.
    HWND                             hwnd;            ///< Window handle for the calling application.
    DXGI_SWAP_CHAIN_DESC1*           desc;            ///< Swap chain creation parameters.
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc;  ///< Fullscreen swap chain creation parameters (can be NULL for windowed mode).
    IDXGIFactory*                    dxgiFactory;     ///< The DXGI factory to use for DX12 swapchain creation.
    ID3D12CommandQueue*              gameQueue;       ///< The command queue to be used for presentation.
};

#define TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x02)
struct tssConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12
{
    tssConfigureDescHeader header;      ///< Description header for UI resource registration configuration.
    struct TssApiResource  uiResource;  ///< Resource containing user interface to composite onto generated frames. May be empty.
    uint32_t               flags;       ///< UI composition flags. Zero or combination of values from TssApiUiCompositionFlags.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x03)
struct tssQueryDescFrameGenerationSwapChainInterpolationCommandListDX12
{
    tssQueryDescHeader header;           ///< Description header for interpolation command list query.
    void**             pOutCommandList;  ///< Output pointer to receive the command list (ID3D12GraphicsCommandList) to be used for frame generation dispatch.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x04)
struct tssQueryDescFrameGenerationSwapChainInterpolationTextureDX12
{
    tssQueryDescHeader     header;       ///< Description header for interpolation texture query.
    struct TssApiResource* pOutTexture;  ///< Output pointer to receive the resource in which the frame interpolation result should be placed.
};

#define TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x07)
struct tssDispatchDescFrameGenerationSwapChainWaitForPresentsDX12
{
    tssDispatchDescHeader header;  ///< Description header for wait for presents dispatch.
};

#define TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x08)
struct tssConfigureDescFrameGenerationSwapChainKeyValueDX12
{
    tssConfigureDescHeader header;  ///< Description header for key-value configuration.
    uint64_t               key;     ///< Configuration key from the TssApiConfigureFrameGenerationSwapChainKeyDX12 enumeration.
    uint64_t               u64;     ///< Integer or enum value to set.
    void*                  ptr;     ///< Pointer value to set or pointer to structure containing value to set.
};

//enum value matches enum TssFrameInterpolationSwapchainConfigureKey
enum TssApiConfigureFrameGenerationSwapChainKeyDX12
{
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_WAITCALLBACK      = 0,  ///< Sets TssWaitCallbackFunc.
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING = 2,  ///< Sets TssApiSwapchainFramePacingTuning.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x09)
struct tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12
{
    tssQueryDescHeader              header;                                  ///< Description header for GPU memory usage query.
    struct TssApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;  ///< Output pointer to receive GPU memory usage information for frame generation swapchain.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12_V2 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x0a)
struct tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2
{
    tssQueryDescHeader              header;                                  ///< Description header for GPU memory usage query.
    void*                           device;                                  ///< The GPU device (pointer to ID3D12Device). App needs to fill out before Query() call.
    struct TssApiDimensions2D       displaySize;                             ///< The resolution at which both rendered and generated frames will be displayed. App needs to fill out before Query() call.
    uint32_t                        backBufferFormat;                        ///< The surface format for the backbuffer. One of the values from TssApiSurfaceFormat. App needs to fill out before Query() call.
    uint32_t                        backBufferCount;                         ///< The number of backbuffers in the swapchain. App needs to fill out before Query() call.
    struct TssApiDimensions2D       uiResourceSize;                          ///< The resolution of the resource that will be used for UI composition. Set to (0,0) if providing null uiResource in  tssConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12. App needs to fill out before Query() call.
    uint32_t                        uiResourceFormat;                        ///< The surface format for the uiResource. One of the values from TssApiSurfaceFormat. Set to TSS_API_SURFACE_FORMAT_UNKNOWN(0) if providing null uiResource in  tssConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12. App needs to fill out before Query() call.
    uint32_t                        flags;                                   ///< UI composition flags. Zero or combination of values from TssApiUiCompositionFlags. App needs to fill out before Query() call.
    struct TssApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;  ///< Output pointer to receive GPU memory usage information for frame generation swapchain. Populated by Query() call.
};

#define TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12 TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(TSS_API_BACKEND_ID_DX12, TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN, 0x0b)
struct tssCreateContextDescFrameGenerationSwapChainVersionDX12
{
    tssCreateContextDescHeader header;   ///< Description header for frame generation swapchain version context creation.
    uint32_t                   version;  ///< The API version the application was built against. This must be set to FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION.
};
