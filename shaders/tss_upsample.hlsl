#define TSS_UPSAMPLE_VERSION "1.0.0"
#include "tss_common.hlsl"

RWTexture2D<float4> g_UpscaledColor : register(u0);

Texture2D<float4> g_LinearColor : register(t0);
Texture2D<float> g_LinearDepth : register(t1);
Texture2D<float3> g_LinearNormal : register(t2);

cbuffer TSSUpsampleConstants : register(b0) {
    float g_Sharpness;
    float g_EdgeThreshold;
    float g_LanczosTaps;
    float g_BicubicWeight;
    float g_GaussianSigma;
    float g_ContrastBoost;
    float2 g_InputSize;
    float2 g_OutputSize;
};

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

float CatmullRomWeight(float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    if (t < 1.0f) {
        return 1.5f * t3 - 2.5f * t2 + 1.0f;
    } else if (t < 2.0f) {
        return -0.5f * t3 + 2.5f * t2 - 4.0f * t + 2.0f;
    }
    return 0.0f;
}

float3 CatmullRomSample(Texture2D<float4> tex, float2 uv, float2 texelSize) {
    float2 p = uv / texelSize;
    float2 f = frac(p);
    float2 p0 = (floor(p) - 1.0f) * texelSize;
    float2 p1 = floor(p) * texelSize;
    float2 p2 = (floor(p) + 1.0f) * texelSize;
    float2 p3 = (floor(p) + 2.0f) * texelSize;
    
    float3 c0 = tex.SampleLevel(g_LinearSampler, p0, 0.0f).rgb;
    float3 c1 = tex.SampleLevel(g_LinearSampler, p1, 0.0f).rgb;
    float3 c2 = tex.SampleLevel(g_LinearSampler, p2, 0.0f).rgb;
    float3 c3 = tex.SampleLevel(g_LinearSampler, p3, 0.0f).rgb;
    
    float3 result = c1;
    
    float t = f.x;
    result = lerp(result, lerp(c1, c2, t), 0.5f);
    
    return result;
}

float ComputeEdgeStrength(float2 uv, float2 texelSize) {
    float3 center = g_LinearColor.SampleLevel(g_LinearSampler, uv, 0.0f).rgb;
    float3 left = g_LinearColor.SampleLevel(g_LinearSampler, uv + float2(-texelSize.x, 0.0f), 0.0f).rgb;
    float3 right = g_LinearColor.SampleLevel(g_LinearSampler, uv + float2(texelSize.x, 0.0f), 0.0f).rgb;
    float3 top = g_LinearColor.SampleLevel(g_LinearSampler, uv + float2(0.0f, -texelSize.y), 0.0f).rgb;
    float3 bottom = g_LinearColor.SampleLevel(g_LinearSampler, uv + float2(0.0f, texelSize.y), 0.0f).rgb;
    
    float3 gradX = abs(right - left);
    float3 gradY = abs(bottom - top);
    
    float edgeX = (gradX.r + gradX.g + gradX.b) / 3.0f;
    float edgeY = (gradY.r + gradY.g + gradY.b) / 3.0f;
    
    float edgeStrength = sqrt(edgeX * edgeX + edgeY * edgeY);
    
    return saturate(edgeStrength * 10.0f);
}

float ComputeDepthEdge(float2 uv, float2 texelSize) {
    float centerDepth = g_LinearDepth.SampleLevel(g_PointSampler, uv, 0.0f);
    float leftDepth = g_LinearDepth.SampleLevel(g_PointSampler, uv + float2(-texelSize.x, 0.0f), 0.0f);
    float rightDepth = g_LinearDepth.SampleLevel(g_PointSampler, uv + float2(texelSize.x, 0.0f), 0.0f);
    float topDepth = g_LinearDepth.SampleLevel(g_PointSampler, uv + float2(0.0f, -texelSize.y), 0.0f);
    float bottomDepth = g_LinearDepth.SampleLevel(g_PointSampler, uv + float2(0.0f, texelSize.y), 0.0f);
    
    float depthEdge = 0.0f;
    depthEdge = max(depthEdge, abs(centerDepth - leftDepth));
    depthEdge = max(depthEdge, abs(centerDepth - rightDepth));
    depthEdge = max(depthEdge, abs(centerDepth - topDepth));
    depthEdge = max(depthEdge, abs(centerDepth - bottomDepth));
    
    return saturate(depthEdge * 100.0f);
}

