#ifndef TSS_JITTER_H
#define TSS_JITTER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t index;
    int32_t phaseCount;
    float offsetX;
    float offsetY;
    float scaleX;
    float scaleY;
} TSSJitterState;

typedef enum {
    TSS_JITTER_HALTON,
    TSS_JITTER_R2,
    TSS_JITTER_POZZI
} TSSJitterType;

void TSSJitter_Init(TSSJitterState* state, int32_t renderWidth, int32_t displayWidth);
void TSSJitter_InitCustom(TSSJitterState* state, int32_t renderWidth, int32_t displayWidth, TSSJitterType type);

void TSSJitter_Next(TSSJitterState* state);

void TSSJitter_Reset(TSSJitterState* state);

float TSSJitter_GetOffsetX(TSSJitterState* state);
float TSSJitter_GetOffsetY(TSSJitterState* state);

void TSSJitter_GetProjectionOffset(
    TSSJitterState* state,
    int32_t renderWidth,
    int32_t renderHeight,
    float* outProjOffsetX,
    float* outProjOffsetY
);

int32_t TSSJitter_CalculatePhaseCount(int32_t renderWidth, int32_t displayWidth);

float TSSHalton(int32_t index, int32_t base);

float TSSHalton2(int32_t index);
float TSSHalton3(int32_t index);
float TSSHalton5(int32_t index);
float TSSHalton7(int32_t index);

void TSSR2Sequence(int32_t index, float* outX, float* outY);

void TSSPozziSequence(int32_t index, float* outX, float* outY);

typedef struct {
    float dx;
    float dy;
    float confidence;
    int32_t frameAge;
} TSSJitterAnalysis;

void TSSJitter_Analyze(TSSJitterState* state, float mvX, float mvY, TSSJitterAnalysis* outAnalysis);

#ifdef __cplusplus
}
#endif

#endif
