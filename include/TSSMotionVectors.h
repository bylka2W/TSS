#ifndef TSS_MOTION_VECTORS_H
#define TSS_MOTION_VECTORS_H

#include "TSSTransform3D.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_MAX_MV_HISTORY 4
#define TSS_MV_FORMAT_RGBA16F 1
#define TSS_MV_FORMAT_RG16    2

typedef struct {
    float posX, posY;
} TSSMotionVector2D;

typedef struct {
    float posX, posY, posZ;
} TSSMotionVector3D;

typedef struct {
    float mvX, mvY;
    float confidence;
    uint64_t timestamp;
} TSSPixelMotionVector;

typedef struct {
    TSSPixelMotionVector* vectors;
    int width;
    int height;
    int stride;
    int format;
} TSSMotionVectorField;

typedef struct {
    TSSVec2 current;
    TSSVec2 previous;
    float confidence;
    int frameAge;
} TSSMotionVectorHistory;

typedef struct {
    TSSMotionVectorHistory* history;
    int width;
    int height;
    int historyLength;
    float confidenceThreshold;
} TSSMotionVectorCache;

TSSMotionVectorField* TSSMV_CreateField(int width, int height, int format);
void TSSMV_DestroyField(TSSMotionVectorField* field);

void TSSMV_ClearField(TSSMotionVectorField* field);

void TSSMV_SetVector(TSSMotionVectorField* field, int x, int y, float mvX, float mvY, float confidence);
TSSPixelMotionVector TSSMV_GetVector(TSSMotionVectorField* field, int x, int y);

void TSSMV_GenerateFromTransform(
    TSSMotionVectorField* field,
    TSSMat4 prevViewProj,
    TSSMat4 currViewProj,
    TSSVec3* positions,
    int count
);

void TSSMV_GenerateSceneMotionVectors(
    TSSMotionVectorField* field,
    TSSCamera3D* prevCamera,
    TSSCamera3D* currCamera
);

void TSSMV_Dilate(TSSMotionVectorField* field, int radius);

void TSSMV_TemporalReproject(
    TSSMotionVectorField* field,
    TSSMotionVectorCache* cache,
    TSSMat4 prevVP,
    TSSMat4 currVP
);

void TSSMV_Upscale(
    TSSMotionVectorField* dst,
    TSSMotionVectorField* src,
    float scaleX,
    float scaleY
);

float TSSMV_GetConfidence(TSSMotionVectorField* field, int x, int y);

TSSMotionVectorCache* TSSMV_CreateCache(int width, int height, int historyLength);
void TSSMV_DestroyCache(TSSMotionVectorCache* cache);

void TSSMV_CacheUpdate(TSSMotionVectorCache* cache, TSSMotionVectorField* field);
TSSMotionVectorHistory* TSSMV_CacheGet(TSSMotionVectorCache* cache, int x, int y);

typedef struct {
    float avgConfidence;
    float coverage;
    float avgMagnitude;
    float maxMagnitude;
} TSSMVStats;

TSSMVStats TSSMV_CalculateStats(TSSMotionVectorField* field);

#ifdef __cplusplus
}
#endif

#endif
