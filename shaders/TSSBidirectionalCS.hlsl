#define TSS_BIDIRECTIONAL_VERSION "1.0.0"

#define TSS_LDS_SIZE 16
#define TSS_MAX_SPLATS 64

RWTexture2D<float4> g_OutputColor : register(u0);
RWTexture2D<float> g_OutputDepth : register(u1);
RWTexture2D<float2> g_OutputMotionVector : register(u2);
RWTexture2D<float> g_OutputConfidence : register(u3);

Texture2D<float4> g_CurrentColor : register(t0);
Texture2D<float> g_CurrentDepth : register(t1);
Texture2D<float2> g_CurrentMotionVector : register(t2);
Texture2D<float4> g_HistoryColor : register(t3);
Texture2D<float> g_HistoryDepth : register(t4);
Texture2D<float2> g_HistoryMotionVector : register(t5);
Texture2D<float> g_HistoryConfidence : register(t6);

cbuffer TSSBidirectionalConstants : register(b0) {
    uint2 g_Resolution;
    uint2 g_HistoryResolution;
    float g_JitterX;
    float g_JitterY;
    float g_SplatRadius;
    float g_ConfidenceScale;
    float g_DepthWeight;
    float g_MotionScale;
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

groupshared float4 gs_SplatData[TSS_MAX_SPLATS];
groupshared float gs_SplatDepths[TSS_MAX_SPLATS];
groupshared float2 gs_SplatUVs[TSS_MAX_SPLATS];

groupshared float4 gs_AccumulatedColor;
groupshared float gs_AccumulatedWeight;
groupshared float gs_AccumulatedDepth;
groupshared uint gs_SplatCount;

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

float LanczosWeight(float x, float a) {
    float pi_x = 3.14159265f * x;
    if (abs(x) < 0.0001f) return 1.0f;
    if (abs(x) >= a) return 0.0f;
    float lanczos = (sin(pi_x) / pi_x) * (sin(pi_x / a) / (pi_x / a));
    return lanczos;
}

float GaussianWeight(float x, float sigma) {
    return exp(-(x * x) / (2.0f * sigma * sigma));
}

void AddSplat(uint2 pixelCoord, float2 targetUV, float2 motionVector, float3 color, float depth, float confidence) {
    uint index = 0;
    gs_AccumulatedColor += float4(color * confidence, confidence);
    gs_AccumulatedWeight += confidence;
    
    if (depth < gs_AccumulatedDepth || gs_AccumulatedDepth < 0.0f) {
        gs_AccumulatedDepth = depth;
    }
}

float3 ForwardProject(float2 uv, float2 mv) {
    float2 projectedUV = uv + mv;
    if (any(projectedUV < 0.0f) || any(projectedUV > 1.0f)) {
        return float3(0.0f, 0.0f, 0.0f);
    }
    return g_CurrentColor.SampleLevel(g_CurrentColor, projectedUV, 0.0f).rgb;
}

float3 BackwardProject(float2 uv, float2 invMV) {
    float2 projectedUV = uv + invMV;
    if (any(projectedUV < 0.0f) || any(projectedUV > 1.0f)) {
        return float3(0.0f, 0.0f, 0.0f);
    }
    return g_HistoryColor.SampleLevel(g_HistoryColor, projectedUV, 0.0f).rgb;
}

float3 BidirectionalBlend(float2 uv, float2 mv, float depth, float historyDepth) {
    float3 forwardColor = ForwardProject(uv, mv);
    float3 backwardColor = BackwardProject(uv, -mv);
    
    float depthConsistency = 1.0f - saturate(abs(depth - historyDepth) * 100.0f);
    
    float mvMagnitude = length(mv);
    float forwardWeight = 0.5f + (1.0f - mvMagnitude * g_MotionScale) * 0.3f;
    float backwardWeight = 0.5f + (1.0f - mvMagnitude * g_MotionScale) * 0.3f;
    
    float3 blendColor = forwardColor * forwardWeight + backwardColor * backwardWeight;
    
    float3 consensus = abs(forwardColor - backwardColor);
    float consensusStrength = 1.0f - saturate((consensus.r + consensus.g + consensus.b) / 3.0f);
    
    float3 finalColor = lerp(blendColor, forwardColor, depthConsistency * 0.5f + consensusStrength * 0.5f);
    
    return finalColor;
}

float CalculateVelocityConsistency(float2 mv, float2 neighborMVs[8]) {
    float sum = 0.0f;
    [unroll]
    for (int i = 0; i < 8; i++) {
        float2 diff = mv - neighborMVs[i];
        sum += dot(diff, diff);
    }
    float avgDiff = sum / 8.0f;
    return saturate(1.0f - avgDiff * 10.0f);
}

void HoleFill(uint2 globalId) {
    float2 uv = (float2(globalId) + 0.5f) / float2(g_Resolution);
    float2 texelSize = 1.0f / float2(g_Resolution);
    
    float centerConfidence = g_OutputConfidence[globalId];
    
    if (centerConfidence > 0.01f) return;
    
    float3 colors[8];
    float depths[8];
    int validCount = 0;
    
    uint2 offsets[8] = {
        uint2(-1, 0), uint2(1, 0), uint2(0, -1), uint2(0, 1),
        uint2(-1, -1), uint2(1, -1), uint2(-1, 1), uint2(1, 1)
    };
    
    [unroll]
    for (int i = 0; i < 8; i++) {
        uint2 neighborCoord = globalId + offsets[i];
        if (any(neighborCoord >= g_Resolution)) continue;
        
        float neighborConf = g_OutputConfidence[neighborCoord];
        if (neighborConf > 0.1f) {
            colors[validCount] = g_OutputColor[neighborCoord].rgb;
            depths[validCount] = g_OutputDepth[neighborCoord];
            validCount++;
        }
    }
    
    if (validCount == 0) return;
    
    float3 sumColor = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    float centerDepth = g_CurrentDepth.SampleLevel(g_CurrentDepth, uv, 0.0f);
    
    [unroll]
    for (int i = 0; i < validCount; i++) {
        float depthDiff = abs(depths[i] - centerDepth);
        float weight = 1.0f / (1.0f + depthDiff * g_DepthWeight * 10.0f);
        
        if (depthDiff < 0.01f) {
            weight *= 2.0f;
        }
        
        sumColor += colors[i] * weight;
        totalWeight += weight;
    }
    
    float3 filledColor = sumColor / max(totalWeight, 0.0001f);
    
    float blendFactor = 1.0f - centerConfidence;
    float3 finalColor = lerp(g_OutputColor[globalId].rgb, filledColor, blendFactor * 0.8f);
    
    g_OutputColor[globalId] = float4(saturate(finalColor), g_OutputColor[globalId].a);
}

[numthreads(256, 1, 1)]
void SplatForwardCS(uint3 dispatchId : SV_DispatchThreadID) {
    uint flatIndex = dispatchId.x;
    
    uint2 tileSize = uint2(32, 32);
    uint tilesX = (g_Resolution.x + tileSize.x - 1) / tileSize.x;
    uint tilesY = (g_Resolution.y + tileSize.y - 1) / tileSize.y;
    uint totalTiles = tilesX * tilesY;
    
    if (flatIndex >= totalTiles * tileSize.x * tileSize.y) return;
    
    uint tileIndex = flatIndex / (tileSize.x * tileSize.y);
    uint pixelIndex = flatIndex % (tileSize.x * tileSize.y);
    
    uint tileX = tileIndex % tilesX;
    uint tileY = tileIndex / tilesX;
    
    uint pixelX = tileX * tileSize.x + (pixelIndex % tileSize.x);
    uint pixelY = tileY * tileSize.y + (pixelIndex / tileSize.x);
    
    if (pixelX >= g_Resolution.x || pixelY >= g_Resolution.y) return;
    
    uint2 pixelCoord = uint2(pixelX, pixelY);
    float2 uv = (float2(pixelCoord) + 0.5f) / float2(g_Resolution);
    
    float depth = g_CurrentDepth[pixelCoord];
    float2 mv = g_CurrentMotionVector[pixelCoord] * g_MotionScale;
    float3 color = g_CurrentColor[pixelCoord].rgb;
    float confidence = 1.0f;
    
    gs_AccumulatedColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    gs_AccumulatedWeight = 0.0f;
    gs_AccumulatedDepth = -1.0f;
    
    int splatRadius = (int)g_SplatRadius;
    
    [unroll]
    for (int dy = -splatRadius; dy <= splatRadius; dy++) {
        [unroll]
        for (int dx = -splatRadius; dx <= splatRadius; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            float dist = sqrt(float(dx * dx + dy * dy));
            if (dist > g_SplatRadius) continue;
            
            float weight = GaussianWeight(dist, g_SplatRadius * 0.5f);
            
            float2 offsetUV = uv + float2(dx, dy) * (1.0f / float2(g_Resolution));
            
            float3 sampleColor = g_CurrentColor.SampleLevel(g_CurrentColor, offsetUV, 0.0f).rgb;
            float sampleDepth = g_CurrentDepth.SampleLevel(g_CurrentDepth, offsetUV, 0.0f);
            
            if (sampleDepth > depth) {
                weight *= 0.5f;
            }
            
            if (weight > 0.01f) {
                AddSplat(pixelCoord, offsetUV, mv, sampleColor, sampleDepth, weight * confidence);
            }
        }
    }
    
    if (gs_AccumulatedWeight > 0.0f) {
        float3 avgColor = gs_AccumulatedColor.rgb / gs_AccumulatedColor.a;
        float3 finalColor = lerp(color, avgColor, g_ConfidenceScale);
        
        g_OutputColor[pixelCoord] = float4(saturate(finalColor), 0.5f);
        g_OutputDepth[pixelCoord] = gs_AccumulatedDepth >= 0.0f ? gs_AccumulatedDepth : depth;
        g_OutputMotionVector[pixelCoord] = mv;
        g_OutputConfidence[pixelCoord] = saturate(gs_AccumulatedWeight / (float)((splatRadius * 2 + 1) * (splatRadius * 2 + 1)));
    } else {
        g_OutputColor[pixelCoord] = float4(color, 0.5f);
        g_OutputDepth[pixelCoord] = depth;
        g_OutputMotionVector[pixelCoord] = mv;
        g_OutputConfidence[pixelCoord] = confidence;
    }
}

[numthreads(256, 1, 1)]
void BidirectionalBlendCS(uint3 dispatchId : SV_DispatchThreadID) {
    uint flatIndex = dispatchId.x;
    
    uint2 pixelCoord = uint2(flatIndex % g_Resolution.x, flatIndex / g_Resolution.x);
    
    if (any(pixelCoord >= g_Resolution)) return;
    
    float2 uv = (float2(pixelCoord) + 0.5f) / float2(g_Resolution);
    
    float depth = g_CurrentDepth[pixelCoord];
    float historyDepth = g_HistoryDepth[pixelCoord];
    float2 mv = g_CurrentMotionVector[pixelCoord] * g_MotionScale;
    
    float3 currentColor = g_CurrentColor[pixelCoord].rgb;
    float4 historyColor = g_HistoryColor[pixelCoord];
    float historyConfidence = g_HistoryConfidence[pixelCoord];
    
    float3 bidirColor = BidirectionalBlend(uv, mv, depth, historyDepth);
    
    float velocityConsistency = CalculateVelocityConsistency(mv, &g_HistoryMotionVector[pixelCoord]);
    
    float depthDiff = abs(depth - historyDepth);
    float depthConsistency = 1.0f - saturate(depthDiff * 100.0f);
    
    float totalConsistency = (velocityConsistency + depthConsistency) * 0.5f;
    
    float3 blendedColor = lerp(currentColor, bidirColor, totalConsistency * 0.5f);
    
    float3 finalColor = lerp(blendedColor, historyColor.rgb, historyConfidence * 0.3f);
    
    g_OutputColor[pixelCoord] = float4(saturate(finalColor), historyColor.a);
    g_OutputDepth[pixelCoord] = depth;
    g_OutputMotionVector[pixelCoord] = mv;
    g_OutputConfidence[pixelCoord] = totalConsistency;
}

[numthreads(8, 8, 1)]
void HoleFillCS(uint3 dispatchId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchId.xy;
    
    if (any(pixelCoord >= g_Resolution)) return;
    
    HoleFill(pixelCoord);
}
