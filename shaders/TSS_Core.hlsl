#define TSS_CORE_VERSION "1.0.0"

#define TSS_PI 3.14159265f
#define TSS_LN2 0.69314718f
#define TSS_EPSILON 1e-6f
#define TSS_WAVE_SIZE 32
#define TSS_LDS_SIZE 16

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
TextureTexture2D<float> g_HistoryConfidence : register(t6);

cbuffer TSSCoreConstants : register(b0) {
    uint2 g_Resolution;
    uint2 g_InputResolution;
    float g_JitterX;
    float g_JitterY;
    float g_Sharpness;
    float g_KSigma;
    float g_MotionScale;
    float g_DisocclusionThreshold;
    float g_MinAlpha;
    float g_MaxAlpha;
    float g_RepairStrength;
    float g_JitterStabilization;
    float g_TimeDelta;
    float g_FrameIndex;
    float g_EnableYCoCg;
    float g_EnableNeuralRepair;
    float g_EnableAdaptiveClamp;
    float g_EnableDilatedMV;
    float2 g_texelSize;
    float2 g_inputTexelSize;
};

groupshared float4 gs_LDSColor[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float gs_LDSDepth[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float2 gs_LDSMotion[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float gs_LDSConfidence[TSS_LDS_SIZE * TSS_LDS_SIZE];
groupshared float gs_LDSRepair[TSS_LDS_SIZE * TSS_LDS_SIZE];

float3 rgb2y(float3 c) {
    float y = 0.25f * c.r + 0.5f * c.g + 0.25f * c.b;
    float co = 0.5f * c.r - 0.5f * c.b;
    float cg = -0.25f * c.r + 0.5f * c.g - 0.25f * c.b;
    return float3(y, co, cg);
}

float3 y2rgb(float3 ycc) {
    float tmp = ycc.x - ycc.z;
    float r = tmp + ycc.y;
    float g = ycc.x + ycc.z;
    float b = tmp - ycc.y;
    return float3(r, g, b);
}

float3 rgb2y_ultra(float3 c) {
    float tmp = 0.5f * (c.r + c.b);
    float y = 0.25f * c.r + 0.5f * c.g + 0.25f * c.b;
    float co = c.r - tmp;
    float cg = tmp - c.g + tmp - c.b;
    return float3(y, co, cg);
}

float Luminance(float3 c) {
    return dot(c, float3(0.2126f, 0.7152f, 0.0722f));
}

float3 DecodeNormal(float3 n) {
    return n * 2.0f - 1.0f;
}

float3 EncodeNormal(float3 n) {
    return n * 0.5f + 0.5f;
}

float lanczos_weight(float x, float a) {
    float pi_x = TSS_PI * x;
    if (abs(x) < TSS_EPSILON) return 1.0f;
    if (abs(x) >= a) return 0.0f;
    return (sin(pi_x) / pi_x) * (sin(pi_x / a) / (pi_x / a));
}

float catmull_rom_weight(float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    if (t < 1.0f) return 1.5f * t3 - 2.5f * t2 + 1.0f;
    if (t < 2.0f) return -0.5f * t3 + 2.5f * t2 - 4.0f * t + 2.0f;
    return 0.0f;
}

float2 forward_project(float2 uv, float2 mv, float dt) {
    return uv - mv * dt * 0.5f;
}

float2 backward_project(float2 uv, float2 mv) {
    return uv + mv;
}

float4 sample_history_bicubic(Texture2D<float4> tex, float2 uv) {
    float2 texel = uv * g_Resolution - 0.5f;
    float2 f = frac(texel);
    
    float2 w0 = ((-0.75f * f + 1.5f) * f - 1.125f) * f + 0.5625f;
    float2 w1 = ((-0.75f * f + 0.75f) * f + 0.75f) * f + 0.25f;
    float2 w2 = ((0.75f * f - 1.5f) * f + 0.75f) * f + 0.5625f;
    float2 w3 = ((0.75f * f - 0.75f) * f - 0.75f) * f + 0.25f;
    
    float2 w12 = 1.0f - w0 - w1 - w2 - w3;
    
    float2 start = floor(texel) - 1.0f;
    start = max(start, 0.0f);
    
    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float2 uv0 = (start + 0.5f) / g_Resolution;
    float2 uv1 = (start + 1.5f) / g_Resolution;
    float2 uv2 = (start + 2.5f) / g_Resolution;
    float2 uv3 = (start + 3.5f) / g_Resolution;
    
    for (int y = 0; y < 4; y++) {
        float wy = (y == 0) ? w0.y : (y == 1) ? w1.y : (y == 2) ? w2.y : w3.y;
        float2 uv_y = (y == 0) ? uv0 : (y == 1) ? uv1 : (y == 2) ? uv2 : uv3;
        
        result += tex.SampleLevel(g_LinearSampler, uv_y, 0.0f) * (w0.x * wy);
        result += tex.SampleLevel(g_LinearSampler, uv_y + float2(g_texelSize.x, 0.0f), 0.0f) * (w1.x * wy);
        result += tex.SampleLevel(g_LinearSampler, uv_y + float2(g_texelSize.x * 2.0f, 0.0f), 0.0f) * (w2.x * wy);
        result += tex.SampleLevel(g_LinearSampler, uv_y + float2(g_texelSize.x * 3.0f, 0.0f), 0.0f) * (w3.x * wy);
    }
    
    return result;
}

float4 sample_history_catmull(Texture2D<float4> tex, float2 uv) {
    float2 texel = uv / g_texelSize;
    float2 f = frac(texel);
    
    float2 p0 = (floor(texel) - 1.0f) * g_texelSize;
    float2 p1 = floor(texel) * g_texelSize;
    float2 p2 = (floor(texel) + 1.0f) * g_texelSize;
    float2 p3 = (floor(texel) + 2.0f) * g_texelSize;
    
    float4 c0 = tex.SampleLevel(g_LinearSampler, p0, 0.0f);
    float4 c1 = tex.SampleLevel(g_LinearSampler, p1, 0.0f);
    float4 c2 = tex.SampleLevel(g_LinearSampler, p2, 0.0f);
    float4 c3 = tex.SampleLevel(g_LinearSampler, p3, 0.0f);
    
    float4 result = c1;
    
    float wx = catmull_rom_weight(f.x);
    float wy = catmull_rom_weight(f.y);
    
    result = lerp(result, lerp(c1, c2, wx), wy);
    
    return result;
}

float2 get_dilated_mv(float2 uv, float2 mv, float depth) {
    if (!g_EnableDilatedMV) return mv;
    
    float minDepth = depth;
    float2 dilatedMV = mv;
    float weightSum = 0.0f;
    
    float2 offsets[8] = {
        float2(-1.0f, 0.0f), float2(1.0f, 0.0f),
        float2(0.0f, -1.0f), float2(0.0f, 1.0f),
        float2(-1.0f, -1.0f), float2(1.0f, -1.0f),
        float2(-1.0f, 1.0f), float2(1.0f, 1.0f)
    };
    
    for (int i = 0; i < 8; i++) {
        float2 sampleUV = uv + offsets[i] * g_texelSize * 2.0f;
        float sampleDepth = g_CurrentDepth.SampleLevel(g_PointSampler, sampleUV, 0.0f);
        
        if (sampleDepth < minDepth) {
            minDepth = sampleDepth;
            float2 neighborMV = g_CurrentMotionVector.SampleLevel(g_PointSampler, sampleUV, 0.0f) * g_MotionScale;
            
            float mvDiff = length(mv - neighborMV);
            float weight = 1.0f / (1.0f + mvDiff * 10.0f);
            
            dilatedMV = lerp(dilatedMV, neighborMV, weight);
            weightSum += weight;
        }
    }
    
    return dilatedMV;
}

float compute_depth_trust(float currDepth, float histDepth) {
    if (currDepth < TSS_EPSILON) return 0.0f;
    float depthDiff = abs(currDepth - histDepth) / currDepth;
    return 1.0f - saturate(depthDiff * 10.0f);
}

float compute_velocity_trust(float2 mv, float2 prevMV) {
    float mvLen = length(mv);
    float speed = mvLen * 50.0f;
    
    float lenTrust = 1.0f - saturate(mvLen / 0.1f);
    
    float2 mvDiff = mv - prevMV;
    float dirTrust = 1.0f - saturate(length(mvDiff) * 5.0f);
    
    return lenTrust * 0.6f + dirTrust * 0.4f;
}

float compute_luma_stability(float currY, float histY, float neighborhoodStd) {
    if (abs(histY) < TSS_EPSILON) return 0.0f;
    float lumaDiff = abs(currY - histY) / (histY + 0.01f);
    float stability = 1.0f - saturate(lumaDiff * 2.0f);
    stability *= 1.0f - saturate(neighborhoodStd * 0.5f);
    return saturate(stability);
}

float tss_nerve(float3 curY, float3 histY, float2 mv, float depth, float histDepth, float2 prevMV, float neighborhoodStd) {
    float diff = abs(curY.x - histY.x);
    float speed = length(mv) * 50.0f;
    float confidence = 1.0f - saturate(diff * speed);
    
    float depthTrust = compute_depth_trust(depth, histDepth);
    confidence *= depthTrust;
    
    float velocityTrust = compute_velocity_trust(mv, prevMV);
    confidence *= velocityTrust;
    
    float lumaStability = compute_luma_stability(curY.x, histY.x, neighborhoodStd);
    confidence = confidence * 0.7f + lumaStability * 0.3f;
    
    return saturate(confidence);
}

float3 tss_clamp(float3 cur, float3 hist, float3 box_min, float3 box_max) {
    if (!g_EnableAdaptiveClamp) return clamp(hist, box_min, box_max);
    
    float3 center = (box_min + box_max) * 0.5f;
    float3 range = (box_max - box_min) * 0.5f;
    
    float curLuma = Luminance(cur);
    float histLuma = Luminance(hist);
    float lumaRatio = curLuma / (histLuma + 0.01f);
    
    float highlightBoost = smoothstep(0.8f, 1.2f, lumaRatio) * 0.15f;
    
    return clamp(hist, box_min - range * 0.1f - highlightBoost, box_max + range * 0.1f + highlightBoost);
}

float variance_clipping(float historyVal, float currentVal, float mean, float stdDev, float kSigma) {
    float minB = mean - kSigma * stdDev;
    float maxB = mean + kSigma * stdDev;
    
    if (historyVal < minB || historyVal > maxB) {
        float clip = min((maxB - minB) / (abs(historyVal - mean) + TSS_EPSILON), 1.0f);
        return lerp(historyVal, currentVal, clip * 0.5f);
    }
    return historyVal;
}

float4 bidirectional_splat(float2 uv, float2 mv, float depth) {
    float2 fUV = forward_project(uv, mv, g_TimeDelta);
    float2 bUV = backward_project(uv, mv);
    
    fUV = clamp(fUV, 0.0f, 1.0f);
    bUV = clamp(bUV, 0.0f, 1.0f);
    
    float4 fSample = sample_history_bicubic(g_HistoryColor, fUV);
    float4 bSample = sample_history_bicubic(g_HistoryColor, bUV);
    
    float fDepth = g_HistoryDepth.SampleLevel(g_PointSampler, fUV, 0.0f);
    float bDepth = g_HistoryDepth.SampleLevel(g_PointSampler, bUV, 0.0f);
    
    float depthWF = 1.0f - saturate(abs(depth - fDepth) * 100.0f);
    float depthWB = 1.0f - saturate(abs(depth - bDepth) * 100.0f);
    
    float mvLen = length(mv);
    float mvWF = 1.0f - saturate(mvLen * 0.5f);
    float mvWB = 1.0f - saturate(mvLen * 0.3f);
    
    float wF = depthWF * mvWF;
    float wB = depthWB * mvWB;
    
    float4 result;
    if (wF + wB > 0.001f) {
        result = (fSample * wF + bSample * wB) / (wF + wB);
    } else {
        result = bSample;
    }
    
    float conf = saturate((wF + wB) * 0.5f);
    
    return float4(result.rgb, conf);
}

float compute_disocclusion(float currDepth, float histDepth, float2 mv, float2 prevMV) {
    if (currDepth < TSS_EPSILON) return 1.0f;
    
    float depthDiff = abs(currDepth - histDepth) / currDepth;
    float depthScore = depthDiff * 10.0f;
    
    float2 mvDiff = mv - prevMV;
    float mvDivergence = length(mvDiff) * 2.0f;
    
    float disocclusion = max(depthScore, mvDivergence);
    
    return saturate(disocclusion);
}

float compute_jitter(float3 curY, float3 histY, float velocity) {
    float lumaDiff = abs(curY.x - histY.x);
    float jitter = lumaDiff * 3.0f;
    jitter += step(0.1f, velocity) * 0.3f;
    return saturate(jitter);
}

float compute_ghosting(float3 curY, float3 histY, float velocity) {
    float yDiff = abs(curY.x - histY.x);
    float chromaDiff = (abs(curY.y - histY.y) + abs(curY.z - histY.z)) * 0.5f;
    
    float ghosting = yDiff * 2.0f + chromaDiff * 0.5f;
    ghosting += step(0.5f, velocity) * 0.2f;
    
    return saturate(ghosting);
}

float3 fill_disocclusion_holes(float2 uv, float3 curY, float disocclusion, float depth) {
    if (disocclusion < 0.3f) return curY;
    
    float3 colors[8];
    float depths[8];
    int valid = 0;
    
    float2 offsets[8] = {
        float2(-1.0f, 0.0f), float2(1.0f, 0.0f),
        float2(0.0f, -1.0f), float2(0.0f, 1.0f),
        float2(-1.0f, -1.0f), float2(1.0f, -1.0f),
        float2(-1.0f, 1.0f), float2(1.0f, 1.0f)
    };
    
    for (int i = 0; i < 8; i++) {
        float2 neighborUV = uv + offsets[i] * g_texelSize;
        if (any(neighborUV < 0.0f) || any(neighborUV > 1.0f)) continue;
        
        float neighborDepth = g_CurrentDepth.SampleLevel(g_PointSampler, neighborUV, 0.0f);
        if (abs(neighborDepth - depth) < 0.01f) {
            float3 neighborColor = g_CurrentColor.SampleLevel(g_PointSampler, neighborUV, 0.0f).rgb;
            colors[valid] = rgb2y(neighborColor);
            depths[valid] = neighborDepth;
            valid++;
        }
    }
    
    if (valid == 0) return curY;
    
    float3 sumColor = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    
    for (int i = 0; i < valid; i++) {
        float depthDiff = abs(depths[i] - depth);
        float weight = 1.0f / (1.0f + depthDiff * 10.0f);
        if (depthDiff < 0.01f) weight *= 2.0f;
        
        sumColor += colors[i] * weight;
        totalWeight += weight;
    }
    
    float3 filled = sumColor / max(totalWeight, 0.001f);
    
    float blend = saturate((disocclusion - 0.3f) / 0.7f);
    return lerp(curY, filled, blend);
}

float3 edge_preserving_blend(float2 uv, float3 curY, float3 neighbors[9]) {
    float weights[9];
    float totalWeight = 0.0f;
    
    for (int i = 0; i < 9; i++) {
        float centerLuma = neighbors[4].x;
        float neighborLuma = neighbors[i].x;
        float lumaDiff = abs(centerLuma - neighborLuma);
        weights[i] = 1.0f / (1.0f + lumaDiff * 10.0f);
        totalWeight += weights[i];
    }
    
    float3 edgeAware = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 9; i++) {
        edgeAware += neighbors[i] * (weights[i] / totalWeight);
    }
    
    return edgeAware;
}

float adaptive_sharpness(float centerLuma, float neighborLuma, float threshold) {
    float diff = abs(centerLuma - neighborLuma);
    if (diff < threshold) return 0.0f;
    return min(diff * 2.0f, 1.0f);
}

[numthreads(TSS_LDS_SIZE, TSS_LDS_SIZE, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID) {
    uint2 tid = dispatchId.xy;
    if (any(tid >= g_Resolution)) return;
    
    float2 uv = (float2(tid) + 0.5f) * g_texelSize;
    
    float depth = g_CurrentDepth[tid];
    float2 mv = g_CurrentMotionVector[tid] * g_MotionScale;
    float2 prevMV = g_HistoryMotionVector[tid] * g_MotionScale;
    
    float2 dilatedMV = get_dilated_mv(uv, mv, depth);
    if (g_EnableDilatedMV) mv = dilatedMV;
    
    float3 curRGB = g_CurrentColor[tid].rgb;
    float4 histColorData = g_HistoryColor[tid];
    float3 histRGB = histColorData.rgb;
    float histConfidence = g_HistoryConfidence[tid];
    float stabilityFrames = histColorData.a;
    
    float3 curY = g_EnableYCoCg ? rgb2y(curRGB) : float3(Luminance(curRGB), 0.0f, 0.0f);
    float3 histY = g_EnableYCoCg ? rgb2y(histRGB) : float3(Luminance(histRGB), 0.0f, 0.0f);
    
    float3 neighbors[9];
    int idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            uint2 neighborCoord = tid + uint2(dx, dy);
            float3 neighborColor = g_CurrentColor[neighborCoord].rgb;
            neighbors[idx++] = g_EnableYCoCg ? rgb2y(neighborColor) : float3(Luminance(neighborColor), 0.0f, 0.0f);
        }
    }
    
    float sum = 0.0f, sumSq = 0.0f;
    for (int i = 0; i < 9; i++) {
        sum += neighbors[i].x;
        sumSq += neighbors[i].x * neighbors[i].x;
    }
    float mean = sum / 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    float stdDev = sqrt(max(variance, 0.0f));
    
    float disocclusion = compute_disocclusion(depth, depth, mv, prevMV);
    float velocity = length(mv);
    float jitter = compute_jitter(curY, histY, velocity);
    float ghosting = compute_ghosting(curY, histY, velocity);
    
    if (disocclusion > 0.8f) {
        g_OutputColor[tid] = float4(saturate(curRGB), 0.0f);
        g_OutputConfidence[tid] = 0.0f;
        g_OutputRepairMask[tid] = 1.0f;
        return;
    }
    
    float3 box_min = neighbors[0];
    float3 box_max = neighbors[0];
    for (int i = 1; i < 9; i++) {
        box_min = min(box_min, neighbors[i]);
        box_max = max(box_max, neighbors[i]);
    }
    
    histY.x = variance_clipping(histY.x, curY.x, mean, stdDev, g_KSigma);
    histY.y = variance_clipping(histY.y, curY.y, 0.0f, 0.1f, g_KSigma * 2.0f);
    histY.z = variance_clipping(histY.z, curY.z, 0.0f, 0.1f, g_KSigma * 2.0f);
    
    float4 bidir = bidirectional_splat(uv, mv, depth);
    float3 bidirY = g_EnableYCoCg ? rgb2y(bidir.rgb) : float3(Luminance(bidir.rgb), 0.0f, 0.0f);
    curY = lerp(curY, bidirY, bidir.a * 0.3f);
    
    float3 filledY = fill_disocclusion_holes(uv, curY, disocclusion, depth);
    curY = lerp(curY, filledY, disocclusion * 0.5f);
    
    if (g_EnableNeuralRepair) {
        float nerve = tss_nerve(curY, histY, mv, depth, depth, prevMV, stdDev);
        float adaptiveAlpha = nerve * g_RepairStrength;
        curY = lerp(curY, histY, adaptiveAlpha);
    }
    
    float3 edgeY = edge_preserving_blend(uv, curY, neighbors);
    curY = lerp(curY, edgeY, 0.2f);
    
    float stability = 1.0f;
    if (velocity < 0.01f) {
        stability = min(1.0f, g_JitterStabilization * (stabilityFrames + 0.1f));
    }
    curY = lerp(curY, histY, stability * 0.1f);
    
    float neighborsSharp[8];
    idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            neighborsSharp[idx++] = neighbors[(dy + 1) * 3 + (dx + 1)].x;
        }
    }
    
    float sharpness = adaptive_sharpness(curY.x, neighborsSharp[4], 0.05f);
    float finalSharp = sharpness * g_Sharpness * 0.3f;
    curY = lerp(curY, histY, finalSharp);
    
    float baseAlpha = g_MinAlpha;
    baseAlpha += velocity * 0.2f;
    baseAlpha += disocclusion * 0.3f;
    baseAlpha += jitter * 0.1f;
    baseAlpha += ghosting * 0.2f;
    
    float stabilityBoost = min(stabilityFrames / 60.0f, 0.3f);
    baseAlpha -= stabilityBoost;
    
    baseAlpha = clamp(baseAlpha, g_MinAlpha, g_MaxAlpha);
    baseAlpha *= (histConfidence + 0.1f);
    
    float3 clampedHistY = tss_clamp(curY, histY, box_min, box_max);
    float3 finalY = lerp(clampedHistY, curY, baseAlpha);
    
    float3 finalRGB = g_EnableYCoCg ? y2rgb(finalY) : float3(finalY.x, finalY.x, finalY.x);
    
    float newStability = disocclusion > 0.5f ? 0.0f : min(stabilityFrames + (1.0f / 60.0f), 1.0f);
    
    float outputConf = (1.0f - disocclusion) * (1.0f - jitter * 0.5f) * (1.0f - ghosting * 0.3f);
    outputConf = saturate(outputConf);
    
    g_OutputColor[tid] = float4(saturate(finalRGB), newStability);
    g_OutputDepth[tid] = depth;
    g_OutputMotionVector[tid] = mv;
    g_OutputConfidence[tid] = outputConf;
    g_OutputRepairMask[tid] = saturate(disocclusion + jitter + ghosting);
}
