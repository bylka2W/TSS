#include "TSSSIMDInterpolation.h"
#include <stdlib.h>
#include <string.h>

#define TSS_SIMD_ALIGNMENT 32

static void* TSSSIMDAlignedAlloc(size_t size) {
    void* ptr = _aligned_malloc(size, TSS_SIMD_ALIGNMENT);
    return ptr;
}

static void TSSSIMDAlignedFree(void* ptr) {
    if (ptr) _aligned_free(ptr);
}

TSSSIMDInterpolator* TSSSIMDCreate(unsigned int maxEntities) {
    TSSSIMDInterpolator* interp = (TSSSIMDInterpolator*)malloc(sizeof(TSSSIMDInterpolator));
    if (!interp) return NULL;
    
    memset(interp, 0, sizeof(TSSSIMDInterpolator));
    
    interp->capacity = maxEntities;
    
    interp->positionsX = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->positionsY = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->positionsZ = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    
    interp->velocitiesX = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->velocitiesY = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->velocitiesZ = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    
    interp->prevPositionsX = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->prevPositionsY = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->prevPositionsZ = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    
    interp->alphas = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    
    interp->outputsX = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->outputsY = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    interp->outputsZ = (float*)TSSSIMDAlignedAlloc(maxEntities * sizeof(float));
    
    if (!interp->positionsX || !interp->positionsY || !interp->positionsZ ||
        !interp->velocitiesX || !interp->velocitiesY || !interp->velocitiesZ ||
        !interp->prevPositionsX || !interp->prevPositionsY || !interp->prevPositionsZ ||
        !interp->alphas || !interp->outputsX || !interp->outputsY || !interp->outputsZ) {
        TSSSIMDDestroy(interp);
        return NULL;
    }
    
    interp->count = 0;
    
    return interp;
}

void TSSSIMDDestroy(TSSSIMDInterpolator* interp) {
    if (!interp) return;
    
    TSSSIMDAlignedFree(interp->positionsX);
    TSSSIMDAlignedFree(interp->positionsY);
    TSSSIMDAlignedFree(interp->positionsZ);
    
    TSSSIMDAlignedFree(interp->velocitiesX);
    TSSSIMDAlignedFree(interp->velocitiesY);
    TSSSIMDAlignedFree(interp->velocitiesZ);
    
    TSSSIMDAlignedFree(interp->prevPositionsX);
    TSSSIMDAlignedFree(interp->prevPositionsY);
    TSSSIMDAlignedFree(interp->prevPositionsZ);
    
    TSSSIMDAlignedFree(interp->alphas);
    
    TSSSIMDAlignedFree(interp->outputsX);
    TSSSIMDAlignedFree(interp->outputsY);
    TSSSIMDAlignedFree(interp->outputsZ);
    
    free(interp);
}

void TSSSIMDAddEntity(TSSSIMDInterpolator* interp, float posX, float posY, float posZ,
                      float velX, float velY, float velZ,
                      float prevX, float prevY, float prevZ) {
    if (!interp || interp->count >= interp->capacity) return;
    
    unsigned int idx = interp->count++;
    
    interp->positionsX[idx] = posX;
    interp->positionsY[idx] = posY;
    interp->positionsZ[idx] = posZ;
    
    interp->velocitiesX[idx] = velX;
    interp->velocitiesY[idx] = velY;
    interp->velocitiesZ[idx] = velZ;
    
    interp->prevPositionsX[idx] = prevX;
    interp->prevPositionsY[idx] = prevY;
    interp->prevPositionsZ[idx] = prevZ;
    
    interp->alphas[idx] = 0.0f;
}

void TSSSIMDInterpolate(TSSSIMDInterpolator* interp, float baseAlpha, float velocityInfluence) {
    if (!interp) return;
    
    unsigned int i;
    for (i = 0; i < interp->count; i++) {
        float dx = interp->positionsX[i] - interp->prevPositionsX[i];
        float dy = interp->positionsY[i] - interp->prevPositionsY[i];
        float dz = interp->positionsZ[i] - interp->prevPositionsZ[i];
        
        float velocityMag = sqrtf(dx * dx + dy * dy + dz * dz);
        float alpha = baseAlpha + velocityMag * velocityInfluence;
        alpha = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
        
        interp->outputsX[i] = interp->positionsX[i] - dx * alpha;
        interp->outputsY[i] = interp->positionsY[i] - dy * alpha;
        interp->outputsZ[i] = interp->positionsZ[i] - dz * alpha;
        
        interp->alphas[i] = alpha;
    }
}

