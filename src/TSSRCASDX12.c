#include "TSSRCASDX12.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

void TSSRCAS_DepthBasedRejection(
    float currentDepth,
    float historyDepth,
    float mvX,
    float mvY,
    float threshold,
    float* outRejectWeight
) {
    float depthDiff = fabsf(currentDepth - historyDepth);
    
    float mvMagnitude = sqrtf(mvX * mvX + mvY * mvY);
    
    float rejectWeight = 0.0f;
    
    if (depthDiff > threshold) {
        rejectWeight = fminf(depthDiff * 100.0f, 1.0f);
    }
    
    float mvPenalty = fminf(mvMagnitude * 0.1f, 0.5f);
    rejectWeight = fmaxf(rejectWeight, mvPenalty);
    
    if (outRejectWeight) {
        *outRejectWeight = rejectWeight;
    }
}

void TSSRCAS_NeighborhoodAABBClip(
    const float* neighborhood9,
    float* historyColor,
    float clipScale
) {
    if (!neighborhood9 || !historyColor) return;
    
    float minR = neighborhood9[0], maxR = neighborhood9[0];
    float minG = neighborhood9[1], maxG = neighborhood9[1];
    float minB = neighborhood9[2], maxB = neighborhood9[2];
    
    int i;
    for (i = 1; i < 9; i++) {
        float r = neighborhood9[i * 3 + 0];
        float g = neighborhood9[i * 3 + 1];
        float b = neighborhood9[i * 3 + 2];
        
        minR = fminf(minR, r);
        maxR = fmaxf(maxR, r);
        minG = fminf(minG, g);
        maxG = fmaxf(maxG, g);
        minB = fminf(minB, b);
        maxB = fmaxf(maxB, b);
    }
    
    float rangeR = (maxR - minR) * clipScale;
    float rangeG = (maxG - minG) * clipScale;
    float rangeB = (maxB - minB) * clipScale;
    
    historyColor[0] = fmaxf(minR - rangeR, fminf(maxR + rangeR, historyColor[0]));
    historyColor[1] = fmaxf(minG - rangeG, fminf(maxG + rangeG, historyColor[1]));
    historyColor[2] = fmaxf(minB - rangeB, fminf(maxB + rangeB, historyColor[2]));
}

void TSSRCAS_VarianceClipping(
    const float* neighborhood9,
    float* historyColor,
    float kSigma
) {
    if (!neighborhood9 || !historyColor) return;
    
    float sumR = 0, sumG = 0, sumB = 0;
    float sumR2 = 0, sumG2 = 0, sumB2 = 0;
    
    int i;
    for (i = 0; i < 9; i++) {
        float r = neighborhood9[i * 3 + 0];
        float g = neighborhood9[i * 3 + 1];
        float b = neighborhood9[i * 3 + 2];
        
        sumR += r;
        sumG += g;
        sumB += b;
        sumR2 += r * r;
        sumG2 += g * g;
        sumB2 += b * b;
    }
    
    float n = 9.0f;
    float meanR = sumR / n;
    float meanG = sumG / n;
    float meanB = sumB / n;
    
    float varR = (sumR2 / n) - (meanR * meanR);
    float varG = (sumG2 / n) - (meanG * meanG);
    float varB = (sumB2 / n) - (meanB * meanB);
    
    float stdR = sqrtf(fmaxf(varR, 0.0f));
    float stdG = sqrtf(fmaxf(varG, 0.0f));
    float stdB = sqrtf(fmaxf(varB, 0.0f));
    
    float minR = meanR - kSigma * stdR;
    float maxR = meanR + kSigma * stdR;
    float minG = meanG - kSigma * stdG;
    float maxG = meanG + kSigma * stdG;
    float minB = meanB - kSigma * stdB;
    float maxB = meanB + kSigma * stdB;
    
    historyColor[0] = fmaxf(minR, fminf(maxR, historyColor[0]));
    historyColor[1] = fmaxf(minG, fminf(maxG, historyColor[1]));
    historyColor[2] = fmaxf(minB, fminf(maxB, historyColor[2]));
}

void TSSRCAS_LumaWeightedBlend(
    float historyLuma,
    float currentLuma,
    float historyWeight,
    float lumaThreshold,
    float* outFinalWeight
) {
    float lumaDiff = fabsf(currentLuma - historyLuma);
    
    float lumaWeight = 1.0f;
    if (lumaDiff > lumaThreshold) {
        float excess = lumaDiff - lumaThreshold;
        lumaWeight = fmaxf(0.1f, 1.0f - excess * 2.0f);
    }
    
    float finalWeight = historyWeight * lumaWeight;
    finalWeight = fmaxf(0.05f, fminf(1.0f, finalWeight));
    
    if (outFinalWeight) {
        *outFinalWeight = finalWeight;
    }
}

