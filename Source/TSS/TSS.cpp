#include "TSS.h"

#include "TSSViewExtension.h"

#include "CoreGlobals.h"
#include "GlobalShader.h"
#include "Modules/ModuleManager.h"
#include "Projects.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "ShaderParameterStruct.h"

IMPLEMENT_GLOBAL_SHADER(FTSSSharpenShader, "/TSS/TSSSharpen.usf", "MainCS", SF_Compute);

#define LOCTEXT_NAMESPACE "FTSSModule"

TSharedPtr<FTSSViewExtension, ESPMode::ThreadSafe> TSSViewExtension;

void FTSSModule::StartupModule()
{
    FString PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("TSS"))->GetBaseDir();
    FString ShaderDir = FPaths::Combine(PluginBaseDir, TEXT("Shaders"));
    AddShaderSourceDirectoryMapping(TEXT("/TSS"), ShaderDir);

    FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTSSModule::OnPostEngineInit);
}

void FTSSModule::ShutdownModule()
{
    FCoreDelegates::OnPostEngineInit.RemoveAll(this);
    TSSViewExtension.Reset();
}

void FTSSModule::OnPostEngineInit()
{
    UE_LOG(LogTSS, Warning, TEXT("TSS: OnPostEngineInit"));

    TSSViewExtension = FSceneViewExtensions::NewExtension<FTSSViewExtension>();

    UE_LOG(LogTSS, Warning,
        TEXT("TSS: ViewExtension created=%d"),
        TSSViewExtension.IsValid() ? 1 : 0);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTSSModule, TSS)

void AddSharpenPass(
    FRDGBuilder& GraphBuilder,
    ERHIFeatureLevel::Type FeatureLevel,
    FRDGTextureRef InputTexture,
    FRDGTextureRef OutputTexture,
    float Strength)
{
    TShaderMapRef<FTSSSharpenShader> ComputeShader(
        GetGlobalShaderMap(FeatureLevel)
    );

    FTSSSharpenShader::FParameters* PassParameters =
        GraphBuilder.AllocParameters<FTSSSharpenShader::FParameters>();

    PassParameters->InputTexture = GraphBuilder.CreateSRV(
        FRDGTextureSRVDesc(InputTexture)
    );

    PassParameters->OutputTexture = GraphBuilder.CreateUAV(
        FRDGTextureUAVDesc(OutputTexture)
    );

    PassParameters->InputSampler = TStaticSamplerState<
        SF_Point,
        AM_Clamp,
        AM_Clamp,
        AM_Clamp
    >::GetRHI();

    PassParameters->Strength = Strength;

    const FIntPoint TextureSize = InputTexture->Desc.Extent;
    PassParameters->InvTextureSize = FVector2f(
        1.0f / static_cast<float>(TextureSize.X),
        1.0f / static_cast<float>(TextureSize.Y)
    );

    const int32 GroupsX = FMath::DivideAndRoundUp(TextureSize.X, 8);
    const int32 GroupsY = FMath::DivideAndRoundUp(TextureSize.Y, 8);
    const FIntVector DispatchGroupCount(GroupsX, GroupsY, 1);

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("TSS Sharpen"),
        PassParameters,
        ERDGPassFlags::Compute,
        [ComputeShader, PassParameters, DispatchGroupCount](FRHICommandListImmediate& RHICmdList)
        {
            FComputeShaderUtils::Dispatch(
                RHICmdList,
                ComputeShader,
                *PassParameters,
                DispatchGroupCount
            );
        }
    );
}