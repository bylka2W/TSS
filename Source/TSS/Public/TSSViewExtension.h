#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "RendererInterface.h"
#include "RenderGraphFwd.h"
#include "TSSPipeline.h"
#include "TSSHistory.h"

class FTSSViewExtension : public FSceneViewExtensionBase
{
public:
    FTSSViewExtension(const FAutoRegister& AutoRegister);

    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

    virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
    virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;

    virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;

    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override
    {
        return true;
    }

private:
    FRDGTextureRef SceneColorRDG = nullptr;
    FRDGTextureRef SceneDepthRDG = nullptr;
    FRDGTextureRef MotionVectorsRDG = nullptr;
    FIntPoint RenderRes = FIntPoint::ZeroValue;

    FVector2f CachedJitter = FVector2f::ZeroVector;
    FVector4f CachedDeviceToViewDepth = FVector4f(0);
    float CachedTanHalfFOV = 1.0f;
    float CachedPreExposure = 1.0f;
    float CachedPrevPreExposure = 1.0f;
    float CachedDeltaTime = 0.016f;
    float CachedViewSpaceToMeters = 0.01f;

    FTSSPipelineState PipelineState;
    FTSSHistory History;
    uint32 FrameIndex = 0;
};
