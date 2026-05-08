// TSS Ultimate - FSR 2.2 Killer
// Wave Intrinsics + Variance Clipping + YCoCg + Jitter
// Optimized for RTX 5060 Ti / DX12 Agility SDK

#define TSS_WAVE_INTRINSICS 1
#define TSS_YCOCG_COLOR_SPACE 1
#define TSS_VARIANCE_CLIPPING 1
#define TSS_JITTER_SUPPORT 1

// Resource declarations
RWTexture2D<float4> OutputUAV : register(u0);
RWTexture2D<float> HistoryLockUAV : register(u1);
RWTexture2D<float2> AccumulationCounterUAV : register(u2);

Texture2D<float4> CurrentColor : register(t0);
Texture2D<float4> PrevColor : register(t1);
Texture2D<float2> MotionVectors : register(t2);
Texture2D<float> DepthTexture : register(t3);
Texture2D<float> ReactiveMaskTex : register(t4);
Texture2D<float> LockMapTex : register(t5);

cbuffer TSSConstants : register(b0)
{
    uint2 RenderOffset;
    uint2 RenderSize;
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
    float JitterX;
    float JitterY;
    uint FrameIndex;
    uint EnableJitter;
    uint ColorSpace; // 0=RGB, 1=YCoCg
    uint Padding;
};

// Wave intrinsics
float WaveMin(float v) { return WaveActiveMin(v); }
float WaveMax(float v) { return WaveActiveMax(v); }
float WaveSum(float v) { return WaveActiveSum(v); }
float WaveAvg(float v) { return WaveSum(v) / (float)WaveGetLaneCount(); }

// LDS for cross-thread data
groupshared float4 gs_Color[64];
groupshared float gs_Depth[64];
groupshared float2 gs_MV[64];

// Constants
static const float PI = 3.14159265f;
static const float EDGE_THRESHOLD = 0.02f;
static const float LOCK_THRESHOLD = 0.95f;
static const float YCOCG_RC = 0.5f;
static const float YCOCG_GC = 0.25f;
static const float YCOCG_BC = 0.25f;

// RGB to YCoCg conversion
float3 RGB_to_YCoCg(float3 rgb) {
    float Y = YCOCG_RC * rgb.r + YCOCG_GC * rgb.g + YCOCG_BC * rgb.b;
    float Co = YCOCG_RC * rgb.r - YCOCG_BC * rgb.b;
    float Cg = -YCOCG_RC * rgb.r + YCOCG_GC * rgb.g - YCOCG_BC * rgb.b;
    return float3(Y, Co, Cg);
}

// YCoCg to RGB conversion
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

// Wave-based variance calculation (kSigma clipping)
struct VarianceResult {
    float mean;
    float variance;
    float stdDev;
    float minVal;
    float maxVal;
};

VarianceResult CalcVarianceWave(float values[9]) {
    VarianceResult result;
    
    float sum = WaveSum(values[4]);
    float sumSq = WaveSum(values[4] * values[4]);
    float count = (float)WaveGetLaneCount();
    
    result.mean = sum / count;
    result.variance = (sumSq / count) - (result.mean * result.mean);
    result.stdDev = sqrt(max(result.variance, 0.0));
    
    result.minVal = WaveMin(values[4]);
    result.maxVal = WaveMax(values[4]);
    
    return result;
}

// Variance clipping (Unreal Engine TAA style)
float3 VarianceClip(float3 history, float3 current, float k) {
    float3 center = current;
    
    // Get 3x3 neighborhood
    float3 neighborhood[9];
    int idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            float2 samplePos = (float2)RenderOffset + float2(dx, dy) * TexelSize;
            neighborhood[idx++] = CurrentColor.SampleLevel(linear_sampler, samplePos, 0).rgb;
        }
    }
    
    // Calculate variance using Wave intrinsics
    float luma[9];
    for (int i = 0; i < 9; i++) {
        luma[i] = CalcLuma(neighborhood[i]);
    }
    
    VarianceResult var = CalcVarianceWave(luma);
    
    float historyLuma = CalcLuma(history);
    float currentLuma = CalcLuma(current);
    
    // Variance-based clamping
    float minLuma = var.mean - k * var.stdDev;
    float maxLuma = var.mean + k * var.stdDev;
    
    if (historyLuma < minLuma || historyLuma > maxLuma) {
        float clipFactor = saturate((maxLuma - minLuma) / (abs(historyLuma - var.mean) + 0.001));
        history = lerp(history, current, clipFactor * 0.5);
    }
    
    return history;
}

// Depth-based disocclusion check
float CheckDisocclusion(float currentDepth, float historyDepth, float2 mv) {
    float depthDiff = abs(currentDepth - historyDepth);
    float mvMag = length(mv);
    
    float disocclusion = depthDiff * 100.0 + mvMag * 0.1;
    
    return saturate(disocclusion);
}

