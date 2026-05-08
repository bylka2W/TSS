#include "TSSVarianceClipping.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

void TSSVC_ConfigDefault(TSSVarianceClippingConfig* config) {
    if (!config) return;
    config->kSigma = 1.25f;
    config->minVarianceWeight = 0.02f;
    config->maxVarianceWeight = 0.5f;
    config->enableAdaptiveK = true;
    config->adaptiveThreshold = 0.1f;
}

TSSVarianceStats TSSVC_CalculateStats(const float* samples, int count) {
    TSSVarianceStats stats = {0};
    
    if (!samples || count <= 0) return stats;
    
    float sum = 0.0f;
    float sumSq = 0.0f;
    float minVal = samples[0];
    float maxVal = samples[0];
    
    int i;
    for (i = 0; i < count; i++) {
        float val = samples[i];
        sum += val;
        sumSq += val * val;
        minVal = (val < minVal) ? val : minVal;
        maxVal = (val > maxVal) ? val : maxVal;
    }
    
    float n = (float)count;
    stats.mean = sum / n;
    
    float meanSq = stats.mean * stats.mean;
    float expectedSq = sumSq / n;
    stats.variance = expectedSq - meanSq;
    if (stats.variance < 0.0f) stats.variance = 0.0f;
    
    stats.stdDev = sqrtf(stats.variance);
    stats.min = minVal;
    stats.max = maxVal;
    stats.range = maxVal - minVal;
    
    return stats;
}

float TSSVC_ClampValue(float value, TSSVarianceStats* stats, float kSigma) {
    if (!stats) return value;
    
    float minBound = stats->mean - kSigma * stats->stdDev;
    float maxBound = stats->mean + kSigma * stats->stdDev;
    
    minBound = (minBound < stats->min) ? stats->min : minBound;
    maxBound = (maxBound > stats->max) ? stats->max : maxBound;
    
    return (value < minBound) ? minBound : (value > maxBound) ? maxBound : value;
}

void TSSVC_ClampRGB(float* color, const float* neighborhood, int sampleCount, float kSigma) {
    if (!color || !neighborhood || sampleCount <= 0) return;
    
    float samplesR[64], samplesG[64], samplesB[64];
    if (sampleCount > 64) sampleCount = 64;
    
    int i;
    for (i = 0; i < sampleCount; i++) {
        samplesR[i] = neighborhood[i * 3 + 0];
        samplesG[i] = neighborhood[i * 3 + 1];
        samplesB[i] = neighborhood[i * 3 + 2];
    }
    
    TSSVarianceStats statsR = TSSVC_CalculateStats(samplesR, sampleCount);
    TSSVarianceStats statsG = TSSVC_CalculateStats(samplesG, sampleCount);
    TSSVarianceStats statsB = TSSVC_CalculateStats(samplesB, sampleCount);
    
    color[0] = TSSVC_ClampValue(color[0], &statsR, kSigma);
    color[1] = TSSVC_ClampValue(color[1], &statsG, kSigma);
    color[2] = TSSVC_ClampValue(color[2], &statsB, kSigma);
}

TSSYCoCg TSSVC_RGBToYCoCg(float r, float g, float b) {
    TSSYCoCg result;
    result.y = 0.25f * r + 0.5f * g + 0.25f * b;
    result.co = 0.5f * r - 0.5f * b;
    result.cg = -0.25f * r + 0.5f * g - 0.25f * b;
    return result;
}

void TSSVC_YCoCgToRGB(TSSYCoCg ycocg, float* outR, float* outG, float* outB) {
    if (outR) *outR = ycocg.y + ycocg.co - ycocg.cg;
    if (outG) *outG = ycocg.y + ycocg.cg;
    if (outB) *outB = ycocg.y - ycocg.co - ycocg.cg;
}

