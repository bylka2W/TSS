#define TSS_WARP_VERSION "1.0.0"
#include "tss_common.hlsl"

RWTexture2D<float4> g_WarpedHistory : register(u0);
RWTexture2D<float> g_WarpConfidence : register(u1);

Texture2D<float4> g_HistoryColor : register(t0);
Texture2D<float> g_HistoryDepth : register(t1);
Texture2D<float2> g_HistoryMotionVector : register(t2);
Texture2D<float> g_CurrentDepth : register(t3);

cbuffer TSSWarpConstants : register(b0) {
    float g_MotionScale;
    float g_WarpRadius;
    float g_DilateRadius;
    float g_Padding1;
};

float2 GetDilatedMotionVector(float2 uv, float2 mv, float currentDepth) {
    float minDepth = currentDepth;
    float2 dilatedMV = mv;
    int dilateCount = 0;
    
    float offsets[8] = { -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f };
    
    for (int i = 0; i < 8; i += 2) {
        float2 sampleUV = uv + float2(offsets[i], offsets[i + 1]) * g_texelSize * g_DilateRadius;
        float sampleDepth = g_CurrentDepth.SampleLevel(g_PointSampler, sampleUV, 0.0f);
        
        if (sampleDepth < minDepth) {
            minDepth = sampleDepth;
            float2 neighborMV = g_HistoryMotionVector.SampleLevel(g_PointSampler, sampleUV, 0.0f) * g_MotionScale;
            
            float mvDiff = length(mv - neighborMV);
            float weight = 1.0f / (1.0f + mvDiff * 10.0f);
            
            dilatedMV = lerp(dilatedMV, neighborMV, weight);
            dilateCount++;
        }
    }
    
    return dilatedMV;
}

float2 ForwardProject(float2 uv, float2 mv, float dt) {
    return uv - mv * dt * 0.5f;
}

float2 BackwardProject(float2 uv, float2 mv) {
    return uv + mv;
}

float4 SampleHistory(float2 uv) {
    uv = clamp(uv, 0.0f, 1.0f);
    return g_HistoryColor.SampleLevel(g_LinearSampler, uv, 0.0f);
}

float SampleHistoryDepth(float2 uv) {
    uv = clamp(uv, 0.0f, 1.0f);
    return g_HistoryDepth.SampleLevel(g_PointSampler, uv, 0.0f);
}

float4 BilinearSampleHistory(float2 uv) {
    float2 texel = uv * g_Resolution - 0.5f;
    float2 frac = frac(texel);
    float2 texelUV = (floor(texel) + 0.5f) / g_Resolution;
    
    float4 s00 = SampleHistory(texelUV);
    float4 s10 = SampleHistory(texelUV + float2(g_texelSize.x, 0.0f));
    float4 s01 = SampleHistory(texelUV + float2(0.0f, g_texelSize.y));
    float4 s11 = SampleHistory(texelUV + g_texelSize);
    
    float4 top = lerp(s00, s10, frac.x);
    float4 bottom = lerp(s01, s11, frac.x);
    
    return lerp(top, bottom, frac.y);
}

