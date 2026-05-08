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

#include "../../../api/include/dx12/tss_api_dx12.hpp"
#include "tss_api_framegeneration_dx12.h"

// Helper types for header initialization. Api definition is in .h file.

namespace ffx
{

template<>
struct struct_type<tssCreateContextDescFrameGenerationSwapChainWrapDX12> : std::integral_constant<uint64_t, TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX12>
{};

struct CreateContextDescFrameGenerationSwapChainWrapDX12 : public InitHelper<tssCreateContextDescFrameGenerationSwapChainWrapDX12>
{};

template<>
struct struct_type<tssCreateContextDescFrameGenerationSwapChainNewDX12> : std::integral_constant<uint64_t, TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12>
{};

struct CreateContextDescFrameGenerationSwapChainNewDX12 : public InitHelper<tssCreateContextDescFrameGenerationSwapChainNewDX12>
{};

template<>
struct struct_type<tssCreateContextDescFrameGenerationSwapChainForHwndDX12> : std::integral_constant<uint64_t, TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12>
{};

struct CreateContextDescFrameGenerationSwapChainForHwndDX12 : public InitHelper<tssCreateContextDescFrameGenerationSwapChainForHwndDX12>
{};

template<>
struct struct_type<tssConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12> : std::integral_constant<uint64_t, TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX12>
{};

struct ConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 : public InitHelper<tssConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12>
{};

template<>
struct struct_type<tssQueryDescFrameGenerationSwapChainInterpolationCommandListDX12> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX12>
{};

struct QueryDescFrameGenerationSwapChainInterpolationCommandListDX12 : public InitHelper<tssQueryDescFrameGenerationSwapChainInterpolationCommandListDX12>
{};

template<>
struct struct_type<tssQueryDescFrameGenerationSwapChainInterpolationTextureDX12> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX12>
{};

struct QueryDescFrameGenerationSwapChainInterpolationTextureDX12 : public InitHelper<tssQueryDescFrameGenerationSwapChainInterpolationTextureDX12>
{};

template<>
struct struct_type<tssDispatchDescFrameGenerationSwapChainWaitForPresentsDX12> : std::integral_constant<uint64_t, TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX12>
{};

struct DispatchDescFrameGenerationSwapChainWaitForPresentsDX12 : public InitHelper<tssDispatchDescFrameGenerationSwapChainWaitForPresentsDX12>
{};

template<>
struct struct_type<tssConfigureDescFrameGenerationSwapChainKeyValueDX12> : std::integral_constant<uint64_t, TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_DX12>
{};

struct ConfigureDescFrameGenerationSwapChainKeyValueDX12 : public InitHelper<tssConfigureDescFrameGenerationSwapChainKeyValueDX12>
{};

template<>
struct struct_type<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12>
{};

struct QueryFrameGenerationSwapChainGetGPUMemoryUsageDX12 : public InitHelper<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12>
{};

template<>
struct struct_type<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX12_V2>
{};

struct QueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2 : public InitHelper<tssQueryFrameGenerationSwapChainGetGPUMemoryUsageDX12V2>
{};

template<>
struct struct_type<tssCreateContextDescFrameGenerationSwapChainVersionDX12> : std::integral_constant<uint64_t, TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12>
{};

struct CreateContextDescFrameGenerationSwapChainVersionDX12 : public InitHelper<tssCreateContextDescFrameGenerationSwapChainVersionDX12>
{};

}  // namespace ffx
