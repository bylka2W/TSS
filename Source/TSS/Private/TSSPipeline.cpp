#include "TSSPipeline.h"
#include "TSSShaders.h"
#include "TSSHistory.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"

extern int32 GTSSDebugMode;

static const int32 TSS_SHADING_CHANGE_MIP_LEVEL_CPP = 4;

static FRDGTextureRef MakeTexture(
    FRDGBuilder& GraphBuilder,
    const TCHAR* Name,
    FIntPoint Size,
    EPixelFormat Format)
{
    FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(
        Size, Format, FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV);
    return GraphBuilder.CreateTexture(Desc, Name);
}

static FRDGTextureRef MakeTextureUAVOnly(
    FRDGBuilder& GraphBuilder,
    const TCHAR* Name,
    FIntPoint Size,
    EPixelFormat Format)
{
    FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(
        Size, Format, FClearValueBinding::Black,
        TexCreate_ShaderResource | TexCreate_UAV);
    return GraphBuilder.CreateTexture(Desc, Name);
}

template<typename FParameters>
static void FillFSR2Constants(
    const FTSSPipelineInputs& Inputs,
    FParameters& P)
{
    P.RenderSize = Inputs.RenderResolution;
    P.MaxRenderSize = Inputs.RenderResolution;
    P.DisplaySize = Inputs.OutputResolution;
    P.InputColorResourceDimensions = Inputs.RenderResolution;
    P.LumaMipDimensions = FIntPoint(1, 1);
    P.LumaMipLevelToUse = 4;
    P.FrameIndex = (int)Inputs.FrameIndex;
    P.DeviceToViewDepth = Inputs.DeviceToViewDepth;
    P.Jitter = Inputs.Jitter;
    P.MotionVectorScale = FVector2f(1.0f, 1.0f);
    P.DownscaleFactor = FVector2f(
        (float)Inputs.RenderResolution.X / (float)Inputs.OutputResolution.X,
        (float)Inputs.RenderResolution.Y / (float)Inputs.OutputResolution.Y);
    P.MotionVectorJitterCancellation = FVector2f::Zero();
    P.PreExposure = Inputs.PreExposure;
    P.PreviousFramePreExposure = Inputs.PreviousFramePreExposure;
    P.TanHalfFOV = Inputs.TanHalfFOV;
    P.JitterSequenceLength = 8.0f;
    P.DeltaTime = Inputs.DeltaTime;
    P.DynamicResChangeFactor = 1.0f;
    P.ViewSpaceToMetersFactor = Inputs.ViewSpaceToMetersFactor;
}

static FRDGTextureRef ExecuteLuminancePyramid(
    FRDGBuilder& GraphBuilder,
    const FTSSPipelineInputs& Inputs)
{
    FIntPoint RenderRes = Inputs.RenderResolution;

    FRDGTextureRef LuminanceMip0 = MakeTexture(GraphBuilder, TEXT("TSS_LumaMip0"), RenderRes, PF_R16F);
    FRDGTextureRef LuminanceMip4 = MakeTexture(GraphBuilder, TEXT("TSS_LumaMip4"),
        FIntPoint(FMath::Max(1, RenderRes.X >> TSS_SHADING_CHANGE_MIP_LEVEL_CPP),
                  FMath::Max(1, RenderRes.Y >> TSS_SHADING_CHANGE_MIP_LEVEL_CPP)), PF_R16F);
    FRDGTextureRef LuminanceMip5 = MakeTexture(GraphBuilder, TEXT("TSS_LumaMip5"),
        FIntPoint(FMath::Max(1, RenderRes.X >> 5), FMath::Max(1, RenderRes.Y >> 5)), PF_R16F);
    FRDGTextureRef AutoExposure = MakeTexture(GraphBuilder, TEXT("TSS_AutoExposure"), FIntPoint(1, 1), PF_G32R32F);
    FRDGTextureRef SpdCounter = MakeTexture(GraphBuilder, TEXT("TSS_SpdCounter"), FIntPoint(1, 1), PF_R32_UINT);

    TShaderMapRef<FTSSLuminancePyramidCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FTSSLuminancePyramidCS::FParameters* Params = GraphBuilder.AllocParameters<FTSSLuminancePyramidCS::FParameters>();

    Params->InputColor = Inputs.SceneColor;
    Params->LuminanceMip0 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(LuminanceMip0));
    Params->LuminanceMip4 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(LuminanceMip4));
    Params->LuminanceMip5 = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(LuminanceMip5));
    Params->AutoExposure = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(AutoExposure));
    Params->SpdAtomicCounter = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(SpdCounter));
    FillFSR2Constants(Inputs, *Params);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("TSS_LuminancePyramid"),
        Shader, Params,
        FIntVector(
            FMath::DivideAndRoundUp(RenderRes.X, 8),
            FMath::DivideAndRoundUp(RenderRes.Y, 8),
            1));

    return AutoExposure;
}

