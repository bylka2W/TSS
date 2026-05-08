#ifndef TSS_GPU_WAVE_INTRINSICS_H
#define TSS_GPU_WAVE_INTRINSICS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__HLSL_VERSION) || defined(__cplusplus_winrt)
#define TSS_WAVE_AVAILABLE 1
#else
#define TSS_WAVE_AVAILABLE 0
#endif

typedef enum {
    TSS_WAVE_SIZE_32 = 32,
    TSS_WAVE_SIZE_64 = 64
} TSSWaveSize;

typedef struct {
    float minLuma;
    float maxLuma;
    float avgLuma;
    float sumLuma;
    float variance;
    float edgeStrength;
    float localContrast;
} TSSWaveLumaStats;

typedef struct {
    float minDepth;
    float maxDepth;
    float avgDepth;
    float depthRange;
    float disocclusionAmount;
} TSSWaveDepthStats;

typedef struct {
    int activeCount;
    int laneCount;
    float fillRate;
} TSSWaveActiveInfo;

typedef struct {
    float horizontalGradient;
    float verticalGradient;
    float gradientMagnitude;
    float gradientDirection;
    float edgeScore;
} TSSWaveEdgeStats;

TSSWaveLumaStats TSSWaveReduceLuma(
    float* lumaValues,
    int laneCount
);

TSSWaveDepthStats TSSWaveReduceDepth(
    float* depthValues,
    int laneCount
);

float TSSWavePrefixSum(float value, int laneIndex);

float TSSWavePrefixProduct(float value, int laneIndex);

float TSSWaveBallotAny(bool condition);

float TSSWaveBallotAll(bool condition);

int TSSWaveMatch(float value);

float TSSWaveBroadcast(float value, int laneIndex);

float TSSWaveShuffle(float value, int fromLane, int toLane);

float TSSWaveShuffleUp(float value, int delta);

float TSSWaveShuffleDown(float value, int delta);

float TSSWaveShuffleXor(float value, int mask);

TSSWaveActiveInfo TSSWaveGetActiveInfo(int laneIndex);

float TSSWaveCountBits(bool* bits, int count);

void TSSWaveQuadSwap(float* value, int direction);

void TSSWaveOctetSwap(float* value, int direction);

typedef struct {
    float minValue;
    float maxValue;
    float sumValue;
    float avgValue;
    int minIndex;
    int maxIndex;
} TSSWaveReduceResult;

TSSWaveReduceResult TSSWaveReduceMinMaxSum(
    float* values,
    int count
);

float TSSWaveLerp(float a, float b, float t);

float TSSWaveSmoothStep(float edge0, float edge1, float x);

float TSSWaveClamp(float x, float minVal, float maxVal);

float TSSWaveSaturation(float x);

float TSSWaveMad(float a, float b, float c);

float TSSWaveLerpClamped(float a, float b, float t);

typedef struct {
    float bilateralWeight;
    float spatialWeight;
    float rangeWeight;
    float totalWeight;
} TSSWaveBilateralResult;

TSSWaveBilateralResult TSSWaveBilateralFilter(
    float centerValue,
    float centerLuma,
    float* neighborValues,
    float* neighborLumas,
    float sigmaSpatial,
    float sigmaRange,
    int neighborCount
);

void TSSWaveMinMax3x3(
    float* values,
    int x,
    int y,
    int width,
    int height,
    float* outMin,
    float* outMax
);

float TSSWaveMedian3(float a, float b, float c);

float TSSWaveMedian5(float a, float b, float c, float d, float e);

float TSSWaveEdgeAwareInterpolate(
    float p0,
    float p1,
    float gradientH,
    float gradientV,
    float threshold
);

float TSSWaveStructurallyAwareSample(
    float center,
    float left,
    float right,
    float top,
    float bottom,
    float diagonalTL,
    float diagonalTR,
    float diagonalBL,
    float diagonalBR,
    float edgeThreshold
);

#ifdef __cplusplus
}
#endif

#endif