// Dilated motion vector (closest to camera in 3x3)
float2 DilateMotionVector(float2 uv, float currentDepth) {
    float2 dilatedMV = float2(0, 0);
    float minDepth = currentDepth;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            float2 offset = float2(dx, dy) * TexelSize;
            float2 sampleUV = uv + offset;
            
            float2 sampleMV = MotionVectors.SampleLevel(linear_sampler, sampleUV, 0).xy;
            float sampleDepth = DepthTexture.SampleLevel(linear_sampler, sampleUV, 0).r;
            
            if (sampleDepth < minDepth && length(sampleMV) > 0.001) {
                minDepth = sampleDepth;
                dilatedMV = sampleMV;
            }
        }
    }
    
    return dilatedMV;
}

// YCoCg clamping with neighborhood
float3 ClampYCoCg(float3 historyYCC, float3 currentYCC, float k) {
    float3 result = historyYCC;
    
    // Get 3x3 neighborhood in YCoCg
    float3 samples[9];
    int idx = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            float2 offset = float2(dx, dy) * TexelSize;
            float2 sampleUV = (float2)RenderOffset / (float2)InputSize + offset;
            float3 rgb = CurrentColor.SampleLevel(linear_sampler, sampleUV, 0).rgb;
            samples[idx++] = RGB_to_YCoCg(rgb);
        }
    }
    
    // Calculate Y statistics
    float ySamples[9];
    for (int i = 0; i < 9; i++) ySamples[i] = samples[i].x;
    VarianceResult yVar = CalcVarianceWave(ySamples);
    
    // Clamp Y with k-sigma
    float yMin = yVar.mean - k * yVar.stdDev;
    float yMax = yVar.mean + k * yVar.stdDev;
    result.x = clamp(result.x, yMin, yMax);
    
    // Clamp Co and Cg with wider bounds (less sensitive)
    float coMin = WaveMin(result.y);
    float coMax = WaveMax(result.y);
    float cgMin = WaveMin(result.z);
    float cgMax = WaveMax(result.z);
    
    result.y = clamp(result.y, coMin - 0.1, coMax + 0.1);
    result.z = clamp(result.z, cgMin - 0.1, cgMax + 0.1);
    
    return result;
}

// Adaptive sharpening
float3 AdaptiveSharpen(float3 color, float2 uv, float amount) {
    float3 blurH = float3(0.0);
    blurH += CurrentColor.SampleLevel(linear_sampler, uv + float2(-2, 0) * TexelSize, 0).rgb * 0.06136;
    blurH += CurrentColor.SampleLevel(linear_sampler, uv + float2(-1, 0) * TexelSize, 0).rgb * 0.24477;
    blurH += CurrentColor.SampleLevel(linear_sampler, uv, 0).rgb * 0.38774;
    blurH += CurrentColor.SampleLevel(linear_sampler, uv + float2(1, 0) * TexelSize, 0).rgb * 0.24477;
    blurH += CurrentColor.SampleLevel(linear_sampler, uv + float2(2, 0) * TexelSize, 0).rgb * 0.06136;
    
    float3 blurV = float3(0.0);
    blurV += CurrentColor.SampleLevel(linear_sampler, uv + float2(0, -2) * TexelSize, 0).rgb * 0.06136;
    blurV += CurrentColor.SampleLevel(linear_sampler, uv + float2(0, -1) * TexelSize, 0).rgb * 0.24477;
    blurV += CurrentColor.SampleLevel(linear_sampler, uv, 0).rgb * 0.38774;
    blurV += CurrentColor.SampleLevel(linear_sampler, uv + float2(0, 1) * TexelSize, 0).rgb * 0.24477;
    blurV += CurrentColor.SampleLevel(linear_sampler, uv + float2(0, 2) * TexelSize, 0).rgb * 0.06136;
    
    float3 blur = (blurH + blurV) * 0.5;
    
    // Edge detection for adaptive sharpening
    float centerLuma = CalcLuma(color);
    float blurLuma = CalcLuma(blur);
    float contrast = abs(centerLuma - blurLuma);
    
    float sharpenAmount = amount * saturate(contrast * 2.0);
    
    return color + (color - blur) * sharpenAmount;
}

// Anti-ringing
float3 AntiRinging(float3 color, float3 neighbors[8]) {
    float3 minN = color;
    float3 maxN = color;
    
    [unroll]
    for (int i = 0; i < 8; i++) {
        minN = min(minN, neighbors[i]);
        maxN = max(maxN, neighbors[i]);
    }
    
    float3 range = (maxN - minN) * 0.05;
    return clamp(color, minN - range, maxN + range);
}

