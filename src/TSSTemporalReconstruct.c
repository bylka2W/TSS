#include "TSSTemporalReconstruct.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

TSSTemporalReconstruct* TSSRC_Create(void) {
    TSSTemporalReconstruct* rc = (TSSTemporalReconstruct*)calloc(1, sizeof(TSSTemporalReconstruct));
    if (!rc) return NULL;
    
    rc->config.lanczosA = 2.0f;
    rc->config.catmullRomAlpha = 0.0f;
    rc->config.disocclusionThreshold = 0.1f;
    rc->config.lumaClampMin = 0.02f;
    rc->config.lumaClampMax = 0.1f;
    rc->config.lockThreshold = 0.95f;
    rc->config.blendWeight = 0.05f;
    rc->config.velocityScale = 1.0f;
    rc->config.depthScale = 100.0f;
    rc->config.enableLocking = true;
    rc->config.enableDisocclusion = true;
    rc->config.enableReactive = true;
    rc->config.useYCoCg = true;
    rc->config.mode = TSS_RC_STANDARD;
    
    rc->cascades.cascadeCount = 3;
    rc->cascades.cascadeWeights[0] = 0.5f;
    rc->cascades.cascadeWeights[1] = 0.3f;
    rc->cascades.cascadeWeights[2] = 0.2f;
    
    rc->lockMapResolution = 1.0f;
    rc->lockDecayRate = 0.95f;
    rc->lockBoostAmount = 0.5f;
    
    rc->disocclusionScale = 1.0f;
    rc->reactiveScale = 1.0f;
    
    TSSRC_InitializeWeights(rc);
    
    return rc;
}

void TSSRC_Destroy(TSSTemporalReconstruct* rc) {
    free(rc);
}

void TSSRC_SetConfig(TSSTemporalReconstruct* rc, TSSRCASConfig* config) {
    if (!rc || !config) return;
    memcpy(&rc->config, config, sizeof(TSSRCASConfig));
}

void TSSRC_InitializeWeights(TSSTemporalReconstruct* rc) {
    float a = rc->config.lanczosA;
    
    int i;
    for (i = 0; i < TSS_RC_LANCZOS_TAPS * 2; i++) {
        float x = (float)(i - TSS_RC_LANCZOS_TAPS + 1);
        rc->lanczosWeights[i] = TSSRC_LanczosWeight(x, a);
    }
    
    float catmullAlpha = rc->config.catmullRomAlpha;
    rc->catmullRomWeights[0] = -catmullAlpha;
    rc->catmullRomWeights[1] = 2.0f - catmullAlpha;
    rc->catmullRomWeights[2] = catmullAlpha - 2.0f;
    rc->catmullRomWeights[3] = catmullAlpha;
}

float TSSRC_LanczosWeight(float x, float a) {
    if (x == 0.0f) return 1.0f;
    if (fabsf(x) >= a) return 0.0f;
    
    float pix = M_PI_F * x;
    float sinc = sinf(pix) / pix;
    float lanczosWindow = sinf(pix / a) / (pix / a);
    
    return sinc * lanczosWindow;
}

float TSSRC_CatmullRomWeight(float x) {
    float absX = fabsf(x);
    
    if (absX >= 2.0f) return 0.0f;
    if (absX < 1.0f) {
        float x2 = absX * absX;
        float x3 = x2 * absX;
        return 1.5f * x3 - 2.5f * x2 + 1.0f;
    }
    
    float x2 = (absX - 1.0f) * (absX - 1.0f);
    float x3 = x2 * (absX - 1.0f);
    return -0.5f * x3 + 0.5f * x2;
}

float TSSRC_LanczosSample(
    const float* src,
    int width,
    int height,
    int stride,
    float u,
    float v
) {
    float accum = 0.0f;
    float weightSum = 0.0f;
    
    int startX = (int)floorf(u) - TSS_RC_LANCZOS_TAPS + 1;
    int startY = (int)floorf(v) - TSS_RC_LANCZOS_TAPS + 1;
    
    int x, y;
    for (y = 0; y < TSS_RC_LANCZOS_TAPS * 2; y++) {
        int sampleY = startY + y;
        if (sampleY < 0 || sampleY >= height) continue;
        
        float vWeight = rc->lanczosWeights[y];
        
        for (x = 0; x < TSS_RC_LANCZOS_TAPS * 2; x++) {
            int sampleX = startX + x;
            if (sampleX < 0 || sampleX >= width) continue;
            
            float uWeight = rc->lanczosWeights[x];
            float weight = uWeight * vWeight;
            
            int idx = sampleY * stride + sampleX;
            accum += src[idx] * weight;
            weightSum += weight;
        }
    }
    
    return (weightSum > 0.0f) ? (accum / weightSum) : src[(int)v * stride + (int)u];
}