static void ExecuteReconstructDilate(
    FRDGBuilder& GraphBuilder,
    const FTSSPipelineInputs& Inputs,
    FRDGTextureRef ReconstructedPrevDepth,
    FRDGTextureRef DilatedMotionVectors,
    FRDGTextureRef DilatedDepth,
    FRDGTextureRef LockInputLuma)
{
    TShaderMapRef<FTSSReconstructDilateCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FTSSReconstructDilateCS::FParameters* Params = GraphBuilder.AllocParameters<FTSSReconstructDilateCS::FParameters>();

    Params->InputDepthTex = Inputs.SceneDepth;
    Params->InputColorTex = Inputs.SceneColor;
    Params->InputMotionVectorsTex = Inputs.MotionVectors;
    Params->ReconstructedPrevDepth = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(ReconstructedPrevDepth));
    Params->DilatedMotionVectors = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DilatedMotionVectors));
    Params->DilatedDepth = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DilatedDepth));
    Params->LockInputLuma = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(LockInputLuma));
    FillFSR2Constants(Inputs, *Params);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("TSS_ReconstructDilate"),
        Shader, Params,
        FIntVector(
            FMath::DivideAndRoundUp(Inputs.RenderResolution.X, 8),
            FMath::DivideAndRoundUp(Inputs.RenderResolution.Y, 8),
            1));
}

static void ExecuteDepthClip(
    FRDGBuilder& GraphBuilder,
    const FTSSPipelineInputs& Inputs,
    FRDGTextureRef ReconstructedPrevDepth,
    FRDGTextureRef DilatedMotionVectors,
    FRDGTextureRef DilatedDepth,
    FRDGTextureRef PrevDilatedMV,
    FRDGTextureRef DilatedReactiveMasks,
    FRDGTextureRef PreparedInputColor)
{
    TShaderMapRef<FTSSDepthClipCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FTSSDepthClipCS::FParameters* Params = GraphBuilder.AllocParameters<FTSSDepthClipCS::FParameters>();

    Params->ReconstructedPrevDepthTex = ReconstructedPrevDepth;
    Params->DilatedMotionVectorsTex = DilatedMotionVectors;
    Params->DilatedDepthTex = DilatedDepth;
    Params->ReactiveMaskTex = Inputs.ReactiveMask ? Inputs.ReactiveMask : Inputs.SceneColor;
    Params->TransparencyCompositionMaskTex = Inputs.TransparencyCompositionMask ? Inputs.TransparencyCompositionMask : Inputs.SceneColor;
    Params->PrevDilatedMotionVectorsTex = PrevDilatedMV ? PrevDilatedMV : DilatedMotionVectors;
    Params->InputMotionVectorsTex = Inputs.MotionVectors;
    Params->InputColorTex = Inputs.SceneColor;
    Params->InputDepthTex = Inputs.SceneDepth;
    Params->DilatedReactiveMasks = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DilatedReactiveMasks));
    Params->PreparedInputColor = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(PreparedInputColor));
    Params->LinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    FillFSR2Constants(Inputs, *Params);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("TSS_DepthClip"),
        Shader, Params,
        FIntVector(
            FMath::DivideAndRoundUp(Inputs.RenderResolution.X, 8),
            FMath::DivideAndRoundUp(Inputs.RenderResolution.Y, 8),
            1));
}

