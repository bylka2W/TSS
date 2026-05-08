// TSS Temporal Reconstruction Compute Shader
// Single-Pass upscaling with Wave Intrinsics
// Compatible with DX12 HLSL 6.0+

#define TSS_Float16 min16float
#define TSS_Float2 min16float2
#define TSS_Float3 min16float3
#define TSS_Float4 min16float4

#define TSS_UINT2 min16uint2
#define TSS_UINT3 min16uint3
#define TSS_UINT4 min16uint4

#define TSS_STATIC const

// Shader resources
RWTexture2D<TSS_Float4> OutputTexture : register(u0);
Texture2D<TSS_Float4> CurrentFrame : register(t0);
Texture2D<TSS_Float4> HistoryFrame : register(t1);
Texture2D<TSS_Float2> MotionVectors : register(t2);
Texture2D<TSS_Float> DepthBuffer : register(t3);
Texture2D<TSS_Float> ReactiveMask : register(t4);
Texture2D<TSS_Float> LockMap : register(t5);
StructuredBuffer<float> RCASWeights : register(t6);

cbuffer TSSRCASConstants : register(b0)
{
    float4 ViewRect;
    float4 DepthViewRect;
    float2 InputSize;
    float2 OutputSize;
    float2 HistorySize;
    float2 MVScale;
    float2 Padding0;
    float RCASConfig;
    float LanczosASqrt;
    float DisocclusionThreshold;
    float LumaClampScale;
    float2 Padding1;
};

// Wave operations (HLSL intrinsic wrappers)
groupshared float gs_lumaMin[64];
groupshared float gs_lumaMax[64];
groupshared float gs_lumaSum[64];
groupshared float gs_depthMin[64];
groupshared float gs_depthMax[64];

TSS_STATIC const float PI = 3.14159265f;
TSS_STATIC const float RCAS_DENOISE = 0.25f;
TSS_STATIC const float EDGE_THRESHOLD = 0.02f;
TSS_STATIC const float LOCK_THRESHOLD = 0.95f;

TSS_Float4 TSSSampleHistory bilinear(TSS_Float2 uv)
{
    return HistoryFrame.SampleLevel(linear_sampler, uv, 0);
}

TSS_Float4 TSSSampleCurrent bilinear(TSS_Float2 uv)
{
    return CurrentFrame.SampleLevel(linear_sampler, uv, 0);
}

TSS_Float TSSRCAS_LanczosWeight(TSS_Float x, TSS_Float a)
{
    if (abs(x) < 0.00001f) return 1.0f;
    if (abs(x) >= a) return 0.0f;
    
    float pix = PI * x;
    float sinc = sin(pix) / pix;
    float lanczosWindow = sin(pix / a) / (pix / a);
    
    return sinc * lanczosWindow;
}

TSS_Float TSSRCAS_CatmullRomWeight(TSS_Float x)
{
    float absX = abs(x);
    
    if (absX < 1.0f)
    {
        return 1.5f * absX * absX * absX - 2.5f * absX * absX + 1.0f;
    }
    else if (absX < 2.0f)
    {
        float t = absX - 1.0f;
        return -0.5f * t * t * t + 0.5f * t * t;
    }
    
    return 0.0f;
}

