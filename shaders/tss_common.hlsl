#define TSS_COMMON_VERSION "1.0.0"

#ifndef TSS_HLSL
#define TSS_HLSL

#define TSS_PI 3.14159265f
#define TSS_LN2 0.69314718f
#define TSS_EPSILON 1e-6f
#define TSS_WAVE_SIZE 32
#define TSS_LDS_SIZE 16

SamplerState g_PointSampler : register(s0);
SamplerState g_LinearSampler : register(s1);
SamplerComparisonState g_DepthSampler : register(s2);

cbuffer TSSGlobalConstants : register(b0) {
    uint2 g_Resolution;
    uint2 g_InputResolution;
    float g_JitterX;
    float g_JitterY;
    float g_Sharpness;
    float g_TimeDelta;
    float g_FrameIndex;
    float2 g_texelSize;
    float2 g_inputTexelSize;
    float2 g_uvToInputScale;
};

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

float3 RGBToYCoCg_Improved(float3 rgb) {
    float tmp = 0.5f * (rgb.r + rgb.b);
    float y = 0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b;
    float co = rgb.r - tmp;
    float cg = tmp - rgb.g + tmp - rgb.b;
    return float3(y, co, cg);
}

float3 YCoCgToRGB_Improved(float3 ycocg) {
    float y = ycocg.x;
    float co = ycocg.y;
    float cg = ycocg.z;
    float tmp = y - cg;
    float r = tmp + co;
    float g = y + cg;
    float b = tmp - co;
    return float3(r, g, b);
}

float Luminance(float3 color) {
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float3x3 PackNormal(float3 normal) {
    return float3x3(
        normal.x * 0.5f + 0.5f,
        normal.y * 0.5f + 0.5f,
        normal.z * 0.5f + 0.5f
    );
}

float3 UnpackNormal(float3 packed) {
    return float3(
        packed.x * 2.0f - 1.0f,
        packed.y * 2.0f - 1.0f,
        packed.z * 2.0f - 1.0f
    );
}

float3 EncodeNormal(float3 n) {
    return n * 0.5f + 0.5f;
}

float3 DecodeNormal(float3 n) {
    return n * 2.0f - 1.0f;
}

float LinearizeDepth(float rawDepth, float near, float far) {
    float z = rawDepth * 2.0f - 1.0f;
    return (2.0f * near * far) / (far + near - z * (far - near));
}

float PackDepth(float linearDepth, float near, float far) {
    float a = far / (far - near);
    float b = far * near / (near - far);
    float z = linearDepth;
    return (a * z + b) / z;
}

float3 RGBToYUV(float3 rgb) {
    float y = 0.299f * rgb.r + 0.587f * rgb.g + 0.114f * rgb.b;
    float u = -0.147f * rgb.r - 0.289f * rgb.g + 0.436f * rgb.b;
    float v = 0.615f * rgb.r - 0.515f * rgb.g - 0.100f * rgb.b;
    return float3(y, u, v);
}

float3 YUVToRGB(float3 yuv) {
    float r = yuv.x + 1.14f * yuv.z;
    float g = yuv.x - 0.395f * yuv.y - 0.581f * yuv.z;
    float b = yuv.x + 2.033f * yuv.y;
    return float3(r, g, b);
}

float SmoothMax(float a, float b, float k) {
    k = max(k, 0.0001f);
    return a - (a - b) / k + k * 0.25f;
}

float SmoothMin(float a, float b, float k) {
    return -SmoothMax(-a, -b, k);
}

float SoftMax(float a, float b, float k) {
    return log2(exp2(k * a) + exp2(k * b)) / k;
}

float SoftMin(float a, float b, float k) {
    return -SoftMax(-a, -b, k);
}

float FilmicReinhardCurve(float x) {
    float q = (x + 0.001f) / (x + 1.0f);
    return q * q;
}

float3 FilmicReinhardCurve3(float3 x) {
    return float3(
        FilmicReinhardCurve(x.r),
        FilmicReinhardCurve(x.g),
        FilmicReinhardCurve(x.b)
    );
}

float FilmicATC(float x) {
    float k = 0.08f;
    float a = 0.63f;
    float b = 0.07f;
    return (x * (x + k)) / (x * (a + k) + b);
}

float ApproximatePow2(float x, float k) {
    float l = x * k;
    float x2 = x * x;
    return lerp(x2, exp2(l), saturate(l));
}

float FastMax3(float3 v) {
    return max(max(v.x, v.y), v.z);
}

float FastMin3(float3 v) {
    return min(min(v.x, v.y), v.z);
}

float FastMax4(float4 v) {
    return max(max(v.x, v.y), max(v.z, v.w));
}

float FastMin4(float4 v) {
    return min(min(v.x, v.y), min(v.z, v.w));
}

#endif