#if defined(__AVX2__) || defined(_M_X64) || defined(_M_AMD64)

void TSSSIMDInterpolateAVX2(TSSSIMDInterpolator* interp, float baseAlpha, float velocityInfluence) {
    if (!interp) return;
    
    __m256 baseAlphaVec = _mm256_set1_ps(baseAlpha);
    __m256 velocityInfluenceVec = _mm256_set1_ps(velocityInfluence);
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    
    unsigned int i = 0;
    unsigned int simdWidth = 8;
    
    for (; i + simdWidth <= interp->count; i += simdWidth) {
        __m256 posX = _mm256_load_ps(&interp->positionsX[i]);
        __m256 posY = _mm256_load_ps(&interp->positionsY[i]);
        __m256 posZ = _mm256_load_ps(&interp->positionsZ[i]);
        
        __m256 prevX = _mm256_load_ps(&interp->prevPositionsX[i]);
        __m256 prevY = _mm256_load_ps(&interp->prevPositionsY[i]);
        __m256 prevZ = _mm256_load_ps(&interp->prevPositionsZ[i]);
        
        __m256 dx = _mm256_sub_ps(posX, prevX);
        __m256 dy = _mm256_sub_ps(posY, prevY);
        __m256 dz = _mm256_sub_ps(posZ, prevZ);
        
        __m256 velocitySq = _mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy));
        velocitySq = _mm256_add_ps(velocitySq, _mm256_mul_ps(dz, dz));
        
        __m256 velocityMag = _mm256_sqrt_ps(velocitySq);
        
        __m256 alpha = _mm256_fmadd_ps(velocityMag, velocityInfluenceVec, baseAlphaVec);
        alpha = _mm256_max_ps(zero, alpha);
        alpha = _mm256_min_ps(one, alpha);
        
        _mm256_store_ps(&interp->outputsX[i], _mm256_sub_ps(posX, _mm256_mul_ps(dx, alpha)));
        _mm256_store_ps(&interp->outputsY[i], _mm256_sub_ps(posY, _mm256_mul_ps(dy, alpha)));
        _mm256_store_ps(&interp->outputsZ[i], _mm256_sub_ps(posZ, _mm256_mul_ps(dz, alpha)));
        
        _mm256_store_ps(&interp->alphas[i], alpha);
    }
    
    for (; i < interp->count; i++) {
        float dx = interp->positionsX[i] - interp->prevPositionsX[i];
        float dy = interp->positionsY[i] - interp->prevPositionsY[i];
        float dz = interp->positionsZ[i] - interp->prevPositionsZ[i];
        
        float velocityMag = sqrtf(dx * dx + dy * dy + dz * dz);
        float alpha = baseAlpha + velocityMag * velocityInfluence;
        alpha = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
        
        interp->outputsX[i] = interp->positionsX[i] - dx * alpha;
        interp->outputsY[i] = interp->positionsY[i] - dy * alpha;
        interp->outputsZ[i] = interp->positionsZ[i] - dz * alpha;
        
        interp->alphas[i] = alpha;
    }
}

#else

void TSSSIMDInterpolateAVX2(TSSSIMDInterpolator* interp, float baseAlpha, float velocityInfluence) {
    TSSSIMDInterpolate(interp, baseAlpha, velocityInfluence);
}

#endif

void TSSSIMDGetPosition(TSSSIMDInterpolator* interp, unsigned int index, float* outX, float* outY, float* outZ) {
    if (!interp || index >= interp->count) return;
    
    if (outX) *outX = interp->outputsX[index];
    if (outY) *outY = interp->outputsY[index];
    if (outZ) *outZ = interp->outputsZ[index];
}

void TSSSIMDUpdateVelocities(TSSSIMDInterpolator* interp, unsigned int index, float velX, float velY, float velZ) {
    if (!interp || index >= interp->count) return;
    
    interp->velocitiesX[index] = velX;
    interp->velocitiesY[index] = velY;
    interp->velocitiesZ[index] = velZ;
}

void TSSSIMDClear(TSSSIMDInterpolator* interp) {
    if (!interp) return;
    interp->count = 0;
}

unsigned int TSSSIMDGetCount(TSSSIMDInterpolator* interp) {
    return interp ? interp->count : 0;
}
