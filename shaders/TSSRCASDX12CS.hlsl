// TSS RCAS v2.0 - Ultimate FSR 2.2 Killer
// Single-Pass Compute Shader with Wave Intrinsics & LDS
// Optimized for RTX 5060 Ti (DX12 Agility SDK)

#define TSS_GPU_WAVE_INTRINSICS 1

// Resource declarations
RWTexture2D<float4> OutputUAV : register(u0);
RWTexture2D<float2> HistoryLockUAV : register(u1);

Texture2D<float4> CurrentColor : register(t0);
Texture2D<float4> HistoryColor : register(t1);
Texture2D<float2> MotionVector : register(t2);
Texture2D<float> DepthTexture : register(t3);
Texture2D<float> ReactiveMask : register(t4);
Texture2D<float> LockMap : register(t5);
Texture2D<float2> AccumulationCounter : register(t6);
Texture2D<float> ExposureTex : register(t7);

cbuffer TSSConstants : register(b0)
{
    uint2 ViewOffset;
    uint2 ViewSize;
    uint2 InputSize;
    uint2 OutputSize;
    uint2 HistorySize;
    float2 TexelSize;
    float2 HistoryTexelSize;
    float Scale;
    float Sharpness;
    float VarianceK;
    float DisocclusionThreshold;
    float DepthThreshold;
    float LumaClampScale;
    float MinAlpha;
    float MaxAlpha;
    float VelocityScale;
    float AdaptationRate;
    float TimeDelta;
    uint FrameIndex;
    uint EnableJitter;
    float2 JitterOffset;
};

// Wave intrinsics wrappers
float WaveMin(float v) { return WaveActiveMin(v); }
float WaveMax(float v) { return WaveActiveMax(v); }
float WaveSum(float v) { return WaveActiveSum(v); }
float WaveAvg(float v) { return WaveSum(v) / (float)WaveGetLaneCount(); }
bool WaveAll(bool b) { return WaveActiveAll(b); }
bool WaveAny(bool b) { return WaveActiveAny(b); }

// LDS (Shared Memory) for cross-thread data sharing
groupshared float4 gs_Color[64];
groupshared float gs_Depth[64];
groupshared float2 gs_MV[64];
groupshared float gs_Lock[64];

// Constants
static const float PI = 3.14159265f;
static const float RCAS_DENOISE = 0.25f;
static const float EDGE_THRESHOLD = 0.02f;
static const float LOCK_THRESHOLD = 0.95f;
static const float STABILITY_THRESHOLD = 10.0f;
static const float DIVERGENCE_PENALTY = 5.0f;

// Half-precision helpers
#define F16 float
#define F16_2 float2
#define F16_4 float4

// Pack/unpack motion vectors (SNORM16)
F16_2 PackMV(float2 mv) {
    return f16tof32(mv * 32767.0);
}

float2 UnpackMV(F16_2 packed) {
    return f32tof16(packed) / 32767.0;
}

// YCoCg color space conversion
float3 RGB_to_YCoCg(float3 rgb) {
    float Y = 0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b;
    float Co = 0.5 * rgb.r - 0.5 * rgb.b;
    float Cg = -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b;
    return float3(Y, Co, Cg);
}

float3 YCoCg_to_RGB(float3 ycocg) {
    float r = ycocg.x + ycocg.y - ycocg.z;
    float g = ycocg.x + ycocg.z;
    float b = ycocg.x - ycocg.y - ycocg.z;
    return float3(r, g, b);
}

