#define TSS_REPAIR_VERSION "1.0.0"

RWTexture2D<float4> g_OutputColor : register(u0);
RWTexture2D<float> g_OutputConfidence : register(u1);

Texture2D<float4> g_CurrentColor : register(t0);
Texture2D<float> g_CurrentDepth : register(t1);
Texture2D<float2> g_CurrentMotionVector : register(t2);
Texture2D<float4> g_HistoryColor : register(t3);
Texture2D<float> g_HistoryDepth : register(t4);
Texture2D<float2> g_HistoryMotionVector : register(t5);
Texture2D<float> g_HistoryConfidence : register(t6);
Texture2D<float> g_LumaBuffer : register(t7);

cbuffer TSSRepairConstants : register(b0) {
    uint2 g_Resolution;
    uint2 g_HistoryResolution;
    float g_JitterX;
    float g_JitterY;
    float g_Sharpness;
    float g_KSigma;
    float g_MotionScale;
    float g_DisocclusionThreshold;
    float g_ConfidenceThreshold;
    float g_RepairStrength;
    float g_JitterStabilization;
    int g_FrameIndex;
    int g_EnableYCoCg;
    int g_EnableDepthTest;
    int g_EnableLumaRepair;
    int g_EnableAsyncCompute;
    float g_DeltaTime;
    float2 g_texelSize;
};

groupshared float4 gs_LDSColor[16 * 16];
groupshared float gs_LDSDepth[16 * 16];
groupshared float2 gs_LDSMotion[16 * 16];
groupshared float gs_LDSConfidence[16 * 16];

static const float3x3 g_NeuralRepairWeights1 = float3x3(
    0.1f, 0.2f, 0.1f,
    0.2f, 0.4f, 0.2f,
    0.1f, 0.2f, 0.1f
);

static const float3x3 g_NeuralRepairWeights2 = float3x3(
    -0.05f, -0.1f, -0.05f,
    -0.1f,  0.6f, -0.1f,
    -0.05f, -0.1f, -0.05f
);

static const float3 g_NeuralBias = float3(-0.02f, -0.02f, -0.02f);

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

float CalculateLocalEntropy(float3 colors[9]) {
    float lumas[9];
    float sum = 0.0f, sumSq = 0.0f;
    
    [unroll]
    for (int i = 0; i < 9; i++) {
        lumas[i] = CalculateLuma(colors[i]);
        sum += lumas[i];
        sumSq += lumas[i] * lumas[i];
    }
    
    float mean = sum / 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    
    return sqrt(max(variance, 0.0f));
}

float DetectDisocclusion(float currentDepth, float historyDepth, float2 mv, float threshold) {
    float depthDiff = abs(currentDepth - historyDepth);
    float mvMagnitude = length(mv);
    
    float disocclusionScore = depthDiff * 100.0f;
    
    disocclusionScore += min(mvMagnitude * 0.05f, 0.3f);
    
    float2 dilatedMV = float2(0.0f, 0.0f);
    float sampleCount = 0.0f;
    
    [unroll]
    for (int dy = -1; dy <= 1; dy++) {
        [unroll]
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            float2 neighborMV = g_CurrentMotionVector.SampleLevel(g_CurrentMotionVector, 
                (float2(dx, dy) * g_texelSize), 0.0f);
            dilatedMV += neighborMV;
            sampleCount += 1.0f;
        }
    }
    
    if (sampleCount > 0.0f) {
        dilatedMV /= sampleCount;
        float mvDivergence = length(mv - dilatedMV);
        disocclusionScore += mvDivergence * 0.2f;
    }
    
    return step(threshold, disocclusionScore);
}

float DetectJitterArtifact(float3 currentColor, float3 historyColor, float velocity) {
    float3 colorDiff = abs(currentColor - historyColor);
    float lumaDiff = colorDiff.y;
    
    float velocityPenalty = step(0.1f, velocity) * 0.5f;
    
    float jitterScore = lumaDiff * 3.0f + velocityPenalty;
    
    float3 gradX = float3(0.0f);
    float3 gradY = float3(0.0f);
    
    [unroll]
    for (int i = 0; i < 3; i++) {
        gradX += abs(currentColor[i] - float3(
            g_CurrentColor.SampleLevel(g_CurrentColor, g_texelSize * float2(1, 0), 0.0f)[i],
            g_CurrentColor.SampleLevel(g_CurrentColor, g_texelSize * float2(1, 0), 0.0f)[i],
            g_CurrentColor.SampleLevel(g_CurrentColor, g_texelSize * float2(1, 0), 0.0f)[i]
        ));
        gradY += abs(currentColor[i] - float3(
            g_CurrentColor.SampleLevel(g_CurrentColor, g_texelSize * float2(0, 1), 0.0f)[i],
            g_CurrentColor.SampleLevel(g_CurrentColor, g_texelSize * float2(0, 1), 0.0f)[i],
            g_CurrentColor.SampleLevel(g_CurrentColor, g_texelSize * float2(0, 1), 0.0f)[i]
        ));
    }
    
    float edgeStrength = (gradX.x + gradX.y + gradX.z + gradY.x + gradY.y + gradY.z) / 6.0f;
    
    jitterScore *= (1.0f + edgeStrength * 0.5f);
    
    return saturate(jitterScore);
}

