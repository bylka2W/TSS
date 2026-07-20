#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"

class FTSSLuminancePyramidCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSLuminancePyramidCS);
    SHADER_USE_PARAMETER_STRUCT(FTSSLuminancePyramidCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputColor)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, LuminanceMip0)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, LuminanceMip4)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, LuminanceMip5)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, AutoExposure)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, SpdAtomicCounter)
        SHADER_PARAMETER_SAMPLER(SamplerState, PointClampSampler)
        SHADER_PARAMETER(FIntPoint, RenderSize)
        SHADER_PARAMETER(FIntPoint, MaxRenderSize)
        SHADER_PARAMETER(FIntPoint, DisplaySize)
        SHADER_PARAMETER(FIntPoint, InputColorResourceDimensions)
        SHADER_PARAMETER(FIntPoint, LumaMipDimensions)
        SHADER_PARAMETER(int, LumaMipLevelToUse)
        SHADER_PARAMETER(int, FrameIndex)
        SHADER_PARAMETER(FVector4f, DeviceToViewDepth)
        SHADER_PARAMETER(FVector2f, Jitter)
        SHADER_PARAMETER(FVector2f, MotionVectorScale)
        SHADER_PARAMETER(FVector2f, DownscaleFactor)
        SHADER_PARAMETER(FVector2f, MotionVectorJitterCancellation)
        SHADER_PARAMETER(float, PreExposure)
        SHADER_PARAMETER(float, PreviousFramePreExposure)
        SHADER_PARAMETER(float, TanHalfFOV)
        SHADER_PARAMETER(float, JitterSequenceLength)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, DynamicResChangeFactor)
        SHADER_PARAMETER(float, ViewSpaceToMetersFactor)
    END_SHADER_PARAMETER_STRUCT()
};

class FTSSReconstructDilateCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSReconstructDilateCS);
    SHADER_USE_PARAMETER_STRUCT(FTSSReconstructDilateCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputDepthTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputColorTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputMotionVectorsTex)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, ReconstructedPrevDepth)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, DilatedMotionVectors)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, DilatedDepth)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, LockInputLuma)
        SHADER_PARAMETER(FIntPoint, RenderSize)
        SHADER_PARAMETER(FIntPoint, MaxRenderSize)
        SHADER_PARAMETER(FIntPoint, DisplaySize)
        SHADER_PARAMETER(FIntPoint, InputColorResourceDimensions)
        SHADER_PARAMETER(FIntPoint, LumaMipDimensions)
        SHADER_PARAMETER(int, LumaMipLevelToUse)
        SHADER_PARAMETER(int, FrameIndex)
        SHADER_PARAMETER(FVector4f, DeviceToViewDepth)
        SHADER_PARAMETER(FVector2f, Jitter)
        SHADER_PARAMETER(FVector2f, MotionVectorScale)
        SHADER_PARAMETER(FVector2f, DownscaleFactor)
        SHADER_PARAMETER(FVector2f, MotionVectorJitterCancellation)
        SHADER_PARAMETER(float, PreExposure)
        SHADER_PARAMETER(float, PreviousFramePreExposure)
        SHADER_PARAMETER(float, TanHalfFOV)
        SHADER_PARAMETER(float, JitterSequenceLength)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, DynamicResChangeFactor)
        SHADER_PARAMETER(float, ViewSpaceToMetersFactor)
    END_SHADER_PARAMETER_STRUCT()
};

class FTSSDepthClipCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSDepthClipCS);
    SHADER_USE_PARAMETER_STRUCT(FTSSDepthClipCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ReconstructedPrevDepthTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DilatedMotionVectorsTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DilatedDepthTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ReactiveMaskTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, TransparencyCompositionMaskTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, PrevDilatedMotionVectorsTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputMotionVectorsTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputColorTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputDepthTex)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, DilatedReactiveMasks)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, PreparedInputColor)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
        SHADER_PARAMETER(FIntPoint, RenderSize)
        SHADER_PARAMETER(FIntPoint, MaxRenderSize)
        SHADER_PARAMETER(FIntPoint, DisplaySize)
        SHADER_PARAMETER(FIntPoint, InputColorResourceDimensions)
        SHADER_PARAMETER(FIntPoint, LumaMipDimensions)
        SHADER_PARAMETER(int, LumaMipLevelToUse)
        SHADER_PARAMETER(int, FrameIndex)
        SHADER_PARAMETER(FVector4f, DeviceToViewDepth)
        SHADER_PARAMETER(FVector2f, Jitter)
        SHADER_PARAMETER(FVector2f, MotionVectorScale)
        SHADER_PARAMETER(FVector2f, DownscaleFactor)
        SHADER_PARAMETER(FVector2f, MotionVectorJitterCancellation)
        SHADER_PARAMETER(float, PreExposure)
        SHADER_PARAMETER(float, PreviousFramePreExposure)
        SHADER_PARAMETER(float, TanHalfFOV)
        SHADER_PARAMETER(float, JitterSequenceLength)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, DynamicResChangeFactor)
        SHADER_PARAMETER(float, ViewSpaceToMetersFactor)
    END_SHADER_PARAMETER_STRUCT()
};

class FTSSLockCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSLockCS);
    SHADER_USE_PARAMETER_STRUCT(FTSSLockCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, LockInputLumaTex)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<unorm float>, NewLocks)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, ReconstructedPrevDepth)
        SHADER_PARAMETER(FIntPoint, RenderSize)
        SHADER_PARAMETER(FIntPoint, MaxRenderSize)
        SHADER_PARAMETER(FIntPoint, DisplaySize)
        SHADER_PARAMETER(FIntPoint, InputColorResourceDimensions)
        SHADER_PARAMETER(FIntPoint, LumaMipDimensions)
        SHADER_PARAMETER(int, LumaMipLevelToUse)
        SHADER_PARAMETER(int, FrameIndex)
        SHADER_PARAMETER(FVector4f, DeviceToViewDepth)
        SHADER_PARAMETER(FVector2f, Jitter)
        SHADER_PARAMETER(FVector2f, MotionVectorScale)
        SHADER_PARAMETER(FVector2f, DownscaleFactor)
        SHADER_PARAMETER(FVector2f, MotionVectorJitterCancellation)
        SHADER_PARAMETER(float, PreExposure)
        SHADER_PARAMETER(float, PreviousFramePreExposure)
        SHADER_PARAMETER(float, TanHalfFOV)
        SHADER_PARAMETER(float, JitterSequenceLength)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, DynamicResChangeFactor)
        SHADER_PARAMETER(float, ViewSpaceToMetersFactor)
    END_SHADER_PARAMETER_STRUCT()
};

class FTSSAccumulateCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSAccumulateCS);
    SHADER_USE_PARAMETER_STRUCT(FTSSAccumulateCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, PreparedInputColorTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DilatedReactiveMasksTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputMotionVectorsTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, LockStatusTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InternalUpscaledColorTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneLuminanceMipsTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, AutoExposureTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, LumaHistoryTex)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, NewLocksTex)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, InternalUpscaledOut)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, LockStatusOut)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, LumaHistoryOut)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, UpscaledOutput)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
        SHADER_PARAMETER(FIntPoint, RenderSize)
        SHADER_PARAMETER(FIntPoint, MaxRenderSize)
        SHADER_PARAMETER(FIntPoint, DisplaySize)
        SHADER_PARAMETER(FIntPoint, InputColorResourceDimensions)
        SHADER_PARAMETER(FIntPoint, LumaMipDimensions)
        SHADER_PARAMETER(int, LumaMipLevelToUse)
        SHADER_PARAMETER(int, FrameIndex)
        SHADER_PARAMETER(FVector4f, DeviceToViewDepth)
        SHADER_PARAMETER(FVector2f, Jitter)
        SHADER_PARAMETER(FVector2f, MotionVectorScale)
        SHADER_PARAMETER(FVector2f, DownscaleFactor)
        SHADER_PARAMETER(FVector2f, MotionVectorJitterCancellation)
        SHADER_PARAMETER(float, PreExposure)
        SHADER_PARAMETER(float, PreviousFramePreExposure)
        SHADER_PARAMETER(float, TanHalfFOV)
        SHADER_PARAMETER(float, JitterSequenceLength)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, DynamicResChangeFactor)
        SHADER_PARAMETER(float, ViewSpaceToMetersFactor)
    END_SHADER_PARAMETER_STRUCT()
};

class FTSSRCASCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSRCASCS);
    SHADER_USE_PARAMETER_STRUCT(FTSSRCASCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, RCASInputTex)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputColor)
        SHADER_PARAMETER(FIntPoint, DisplaySize)
        SHADER_PARAMETER(FIntPoint, RenderSize)
        SHADER_PARAMETER(FIntPoint, MaxRenderSize)
        SHADER_PARAMETER(FIntPoint, InputColorResourceDimensions)
        SHADER_PARAMETER(FIntPoint, LumaMipDimensions)
        SHADER_PARAMETER(int, LumaMipLevelToUse)
        SHADER_PARAMETER(int, FrameIndex)
        SHADER_PARAMETER(FVector4f, DeviceToViewDepth)
        SHADER_PARAMETER(FVector2f, Jitter)
        SHADER_PARAMETER(FVector2f, MotionVectorScale)
        SHADER_PARAMETER(FVector2f, DownscaleFactor)
        SHADER_PARAMETER(FVector2f, MotionVectorJitterCancellation)
        SHADER_PARAMETER(float, PreExposure)
        SHADER_PARAMETER(float, PreviousFramePreExposure)
        SHADER_PARAMETER(float, TanHalfFOV)
        SHADER_PARAMETER(float, JitterSequenceLength)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, DynamicResChangeFactor)
        SHADER_PARAMETER(float, ViewSpaceToMetersFactor)
        SHADER_PARAMETER(float, Sharpness)
    END_SHADER_PARAMETER_STRUCT()
};

class FTSSBlitPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FTSSBlitPS);
    SHADER_USE_PARAMETER_STRUCT(FTSSBlitPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
        SHADER_PARAMETER(FIntPoint, InputOffset)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};