float TSSRC_CatmullRomSample(
    const float* src,
    int width,
    int height,
    int stride,
    float u,
    float v
) {
    int ix = (int)floorf(u);
    int iy = (int)floorf(v);
    float fx = u - ix;
    float fy = v - iy;
    
    float accum = 0.0f;
    int y, x;
    
    for (y = -1; y <= 2; y++) {
        float vy = TSSRC_CatmullRomWeight(fy - (float)y);
        
        for (x = -1; x <= 2; x++) {
            int sx = ix + x;
            int sy = iy + y;
            
            sx = (sx < 0) ? 0 : ((sx >= width) ? width - 1 : sx);
            sy = (sy < 0) ? 0 : ((sy >= height) ? height - 1 : sy);
            
            float vx = TSSRC_CatmullRomWeight(fx - (float)x);
            float weight = vx * vy;
            
            int idx = sy * stride + sx;
            accum += src[idx] * weight;
        }
    }
    
    return accum;
}

void TSSRC_DisocclusionCheck(
    float currentDepth,
    float historyDepth,
    float mvX,
    float mvY,
    float* outDisocclusion,
    float* outConfidence
) {
    float depthDiff = fabsf(currentDepth - historyDepth);
    float mvMagnitude = sqrtf(mvX * mvX + mvY * mvY);
    
    float disocclusion = depthDiff * 100.0f;
    disocclusion += mvMagnitude * 0.1f;
    
    if (outDisocclusion) *outDisocclusion = disocclusion;
    
    if (outConfidence) {
        float conf = 1.0f - fminf(disocclusion * 2.0f, 1.0f);
        conf *= (1.0f - fminf(mvMagnitude * 0.5f, 0.5f));
        *outConfidence = fmaxf(conf, 0.0f);
    }
}

void TSSRC_LockCheck(
    const float* currentLuma,
    int width,
    int height,
    int x,
    int y,
    float threshold,
    float* outLockWeight
) {
    float center = currentLuma[y * width + x];
    
    float localMin = center;
    float localMax = center;
    float neighbors = 0;
    
    int dy, dx;
    for (dy = -2; dy <= 2; dy++) {
        for (dx = -2; dx <= 2; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
            
            float neighbor = currentLuma[ny * width + nx];
            localMin = fminf(localMin, neighbor);
            localMax = fmaxf(localMax, neighbor);
            neighbors++;
        }
    }
    
    if (neighbors == 0) {
        if (outLockWeight) *outLockWeight = 0.0f;
        return;
    }
    
    float range = localMax - localMin;
    float lockWeight = 0.0f;
    
    if (range < threshold) {
        lockWeight = 1.0f - (range / threshold);
    } else if (center < localMin * 1.5f || center > localMax * 0.5f) {
        lockWeight = 0.8f;
    }
    
    if (outLockWeight) *outLockWeight = lockWeight;
}

void TSSRC_ColorClampYCoCg(
    const float* neighborhood,
    float centerLuma,
    float centerCo,
    float centerCg,
    float minDiff,
    float maxDiff,
    float* outCo,
    float* outCg
) {
    float minCo = centerCo;
    float maxCo = centerCo;
    float minCg = centerCg;
    float maxCg = centerCg;
    
    int i;
    for (i = 0; i < 8; i++) {
        float y = neighborhood[i * 4 + 0];
        float co = neighborhood[i * 4 + 1];
        float cg = neighborhood[i * 4 + 2];
        
        if (fabsf(y - centerLuma) < maxDiff) {
            minCo = fminf(minCo, co);
            maxCo = fmaxf(maxCo, co);
            minCg = fminf(minCg, cg);
            maxCg = fmaxf(maxCg, cg);
        }
    }
    
    float coScale = maxDiff / (maxCo - minCo + 0.0001f);
    float cgScale = maxDiff / (maxCg - minCg + 0.0001f);
    float scale = fminf(coScale, cgScale);
    
    if (outCo) *outCo = centerCo * (1.0f - scale) + fmaxf(minCo, fminf(maxCo, centerCo)) * scale;
    if (outCg) *outCg = centerCg * (1.0f - scale) + fmaxf(minCg, fminf(maxCg, centerCg)) * scale;
}

