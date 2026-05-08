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

#include "../../api/include/tss_api.hpp"
#include "tss_framegeneration.h"

// Helper types for header initialization. Api definition is in .h file.

namespace ffx
{

template<>
struct struct_type<tssCreateContextDescFrameGeneration> : std::integral_constant<uint64_t, TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION>
{};

struct CreateContextDescFrameGeneration : public InitHelper<tssCreateContextDescFrameGeneration>
{};

template<>
struct struct_type<tssConfigureDescFrameGeneration> : std::integral_constant<uint64_t, TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION>
{};

struct ConfigureDescFrameGeneration : public InitHelper<tssConfigureDescFrameGeneration>
{};

template<>
struct struct_type<tssDispatchDescFrameGeneration> : std::integral_constant<uint64_t, TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION>
{};

struct DispatchDescFrameGeneration : public InitHelper<tssDispatchDescFrameGeneration>
{};

#pragma TSS_PRAGMA_WARNING_PUSH
#pragma TSS_PRAGMA_WARNING_DISABLE_DEPRECATIONS

template<>
struct TSS_DEPRECATION("tssDispatchDescFrameGenerationPrepare is deprecated, use tssDispatchDescFrameGenerationPrepareV2") struct_type<tssDispatchDescFrameGenerationPrepare> : std::integral_constant<uint64_t, TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE>
{};

struct TSS_DEPRECATION("DispatchDescFrameGenerationPrepare is deprecated, use DispatchDescFrameGenerationPrepareV2") DispatchDescFrameGenerationPrepare : public InitHelper<tssDispatchDescFrameGenerationPrepare>
{};

#pragma TSS_PRAGMA_WARNING_POP

template<>
struct struct_type<tssConfigureDescFrameGenerationKeyValue> : std::integral_constant<uint64_t, TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_KEYVALUE>
{};

struct ConfigureDescFrameGenerationKeyValue : public InitHelper<tssConfigureDescFrameGenerationKeyValue>
{};

template<>
struct struct_type<tssQueryDescFrameGenerationGetGPUMemoryUsage> : std::integral_constant<uint64_t, TSS_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE>
{};

struct QueryDescFrameGenerationGetGPUMemoryUsage : public InitHelper<tssQueryDescFrameGenerationGetGPUMemoryUsage>
{};

template<>
struct struct_type<tssConfigureDescFrameGenerationRegisterDistortionFieldResource> : std::integral_constant<uint64_t, TSS_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION_REGISTERDISTORTIONRESOURCE>
{};

struct ConfigureDescFrameGenerationRegisterDistortionFieldResource : public InitHelper<tssConfigureDescFrameGenerationRegisterDistortionFieldResource>
{};

template<>
struct struct_type<tssCreateContextDescFrameGenerationHudless> : std::integral_constant<uint64_t, TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_HUDLESS>
{};

struct CreateContextDescFrameGenerationHudless : public InitHelper<tssCreateContextDescFrameGenerationHudless>
{};

#pragma TSS_PRAGMA_WARNING_PUSH
#pragma TSS_PRAGMA_WARNING_DISABLE_DEPRECATIONS

template<>
struct TSS_DEPRECATION("tssDispatchDescFrameGenerationPrepareCameraInfo is deprecated, use tssDispatchDescFrameGenerationPrepareV2") struct_type<tssDispatchDescFrameGenerationPrepareCameraInfo> : std::integral_constant<uint64_t, TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_CAMERAINFO>
{};

struct TSS_DEPRECATION("DispatchDescFrameGenerationPrepareCameraInfo is deprecated, use DispatchDescFrameGenerationPrepareV2") DispatchDescFrameGenerationPrepareCameraInfo : public InitHelper<tssDispatchDescFrameGenerationPrepareCameraInfo>
{};

#pragma TSS_PRAGMA_WARNING_POP

template<>
struct struct_type<tssQueryDescFrameGenerationGetGPUMemoryUsageV2> : std::integral_constant<uint64_t, TSS_API_QUERY_DESC_TYPE_FRAMEGENERATION_GPU_MEMORY_USAGE_V2>
{};

struct QueryDescFrameGenerationGetGPUMemoryUsageV2 : public InitHelper<tssQueryDescFrameGenerationGetGPUMemoryUsageV2>
{};

template<>
struct struct_type<tssDispatchDescFrameGenerationPrepareV2> : std::integral_constant<uint64_t, TSS_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2>
{};

struct DispatchDescFrameGenerationPrepareV2 : public InitHelper<tssDispatchDescFrameGenerationPrepareV2>
{};

template<>
struct struct_type<tssCallbackDescFrameGenerationPresentPremulAlpha> : std::integral_constant<uint64_t, TSS_API_CALLBACK_DESC_TYPE_FRAMEGENERATION_PRESENT_PREMUL_ALPHA>
{};

struct CallbackDescFrameGenerationPresentPremulAlpha : public InitHelper<tssCallbackDescFrameGenerationPresentPremulAlpha>
{};

template<>
struct struct_type<tssCreateContextDescFrameGenerationVersion> : std::integral_constant<uint64_t, TSS_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION>
{};

struct CreateContextDescFrameGenerationVersion : public InitHelper<tssCreateContextDescFrameGenerationVersion>
{};

}  // namespace ffx
