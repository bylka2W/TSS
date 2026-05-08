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

#include "tss_api.h"

typedef struct ffxFunctions {
    PfnTssCreateContext CreateContext;
    PfnTssDestroyContext DestroyContext;
    PfnTssConfigure Configure;
    PfnTssQuery Query;
    PfnTssDispatch Dispatch;
} ffxFunctions;

// _GAMING_XBOX defined by GDK tools build
// _WINDOWS defined by MSBuild x64 windows configurations
// PLATFORM_WINDOWS defined for Unreal Engine build processes
#if defined(_WINDOWS) || defined(PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // defined(_WINDOWS) || defined(PLATFORM_WINDOWS)
#if defined(_GAMING_XBOX) || defined(_WINDOWS) || defined(PLATFORM_WINDOWS)
#include <libloaderapi.h>
#else
#pragma error "Unsupported ffx API platform"
#endif // #if defined(_GAMING_XBOX) || defined(_WINDOWS) || defined(PLATFORM_WINDOWS)

static inline void ffxLoadFunctions(ffxFunctions* pOutFunctions, void* module)
{
    // _GAMING_XBOX defined by GDK tools build
    // _WINDOWS defined by MSBuild x64 windows configurations
    // PLATFORM_WINDOWS defined for Unreal Engine build processes
#if defined(_GAMING_XBOX) || defined(_WINDOWS) || defined(PLATFORM_WINDOWS)
    pOutFunctions->CreateContext  = (PfnTssCreateContext)GetProcAddress((HMODULE)module, "tssCreateContext");
    pOutFunctions->DestroyContext = (PfnTssDestroyContext)GetProcAddress((HMODULE)module, "tssDestroyContext");
    pOutFunctions->Configure      = (PfnTssConfigure)GetProcAddress((HMODULE)module, "tssConfigure");
    pOutFunctions->Query          = (PfnTssQuery)GetProcAddress((HMODULE)module, "tssQuery");
    pOutFunctions->Dispatch       = (PfnTssDispatch)GetProcAddress((HMODULE)module, "tssDispatch");
#endif // #if defined(_GAMING_XBOX) || defined(_WINDOWS) || defined(PLATFORM_WINDOWS)
}
