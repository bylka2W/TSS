#define TSS_CORE_VERSION "1.0.0"

#define TSS_WAVE_SIZE 32
#define TSS_LDS_SIZE 16
#define TSS_NEURAL_FP16 1

RWTexture2D<float4> g_OutputColor : register(u0);
RWTexture2D<float> g_OutputDepth : register(u1);
RWTexture2D<float2> g_OutputMotionVector : register(u2);
RWTexture2D<float> g_OutputConfidence : register(u3);
RWTexture2D<float> g_OutputRepairMask : register(u4);

Texture2D<float4> g_CurrentColor : register(t0);
Texture2D<float> g_CurrentDepth : register(t1);
Texture2D<float2> g_CurrentMotionVector : register(t2);
Texture2D<float4> g_HistoryColor : register(t3);
Texture2D<float> g_HistoryDepth : register(t4);
Texture2D<float2> g_HistoryMotionVector : register(t5);
Texture2D<float> g_HistoryConfidence : register(t6);
Texture2D<float> g_VelocityBuffer : register(t7);

cbuffer TSSCoreConstants : register(b0) {
    uint2 g_Resolution;
    uint2 g_HistoryResolution;
    float g_JitterX;
    float g_JitterY;
    float g_Sharpness;
    float g_KSigmaY;
    float g_KSigmaChroma;
    float g_MotionScale;
    float g_DisocclusionThreshold;
    float g_VelocityThreshold;
    float g_LumaThreshold;
    float g_EntropyMax;
    float g_MinAlpha;
    float g_MaxAlpha;
    float g_RepairStrength;
    float g_StabilizationFactor;
    int g_FrameIndex;
    int g_EnableYCoCg;
    int g_EnableForwardProject;
    int g_EnableNeuralRepair;
    int g_EnableLumaRepair;
    int g_EnableAsyncCompute;
    int g_EnableDilatedMV;
    int g_EnableJitterStabilization;
    float g_DeltaTime;
    float2 g_texelSize;
};