void TSSRC_ColorClampRGB(
    const float* neighborhood,
    float* rgb,
    float scale
) {
    int i;
    float minR = rgb[0], maxR = rgb[0];
    float minG = rgb[1], maxG = rgb[1];
    float minB = rgb[2], maxB = rgb[2];
    
    for (i = 0; i < 8; i++) {
        minR = fminf(minR, neighborhood[i * 3 + 0]);
        maxR = fmaxf(maxR, neighborhood[i * 3 + 0]);
        minG = fminf(minG, neighborhood[i * 3 + 1]);
        maxG = fmaxf(maxG, neighborhood[i * 3 + 1]);
        minB = fminf(minB, neighborhood[i * 3 + 2]);
        maxB = fmaxf(maxB, neighborhood[i * 3 + 2]);
    }
    
    float centerR = rgb[0];
    float centerG = rgb[1];
    float centerB = rgb[2];
    
    rgb[0] = centerR + (fmaxf(minR, fminf(maxR, centerR)) - centerR) * scale;
    rgb[1] = centerG + (fmaxf(minG, fminf(maxG, centerG)) - centerG) * scale;
    rgb[2] = centerB + (fmaxf(minB, fminf(maxB, centerB)) - centerB) * scale;
}

TSSAccumulation TSSRC_Accumulate(
    const float* historyColor,
    const float* currentColor,
    float mvX,
    float mvY,
    float currentDepth,
    float historyDepth,
    float alpha,
    TSSTemporalReconstruct* rc
) {
    TSSAccumulation result = {0};
    
    float disocclusion = 0.0f;
    float confidence = 1.0f;
    
    if (rc->config.enableDisocclusion) {
        TSSRC_DisocclusionCheck(currentDepth, historyDepth, mvX, mvY, &disocclusion, &confidence);
        result.disocclusion = disocclusion;
    }
    result.confidence = confidence;
    
    float lumaHistory = TSSRC_CalculateLuma(historyColor);
    float lumaCurrent = TSSRC_CalculateLuma(currentColor);
    result.lumaHistory = lumaHistory;
    result.lumaCurrent = lumaCurrent;
    result.depthHistory = historyDepth;
    result.depthCurrent = currentDepth;
    result.mvX = mvX;
    result.mvY = mvY;
    
    float lumaDiff = fabsf(lumaCurrent - lumaHistory) / (lumaHistory + 0.0001f);
    
    float baseBlend = alpha * confidence;
    baseBlend = fmaxf(baseBlend, rc->config.blendWeight);
    
    if (disocclusion > rc->config.disocclusionThreshold) {
        baseBlend = 1.0f;
    }
    
    int i;
    for (i = 0; i < 3; i++) {
        float clampedHistory = historyColor[i];
        float clampedCurrent = currentColor[i];
        
        float neighborMin = fminf(historyColor[i], currentColor[i]);
        float neighborMax = fmaxf(historyColor[i], currentColor[i]);
        
        float spread = neighborMax - neighborMin;
        float clampScale = fminf(spread * 2.0f, 1.0f) * rc->config.lumaClampMax;
        
        clampedHistory = fmaxf(neighborMin - clampScale, fminf(neighborMax + clampScale, clampedHistory));
        
        result.history.r = (i == 0) ? clampedHistory : result.history.r;
        result.history.g = (i == 1) ? clampedHistory : result.history.g;
        result.history.b = (i == 2) ? clampedHistory : result.history.b;
        
        result.current.r = (i == 0) ? clampedCurrent : result.current.r;
        result.current.g = (i == 1) ? clampedCurrent : result.current.g;
        result.current.b = (i == 2) ? clampedCurrent : result.current.b;
    }
    
    float finalWeight = baseBlend;
    
    result.history.r = result.history.r * (1.0f - finalWeight) + result.current.r * finalWeight;
    result.history.g = result.history.g * (1.0f - finalWeight) + result.current.g * finalWeight;
    result.history.b = result.history.b * (1.0f - finalWeight) + result.current.b * finalWeight;
    
    return result;
}

float TSSRC_CalculateLuma(const float* rgb) {
    return 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
}

void TSSRC_RGBToYCoCg(float r, float g, float b, float* y, float* co, float* cg) {
    if (y) *y = 0.25f * r + 0.5f * g + 0.25f * b;
    if (co) *co = 0.5f * r - 0.5f * b;
    if (cg) *cg = -0.25f * r + 0.5f * g - 0.25f * b;
}

