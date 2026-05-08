#ifndef TSS_SIMD_INTERPOLATION_H
#define TSS_SIMD_INTERPOLATION_H

#include "TSSTransform3D.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_SIMD_VERSION "1.0.0"

#if defined(__AVX2__) || defined(_M_X64) || defined(_M_AMD64)
#define TSS_USE_AVX2 1
#include <immintrin.h>
#else
#define TSS_USE_AVX2 0
#endif

typedef struct {
    float* positionsX;
    float* positionsY;
    float* positionsZ;
    float* velocitiesX;
    float* velocitiesY;
    float* velocitiesZ;
    float* prevPositionsX;
    float* prevPositionsY;
    float* prevPositionsZ;
    float* alphas;
    float* outputsX;
    float* outputsY;
    float* outputsZ;
    unsigned int count;
    unsigned int capacity;
} TSSSIMDInterpolator;

TSSSIMDInterpolator* TSSSIMDCreate(unsigned int maxEntities);
void TSSSIMDDestroy(TSSSIMDInterpolator* interp);

void TSSSIMDAddEntity(TSSSIMDInterpolator* interp, float posX, float posY, float posZ,
                      float velX, float velY, float velZ,
                      float prevX, float prevY, float prevZ);

void TSSSIMDInterpolate(TSSSIMDInterpolator* interp, float baseAlpha, float velocityInfluence);

void TSSSIMDInterpolateAVX2(TSSSIMDInterpolator* interp, float baseAlpha, float velocityInfluence);

void TSSSIMDGetPosition(TSSSIMDInterpolator* interp, unsigned int index, float* outX, float* outY, float* outZ);

void TSSSIMDUpdateVelocities(TSSSIMDInterpolator* interp, unsigned int index, float velX, float velY, float velZ);

void TSSSIMDClear(TSSSIMDInterpolator* interp);

unsigned int TSSSIMDGetCount(TSSSIMDInterpolator* interp);

#ifdef __cplusplus
}
#endif

#endif