void TSSRCAS_ExposureCompensation(
    float* historyColor,
    float currentExposure,
    float historyExposure
) {
    if (!historyColor || historyExposure < 0.0001f) return;
    
    float exposureRatio = currentExposure / historyExposure;
    
    exposureRatio = fmaxf(0.1f, fminf(10.0f, exposureRatio));
    
    historyColor[0] *= exposureRatio;
    historyColor[1] *= exposureRatio;
    historyColor[2] *= exposureRatio;
}

float TSSRCAS_AdaptiveSharpen(
    const float* center,
    const float* neighbors,
    float contrastThreshold,
    float maxSharpness
) {
    if (!center || !neighbors) return 0.0f;
    
    float centerLuma = 0.2126f * center[0] + 0.7152f * center[1] + 0.0722f * center[2];
    
    float minLuma = centerLuma;
    float maxLuma = centerLuma;
    float sumLuma = centerLuma;
    int count = 1;
    
    int i;
    for (i = 0; i < 8; i++) {
        float neighborLuma = 0.2126f * neighbors[i * 3 + 0] + 
                            0.7152f * neighbors[i * 3 + 1] + 
                            0.0722f * neighbors[i * 3 + 2];
        minLuma = fminf(minLuma, neighborLuma);
        maxLuma = fmaxf(maxLuma, neighborLuma);
        sumLuma += neighborLuma;
        count++;
    }
    
    float avgLuma = sumLuma / (float)count;
    float localContrast = (maxLuma - minLuma) / (avgLuma + 0.0001f);
    
    float sharpness = 0.0f;
    if (localContrast > contrastThreshold) {
        sharpness = fminf(maxSharpness, localContrast * 0.5f);
    }
    
    return sharpness;
}

void TSSRCAS_AntiRinging(
    float* color,
    const float* neighborhoodMin,
    const float* neighborhoodMax
) {
    if (!color || !neighborhoodMin || !neighborhoodMax) return;
    
    int c;
    for (c = 0; c < 3; c++) {
        float minVal = neighborhoodMin[c];
        float maxVal = neighborhoodMax[c];
        
        float range = (maxVal - minVal) * 0.1f;
        
        color[c] = fmaxf(minVal - range, fminf(maxVal + range, color[c]));
    }
}

float TSSRCAS_CalculateVariance(const float* samples, int count, float mean) {
    if (!samples || count <= 0) return 0.0f;
    
    float sumSq = 0.0f;
    int i;
    for (i = 0; i < count; i++) {
        float diff = samples[i] - mean;
        sumSq += diff * diff;
    }
    
    return sumSq / (float)count;
}

float TSSRCAS_CalculateStdDev(float variance) {
    return sqrtf(fmaxf(variance, 0.0f));
}

void TSSRCAS_PackMotionVector(float mvX, float mvY, uint16_t* outPacked) {
    if (!outPacked) return;
    
    int16_t mvX16 = (int16_t)(fmaxf(-1.0f, fminf(1.0f, mvX)) * 32767.0f);
    int16_t mvY16 = (int16_t)(fmaxf(-1.0f, fminf(1.0f, mvY)) * 32767.0f);
    
    outPacked[0] = (uint16_t)mvX16;
    outPacked[1] = (uint16_t)mvY16;
}

void TSSRCAS_UnpackMotionVector(uint16_t packed, float* outMVX, float* outMVY) {
    if (outMVX) {
        int16_t mvX16 = (int16_t)(packed & 0xFFFF);
        *outMVX = (float)mvX16 / 32767.0f;
    }
    if (outMVY) {
        int16_t mvY16 = (int16_t)((packed >> 16) & 0xFFFF);
        *outMVY = (float)mvY16 / 32767.0f;
    }
}

float TSSRCAS_CalculateEMAWeight(
    float velocityMagnitude,
    TSSRCASEMAConfig* config
) {
    if (!config) return 0.1f;
    
    float normalizedVelocity = fminf(velocityMagnitude * config->velocityScale, 1.0f);
    
    float alpha = config->alphaMin + (config->alphaMax - config->alphaMin) * normalizedVelocity;
    
    return fmaxf(config->alphaMin, fminf(config->alphaMax, alpha));
}

void TSSRCAS_UpdateAccumulationCounter(
    TSSRCASAccumulationCounter* counter,
    float lumaDiff,
    float mvMagnitude,
    bool isStable
) {
    if (!counter) return;
    
    if (lumaDiff < 0.01f && mvMagnitude < 0.01f && isStable) {
        counter->historyCount = (uint8_t)fminf(counter->historyCount + 1, 255);
    } else {
        float penalty = fminf(lumaDiff * 10.0f + mvMagnitude * 5.0f, counter->historyCount);
        counter->historyCount = (uint8_t)fmaxf(0, (int)counter->historyCount - (int)penalty);
    }
}

