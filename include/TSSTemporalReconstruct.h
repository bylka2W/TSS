#ifndef TSS_TEMPORAL_RECONSTRUCT_H
#define TSS_TEMPORAL_RECONSTRUCT_H

#include "TSSTransform3D.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_RC_LANCZOS_TAPS 4
#define TSS_MAX_CASCADE 4
#define TSS_CONFIDENCE_SCALES 5

typedef struct {
    float currentPosX;
    float currentPosY;
    float currentPosZ;
    float currentDepth;
    
    float historyPosX;
    float historyPosY;
    float historyPosZ;
    float historyDepth;
    
    float mvX;
    float mvY;
    
    float confidence;
    float disocclusion;
    float lumaDiff;
} TSSReconstructParams;

typedef struct {
    float mvX;
    float mvY;
    float depth;
    float prevDepth;
    float reactiveMask;
    float locked;
} TSSPixelData;

typedef struct {
    float r, g, b, a;
} TSSPixelColor;

typedef struct {
    TSSPixelColor history;
    TSSPixelColor current;
    float lumaHistory;
    float lumaCurrent;
    float depthHistory;
    float depthCurrent;
    float mvX, mvY;
    float confidence;
    float disocclusion;
    float lockWeight;
} TSSAccumulation;

typedef enum {
    TSS_RC_STANDARD,
    TSS_RC_DISOCCLUSION,
    TSS_RC_LOCK,
    TSS_RC_REACTIVE
} TSSReconstructMode;

typedef struct {
    float lanczosA;
    float catmullRomAlpha;
    float disocclusionThreshold;
    float lumaClampMin;
    float lumaClampMax;
    float lockThreshold;
    float blendWeight;
    float velocityScale;
    float depthScale;
    bool enableLocking;
    bool enableDisocclusion;
    bool enableReactive;
    bool useYCoCg;
    TSSReconstructMode mode;
} TSSRCASConfig;

typedef struct {
    float historyLuma[TSS_MAX_CASCADE];
    float historyContrast[TSS_MAX_CASCADE];
    float historyStructure[TSS_MAX_CASCADE];
    float cascadeWeights[TSS_MAX_CASCADE];
    int cascadeCount;
} TSSRCASCascades;

typedef struct {
    TSSRCASConfig config;
    TSSRCASCascades cascades;
    
    float lanczosWeights[TSS_RC_LANCZOS_TAPS * 2];
    float catmullRomWeights[4];
    
    float lockMapResolution;
    float lockDecayRate;
    float lockBoostAmount;
    
    float disocclusionScale;
    float reactiveScale;
} TSSTemporalReconstruct;

TSSTemporalReconstruct* TSSRC_Create(void);
void TSSRC_Destroy(TSSTemporalReconstruct* rc);

void TSSRC_SetConfig(TSSTemporalReconstruct* rc, TSSRCASConfig* config);

void TSSRC_InitializeWeights(TSSTemporalReconstruct* rc);

float TSSRC_LanczosWeight(float x, float a);
float TSSRC_CatmullRomWeight(float x);
float TSSRC_LanczosSample(
    const float* src,
    int width,
    int height,
    int stride,
    float u,
    float v
);

float TSSRC_CatmullRomSample(
    const float* src,
    int width,
    int height,
    int stride,
    float u,
    float v
);

void TSSRC_DisocclusionCheck(
    float currentDepth,
    float historyDepth,
    float mvX,
    float mvY,
    float* outDisocclusion,
    float* outConfidence
);

void TSSRC_LockCheck(
    const float* currentLuma,
    int width,
    int height,
    int x,
    int y,
    float threshold,
    float* outLockWeight
);

void TSSRC_ColorClampYCoCg(
    const float* neighborhood,
    float centerLuma,
    float centerCo,
    float centerCg,
    float minDiff,
    float maxDiff,
    float* outCo,
    float* outCg
);

void TSSRC_ColorClampRGB(
    const float* neighborhood,
    float* rgb,
    float scale
);

TSSAccumulation TSSRC_Accumulate(
    const float* historyColor,
    const float* currentColor,
    float mvX,
    float mvY,
    float currentDepth,
    float historyDepth,
    float alpha,
    TSSTemporalReconstruct* rc
);

float TSSRC_CalculateLuma(const float* rgb);
void TSSRC_RGBToYCoCg(float r, float g, float b, float* y, float* co, float* cg);
void TSSRC_YCoCgToRGB(float y, float co, float cg, float* r, float* g, float* b);

void TSSRC_ApplyReactiveMask(
    const float* reactiveMask,
    float* blendWeight,
    float scale
);

float TSSRC_CalculateConfidence(
    float disocclusion,
    float lumaDiff,
    float mvMagnitude,
    float lockWeight
);

void TSSRC_WarpHistory(
    const float* historyColor,
    int historyWidth,
    int historyHeight,
    int historyStride,
    float reprojectX,
    float reprojectY,
    float* outWarped
);

typedef struct {
    float exposure;
    float avgLuma;
    float sceneLuma;
    float prevSceneLuma;
    float adaptationRate;
} TSSExposureFeedback;

TSSExposureFeedback* TSSRC_CreateExposureFeedback(float adaptationRate);
void TSSRC_DestroyExposureFeedback(TSSExposureFeedback* fb);
void TSSRC_UpdateExposure(TSSExposureFeedback* fb, const float* src, int width, int height);
float TSSRC_GetExposureScale(TSSExposureFeedback* fb);

#ifdef __cplusplus
}
#endif

#endif