static void ExecuteLock(
    FRDGBuilder& GraphBuilder,
    const FTSSPipelineInputs& Inputs,
    FRDGTextureRef LockInputLuma,
    FRDGTextureRef ReconstructedPrevDepth,
    FRDGTextureRef NewLocks)
{
    TShaderMapRef<FTSSLockCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FTSSLockCS::FParameters* Params = GraphBuilder.AllocParameters<FTSSLockCS::FParameters>();

    Params->LockInputLumaTex = LockInputLuma;
    Params->NewLocks = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(NewLocks));
    Params->ReconstructedPrevDepth = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(ReconstructedPrevDepth));
    FillFSR2Constants(Inputs, *Params);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("TSS_Lock"),
        Shader, Params,
        FIntVector(
            FMath::DivideAndRoundUp(Inputs.RenderResolution.X, 8),
            FMath::DivideAndRoundUp(Inputs.RenderResolution.Y, 8),
            1));
}

static FRDGTextureRef ExecuteAccumulate(
    FRDGBuilder& GraphBuilder,
    const FTSSPipelineInputs& Inputs,
    FRDGTextureRef PreparedInputColor,
    FRDGTextureRef DilatedReactiveMasks,
    FRDGTextureRef AutoExposure,
    FRDGTextureRef NewLocks,
    FRDGTextureRef HistoryInternalColor,
    FRDGTextureRef HistoryLockStatus)
{
    FIntPoint DisplayRes = Inputs.OutputResolution;

    FRDGTextureRef InternalUpscaledOut = MakeTexture(GraphBuilder, TEXT("TSS_InternalUpscaled"), DisplayRes, PF_FloatRGBA);
    FRDGTextureRef LockStatusOut = MakeTexture(GraphBuilder, TEXT("TSS_LockStatus"), DisplayRes, PF_G16R16F);
    FRDGTextureRef LumaHistoryOut = MakeTexture(GraphBuilder, TEXT("TSS_LumaHistory"), DisplayRes, PF_B8G8R8A8);
    FRDGTextureRef UpscaledOutput = MakeTexture(GraphBuilder, TEXT("TSS_Upscaled"), DisplayRes, PF_FloatRGBA);

    TShaderMapRef<FTSSAccumulateCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FTSSAccumulateCS::FParameters* Params = GraphBuilder.AllocParameters<FTSSAccumulateCS::FParameters>();

    Params->PreparedInputColorTex = PreparedInputColor;
    Params->DilatedReactiveMasksTex = DilatedReactiveMasks;
    Params->InputMotionVectorsTex = Inputs.MotionVectors;
    Params->LockStatusTex = HistoryLockStatus ? HistoryLockStatus : DilatedReactiveMasks;
    Params->InternalUpscaledColorTex = HistoryInternalColor ? HistoryInternalColor : PreparedInputColor;
    Params->SceneLuminanceMipsTex = PreparedInputColor;
    Params->AutoExposureTex = AutoExposure;
    Params->LumaHistoryTex = LumaHistoryOut;
    Params->NewLocksTex = NewLocks;
    Params->InternalUpscaledOut = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(InternalUpscaledOut));
    Params->LockStatusOut = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(LockStatusOut));
    Params->LumaHistoryOut = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(LumaHistoryOut));
    Params->UpscaledOutput = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(UpscaledOutput));
    Params->LinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    FillFSR2Constants(Inputs, *Params);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("TSS_Accumulate"),
        Shader, Params,
        FIntVector(
            FMath::DivideAndRoundUp(DisplayRes.X, 8),
            FMath::DivideAndRoundUp(DisplayRes.Y, 8),
            1));

    return UpscaledOutput;
}

static FRDGTextureRef ExecuteRCAS(
    FRDGBuilder& GraphBuilder,
    const FTSSPipelineInputs& Inputs,
    FRDGTextureRef Input)
{
    FIntPoint DisplayRes = Inputs.OutputResolution;
    FRDGTextureRef RCASOutput = MakeTexture(GraphBuilder, TEXT("TSS_RCASOutput"), DisplayRes, PF_FloatRGBA);

    TShaderMapRef<FTSSRCASCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FTSSRCASCS::FParameters* Params = GraphBuilder.AllocParameters<FTSSRCASCS::FParameters>();

    Params->RCASInputTex = Input;
    Params->OutputColor = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(RCASOutput));
    FillFSR2Constants(Inputs, *Params);
    Params->Sharpness = 0.25f;

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("TSS_RCAS"),
        Shader, Params,
        FIntVector(
            FMath::DivideAndRoundUp(DisplayRes.X, 8),
            FMath::DivideAndRoundUp(DisplayRes.Y, 8),
            1));

    return RCASOutput;
}

