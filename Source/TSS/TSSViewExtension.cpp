#include "TSSViewExtension.h"

#include "TSS.h"

#include "PostProcess/PostProcessMaterialInputs.h"
#include "ScreenPass.h"
#include "RenderGraphUtils.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY(LogTSS);

static TAutoConsoleVariable<int32> CVarTSSEnabled(
    TEXT("r.TSS.Enabled"),
    0,
    TEXT("Включает/выключает шарпинг TSS")
);

static TAutoConsoleVariable<float> CVarTSSStrength(
    TEXT("r.TSS.Strength"),
    1.0f,
    TEXT("Сила шарпинга")
);

FTSSViewExtension::FTSSViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
}

void FTSSViewExtension::SubscribeToPostProcessingPass(
    EPostProcessingPass PassId,
    FAfterPassCallbackDelegateArray& InOutPassCallbacks,
    bool bIsPassEnabled)
{
    if (PassId == EPostProcessingPass::Tonemap)
    {
        UE_LOG(LogTSS, Warning, TEXT("TSS: subscribing to Tonemap"));

        InOutPassCallbacks.Add(
            FAfterPassCallbackDelegate::CreateRaw(
                this,
                &FTSSViewExtension::PostProcessPassAfterTonemap_RenderThread
            )
        );
    }
}

FScreenPassTexture FTSSViewExtension::PostProcessPassAfterTonemap_RenderThread(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs)
{
    const FScreenPassTexture& SceneColor =
        Inputs.GetInput(EPostProcessMaterialInput::SceneColor);

    check(SceneColor.IsValid());

    const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());

    if (!CVarTSSEnabled.GetValueOnRenderThread())
    {
        if (Inputs.OverrideOutput.IsValid())
        {
            AddDrawTexturePass(
                GraphBuilder,
                ShaderMap,
                SceneColor.Texture,
                Inputs.OverrideOutput.Texture,
                FRDGDrawTextureInfo()
            );

            return FScreenPassTexture(
                Inputs.OverrideOutput.Texture,
                Inputs.OverrideOutput.ViewRect
            );
        }

        return SceneColor;
    }

    FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;

    OutputDesc.Flags |= TexCreate_ShaderResource | TexCreate_UAV;

    FRDGTextureRef OutputTexture =
        GraphBuilder.CreateTexture(
            OutputDesc,
            TEXT("TSSOutput")
        );

    AddSharpenPass(
        GraphBuilder,
        View.GetFeatureLevel(),
        SceneColor.Texture,
        OutputTexture,
        CVarTSSStrength.GetValueOnRenderThread()
    );

    if (Inputs.OverrideOutput.IsValid())
    {
        AddDrawTexturePass(
            GraphBuilder,
            ShaderMap,
            OutputTexture,
            Inputs.OverrideOutput.Texture,
            FRDGDrawTextureInfo()
        );

        return FScreenPassTexture(
            Inputs.OverrideOutput.Texture,
            Inputs.OverrideOutput.ViewRect
        );
    }

    return FScreenPassTexture(
        OutputTexture,
        SceneColor.ViewRect
    );
}
