#define TSS_SMART_VERSION "1.0.0"

#define TSS_WAVE_SIZE 32
#define TSS_LDS_SIZE 16

RWTexture2D<float4> g_OutputColor : register(u0);
RWTexture2D<float2> g_OutputMotionVector : register(u1);
RWTexture2D<float> g_OutputConfidence : register(u2);

Texture2D<float4> g_CurrentColor : register(t0);
Texture2D<float> g_CurrentDepth : register(t1);
Texture2D<float2> g_CurrentMotionVector : register(t2);
Texture2D<float4> g_HistoryColor : register(t3);
Texture2D<float> g_HistoryDepth : register(t4);
Texture2D<float2> g_HistoryMotionVector : register(t5);
Texture2D<float> g_HistoryConfidence : register(t6);

cbuffer TSSSmartConstants : register(b0) {
    uint2 g_Resolution;
    uint2 g_HistoryResolution;
    float g_JitterX;
    float g_JitterY;
    float g_Sharpness;
    float g_KSigma;
    float g_MotionScale;
    float g_DisocclusionThreshold;
    float g_MaxVelocity;
    int g_FrameIndex;
    int g_EnableYCoCg;
    int g_EnableDepthTest;
    int g_EnableAsyncCompute;
    int g_EnableNegativeLOD;
    float g_DeltaTime;
    float g_Padding1;
    float g_Padding2;
    float g_Padding3;
};

