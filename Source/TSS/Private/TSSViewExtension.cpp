#include "TSSViewExtension.h"
#include "TSS.h"
#include "TSSPipeline.h"
#include "TSSHistory.h"
#include "SceneView.h"
#include "SceneTexturesConfig.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIResources.h"
#include "RHICommandList.h"
#include "GlobalShader.h"
#include "PixelShaderUtils.h"
#include "Math/UnrealMathUtility.h"

struct FTSSPostProcessingInputsMirror
{
    TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures;
    FRDGTextureRef ViewFamilyTexture;
};

FTSSViewExtension::FTSSViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
    UE_LOG(LogTemp, Warning, TEXT("TSS: ViewExtension created"));
}

void FTSSViewExtension::PrePostProcessPass_RenderThread(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessingInputs& Inputs)
{
    if (!GTSSActive)
    {
        SceneColorRDG = nullptr;
        SceneDepthRDG = nullptr;
        MotionVectorsRDG = nullptr;
        return;
    }

    const FTSSPostProcessingInputsMirror& Mirror =
        *reinterpret_cast<const FTSSPostProcessingInputsMirror*>(&Inputs);
    const FSceneTextureUniformParameters* Params =
        Mirror.SceneTextures->GetContents();

    SceneColorRDG = Params->SceneColorTexture;
    SceneDepthRDG = Params->SceneDepthTexture;
    MotionVectorsRDG = Params->GBufferVelocityTexture;

    RenderRes = FIntPoint(
        SceneColorRDG->Desc.Extent.X,
        SceneColorRDG->Desc.Extent.Y);

    const FMatrix& Proj = View.ViewMatrices.GetProjectionMatrix();
    CachedDeviceToViewDepth.X = Proj.M[2][2];
    CachedDeviceToViewDepth.Y = Proj.M[3][2];
    CachedDeviceToViewDepth.Z = 1.0f / FMath::Max(Proj.M[0][0], 1e-6f);
    CachedDeviceToViewDepth.W = 1.0f / FMath::Max(Proj.M[1][1], 1e-6f);

    FVector2f TanHalfFov = View.ViewMatrices.GetTanHalfFov();
    CachedTanHalfFOV = TanHalfFov.Y;

    CachedPreExposure = View.GetLastEyeAdaptationExposure();
    CachedPrevPreExposure = CachedPreExposure;
    CachedDeltaTime = View.Family->Time.GetDeltaWorldTimeSeconds();

    FVector2D JitterNDC = View.ViewMatrices.GetTemporalAAJitter();
    CachedJitter = FVector2f(JitterNDC * FVector2D(RenderRes));

    CachedViewSpaceToMeters = (View.WorldToMetersScale > 0.0f) ? (1.0f / View.WorldToMetersScale) : 0.01f;

    static double LastLogTime = 0.0;
    double Now = FPlatformTime::Seconds();
    if (Now - LastLogTime >= 15.0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TSS: Frame=%d | SceneColor %dx%d fmt=%d | Depth %s | Velocity %s | Jitter=(%.4f,%.4f) | TanHalfFOV=%.4f"),
            FrameIndex,
            RenderRes.X, RenderRes.Y, (int32)SceneColorRDG->Desc.Format,
            SceneDepthRDG ? TEXT("OK") : TEXT("NULL"),
            MotionVectorsRDG ? TEXT("OK") : TEXT("NULL"),
            CachedJitter.X, CachedJitter.Y, CachedTanHalfFOV);
        LastLogTime = Now;
    }
}

void FTSSViewExtension::PostRenderViewFamily_RenderThread(
    FRDGBuilder& GraphBuilder,
    FSceneViewFamily& InViewFamily)
{
    if (!SceneColorRDG) return;
    if (!InViewFamily.RenderTarget) return;

    const FTextureRHIRef& BackBufferRHI = InViewFamily.RenderTarget->GetRenderTargetTexture();
    if (!BackBufferRHI.IsValid()) return;

    FRDGTextureRef BackBuffer = RegisterExternalTexture(
        GraphBuilder, BackBufferRHI.GetReference(), TEXT("TSS_BackBuffer"));
    FIntPoint OutputRes(BackBuffer->Desc.Extent.X, BackBuffer->Desc.Extent.Y);

    History.RegisterHistory(GraphBuilder);

    FTSSPipelineInputs PipeInputs;
    PipeInputs.SceneColor = SceneColorRDG;
    PipeInputs.SceneDepth = SceneDepthRDG;
    PipeInputs.MotionVectors = MotionVectorsRDG;
    PipeInputs.RenderResolution = RenderRes;
    PipeInputs.OutputResolution = OutputRes;
    PipeInputs.History = &History;
    PipeInputs.FrameIndex = FrameIndex;
    PipeInputs.Jitter = CachedJitter;
    PipeInputs.DeviceToViewDepth = CachedDeviceToViewDepth;
    PipeInputs.PreExposure = CachedPreExposure;
    PipeInputs.PreviousFramePreExposure = CachedPrevPreExposure;
    PipeInputs.TanHalfFOV = CachedTanHalfFOV;
    PipeInputs.DeltaTime = CachedDeltaTime;
    PipeInputs.ViewSpaceToMetersFactor = CachedViewSpaceToMeters;

    FRDGTextureRef FinalOutput = FTSSPipeline::Execute(GraphBuilder, PipeInputs, PipelineState);

    if (FinalOutput)
    {
        TShaderMapRef<FTSSBlitPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
        FTSSBlitPS::FParameters* BlitParams = GraphBuilder.AllocParameters<FTSSBlitPS::FParameters>();
        BlitParams->InputTexture = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(FinalOutput));
        BlitParams->InputOffset = FIntPoint::ZeroValue;
        BlitParams->RenderTargets[0] = FRenderTargetBinding(BackBuffer, ERenderTargetLoadAction::ENoAction);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GetGlobalShaderMap(GMaxRHIFeatureLevel),
            RDG_EVENT_NAME("TSS_BlitToBackBuffer"),
            PixelShader,
            BlitParams,
            FIntRect(FIntPoint(0, 0), OutputRes));
    }

    FrameIndex++;
    SceneColorRDG = nullptr;
    SceneDepthRDG = nullptr;
    MotionVectorsRDG = nullptr;
}
