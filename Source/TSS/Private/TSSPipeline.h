#pragma once

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "RenderGraphFwd.h"
#include "TSSHistory.h"

struct FTSSPipelineState
{
    bool bValid = false;
};

struct FTSSPipelineInputs
{
    FRDGTextureRef SceneColor = nullptr;
    FRDGTextureRef SceneDepth = nullptr;
    FRDGTextureRef MotionVectors = nullptr;
    FRDGTextureRef ReactiveMask = nullptr;
    FRDGTextureRef TransparencyCompositionMask = nullptr;
    FIntPoint RenderResolution = FIntPoint::ZeroValue;
    FIntPoint OutputResolution = FIntPoint::ZeroValue;
    FTSSHistory* History = nullptr;
    uint32 FrameIndex = 0;
    FVector2f Jitter = FVector2f::ZeroVector;
    FVector4f DeviceToViewDepth = FVector4f(0);
    float PreExposure = 1.0f;
    float PreviousFramePreExposure = 1.0f;
    float TanHalfFOV = 1.0f;
    float DeltaTime = 0.016f;
    float ViewSpaceToMetersFactor = 100.0f;
};

class FTSSPipeline
{
public:
    static FRDGTextureRef Execute(
        FRDGBuilder& GraphBuilder,
        const FTSSPipelineInputs& Inputs,
        FTSSPipelineState& State);
};
