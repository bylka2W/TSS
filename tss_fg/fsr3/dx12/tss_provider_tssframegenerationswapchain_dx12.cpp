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

#include "../../include/dx12/tss_api_framegeneration_dx12.hpp"
#include "../../../backend/dx12/tss_dx12.h"
#include "../../include/ffx_framegeneration.hpp"
#include "../include/tss_provider_framegenerationswapchain.h"
#include "../../../framegeneration/include/ffx_framegeneration.h"

#include <stdlib.h>

#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR (FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MAJOR)
#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR (FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MINOR)
#define FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH (FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_PATCH)

bool tssProvider_FrameGenerationSwapChain::CanProvide(uint64_t type) const
{
    return ((type & TSS_API_EFFECT_MASK) == TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN &&
            (type & TSS_API_BACKEND_MASK) == TSS_API_BACKEND_ID_DX12);
}

struct InternalFgScContext
{
    InternalContextHeader header;
    IDXGISwapChain4* fiSwapChain;
    FfxUInt32 version;
};

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X) 
#define MAKE_VERSION_STRING(major, minor, patch) STRINGIFY major "." STRINGIFY minor "." STRINGIFY patch

// If you change versioning in Id and VersionName, you must also update the DLL
// version defines in framegeneration/dx12/resource/resource.h
tssProvider_FrameGenerationSwapChain::tssProvider_FrameGenerationSwapChain()
: ffxProvider(0xF65C'DD12i64 << 32 | (FFX_SDK_MAKE_VERSION(FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH) & 0xFFFF'FFFF),
              TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN,
              MAKE_VERSION_STRING(FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_MINOR, FFX_FRAMEINTERPOLATION_SWAPCHAIN_VERSION_PATCH))
{
}

static tssReturnCode_t TssFrameGenerationSwapChainParseVersion(tssCreateContextDescHeader* header, FfxUInt32& versionPtr)
{
    // Assume the 3.1.5 API version unless otherwise specified - this is the last version prior to ABI stability.
    // We can't assume callers will supply this (due to tacit support for DLL swapping)
    uint32_t version = FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(3, 1, 5);
    bool bFoundVersion = false;

    for (auto it = header; it; it = it->pNext)
    {
        if (auto versionDesc = ffx::DynamicCast<tssCreateContextDescFrameGenerationSwapChainVersionDX12>(it))
        {
            version = versionDesc->version;
            bFoundVersion = true;
            break;
        }
    }

    if (version < FFX_FRAMEGENERATION_SWAPCHAIN_DX12_MAKE_VERSION(3, 1, 5) || version > FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION)
    {
        FFX_PRINT_MESSAGE(TSS_API_MESSAGE_TYPE_ERROR, L"tssCreateContextDescFrameGenerationSwapChainVersionDX12 version is invalid.");
        return TSS_API_RETURN_ERROR_PARAMETER;
    }

    if (!bFoundVersion)
    {
        FFX_PRINT_MESSAGE(TSS_API_MESSAGE_TYPE_ERROR, L"An instance of tssCreateContextDescFrameGenerationSwapChainVersionDX12 must be attached to tssCreateContextDescFrameGenerationSwapChain* and specify a valid version to access new API functions.");
    }

    versionPtr = version;

    return FFX_OK;
}

