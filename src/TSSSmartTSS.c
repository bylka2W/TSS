#include "TSSSmartTSS.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

TSSSmartConfig TSSSmart_DefaultConfig(void) {
    TSSSmartConfig config;
    config.mode = TSS_WEIGHT_MODE_HYBRID;
    config.kSigma = 1.25f;
    config.minAlpha = 0.05f;
    config.maxAlpha = 0.5f;
    config.velocityScale = 1.0f;
    config.disocclusionThreshold = 0.1f;
    config.confidenceThreshold = 0.5f;
    config.neuralBlendStrength = 0.3f;
    config.enableYCoCg = 1;
    config.enableDepthTest = 1;
    config.enableAsyncCompute = 1;
    config.enableNegativeLOD = 1;
    return config;
}

void TSSSmart_Init(TSSSmartConfig* config) {
    if (!config) return;
    *config = TSSSmart_DefaultConfig();
}

TSSYCoCgColor TSSSmart_RGBToYCoCg(TSSPixelColor rgb) {
    TSSYCoCgColor result;
    result.y = 0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b;
    result.co = 0.5f * rgb.r - 0.5f * rgb.b;
    result.cg = -0.25f * rgb.r + 0.5f * rgb.g - 0.25f * rgb.b;
    return result;
}

TSSPixelColor TSSSmart_YCoCgToRGB(TSSYCoCgColor ycocg) {
    TSSPixelColor result;
    result.r = ycocg.y + ycocg.co - ycocg.cg;
    result.g = ycocg.y + ycocg.cg;
    result.b = ycocg.y - ycocg.co - ycocg.cg;
    return result;
}

float TSSSmart_CalculateLuma(TSSPixelColor rgb) {
    return 0.2126f * rgb.r + 0.7152f * rgb.g + 0.0722f * rgb.b;
}

float TSSSmart_CalculateLumaYCoCg(TSSYCoCgColor ycocg) {
    return ycocg.y;
}

float TSSSmart_CalculateDisocclusionMask(
    float currentDepth,
    float historyDepth,
    float mvMagnitude,
    float threshold
) {
    float depthDiff = fabsf(currentDepth - historyDepth);
    
    float mvPenalty = fminf(mvMagnitude * 0.1f, 0.3f);
    
    float disocclusion = depthDiff * 100.0f + mvPenalty;
    
    return (disocclusion > threshold) ? fminf(disocclusion, 1.0f) : 0.0f;
}

float TSSSmart_CalculateVelocityDivergence(
    TSSVec2 mv,
    TSSVec2 neighborMVs[8],
    int neighborCount
) {
    if (neighborCount <= 0 || fabsf(mv.x) < 0.001f && fabsf(mv.y) < 0.001f) {
        return 0.0f;
    }
    
    float sumDiff = 0.0f;
    int i;
    for (i = 0; i < neighborCount; i++) {
        float dx = mv.x - neighborMVs[i].x;
        float dy = mv.y - neighborMVs[i].y;
        sumDiff += sqrtf(dx * dx + dy * dy);
    }
    
    float avgDiff = sumDiff / (float)neighborCount;
    
    return fminf(avgDiff * 2.0f, 1.0f);
}

float TSSSmart_CalculateLumaInstability(
    float currentLuma,
    float historyLuma,
    float neighborhoodStdDev
) {
    float lumaDiff = fabsf(currentLuma - historyLuma) / (historyLuma + 0.0001f);
    
    float instability = lumaDiff * 2.0f;
    
    instability += fminf(neighborhoodStdDev * 0.5f, 0.5f);
    
    return fminf(instability, 1.0f);
}

