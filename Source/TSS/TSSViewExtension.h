#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTSS, Log, All);

class FTSSViewExtension : public FSceneViewExtensionBase
{
public:
    FTSSViewExtension(const FAutoRegister& AutoRegister);

    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

    virtual void SubscribeToPostProcessingPass(
        EPostProcessingPass PassId,
        FAfterPassCallbackDelegateArray& InOutPassCallbacks,
        bool bIsPassEnabled) override;

    FScreenPassTexture PostProcessPassAfterTonemap_RenderThread(
        FRDGBuilder& GraphBuilder,
        const FSceneView& View,
        const FPostProcessMaterialInputs& Inputs);
};