float4 BicubicSampleHistory(float2 uv) {
    float2 texel = uv * g_Resolution - 0.5f;
    float2 texelUV = texel / g_Resolution;
    float2 frac = frac(texel);
    
    float2 f = frac;
    
    float2 w0 = ((-0.75f * f + 1.5f) * f - 1.125f) * f + 0.5625f;
    float2 w1 = ((-0.75f * f + 0.75f) * f + 0.75f) * f + 0.25f;
    float2 w2 = ((0.75f * f - 1.5f) * f + 0.75f) * f + 0.5625f;
    float2 w3 = ((0.75f * f - 0.75f) * f - 0.75f) * f + 0.25f;
    
    float2 w12 = 1.0f - w0 - w1 - w2 - w3;
    
    float2 start = floor(texel) - 1.0f;
    if (start.x < 0.0f) { w0.x += w1.x; w1.x = w12.x; w12.x = 0.0f; start.x = 0.0f; }
    if (start.y < 0.0f) { w0.y += w1.y; w1.y = w12.y; w12.y = 0.0f; start.y = 0.0f; }
    
    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float2 uv0 = (start + 0.5f) / g_Resolution;
    float2 uv1 = (start + 1.5f) / g_Resolution;
    float2 uv2 = (start + 2.5f) / g_Resolution;
    float2 uv3 = (start + 3.5f) / g_Resolution;
    
    result += SampleHistory(uv0) * (w0.x * w0.y);
    result += SampleHistory(uv1) * (w1.x * w0.y);
    result += SampleHistory(uv2) * (w2.x * w0.y);
    result += SampleHistory(uv3) * (w3.x * w0.y);
    
    result += SampleHistory(uv0 + float2(0.0f, g_texelSize.y)) * (w0.x * w1.y);
    result += SampleHistory(uv1 + float2(0.0f, g_texelSize.y)) * (w1.x * w1.y);
    result += SampleHistory(uv2 + float2(0.0f, g_texelSize.y)) * (w2.x * w1.y);
    result += SampleHistory(uv3 + float2(0.0f, g_texelSize.y)) * (w3.x * w1.y);
    
    result += SampleHistory(uv0 + float2(0.0f, g_texelSize.y * 2.0f)) * (w0.x * w2.y);
    result += SampleHistory(uv1 + float2(0.0f, g_texelSize.y * 2.0f)) * (w1.x * w2.y);
    result += SampleHistory(uv2 + float2(0.0f, g_texelSize.y * 2.0f)) * (w2.x * w2.y);
    result += SampleHistory(uv3 + float2(0.0f, g_texelSize.y * 2.0f)) * (w3.x * w2.y);
    
    result += SampleHistory(uv0 + float2(0.0f, g_texelSize.y * 3.0f)) * (w0.x * w3.y);
    result += SampleHistory(uv1 + float2(0.0f, g_texelSize.y * 3.0f)) * (w1.x * w3.y);
    result += SampleHistory(uv2 + float2(0.0f, g_texelSize.y * 3.0f)) * (w2.x * w3.y);
    result += SampleHistory(uv3 + float2(0.0f, g_texelSize.y * 3.0f)) * (w3.x * w3.y);
    
    return result;
}

[numthreads(8, 8, 1)]
void WarpHistoryCS(uint3 dispatchId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchId.xy;
    if (any(pixelCoord >= g_Resolution)) return;
    
    float2 uv = (float2(pixelCoord) + 0.5f) * g_texelSize;
    
    float currentDepth = g_CurrentDepth[pixelCoord];
    float2 mv = g_HistoryMotionVector[pixelCoord] * g_MotionScale;
    
    float2 dilatedMV = GetDilatedMotionVector(uv, mv, currentDepth);
    
    float2 forwardUV = ForwardProject(uv, dilatedMV, g_TimeDelta);
    float2 backwardUV = BackwardProject(uv, dilatedMV);
    
    float4 forwardSample = BicubicSampleHistory(forwardUV);
    float4 backwardSample = BilinearSampleHistory(backwardUV);
    
    float forwardDepth = SampleHistoryDepth(forwardUV);
    float backwardDepth = SampleHistoryDepth(backwardUV);
    
    float depthDiffF = abs(currentDepth - forwardDepth);
    float depthDiffB = abs(currentDepth - backwardDepth);
    
    float depthWeightF = 1.0f - saturate(depthDiffF * 100.0f);
    float depthWeightB = 1.0f - saturate(depthDiffB * 100.0f);
    
    float mvLen = length(dilatedMV);
    float mvWeightF = 1.0f - saturate(mvLen * 0.5f);
    float mvWeightB = 1.0f - saturate(mvLen * 0.3f);
    
    float totalWeightF = depthWeightF * mvWeightF;
    float totalWeightB = depthWeightB * mvWeightB;
    
    float4 warpedColor;
    if (totalWeightF + totalWeightB > 0.0f) {
        warpedColor = (forwardSample * totalWeightF + backwardSample * totalWeightB) / (totalWeightF + totalWeightB);
    } else {
        warpedColor = SampleHistory(backwardUV);
    }
    
    float warpConfidence = saturate((totalWeightF + totalWeightB) * 0.5f);
    
    g_WarpedHistory[pixelCoord] = warpedColor;
    g_WarpConfidence[pixelCoord] = warpConfidence;
}