void TSSRC_YCoCgToRGB(float y, float co, float cg, float* r, float* g, float* b) {
    if (r) *r = y + co - cg;
    if (g) *g = y + cg;
    if (b) *b = y - co - cg;
}

void TSSRC_ApplyReactiveMask(
    const float* reactiveMask,
    float* blendWeight,
    float scale
) {
    if (!reactiveMask || !blendWeight) return;
    
    float reactive = *reactiveMask * scale;
    reactive = fminf(reactive, 1.0f);
    
    *blendWeight = fmaxf(*blendWeight, reactive);
}

float TSSRC_CalculateConfidence(
    float disocclusion,
    float lumaDiff,
    float mvMagnitude,
    float lockWeight
) {
    float conf = 1.0f;
    
    conf *= (1.0f - fminf(disocclusion * 2.0f, 0.9f));
    
    conf *= (1.0f - fminf(lumaDiff * 0.5f, 0.5f));
    
    conf *= (1.0f - fminf(mvMagnitude * 0.1f, 0.3f));
    
    conf = fmaxf(conf + lockWeight * 0.5f, 0.0f);
    
    return fminf(conf, 1.0f);
}

void TSSRC_WarpHistory(
    const float* historyColor,
    int historyWidth,
    int historyHeight,
    int historyStride,
    float reprojectX,
    float reprojectY,
    float* outWarped
) {
    int srcX = (int)floorf(reprojectX);
    int srcY = (int)floorf(reprojectY);
    
    srcX = (srcX < 0) ? 0 : ((srcX >= historyWidth) ? historyWidth - 1 : srcX);
    srcY = (srcY < 0) ? 0 : ((srcY >= historyHeight) ? historyHeight - 1 : srcY);
    
    float fx = reprojectX - srcX;
    float fy = reprojectY - srcY;
    
    int x0 = srcX, x1 = srcX + 1;
    int y0 = srcY, y1 = srcY + 1;
    
    x1 = (x1 >= historyWidth) ? historyWidth - 1 : x1;
    y1 = (y1 >= historyHeight) ? historyHeight - 1 : y1;
    
    int i;
    for (i = 0; i < 4; i++) {
        float tl = historyColor[y0 * historyStride + x0 * 4 + i];
        float tr = historyColor[y0 * historyStride + x1 * 4 + i];
        float bl = historyColor[y1 * historyStride + x0 * 4 + i];
        float br = historyColor[y1 * historyStride + x1 * 4 + i];
        
        float top = tl * (1.0f - fx) + tr * fx;
        float bottom = bl * (1.0f - fx) + br * fx;
        
        outWarped[i] = top * (1.0f - fy) + bottom * fy;
    }
}

TSSExposureFeedback* TSSRC_CreateExposureFeedback(float adaptationRate) {
    TSSExposureFeedback* fb = (TSSExposureFeedback*)calloc(1, sizeof(TSSExposureFeedback));
    if (!fb) return NULL;
    
    fb->exposure = 1.0f;
    fb->avgLuma = 0.5f;
    fb->sceneLuma = 0.5f;
    fb->prevSceneLuma = 0.5f;
    fb->adaptationRate = adaptationRate;
    
    return fb;
}

void TSSRC_DestroyExposureFeedback(TSSExposureFeedback* fb) {
    free(fb);
}

void TSSRC_UpdateExposure(TSSExposureFeedback* fb, const float* src, int width, int height) {
    if (!fb || !src) return;
    
    float totalLuma = 0.0f;
    int count = 0;
    
    int x, y;
    for (y = 0; y < height; y += 4) {
        for (x = 0; x < width; x += 4) {
            int idx = y * width + x;
            totalLuma += TSSRC_CalculateLuma(&src[idx * 4]);
            count++;
        }
    }
    
    if (count > 0) {
        fb->prevSceneLuma = fb->sceneLuma;
        fb->sceneLuma = totalLuma / count;
    }
    
    float targetLuma = fb->sceneLuma;
    float adaptation = (targetLuma - fb->avgLuma) * fb->adaptationRate;
    fb->avgLuma += adaptation;
    
    if (fb->avgLuma > 0.001f) {
        fb->exposure = 0.5f / fb->avgLuma;
    }
    
    fb->exposure = fmaxf(0.1f, fminf(10.0f, fb->exposure));
}

float TSSRC_GetExposureScale(TSSExposureFeedback* fb) {
    return fb ? fb->exposure : 1.0f;
}