FRDGTextureRef FTSSPipeline::Execute(
    FRDGBuilder& GraphBuilder,
    const FTSSPipelineInputs& Inputs,
    FTSSPipelineState& State)
{
    if (!Inputs.SceneColor)
        return nullptr;

    const FIntPoint RenderRes = Inputs.RenderResolution;
    const FIntPoint DisplayRes = Inputs.OutputResolution;

    if (GTSSDebugMode == 1)
    {
        State.bValid = true;
        return Inputs.SceneColor;
    }

    FTSSHistory* History = Inputs.History;
    bool bHasHistory = History && History->bValid;

    FRDGTextureRef HistoryInternalColor = bHasHistory ? History->HistoryInternalColorRDG : nullptr;
    FRDGTextureRef HistoryLockStatus = bHasHistory ? History->HistoryLockStatusRDG : nullptr;

    FRDGTextureRef ReconstructedPrevDepth = MakeTextureUAVOnly(GraphBuilder, TEXT("TSS_ReconstructedPrevDepth"), RenderRes, PF_R32_UINT);
    FRDGTextureRef DilatedMotionVectors = MakeTexture(GraphBuilder, TEXT("TSS_DilatedMV"), RenderRes, PF_G16R16F);
    FRDGTextureRef DilatedDepth = MakeTexture(GraphBuilder, TEXT("TSS_DilatedDepth"), RenderRes, PF_R32_FLOAT);
    FRDGTextureRef LockInputLuma = MakeTexture(GraphBuilder, TEXT("TSS_LockInputLuma"), RenderRes, PF_R32_FLOAT);
    FRDGTextureRef DilatedReactiveMasks = MakeTexture(GraphBuilder, TEXT("TSS_DilatedRM"), RenderRes, PF_R8G8);
    FRDGTextureRef PreparedInputColor = MakeTexture(GraphBuilder, TEXT("TSS_PreparedColor"), RenderRes, PF_FloatRGBA);
    FRDGTextureRef NewLocks = MakeTexture(GraphBuilder, TEXT("TSS_NewLocks"), DisplayRes, PF_R8);

    FRDGTextureRef AutoExposure = ExecuteLuminancePyramid(GraphBuilder, Inputs);

    ExecuteReconstructDilate(GraphBuilder, Inputs,
        ReconstructedPrevDepth, DilatedMotionVectors, DilatedDepth, LockInputLuma);

    ExecuteDepthClip(GraphBuilder, Inputs,
        ReconstructedPrevDepth, DilatedMotionVectors, DilatedDepth,
        History->HistoryDilatedMVRDG ? History->HistoryDilatedMVRDG : DilatedMotionVectors,
        DilatedReactiveMasks, PreparedInputColor);

    ExecuteLock(GraphBuilder, Inputs,
        LockInputLuma, ReconstructedPrevDepth, NewLocks);

    if (GTSSDebugMode == 2 && bHasHistory && HistoryInternalColor)
    {
        State.bValid = true;
        return HistoryInternalColor;
    }

    FRDGTextureRef AccumulateOutput = ExecuteAccumulate(GraphBuilder, Inputs,
        PreparedInputColor, DilatedReactiveMasks, AutoExposure, NewLocks,
        HistoryInternalColor, HistoryLockStatus);

    if (GTSSDebugMode == 3)
    {
        State.bValid = true;
        return AccumulateOutput;
    }

    FRDGTextureRef Final = ExecuteRCAS(GraphBuilder, Inputs, AccumulateOutput);

    if (History)
    {
        History->UpdateFromPipeline(GraphBuilder, AccumulateOutput, nullptr);
    }

    State.bValid = true;
    return Final;
}
