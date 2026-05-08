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

#if defined(__cplusplus)
extern "C" {
#endif  // #if defined(__cplusplus)

#define FFX_API_ENTRY __declspec(dllexport)

#include <stdint.h>

enum TssApiReturnCodes
{
    TSS_API_RETURN_OK                     = 0, ///< The oparation was successful.
    TSS_API_RETURN_ERROR                  = 1, ///< An error occurred that is not further specified.
    TSS_API_RETURN_ERROR_UNKNOWN_DESCTYPE = 2, ///< The structure type given was not recognized for the function or context with which it was used. This is likely a programming error.
    TSS_API_RETURN_ERROR_RUNTIME_ERROR    = 3, ///< The underlying runtime (e.g. D3D12, Vulkan) or effect returned an error code.
    TSS_API_RETURN_NO_PROVIDER            = 4, ///< No provider was found for the given structure type. This is likely a programming error.
    TSS_API_RETURN_ERROR_MEMORY           = 5, ///< A memory allocation failed.
    TSS_API_RETURN_ERROR_PARAMETER        = 6, ///< A parameter was invalid, e.g. a null pointer, empty resource or out-of-bounds enum value.
    TSS_API_RETURN_PROVIDER_NO_SUPPORT_NEW_DESCTYPE = 7, ///< The structure type given is new and not supported in the old provider. This is likely fixed with driver upgrade or effect DLL upgrade.
};

typedef void* tssContext;
typedef uint32_t tssReturnCode_t;

#define TSS_API_EFFECT_MASK         0x00ff0000u
#define TSS_API_BACKEND_MASK        0xff000000u
#define TSS_API_EFFECT_ID_GENERAL   0x00000000u

// Base Descriptor types
typedef uint64_t tssStructType_t;
typedef struct tssApiHeader
{
    tssStructType_t      type;  ///< The structure type. Must always be set to the corresponding value for any structure (found nearby with a similar name).
    struct tssApiHeader* pNext; ///< Pointer to next structure, used for optional parameters and extensions. Can be null.
} tssApiHeader;

typedef tssApiHeader tssCreateContextDescHeader;
typedef tssApiHeader tssConfigureDescHeader;
typedef tssApiHeader tssQueryDescHeader;
typedef tssApiHeader tssDispatchDescHeader;

// Extensions for global debug
#define TSS_API_CONFIGURE_GLOBALDEBUG_LEVEL_SILENCE  0x0000000u
#define TSS_API_CONFIGURE_GLOBALDEBUG_LEVEL_ERRORS   0x0000001u
#define TSS_API_CONFIGURE_GLOBALDEBUG_LEVEL_WARNINGS 0x0000002u
#define TSS_API_CONFIGURE_GLOBALDEBUG_LEVEL_VERBOSE  0xfffffffu

enum TssApiMsgType
{
    TSS_API_MESSAGE_TYPE_ERROR   = 0,
    TSS_API_MESSAGE_TYPE_WARNING = 1,
    TSS_API_MESSAGE_TYPE_COUNT
};

typedef void (*tssApiMessage)(uint32_t type, const wchar_t* message);

#define TSS_API_CONFIGURE_DESC_TYPE_GLOBALDEBUG1 0x0000001u
struct tssConfigureDescGlobalDebug1
{
    tssConfigureDescHeader header;
    tssApiMessage          fpMessage;
    uint32_t               debugLevel;
};

#define TSS_API_QUERY_DESC_TYPE_GET_VERSIONS 4u
struct tssQueryDescGetVersions
{
    tssQueryDescHeader header;
    uint64_t createDescType;    ///< Create description for the effect whose versions should be enumerated.
    void* device;               ///< For DX12: pointer to ID3D12Device.
    uint64_t *outputCount;      ///< Input capacity of id and name arrays. Output number of returned versions. If initially zero, output is number of available versions.
    uint64_t *versionIds;       ///< Output array of version ids to be used as version overrides. If null, only names and count are returned.
    const char** versionNames;  ///< Output array of version names for display. If null, only ids and count are returned. If both this and versionIds are null, only count is returned.
};

#define TSS_API_DESC_TYPE_OVERRIDE_VERSION 5u
struct tssOverrideVersion
{
    tssApiHeader header;
    uint64_t versionId;  ///< Id of version to use. Must be a value returned from a query in tssQueryDescGetVersions.versionIds array.
};

#define TSS_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION 6u
struct tssQueryGetProviderVersion
{
    tssQueryDescHeader header;
    uint64_t versionId;      ///< Id of provider being used for queried context. 0 if invalid.
    const char* versionName; ///< Version name for display. If nullptr, the query was invalid.
};

// Memory allocation function. Must return a valid pointer to at least size bytes of memory aligned to hold any type.
// May return null to indicate failure. Standard library malloc fulfills this requirement.
typedef void* (*tssAlloc)(void* pUserData, uint64_t size);

// Memory deallocation function. May be called with null pointer as second argument.
typedef void (*tssDealloc)(void* pUserData, void* pMem);

typedef struct tssAllocationCallbacks
{
    void* pUserData;
    tssAlloc alloc;
    tssDealloc dealloc;
} tssAllocationCallbacks;

// Creates a FFX object context.
// Depending on the desc structures provided to this function, the context will be created with the desired version and attributes.
// Non-zero return indicates error code.
// Pointers passed in desc must remain live until tssDestroyContext is called on the context.
// MemCb may be null; the system allocator (malloc/free) will be used in this case.
FFX_API_ENTRY tssReturnCode_t tssCreateContext(tssContext* context, tssCreateContextDescHeader* desc, const tssAllocationCallbacks* memCb);
typedef tssReturnCode_t (*PfnTssCreateContext)(tssContext* context, tssCreateContextDescHeader* desc, const tssAllocationCallbacks* memCb);

// Destroys an FFX object context.
// Non-zero return indicates error code.
// MemCb must be compatible with the callbacks passed into tssCreateContext.
FFX_API_ENTRY tssReturnCode_t tssDestroyContext(tssContext* context, const tssAllocationCallbacks* memCb);
typedef tssReturnCode_t (*PfnTssDestroyContext)(tssContext* context, const tssAllocationCallbacks* memCb);

// Configures the provided FFX object context.
// If context is null, configure operates on any global state.
// Non-zero return indicates error code.
FFX_API_ENTRY tssReturnCode_t tssConfigure(tssContext* context, const tssConfigureDescHeader* desc);
typedef tssReturnCode_t (*PfnTssConfigure)(tssContext* context, const tssConfigureDescHeader* desc);

// Queries the provided FFX object context.
// If context is null, query operates on any global state.
// Non-zero return indicates error code.
FFX_API_ENTRY tssReturnCode_t tssQuery(tssContext* context, tssQueryDescHeader* desc);
typedef tssReturnCode_t (*PfnTssQuery)(tssContext* context, tssQueryDescHeader* desc);

// Dispatches work on the given FFX object context defined by the dispatch descriptor.
// Non-zero return indicates error code.
FFX_API_ENTRY tssReturnCode_t tssDispatch(tssContext* context, const tssDispatchDescHeader* desc);
typedef tssReturnCode_t (*PfnTssDispatch)(tssContext* context, const tssDispatchDescHeader* desc);

// FFX_API_EFFECT_IDs
#define TSS_API_EFFECT_ID_UPSCALE                       	0x00010000u
#define TSS_API_EFFECT_ID_FRAMEGENERATION               	0x00020000u
#define TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN      	0x00030000u
// Need to keep this ID around for the deprecated VK frame gen swapchain
#define TSS_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_VK       0x00040000u
// Need to keep this ID around for the deprecated VK frame gen swapchain
#define FFX_API_EFFECT_ID_DENOISER                      	0x00050000u
#define FFX_API_EFFECT_ID_RADIANCECACHE                   	0x00060000u

#define TSS_API_MAKE_EFFECT_SUB_ID(effectId, subversion) ((effectId & TSS_API_EFFECT_MASK) | (subversion & ~TSS_API_EFFECT_MASK))

// FFX_APID_BACKEND_IDs
#define TSS_API_BACKEND_ID_DX12               0x00000000u
#define TSS_API_BACKEND_ID_XBOX               0x01000000u
#define TSS_API_BACKEND_ID_VK                 0x02000000u // For new effects going forward, please use this backend ID for vulkan specifics

#define TSS_API_MAKE_BACKEND_SUB_ID(backendId, subversion) ((backendId & TSS_API_BACKEND_MASK) | (subversion & ~TSS_API_BACKEND_MASK))

// Combiner for BACKEND-specific EFFECT sub-Ids
#define TSS_API_MAKE_BACKEND_EFFECT_SUB_ID(backendId, effectId, subversion) ((subversion & ~TSS_API_EFFECT_MASK) | (effectId & TSS_API_EFFECT_MASK) | (backendId & TSS_API_BACKEND_MASK) | (subversion & ~(TSS_API_BACKEND_MASK | TSS_API_EFFECT_MASK)))

// Pragma macros for controlling warnings so that deprecations take affect externally but can be suppressed internally.
// This is so we can maintain the API/ABI until we are ready to make the breaking change.
// These are sadly compiler specific.
#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define TSS_PRAGMA_WARNING_PUSH warning( push )
#define TSS_PRAGMA_WARNING_POP warning( pop )
#define TSS_PRAGMA_WARNING_DISABLE_DEPRECATIONS warning( disable: 4996 )
#define TSS_PRAGMA_WARNING_WARN_DEPRECATIONS warning( default: 4996 )
#elif defined(__clang__) || defined(__GNUC__)
#define TSS_PRAGMA_WARNING_PUSH GCC diagnostic push
#define TSS_PRAGMA_WARNING_POP GCC diagnostic pop
#define TSS_PRAGMA_WARNING_DISABLE_DEPRECATIONS GCC diagnostic ignored "-Wdeprecated"
#define TSS_PRAGMA_WARNING_WARN_DEPRECATIONS GCC diagnostic warning "-Wdeprecated"
#else
#define TSS_PRAGMA_WARNING_PUSH 
#define TSS_PRAGMA_WARNING_POP 
#define TSS_PRAGMA_WARNING_DISABLE_DEPRECATIONS 
#endif

#define TSS_DEPRECATION(message) [[deprecated(message)]]

#if defined(__cplusplus)
}
#endif  // #if defined(__cplusplus)
