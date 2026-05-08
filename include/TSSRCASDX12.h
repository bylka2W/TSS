#ifndef TSS_RCAS_DX12_H
#define TSS_RCAS_DX12_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_RCAS_VERSION "1.0.0"
#define TSS_RCAS_HALF 1
#define TSS_RCAS_MAX_TILES 16

typedef struct {
    float sharpStrength;
    float contrastThreshold;
    float diapasonK;
    float diapasonE;
} TSSRCASConfig;

typedef struct {
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;
    float scale;
    TSSRCASConfig config;
    bool enableAntiRinging;
    bool enableVarianceClipping;
    bool enableAdaptiveSharpening;
} TSSRCASInitParams;

typedef struct {
    float output[4];
} TSSRCASOutput;

typedef struct {
    float minColor[3];
    float maxColor[3];
    float avgColor[3];
    float variance;
    float stdDev;
} TSSRCASNeighborStats;

void TSSRCAS_ProcessTile(
    const void* currentFrame,
    const void* historyFrame,
    const void* motionVectors,
    const void* depthBuffer,
    void* outputBuffer,
    uint32_t tileX,
    uint32_t tileY,
    uint32_t tileWidth,
    uint32_t tileHeight,
    TSSRCASInitParams* params
);

void TSSRCAS_ProcessFrame(
    const void* currentFrame,
    const void* historyFrame,
    const void* motionVectors,
    const void* depthBuffer,
    const void* reactiveMask,
    const void* lockMap,
    void* outputBuffer,
    TSSRCASInitParams* params
);

void TSSRCAS_DepthBasedRejection(
    float currentDepth,
    float historyDepth,
    float mvX,
    float mvY,
    float threshold,
    float* outRejectWeight
);

void TSSRCAS_NeighborhoodAABBClip(
    const float* neighborhood9,
    float* historyColor,
    float clipScale
);

void TSSRCAS_VarianceClipping(
    const float* neighborhood9,
    float* historyColor,
    float kSigma
);

void TSSRCAS_LumaWeightedBlend(
    float historyLuma,
    float currentLuma,
    float historyWeight,
    float lumaThreshold,
    float* outFinalWeight
);

void TSSRCAS_ExposureCompensation(
    float* historyColor,
    float currentExposure,
    float historyExposure
);

float TSSRCAS_AdaptiveSharpen(
    const float* center,
    const float* neighbors,
    float contrastThreshold,
    float maxSharpness
);

void TSSRCAS_AntiRinging(
    float* color,
    const float* neighborhoodMin,
    const float* neighborhoodMax
);

float TSSRCAS_CalculateVariance(const float* samples, int count, float mean);
float TSSRCAS_CalculateStdDev(float variance);

void TSSRCAS_PackMotionVector(float mvX, float mvY, uint16_t* outPacked);
void TSSRCAS_UnpackMotionVector(uint16_t packed, float* outMVX, float* outMVY);

typedef struct {
    float alphaEMA;
    float alphaMin;
    float alphaMax;
    float velocityScale;
} TSSRCASEMAConfig;

float TSSRCAS_CalculateEMAWeight(
    float velocityMagnitude,
    TSSRCASEMAConfig* config
);

typedef struct {
    uint8_t historyCount;
    uint8_t stabilityThreshold;
    float convergenceRate;
    float divergenceThreshold;
} TSSRCASAccumulationCounter;

void TSSRCAS_UpdateAccumulationCounter(
    TSSRCASAccumulationCounter* counter,
    float lumaDiff,
    float mvMagnitude,
    bool isStable
);

float TSSRCAS_GetAccumulationWeight(TSSRCASAccumulationCounter* counter);

typedef struct {
    float exposure;
    float prevExposure;
    float avgLuma;
    float adaptationRate;
} TSSRCASExposureState;

void TSSRCAS_InitExposureState(TSSRCASExposureState* state, float adaptationRate);
void TSSRCAS_UpdateExposure(TSSRCASExposureState* state, const void* frame, uint32_t width, uint32_t height);
float TSSRCAS_GetExposureScale(TSSRCASExposureState* state);

#ifdef __cplusplus
}
#endif

#endif
