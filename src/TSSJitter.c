#include "TSSJitter.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

int32_t TSSJitter_CalculatePhaseCount(int32_t renderWidth, int32_t displayWidth) {
    if (renderWidth <= 0 || displayWidth <= 0) return 1;
    
    float scale = (float)displayWidth / (float)renderWidth;
    
    if (scale >= 2.5f) return 72;
    if (scale >= 2.0f) return 32;
    if (scale >= 1.5f) return 18;
    if (scale >= 1.3f) return 12;
    if (scale >= 1.2f) return 8;
    
    return (int32_t)ceilf(8.0f * scale * scale);
}

float TSSHalton(int32_t index, int32_t base) {
    float result = 0.0f;
    float f = 1.0f;
    int32_t i = index;
    
    while (i > 0) {
        f = f / (float)base;
        result = result + f * (float)(i % base);
        i = i / base;
    }
    
    return result;
}

float TSSHalton2(int32_t index) {
    return TSSHalton(index, 2);
}

float TSSHalton3(int32_t index) {
    return TSSHalton(index, 3);
}

float TSSHalton5(int32_t index) {
    return TSSHalton(index, 5);
}

float TSSHalton7(int32_t index) {
    return TSSHalton(index, 7);
}

void TSSR2Sequence(int32_t index, float* outX, float* outY) {
    if (!outX || !outY) return;
    
    float phi = 1.32471795724474602596f;
    
    float a1 = 1.0f / phi;
    float a2 = 1.0f / (phi * phi);
    
    float x = fmodf((float)index * a1, 1.0f);
    float y = fmodf((float)index * a2 + 0.5f, 1.0f);
    
    x = x * 2.0f - 1.0f;
    y = y * 2.0f - 1.0f;
    
    *outX = x;
    *outY = y;
}

void TSSPozziSequence(int32_t index, float* outX, float* outY) {
    if (!outX || !outY) return;
    
    float r2 = 0.7548776662466927f;
    float r3 = 0.5698402909980532f;
    
    float x = fmodf((float)index * r2, 1.0f);
    float y = fmodf((float)index * r3 + 0.5f, 1.0f);
    
    x = x * 2.0f - 1.0f;
    y = y * 2.0f - 1.0f;
    
    *outX = x;
    *outY = y;
}

void TSSJitter_Init(TSSJitterState* state, int32_t renderWidth, int32_t displayWidth) {
    TSSJitter_InitCustom(state, renderWidth, displayWidth, TSS_JITTER_HALTON);
}

void TSSJitter_InitCustom(TSSJitterState* state, int32_t renderWidth, int32_t displayWidth, TSSJitterType type) {
    if (!state) return;
    
    state->index = 0;
    state->phaseCount = TSSJitter_CalculatePhaseCount(renderWidth, displayWidth);
    
    float jitterX = 0.0f;
    float jitterY = 0.0f;
    
    switch (type) {
        case TSS_JITTER_R2:
            TSSR2Sequence(0, &jitterX, &jitterY);
            break;
        case TSS_JITTER_POZZI:
            TSSPozziSequence(0, &jitterX, &jitterY);
            break;
        case TSS_JITTER_HALTON:
        default:
            jitterX = TSSHalton2(0) - 0.5f;
            jitterY = TSSHalton3(0) - 0.5f;
            break;
    }
    
    state->offsetX = jitterX;
    state->offsetY = jitterY;
    
    if (renderWidth > 0 && displayWidth > 0) {
        state->scaleX = 2.0f / (float)renderWidth;
        state->scaleY = -2.0f / (float)displayWidth;
    } else {
        state->scaleX = 0.0f;
        state->scaleY = 0.0f;
    }
}

void TSSJitter_Next(TSSJitterState* state) {
    if (!state) return;
    
    state->index++;
    if (state->index >= state->phaseCount) {
        state->index = 0;
    }
    
    state->offsetX = TSSHalton2(state->index) - 0.5f;
    state->offsetY = TSSHalton3(state->index) - 0.5f;
}

void TSSJitter_Reset(TSSJitterState* state) {
    if (!state) return;
    state->index = 0;
    state->offsetX = TSSHalton2(0) - 0.5f;
    state->offsetY = TSSHalton3(0) - 0.5f;
}

float TSSJitter_GetOffsetX(TSSJitterState* state) {
    return state ? state->offsetX : 0.0f;
}

float TSSJitter_GetOffsetY(TSSJitterState* state) {
    return state ? state->offsetY : 0.0f;
}

void TSSJitter_GetProjectionOffset(
    TSSJitterState* state,
    int32_t renderWidth,
    int32_t renderHeight,
    float* outProjOffsetX,
    float* outProjOffsetY
) {
    if (!state) return;
    
    float jitterX = state->offsetX;
    float jitterY = state->offsetY;
    
    if (outProjOffsetX) {
        *outProjOffsetX = 2.0f * jitterX / (float)renderWidth;
    }
    if (outProjOffsetY) {
        *outProjOffsetY = -2.0f * jitterY / (float)renderHeight;
    }
}

void TSSJitter_Analyze(TSSJitterState* state, float mvX, float mvY, TSSJitterAnalysis* outAnalysis) {
    if (!state || !outAnalysis) return;
    
    outAnalysis->dx = state->offsetX;
    outAnalysis->dy = state->offsetY;
    
    float mvMag = sqrtf(mvX * mvX + mvY * mvY);
    
    if (mvMag > 0.01f) {
        float alignment = (state->offsetX * mvX + state->offsetY * mvY) / mvMag;
        outAnalysis->confidence = fabsf(alignment);
    } else {
        outAnalysis->confidence = 1.0f;
    }
    
    outAnalysis->frameAge = state->index;
}