float TSSSmart_NeuralWeightArbitrator(
    float disocclusionMask,
    float velocityDivergence,
    float lumaInstability,
    float mvMagnitude,
    int stabilityFrames,
    TSSSmartConfig* config
) {
    float weight = 0.1f;
    
    if (disocclusionMask > config->disocclusionThreshold) {
        weight = 1.0f;
    } else {
        float velocityWeight = fminf(mvMagnitude * config->velocityScale, 0.4f);
        
        float divergenceWeight = velocityDivergence * 0.3f;
        
        float instabilityWeight = lumaInstability * 0.2f;
        
        float stabilityBoost = fminf((float)stabilityFrames / 60.0f, 0.3f);
        
        weight = config->minAlpha + velocityWeight + divergenceWeight + instabilityWeight - stabilityBoost;
        
        weight = fmaxf(config->minAlpha, fminf(config->maxAlpha, weight));
    }
    
    return weight;
}

TSSConfidenceMasks TSSSmart_GenerateConfidenceMasks(
    float currentDepth,
    float historyDepth,
    TSSVec2 currentMV,
    TSSVec2 historyMV,
    float currentLuma,
    float historyLuma,
    TSSSmartConfig* config
) {
    TSSConfidenceMasks masks;
    
    float mvMagnitude = sqrtf(currentMV.x * currentMV.x + currentMV.y * currentMV.y);
    
    TSSVec2 neighborMVs[8];
    neighborMVs[0] = historyMV;
    neighborMVs[1] = historyMV;
    neighborMVs[2] = currentMV;
    neighborMVs[3] = currentMV;
    
    masks.disocclusionMask = TSSSmart_CalculateDisocclusionMask(
        currentDepth, historyDepth, mvMagnitude, config->disocclusionThreshold
    );
    
    TSSVec2 nMVs[8];
    nMVs[0] = historyMV;
    nMVs[1] = historyMV;
    masks.velocityDivergence = TSSSmart_CalculateVelocityDivergence(currentMV, nMVs, 2);
    
    masks.lumaInstability = TSSSmart_CalculateLumaInstability(
        currentLuma, historyLuma, 0.05f
    );
    
    masks.edgeConfidence = 1.0f - masks.velocityDivergence - masks.lumaInstability * 0.5f;
    
    masks.totalConfidence = 
        masks.edgeConfidence * 0.5f +
        (1.0f - masks.disocclusionMask) * 0.3f +
        (1.0f - masks.velocityDivergence) * 0.2f;
    
    masks.totalConfidence = fmaxf(0.0f, fminf(1.0f, masks.totalConfidence));
    
    return masks;
}