float ComputeNormalEdge(float2 uv, float2 texelSize) {
    float3 center = g_LinearNormal.SampleLevel(g_LinearSampler, uv, 0.0f).xyz;
    center = DecodeNormal(center);
    
    float3 left = g_LinearNormal.SampleLevel(g_LinearSampler, uv + float2(-texelSize.x, 0.0f), 0.0f).xyz;
    left = DecodeNormal(left);
    
    float3 right = g_LinearNormal.SampleLevel(g_LinearSampler, uv + float2(texelSize.x, 0.0f), 0.0f).xyz;
    right = DecodeNormal(right);
    
    float3 top = g_LinearNormal.SampleLevel(g_LinearSampler, uv + float2(0.0f, -texelSize.y), 0.0f).xyz;
    top = DecodeNormal(top);
    
    float3 bottom = g_LinearNormal.SampleLevel(g_LinearSampler, uv + float2(0.0f, texelSize.y), 0.0f).xyz;
    bottom = DecodeNormal(bottom);
    
    float normalEdge = 0.0f;
    normalEdge = max(normalEdge, 1.0f - dot(center, left));
    normalEdge = max(normalEdge, 1.0f - dot(center, right));
    normalEdge = max(normalEdge, 1.0f - dot(center, top));
    normalEdge = max(normalEdge, 1.0f - dot(center, bottom));
    
    return saturate(normalEdge * 5.0f);
}

float3 BilinearUpsample(Texture2D<float4> tex, float2 uv, float2 texelSize) {
    float2 samplePos = uv / texelSize - 0.5f;
    float2 frac = frac(samplePos);
    float2 base = floor(samplePos) * texelSize + 0.5f * texelSize;
    
    float3 s00 = tex.SampleLevel(g_PointSampler, base, 0.0f).rgb;
    float3 s10 = tex.SampleLevel(g_PointSampler, base + float2(texelSize.x, 0.0f), 0.0f).rgb;
    float3 s01 = tex.SampleLevel(g_PointSampler, base + float2(0.0f, texelSize.y), 0.0f).rgb;
    float3 s11 = tex.SampleLevel(g_PointSampler, base + texelSize, 0.0f).rgb;
    
    float3 top = lerp(s00, s10, frac.x);
    float3 bottom = lerp(s01, s11, frac.x);
    
    return lerp(top, bottom, frac.y);
}

float3 BicubicUpsample(Texture2D<float4> tex, float2 uv, float2 texelSize, float a) {
    float2 texel = uv / texelSize;
    float2 frac = frac(texel);
    
    float2 w0 = ((-a * frac + 2.0f * a) * frac - a) * frac + 0.0f;
    float2 w1 = ((a + 2.0f) * frac - a - 3.0f) * frac * frac + 1.0f;
    float2 w2 = ((-a - 2.0f) * frac + a + 3.0f) * frac * frac + 0.0f;
    float2 w3 = (a * frac - a) * frac * frac + 0.0f;
    
    float2 w12 = 1.0f - w0 - w1 - w2 - w3;
    
    float2 start = floor(texel) - 1.0f;
    if (start.x < 0.0f) { w0.x += w1.x; w1.x = w12.x; w12.x = 0.0f; start.x = 0.0f; }
    if (start.y < 0.0f) { w0.y += w1.y; w1.y = w12.y; w12.y = 0.0f; start.y = 0.0f; }
    
    float3 result = float3(0.0f, 0.0f, 0.0f);
    
    float2 uv0 = (start + 0.5f) * texelSize;
    float2 uv1 = (start + 1.5f) * texelSize;
    float2 uv2 = (start + 2.5f) * texelSize;
    float2 uv3 = (start + 3.5f) * texelSize;
    
    result += tex.SampleLevel(g_LinearSampler, uv0, 0.0f).rgb * (w0.x * w0.y);
    result += tex.SampleLevel(g_LinearSampler, uv1, 0.0f).rgb * (w1.x * w0.y);
    result += tex.SampleLevel(g_LinearSampler, uv2, 0.0f).rgb * (w2.x * w0.y);
    result += tex.SampleLevel(g_LinearSampler, uv3, 0.0f).rgb * (w3.x * w0.y);
    
    result += tex.SampleLevel(g_LinearSampler, uv0 + float2(0.0f, texelSize.y), 0.0f).rgb * (w0.x * w1.y);
    result += tex.SampleLevel(g_LinearSampler, uv1 + float2(0.0f, texelSize.y), 0.0f).rgb * (w1.x * w1.y);
    result += tex.SampleLevel(g_LinearSampler, uv2 + float2(0.0f, texelSize.y), 0.0f).rgb * (w2.x * w1.y);
    result += tex.SampleLevel(g_LinearSampler, uv3 + float2(0.0f, texelSize.y), 0.0f).rgb * (w3.x * w1.y);
    
    result += tex.SampleLevel(g_LinearSampler, uv0 + float2(0.0f, texelSize.y * 2.0f), 0.0f).rgb * (w0.x * w2.y);
    result += tex.SampleLevel(g_LinearSampler, uv1 + float2(0.0f, texelSize.y * 2.0f), 0.0f).rgb * (w1.x * w2.y);
    result += tex.SampleLevel(g_LinearSampler, uv2 + float2(0.0f, texelSize.y * 2.0f), 0.0f).rgb * (w2.x * w2.y);
    result += tex.SampleLevel(g_LinearSampler, uv3 + float2(0.0f, texelSize.y * 2.0f), 0.0f).rgb * (w3.x * w2.y);
    
    result += tex.SampleLevel(g_LinearSampler, uv0 + float2(0.0f, texelSize.y * 3.0f), 0.0f).rgb * (w0.x * w3.y);
    result += tex.SampleLevel(g_LinearSampler, uv1 + float2(0.0f, texelSize.y * 3.0f), 0.0f).rgb * (w1.x * w3.y);
    result += tex.SampleLevel(g_LinearSampler, uv2 + float2(0.0f, texelSize.y * 3.0f), 0.0f).rgb * (w2.x * w3.y);
    result += tex.SampleLevel(g_LinearSampler, uv3 + float2(0.0f, texelSize.y * 3.0f), 0.0f).rgb * (w3.x * w3.y);
    
    return result;
}