TSS_Float TSSRCAS_LanczosSample(Texture2D tex, SamplerState samp, TSS_Float2 uv, TSS_Float2 texelSize, TSS_Float a)
{
    TSS_Float2 pixelPos = uv / texelSize;
    TSS_Float2 fractPos = frac(pixelPos);
    
    int startX = (int)floor(pixelPos.x) - int(a) + 1;
    int startY = (int)floor(pixelPos.y) - int(a) + 1;
    
    TSS_Float accum = 0.0f;
    TSS_Float weightSum = 0.0f;
    
    for (int y = 0; y < int(a) * 2; y++)
    {
        TSS_Float vWeight = TSSRCAS_LanczosWeight((TSS_Float)y - fractPos.y - (TSS_Float)(int(a)) + 1.0f, a);
        
        for (int x = 0; x < int(a) * 2; x++)
        {
            TSS_Float uWeight = TSSRCAS_LanczosWeight((TSS_Float)x - fractPos.x - (TSS_Float)(int(a)) + 1.0f, a);
            TSS_Float weight = uWeight * vWeight;
            
            TSS_Float2 sampleUV = TSS_Float2((TSS_Float)(startX + x) + 0.5f, (TSS_Float)(startY + y) + 0.5f) * texelSize;
            
            accum += tex.SampleLevel(samp, sampleUV, 0).r * weight;
            weightSum += weight;
        }
    }
    
    return weightSum > 0.0001f ? accum / weightSum : 0.0f;
}

TSS_Float TSSRCAS_CatmullRomSample(Texture2D tex, SamplerState samp, TSS_Float2 uv, TSS_Float2 texelSize)
{
    TSS_Float2 pixelPos = uv / texelSize;
    int ix = (int)floor(pixelPos.x);
    int iy = (int)floor(pixelPos.y);
    TSS_Float fx = pixelPos.x - (TSS_Float)ix;
    TSS_Float fy = pixelPos.y - (TSS_Float)iy;
    
    TSS_Float accum = 0.0f;
    
    for (int j = -1; j <= 2; j++)
    {
        TSS_Float vy = TSSRCAS_CatmullRomWeight(fy - (TSS_Float)j);
        
        for (int i = -1; i <= 2; i++)
        {
            TSS_Float vx = TSSRCAS_CatmullRomWeight(fx - (TSS_Float)i);
            
            int sx = clamp(ix + i, 0, (int)tex.GetWidth() - 1);
            int sy = clamp(iy + j, 0, (int)tex.GetHeight() - 1);
            
            TSS_Float2 sampleUV = TSS_Float2((TSS_Float)sx + 0.5f, (TSS_Float)sy + 0.5f) * texelSize;
            TSS_Float weight = vx * vy;
            
            accum += tex.SampleLevel(samp, sampleUV, 0).r * weight;
        }
    }
    
    return accum;
}

TSS_Float4 TSSRCAS_GetNeighborhood(Texture2D tex, SamplerState samp, TSS_Float2 uv, TSS_Float2 texelSize)
{
    TSS_Float4 samples;
    
    samples.x = tex.SampleLevel(samp, uv + TSS_Float2(-texelSize.x, 0), 0).r;
    samples.y = tex.SampleLevel(samp, uv + TSS_Float2(texelSize.x, 0), 0).r;
    samples.z = tex.SampleLevel(samp, uv + TSS_Float2(0, -texelSize.y), 0).r;
    samples.w = tex.SampleLevel(samp, uv + TSS_Float2(0, texelSize.y), 0).r;
    
    return samples;
}

void TSSWaveReduceLumaInit(groupshared float* gs_min, groupshared float* gs_max, groupshared float* gs_sum, uint localId)
{
    gs_min[localId] = 1e10f;
    gs_max[localId] = -1e10f;
    gs_sum[localId] = 0.0f;
    GroupMemoryBarrierWithGroupSync();
}

void TSSWaveReduceLumaAdd(groupshared float* gs_min, groupshared float* gs_max, groupshared float* gs_sum, uint localId, float luma)
{
    InterlockedMin(gs_min[0], asuint(luma));
    InterlockedMax(gs_max[0], asuint(luma));
    
    float slot = 0.0f;
    InterlockedAdd(gs_sum[0], asuint(luma), slot);
}

