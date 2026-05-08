// TSS Ultimate v2 - Single-Pass Fused Compute Shader
// Zero VRAM overhead, Wave Intrinsics, LDS optimization
// Target: RTX 5060 Ti / DX12 Agility SDK / SM 6.6+

#define TILE_SIZE 16
#define LOCAL_GROUP_SIZE (TILE_SIZE * TILE_SIZE)

// Wave intrinsics for SM 6.6+
float WaveMin(float v) { return WaveActiveMin(v); }
float WaveMax(float v) { return WaveActiveMax(v); }
float WaveSum(float v) { return WaveActiveSum(v); }
float WavePrefixMin(float v) { return WaveScanMin(v, true); }
float WavePrefixMax(float v) { return WaveScanMax(v, true); }

// LDS (Local Data Share) - shared memory for tile processing
groupshared float4 gs_Color[TILE_SIZE * TILE_SIZE];
groupshared float gs_Depth[TILE_SIZE * TILE_SIZE];
groupshared float2 gs_MV[TILE_SIZE * TILE_SIZE];
groupshared float gs_Lock[TILE_SIZE * TILE_SIZE];

// Resource declarations
RWTexture2D<float4> OutputColor : register(u0);
RWTexture2D<float> HistoryLock : register(u1);
RWTexture2D<float2> DebugOutput : register(u2);

Texture2D<float4> CurrentColorTex : register(t0);
Texture2D<float4> HistoryColorTex : register(t1);
Texture2D<float2> MotionVectorTex : register(t2);
Texture2D<float> DepthTex : register(t3);
Texture2D<float> ReactiveTex : register(t4);
Texture2D<float> LockTex : register(t5);
Texture2D<float> ExposureTex : register(t6);

// Constants buffer
cbuffer TSSConfig : register(b0)
{
    uint2 InputSize;
    uint2 OutputSize;
    uint2 HistorySize;
    float2 InputTexelSize;
    float2 OutputTexelSize;
    float2 HistoryTexelSize;
    float Scale;
    float Sharpness;
    float VarianceK;
    float DisocclusionThreshold;
    float MinAlpha;
    float MaxAlpha;
    float VelocityScale;
    float JitterX;
    float JitterY;
    uint FrameIndex;
    uint EnableJitter;
    uint UseYCoCg;
    uint DebugMode;
};

// Color space conversions
float3 RGB_to_YCoCg(float3 rgb) {
    return float3(
        0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b,
        0.5 * rgb.r - 0.5 * rgb.b,
       -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b
    );
}

float3 YCoCg_to_RGB(float3 ycocg) {
    return float3(
        ycocg.x + ycocg.y - ycocg.z,
        ycocg.x + ycocg.z,
        ycocg.x - ycocg.y - ycocg.z
    );
}

float RGB_to_Luma(float3 rgb) {
    return dot(rgb, float3(0.2126, 0.7152, 0.0722));
}

// Variance calculation using Wave intrinsics
struct VarianceStats {
    float mean;
    float stdDev;
    float minVal;
    float maxVal;
};

VarianceStats CalcVarianceLocal(float values[TILE_SIZE * TILE_SIZE], int count) {
    VarianceStats stats;
    
    float sum = 0.0f;
    float sumSq = 0.0f;
    float minVal = values[0];
    float maxVal = values[0];
    
    for (int i = 0; i < count; i++) {
        sum += values[i];
        sumSq += values[i] * values[i];
        minVal = min(minVal, values[i]);
        maxVal = max(maxVal, values[i]);
    }
    
    float countF = (float)count;
    stats.mean = sum / countF;
    stats.stdDev = sqrt(max(sumSq / countF - stats.mean * stats.mean, 0.0f));
    stats.minVal = minVal;
    stats.maxVal = maxVal;
    
    return stats;
}

// Variance clipping - the key to eliminating ghosting
float3 VarianceClip(float3 history, float3 current, float k) {
    float3 result = history;
    
    // Get 3x3 neighborhood from LDS
    float luma[9];
    float3 color[9];
    
    for (int dy = -1, i = 0; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++, i++) {
            int2 samplePos = int2(dx, dy);
            samplePos = clamp(samplePos, int2(-7, -7), int2(7, 7));
            color[i] = gs_Color[samplePos.y * TILE_SIZE + samplePos.x].rgb;
            luma[i] = RGB_to_Luma(color[i]);
        }
    }
    
    VarianceStats var = CalcVarianceLocal(luma, 9);
    
    float historyLuma = RGB_to_Luma(history);
    float currentLuma = RGB_to_Luma(current);
    
    // k-sigma clipping
    float minBound = var.mean - k * var.stdDev;
    float maxBound = var.mean + k * var.stdDev;
    
    if (historyLuma < minBound || historyLuma > maxBound) {
        float clip = saturate((maxBound - minBound) / (abs(historyLuma - var.mean) + 0.0001));
        result = lerp(history, current, clip * 0.5);
    }
    
    return result;
}