TSSSmartAccumulation TSSSmart_Accumulate(
    TSSPixelColor currentColor,
    TSSPixelColor historyColor,
    TSSPixelColor neighborhood[9],
    TSSVec2 motionVector,
    float currentDepth,
    float historyDepth,
    int stabilityFrames,
    TSSSmartConfig* config
) {
    TSSSmartAccumulation result;
    
    TSSYCoCgColor currentYCC = TSSSmart_RGBToYCoCg(currentColor);
    TSSYCoCgColor historyYCC = TSSSmart_RGBToYCoCg(historyColor);
    
    TSSYCoCgColor neighborsYCC[9];
    int i;
    for (i = 0; i < 9; i++) {
        neighborsYCC[i] = TSSSmart_RGBToYCoCg(neighborhood[i]);
    }
    
    float mvMagnitude = sqrtf(motionVector.x * motionVector.x + motionVector.y * motionVector.y);
    
    float currentLuma = TSSSmart_CalculateLuma(currentColor);
    float historyLuma = TSSSmart_CalculateLuma(historyColor);
    
    TSSConfidenceMasks masks = TSSSmart_GenerateConfidenceMasks(
        currentDepth, historyDepth,
        motionVector, motionVector,
        currentLuma, historyLuma,
        config
    );
    
    result.disocclusion = masks.disocclusionMask;
    
    if (config->enableDepthTest && fabsf(currentDepth - historyDepth) > 0.01f) {
        result.confidence = 0.0f;
        result.weight = 1.0f;
        
        result.historyY = currentYCC.y;
        result.historyCo = currentYCC.co;
        result.historyCg = currentYCC.cg;
        result.stabilityFrames = 0;
        
        return result;
    }
    
    float mean = 0.0f;
    float sumSq = 0.0f;
    for (i = 0; i < 9; i++) {
        mean += neighborsYCC[i].y;
        sumSq += neighborsYCC[i].y * neighborsYCC[i].y;
    }
    mean /= 9.0f;
    float variance = (sumSq / 9.0f) - (mean * mean);
    float stdDev = sqrtf(fmaxf(variance, 0.0f));
    
    result.historyY = TSSSmart_VarianceClipping(historyYCC.y, currentYCC.y, mean, stdDev, config->kSigma);
    result.historyCo = TSSSmart_VarianceClipping(historyYCC.co, currentYCC.co, 0.0f, 0.1f, config->kSigma * 2.0f);
    result.historyCg = TSSSmart_VarianceClipping(historyYCC.cg, currentYCC.cg, 0.0f, 0.1f, config->kSigma * 2.0f);
    
    if (config->enableYCoCg) {
        TSSSmart_YCoCgClamping(&result, neighborsYCC, config->kSigma, config->kSigma * 2.0f, config->kSigma * 2.0f);
    }
    
    float neuralWeight = TSSSmart_NeuralWeightArbitrator(
        masks.disocclusionMask,
        masks.velocityDivergence,
        masks.lumaInstability,
        mvMagnitude,
        stabilityFrames,
        config
    );
    
    result.weight = neuralWeight;
    result.confidence = masks.totalConfidence;
    
    float finalAlpha = neuralWeight * masks.totalConfidence;
    
    result.historyY = result.historyY * (1.0f - finalAlpha) + currentYCC.y * finalAlpha;
    result.historyCo = result.historyCo * (1.0f - finalAlpha) + currentYCC.co * finalAlpha;
    result.historyCg = result.historyCg * (1.0f - finalAlpha) + currentYCC.cg * finalAlpha;
    
    if (masks.disocclusionMask > config->disocclusionThreshold) {
        result.stabilityFrames = 0;
    } else {
        result.stabilityFrames = stabilityFrames + 1;
    }
    
    return result;
}

float TSSSmart_DepthTestedSplatting(
    TSSProjectedPixel* pixels,
    int pixelCount,
    float outputDepth,
    TSSVec2 outputUV
) {
    if (pixelCount <= 0) return 0.0f;
    
    float nearestDepth = 1.0f;
    float totalWeight = 0.0f;
    float resultR = 0.0f, resultG = 0.0f, resultB = 0.0f;
    
    int i;
    for (i = 0; i < pixelCount; i++) {
        if (pixels[i].depth < nearestDepth) {
            float weight = pixels[i].confidence;
            float uvDist = sqrtf(
                (pixels[i].position.x - outputUV.x) * (pixels[i].position.x - outputUV.x) +
                (pixels[i].position.y - outputUV.y) * (pixels[i].position.y - outputUV.y)
            );
            weight *= fmaxf(0.0f, 1.0f - uvDist * 10.0f);
            
            if (weight > 0.0f) {
                resultR += pixels[i].position.z * weight;
                totalWeight += weight;
                nearestDepth = pixels[i].depth;
            }
        }
    }
    
    if (totalWeight > 0.0f) {
        return resultR / totalWeight;
    }
    
    return 0.0f;
}

TSSPixelColor TSSSmart_HoleFilling(
    TSSPixelColor colors[9],
    float depths[9],
    TSSVec2 uv,
    float centerDepth
) {
    TSSPixelColor result = colors[4];
    
    float totalWeight = 1.0f;
    float filledR = result.r;
    float filledG = result.g;
    float filledB = result.b;
    
    int i;
    for (i = 0; i < 9; i++) {
        if (i == 4) continue;
        
        float depthDiff = fabsf(depths[i] - centerDepth);
        if (depthDiff < 0.01f) {
            float weight = 1.0f / (1.0f + sqrtf(
                (uv.x - i) * (uv.x - i) + 
                (uv.y - i) * (uv.y - i)
            ));
            filledR += colors[i].r * weight;
            filledG += colors[i].g * weight;
            filledB += colors[i].b * weight;
            totalWeight += weight;
        }
    }
    
    result.r = filledR / totalWeight;
    result.g = filledG / totalWeight;
    result.b = filledB / totalWeight;
    
    return result;
}