// Luminance calculation
float CalcLuma(float3 color) {
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// Variance calculation using Wave intrinsics
float2 CalcVarianceWithWave(float luma) {
    float sum = WaveSum(luma);
    float sumSq = WaveSum(luma * luma);
    float count = (float)WaveGetLaneCount();
    
    float mean = sum / count;
    float variance = (sumSq / count) - (mean * mean);
    
    return float2(variance, mean);
}

// Depth-based disocclusion check
float CheckDisocclusion(float currentDepth, float historyDepth, float2 mv) {
    float depthDiff = abs(currentDepth - historyDepth);
    float mvMag = length(mv);
    
    float disocclusion = depthDiff * 100.0 + mvMag * 0.1;
    
    return saturate(disocclusion);
}

// Wave-based min/max reduction for AABB clipping
float2 WaveReduceMinMax(float value) {
    float minVal = WaveMin(value);
    float maxVal = WaveMax(value);
    return float2(minVal, maxVal);
}

// Neighborhood AABB color clamping (using Wave intrinsics)
float3 AABBClipWithWave(float3 historyColor, float3 centerColor, float clipScale) {
    // Gather neighborhood values from LDS
    float3 minColor = centerColor;
    float3 maxColor = centerColor;
    
    [unroll]
    for (int i = 0; i < 8; i++) {
        float3 neighbor = gs_Color[i].rgb;
        minColor = min(minColor, neighbor);
        maxColor = max(maxColor, neighbor);
    }
    
    // Apply Wave reduction for cross-thread minimum/maximum
    float2 minMaxR = WaveReduceMinMax(historyColor.r);
    float2 minMaxG = WaveReduceMinMax(historyColor.g);
    float2 minMaxB = WaveReduceMinMax(historyColor.b);
    
    minColor.r = WaveMin(minColor.r);
    minColor.g = WaveMin(minColor.g);
    minColor.b = WaveMin(minColor.b);
    maxColor.r = WaveMax(maxColor.r);
    maxColor.g = WaveMax(maxColor.g);
    maxColor.b = WaveMax(maxColor.b);
    
    // Clamp history to AABB bounds
    float3 range = (maxColor - minColor) * clipScale;
    float3 clamped = clamp(historyColor, minColor - range, maxColor + range);
    
    return clamped;
}

// Variance clipping (Unreal Engine TAA style)
float3 VarianceClipWithWave(float3 historyColor, float3 centerColor, float kSigma) {
    float centerLuma = CalcLuma(centerColor);
    float historyLuma = CalcLuma(historyColor);
    
    // Calculate mean and variance using Wave intrinsics
    float2 varMean = CalcVarianceWithWave(centerLuma);
    float variance = varMean.x;
    float mean = varMean.y;
    float stdDev = sqrt(max(variance, 0.0));
    
    // Calculate luminance bounds
    float minLuma = mean - kSigma * stdDev;
    float maxLuma = mean + kSigma * stdDev;
    
    // If history luminance is outside bounds, clip
    if (historyLuma < minLuma || historyLuma > maxLuma) {
        float clipFactor = saturate((maxLuma - minLuma) / (abs(historyLuma - mean) + 0.001));
        historyColor = lerp(historyColor, centerColor, clipFactor * 0.5);
    }
    
    return historyColor;
}

// Adaptive sharpening using local contrast
float CalculateAdaptiveSharpness(float3 center, float3 neighbors[8]) {
    float centerLuma = CalcLuma(center);
    
    float minLuma = centerLuma;
    float maxLuma = centerLuma;
    float sumLuma = centerLuma;
    int count = 1;
    
    [unroll]
    for (int i = 0; i < 8; i++) {
        float neighborLuma = CalcLuma(neighbors[i]);
        minLuma = min(minLuma, neighborLuma);
        maxLuma = max(maxLuma, neighborLuma);
        sumLuma += neighborLuma;
        count++;
    }
    
    float avgLuma = sumLuma / (float)count;
    float localContrast = (maxLuma - minLuma) / (avgLuma + 0.001);
    
    float sharpness = 0.0;
    if (localContrast > EDGE_THRESHOLD) {
        sharpness = min(Sharpness, localContrast * 0.5);
    }
    
    return sharpness;
}

// Anti-ringing filter
float3 AntiRinging(float3 color, float3 neighbors[8]) {
    float3 minN = color;
    float3 maxN = color;
    
    [unroll]
    for (int i = 0; i < 8; i++) {
        minN = min(minN, neighbors[i]);
        maxN = max(maxN, neighbors[i]);
    }
    
    float3 range = (maxN - minN) * 0.1;
    return clamp(color, minN - range, maxN + range);
}

// Luma-weighted blending for particles
float CalculateLumaWeight(float historyLuma, float currentLuma, float threshold) {
    float lumaDiff = abs(currentLuma - historyLuma);
    
    float weight = 1.0;
    if (lumaDiff > threshold) {
        float excess = lumaDiff - threshold;
        weight = max(0.1, 1.0 - excess * 2.0);
    }
    
    return weight;
}

// Accumulation counter update
float2 UpdateAccumulationCounter(float2 counter, float lumaDiff, float mvMag, bool isStable) {
    float historyCount = counter.x;
    float lastLumaDiff = counter.y;
    
    if (lumaDiff < 0.01 && mvMag < 0.01 && isStable) {
        historyCount = min(historyCount + 1, 255);
    } else {
        float penalty = min(lumaDiff * 10.0 + mvMag * 5.0, historyCount);
        historyCount = max(0, historyCount - penalty);
    }
    
    return float2(historyCount, lumaDiff);
}

// Calculate EMA weight based on velocity
float CalculateEMAWeight(float velocityMag) {
    float normalizedVel = min(velocityMag * VelocityScale, 1.0);
    return lerp(MinAlpha, MaxAlpha, normalizedVel);
}

// Load tile data into LDS
void LoadTileIntoLDS(uint2 pixelPos) {
    uint localId = (WaveGetLaneIndex() % 8) + (WaveGetLaneIndex() / 8) * 8;
    
    float4 color = CurrentColor.Load(int3(pixelPos, 0));
    float depth = DepthTexture.Load(int3(pixelPos, 0)).r;
    float2 mv = MotionVector.Load(int3(pixelPos, 0)).xy;
    float lock = LockMap.Load(int3(pixelPos, 0)).r;
    
    gs_Color[localId] = color;
    gs_Depth[localId] = depth;
    gs_MV[localId] = mv;
    gs_Lock[localId] = lock;
    
    GroupMemoryBarrierWithGroupSync();
}

// Main RCAS kernel
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadId.xy;
    
    if (pixelPos.x >= OutputSize.x || pixelPos.y >= OutputSize.y)
        return;
    
    // Load data into LDS
    LoadTileIntoLDS(pixelPos);
    
    // Calculate UV coordinates
    float2 outputUv = ((float2)pixelPos + 0.5) / (float2)OutputSize;
    float2 inputUv = outputUv * Scale;
    uint2 inputPixelPos = uint2(inputUv * (float2)InputSize);
    
    // Sample current frame
    float4 currentColor = CurrentColor.Load(int3(pixelPos, 0));
    float currentDepth = DepthTexture.Load(int3(pixelPos, 0)).r;
    float2 mv = MotionVector.Load(int3(pixelPos, 0)).xy;
    float reactive = ReactiveMask.Load(int3(pixelPos, 0)).r;
    float lock = LockMap.Load(int3(pixelPos, 0)).r;
    
    // Reproject to history
    float2 historyUv = outputUv - mv * Scale / (float2)HistorySize;
    float4 historyColor = HistoryColor.SampleLevel(linear_sampler, historyUv, 0);
    float historyDepth = DepthTexture.SampleLevel(linear_sampler, historyUv, 0).r;
    
    // Load accumulation counter
    float2 accumCounter = AccumulationCounter.Load(int3(pixelPos, 0)).xy;
    
    // Disocclusion check
    float disocclusion = CheckDisocclusion(currentDepth, historyDepth, mv);
    bool isDisoccluded = disocclusion > DisocclusionThreshold;
    
    // Load neighborhood for clamping
    float3 neighbors[8];
    neighbors[0] = CurrentColor.Load(int3(pixelPos + int2(-1, 0), 0)).rgb;
    neighbors[1] = CurrentColor.Load(int3(pixelPos + int2(1, 0), 0)).rgb;
    neighbors[2] = CurrentColor.Load(int3(pixelPos + int2(0, -1), 0)).rgb;
    neighbors[3] = CurrentColor.Load(int3(pixelPos + int2(0, 1), 0)).rgb;
    neighbors[4] = CurrentColor.Load(int3(pixelPos + int2(-1, -1), 0)).rgb;
    neighbors[5] = CurrentColor.Load(int3(pixelPos + int2(1, -1), 0)).rgb;
    neighbors[6] = CurrentColor.Load(int3(pixelPos + int2(-1, 1), 0)).rgb;
    neighbors[7] = CurrentColor.Load(int3(pixelPos + int2(1, 1), 0)).rgb;
    
    // Apply AABB clipping
    float3 clippedHistory = AABBClipWithWave(historyColor.rgb, currentColor.rgb, LumaClampScale);
    
    // Apply variance clipping
    clippedHistory = VarianceClipWithWave(clippedHistory, currentColor.rgb, VarianceK);
    
    // Calculate luma-weighted blend weight
    float historyLuma = CalcLuma(clippedHistory);
    float currentLuma = CalcLuma(currentColor.rgb);
    float lumaWeight = CalculateLumaWeight(historyLuma, currentLuma, 0.05);
    
    // Calculate adaptive alpha based on velocity
    float mvMagnitude = length(mv);
    float alpha = CalculateEMAWeight(mvMagnitude);
    alpha = max(alpha, reactive);
    
    // Override for disocclusion
    if (isDisoccluded) {
        alpha = 1.0;
    }
    
    // Apply lock
    float lockWeight = max(lock, LockThreshold);
    alpha = lerp(alpha, 0.0, lockWeight * 0.5);
    
    // Accumulate
    float4 result = float4(lerp(clippedHistory, currentColor.rgb, alpha), 1.0);
    
    // Apply anti-ringing
    result.rgb = AntiRinging(result.rgb, neighbors);
    
    // Adaptive sharpening
    float sharpness = CalculateAdaptiveSharpness(result.rgb, neighbors);
    if (sharpness > 0.01) {
        // Simple sharpen using 5-tap filter
        float3 blurH = float3(0.0);
        blurH += CurrentColor.Load(int3(pixelPos + int2(-2, 0), 0)).rgb * 0.06136;
        blurH += CurrentColor.Load(int3(pixelPos + int2(-1, 0), 0)).rgb * 0.24477;
        blurH += CurrentColor.Load(int3(pixelPos, 0)).rgb * 0.38774;
        blurH += CurrentColor.Load(int3(pixelPos + int2(1, 0), 0)).rgb * 0.24477;
        blurH += CurrentColor.Load(int3(pixelPos + int2(2, 0), 0)).rgb * 0.06136;
        
        float3 blurV = float3(0.0);
        blurV += CurrentColor.Load(int3(pixelPos + int2(0, -2), 0)).rgb * 0.06136;
        blurV += CurrentColor.Load(int3(pixelPos + int2(0, -1), 0)).rgb * 0.24477;
        blurV += CurrentColor.Load(int3(pixelPos, 0)).rgb * 0.38774;
        blurV += CurrentColor.Load(int3(pixelPos + int2(0, 1), 0)).rgb * 0.24477;
        blurV += CurrentColor.Load(int3(pixelPos + int2(0, 2), 0)).rgb * 0.06136;
        
        float3 blur = blurH * 0.5 + blurV * 0.5;
        
        result.rgb += (result.rgb - blur) * sharpness;
    }
    
    // Clamp final output
    result.rgb = saturate(result.rgb);
    
    // Write output
    OutputUAV[pixelPos] = result;
    
    // Update accumulation counter
    float lumaDiff = abs(currentLuma - historyLuma);
    float2 newCounter = UpdateAccumulationCounter(accumCounter, lumaDiff, mvMagnitude, !isDisoccluded);
    HistoryLockUAV[pixelPos] = newCounter;
}
