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

#include "../../api/include/dx12/tss_api_dx12.h"
#include "../../api/internal/tss_backends.h"

#if defined(FFX_FRAMEGENERATION)
#include "../../framegeneration/include/tss_framegeneration.h"
#include "../../framegeneration/include/dx12/tss_api_framegeneration_dx12.h"
#endif // defined(FFX_FRAMEGENERATION)
#if defined(TSS_UPSCALER)
#include "../../upscalers/include/TSS_UPSCALE.h"
#endif // defined(TSS_UPSCALER)
#if defined(FFX_DENOISER)
#include "../../denoisers/include/ffx_denoiser.h"
#endif // defined(FFX_DENOISER)
#if defined(FFX_RADIANCECACHE)
#include "../../radiancecache/include/ffx_radiancecache.h"
#endif // defined(FFX_RADIANCECACHE)

#include "tss_dx12.h"

tssReturnCode_t CreateBackend(const tssCreateContextDescHeader *desc, bool& backendFound, TssInterface *iface, size_t contexts, Allocator& alloc)
{
    for (const auto* it = desc->pNext; it; it = it->pNext)
    {
        switch (it->type)
        {
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12:
        {
            // check for double backend just to make sure.
            if (backendFound)
            {
                return TSS_API_RETURN_ERROR;
            }
            backendFound = true;

            const auto *backendDesc = reinterpret_cast<const ffxCreateBackendDX12Desc*>(it);
            TssDevice device = tssGetDeviceDX12(backendDesc->device);
            size_t scratchBufferSize = tssGetScratchMemorySizeDX12(contexts);
            void* scratchBuffer = alloc.alloc(scratchBufferSize);
            memset(scratchBuffer, 0, scratchBufferSize);
            TRY2(tssGetInterfaceDX12(iface, device, scratchBuffer, scratchBufferSize, contexts));

            break;
        }
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12_ALLOCATION_CALLBACKS:
        {
            const auto* allocationCallbacksDesc = reinterpret_cast<const ffxCreateBackendDX12AllocationCallbacksDesc*>(it);
            if (allocationCallbacksDesc->pfnTssResourceAllocator && allocationCallbacksDesc->pfnTssResourceDeallocator)
            {
                tssRegisterResourceAllocatorDX12(allocationCallbacksDesc->pfnTssResourceAllocator);
                tssRegisterResourceDeallocatorDX12(allocationCallbacksDesc->pfnTssResourceDeallocator);
            }
            if (allocationCallbacksDesc->pfnFfxHeapAllocator && allocationCallbacksDesc->pfnFfxHeapDeallocator)
            {
                tssRegisterHeapAllocatorDX12(allocationCallbacksDesc->pfnFfxHeapAllocator);
                tssRegisterHeapDeallocatorDX12(allocationCallbacksDesc->pfnFfxHeapDeallocator);
            }
            if (allocationCallbacksDesc->pfnFfxConstantBufferAllocator)
            {
                tssRegisterConstantBufferAllocatorDX12(allocationCallbacksDesc->pfnFfxConstantBufferAllocator);
            }
            break;
        }
        }
    }
    return TSS_API_RETURN_OK;
}

void* GetDevice(const tssApiHeader* desc)
{
    for (const auto* it = desc; it; it = it->pNext)
    {
        switch (it->type)
        {
        case TSS_API_QUERY_DESC_TYPE_GET_VERSIONS:
        {
            return reinterpret_cast<const tssQueryDescGetVersions*>(it)->device;
        }
#if defined(TSS_UPSCALER)
        case FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2:
        {
            return reinterpret_cast<const tssQueryDescUpscaleGetGPUMemoryUsageV2*>(it)->device;
        }
#endif //#if defined(TSS_UPSCALER)
#if defined(FFX_DENOISER)
        case FFX_API_QUERY_DESC_TYPE_DENOISER_GPU_MEMORY_USAGE:
        {
            return reinterpret_cast<const tssQueryDescDenoiserGetGPUMemoryUsage*>(it)->device;
        }
        case FFX_API_QUERY_DESC_TYPE_DENOISER_GET_VERSION:
        {
            return reinterpret_cast<const tssQueryDescDenoiserGetVersion*>(it)->device;
        }
#endif //#if defined(FFX_DENOISER)
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12:
        {
            return reinterpret_cast<const ffxCreateBackendDX12Desc*>(it)->device;
        }
#if defined(FFX_FRAMEGENERATION)
        case TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12:
        {
            ID3D12Device* device = nullptr;
            reinterpret_cast<const tssCreateContextDescFrameGenerationSwapChainForHwndDX12*>(it)->gameQueue->GetDevice(IID_PPV_ARGS(&device));
            device->Release();
            return device;
        }
        case TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12:
        {
            ID3D12Device* device = nullptr;
            reinterpret_cast<const tssCreateContextDescFrameGenerationSwapChainNewDX12*>(it)->gameQueue->GetDevice(IID_PPV_ARGS(&device));
            device->Release();
            return device;
        }
        case TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX12:
        {
            ID3D12Device* device = nullptr;
            reinterpret_cast<const tssCreateContextDescFrameGenerationSwapChainWrapDX12*>(it)->gameQueue->GetDevice(IID_PPV_ARGS(&device));
            device->Release();
            return device;
        }
        case TSS_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE_V2:
        {
            return reinterpret_cast<const tssQueryDescFrameGenerationGetGPUMemoryUsageV2*>(it)->device;
        }
        case FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12_V2:
        {
            return reinterpret_cast<const tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2*>(it)->device;
        }
#endif // defined(FFX_FRAMEGENERATION)
        }
    }
    return nullptr;
}

TssErrorCode GetResourceSizeFromDescription(TssDevice device, const FfxCreateResourceDescription* createResourceDescription, uint64_t* sizeInBytes, uint64_t* alignment)
{
    return tssGetResourceSizeFromDescriptionDX12(device, createResourceDescription, sizeInBytes, alignment);
}