#ifndef TSS_VARIANCE_CLIPPING_H
#define TSS_VARIANCE_CLIPPING_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kSigma;
    float minVarianceWeight;
    float maxVarianceWeight;
    bool enableAdaptiveK;
    float adaptiveThreshold;
} TSSVarianceClippingConfig;

typedef struct {
    float mean;
    float variance;
    float stdDev;
    float min;
    float max;
    float range;
} TSSVarianceStats;

typedef struct {
    float y;
    float co;
    float cg;
} TSSYCoCg;

void TSSVC_ConfigDefault(TSSVarianceClippingConfig* config);

TSSVarianceStats TSSVC_CalculateStats(const float* samples, int count);

float TSSVC_ClampValue(float value, TSSVarianceStats* stats, float kSigma);

void TSSVC_ClampRGB(float* color, const float* neighborhood, int sampleCount, float kSigma);

TSSYCoCg TSSVC_RGBToYCoCg(float r, float g, float b);
void TSSVC_YCoCgToRGB(TSSYCoCg ycocg, float* outR, float* outG, float* outB);

void TSSVC_ClampYCoCg(
    TSSYCoCg* history,
    const TSSYCoCg* neighborhood,
    int sampleCount,
    float kSigmaY,
    float kSigmaCo,
    float kSigmaCg
);

float TSSVC_CalculateLuma(const float* rgb);

typedef struct {
    float historyY;
    float historyCo;
    float historyCg;
    float currentY;
    float currentCo;
    float currentCg;
    float confidence;
    float disocclusion;
} TSSVCAccumulation;

TSSVCAccumulation TSSVC_Accumulate(
    const float* historyRGB,
    const float* currentRGB,
    const float* neighborhood,
    int neighborhoodCount,
    float mvX,
    float mvY,
    float currentDepth,
    float historyDepth,
    TSSVarianceClippingConfig* config
);

#ifdef __cplusplus
}
#endif

#endif