// Main compute shader
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixelPos = dispatchThreadId.xy;
    
    if (pixelPos.x >= OutputSize.x || pixelPos.y >= OutputSize.y)
        return;
    
    // UV coordinates
    float2 outputUv = ((float2)pixelPos + 0.5) / (float2)OutputSize;
    float2 inputUv = outputUv * Scale;
    uint2 inputPixelPos = uint2(inputUv * (float2)InputSize);
    
    // Sample current frame
    float4 currentColor = CurrentColor.Load(int3(pixelPos, 0));
    float currentDepth = DepthTexture.Load(int3(pixelPos, 0)).r;
    float2 mv = MotionVectors.Load(int3(pixelPos, 0)).xy;
    float reactive = ReactiveMaskTex.Load(int3(pixelPos, 0)).r;
    float lock = LockMapTex.Load(int3(pixelPos, 0)).r;
    
    // Apply jitter to input UV
    if (EnableJitter > 0) {
        float2 jitterOffset = float2(JitterX, JitterY) * TexelSize;
        inputUv -= jitterOffset;
    }
    
    // Dilate motion vectors for thin geometry
    float2 dilatedMV = DilateMotionVector(outputUv, currentDepth);
    
    // Reproject to history
    float2 historyUv = outputUv - dilatedMV * Scale / (float2)HistorySize;
    float4 historyColor = PrevColor.SampleLevel(linear_sampler, historyUv, 0);
    float historyDepth = DepthTexture.SampleLevel(linear_sampler, historyUv, 0).r;
    
    // Disocclusion check
    float disocclusion = CheckDisocclusion(currentDepth, historyDepth, dilatedMV);
    bool isDisoccluded = disocclusion > DisocclusionThreshold;
    
    // Load neighborhood for anti-ringing
    float3 neighbors[8];
    neighbors[0] = CurrentColor.Load(int3(pixelPos + int2(-1, 0), 0)).rgb;
    neighbors[1] = CurrentColor.Load(int3(pixelPos + int2(1, 0), 0)).rgb;
    neighbors[2] = CurrentColor.Load(int3(pixelPos + int2(0, -1), 0)).rgb;
    neighbors[3] = CurrentColor.Load(int3(pixelPos + int2(0, 1), 0)).rgb;
    neighbors[4] = CurrentColor.Load(int3(pixelPos + int2(-1, -1), 0)).rgb;
    neighbors[5] = CurrentColor.Load(int3(pixelPos + int2(1, -1), 0)).rgb;
    neighbors[6] = CurrentColor.Load(int3(pixelPos + int2(-1, 1), 0)).rgb;
    neighbors[7] = CurrentColor.Load(int3(pixelPos + int2(1, 1), 0)).rgb;
    
    // Convert to YCoCg if enabled
    float3 currentYCC = (ColorSpace == 1) ? RGB_to_YCoCg(currentColor.rgb) : float3(currentColor.r, currentColor.g, currentColor.b);
    float3 historyYCC = (ColorSpace == 1) ? RGB_to_YCoCg(historyColor.rgb) : float3(historyColor.r, historyColor.g, historyColor.b);
    
    // Variance clipping
    historyYCC = VarianceClip(historyYCC, currentYCC, VarianceK);
    
    // YCoCg clamping
    if (ColorSpace == 1) {
        historyYCC = ClampYCoCg(historyYCC, currentYCC, VarianceK);
    }
    
    // Calculate adaptive blend weight
    float mvMagnitude = length(dilatedMV);
    float alpha = lerp(MinAlpha, MaxAlpha, saturate(mvMagnitude * VelocityScale));
    alpha = max(alpha, reactive);
    
    // Override for disocclusion
    if (isDisoccluded) {
        alpha = 1.0;
    }
    
    // Apply lock
    alpha = lerp(alpha, 0.0, lock * 0.5);
    
    // Accumulate
    float3 resultYCC = lerp(historyYCC, currentYCC, alpha);
    
    // Convert back to RGB
    float3 result = (ColorSpace == 1) ? YCoCg_to_RGB(resultYCC) : resultYCC;
    
    // Anti-ringing
    result = AntiRinging(result, neighbors);
    
    // Adaptive sharpening
    if (Sharpness > 0.01) {
        result = AdaptiveSharpen(result, inputUv, Sharpness);
    }
    
    // Clamp final output
    result = saturate(result);
    
    // Write output
    OutputUAV[pixelPos] = float4(result, 1.0);
    
    // Update accumulation counter
    float2 accumCounter = AccumulationCounterUAV.Load(int3(pixelPos, 0)).xy;
    float historyLuma = CalcLuma(historyColor.rgb);
    float currentLuma = CalcLuma(currentColor.rgb);
    float lumaDiff = abs(currentLuma - historyLuma);
    
    accumCounter.x = lerp(accumCounter.x, isDisoccluded ? 0.0 : 1.0, 0.1);
    accumCounter.y = lumaDiff;
    AccumulationCounterUAV[pixelPos] = accumCounter;
}
