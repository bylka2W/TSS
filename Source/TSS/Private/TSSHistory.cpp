#include "TSSHistory.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIResources.h"

void FTSSHistory::RegisterHistory(FRDGBuilder& GraphBuilder)
{
    HistoryInternalColorRDG = nullptr;
    HistoryLockStatusRDG = nullptr;
    HistoryDilatedMVRDG = nullptr;

    if (PrevInternalColor.IsValid())
    {
        HistoryInternalColorRDG = GraphBuilder.RegisterExternalTexture(
            PrevInternalColor, TEXT("TSS_PrevInternalColor"));
    }

    if (PrevLockStatus.IsValid())
    {
        HistoryLockStatusRDG = GraphBuilder.RegisterExternalTexture(
            PrevLockStatus, TEXT("TSS_PrevLockStatus"));
    }

    if (PrevDilatedMV.IsValid())
    {
        HistoryDilatedMVRDG = GraphBuilder.RegisterExternalTexture(
            PrevDilatedMV, TEXT("TSS_PrevDilatedMV"));
    }
}

void FTSSHistory::UpdateFromPipeline(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef NewInternalColor,
    FRDGTextureRef NewLockStatus)
{
    if (NewInternalColor)
    {
        GraphBuilder.QueueTextureExtraction(NewInternalColor, &PrevInternalColor);
    }

    if (NewLockStatus)
    {
        GraphBuilder.QueueTextureExtraction(NewLockStatus, &PrevLockStatus);
    }

    bValid = true;
    FrameIndex++;
}

void FTSSHistory::Reset()
{
    bValid = false;
    FrameIndex = 0;
    PrevInternalColor = nullptr;
    PrevLockStatus = nullptr;
    PrevDilatedMV = nullptr;
    HistoryInternalColorRDG = nullptr;
    HistoryLockStatusRDG = nullptr;
    HistoryDilatedMVRDG = nullptr;
    Resolution = FIntPoint::ZeroValue;
    DisplayResolution = FIntPoint::ZeroValue;
}