groupshared float4 gs_LDSColor[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float gs_LDSDepth[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float2 gs_LDSMotion[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float gs_LDSConfidence[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float gs_LDSRepair[TSS_LDS_SIZE * TSS_LDS_SIZE];

#if TSS_NEURAL_FP16
typedef min16float4 color4;
typedef min16float3 color3;
typedef min16float color1;
#else
typedef float4 color4;
typedef float3 color3;
typedef float color1;
#endif

float3 RGBToYCoCg(float3 rgb) {
    float y = 0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b;
    float co = 0.5f * rgb.r - 0.5f * rgb.b;
    float cg = -0.25f * rgb.r + 0.5f * rgb.g - 0.25f * rgb.b;
    return float3(y, co, cg);
}

float3 YCoCgToRGB(float3 ycocg) {
    float y = ycocg.x;
    float co = ycocg.y;
    float cg = ycocg.z;
    float r = y + co - cg;
    float g = y + cg;
    float b = y - co - cg;
    return float3(r, g, b);
}

float CalculateLuma(float3 color) {
    return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
}

float2 GetDilatedMV(float2 uv, float2 currentMV) {
    if (!g_EnableDilatedMV) return currentMV;
    
    float minDepth = g_CurrentDepth.SampleLevel(g_CurrentDepth, uv, 0.0f);
    float2 dilatedMV = currentMV;
    
    float2 offsets[8] = {
        float2(-1, 0), float2(1, 0), float2(0, -1), float2(0, 1),
        float2(-1, -1), float2(1, -1), float2(-1, 1), float2(1, 1)
    };
    
    float mvSumX = 0.0f, mvSumY = 0.0f;
    float weightSum = 0.0f;
    int validCount = 0;
    
    for (int i = 0; i < 8; i++) {
        float2 sampleUV = uv + offsets[i] * g_texelSize;
        float sampleDepth = g_CurrentDepth.SampleLevel(g_CurrentDepth, sampleUV, 0.0f);
        
        if (sampleDepth < minDepth) {
            minDepth = sampleDepth;
            float2 neighborMV = g_CurrentMotionVector.SampleLevel(g_CurrentMotionVector, sampleUV, 0.0f) * g_MotionScale;
            
            float weight = 1.0f / (1.0f + length(sampleMV - currentMV));
            mvSumX += neighborMV.x * weight;
            mvSumY += neighborMV.y * weight;
            weightSum += weight;
            validCount++;
        }
    }
    
    if (validCount > 0 && weightSum > 0.0f) {
        dilatedMV = float2(mvSumX / weightSum, mvSumY / weightSum);
    }
    
    return dilatedMV;
}

float2 ForwardProjectUV(float2 uv, float2 mv, float dt) {
    float2 projectedUV = uv - mv * dt * 0.5f;
    projectedUV = clamp(projectedUV, 0.0f, 1.0f);
    return projectedUV;
}

float2 BackwardProjectUV(float2 uv, float2 mv) {
    float2 projectedUV = uv + mv;
    projectedUV = clamp(projectedUV, 0.0f, 1.0f);
    return projectedUV;
}

float DetectDisocclusion(float2 uv, float currentDepth, float2 currentMV, float2 historyMV) {
    float historyDepth = g_HistoryDepth.SampleLevel(g_HistoryDepth, uv, 0.0f);
    
    float depthDiff = abs(currentDepth - historyDepth);
    float mvMagnitude = length(currentMV);
    
    float depthScore = depthDiff * 100.0f;
    float mvScore = min(mvMagnitude * 0.05f, 0.3f);
    
    float mvDivergence = length(currentMV - historyMV);
    float divergenceScore = mvDivergence * 0.2f;
    
    float2 dilatedMV = GetDilatedMV(uv, currentMV);
    float dilatedScore = length(currentMV - dilatedMV) * 0.1f;
    
    float disocclusionScore = depthScore + mvScore + divergenceScore + dilatedScore;
    
    return step(g_DisocclusionThreshold, disocclusionScore);
}

float DetectJitter(float2 uv, float3 currentYCC, float3 historyYCC, float velocity) {
    float3 diff = abs(currentYCC - historyYCC);
    float lumaDiff = abs(diff.x);
    
    float velocityPenalty = step(0.1f, velocity) * 0.3f;
    
    float jitterScore = lumaDiff * 3.0f + velocityPenalty;
    
    float2 leftUV = uv + float2(-1.0f, 0.0f) * g_texelSize;
    float2 rightUV = uv + float2(1.0f, 0.0f) * g_texelSize;
    float2 upUV = uv + float2(0.0f, -1.0f) * g_texelSize;
    float2 downUV = uv + float2(0.0f, 1.0f) * g_texelSize;
    
    float3 leftColor = g_CurrentColor.SampleLevel(g_CurrentColor, leftUV, 0.0f).rgb;
    float3 rightColor = g_CurrentColor.SampleLevel(g_CurrentColor, rightUV, 0.0f).rgb;
    float3 upColor = g_CurrentColor.SampleLevel(g_CurrentColor, upUV, 0.0f).rgb;
    float3 downColor = g_CurrentColor.SampleLevel(g_CurrentColor, downUV, 0.0f).rgb;
    
    float3 gradX = abs(currentYCC - (g_EnableYCoCg ? RGBToYCoCg(leftColor) : leftColor)) +
                   abs((g_EnableYCoCg ? RGBToYCoCg(rightColor) : rightColor) - currentYCC);
    float3 gradY = abs(currentYCC - (g_EnableYCoCg ? RGBToYCoCg(upColor) : upColor)) +
                   abs((g_EnableYCoCg ? RGBToYCoCg(downColor) : downColor) - currentYCC);
    
    float edgeStrength = (gradX.x + gradY.x) * 0.5f;
    jitterScore *= (1.0f + edgeStrength * 0.3f);
    
    return saturate(jitterScore);
}

float DetectGhosting(float3 currentYCC, float3 historyYCC, float velocity) {
    float3 diff = abs(currentYCC - historyYCC);
    float yDiff = diff.x;
    float chromaDiff = (abs(diff.y) + abs(diff.z)) * 0.5f;
    
    float ghostingScore = yDiff * 2.0f + chromaDiff * 0.5f;
    
    float velocityBoost = step(0.5f, velocity) * 0.2f;
    ghostingScore += velocityBoost;
    
    return saturate(ghostingScore);
}

float CalculateEntropy(float3 neighborhood[9]) {
    float lumas[9];
    float sum = 0.0f, sumSq = 0.0f;
    
    for (int i = 0; i < 9; i++) {
        lumas[i] = g_EnableYCoCg ? neighborhood[i].x : CalculateLuma(neighborhood[i]);
        sum += lumas[i];
        sumSq += lumas[i] * lumas[i];
    }
    
    float mean = sum / 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    return sqrt(max(variance, 0.0f));
}

float VarianceClipping(float historyValue, float currentValue, float mean, float stdDev, float kSigma) {
    float minBound = mean - kSigma * stdDev;
    float maxBound = mean + kSigma * stdDev;
    
    if (historyValue < minBound || historyValue > maxBound) {
        float clip = min((maxBound - minBound) / (abs(historyValue - mean) + 0.0001f), 1.0f);
        return lerp(historyValue, currentValue, clip * 0.5f);
    }
    
    return historyValue;
}

float4 NeuralRepairLuma(float3 currentYCC, float3 historyYCC, float3 neighborhood[9],
                        float disocclusion, float jitter, float ghosting, float velocity,
                        float entropy, float repairStrength) {
    float featureY = disocclusion * 0.4f + jitter * 0.3f + ghosting * 0.3f;
    featureY = saturate(featureY);
    
    float repairWeight = featureY * repairStrength;
    
    float lumaRepair = lerp(historyYCC.x, currentYCC.x, repairWeight);
    
    float4 result;
    result.x = lumaRepair;
    result.y = historyYCC.y;
    result.z = historyYCC.z;
    result.w = saturate(1.0f - disocclusion);
    
    if (disocclusion > 0.8f) {
        result.x = currentYCC.x;
        result.w = 0.0f;
    }
    
    return result;
}

float3 LumaRepair(float3 current, float3 history, float entropy, float lumaThreshold) {
    float currentY = g_EnableYCoCg ? current.x : CalculateLuma(current);
    float historyY = g_EnableYCoCg ? history.x : CalculateLuma(history);
    
    float lumaDiff = abs(currentY - historyY);
    
    float adaptiveThreshold = lumaThreshold * (1.0f + entropy * 0.5f);
    
    if (lumaDiff > adaptiveThreshold) {
        return current;
    }
    
    return lerp(history, current, 0.3f);
}

float3 EdgePreservingBlend(float2 uv, float3 history) {
    float3 colors[9];
    float weights[9];
    float totalWeight = 0.0f;
    
    int idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            float2 sampleUV = uv + float2(dx, dy) * g_texelSize;
            colors[idx] = g_CurrentColor.SampleLevel(g_CurrentColor, sampleUV, 0.0f).rgb;
            
            if (g_EnableYCoCg) colors[idx] = RGBToYCoCg(colors[idx]);
            
            float centerLuma = colors[4].x;
            float neighborLuma = colors[idx].x;
            float lumaDiff = abs(centerLuma - neighborLuma);
            
            weights[idx] = 1.0f / (1.0f + lumaDiff * 10.0f);
            totalWeight += weights[idx];
            idx++;
        }
    }
    
    float3 edgeAware = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 9; i++) {
        edgeAware += colors[i] * (weights[i] / totalWeight);
    }
    
    return edgeAware;
}

float JitterStabilization(float velocity, float stabilityFrames) {
    if (velocity < 0.01f) {
        return min(1.0f, g_StabilizationFactor * (stabilityFrames + 0.1f));
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

void CalculateBidirectionalProjection(float2 uv, float2 mv, float depth, 
                                    out float3 forwardColor, out float3 backwardColor,
                                    out float forwardWeight, out float backwardWeight) {
    float2 forwardUV = ForwardProjectUV(uv, mv, g_DeltaTime);
    float2 backwardUV = BackwardProjectUV(uv, -mv);
    
    forwardColor = g_CurrentColor.SampleLevel(g_CurrentColor, forwardUV, 0.0f).rgb;
    backwardColor = g_HistoryColor.SampleLevel(g_HistoryColor, backwardUV, 0.0f).rgb;
    
    if (g_EnableYCoCg) {
        forwardColor = RGBToYCoCg(forwardColor);
        backwardColor = RGBToYCoCg(backwardColor);
    }
    
    float historyDepth = g_HistoryDepth.SampleLevel(g_HistoryDepth, backwardUV, 0.0f);
    float depthConsistency = 1.0f - saturate(abs(depth - historyDepth) * 100.0f);
    
    float mvMagnitude = length(mv);
    
    forwardWeight = 0.5f + (1.0f - mvMagnitude * g_MotionScale) * 0.3f;
    backwardWeight = 0.5f + depthConsistency * 0.3f;
    
    float3 diff = abs(forwardColor - backwardColor);
    float consensus = 1.0f - saturate((diff.x + diff.y + diff.z) / 3.0f);
    
    forwardWeight *= (0.5f + consensus * 0.5f);
    backwardWeight *= (0.5f + consensus * 0.5f);
}

float3 CalculateRepairMask(float disocclusion, float jitter, float ghosting, float velocity) {
    float mask = disocclusion * 0.4f + jitter * 0.3f + ghosting * 0.3f;
    mask = saturate(mask);
    
    float repairZone = step(0.2f, mask);
    float repairIntensity = saturate((mask - 0.2f) / 0.8f);
    
    return float3(repairZone, repairIntensity, velocity);
}

float CalculateFinalAlpha(float disocclusion, float jitter, float ghosting, float velocity,
                         float historyConfidence, float stabilityFrames) {
    float baseAlpha = g_MinAlpha;
    
    baseAlpha += velocity * 0.2f;
    baseAlpha += disocclusion * 0.3f;
    baseAlpha += jitter * 0.1f;
    baseAlpha += ghosting * 0.2f;
    
    float stabilityBoost = min(stabilityFrames / 60.0f, 0.3f);
    baseAlpha -= stabilityBoost;
    
    baseAlpha = clamp(baseAlpha, g_MinAlpha, g_MaxAlpha);
    
    baseAlpha *= (historyConfidence + 0.1f);
    
    return baseAlpha;
}

[numthreads(TSS_LDS_SIZE, TSS_LDS_SIZE, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
    uint2 pixelCoord = dispatchId.xy;
    
    if (any(pixelCoord >= g_Resolution)) return;
    
    float2 uv = (float2(pixelCoord) + 0.5f) * g_texelSize;
    
    float currentDepth = g_CurrentDepth[pixelCoord];
    float historyDepth = g_HistoryDepth[pixelCoord];
    float2 currentMV = g_CurrentMotionVector[pixelCoord] * g_MotionScale;
    float2 historyMV = g_HistoryMotionVector[pixelCoord] * g_MotionScale;
    
    float velocity = length(currentMV);
    
    float2 dilatedMV = GetDilatedMV(uv, currentMV);
    if (g_EnableDilatedMV) currentMV = dilatedMV;
    
    float3 currentColor = g_CurrentColor[pixelCoord].rgb;
    float4 historyColorData = g_HistoryColor[pixelCoord];
    float3 historyColor = historyColorData.rgb;
    float historyConfidence = g_HistoryConfidence[pixelCoord];
    
    float3 currentYCC = g_EnableYCoCg ? RGBToYCoCg(currentColor) : float3(currentColor, 0.0f, 0.0f);
    float3 historyYCC = g_EnableYCoCg ? RGBToYCoCg(historyColor) : float3(historyColor, 0.0f, 0.0f);
    
    float3 neighborhood[9];
    int idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            uint2 neighborCoord = pixelCoord + uint2(dx, dy);
            float3 neighborColor = g_CurrentColor[neighborCoord].rgb;
            neighborhood[idx] = g_EnableYCoCg ? RGBToYCoCg(neighborColor) : float3(neighborColor, 0.0f, 0.0f);
            idx++;
        }
    }
    
    float entropy = CalculateEntropy(neighborhood);
    
    float disocclusion = DetectDisocclusion(uv, currentDepth, currentMV, historyMV);
    float jitter = DetectJitter(uv, currentYCC, historyYCC, velocity);
    float ghosting = DetectGhosting(currentYCC, historyYCC, velocity);
    
    float3 repairMask = CalculateRepairMask(disocclusion, jitter, ghosting, velocity);
    
    float3 forwardColor, backwardColor;
    float forwardWeight, backwardWeight;
    
    if (g_EnableForwardProject) {
        CalculateBidirectionalProjection(uv, currentMV, currentDepth, 
                                        forwardColor, backwardColor,
                                        forwardWeight, backwardWeight);
        
        float3 bidirBlend = forwardColor * forwardWeight + backwardColor * backwardWeight;
        currentYCC = lerp(currentYCC, bidirBlend, 0.3f);
    }
    
    float mean = 0.0f, sumSq = 0.0f;
    for (int i = 0; i < 9; i++) {
        mean += neighborhood[i].x;
        sumSq += neighborhood[i].x * neighborhood[i].x;
    }
    mean /= 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    float stdDev = sqrt(max(variance, 0.0f));
    
    historyYCC.x = VarianceClipping(historyYCC.x, currentYCC.x, mean, stdDev, g_KSigmaY);
    historyYCC.y = VarianceClipping(historyYCC.y, currentYCC.y, 0.0f, 0.1f, g_KSigmaChroma);
    historyYCC.z = VarianceClipping(historyYCC.z, currentYCC.z, 0.0f, 0.1f, g_KSigmaChroma);
    
    if (g_EnableLumaRepair) {
        float3 lumaRepaired = LumaRepair(currentYCC, historyYCC, entropy, g_LumaThreshold);
        currentYCC = lerp(currentYCC, lumaRepaired, 0.2f);
    }
    
    if (g_EnableNeuralRepair) {
        float4 neuralResult = NeuralRepairLuma(currentYCC, historyYCC, neighborhood,
                                              disocclusion, jitter, ghosting, velocity,
                                              entropy, g_RepairStrength);
        historyYCC.x = neuralResult.x;
        historyYCC.y = lerp(historyYCC.y, neuralResult.y, g_RepairStrength);
        historyYCC.z = lerp(historyYCC.z, neuralResult.z, g_RepairStrength);
    }
    
    if (g_EnableJitterStabilization) {
        float stability = JitterStabilization(velocity, historyColorData.a * 60.0f);
        historyYCC = lerp(historyYCC, currentYCC, stability * 0.1f);
    }
    
    float3 edgeHistory = EdgePreservingBlend(uv, historyYCC);
    historyYCC = lerp(historyYCC, edgeHistory, 0.2f);
    
    float3 neighborsSharp[8];
    idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            neighborsSharp[idx++] = neighborhood[(dy + 1) * 3 + (dx + 1)];
        }
    }
    
    float sharpness = AdaptiveSharpness(currentYCC.x, neighborsSharp[4].x, 0.05f);
    float finalSharpness = sharpness * g_Sharpness * 0.3f;
    historyYCC = lerp(historyYCC, currentYCC, finalSharpness);
    
    float finalAlpha = CalculateFinalAlpha(disocclusion, jitter, ghosting, velocity,
                                         historyConfidence, historyColorData.a * 60.0f);
    
    float3 accumulatedYCC = lerp(historyYCC, currentYCC, finalAlpha);
    
    float3 finalRGB = g_EnableYCoCg ? YCoCgToRGB(accumulatedYCC) : accumulatedYCC;
    
    float newStability = disocclusion > 0.5f ? 0.0f : min(historyColorData.a + (1.0f / 60.0f), 1.0f);
    
    float outputConfidence = (1.0f - disocclusion) * (1.0f - jitter * 0.5f) * (1.0f - ghosting * 0.3f);
    outputConfidence = saturate(outputConfidence);
    
    g_OutputColor[pixelCoord] = float4(saturate(finalRGB), newStability);
    g_OutputDepth[pixelCoord] = currentDepth;
    g_OutputMotionVector[pixelCoord] = currentMV;
    g_OutputConfidence[pixelCoord] = outputConfidence;
    g_OutputRepairMask[pixelCoord] = repairMask.x;
}
