#ifndef TSS_VELOCITY_EXTRAPOLATOR_H
#define TSS_VELOCITY_EXTRAPOLATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_VE_VERSION "4.0.0"

typedef struct {
    float x, y;
} TSSVector2;

typedef struct {
    float x, y, z;
} TSSVector3;

typedef enum {
    TSS_ENTITY_CIRCLE = 0,
    TSS_ENTITY_RECT = 1,
    TSS_ENTITY_LINE = 2,
    TSS_ENTITY_POINT = 3
} TSSEntityType;

typedef struct TSSEntity {
    TSSVector2 position;
    TSSVector2 prevPosition;
    TSSVector2 velocity;
    TSSVector2 acceleration;
    TSSVector2 predictedPosition;
    float rotation;
    float angularVelocity;
    float scale;
    float mass;
    float restitution;
    float friction;
    uint32_t id;
    uint8_t type;
    uint8_t r, g, b, a;
    bool active;
    bool kinematic;
    bool wasTeleported;
} TSSEntity;

typedef struct {
    float velocityDecay;
    float accelerationDecay;
    float maxVelocity;
    float minSpeedThreshold;
    float teleportThreshold;
    float jitterSmoothFactor;
    bool enableJitterCompensation;
    bool enableVelocitySmoothing;
    bool enableExtrapolation;
    bool enableInterpolation;
} TSSVelocityConfig;

typedef struct {
    TSSEntity* entities;
    uint32_t maxEntities;
    uint32_t activeCount;
    float fixedDeltaTime;
    float accumulator;
    float physicsFrequency;
    float renderFrequency;
    float lastPhysicsTime;
    float lastRenderTime;
    TSSVelocityConfig config;
} TSSVelocityExtrapolator;

TSSVelocityExtrapolator* TSSCreateVelocityExtrapolator(
    uint32_t maxEntities,
    float physicsHz,
    float renderHz
);

void TSSDestroyVelocityExtrapolator(TSSVelocityExtrapolator* ve);

TSSEntity* TSSAddEntity(
    TSSVelocityExtrapolator* ve,
    TSSVector2 pos,
    TSSVector2 vel,
    uint8_t type
);

void TSSRemoveEntity(TSSVelocityExtrapolator* ve, uint32_t id);

void TSSSetEntityVelocity(TSSVelocityExtrapolator* ve, uint32_t id, TSSVector2 vel);

void TSSApplyForce(TSSVelocityExtrapolator* ve, uint32_t id, TSSVector2 force);

void TSSUpdatePhysics(TSSVelocityExtrapolator* ve, float deltaTime);

void TSSUpdateRender(TSSVelocityExtrapolator* ve, float renderAlpha);

TSSVector2 TSSExtrapolatePosition(
    TSSVector2 currentPos,
    TSSVector2 velocity,
    float timeDelta
);

TSSVector2 TSSJitterCompensation(
    TSSVector2 predicted,
    TSSVector2 actual,
    float smoothness
);

float TSSCalculateVelocityMagnitude(TSSVector2 velocity);

TSSVector2 TSSNormalizeVector(TSSVector2 v);

float TSSVectorDot(TSSVector2 a, TSSVector2 b);

void TSSHandleBoundaryCollision(TSSEntity* entity, float minX, float minY, float maxX, float maxY);

void TSSDetectTeleport(TSSVelocityExtrapolator* ve);

struct TSSFrameGeneratorImpl;
typedef struct TSSFrameGeneratorImpl* TSSFrameGenerator;

TSSFrameGenerator TSSCreateFrameGenerator(
    uint32_t maxEntities,
    float physicsHz,
    float renderHz
);

void TSSDestroyFrameGenerator(TSSFrameGenerator fg);

void TSSFGAddEntity(
    TSSFrameGenerator fg,
    TSSVector2 pos,
    TSSVector2 vel,
    float radius,
    uint8_t r, uint8_t g, uint8_t b
);

void TSSFGUpdate(TSSFrameGenerator fg, float deltaTime);

void TSSFGSetBoundary(TSSFrameGenerator fg, float width, float height);

TSSEntity* TSSFGGetEntity(TSSFrameGenerator fg, uint32_t index);

float TSSFGGetLatencyMs(TSSFrameGenerator fg);

int TSSFGGetPhysicsHz(TSSFrameGenerator fg);

int TSSFGGetRenderHz(TSSFrameGenerator fg);

int TSSFGGetEntityCount(TSSFrameGenerator fg);

#ifdef __cplusplus
}
#endif

#endif
