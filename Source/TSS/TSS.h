#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"

class FTSSSharpenShader : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSSharpenShader);
    SHADER_USE_PARAMETER_STRUCT(FTSSSharpenShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InputTexture)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        SHADER_PARAMETER(float, Strength)
        SHADER_PARAMETER(FVector2f, InvTextureSize)
    END_SHADER_PARAMETER_STRUCT()
};

class TSS_API FTSSModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

TSS_API void AddSharpenPass(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FRDGTextureRef OutputTexture,
    float Strength
);