float TSSSmart_VarianceClipping(
    float historyValue,
    float currentValue,
    float mean,
    float stdDev,
    float kSigma
) {
    float minBound = mean - kSigma * stdDev;
    float maxBound = mean + kSigma * stdDev;
    
    if (historyValue < minBound || historyValue > maxBound) {
        float clip = fminf((maxBound - minBound) / (fabsf(historyValue - mean) + 0.0001f), 1.0f);
        return historyValue * (1.0f - clip * 0.5f) + currentValue * (clip * 0.5f);
    }
    
    return historyValue;
}

void TSSSmart_YCoCgClamping(
    TSSYCoCgColor* history,
    TSSYCoCgColor neighbors[9],
    float kY,
    float kCo,
    float kCg
) {
    float yMin = history->y, yMax = history->y;
    float coMin = history->co, coMax = history->co;
    float cgMin = history->cg, cgMax = history->cg;
    
    int i;
    for (i = 0; i < 9; i++) {
        yMin = fminf(yMin, neighbors[i].y);
        yMax = fmaxf(yMax, neighbors[i].y);
        coMin = fminf(coMin, neighbors[i].co);
        coMax = fmaxf(coMax, neighbors[i].co);
        cgMin = fminf(cgMin, neighbors[i].cg);
        cgMax = fmaxf(cgMax, neighbors[i].cg);
    }
    
    float yRange = (yMax - yMin) * kY;
    float coRange = (coMax - coMin) * kCo;
    float cgRange = (cgMax - cgMin) * kCg;
    
    history->y = fmaxf(yMin - yRange, fminf(yMax + yRange, history->y));
    history->co = fmaxf(coMin - coRange, fminf(coMax + coRange, history->co));
    history->cg = fmaxf(cgMin - cgRange, fminf(cgMax + cgRange, history->cg));
}

TSSSmartWeight TSSSmart_CalculateSmartWeight(
    float disocclusion,
    float velocityMag,
    float lumaDiff,
    int framesSinceChange,
    TSSSmartConfig* config
) {
    TSSSmartWeight weight;
    
    weight.weight = config->minAlpha;
    weight.alpha = config->minAlpha;
    weight.confidence = 1.0f;
    
    if (disocclusion > config->disocclusionThreshold) {
        weight.alpha = 1.0f;
        weight.weight = 1.0f;
        weight.confidence = 0.0f;
        return weight;
    }
    
    float velocityFactor = fminf(velocityMag * config->velocityScale, 1.0f);
    
    float lumaFactor = fminf(lumaDiff * 2.0f, 0.5f);
    
    float stabilityBoost = fminf((float)framesSinceChange / 120.0f, 0.3f);
    
    weight.alpha = config->minAlpha + velocityFactor * 0.3f + lumaFactor * 0.2f - stabilityBoost;
    weight.alpha = fmaxf(config->minAlpha, fminf(config->maxAlpha, weight.alpha));
    
    weight.weight = weight.alpha;
    weight.confidence = 1.0f - disocclusion;
    
    return weight;
}

float TSSSmart_AdaptiveSharpness(
    float centerLuma,
    float neighborLuma,
    float edgeThreshold
) {
    float diff = fabsf(centerLuma - neighborLuma);
    
    if (diff < edgeThreshold) {
        return 0.0f;
    }
    
    return fminf(diff * 2.0f, 1.0f);
}

void TSSSmart_UpdateStats(TSSSmartStats* stats, float deltaTime_ms) {
    if (!stats) return;
    
    stats->totalTime_ms += deltaTime_ms;
    stats->inputLag_ms = deltaTime_ms * 0.3f;
}
