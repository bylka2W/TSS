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

#include "../../api/internal/tss_api_helper.h"
#include "tss_provider.h"

tssReturnCode_t tssQueryFallback(tssContext* context, tssQueryDescHeader* header, tssReturnCode_t retCode)
{
    if (context == nullptr)
    {
        if (retCode != TSS_API_RETURN_OK)
        {
            //pass-through retCode
            //Fill in valid default output. Otherwise if case app doesn't check retCode, and have not previously zero out the struct, app would read random values.
            if (header->type == TSS_API_QUERY_DESC_TYPE_GET_VERSIONS)
            {
                auto desc = reinterpret_cast<tssQueryDescGetVersions*>(header);
                desc->outputCount = 0;
            }
#if defined(TSS_UPSCALER)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetRenderResolutionFromQualityMode*>(header);
                if (desc->pOutRenderWidth != nullptr)
                {
                    *desc->pOutRenderWidth = 0u;
                }
                if (desc->pOutRenderHeight != nullptr)
                {
                    *desc->pOutRenderHeight = 0u;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetUpscaleRatioFromQualityMode*>(header);
                if (desc->pOutUpscaleRatio != nullptr)
                {
                    *desc->pOutUpscaleRatio = 0.0f;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetJitterPhaseCount*>(header);
                if (desc->pOutPhaseCount != nullptr)
                {
                    *desc->pOutPhaseCount = 0;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetJitterOffset*>(header);
                if (desc->pOutX != nullptr)
                {
                    *desc->pOutX = 0.0f;
                }
                if (desc->pOutY != nullptr)
                {
                    *desc->pOutY = 0.0f;
                }
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetGPUMemoryUsage*>(header);
                if (desc->gpuMemoryUsageUpscaler != nullptr)
                {
                    desc->gpuMemoryUsageUpscaler->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageUpscaler->aliasableUsageInBytes = 0u;
                }
            }
#endif //#if defined(TSS_UPSCALER)
#if defined(FFX_FRAMEGENERATION)
            else if (header->type == TSS_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE)
            {
                auto desc = reinterpret_cast<tssQueryDescFrameGenerationGetGPUMemoryUsage*>(header);
                if (desc->gpuMemoryUsageFrameGeneration != nullptr)
                {
                    desc->gpuMemoryUsageFrameGeneration->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGeneration->aliasableUsageInBytes = 0u;
                }
            }
#if defined(FFX_BACKEND_DX12)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12)
            {
                auto desc = reinterpret_cast<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12*>(header);
                if (desc->gpuMemoryUsageFrameGenerationSwapchain != nullptr)
                {
                    desc->gpuMemoryUsageFrameGenerationSwapchain->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGenerationSwapchain->aliasableUsageInBytes = 0u;
                    retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
#elif defined(FFX_BACKEND_XBOX)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_XBOX)
            {
                auto desc = reinterpret_cast<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageXbox*>(header);
                if (desc->gpuMemoryUsageFrameGenerationSwapchain != nullptr)
                {
                    desc->gpuMemoryUsageFrameGenerationSwapchain->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGenerationSwapchain->aliasableUsageInBytes = 0u;
                    retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;
                }
            }
#endif // defined(FFX_BACKEND_DX12)
#endif  // defined(FFX_FRAMEGENERATION)
#if defined(TSS_UPSCALER)
            // Fixup retCode for new DESCTYPE that are not supported by oldest FSR driver provider (Adrenaline 25.3.1)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GET_RESOURCE_REQUIREMENTS)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetResourceRequirements*>(header);
                desc->required_resources = ~uint64_t(0);
                desc->optional_resources = ~uint64_t(0);
                if (header->type == TSS_API_RETURN_ERROR_UNKNOWN_DESCTYPE)
                    retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;
            }
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GPU_MEMORY_USAGE_V2)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetGPUMemoryUsageV2*>(header);
                if (desc->gpuMemoryUsageUpscaler != nullptr)
                {
                    desc->gpuMemoryUsageUpscaler->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageUpscaler->aliasableUsageInBytes = 0u;
                    retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
#endif // defined(TSS_UPSCALER)
#if defined(FFX_FRAMEGENERATION)
            else if (header->type == TSS_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE_V2)
            {
                auto desc = reinterpret_cast<tssQueryDescFrameGenerationGetGPUMemoryUsageV2*>(header);
                if (desc->gpuMemoryUsageFrameGeneration != nullptr)
                {
                    desc->gpuMemoryUsageFrameGeneration->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGeneration->aliasableUsageInBytes = 0u;
                    retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
#if defined(FFX_BACKEND_DX12)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12_V2)
            {
                auto desc = reinterpret_cast<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2*>(header);
                if (desc->gpuMemoryUsageFrameGenerationSwapchain != nullptr)
                {
                    desc->gpuMemoryUsageFrameGenerationSwapchain->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsageFrameGenerationSwapchain->aliasableUsageInBytes = 0u;
                    retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;                }
            }
#elif defined(FFX_BACKEND_XBOX)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_XBOX_V2)
            {
            auto desc = reinterpret_cast<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageXboxV2*>(header);
            if (desc->gpuMemoryUsageFrameGenerationSwapchain != nullptr)
            {
                desc->gpuMemoryUsageFrameGenerationSwapchain->totalUsageInBytes = 0u;
                desc->gpuMemoryUsageFrameGenerationSwapchain->aliasableUsageInBytes = 0u;
                retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;
            }
            }
#endif // defined(FFX_BACKEND_DX12)
#endif  // defined(FFX_FRAMEGENERATION)
#if defined(FFX_DENOISER)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_DENOISER_GPU_MEMORY_USAGE)
            {
                auto desc = reinterpret_cast<tssQueryDescDenoiserGetGPUMemoryUsage*>(header);
                if (desc->gpuMemoryUsage != nullptr)
                {
                    desc->gpuMemoryUsage->totalUsageInBytes = 0u;
                    desc->gpuMemoryUsage->aliasableUsageInBytes = 0u;
                }
            }
#endif // defined(FFX_DENOISER)
        }
    }
    else
    {
        if (retCode == TSS_API_RETURN_ERROR_UNKNOWN_DESCTYPE)
        {
#if defined(TSS_UPSCALER) || defined(FFX_FRAMEGENERATION) || defined(FFX_DENOISER) || defined(FFX_RADIANCECACHE)
            auto provider = GetAssociatedProvider(*context);
            if (header->type == TSS_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION)
            {
                auto desc = reinterpret_cast<tssQueryGetProviderVersion*>(header);
                desc->versionId = provider->GetId();
                desc->versionName = provider->GetVersionName();
                retCode = TSS_API_RETURN_OK;
            }
#endif // defined(TSS_UPSCALER) || defined(FFX_FRAMEGENERATION) || defined(FFX_DENOISER)
            // on a older driver or effect DLL that doesn't support this new query DESTYPE, fill in default output.
#if defined(TSS_UPSCALER)
            else if (header->type == FFX_API_QUERY_DESC_TYPE_UPSCALE_GET_RESOURCE_REQUIREMENTS)
            {
                auto desc = reinterpret_cast<tssQueryDescUpscaleGetResourceRequirements*>(header);
                desc->required_resources = ~uint64_t(0);
                desc->optional_resources = ~uint64_t(0);
                retCode = TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE;
            }
#endif //defined(TSS_UPSCALER)
        }
    }
    return retCode;
}
