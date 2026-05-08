#ifndef TSS_ANTI_ALIASING_H
#define TSS_ANTI_ALIASING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_AA_MAX_SAMPLES 8
#define TSS_AA_JITTER_COUNT 16

typedef struct {
    float subpixelOffsetX;
    float subpixelOffsetY;
    float mipBias;
    float sharpenAmount;
} TSSAAConfig;

typedef struct {
    float jitterX[TSS_AA_JITTER_COUNT];
    float jitterY[TSS_AA_JITTER_COUNT];
    int currentIndex;
    bool enable;
    bool useTemporal;
    float colorWeight;
    float spatialWeight;
    float centerWeight;
} TSSTemporalAA;

typedef struct {
    float weights[3][3];
    float offsetX[3][3];
    float offsetY[3][3];
    int radius;
} TSSFXAAConfig;

typedef struct {
    float positions[2];
    float weights[2];
    float mipOffsets[2];
} TSSSMAAConfig;

typedef enum {
    TSS_AA_NONE = 0,
    TSS_AA_FXAA,
    TSS_AA_TAA,
    TSS_AA_SMAA,
    TSS_AA_TSSAA
} TSSAAType;

typedef struct {
    TSSAAConfig config;
    TSSAAType type;
    TSSTemporalAA taa;
    TSSFXAAConfig fxaa;
    TSSSMAAConfig smaa;
} TSSAntiAliasing;

TSSAntiAliasing* TSSAA_Create(TSSAAType type);
void TSSAA_Destroy(TSSAntiAliasing* aa);

void TSSAA_SetConfig(TSSAntiAliasing* aa, TSSAAConfig* config);
void TSSAA_GetJitter(TSSAntiAliasing* aa, float* outX, float* outY);

void TSSAA_ApplyFXAA(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int pitch,
    TSSFXAAConfig* config
);

void TSSAA_ApplyFXAA_Float(
    const float* src,
    float* dst,
    int width,
    int height,
    int pitch,
    TSSFXAAConfig* config
);

typedef struct {
    float r, g, b, a;
} TSSColor;

TSSColor TSSAA_ApplyTAASample(
    TSSColor* samples,
    int count,
    float centerWeight
);

typedef struct {
    float luma;
    float edgeThreshold;
    float subpixelBlend;
} TSSFXAAResult;

float TSSAA_CalculateLuma(TSSColor color);
float TSSAA_CalculateEdgeStrength(float lumaCenter, float lumaN, float lumaS, float lumaE, float lumaW);

void TSSAA_GenerateJitterPattern(
    float* outX,
    float* outY,
    int count,
    int width,
    int height
);

void TSSAA_Sharpen(
    const float* src,
    float* dst,
    int width,
    int height,
    int pitch,
    float amount
);

float TSSAA_CalculateSharpenWeight(
    float center,
    float neighbor,
    float amount
);

typedef struct {
    float exposure;
    float contrast;
    float highlights;
    float shadows;
    float saturation;
} TSSPostProcess;

void TSSPostProcess_Apply(
    const float* src,
    float* dst,
    int width,
    int height,
    TSSPostProcess* pp
);

#ifdef __cplusplus
}
#endif

#endif