TSS_Float4 TSSWaveReduceLumaFinalize(groupshared float* gs_min, groupshared float* gs_max, groupshared float* gs_sum, uint localId, int waveCount)
{
    GroupMemoryBarrierWithGroupSync();
    
    TSS_Float4 result;
    result.x = asfloat(gs_min[0]);
    result.y = asfloat(gs_max[0]);
    result.z = asfloat(gs_sum[0]) / (TSS_Float)waveCount;
    result.w = result.y - result.x;
    
    if (localId == 0)
    {
        gs_min[0] = 1e10f;
        gs_max[0] = -1e10f;
        gs_sum[0] = 0.0f;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    return result;
}

TSS_Float TSSWaveActiveMin(TSS_Float value)
{
    return WaveActiveMin(value);
}

TSS_Float TSSWaveActiveMax(TSS_Float value)
{
    return WaveActiveMax(value);
}

TSS_Float TSSWaveActiveSum(TSS_Float value)
{
    return WaveActiveSum(value);
}

TSS_Float TSSWaveActiveAverage(TSS_Float value)
{
    return WaveActiveSum(value) / (TSS_Float)WaveGetLaneCount();
}

TSS_Float4 TSSRCAS_ColorClamp(TSS_Float4 center, TSS_Float4 neighborhood)
{
    TSS_Float4 minVals = TSSWaveActiveMin(TSS_Float4(center.r - 0.1f, center.g - 0.1f, center.b - 0.1f, 0));
    TSS_Float4 maxVals = TSSWaveActiveMax(TSS_Float4(center.r + 0.1f, center.g + 0.1f, center.b + 0.1f, 0));
    
    return clamp(center, minVals, maxVals);
}

TSS_Float TSSRCAS_LumaDiff(TSS_Float4 a, TSS_Float4 b)
{
    TSS_Float lumaA = dot(a.rgb, TSS_Float3(0.2126f, 0.7152f, 0.0722f));
    TSS_Float lumaB = dot(b.rgb, TSS_Float3(0.2126f, 0.7152f, 0.0722f));
    
    return abs(lumaA - lumaB);
}

TSS_Float TSSRCAS_DisocclusionCheck(TSS_Float currentDepth, TSS_Float historyDepth, TSS_Float2 mv)
{
    TSS_Float depthDiff = abs(currentDepth - historyDepth);
    TSS_Float mvMag = length(mv);
    
    TSS_Float disocclusion = depthDiff * 100.0f + mvMag * 0.1f;
    
    return disocclusion;
}

TSS_Float TSSRCAS_LockCheck(TSS_Float4 center, TSS_Float4 neighborhood, TSS_Float threshold)
{
    TSS_Float centerLuma = dot(center.rgb, TSS_Float3(0.2126f, 0.7152f, 0.0722f));
    TSS_Float neighborLuma = dot(neighborhood.rgb, TSS_Float3(0.2126f, 0.7152f, 0.0722f));
    
    TSS_Float lumaDiff = abs(centerLuma - neighborLuma);
    
    TSS_Float lockWeight = lumaDiff < threshold ? (threshold - lumaDiff) / threshold : 0.0f;
    
    return lockWeight;
}

TSS_Float4 TSSRCAS_ReactiveBlend(TSS_Float4 history, TSS_Float4 current, TSS_Float reactive, TSS_Float confidence)
{
    TSS_Float blendWeight = max(reactive, 1.0f - confidence);
    blendWeight = clamp(blendWeight, 0.0f, 1.0f);
    
    return lerp(history, current, blendWeight);
}

TSS_Float4 TSSRCAS_Accumulate(
    TSS_Float4 historyColor,
    TSS_Float4 currentColor,
    TSS_Float2 mv,
    TSS_Float currentDepth,
    TSS_Float historyDepth,
    TSS_Float reactive,
    TSS_Float lockWeight,
    TSS_Float disocclusionThreshold,
    TSS_Float lumaClampScale
)
{
    TSS_Float4 clampedHistory = TSSRCAS_ColorClamp(historyColor, currentColor);
    
    TSS_Float disocclusion = TSSRCAS_DisocclusionCheck(currentDepth, historyDepth, mv);
    
    TSS_Float lumaDiff = TSSRCAS_LumaDiff(currentColor, historyColor);
    
    TSS_Float confidence = 1.0f;
    confidence *= saturate(1.0f - disocclusion * 2.0f);
    confidence *= saturate(1.0f - lumaDiff * 0.5f);
    
    TSS_Float baseBlend = clamp(lumaDiff * 2.0f, 0.05f, 0.5f);
    baseBlend = max(baseBlend, 0.05f);
    
    if (disocclusion > disocclusionThreshold)
    {
        baseBlend = 1.0f;
    }
    
    baseBlend = lerp(baseBlend, 1.0f, lockWeight * 0.5f);
    baseBlend = lerp(baseBlend, 0.0f, reactive * 0.8f);
    
    TSS_Float4 result = lerp(clampedHistory, currentColor, baseBlend);
    
    return result;
}

TSS_Float2 TSSRCAS_Reproject(TSS_Float2 currentUv, TSS_Float2 mv, TSS_Float2 historySize, TSS_Float2 inputSize)
{
    TSS_Float2 historyUv = currentUv - mv / inputSize * historySize / inputSize;
    
    return historyUv;
}

TSS_Float4 TSSRCAS_EdgeDirectedSample(
    Texture2D tex,
    SamplerState samp,
    TSS_Float2 uv,
    TSS_Float2 texelSize,
    TSS_Float4 centerSample
)
{
    TSS_Float4 hSamples;
    hSamples.x = tex.SampleLevel(samp, uv + TSS_Float2(-texelSize.x * 2.0f, 0), 0).r;
    hSamples.y = tex.SampleLevel(samp, uv + TSS_Float2(-texelSize.x, 0), 0).r;
    hSamples.z = tex.SampleLevel(samp, uv + TSS_Float2(texelSize.x, 0), 0).r;
    hSamples.w = tex.SampleLevel(samp, uv + TSS_Float2(texelSize.x * 2.0f, 0), 0).r;
    
    TSS_Float4 vSamples;
    vSamples.x = tex.SampleLevel(samp, uv + TSS_Float2(0, -texelSize.y * 2.0f), 0).r;
    vSamples.y = tex.SampleLevel(samp, uv + TSS_Float2(0, -texelSize.y), 0).r;
    vSamples.z = tex.SampleLevel(samp, uv + TSS_Float2(0, texelSize.y), 0).r;
    vSamples.w = tex.SampleLevel(samp, uv + TSS_Float2(0, texelSize.y * 2.0f), 0).r;
    
    TSS_Float hGradient = abs(hSamples.z - hSamples.y);
    TSS_Float vGradient = abs(vSamples.z - vSamples.y);
    
    TSS_Float gradientStrength = sqrt(hGradient * hGradient + vGradient * vGradient);
    
    TSS_Float4 result;
    
    if (gradientStrength > EDGE_THRESHOLD)
    {
        if (hGradient > vGradient)
        {
            TSS_Float edgePos = hGradient > 0.0001f ? (centerSample.r - hSamples.y) / hGradient : 0.5f;
            edgePos = saturate(edgePos);
            
            TSS_Float leftSample = lerp(hSamples.y, hSamples.x, edgePos);
            TSS_Float rightSample = lerp(hSamples.z, hSamples.w, edgePos);
            
            result.r = lerp(leftSample, rightSample, 0.5f);
        }
        else
        {
            TSS_Float edgePos = vGradient > 0.0001f ? (centerSample.r - vSamples.y) / vGradient : 0.5f;
            edgePos = saturate(edgePos);
            
            TSS_Float topSample = lerp(vSamples.y, vSamples.x, edgePos);
            TSS_Float bottomSample = lerp(vSamples.z, vSamples.w, edgePos);
            
            result.r = lerp(topSample, bottomSample, 0.5f);
        }
    }
    else
    {
        result.r = centerSample.r;
    }
    
    return result;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    TSS_UINT2 pixelPos = dispatchThreadId.xy;
    
    if (pixelPos.x >= (uint)OutputSize.x || pixelPos.y >= (uint)OutputSize.y)
        return;
    
    TSS_Float2 inputUv = ((TSS_Float2)pixelPos + 0.5f) / OutputSize;
    TSS_Float2 inputTexelSize = 1.0f / InputSize;
    TSS_Float2 historyTexelSize = 1.0f / HistorySize;
    
    TSS_Float2 currentUv = ((TSS_Float2)pixelPos) / OutputSize;
    TSS_UINT2 currentPixelPos = TSS_UINT2(currentUv * InputSize);
    
    TSS_Float4 currentColor = TSSSampleCurrent(inputUv);
    
    TSS_Float2 mv = MotionVectors[currentPixelPos].xy * MVScale;
    
    TSS_Float2 historyUv = TSSRCAS_Reproject(currentUv, mv, HistorySize, InputSize);
    
    TSS_Float historyDepth = DepthBuffer.SampleLevel(linear_sampler, historyUv, 0).r;
    TSS_Float currentDepth = DepthBuffer.SampleLevel(linear_sampler, inputUv, 0).r;
    
    TSS_Float4 historyColor = TSSSampleHistory(historyUv);
    
    TSS_Float reactive = ReactiveMask.Load(TSS_UINT3(pixelPos, 0)).r;
    TSS_Float lock = LockMap.Load(TSS_UINT3(pixelPos, 0)).r;
    
    TSS_Float4 neighborhood = TSSRCAS_GetNeighborhood(CurrentFrame, linear_sampler, inputUv, inputTexelSize);
    TSS_Float lockWeight = TSSRCAS_LockCheck(currentColor, neighborhood, LOCK_THRESHOLD);
    lockWeight = max(lockWeight, lock);
    
    TSS_Float4 clampedCurrent = TSSRCAS_ColorClamp(currentColor, neighborhood);
    
    TSS_Float4 result = TSSRCAS_Accumulate(
        historyColor,
        clampedCurrent,
        mv,
        currentDepth,
        historyDepth,
        reactive,
        lockWeight,
        DisocclusionThreshold,
        LumaClampScale
    );
    
    TSS_Float4 sharpened = result;
    
    TSS_Float4 blurH = TSS_Float4(0.0f, 0.0f, 0.0f, 0.0f);
    blurH += TSSSampleCurrent(inputUv + TSS_Float2(-2.0f * inputTexelSize.x, 0)) * 0.06136f;
    blurH += TSSSampleCurrent(inputUv + TSS_Float2(-1.0f * inputTexelSize.x, 0)) * 0.24477f;
    blurH += TSSSampleCurrent(inputUv) * 0.38774f;
    blurH += TSSSampleCurrent(inputUv + TSS_Float2(1.0f * inputTexelSize.x, 0)) * 0.24477f;
    blurH += TSSSampleCurrent(inputUv + TSS_Float2(2.0f * inputTexelSize.x, 0)) * 0.06136f;
    
    TSS_Float4 blur = TSS_Float4(0.0f, 0.0f, 0.0f, 0.0f);
    blur += blurH * 0.06136f;
    blur += TSSRCAS_GetNeighborhood(CurrentFrame, linear_sampler, inputUv + TSS_Float2(0, -inputTexelSize.y), inputTexelSize) * 0.24477f;
    blur += TSS_Float4(currentColor.rgb * 0.38774f, 1.0f);
    blur += TSSRCAS_GetNeighborhood(CurrentFrame, linear_sampler, inputUv + TSS_Float2(0, inputTexelSize.y), inputTexelSize) * 0.24477f;
    blur += blurH * 0.06136f;
    
    TSS_Float sharpenAmount = 0.8f;
    sharpened = result + (result - blur) * sharpenAmount;
    
    OutputTexture[pixelPos] = sharpened;
}
