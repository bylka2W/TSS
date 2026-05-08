#define TSS_BLEND_VERSION "1.0.0"
#include "tss_common.hlsl"
#include "tss_neural.hlsl"

RWTexture2D<float4> g_BlendedColor : register(u0);
RWTexture2D<float> g_BlendedConfidence : register(u1);

Texture2D<float4> g_CurrentColor : register(t0);
Texture2D<float> g_CurrentDepth : register(t1);
Texture2D<float2> g_CurrentMotionVector : register(t2);
Texture2D<float3> g_CurrentNormal : register(t3);
Texture2D<float4> g_WarpedHistory : register(t4);
Texture2D<float> g_WarpedConfidence : register(t5);
Texture2D<float> g_HistoryConfidence : register(t6);

cbuffer TSSBlendConstants : register(b0) {
    float g_KSigma;
    float g_MinAlpha;
    float g_MaxAlpha;
    float g_NeuralBlendStrength;
    float g_DilationRadius;
    float g_Sharpness;
    float g_JitterStabilization;
    float g_StabilityFrames;
};

float3 EdgePreservingBlend(float2 uv, float3 currentYCC, float3 neighborhood[9]) {
    float3 colors[9];
    float weights[9];
    float totalWeight = 0.0f;
    
    for (int i = 0; i < 9; i++) {
        colors[i] = neighborhood[i];
        float centerLuma = neighborhood[4].x;
        float neighborLuma = neighborhood[i].x;
        float lumaDiff = abs(centerLuma - neighborLuma);
        weights[i] = 1.0f / (1.0f + lumaDiff * 10.0f);
        totalWeight += weights[i];
    }
    
    float3 edgeAware = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 9; i++) {
        edgeAware += colors[i] * (weights[i] / totalWeight);
    }
    
    return edgeAware;
}

float JitterStabilization(float velocity, float stabilityFrames) {
    if (velocity < 0.01f) {
        return min(1.0f, g_JitterStabilization * (stabilityFrames + 0.1f));
    }
    return 1.0f;
}

float AdaptiveSharpness(float centerLuma, float neighborLuma, float edgeThreshold) {
    float diff = abs(centerLuma - neighborLuma);
    
    if (diff < edgeThreshold) {
        return 0.0f;
    }
    
    return min(diff * 2.0f, 1.0f);
}

float4 BidirectionalSplat(
    float2 uv,
    float2 mv,
    float depth,
    float3 currentYCC
) {
    float2 forwardUV = uv - mv * g_TimeDelta * 0.5f;
    float2 backwardUV = uv + mv;
    
    forwardUV = clamp(forwardUV, 0.0f, 1.0f);
    backwardUV = clamp(backwardUV, 0.0f, 1.0f);
    
    float4 forwardColor = g_WarpedHistory.SampleLevel(g_LinearSampler, forwardUV, 0.0f);
    float4 backwardColor = g_WarpedHistory.SampleLevel(g_LinearSampler, backwardUV, 0.0f);
    
    float forwardConf = g_WarpedConfidence.SampleLevel(g_PointSampler, forwardUV, 0.0f);
    float backwardConf = g_WarpedConfidence.SampleLevel(g_PointSampler, backwardUV, 0.0f);
    
    float forwardDepth = g_CurrentDepth.SampleLevel(g_PointSampler, forwardUV, 0.0f);
    float backwardDepth = g_CurrentDepth.SampleLevel(g_PointSampler, backwardUV, 0.0f);
    
    float depthDiffF = abs(depth - forwardDepth);
    float depthDiffB = abs(depth - backwardDepth);
    
    float depthWeightF = 1.0f - saturate(depthDiffF * 100.0f);
    float depthWeightB = 1.0f - saturate(depthDiffB * 100.0f);
    
    float mvLen = length(mv);
    float mvWeightF = 1.0f - saturate(mvLen * 0.5f);
    float mvWeightB = 1.0f - saturate(mvLen * 0.3f);
    
    float totalWeightF = depthWeightF * mvWeightF * forwardConf;
    float totalWeightB = depthWeightB * mvWeightB * backwardConf;
    
    float4 bidirColor;
    if (totalWeightF + totalWeightB > 0.001f) {
        bidirColor = (forwardColor * totalWeightF + backwardColor * totalWeightB) / (totalWeightF + totalWeightB);
    } else {
        bidirColor = backwardColor;
    }
    
    float bidirConfidence = saturate((totalWeightF + totalWeightB) * 0.5f);
    
    return float4(bidirColor.rgb, bidirConfidence);
}

