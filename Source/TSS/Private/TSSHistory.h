#pragma once

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "RenderGraphFwd.h"

struct FTSSHistory
{
    TRefCountPtr<IPooledRenderTarget> PrevInternalColor;
    TRefCountPtr<IPooledRenderTarget> PrevLockStatus;
    TRefCountPtr<IPooledRenderTarget> PrevDilatedMV;

    FRDGTextureRef HistoryInternalColorRDG = nullptr;
    FRDGTextureRef HistoryLockStatusRDG = nullptr;
    FRDGTextureRef HistoryDilatedMVRDG = nullptr;

    FIntPoint Resolution = FIntPoint::ZeroValue;
    FIntPoint DisplayResolution = FIntPoint::ZeroValue;
    uint32 FrameIndex = 0;
    bool bValid = false;

    void RegisterHistory(FRDGBuilder& GraphBuilder);

    void UpdateFromPipeline(
        FRDGBuilder& GraphBuilder,
        FRDGTextureRef NewInternalColor,
        FRDGTextureRef NewLockStatus);

    void Reset();
};