void TSSVC_ClampYCoCg(
    TSSYCoCg* history,
    const TSSYCoCg* neighborhood,
    int sampleCount,
    float kSigmaY,
    float kSigmaCo,
    float kSigmaCg
) {
    if (!history || !neighborhood || sampleCount <= 0) return;
    
    float samplesY[64], samplesCo[64], samplesCg[64];
    if (sampleCount > 64) sampleCount = 64;
    
    int i;
    for (i = 0; i < sampleCount; i++) {
        samplesY[i] = neighborhood[i].y;
        samplesCo[i] = neighborhood[i].co;
        samplesCg[i] = neighborhood[i].cg;
    }
    
    TSSVarianceStats statsY = TSSVC_CalculateStats(samplesY, sampleCount);
    TSSVarianceStats statsCo = TSSVC_CalculateStats(samplesCo, sampleCount);
    TSSVarianceStats statsCg = TSSVC_CalculateStats(samplesCg, sampleCount);
    
    history->y = TSSVC_ClampValue(history->y, &statsY, kSigmaY);
    history->co = TSSVC_ClampValue(history->co, &statsCo, kSigmaCo);
    history->cg = TSSVC_ClampValue(history->cg, &statsCg, kSigmaCg);
}

float TSSVC_CalculateLuma(const float* rgb) {
    return 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
}

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
) {
    TSSVCAccumulation result = {0};
    
    if (!config) {
        TSSVC_ConfigDefault(&config);
    }
    
    TSSYCoCg historyYCC = TSSVC_RGBToYCoCg(historyRGB[0], historyRGB[1], historyRGB[2]);
    TSSYCoCg currentYCC = TSSVC_RGBToYCoCg(currentRGB[0], currentRGB[1], currentRGB[2]);
    
    TSSYCoCg neighborhoodYCC[64];
    int i;
    for (i = 0; i < neighborhoodCount && i < 64; i++) {
        neighborhoodYCC[i] = TSSVC_RGBToYCoCg(
            neighborhood[i * 3 + 0],
            neighborhood[i * 3 + 1],
            neighborhood[i * 3 + 2]
        );
    }
    
    TSSVC_ClampYCoCg(&historyYCC, neighborhoodYCC, neighborhoodCount,
                      config->kSigma, config->kSigma * 2.0f, config->kSigma * 2.0f);
    
    float depthDiff = fabsf(currentDepth - historyDepth);
    float mvMagnitude = sqrtf(mvX * mvX + mvY * mvY);
    
    float disocclusion = depthDiff * 100.0f + mvMagnitude * 0.1f;
    result.disocclusion = disocclusion;
    
    float lumaHistory = historyYCC.y;
    float lumaCurrent = currentYCC.y;
    float lumaDiff = fabsf(lumaCurrent - lumaHistory) / (lumaHistory + 0.0001f);
    
    float confidence = 1.0f;
    confidence *= (1.0f - fminf(disocclusion * 2.0f, 0.9f));
    confidence *= (1.0f - fminf(lumaDiff * 0.5f, 0.5f));
    result.confidence = fmaxf(confidence, 0.0f);
    
    float baseBlend = 0.1f;
    if (disocclusion > 0.1f) {
        baseBlend = 1.0f;
    }
    
    float velocityWeight = fminf(mvMagnitude * 0.5f, 0.4f);
    float blendWeight = baseBlend * (1.0f - velocityWeight) + velocityWeight;
    blendWeight = fmaxf(config->minVarianceWeight, fminf(config->maxVarianceWeight, blendWeight));
    
    float finalBlend = blendWeight * result.confidence;
    
    historyYCC.y = historyYCC.y * (1.0f - finalBlend) + currentYCC.y * finalBlend;
    historyYCC.co = historyYCC.co * (1.0f - finalBlend) + currentYCC.co * finalBlend;
    historyYCC.cg = historyYCC.cg * (1.0f - finalBlend) + currentYCC.cg * finalBlend;
    
    result.historyY = historyYCC.y;
    result.historyCo = historyYCC.co;
    result.historyCg = historyYCC.cg;
    result.currentY = currentYCC.y;
    result.currentCo = currentYCC.co;
    result.currentCg = currentYCC.cg;
    
    return result;
}