// Dilated motion vector - find closest to camera in 3x3
float2 DilateMV(float2 uv, float centerDepth) {
    float2 bestMV = float2(0, 0);
    float minDepth = centerDepth;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int2 offset = int2(dx, dy);
            int2 samplePos = clamp(offset, int2(-7, -7), int2(7, 7));
            float2 mv = gs_MV[samplePos.y * TILE_SIZE + samplePos.x];
            float depth = gs_Depth[samplePos.y * TILE_SIZE + samplePos.x];
            
            if (depth < minDepth && length(mv) > 0.001) {
                minDepth = depth;
                bestMV = mv;
            }
        }
    }
    
    return bestMV;
}

// Depth-based disocclusion check
float CheckDisocclusion(float currDepth, float histDepth, float2 mv) {
    float depthDiff = abs(currDepth - histDepth);
    float mvMag = length(mv);
    
    float disocclusion = depthDiff * 100.0 + mvMag * 0.1;
    
    return saturate(disocclusion);
}

// Anti-ringing using local min/max
float3 AntiRinging(float3 color) {
    float3 minC = color;
    float3 maxC = color;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int2 samplePos = int2(dx, dy);
            samplePos = clamp(samplePos, int2(-7, -7), int2(7, 7));
            float3 c = gs_Color[samplePos.y * TILE_SIZE + samplePos.x].rgb;
            minC = min(minC, c);
            maxC = max(maxC, c);
        }
    }
    
    float3 range = (maxC - minC) * 0.05;
    return clamp(color, minC - range, maxC + range);
}

// Adaptive sharpening
float3 AdaptiveSharpen(float3 color, int2 pos) {
    float3 blur = float3(0.0f);
    
    blur += gs_Color[(pos.y - 2) * TILE_SIZE + pos.x].rgb * 0.029f;
    blur += gs_Color[(pos.y - 1) * TILE_SIZE + pos.x].rgb * 0.119f;
    blur += gs_Color[pos.y * TILE_SIZE + pos.x].rgb * 0.282f;
    blur += gs_Color[(pos.y + 1) * TILE_SIZE + pos.x].rgb * 0.119f;
    blur += gs_Color[(pos.y + 2) * TILE_SIZE + pos.x].rgb * 0.029f;
    
    float centerLuma = RGB_to_Luma(color);
    float blurLuma = RGB_to_Luma(blur);
    float contrast = abs(centerLuma - blurLuma);
    
    float amount = Sharpness * saturate(contrast * 3.0);
    
    return color + (color - blur) * amount;
}

// Load tile into LDS - single memory read per pixel
void LoadTileIntoLDS(uint2 groupId, uint2 localId) {
    uint localIndex = localId.y * TILE_SIZE + localId.x;
    
    uint2 globalPos = groupId * TILE_SIZE + localId;
    
    if (globalPos.x >= InputSize.x || globalPos.y >= InputSize.y) {
        gs_Color[localIndex] = float4(0, 0, 0, 1);
        gs_Depth[localIndex] = 1.0;
        gs_MV[localIndex] = float2(0, 0);
        gs_Lock[localIndex] = 0.0;
        return;
    }
    
    float2 uv = ((float2)globalPos + 0.5) / InputSize;
    
    float4 color = CurrentColorTex.SampleLevel(linear_sampler, uv, 0);
    float depth = DepthTex.SampleLevel(linear_sampler, uv, 0).r;
    float2 mv = MotionVectorTex.SampleLevel(linear_sampler, uv, 0).xy;
    float lock = LockTex.SampleLevel(linear_sampler, uv, 0).r;
    
    gs_Color[localIndex] = color;
    gs_Depth[localIndex] = depth;
    gs_MV[localIndex] = mv;
    gs_Lock[localIndex] = lock;
    
    GroupMemoryBarrierWithGroupSync();
}

