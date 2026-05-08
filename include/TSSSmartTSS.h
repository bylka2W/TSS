#ifndef TSS_SMART_TSS_H
#define TSS_SMART_TSS_H

#include "TSSTransform3D.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_SMART_VERSION "1.0.0"

typedef enum {
    TSS_WEIGHT_MODE_FIXED,
    TSS_WEIGHT_MODE_VARIANCE,
    TSS_WEIGHT_MODE_NEURAL,
    TSS_WEIGHT_MODE_HYBRID
} TSSWeightMode;

typedef struct {
    TSSWeightMode mode;
    float kSigma;
    float minAlpha;
    float maxAlpha;
    float velocityScale;
    float disocclusionThreshold;
    float confidenceThreshold;
    float neuralBlendStrength;
    int enableYCoCg;
    int enableDepthTest;
    int enableAsyncCompute;
    int enableNegativeLOD;
} TSSSmartConfig;

typedef struct {
    float historyY;
    float historyCo;
    float historyCg;
    float confidence;
    float disocclusion;
    float weight;
    int stabilityFrames;
} TSSSmartAccumulation;

typedef struct {
    float disocclusionMask;
    float velocityDivergence;
    float lumaInstability;
    float edgeConfidence;
    float totalConfidence;
} TSSConfidenceMasks;

typedef struct {
    float r, g, b;
} TSSPixelColor;

typedef struct {
    float y, co, cg;
} TSSYCoCgColor;

typedef struct {
    TSSVec3 position;
    float depth;
    TSSVec2 motionVector;
    float confidence;
} TSSProjectedPixel;

TSSSmartConfig TSSSmart_DefaultConfig(void);

void TSSSmart_Init(TSSSmartConfig* config);

TSSYCoCgColor TSSSmart_RGBToYCoCg(TSSPixelColor rgb);

TSSPixelColor TSSSmart_YCoCgToRGB(TSSYCoCgColor ycocg);

float TSSSmart_CalculateLuma(TSSPixelColor rgb);

float TSSSmart_CalculateLumaYCoCg(TSSYCoCgColor ycocg);

TSSConfidenceMasks TSSSmart_GenerateConfidenceMasks(
    float currentDepth,
    float historyDepth,
    TSSVec2 currentMV,
    TSSVec2 historyMV,
    float currentLuma,
    float historyLuma,
    TSSSmartConfig* config
);

float TSSSmart_CalculateDisocclusionMask(
    float currentDepth,
    float historyDepth,
    float mvMagnitude,
    float threshold
);

float TSSSmart_CalculateVelocityDivergence(
    TSSVec2 mv,
    TSSVec2 neighborMVs[8],
    int neighborCount
);

float TSSSmart_CalculateLumaInstability(
    float currentLuma,
    float historyLuma,
    float neighborhoodStdDev
);

float TSSSmart_NeuralWeightArbitrator(
    float disocclusionMask,
    float velocityDivergence,
    float lumaInstability,
    float mvMagnitude,
    int stabilityFrames,
    TSSSmartConfig* config
);

TSSSmartAccumulation TSSSmart_Accumulate(
    TSSPixelColor currentColor,
    TSSPixelColor historyColor,
    TSSPixelColor neighborhood[9],
    TSSVec2 motionVector,
    float currentDepth,
    float historyDepth,
    int stabilityFrames,
    TSSSmartConfig* config
);

float TSSSmart_DepthTestedSplatting(
    TSSProjectedPixel* pixels,
    int pixelCount,
    float outputDepth,
    TSSVec2 outputUV
);

TSSPixelColor TSSSmart_HoleFilling(
    TSSPixelColor colors[9],
    float depths[9],
    TSSVec2 uv,
    float centerDepth
);

float TSSSmart_VarianceClipping(
    float historyValue,
    float currentValue,
    float mean,
    float stdDev,
    float kSigma
);

void TSSSmart_YCoCgClamping(
    TSSYCoCgColor* history,
    TSSYCoCgColor neighbors[9],
    float kY,
    float kCo,
    float kCg
);

typedef struct {
    float alpha;
    float weight;
    float confidence;
} TSSSmartWeight;

TSSSmartWeight TSSSmart_CalculateSmartWeight(
    float disocclusion,
    float velocityMag,
    float lumaDiff,
    int framesSinceChange,
    TSSSmartConfig* config
);

float TSSSmart_AdaptiveSharpness(
    float centerLuma,
    float neighborLuma,
    float edgeThreshold
);

typedef struct {
    float inputLag_ms;
    float gpuTime_ms;
    float totalTime_ms;
    int passes;
    int vrsmReads;
    int vrsmWrites;
} TSSSmartStats;

void TSSSmart_UpdateStats(TSSSmartStats* stats, float deltaTime_ms);

#ifdef __cplusplus
}
#endif

#endif