float DetectGhosting(float3 currentYCC, float3 historyYCC, float velocity) {
    float3 diff = abs(currentYCC - historyYCC);
    
    float yDiff = diff.x;
    float chromaDiff = (abs(diff.y) + abs(diff.z)) * 0.5f;
    
    float ghostingScore = yDiff * 2.0f + chromaDiff * 0.5f;
    
    float velocityBoost = step(0.5f, velocity) * 0.3f;
    ghostingScore += velocityBoost;
    
    return saturate(ghostingScore);
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

float3 NeuralRepair(float3 currentYCC, float3 historyYCC, float3 neighborhood[9], 
                    float disocclusion, float jitter, float ghosting, float repairStrength) {
    float3 featureVector = float3(disocclusion, jitter, ghosting);
    
    float3 x = currentYCC * featureVector;
    
    float3 hidden1 = saturate(mul(x, g_NeuralRepairWeights1) + g_NeuralBias);
    
    float3 hidden2 = saturate(mul(hidden1, g_NeuralRepairWeights2) + g_NeuralBias);
    
    float repairFactor = length(hidden2) * repairStrength;
    
    return lerp(historyYCC, currentYCC, repairFactor);
}

float3 LumaRepair(float3 current, float3 history, float entropy, float threshold) {
    float currentLuma = CalculateLuma(current);
    float historyLuma = CalculateLuma(history);
    
    float lumaDiff = abs(currentLuma - historyLuma);
    
    if (lumaDiff > threshold * (1.0f + entropy)) {
        return current;
    }
    
    return lerp(history, current, 0.3f);
}

float3 EdgePreservingBlend(float3 current, float3 history, float2 uv) {
    float3 colors[9];
    float weights[9];
    float totalWeight = 0.0f;
    
    int idx = 0;
    [unroll]
    for (int dy = -1; dy <= 1; dy++) {
        [unroll]
        for (int dx = -1; dx <= 1; dx++) {
            colors[idx] = g_CurrentColor.SampleLevel(g_CurrentColor, uv + float2(dx, dy) * g_texelSize, 0.0f).rgb;
            float centerLuma = CalculateLuma(colors[4]);
            float neighborLuma = CalculateLuma(colors[idx]);
            float lumaDiff = abs(centerLuma - neighborLuma);
            weights[idx] = 1.0f / (1.0f + lumaDiff * 10.0f);
            totalWeight += weights[idx];
            idx++;
        }
    }
    
    float3 edgeAwareHistory = float3(0.0f, 0.0f, 0.0f);
    [unroll]
    for (int i = 0; i < 9; i++) {
        edgeAwareHistory += colors[i] * (weights[i] / totalWeight);
    }
    
    return edgeAwareHistory;
}

float3 JitterStabilization(float3 current, float3 history, float velocity, float stabilizationFactor) {
    if (velocity < 0.01f) {
        float stabilization = min(1.0f, stabilizationFactor);
        return lerp(history, current, stabilization * 0.1f);
    }
    return current;
}

float AdaptiveSharpness(float3 center, float3 neighbors[8], float edgeThreshold) {
    float centerLuma = CalculateLuma(center);
    float sumDiff = 0.0f;
    
    [unroll]
    for (int i = 0; i < 8; i++) {
        float neighborLuma = CalculateLuma(neighbors[i]);
        sumDiff += abs(centerLuma - neighborLuma);
    }
    
    float avgDiff = sumDiff / 8.0f;
    
    if (avgDiff < edgeThreshold) {
        return 0.0f;
    }
    
    return min(avgDiff * 2.0f, 1.0f);
}

void RepairKernel(uint2 pixelCoord) {
    float2 uv = (float2(pixelCoord) + 0.5f) * g_texelSize;
    
    float currentDepth = g_CurrentDepth.SampleLevel(g_CurrentDepth, uv, 0.0f);
    float historyDepth = g_HistoryDepth.SampleLevel(g_HistoryDepth, uv, 0.0f);
    float2 currentMV = g_CurrentMotionVector.SampleLevel(g_CurrentMotionVector, uv, 0.0f) * g_MotionScale;
    float2 historyMV = g_HistoryMotionVector.SampleLevel(g_HistoryMotionVector, uv, 0.0f) * g_MotionScale;
    
    float3 currentColor = g_CurrentColor.SampleLevel(g_CurrentColor, uv, 0.0f).rgb;
    float4 historyColorData = g_HistoryColor.SampleLevel(g_HistoryColor, uv, 0.0f);
    float3 historyColor = historyColorData.rgb;
    float historyConfidence = g_HistoryConfidence.SampleLevel(g_HistoryConfidence, uv, 0.0f);
    
    float velocity = length(currentMV);
    
    float disocclusion = DetectDisocclusion(currentDepth, historyDepth, currentMV, g_DisocclusionThreshold);
    
    float3 currentYCC = g_EnableYCoCg ? RGBToYCoCg(currentColor) : float3(currentColor, 0.0f, 0.0f);
    float3 historyYCC = g_EnableYCoCg ? RGBToYCoCg(historyColor) : float3(historyColor, 0.0f, 0.0f);
    
    float3 neighborhood[9];
    int idx = 0;
    [unroll]
    for (int dy = -1; dy <= 1; dy++) {
        [unroll]
        for (int dx = -1; dx <= 1; dx++) {
            float2 neighborUV = uv + float2(dx, dy) * g_texelSize;
            neighborhood[idx] = g_CurrentColor.SampleLevel(g_CurrentColor, neighborUV, 0.0f).rgb;
            if (g_EnableYCoCg) {
                neighborhood[idx] = RGBToYCoCg(neighborhood[idx]);
            }
            idx++;
        }
    }
    
    float entropy = CalculateLocalEntropy(neighborhood);
    
    float jitter = DetectJitterArtifact(currentColor, historyColor, velocity);
    
    float ghosting = DetectGhosting(currentYCC, historyYCC, velocity);
    
    if (disocclusion > 0.8f) {
        g_OutputColor[pixelCoord] = float4(saturate(YCoCgToRGB(currentYCC)), 0.0f);
        g_OutputConfidence[pixelCoord] = 0.0f;
        return;
    }
    
    float mean = 0.0f, sumSq = 0.0f;
    [unroll]
    for (int i = 0; i < 9; i++) {
        mean += neighborhood[i].x;
        sumSq += neighborhood[i].x * neighborhood[i].x;
    }
    mean /= 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    float stdDev = sqrt(max(variance, 0.0f));
    
    historyYCC.x = VarianceClipping(historyYCC.x, currentYCC.x, mean, stdDev, g_KSigma);
    historyYCC.y = VarianceClipping(historyYCC.y, currentYCC.y, 0.0f, 0.1f, g_KSigma * 2.0f);
    historyYCC.z = VarianceClipping(historyYCC.z, currentYCC.z, 0.0f, 0.1f, g_KSigma * 2.0f);
    
    if (g_EnableLumaRepair) {
        float lumaThreshold = 0.1f + entropy * 0.5f;
        historyYCC.x = LumaRepair(float3(currentYCC.x, 0.0f, 0.0f), 
                                   float3(historyYCC.x, 0.0f, 0.0f), 
                                   entropy, lumaThreshold).x;
    }
    
    float repairedYCC = NeuralRepair(currentYCC, historyYCC, neighborhood, 
                                     disocclusion, jitter, ghosting, g_RepairStrength);
    
    historyYCC = lerp(historyYCC, float3(repairedYCC, currentYCC.yz), g_RepairStrength);
    
    if (g_JitterStabilization > 0.0f) {
        float3 stabilized = JitterStabilization(currentYCC, historyYCC, velocity, g_JitterStabilization);
        historyYCC = stabilized;
    }
    
    float edgeAwareBlend = 0.5f;
    float3 edgeHistory = EdgePreservingBlend(historyYCC, float3(0.0f), uv);
    if (g_EnableYCoCg) {
        edgeHistory = RGBToYCoCg(edgeHistory);
    }
    historyYCC = lerp(historyYCC, edgeHistory, edgeAwareBlend * 0.3f);
    
    float3 neighborsForSharp[8];
    idx = 0;
    [unroll]
    for (int dy = -1; dy <= 1; dy++) {
        [unroll]
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            neighborsForSharp[idx++] = neighborhood[(dy + 1) * 3 + (dx + 1)];
        }
    }
    
    float sharpness = AdaptiveSharpness(currentYCC, neighborsForSharp, 0.05f);
    float finalSharpness = sharpness * g_Sharpness * 0.5f;
    historyYCC = lerp(historyYCC, currentYCC, finalSharpness);
    
    float finalAlpha = 0.1f;
    finalAlpha += velocity * 0.2f;
    finalAlpha += disocclusion * 0.3f;
    finalAlpha += jitter * 0.1f;
    finalAlpha += ghosting * 0.2f;
    finalAlpha = clamp(finalAlpha, 0.05f, 0.5f);
    
    finalAlpha *= historyConfidence + 0.1f;
    
    float3 accumulatedYCC = lerp(historyYCC, currentYCC, finalAlpha);
    
    float3 finalRGB = g_EnableYCoCg ? YCoCgToRGB(accumulatedYCC) : accumulatedYCC;
    
    float stabilityFrames = historyColorData.a;
    float newStability = disocclusion > 0.5f ? 0.0f : min(stabilityFrames + (1.0f / 60.0f), 1.0f);
    
    float outputConfidence = (1.0f - disocclusion) * (1.0f - jitter * 0.5f) * (1.0f - ghosting * 0.3f);
    outputConfidence = saturate(outputConfidence);
    
    g_OutputColor[pixelCoord] = float4(saturate(finalRGB), newStability);
    g_OutputConfidence[pixelCoord] = outputConfidence;
}

[numthreads(16, 16, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
    uint2 pixelCoord = dispatchId.xy;
    
    if (any(pixelCoord >= g_Resolution)) return;
    
    RepairKernel(pixelCoord);
}
