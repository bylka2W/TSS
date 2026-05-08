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

#include "../include/tss_api.h"
#include "../internal/tss_api_helper.h"
#include "../internal/tss_internal_types.h"
#include "../internal/tss_error.h"
#include "../internal/tss_provider.h"
#include "../internal/tss_backends.h"

static uint64_t GetVersionOverride(const tssApiHeader* header)
{
    for (auto it = header; it; it = it->pNext)
    {
        if (auto versionDesc = ffx::DynamicCast<tssOverrideVersion>(it))
        {
            return versionDesc->versionId;
        }
    }
    return 0;
}

FFX_API_ENTRY tssReturnCode_t tssCreateContext(tssContext* context, tssCreateContextDescHeader* desc, const tssAllocationCallbacks* memCb)
{
    VERIFY(desc != nullptr, TSS_API_RETURN_ERROR_PARAMETER);
    VERIFY(context != nullptr, TSS_API_RETURN_ERROR_PARAMETER);

    *context = nullptr;

    Allocator alloc{memCb};
    std::optional<ffxProviderExternal> extProviderSlot;
    ffxProvider* provider = GetProvider(desc->type, GetVersionOverride(desc), GetDevice(desc), extProviderSlot);
    VERIFY(provider != nullptr, TSS_API_RETURN_NO_PROVIDER);

#if FFX_BACKEND_DX12
    if (extProviderSlot && &*extProviderSlot == provider)
    {
        // external provider was selected, need to move to heap allocation.
        provider = alloc.construct<ffxProviderExternal>(std::move(*extProviderSlot));
    }
#endif

    auto retCode = provider->CreateContext(context, desc, alloc);
    if (retCode != TSS_API_RETURN_OK && provider->GetRefCount() == 0)
    {
        provider->~ffxProvider();
        alloc.dealloc(provider);
    }
    return retCode;
}

FFX_API_ENTRY tssReturnCode_t tssDestroyContext(tssContext* context, const tssAllocationCallbacks* memCb)
{
    VERIFY(context != nullptr, TSS_API_RETURN_ERROR_PARAMETER);

    Allocator alloc{memCb};
    ffxProvider* provider = GetAssociatedProvider(*context);
    auto retCode = provider->DestroyContext(context, alloc);

    if (provider->GetRefCount() == 0)
    {
        provider->~ffxProvider();
        alloc.dealloc(provider);
    }
    return retCode;
}

FFX_API_ENTRY tssReturnCode_t tssConfigure(tssContext* context, const tssConfigureDescHeader* desc)
{
    VERIFY(desc != nullptr, TSS_API_RETURN_ERROR_PARAMETER);
    VERIFY(context != nullptr, TSS_API_RETURN_ERROR_PARAMETER);

    return GetAssociatedProvider(*context)->Configure(context, desc);
}

FFX_API_ENTRY tssReturnCode_t tssQuery(tssContext* context, tssQueryDescHeader* header)
{
    VERIFY(header != nullptr, TSS_API_RETURN_ERROR_PARAMETER);

    tssReturnCode_t retCode;
    std::optional<ffxProviderExternal> extProviderSlot;
    if (context == nullptr)
    {
        if (auto desc = ffx::DynamicCast<tssQueryDescGetVersions>(header))
        {
            // if output count is zero or no other pointer passed, count providers only
            if (desc->outputCount && (*desc->outputCount == 0 || (!desc->versionIds && !desc->versionNames)))
            {
                *desc->outputCount = GetProviderCount(desc->createDescType, desc->device, extProviderSlot);
            }
            else if (desc->outputCount && *desc->outputCount > 0)
            {
                uint64_t capacity = *desc->outputCount;
                *desc->outputCount = GetProviderVersions(desc->createDescType, desc->device, capacity, desc->versionIds, desc->versionNames, extProviderSlot);
            }
            return TSS_API_RETURN_OK;
        }
        else if (auto provider = GetProvider(header->type, GetVersionOverride(header), GetDevice(header), extProviderSlot))
        {
            retCode = provider->Query(nullptr, header);
        }
        else
        {
            retCode = TSS_API_RETURN_NO_PROVIDER;
        }
    }
    else
    {
        auto provider = GetAssociatedProvider(*context);
        if (provider)
        {
            retCode = provider->Query(context, header);
        }
        else
        {
            retCode = TSS_API_RETURN_NO_PROVIDER;
        }
    }

    return tssQueryFallback(context, header, retCode);
}

FFX_API_ENTRY tssReturnCode_t tssDispatch(tssContext* context, const tssDispatchDescHeader* desc)
{
    VERIFY(desc != nullptr, TSS_API_RETURN_ERROR_PARAMETER);
    VERIFY(context != nullptr, TSS_API_RETURN_ERROR_PARAMETER);

    return GetAssociatedProvider(*context)->Dispatch(context, desc);
}