float3 FillDisocclusionHoles(float2 uv, float3 currentYCC, float disocclusionMask) {
    if (disocclusionMask < 0.3f) return currentYCC;
    
    float3 colors[8];
    float depths[8];
    int validCount = 0;
    
    float offsets[8] = { -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f };
    
    for (int i = 0; i < 8; i += 2) {
        float2 neighborUV = uv + float2(offsets[i], offsets[i + 1]) * g_texelSize;
        
        if (any(neighborUV < 0.0f) || any(neighborUV > 1.0f)) continue;
        
        float neighborDepth = g_CurrentDepth.SampleLevel(g_PointSampler, neighborUV, 0.0f);
        
        if (abs(neighborDepth - depths[validCount]) < 0.01f) {
            float3 neighborColor = g_CurrentColor.SampleLevel(g_PointSampler, neighborUV, 0.0f).rgb;
            colors[validCount] = RGBToYCoCg(neighborColor);
            depths[validCount] = neighborDepth;
            validCount++;
        }
    }
    
    if (validCount == 0) return currentYCC;
    
    float3 sumColor = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    
    for (int i = 0; i < validCount; i++) {
        float depthDiff = abs(depths[i] - depth);
        float weight = 1.0f / (1.0f + depthDiff * 10.0f);
        
        if (depthDiff < 0.01f) weight *= 2.0f;
        
        sumColor += colors[i] * weight;
        totalWeight += weight;
    }
    
    float3 filledColor = sumColor / max(totalWeight, 0.001f);
    
    float blendFactor = saturate((disocclusionMask - 0.3f) / 0.7f);
    return lerp(currentYCC, filledColor, blendFactor);
}