float TSSRCAS_GetAccumulationWeight(TSSRCASAccumulationCounter* counter) {
    if (!counter) return 0.1f;
    
    if (counter->historyCount >= counter->stabilityThreshold) {
        return 0.02f;
    }
    
    float baseWeight = 0.1f;
    float decayFactor = 1.0f - ((float)counter->historyCount / 255.0f);
    
    return fmaxf(0.02f, baseWeight * decayFactor);
}

void TSSRCAS_InitExposureState(TSSRCASExposureState* state, float adaptationRate) {
    if (!state) return;
    
    state->exposure = 1.0f;
    state->prevExposure = 1.0f;
    state->avgLuma = 0.5f;
    state->adaptationRate = adaptationRate;
}

static float TSSRCAS_CalculateLumaRGB(const float* rgb) {
    return 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
}

void TSSRCAS_UpdateExposure(TSSRCASExposureState* state, const void* frame, uint32_t width, uint32_t height) {
    if (!state || !frame) return;
    
    const float* frameData = (const float*)frame;
    
    float totalLuma = 0.0f;
    uint32_t sampleCount = 0;
    
    uint32_t x, y;
    for (y = 0; y < height; y += 4) {
        for (x = 0; x < width; x += 4) {
            uint32_t idx = (y * width + x) * 4;
            float luma = TSSRCAS_CalculateLumaRGB(&frameData[idx]);
            totalLuma += luma;
            sampleCount++;
        }
    }
    
    if (sampleCount > 0) {
        float currentAvgLuma = totalLuma / (float)sampleCount;
        
        state->prevExposure = state->exposure;
        state->exposure = 0.5f / (currentAvgLuma + 0.001f);
        
        state->exposure = fmaxf(0.1f, fminf(10.0f, state->exposure));
        
        float adaptation = (currentAvgLuma - state->avgLuma) * state->adaptationRate;
        state->avgLuma += adaptation;
    }
}

float TSSRCAS_GetExposureScale(TSSRCASExposureState* state) {
    return state ? state->exposure : 1.0f;
}

TSSRCASNeighborStats TSSRCAS_CalculateNeighborStats(const float* neighborhood, int count) {
    TSSRCASNeighborStats stats = {0};
    
    if (!neighborhood || count <= 0) return stats;
    
    float sumR = 0, sumG = 0, sumB = 0;
    float sumR2 = 0, sumG2 = 0, sumB2 = 0;
    
    stats.minColor[0] = neighborhood[0];
    stats.minColor[1] = neighborhood[1];
    stats.minColor[2] = neighborhood[2];
    stats.maxColor[0] = neighborhood[0];
    stats.maxColor[1] = neighborhood[1];
    stats.maxColor[2] = neighborhood[2];
    
    int i;
    for (i = 0; i < count; i++) {
        float r = neighborhood[i * 3 + 0];
        float g = neighborhood[i * 3 + 1];
        float b = neighborhood[i * 3 + 2];
        
        sumR += r;
        sumG += g;
        sumB += b;
        sumR2 += r * r;
        sumG2 += g * g;
        sumB2 += b * b;
        
        stats.minColor[0] = fminf(stats.minColor[0], r);
        stats.minColor[1] = fminf(stats.minColor[1], g);
        stats.minColor[2] = fminf(stats.minColor[2], b);
        stats.maxColor[0] = fmaxf(stats.maxColor[0], r);
        stats.maxColor[1] = fmaxf(stats.maxColor[1], g);
        stats.maxColor[2] = fmaxf(stats.maxColor[2], b);
    }
    
    float n = (float)count;
    stats.avgColor[0] = sumR / n;
    stats.avgColor[1] = sumG / n;
    stats.avgColor[2] = sumB / n;
    
    float varR = (sumR2 / n) - (stats.avgColor[0] * stats.avgColor[0]);
    float varG = (sumG2 / n) - (stats.avgColor[1] * stats.avgColor[1]);
    float varB = (sumB2 / n) - (stats.avgColor[2] * stats.avgColor[2]);
    
    stats.variance = (varR + varG + varB) / 3.0f;
    stats.stdDev = sqrtf(fmaxf(stats.variance, 0.0f));
    
    return stats;
}

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
) {
    (void)currentFrame;
    (void)historyFrame;
    (void)motionVectors;
    (void)depthBuffer;
    (void)outputBuffer;
    (void)tileX;
    (void)tileY;
    (void)tileWidth;
    (void)tileHeight;
    (void)params;
}

void TSSRCAS_ProcessFrame(
    const void* currentFrame,
    const void* historyFrame,
    const void* motionVectors,
    const void* depthBuffer,
    const void* reactiveMask,
    const void* lockMap,
    void* outputBuffer,
    TSSRCASInitParams* params
) {
    (void)currentFrame;
    (void)historyFrame;
    (void)motionVectors;
    (void)depthBuffer;
    (void)reactiveMask;
    (void)lockMap;
    (void)outputBuffer;
    (void)params;
}