tssReturnCode_t tssProvider_FrameGenerationSwapChain::CreateContext(tssContext* context, tssCreateContextDescHeader* header, Allocator& alloc)
{
    if (auto desc = ffx::DynamicCast<tssCreateContextDescFrameGenerationSwapChainWrapDX12>(header))
    {
        FfxUInt32 version = 0;
        VERIFY(TssFrameGenerationSwapChainParseVersion(header, version) == FFX_OK, TSS_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = alloc.construct<InternalFgScContext>();
        VERIFY(internal_context, TSS_API_RETURN_ERROR_MEMORY);
        internal_context->header.provider = this;

        internal_context->version = version;

        TssSwapchain swapChain = tssGetSwapchainDX12(*desc->swapchain);
        TRY2(tssReplaceSwapchainForFrameinterpolationDX12(desc->gameQueue, swapChain));
        internal_context->fiSwapChain = *desc->swapchain = tssGetDX12SwapchainPtr(swapChain);

        // reference tracked by internal_context
        internal_context->fiSwapChain->AddRef();

        *context = internal_context;
        return TSS_API_RETURN_OK;
    }
    else if (auto desc = ffx::DynamicCast<tssCreateContextDescFrameGenerationSwapChainNewDX12>(header))
    {
        FfxUInt32 version = 0;
        VERIFY(TssFrameGenerationSwapChainParseVersion(header, version) == FFX_OK, TSS_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = alloc.construct<InternalFgScContext>();
        VERIFY(internal_context, TSS_API_RETURN_ERROR_MEMORY);
        internal_context->header.provider = this;

        internal_context->version = version;

        TssSwapchain swapChain;
        TRY2(tssCreateFrameinterpolationSwapchainDX12(desc->desc, desc->gameQueue, desc->dxgiFactory, swapChain));
        internal_context->fiSwapChain = *desc->swapchain = tssGetDX12SwapchainPtr(swapChain);

        // reference tracked by internal_context
        internal_context->fiSwapChain->AddRef();

        *context = internal_context;
        return TSS_API_RETURN_OK;
    }
    else if (auto desc = ffx::DynamicCast<tssCreateContextDescFrameGenerationSwapChainForHwndDX12>(header))
    {
        FfxUInt32 version = 0;
        VERIFY(TssFrameGenerationSwapChainParseVersion(header, version) == FFX_OK, TSS_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = alloc.construct<InternalFgScContext>();
        VERIFY(internal_context, TSS_API_RETURN_ERROR_MEMORY);
        internal_context->header.provider = this;

        internal_context->version = version;

        TssSwapchain swapChain;
        TRY2(tssCreateFrameinterpolationSwapchainForHwndDX12(desc->hwnd, desc->desc, desc->fullscreenDesc, desc->gameQueue, desc->dxgiFactory, swapChain));
        internal_context->fiSwapChain = *desc->swapchain = tssGetDX12SwapchainPtr(swapChain);

        // reference tracked by internal_context
        internal_context->fiSwapChain->AddRef();

        *context = internal_context;
        return TSS_API_RETURN_OK;
    }
    else
    {
        return TSS_API_RETURN_ERROR_UNKNOWN_DESCTYPE;
    }
}

tssReturnCode_t tssProvider_FrameGenerationSwapChain::DestroyContext(tssContext* context, Allocator& alloc)
{
    VERIFY(context, TSS_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context, TSS_API_RETURN_ERROR_PARAMETER);

    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);

    internal_context->fiSwapChain->Release();

    alloc.dealloc(internal_context);

    return TSS_API_RETURN_OK;
}

tssReturnCode_t tssProvider_FrameGenerationSwapChain::Configure(tssContext* context, const tssConfigureDescHeader* header) const
{
    VERIFY(header, TSS_API_RETURN_ERROR_PARAMETER);
    VERIFY(context, TSS_API_RETURN_ERROR_PARAMETER);
    VERIFY(*context, TSS_API_RETURN_ERROR_PARAMETER);

    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
    if (auto desc = ffx::DynamicCast<tssConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12>(header))
    {
        TRY2(tssRegisterFrameinterpolationUiResourceDX12(tssGetSwapchainDX12(internal_context->fiSwapChain), desc->uiResource, desc->flags));

        return TSS_API_RETURN_OK;
    }
    else if (auto desc = ffx::DynamicCast<tssConfigureDescFrameGenerationSwapChainKeyValueDX12>(header))
    {
        TRY2(tssConfigureFrameInterpolationSwapchainDX12(tssGetSwapchainDX12(internal_context->fiSwapChain), static_cast <TssFrameInterpolationSwapchainConfigureKey> (desc->key), desc->ptr));

        return TSS_API_RETURN_OK;
    }
    else
    {
        return TSS_API_RETURN_ERROR_PARAMETER;
    }
}

tssReturnCode_t tssProvider_FrameGenerationSwapChain::Query(tssContext* context, tssQueryDescHeader* header) const
{
    VERIFY(header, TSS_API_RETURN_ERROR_PARAMETER);

    
    if (auto desc = ffx::DynamicCast<tssQueryDescFrameGenerationSwapChainInterpolationCommandListDX12>(header))
    {
        VERIFY(context, TSS_API_RETURN_ERROR_PARAMETER);
        VERIFY(*context, TSS_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
        TssCommandList outCommandList{};
        TRY2(tssGetFrameinterpolationCommandlistDX12(tssGetSwapchainDX12(internal_context->fiSwapChain), outCommandList));
        *desc->pOutCommandList = outCommandList;

        return TSS_API_RETURN_OK;
    }
    else if (auto desc = ffx::DynamicCast<tssQueryDescFrameGenerationSwapChainInterpolationTextureDX12>(header))
    {
        VERIFY(context, TSS_API_RETURN_ERROR_PARAMETER);
        VERIFY(*context, TSS_API_RETURN_ERROR_PARAMETER);

        InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
        *desc->pOutTexture = tssGetFrameinterpolationTextureDX12(tssGetSwapchainDX12(internal_context->fiSwapChain));
        
        return TSS_API_RETURN_OK;
    }
    else if (auto desc = ffx::DynamicCast<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12>(header))
    {
        VERIFY(context, TSS_API_RETURN_ERROR_PARAMETER);
        VERIFY(*context, TSS_API_RETURN_ERROR_PARAMETER);
        VERIFY(desc->gpuMemoryUsageFrameGenerationSwapchain, TSS_API_RETURN_ERROR_PARAMETER);

        memset(desc->gpuMemoryUsageFrameGenerationSwapchain, 0, sizeof(TssApiEffectMemoryUsage));

        InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
        TRY2(tssFrameInterpolationSwapchainGetGpuMemoryUsageDX12(tssGetSwapchainDX12(internal_context->fiSwapChain), desc->gpuMemoryUsageFrameGenerationSwapchain));
        return TSS_API_RETURN_OK;
    }
    else if (auto desc = ffx::DynamicCast<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2>(header))
    {
        VERIFY(desc->gpuMemoryUsageFrameGenerationSwapchain, TSS_API_RETURN_ERROR_PARAMETER);

        memset(desc->gpuMemoryUsageFrameGenerationSwapchain, 0, sizeof(TssApiEffectMemoryUsage));

        TRY2(tssFrameInterpolationSwapchainGetGpuMemoryUsageDX12V2(
            static_cast<TssDevice> (desc->device),
            &(desc->displaySize),
            (TssApiSurfaceFormat)desc->backBufferFormat,
            desc->backBufferCount,
            &(desc->uiResourceSize),
            (TssApiSurfaceFormat)desc->uiResourceFormat,
            desc->flags,
            desc->gpuMemoryUsageFrameGenerationSwapchain));
        return TSS_API_RETURN_OK;
    }
    else
    {
        return TSS_API_RETURN_ERROR_UNKNOWN_DESCTYPE;
    }
}

tssReturnCode_t tssProvider_FrameGenerationSwapChain::Dispatch(tssContext* context, const tssDispatchDescHeader* header) const
{
    VERIFY(*context, TSS_API_RETURN_ERROR_PARAMETER);
    InternalFgScContext* internal_context = reinterpret_cast<InternalFgScContext*>(*context);
    if (auto desc = ffx::DynamicCast<tssDispatchDescFrameGenerationSwapChainWaitForPresentsDX12>(header))
    {
        tssWaitForPresents(internal_context->fiSwapChain);
        return TSS_API_RETURN_OK;
    }
    else
    {
        return TSS_API_RETURN_ERROR;
    }
}

tssProvider_FrameGenerationSwapChain& tssProvider_FrameGenerationSwapChain::GetInstance()
{
    static tssProvider_FrameGenerationSwapChain instance;
    return instance;
}