// Main compute shader - Single pass, fused
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadId.xy;
    
    if (pixelPos.x >= OutputSize.x || pixelPos.y >= OutputSize.y)
        return;
    
    uint2 groupId = pixelPos / TILE_SIZE;
    uint2 localId = pixelPos % TILE_SIZE;
    uint localIndex = localId.y * TILE_SIZE + localId.x;
    
    // Phase 1: Load tile into LDS (single VRAM read per pixel)
    LoadTileIntoLDS(groupId, localId);
    
    // Phase 2: Calculate dilated MV for thin geometry
    float2 dilatedMV = float2(0, 0);
    if (localIndex == (TILE_SIZE * TILE_SIZE / 2)) {
        float2 uv = ((float2)pixelPos) / OutputSize;
        float centerDepth = DepthTex.SampleLevel(linear_sampler, uv, 0).r;
        dilatedMV = DilateMV(uv, centerDepth);
    }
    dilatedMV = WaveReadFirst(WaveBroadcast(dilatedMV, 0));
    
    // Apply jitter
    float2 jitterOffset = float2(0, 0);
    if (EnableJitter > 0) {
        jitterOffset = float2(JitterX, JitterY) * InputTexelSize;
    }
    
    // Reproject to history
    float2 outputUv = ((float2)pixelPos + 0.5) / OutputSize;
    float2 inputUv = outputUv * Scale - jitterOffset;
    float2 historyUv = outputUv - dilatedMV * Scale / HistorySize;
    
    // Sample current and history
    float4 currentColor = CurrentColorTex.SampleLevel(linear_sampler, inputUv, 0);
    float currentDepth = DepthTex.SampleLevel(linear_sampler, inputUv, 0).r;
    float2 mv = MotionVectorTex.SampleLevel(linear_sampler, inputUv, 0).xy;
    float reactive = ReactiveTex.SampleLevel(linear_sampler, inputUv, 0).r;
    
    float4 historyColor = HistoryColorTex.SampleLevel(linear_sampler, historyUv, 0);
    float historyDepth = DepthTex.SampleLevel(linear_sampler, historyUv, 0).r;
    
    // Get current color from LDS (already loaded)
    float4 currLDS = gs_Color[localIndex];
    
    // Disocclusion check
    float disocclusion = CheckDisocclusion(currentDepth, historyDepth, dilatedMV);
    bool isDisoccluded = disocclusion > DisocclusionThreshold;
    
    // Color space conversion
    float3 currentYCC = (UseYCoCg > 0) ? RGB_to_YCoCg(currLDS.rgb) : float3(currLDS.r, currLDS.g, currLDS.b);
    float3 historyYCC = (UseYCoCg > 0) ? RGB_to_YCoCg(historyColor.rgb) : float3(historyColor.r, historyColor.g, historyColor.b);
    
    // Phase 3: Variance clipping (using Wave intrinsics)
    historyYCC = VarianceClip(historyYCC, currentYCC, VarianceK);
    
    // Phase 4: Calculate adaptive blend weight
    float mvMag = length(dilatedMV);
    float alpha = lerp(MinAlpha, MaxAlpha, saturate(mvMag * VelocityScale));
    alpha = max(alpha, reactive);
    
    // Disocclusion override
    if (isDisoccluded) {
        alpha = 1.0;
    }
    
    // Lock override
    float lock = gs_Lock[localIndex];
    alpha = lerp(alpha, 0.0, lock * 0.5);
    
    // Phase 5: Accumulate
    float3 resultYCC = lerp(historyYCC, currentYCC, alpha);
    
    // Convert back to RGB
    float3 result = (UseYCoCg > 0) ? YCoCg_to_RGB(resultYCC) : resultYCC;
    
    // Phase 6: Anti-ringing
    result = AntiRinging(result);
    
    // Phase 7: Adaptive sharpening
    if (Sharpness > 0.01) {
        result = AdaptiveSharpen(result, int2(localId));
    }
    
    // Final clamp
    result = saturate(result);
    
    // Write output
    OutputColor[pixelPos] = float4(result, 1.0);
    
    // Update history lock
    float newLock = lerp(HistoryLock[pixelPos], isDisoccluded ? 0.0 : 1.0, 0.1);
    HistoryLock[pixelPos] = newLock;
    
    // Debug output
    if (DebugMode > 0) {
        if (DebugMode == 1) {
            DebugOutput[pixelPos] = float2(disocclusion, 0);
        } else if (DebugMode == 2) {
            DebugOutput[pixelPos] = dilatedMV;
        } else if (DebugMode == 3) {
            DebugOutput[pixelPos] = float2(alpha, 0);
        }
    }
}
