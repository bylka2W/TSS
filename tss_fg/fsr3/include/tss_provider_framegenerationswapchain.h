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
#include "../../../api/internal/tss_provider.h"

class tssProvider_FrameGenerationSwapChain: public ffxProvider
{
public:
    tssProvider_FrameGenerationSwapChain();
    virtual ~tssProvider_FrameGenerationSwapChain() = default;

    virtual bool CanProvide(uint64_t type) const override;

    virtual tssReturnCode_t CreateContext(tssContext* context, tssCreateContextDescHeader* desc, Allocator& alloc) override;

    virtual tssReturnCode_t DestroyContext(tssContext* context, Allocator& alloc) override;

    virtual tssReturnCode_t Configure(tssContext* context, const tssConfigureDescHeader* desc) const override;

    virtual tssReturnCode_t Query(tssContext* context, tssQueryDescHeader* desc) const override;

    virtual tssReturnCode_t Dispatch(tssContext* context, const tssDispatchDescHeader* desc) const override;

    static tssProvider_FrameGenerationSwapChain& GetInstance();
};