#ifndef TSS_PERFECT_INTERPOLATION_H
#define TSS_PERFECT_INTERPOLATION_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_PI_VERSION "3.0.0"

typedef struct {
    float x, y, z;
} TSSVec3;

typedef struct {
    float x, y;
} TSSVec2;

typedef struct {
    TSSVec2 position;
    TSSVec2 velocity;
    TSSVec2 acceleration;
    float rotation;
    float angularVelocity;
    float scale;
    float lifetime;
    uint32_t id;
    bool active;
} TSSPhysicsObject;

typedef struct {
    TSSPhysicsObject* objects;
    uint32_t objectCount;
    uint32_t maxObjects;
    float fixedDeltaTime;
    float accumulator;
    float physicsFrequency;
} TSSPhysicsEngine;

typedef struct {
    float renderAlpha;
    float physicsAlpha;
    TSSVec2 renderPosition;
    TSSVec2 renderVelocity;
    float renderRotation;
    float subPixelCoverage;
} TSSRenderState;

typedef struct {
    float subPixelThreshold;
    float minVelocityForMotionBlur;
    float maxVelocityForMotionBlur;
    float motionBlurSamples;
    bool useAnalyticalAA;
    bool usePerfectInterpolation;
    bool useVectorMotion;
    float interpolationQuality;
} TSSRenderConfig;

TSSPhysicsEngine* TSSCreatePhysicsEngine(uint32_t maxObjects, float frequency);
void TSSDestroyPhysicsEngine(TSSPhysicsEngine* engine);

void TSSAddObject(TSSPhysicsEngine* engine, TSSVec2 pos, TSSVec2 vel);
void TSSRemoveObject(TSSPhysicsEngine* engine, uint32_t id);

void TSSUpdatePhysics(TSSPhysicsEngine* engine, float deltaTime);

TSSRenderState TSSGetRenderState(
    TSSPhysicsEngine* physics,
    float alpha,
    TSSRenderConfig* config
);

float TSSCalculateSubPixelCoverage(
    TSSVec2 objectPos,
    TSSVec2 objectSize,
    float pixelX,
    float pixelY
);

void TSSApplyAnalyticalAA(
    TSSVec2 pixelPos,
    TSSVec2 objectPos,
    TSSVec2 objectSize,
    float rotation,
    float* coverage
);

TSSVec2 TSSPerfectInterpolation(
    TSSVec2 currentPos,
    TSSVec2 nextPos,
    float alpha
);

TSSVec2 TSSExtrapolatePosition(
    TSSVec2 currentPos,
    TSSVec2 velocity,
    float acceleration,
    float alpha,
    float dt
);

void TSSApplyMotionBlur(
    TSSVec2* outputColor,
    TSSVec2 startPos,
    TSSVec2 endPos,
    float pixelX,
    float pixelY,
    float blurStrength
);

typedef struct TSSDualThreadRendererImpl* TSSDualThreadRenderer;

TSSDualThreadRenderer TSSCreateDualThreadRenderer(
    uint32_t maxObjects,
    float physicsFrequency,
    float renderFrequency
);

void TSSDestroyDualThreadRenderer(TSSDualThreadRenderer renderer);

void TSSAddRenderObject(
    TSSDualThreadRenderer renderer,
    TSSVec2 pos,
    TSSVec2 size,
    float rotation,
    float lifetime
);

void TSSUpdateDualThreadRenderer(TSSDualThreadRenderer renderer, float renderAlpha);

void TSSRenderDualThread(
    TSSDualThreadRenderer renderer,
    float* frameBuffer,
    uint32_t width,
    uint32_t height,
    float renderAlpha
);

float TSSGetLatencyMs(TSSDualThreadRenderer renderer);

#ifdef __cplusplus
}
#endif

#endif