groupshared float4 gs_LDSColor[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float gs_LDSDepth[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float2 gs_LDSMotionVector[TSS_LDS_SIZE * TSS_LDS_SIZE];

groupshared float4 gs_ConfidenceMasks[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float4 gs_NeuralWeights[TSS_LDS_SIZE * TSS_LDS_SIZE];

static const float2 g_Halton2[8] = {
    float2(0.5f, 0.333333f),
    float2(0.25f, 0.666667f),
    float2(0.75f, 0.111111f),
    float2(0.125f, 0.444444f),
    float2(0.625f, 0.777778f),
    float2(0.375f, 0.222222f),
    float2(0.875f, 0.555556f),
    float2(0.0625f, 0.888889f)
};

static const float2 g_R2Sequence[8] = {
    float2(0.5f, 0.366025f),
    float2(0.25f, 0.732051f),
    float2(0.75f, 0.048284f),
    float2(0.125f, 0.414339f),
    float2(0.625f, 0.780614f),
    float2(0.375f, 0.096847f),
    float2(0.875f, 0.462902f),
    float2(0.0625f, 0.829177f)
};

float3x3 g_NeuralLayer1Weights = float3x3(
    0.8f, 0.1f, 0.1f,
    0.1f, 0.8f, 0.1f,
    0.1f, 0.1f, 0.8f
);

float3 g_NeuralLayer1Bias = float3(-0.05f, -0.05f, -0.05f);

float3x3 g_NeuralLayer2Weights = float3x3(
    0.9f, 0.05f, 0.05f,
    0.05f, 0.9f, 0.05f,
    0.05f, 0.05f, 0.9f
);

float3 g_NeuralLayer2Bias = float3(-0.02f, -0.02f, -0.02f);

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

float CalculateDisocclusionMask(float currentDepth, float historyDepth, float2 mv, float threshold) {
    float depthDiff = abs(currentDepth - historyDepth);
    float mvMagnitude = length(mv);
    float mvPenalty = min(mvMagnitude * 0.1f, 0.3f);
    float disocclusion = depthDiff * 100.0f + mvPenalty;
    return step(threshold, disocclusion);
}

float CalculateVelocityDivergence(float2 mv, float2 neighborMVs[8]) {
    float sumDiff = 0.0f;
    
    [unroll]
    for (int i = 0; i < 8; i++) {
        float dx = mv.x - neighborMVs[i].x;
        float dy = mv.y - neighborMVs[i].y;
        sumDiff += sqrt(dx * dx + dy * dy);
    }
    
    float avgDiff = sumDiff / 8.0f;
    return min(avgDiff * 2.0f, 1.0f);
}

float CalculateLumaInstability(float currentLuma, float historyLuma, float neighborhoodStdDev) {
    float lumaDiff = abs(currentLuma - historyLuma) / (historyLuma + 0.0001f);
    float instability = lumaDiff * 2.0f;
    instability += min(neighborhoodStdDev * 0.5f, 0.5f);
    return min(instability, 1.0f);
}

float4 CalculateConfidenceMasks(float2 uv, float currentDepth, float historyDepth, float2 currentMV, float2 historyMV) {
    float mvMagnitude = length(currentMV);
    
    float neighborMVs[8];
    neighborMVs[0] = historyMV.x;
    neighborMVs[1] = historyMV.y;
    neighborMVs[2] = currentMV.x;
    neighborMVs[3] = currentMV.y;
    neighborMVs[4] = historyMV.x;
    neighborMVs[5] = historyMV.y;
    neighborMVs[6] = currentMV.x;
    neighborMVs[7] = currentMV.y;
    
    float2 nMVs[8];
    nMVs[0] = historyMV;
    nMVs[1] = historyMV;
    nMVs[2] = currentMV;
    nMVs[3] = currentMV;
    nMVs[4] = historyMV;
    nMVs[5] = currentMV;
    nMVs[6] = historyMV;
    nMVs[7] = currentMV;
    
    float disocclusionMask = CalculateDisocclusionMask(currentDepth, historyDepth, currentMV, g_DisocclusionThreshold);
    float velocityDivergence = CalculateVelocityDivergence(currentMV, nMVs);
    
    float2 texelSize = 1.0f / float2(g_Resolution);
    float3 currentColor = g_CurrentColor.SampleLevel(g_CurrentColor, uv, g_EnableNegativeLOD ? -1.0f : 0.0f).rgb;
    float3 historyColor = g_HistoryColor.SampleLevel(g_HistoryColor, uv, g_EnableNegativeLOD ? -1.0f : 0.0f).rgb;
    float currentLuma = CalculateLuma(currentColor);
    float historyLuma = CalculateLuma(historyColor);
    
    float sum = 0.0f, sumSq = 0.0f;
    [unroll]
    for (int dy = -1; dy <= 1; dy++) {
        [unroll]
        for (int dx = -1; dx <= 1; dx++) {
            float3 neighborColor = g_CurrentColor.SampleLevel(g_CurrentColor, uv + float2(dx, dy) * texelSize, 0.0f).rgb;
            float neighborLuma = CalculateLuma(neighborColor);
            sum += neighborLuma;
            sumSq += neighborLuma * neighborLuma;
        }
    }
    float mean = sum / 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    float stdDev = sqrt(max(variance, 0.0f));
    
    float lumaInstability = CalculateLumaInstability(currentLuma, historyLuma, stdDev);
    
    float edgeConfidence = 1.0f - velocityDivergence - lumaInstability * 0.5f;
    
    float totalConfidence = edgeConfidence * 0.5f + (1.0f - disocclusionMask) * 0.3f + (1.0f - velocityDivergence) * 0.2f;
    totalConfidence = saturate(totalConfidence);
    
    return float4(disocclusionMask, velocityDivergence, lumaInstability, totalConfidence);
}

float3 NeuralPolisher(float3 mathColor, float4 masks, float2 uv) {
    float2 texelSize = 1.0f / float2(g_Resolution);
    float3 neighborhood[9];
    int idx = 0;
    [unroll]
    for (int dy = -1; dy <= 1; dy++) {
        [unroll]
        for (int dx = -1; dx <= 1; dx++) {
            neighborhood[idx++] = g_CurrentColor.SampleLevel(g_CurrentColor, uv + float2(dx, dy) * texelSize, 0.0f).rgb;
        }
    }
    
    float3 featureData = float3(masks.w, masks.x, masks.y);
    
    float3 x = mathColor * featureData;
    
    float3 hidden1 = saturate(mul(x, g_NeuralLayer1Weights) + g_NeuralLayer1Bias);
    
    float3 hidden2 = saturate(mul(hidden1, g_NeuralLayer2Weights) + g_NeuralLayer2Bias);
    
    return hidden2;
}

float NeuralWeightArbitrator(float4 masks, float mvMagnitude, int stabilityFrames) {
    float weight = 0.1f;
    
    if (masks.x > g_DisocclusionThreshold) {
        weight = 1.0f;
    } else {
        float velocityWeight = min(mvMagnitude * g_MotionScale, 0.4f);
        float divergenceWeight = masks.y * 0.3f;
        float instabilityWeight = masks.z * 0.2f;
        float stabilityBoost = min((float)stabilityFrames / 60.0f, 0.3f);
        
        weight = 0.05f + velocityWeight + divergenceWeight + instabilityWeight - stabilityBoost;
        weight = max(0.05f, min(0.5f, weight));
    }
    
    return weight;
}

float VarianceClipping(float historyValue, float currentValue, float mean, float stdDev, float kSigma) {
    float minBound = mean - kSigma * stdDev;
    float maxBound = mean + kSigma * stdDev;
    
    if (historyValue < minBound || historyValue > maxBound) {
        float clip = min((maxBound - minBound) / (abs(historyValue - mean) + 0.0001f), 1.0f);
        return historyValue * (1.0f - clip * 0.5f) + currentValue * (clip * 0.5f);
    }
    
    return historyValue;
}

void YCoCgClamping(inout float3 historyYCC, float3 neighborsYCC[9], float kY, float kCo, float kCg) {
    float yMin = historyYCC.x, yMax = historyYCC.x;
    float coMin = historyYCC.y, coMax = historyYCC.y;
    float cgMin = historyYCC.z, cgMax = historyYCC.z;
    
    [unroll]
    for (int i = 0; i < 9; i++) {
        yMin = min(yMin, neighborsYCC[i].x);
        yMax = max(yMax, neighborsYCC[i].x);
        coMin = min(coMin, neighborsYCC[i].y);
        coMax = max(coMax, neighborsYCC[i].y);
        cgMin = min(cgMin, neighborsYCC[i].z);
        cgMax = max(cgMax, neighborsYCC[i].z);
    }
    
    float yRange = (yMax - yMin) * kY;
    float coRange = (coMax - coMin) * kCo;
    float cgRange = (cgMax - cgMin) * kCg;
    
    historyYCC.x = clamp(historyYCC.x, yMin - yRange, yMax + yRange);
    historyYCC.y = clamp(historyYCC.y, coMin - coRange, coMax + coRange);
    historyYCC.z = clamp(historyYCC.z, cgMin - cgRange, cgMax + cgRange);
}

float2 BilinearWeight(float2 uv) {
    return uv - floor(uv);
}

float3 BilinearSample(Texture2D<float4> tex, float2 uv) {
    return tex.SampleLevel(tex, uv, 0.0f).rgb;
}

float lanczos3(float x) {
    float pi_x = 3.14159265f * x;
    float a = 3.0f;
    if (x == 0.0f) return 1.0f;
    if (abs(x) >= a) return 0.0f;
    float lanczos = (sin(pi_x) / (pi_x)) * (sin(pi_x / a) / (pi_x / a));
    return lanczos;
}

float3 LanczosSample(Texture2D<float4> tex, float2 uv, int taps) {
    float2 texelSize = 1.0f / float2(g_Resolution);
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    
    int halfTaps = taps / 2;
    [unroll]
    for (int dy = -halfTaps; dy <= halfTaps; dy++) {
        [unroll]
        for (int dx = -halfTaps; dx <= halfTaps; dx++) {
            float2 offset = float2(dx, dy);
            float weight = lanczos3(length(offset));
            float2 sampleUV = uv + offset * texelSize;
            float3 color = tex.SampleLevel(tex, sampleUV, 0.0f).rgb;
            sum += color * weight;
            weightSum += weight;
        }
    }
    
    return sum / weightSum;
}

float3 CatmullRomSample(Texture2D<float4> tex, float2 uv) {
    float2 texelSize = 1.0f / float2(g_Resolution);
    float2 p = uv / texelSize;
    float2 f = frac(p);
    
    float2 p0 = (floor(p) - 1.0f) * texelSize;
    float2 p1 = floor(p) * texelSize;
    float2 p2 = (floor(p) + 1.0f) * texelSize;
    float2 p3 = (floor(p) + 2.0f) * texelSize;
    
    float3 c0 = tex.SampleLevel(tex, p0, 0.0f).rgb;
    float3 c1 = tex.SampleLevel(tex, p1, 0.0f).rgb;
    float3 c2 = tex.SampleLevel(tex, p2, 0.0f).rgb;
    float3 c3 = tex.SampleLevel(tex, p3, 0.0f).rgb;
    
    float3 a0 = -0.5f * c0 + 1.5f * c1 - 1.5f * c2 + 0.5f * c3;
    float3 a1 = c0 - 2.5f * c1 + 2.0f * c2 - 0.5f * c3;
    float3 a2 = -0.5f * c0 + 0.5f * c2;
    float3 a3 = c1;
    
    float fx = f.x;
    float fx2 = fx * fx;
    float fx3 = fx2 * fx;
    
    float3 row0 = a0 * fx3 + a1 * fx2 + a2 * fx + a3;
    
    float3 d0 = -0.5f * c0.yxz + 1.5f * c1.yxz - 1.5f * c2.yxz + 0.5f * c3.yxz;
    float3 d1 = c0.yxz - 2.5f * c1.yxz + 2.0f * c2.yxz - 0.5f * c3.yxz;
    float3 d2 = -0.5f * c0.yxz + 0.5f * c2.yxz;
    float3 d3 = c1.yxz;
    
    float3 a0y = -0.5f * c0.yyz + 1.5f * c1.yyz - 1.5f * c2.yyz + 0.5f * c3.yyz;
    float3 a1y = c0.yyz - 2.5f * c1.yyz + 2.0f * c2.yyz - 0.5f * c3.yyz;
    float3 a2y = -0.5f * c0.yyz + 0.5f * c2.yyz;
    float3 a3y = c1.yyz;
    
    float fy = f.y;
    float fy2 = fy * fy;
    float fy3 = fy2 * fy;
    
    float3 col0 = a0y * fy3 + a1y * fy2 + a2y * fy + a3y;
    
    return row0;
}

float EdgeAwareFilter(float3 colors[9], float4 masks, int filterType) {
    float3 result = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    
    if (filterType == 0) {
        [unroll]
        for (int i = 0; i < 9; i++) {
            float weight = 1.0f;
            if (masks.y > 0.5f) {
                weight *= 0.5f;
            }
            result += colors[i] * weight;
            weightSum += weight;
        }
    } else if (filterType == 1) {
        float3 center = colors[4];
        [unroll]
        for (int i = 0; i < 9; i++) {
            if (i == 4) continue;
            float lumDiff = abs(CalculateLuma(colors[i]) - CalculateLuma(center));
            float weight = 1.0f / (1.0f + lumDiff * 10.0f);
            if (masks.x > 0.5f) {
                weight *= 0.3f;
            }
            result += colors[i] * weight;
            weightSum += weight;
        }
        result += center * 2.0f;
        weightSum += 2.0f;
    } else {
        [unroll]
        for (int i = 0; i < 9; i++) {
            float weight = 1.0f;
            if (abs(i - 4) <= 1) weight *= 2.0f;
            if (abs(i - 4) == 3) weight *= 1.5f;
            if (masks.x > 0.5f) weight *= 0.5f;
            result += colors[i] * weight;
            weightSum += weight;
        }
    }
    
    return weightSum > 0.0f ? weightSum : 1.0f;
}

void SmartAccumulate(uint2 globalId) {
    float2 uv = (float2(globalId) + 0.5f) / float2(g_Resolution);
    float2 texelSize = 1.0f / float2(g_Resolution);
    
    float currentDepth = g_CurrentDepth.SampleLevel(g_CurrentDepth, uv, 0.0f);
    float historyDepth = g_HistoryDepth.SampleLevel(g_HistoryDepth, uv, 0.0f);
    float2 currentMV = g_CurrentMotionVector.SampleLevel(g_CurrentMotionVector, uv, 0.0f);
    float2 historyMV = g_HistoryMotionVector.SampleLevel(g_HistoryMotionVector, uv, 0.0f);
    
    float3 currentColor = g_CurrentColor.SampleLevel(g_CurrentColor, uv, g_EnableNegativeLOD ? -1.0f : 0.0f).rgb;
    float4 historyColorData = g_HistoryColor.SampleLevel(g_HistoryColor, uv, 0.0f);
    float3 historyColor = historyColorData.rgb;
    
    float historyConfidence = g_HistoryConfidence.SampleLevel(g_HistoryConfidence, uv, 0.0f);
    
    float4 masks = CalculateConfidenceMasks(uv, currentDepth, historyDepth, currentMV, historyMV);
    
    float mvMagnitude = length(currentMV);
    
    float historyStability = historyColorData.a;
    int stabilityFrames = (int)(historyStability * 60.0f);
    
    float neuralWeight = NeuralWeightArbitrator(masks, mvMagnitude, stabilityFrames);
    
    float3 currentYCC = g_EnableYCoCg ? RGBToYCoCg(currentColor) : float3(currentColor, 0.0f);
    float3 historyYCC = g_EnableYCoCg ? RGBToYCoCg(historyColor) : float3(historyColor, 0.0f);
    
    float3 neighborsYCC[9];
    int idx = 0;
    [unroll]
    for (int dy = -1; dy <= 1; dy++) {
        [unroll]
        for (int dx = -1; dx <= 1; dx++) {
            float3 neighborColor = g_CurrentColor.SampleLevel(g_CurrentColor, uv + float2(dx, dy) * texelSize, 0.0f).rgb;
            neighborsYCC[idx++] = g_EnableYCoCg ? RGBToYCoCg(neighborColor) : float3(neighborColor, 0.0f);
        }
    }
    
    float sum = 0.0f, sumSq = 0.0f;
    [unroll]
    for (int i = 0; i < 9; i++) {
        sum += neighborsYCC[i].x;
        sumSq += neighborsYCC[i].x * neighborsYCC[i].x;
    }
    float mean = sum / 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    float stdDev = sqrt(max(variance, 0.0f));
    
    historyYCC.x = VarianceClipping(historyYCC.x, currentYCC.x, mean, stdDev, g_KSigma);
    historyYCC.y = VarianceClipping(historyYCC.y, currentYCC.y, 0.0f, 0.1f, g_KSigma * 2.0f);
    historyYCC.z = VarianceClipping(historyYCC.z, currentYCC.z, 0.0f, 0.1f, g_KSigma * 2.0f);
    
    if (g_EnableYCoCg) {
        YCoCgClamping(historyYCC, neighborsYCC, g_KSigma, g_KSigma * 2.0f, g_KSigma * 2.0f);
    }
    
    if (g_EnableDepthTest && abs(currentDepth - historyDepth) > 0.01f) {
        masks.w = 0.0f;
        neuralWeight = 1.0f;
        historyYCC = currentYCC;
    }
    
    float3 mathColor = historyYCC;
    
    float3 neuralCorrection = NeuralPolisher(mathColor, masks, uv);
    
    float3 polishedColor = mathColor + neuralCorrection * g_Sharpness * masks.w;
    
    float finalAlpha = neuralWeight * masks.w;
    float3 accumulatedColor = lerp(polishedColor, currentYCC, finalAlpha);
    
    float3 finalRGB = g_EnableYCoCg ? YCoCgToRGB(accumulatedColor) : accumulatedColor;
    
    float outputConfidence = masks.w * (1.0f - masks.x) * (1.0f - masks.y * 0.5f);
    float outputStability = masks.x > 0.5f ? 0.0f : min((float)stabilityFrames + 1, 60.0f) / 60.0f;
    
    g_OutputColor[globalId] = float4(saturate(finalRGB), outputStability);
    g_OutputMotionVector[globalId] = currentMV;
    g_OutputConfidence[globalId] = outputConfidence;
}

[numthreads(TSS_LDS_SIZE, TSS_LDS_SIZE, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
    uint threadIndex = groupThreadId.x + groupThreadId.y * TSS_LDS_SIZE;
    
    uint2 pixelCoord = dispatchId.xy;
    
    if (any(pixelCoord >= g_Resolution)) return;
    
    SmartAccumulate(pixelCoord);
}
