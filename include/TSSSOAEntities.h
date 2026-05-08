#ifndef TSS_SOA_ENTITIES_H
#define TSS_SOA_ENTITIES_H

#include "TSSTransform3D.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_SOA_VERSION "1.0.0"
#define TSS_MAX_SOA_ENTITIES 65536

typedef struct {
    float* positionsX;
    float* positionsY;
    float* positionsZ;
    
    float* velocitiesX;
    float* velocitiesY;
    float* velocitiesZ;
    
    float* prevPositionsX;
    float* prevPositionsY;
    float* prevPositionsZ;
    
    float* quaternionsX;
    float* quaternionsY;
    float* quaternionsZ;
    float* quaternionsW;
    
    float* scales;
    
    float* boundingRadii;
    
    char* active;
    char* kinematic;
    
    unsigned int* ids;
    
    unsigned int count;
    unsigned int capacity;
} TSSSOAEntities;

TSSSOAEntities* TSSSOACreate(unsigned int maxEntities);
void TSSSOADestroy(TSSSOAEntities* entities);

int TSSSOAAddEntity(TSSSOAEntities* entities, TSSVec3 pos, TSSVec3 vel, float scale, float boundingRadius);
void TSSSOARemoveEntity(TSSSOAEntities* entities, unsigned int index);

void TSSSOAUpdatePhysics(TSSSOAEntities* entities, float dt, unsigned int* indices, unsigned int count);
void TSSSOAInterpolate(TSSSOAEntities* entities, float alpha, unsigned int* indices, unsigned int count);

TSSVec3 TSSSOAGetPosition(TSSSOAEntities* entities, unsigned int index);
TSSVec3 TSSSOAGetVelocity(TSSSOAEntities* entities, unsigned int index);
TSSQuat TSSSOAGetRotation(TSSSOAEntities* entities, unsigned int index);

void TSSSOASetPosition(TSSSOAEntities* entities, unsigned int index, TSSVec3 pos);
void TSSSOASetVelocity(TSSSOAEntities* entities, unsigned int index, TSSVec3 vel);
void TSSSOASetRotation(TSSSOAEntities* entities, unsigned int index, TSSQuat rot);

TSSVec3 TSSSOAGetInterpolatedPosition(TSSSOAEntities* entities, unsigned int index, float alpha);
TSSQuat TSSSOAGetInterpolatedRotation(TSSSOAEntities* entities, unsigned int index, float alpha);

float TSSSOAGetSpeed(TSSSOAEntities* entities, unsigned int index);
int TSSSOAIsActive(TSSSOAEntities* entities, unsigned int index);
int TSSSOAGetActiveCount(TSSSOAEntities* entities);

void TSSSOAClearAll(TSSSOAEntities* entities);

unsigned int TSSSOAGetClosestEntity(TSSSOAEntities* entities, TSSVec3 point, float maxDistance);
void TSSSOAGetEntitiesInRadius(TSSSOAEntities* entities, TSSVec3 center, float radius, unsigned int* outIndices, unsigned int* outCount);

typedef struct TSSSOAWorld {
    TSSSOAEntities* entities;
    TSSBoundingBox* boundingBoxes;
    unsigned int boxCapacity;
    unsigned int boxCount;
} TSSSOAWorld;

TSSSOAWorld* TSSSOACreateWorld(unsigned int maxEntities);
void TSSSOADestroyWorld(TSSSOAWorld* world);

int TSSSOAAddBox(TSSSOAWorld* world, TSSBoundingBox box);
void TSSSOARemoveBox(TSSSOAWorld* world, unsigned int index);

int TSSSOACheckCollision(TSSSOAWorld* world, TSSVec3 pos, float radius, unsigned int* hitBoxIndex);
TSSVec3 TSSSOAResolveCollision(TSSSOAWorld* world, TSSVec3 pos, float radius);

void TSSSOAInterpolateBatchAVX2(TSSSOAEntities* entities, float alpha, float velocityInfluence, unsigned int* indices, unsigned int count, float* outPositions);
void TSSSOAUpdatePhysicsBatchAVX2(TSSSOAEntities* entities, float dt, unsigned int* indices, unsigned int count);

#ifdef __cplusplus
}
#endif

#endif
