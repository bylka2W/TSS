// This file is part of the FidelityFX SDK.
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

#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPUT_MOTION_VECTORS                0
#define FFX_FRAMEINTERPOLATION_BIND_SRV_INPUT_DEPTH                         1

#define FFX_FRAMEINTERPOLATION_BIND_UAV_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME  0
#define FFX_FRAMEINTERPOLATION_BIND_UAV_DILATED_MOTION_VECTORS              1
#define FFX_FRAMEINTERPOLATION_BIND_UAV_DILATED_DEPTH                       2

#define FFX_FRAMEINTERPOLATION_BIND_CB_FRAMEINTERPOLATION                   0

#include "frameinterpolation/ffx_frameinterpolation_callbacks_hlsl.h"
#include "frameinterpolation/ffx_frameinterpolation_common.h"
#include "frameinterpolation/ffx_frameinterpolation_reconstruct_dilated_velocity_and_previous_depth.h"

#ifndef TSS_FRAMEINTERPOLATION_THREAD_GROUP_WIDTH
#define TSS_FRAMEINTERPOLATION_THREAD_GROUP_WIDTH 8
#endif // #ifndef TSS_FRAMEINTERPOLATION_THREAD_GROUP_WIDTH
#ifndef TSS_FRAMEINTERPOLATION_THREAD_GROUP_HEIGHT
#define TSS_FRAMEINTERPOLATION_THREAD_GROUP_HEIGHT 8
#endif // #ifndef TSS_FRAMEINTERPOLATION_THREAD_GROUP_HEIGHT
#ifndef TSS_FRAMEINTERPOLATION_THREAD_GROUP_DEPTH
#define TSS_FRAMEINTERPOLATION_THREAD_GROUP_DEPTH 1
#endif // #ifndef TSS_FRAMEINTERPOLATION_THREAD_GROUP_DEPTH
#ifndef TSS_FRAMEINTERPOLATION_NUM_THREADS
#define TSS_FRAMEINTERPOLATION_NUM_THREADS [numthreads(TSS_FRAMEINTERPOLATION_THREAD_GROUP_WIDTH, TSS_FRAMEINTERPOLATION_THREAD_GROUP_HEIGHT, TSS_FRAMEINTERPOLATION_THREAD_GROUP_DEPTH)]
#endif // #ifndef TSS_FRAMEINTERPOLATION_NUM_THREADS

TSS_FRAMEINTERPOLATION_NUM_THREADS
TSS_FRAMEINTERPOLATION_EMBED_ROOTSIG_CONTENT
void CS(
    int2 iGroupId : SV_GroupID,
    int2 iDispatchThreadId : SV_DispatchThreadID,
    int2 iGroupThreadId : SV_GroupThreadID,
    int iGroupIndex : SV_GroupIndex
)
{
    ReconstructAndDilate(iDispatchThreadId);
}