[numthreads(8, 8, 1)]
void BlendHistoryCS(uint3 dispatchId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchId.xy;
    if (any(pixelCoord >= g_Resolution)) return;
    
    float2 uv = (float2(pixelCoord) + 0.5f) * g_texelSize;
    
    float depth = g_CurrentDepth[pixelCoord];
    float2 mv = g_CurrentMotionVector[pixelCoord] * g_MotionScale;
    float3 normal = g_CurrentNormal.SampleLevel(g_PointSampler, uv, 0.0f).xyz;
    normal = DecodeNormal(normal);
    
    float3 currentRGB = g_CurrentColor[pixelCoord].rgb;
    float3 currentYCC = RGBToYCoCg(currentRGB);
    
    float historyConfidence = g_HistoryConfidence[pixelCoord];
    float stabilityFrames = g_StabilityFrames;
    
    float3 historyNormal = normal;
    float4 warpedData = g_WarpedHistory[pixelCoord];
    float warpedConfidence = g_WarpedConfidence[pixelCoord];
    
    float3 warpedYCC = RGBToYCoCg(warpedData.rgb);
    
    float3 neighborhood[9];
    int idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            uint2 neighborCoord = pixelCoord + uint2(dx, dy);
            if (any(neighborCoord >= g_Resolution)) {
                neighborhood[idx++] = currentYCC;
                continue;
            }
            float3 neighborColor = g_CurrentColor[neighborCoord].rgb;
            neighborhood[idx++] = RGBToYCoCg(neighborColor);
        }
    }
    
    float sum = 0.0f, sumSq = 0.0f;
    for (int i = 0; i < 9; i++) {
        sum += neighborhood[i].x;
        sumSq += neighborhood[i].x * neighborhood[i].x;
    }
    float mean = sum / 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    float stdDev = sqrt(max(variance, 0.0f));
    
    warpedYCC.x = VarianceClipping(warpedYCC.x, currentYCC.x, mean, stdDev, g_KSigma);
    warpedYCC.y = VarianceClipping(warpedYCC.y, currentYCC.y, 0.0f, 0.1f, g_KSigma * 2.0f);
    warpedYCC.z = VarianceClipping(warpedYCC.z, currentYCC.z, 0.0f, 0.1f, g_KSigma * 2.0f);
    
    float disocclusion = ComputeDisocclusionMask(depth, depth, mv, mv, normal, historyNormal);
    float velocity = length(mv);
    float lumaDiff = abs(currentYCC.x - warpedYCC.x);
    
    float historyDepth = depth;
    float neuralWeight = PredictHistoryWeight(
        depth, historyDepth, mv, mv,
        currentYCC.x, warpedYCC.x, stdDev,
        normal, historyNormal,
        g_TimeDelta
    );
    
    float jitter = ComputeJitterScore(currentYCC, warpedYCC, velocity);
    float ghosting = ComputeGhostingScore(currentYCC, warpedYCC, velocity);
    
    float4 bidirSplat = BidirectionalSplat(uv, mv, depth, currentYCC);
    float3 bidirYCC = RGBToYCoCg(bidirSplat.rgb);
    float bidirConfidence = bidirSplat.a;
    
    currentYCC = lerp(currentYCC, bidirYCC, bidirConfidence * 0.3f);
    
    float3 filledYCC = FillDisocclusionHoles(uv, currentYCC, disocclusion);
    currentYCC = lerp(currentYCC, filledYCC, disocclusion * 0.5f);
    
    float4 repaired = NeuralRepair(
        float4(currentYCC, 1.0f),
        float4(warpedYCC, 1.0f),
        disocclusion,
        neuralWeight,
        velocity,
        lumaDiff
    );
    currentYCC = repaired.rgb;
    
    float3 edgeHistory = EdgePreservingBlend(uv, currentYCC, neighborhood);
    currentYCC = lerp(currentYCC, edgeHistory, 0.2f);
    
    float stabilization = JitterStabilization(velocity, stabilityFrames);
    currentYCC = lerp(currentYCC, warpedYCC, stabilization * 0.1f);
    
    float neighborsSharp[8];
    idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            neighborsSharp[idx++] = neighborhood[(dy + 1) * 3 + (dx + 1)].x;
        }
    }
    
    float sharpness = AdaptiveSharpness(currentYCC.x, neighborsSharp[4], 0.05f);
    float finalSharpness = sharpness * g_Sharpness * 0.3f;
    currentYCC = lerp(currentYCC, warpedYCC, finalSharpness);
    
    float baseAlpha = g_MinAlpha;
    baseAlpha += velocity * 0.2f;
    baseAlpha += disocclusion * 0.3f;
    baseAlpha += jitter * 0.1f;
    baseAlpha += ghosting * 0.2f;
    
    float stabilityBoost = min(stabilityFrames / 60.0f, 0.3f);
    baseAlpha -= stabilityBoost;
    
    baseAlpha = clamp(baseAlpha, g_MinAlpha, g_MaxAlpha);
    baseAlpha *= (historyConfidence + 0.1f);
    
    float3 accumulatedYCC = lerp(warpedYCC, currentYCC, baseAlpha);
    
    float3 finalRGB = YCoCgToRGB(accumulatedYCC);
    
    float newStability = disocclusion > 0.5f ? 0.0f : min(stabilityFrames + (1.0f / 60.0f), 1.0f);
    
    float outputConfidence = (1.0f - disocclusion) * (1.0f - jitter * 0.5f) * (1.0f - ghosting * 0.3f);
    outputConfidence = saturate(outputConfidence);
    
    g_BlendedColor[pixelCoord] = float4(saturate(finalRGB), newStability);
    g_BlendedConfidence[pixelCoord] = outputConfidence;
}