float3 LanczosUpsample(Texture2D<float4> tex, float2 uv, float2 texelSize, float a) {
    float2 texel = uv / texelSize;
    float2 base = floor(texel - a + 0.5f);
    float2 f = texel - base;
    
    float3 result = float3(0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    
    for (int j = 0; j < int(ceil(a * 2.0f)); j++) {
        float y = base.y + float(j);
        float wy = LanczosWeight(y - texel.y, a);
        
        for (int i = 0; i < int(ceil(a * 2.0f)); i++) {
            float x = base.x + float(i);
            float wx = LanczosWeight(x - texel.x, a);
            
            float2 sampleUV = (float2(x, y) + 0.5f) * texelSize;
            float3 color = tex.SampleLevel(g_LinearSampler, sampleUV, 0.0f).rgb;
            
            float weight = wx * wy;
            result += color * weight;
            weightSum += weight;
        }
    }
    
    if (weightSum > 0.0f) {
        result /= weightSum;
    }
    
    return result;
}

float3 AdaptiveUpsample(float2 uv, float2 inputTexelSize) {
    float colorEdge = ComputeEdgeStrength(uv, inputTexelSize);
    float depthEdge = ComputeDepthEdge(uv, inputTexelSize);
    float normalEdge = ComputeNormalEdge(uv, inputTexelSize);
    
    float totalEdge = colorEdge * 0.4f + depthEdge * 0.4f + normalEdge * 0.2f;
    
    float edgeThreshold = g_EdgeThreshold;
    
    if (totalEdge > edgeThreshold) {
        return BicubicUpsample(g_LinearColor, uv, inputTexelSize, 0.5f);
    }
    else if (totalEdge > edgeThreshold * 0.5f) {
        float3 bicubic = BicubicUpsample(g_LinearColor, uv, inputTexelSize, 0.5f);
        float3 bilinear = BilinearUpsample(g_LinearColor, uv, inputTexelSize);
        float blend = saturate((totalEdge - edgeThreshold * 0.5f) / (edgeThreshold * 0.5f));
        return lerp(bilinear, bicubic, blend);
    }
    else {
        return BilinearUpsample(g_LinearColor, uv, inputTexelSize);
    }
}

[numthreads(8, 8, 1)]
void UpsampleCS(uint3 dispatchId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchId.xy;
    if (any(pixelCoord >= g_OutputSize)) return;
    
    float2 outputUV = (float2(pixelCoord) + 0.5f) / g_OutputSize;
    float2 inputUV = outputUV * g_uvToInputScale;
    
    float3 upscaled = AdaptiveUpsample(inputUV, g_inputTexelSize);
    
    float centerLuma = Luminance(upscaled);
    
    float3 left = BilinearUpsample(g_LinearColor, inputUV + float2(-g_inputTexelSize.x, 0.0f), g_inputTexelSize);
    float3 right = BilinearUpsample(g_LinearColor, inputUV + float2(g_inputTexelSize.x, 0.0f), g_inputTexelSize);
    float3 top = BilinearUpsample(g_LinearColor, inputUV + float2(0.0f, -g_inputTexelSize.y), g_inputTexelSize);
    float3 bottom = BilinearUpsample(g_LinearColor, inputUV + float2(0.0f, g_inputTexelSize.y), g_inputTexelSize);
    
    float3 gradX = abs(right - left);
    float3 gradY = abs(bottom - top);
    float gradMag = (gradX.r + gradX.g + gradX.b + gradY.r + gradY.g + gradY.b) / 6.0f;
    
    float localContrast = gradMag / (centerLuma + 0.01f);
    float sharpnessBoost = saturate(localContrast * g_Sharpness);
    
    float3 sharpened = upscaled + (upscaled - (left + right + top + bottom) * 0.25f) * sharpnessBoost * 0.5f;
    
    float contrastMult = 1.0f + (g_ContrastBoost - 1.0f) * sharpnessBoost;
    sharpened = (sharpened - 0.5f) * contrastMult + 0.5f;
    
    g_UpscaledColor[pixelCoord] = float4(saturate(sharpened), 1.0f);
}
