#include "TSSSOAEntities.h"
#include "TSSCollision3D.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__AVX2__) || defined(_M_X64) || defined(_M_AMD64)
#define TSS_USE_AVX2 1
#include <immintrin.h>
#else
#define TSS_USE_AVX2 0
#endif

TSSSOAEntities* TSSSOACreate(unsigned int maxEntities) {
    TSSSOAEntities* e = (TSSSOAEntities*)calloc(1, sizeof(TSSSOAEntities));
    if (!e) return NULL;
    
    e->capacity = maxEntities;
    
    e->positionsX = (float*)calloc(maxEntities, sizeof(float));
    e->positionsY = (float*)calloc(maxEntities, sizeof(float));
    e->positionsZ = (float*)calloc(maxEntities, sizeof(float));
    
    e->velocitiesX = (float*)calloc(maxEntities, sizeof(float));
    e->velocitiesY = (float*)calloc(maxEntities, sizeof(float));
    e->velocitiesZ = (float*)calloc(maxEntities, sizeof(float));
    
    e->prevPositionsX = (float*)calloc(maxEntities, sizeof(float));
    e->prevPositionsY = (float*)calloc(maxEntities, sizeof(float));
    e->prevPositionsZ = (float*)calloc(maxEntities, sizeof(float));
    
    e->quaternionsX = (float*)calloc(maxEntities, sizeof(float));
    e->quaternionsY = (float*)calloc(maxEntities, sizeof(float));
    e->quaternionsZ = (float*)calloc(maxEntities, sizeof(float));
    e->quaternionsW = (float*)calloc(maxEntities, sizeof(float));
    
    e->scales = (float*)calloc(maxEntities, sizeof(float));
    e->boundingRadii = (float*)calloc(maxEntities, sizeof(float));
    
    e->active = (char*)calloc(maxEntities, sizeof(char));
    e->kinematic = (char*)calloc(maxEntities, sizeof(char));
    
    e->ids = (unsigned int*)calloc(maxEntities, sizeof(unsigned int));
    
    e->count = 0;
    
    return e;
}

void TSSSOADestroy(TSSSOAEntities* entities) {
    if (!entities) return;
    free(entities->positionsX);
    free(entities->positionsY);
    free(entities->positionsZ);
    free(entities->velocitiesX);
    free(entities->velocitiesY);
    free(entities->velocitiesZ);
    free(entities->prevPositionsX);
    free(entities->prevPositionsY);
    free(entities->prevPositionsZ);
    free(entities->quaternionsX);
    free(entities->quaternionsY);
    free(entities->quaternionsZ);
    free(entities->quaternionsW);
    free(entities->scales);
    free(entities->boundingRadii);
    free(entities->active);
    free(entities->kinematic);
    free(entities->ids);
    free(entities);
}

int TSSSOAAddEntity(TSSSOAEntities* entities, TSSVec3 pos, TSSVec3 vel, float scale, float boundingRadius) {
    if (!entities || entities->count >= entities->capacity) return -1;
    
    unsigned int i = entities->count;
    
    entities->positionsX[i] = pos.x;
    entities->positionsY[i] = pos.y;
    entities->positionsZ[i] = pos.z;
    
    entities->prevPositionsX[i] = pos.x;
    entities->prevPositionsY[i] = pos.y;
    entities->prevPositionsZ[i] = pos.z;
    
    entities->velocitiesX[i] = vel.x;
    entities->velocitiesY[i] = vel.y;
    entities->velocitiesZ[i] = vel.z;
    
    entities->quaternionsX[i] = 0.0f;
    entities->quaternionsY[i] = 0.0f;
    entities->quaternionsZ[i] = 0.0f;
    entities->quaternionsW[i] = 1.0f;
    
    entities->scales[i] = scale;
    entities->boundingRadii[i] = boundingRadius;
    
    entities->active[i] = 1;
    entities->kinematic[i] = 0;
    
    entities->ids[i] = i;
    
    entities->count++;
    
    return i;
}

void TSSSOARemoveEntity(TSSSOAEntities* entities, unsigned int index) {
    if (!entities || index >= entities->count) return;
    entities->active[index] = 0;
}

void TSSSOAUpdatePhysics(TSSSOAEntities* entities, float dt, unsigned int* indices, unsigned int count) {
    unsigned int i;
    for (i = 0; i < count; i++) {
        unsigned int idx = indices[i];
        if (idx >= entities->count || !entities->active[idx] || entities->kinematic[idx]) continue;
        
        entities->prevPositionsX[idx] = entities->positionsX[idx];
        entities->prevPositionsY[idx] = entities->positionsY[idx];
        entities->prevPositionsZ[idx] = entities->positionsZ[idx];
        
        entities->positionsX[idx] += entities->velocitiesX[idx] * dt;
        entities->positionsY[idx] += entities->velocitiesY[idx] * dt;
        entities->positionsZ[idx] += entities->velocitiesZ[idx] * dt;
    }
}

TSSVec3 TSSSOAGetPosition(TSSSOAEntities* entities, unsigned int index) {
    TSSVec3 pos = {0, 0, 0};
    if (!entities || index >= entities->count) return pos;
    pos.x = entities->positionsX[index];
    pos.y = entities->positionsY[index];
    pos.z = entities->positionsZ[index];
    return pos;
}

TSSVec3 TSSSOAGetVelocity(TSSSOAEntities* entities, unsigned int index) {
    TSSVec3 vel = {0, 0, 0};
    if (!entities || index >= entities->count) return vel;
    vel.x = entities->velocitiesX[index];
    vel.y = entities->velocitiesY[index];
    vel.z = entities->velocitiesZ[index];
    return vel;
}

void TSSSOASetPosition(TSSSOAEntities* entities, unsigned int index, TSSVec3 pos) {
    if (!entities || index >= entities->count) return;
    entities->prevPositionsX[index] = entities->positionsX[index];
    entities->prevPositionsY[index] = entities->positionsY[index];
    entities->prevPositionsZ[index] = entities->positionsZ[index];
    entities->positionsX[index] = pos.x;
    entities->positionsY[index] = pos.y;
    entities->positionsZ[index] = pos.z;
}

void TSSSOASetVelocity(TSSSOAEntities* entities, unsigned int index, TSSVec3 vel) {
    if (!entities || index >= entities->count) return;
    entities->velocitiesX[index] = vel.x;
    entities->velocitiesY[index] = vel.y;
    entities->velocitiesZ[index] = vel.z;
}

TSSVec3 TSSSOAGetInterpolatedPosition(TSSSOAEntities* entities, unsigned int index, float alpha) {
    TSSVec3 pos = {0, 0, 0};
    if (!entities || index >= entities->count) return pos;
    
    float prevX = entities->prevPositionsX[index];
    float prevY = entities->prevPositionsY[index];
    float prevZ = entities->prevPositionsZ[index];
    float currX = entities->positionsX[index];
    float currY = entities->positionsY[index];
    float currZ = entities->positionsZ[index];
    
    float extrap = alpha;
    pos.x = prevX + (currX - prevX) * extrap;
    pos.y = prevY + (currY - prevY) * extrap;
    pos.z = prevZ + (currZ - prevZ) * extrap;
    
    return pos;
}

float TSSSOAGetSpeed(TSSSOAEntities* entities, unsigned int index) {
    if (!entities || index >= entities->count) return 0.0f;
    float vx = entities->velocitiesX[index];
    float vy = entities->velocitiesY[index];
    float vz = entities->velocitiesZ[index];
    return sqrtf(vx*vx + vy*vy + vz*vz);
}

int TSSSOAIsActive(TSSSOAEntities* entities, unsigned int index) {
    if (!entities || index >= entities->count) return 0;
    return entities->active[index];
}

int TSSSOAGetActiveCount(TSSSOAEntities* entities) {
    if (!entities) return 0;
    int count = 0;
    unsigned int i;
    for (i = 0; i < entities->count; i++) {
        if (entities->active[i]) count++;
    }
    return count;
}

void TSSSOAClearAll(TSSSOAEntities* entities) {
    if (!entities) return;
    memset(entities->active, 0, entities->capacity * sizeof(char));
    entities->count = 0;
}

unsigned int TSSSOAGetClosestEntity(TSSSOAEntities* entities, TSSVec3 point, float maxDistance) {
    if (!entities) return 0xFFFFFFFF;
    
    float bestDist = maxDistance * maxDistance;
    unsigned int bestIndex = 0xFFFFFFFF;
    
    unsigned int i;
    for (i = 0; i < entities->count; i++) {
        if (!entities->active[i]) continue;
        
        float dx = entities->positionsX[i] - point.x;
        float dy = entities->positionsY[i] - point.y;
        float dz = entities->positionsZ[i] - point.z;
        float distSq = dx*dx + dy*dy + dz*dz;
        
        if (distSq < bestDist) {
            bestDist = distSq;
            bestIndex = i;
        }
    }
    
    return bestIndex;
}

void TSSSOAGetEntitiesInRadius(TSSSOAEntities* entities, TSSVec3 center, float radius, unsigned int* outIndices, unsigned int* outCount) {
    if (!entities || !outIndices || !outCount) return;
    
    float radiusSq = radius * radius;
    unsigned int count = 0;
    
    unsigned int i;
    for (i = 0; i < entities->count && count < 1024; i++) {
        if (!entities->active[i]) continue;
        
        float dx = entities->positionsX[i] - center.x;
        float dy = entities->positionsY[i] - center.y;
        float dz = entities->positionsZ[i] - center.z;
        float distSq = dx*dx + dy*dy + dz*dz;
        
        if (distSq < radiusSq) {
            outIndices[count++] = i;
        }
    }
    
    *outCount = count;
}

TSSSOAWorld* TSSSOACreateWorld(unsigned int maxEntities) {
    TSSSOAWorld* world = (TSSSOAWorld*)calloc(1, sizeof(TSSSOAWorld));
    if (!world) return NULL;
    
    world->entities = TSSSOACreate(maxEntities);
    if (!world->entities) {
        free(world);
        return NULL;
    }
    
    world->boxCapacity = 1024;
    world->boundingBoxes = (TSSBoundingBox*)calloc(world->boxCapacity, sizeof(TSSBoundingBox));
    world->boxCount = 0;
    
    return world;
}

void TSSSOADestroyWorld(TSSSOAWorld* world) {
    if (!world) return;
    if (world->entities) TSSSOADestroy(world->entities);
    if (world->boundingBoxes) free(world->boundingBoxes);
    free(world);
}

int TSSSOAAddBox(TSSSOAWorld* world, TSSBoundingBox box) {
    if (!world || world->boxCount >= world->boxCapacity) return -1;
    world->boundingBoxes[world->boxCount] = box;
    return world->boxCount++;
}

void TSSSOARemoveBox(TSSSOAWorld* world, unsigned int index) {
    if (!world || index >= world->boxCount) return;
    world->boundingBoxes[index] = world->boundingBoxes[world->boxCount - 1];
    world->boxCount--;
}

TSSVec3 TSSSOAResolveCollision(TSSSOAWorld* world, TSSVec3 pos, float radius) {
    TSSVec3 result = pos;
    
    if (!world) return result;
    
    TSSBoundingSphere sphere;
    sphere.center = pos;
    sphere.radius = radius;
    
    unsigned int i;
    for (i = 0; i < world->boxCount; i++) {
        TSSBoundingBox box = world->boundingBoxes[i];
        
        TSSVec3 closest;
        closest.x = (pos.x < box.center.x) ? (box.center.x - box.halfExtents.x) : (box.center.x + box.halfExtents.x);
        closest.y = (pos.y < box.center.y) ? (box.center.y - box.halfExtents.y) : (box.center.y + box.halfExtents.y);
        closest.z = (pos.z < box.center.z) ? (box.center.z - box.halfExtents.z) : (box.center.z + box.halfExtents.z);
        
        TSSVec3 minBounds = TSSVec3_Sub(box.center, box.halfExtents);
        TSSVec3 maxBounds = TSSVec3_Add(box.center, box.halfExtents);
        
        closest.x = (closest.x < minBounds.x) ? minBounds.x : closest.x;
        closest.x = (closest.x > maxBounds.x) ? maxBounds.x : closest.x;
        closest.y = (closest.y < minBounds.y) ? minBounds.y : closest.y;
        closest.y = (closest.y > maxBounds.y) ? maxBounds.y : closest.y;
        closest.z = (closest.z < minBounds.z) ? minBounds.z : closest.z;
        closest.z = (closest.z > maxBounds.z) ? maxBounds.z : closest.z;
        
        TSSVec3 diff = TSSVec3_Sub(closest, pos);
        float dist = TSSVec3_Length(diff);
        
        if (dist < radius && dist > 0.0001f) {
            TSSVec3 pushDir = TSSVec3_Div(diff, dist);
            float overlap = radius - dist;
            result.x += pushDir.x * overlap;
            result.y += pushDir.y * overlap;
            result.z += pushDir.z * overlap;
        }
    }
    
    return result;
}

void TSSSOAInterpolateBatchAVX2(TSSSOAEntities* entities, float alpha, float velocityInfluence, unsigned int* indices, unsigned int count, float* outPositions) {
#if TSS_USE_AVX2
    if (!entities || !indices || !outPositions || count == 0) return;
    
    __m256 alphaVec = _mm256_set1_ps(alpha);
    __m256 velInfluenceVec = _mm256_set1_ps(velocityInfluence);
    __m256 oneVec = _mm256_set1_ps(1.0f);
    __m256i indicesVec;
    
    unsigned int i;
    for (i = 0; i + 8 <= count; i += 8) {
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];
        unsigned int idx3 = indices[i + 3];
        unsigned int idx4 = indices[i + 4];
        unsigned int idx5 = indices[i + 5];
        unsigned int idx6 = indices[i + 6];
        unsigned int idx7 = indices[i + 7];
        
        __m256 prevX = _mm256_set_ps(
            entities->prevPositionsX[idx7], entities->prevPositionsX[idx6],
            entities->prevPositionsX[idx5], entities->prevPositionsX[idx4],
            entities->prevPositionsX[idx3], entities->prevPositionsX[idx2],
            entities->prevPositionsX[idx1], entities->prevPositionsX[idx0]
        );
        __m256 prevY = _mm256_set_ps(
            entities->prevPositionsY[idx7], entities->prevPositionsY[idx6],
            entities->prevPositionsY[idx5], entities->prevPositionsY[idx4],
            entities->prevPositionsY[idx3], entities->prevPositionsY[idx2],
            entities->prevPositionsY[idx1], entities->prevPositionsY[idx0]
        );
        __m256 prevZ = _mm256_set_ps(
            entities->prevPositionsZ[idx7], entities->prevPositionsZ[idx6],
            entities->prevPositionsZ[idx5], entities->prevPositionsZ[idx4],
            entities->prevPositionsZ[idx3], entities->prevPositionsZ[idx2],
            entities->prevPositionsZ[idx1], entities->prevPositionsZ[idx0]
        );
        
        __m256 currX = _mm256_set_ps(
            entities->positionsX[idx7], entities->positionsX[idx6],
            entities->positionsX[idx5], entities->positionsX[idx4],
            entities->positionsX[idx3], entities->positionsX[idx2],
            entities->positionsX[idx1], entities->positionsX[idx0]
        );
        __m256 currY = _mm256_set_ps(
            entities->positionsY[idx7], entities->positionsY[idx6],
            entities->positionsY[idx5], entities->positionsY[idx4],
            entities->positionsY[idx3], entities->positionsY[idx2],
            entities->positionsY[idx1], entities->positionsY[idx0]
        );
        __m256 currZ = _mm256_set_ps(
            entities->positionsZ[idx7], entities->positionsZ[idx6],
            entities->positionsZ[idx5], entities->positionsZ[idx4],
            entities->positionsZ[idx3], entities->positionsZ[idx2],
            entities->positionsZ[idx1], entities->positionsZ[idx0]
        );
        
        __m256 velX = _mm256_set_ps(
            entities->velocitiesX[idx7], entities->velocitiesX[idx6],
            entities->velocitiesX[idx5], entities->velocitiesX[idx4],
            entities->velocitiesX[idx3], entities->velocitiesX[idx2],
            entities->velocitiesX[idx1], entities->velocitiesX[idx0]
        );
        __m256 velY = _mm256_set_ps(
            entities->velocitiesY[idx7], entities->velocitiesY[idx6],
            entities->velocitiesY[idx5], entities->velocitiesY[idx4],
            entities->velocitiesY[idx3], entities->velocitiesY[idx2],
            entities->velocitiesY[idx1], entities->velocitiesY[idx0]
        );
        __m256 velZ = _mm256_set_ps(
            entities->velocitiesZ[idx7], entities->velocitiesZ[idx6],
            entities->velocitiesZ[idx5], entities->velocitiesZ[idx4],
            entities->velocitiesZ[idx3], entities->velocitiesZ[idx2],
            entities->velocitiesZ[idx1], entities->velocitiesZ[idx0]
        );
        
        __m256 deltaX = _mm256_sub_ps(currX, prevX);
        __m256 deltaY = _mm256_sub_ps(currY, prevY);
        __m256 deltaZ = _mm256_sub_ps(currZ, prevZ);
        
        __m256 blend = _mm256_mul_ps(alphaVec, oneVec);
        __m256 blendInv = _mm256_sub_ps(oneVec, blend);
        
        __m256 interpX = _mm256_fmadd_ps(prevX, blendInv, _mm256_mul_ps(currX, blend));
        __m256 interpY = _mm256_fmadd_ps(prevY, blendInv, _mm256_mul_ps(currY, blend));
        __m256 interpZ = _mm256_fmadd_ps(prevZ, blendInv, _mm256_mul_ps(currZ, blend));
        
        __m256 velContribX = _mm256_mul_ps(velX, velInfluenceVec);
        __m256 velContribY = _mm256_mul_ps(velY, velInfluenceVec);
        __m256 velContribZ = _mm256_mul_ps(velZ, velInfluenceVec);
        
        interpX = _mm256_add_ps(interpX, velContribX);
        interpY = _mm256_add_ps(interpY, velContribY);
        interpZ = _mm256_add_ps(interpZ, velContribZ);
        
        float outX[8], outY[8], outZ[8];
        _mm256_storeu_ps(outX, interpX);
        _mm256_storeu_ps(outY, interpY);
        _mm256_storeu_ps(outZ, interpZ);
        
        int j;
        for (j = 0; j < 8; j++) {
            int outIdx = (i + j) * 3;
            outPositions[outIdx + 0] = outX[7 - j];
            outPositions[outIdx + 1] = outY[7 - j];
            outPositions[outIdx + 2] = outZ[7 - j];
        }
    }
    
    for (; i < count; i++) {
        unsigned int idx = indices[i];
        if (idx >= entities->count || !entities->active[idx]) continue;
        
        float prevX = entities->prevPositionsX[idx];
        float prevY = entities->prevPositionsY[idx];
        float prevZ = entities->prevPositionsZ[idx];
        float currX = entities->positionsX[idx];
        float currY = entities->positionsY[idx];
        float currZ = entities->positionsZ[idx];
        
        int outIdx = i * 3;
        outPositions[outIdx + 0] = prevX + (currX - prevX) * alpha + entities->velocitiesX[idx] * velocityInfluence;
        outPositions[outIdx + 1] = prevY + (currY - prevY) * alpha + entities->velocitiesY[idx] * velocityInfluence;
        outPositions[outIdx + 2] = prevZ + (currZ - prevZ) * alpha + entities->velocitiesZ[idx] * velocityInfluence;
    }
#else
    (void)entities;
    (void)alpha;
    (void)velocityInfluence;
    (void)indices;
    (void)count;
    (void)outPositions;
#endif
}

void TSSSOAUpdatePhysicsBatchAVX2(TSSSOAEntities* entities, float dt, unsigned int* indices, unsigned int count) {
#if TSS_USE_AVX2
    if (!entities || !indices || count == 0) return;
    
    __m256 dtVec = _mm256_set1_ps(dt);
    
    unsigned int i;
    for (i = 0; i + 8 <= count; i += 8) {
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];
        unsigned int idx3 = indices[i + 3];
        unsigned int idx4 = indices[i + 4];
        unsigned int idx5 = indices[i + 5];
        unsigned int idx6 = indices[i + 6];
        unsigned int idx7 = indices[i + 7];
        
        if (!entities->active[idx0] || entities->kinematic[idx0]) continue;
        if (!entities->active[idx1] || entities->kinematic[idx1]) continue;
        if (!entities->active[idx2] || entities->kinematic[idx2]) continue;
        if (!entities->active[idx3] || entities->kinematic[idx3]) continue;
        if (!entities->active[idx4] || entities->kinematic[idx4]) continue;
        if (!entities->active[idx5] || entities->kinematic[idx5]) continue;
        if (!entities->active[idx6] || entities->kinematic[idx6]) continue;
        if (!entities->active[idx7] || entities->kinematic[idx7]) continue;
        
        __m256 velX = _mm256_set_ps(
            entities->velocitiesX[idx7], entities->velocitiesX[idx6],
            entities->velocitiesX[idx5], entities->velocitiesX[idx4],
            entities->velocitiesX[idx3], entities->velocitiesX[idx2],
            entities->velocitiesX[idx1], entities->velocitiesX[idx0]
        );
        __m256 velY = _mm256_set_ps(
            entities->velocitiesY[idx7], entities->velocitiesY[idx6],
            entities->velocitiesY[idx5], entities->velocitiesY[idx4],
            entities->velocitiesY[idx3], entities->velocitiesY[idx2],
            entities->velocitiesY[idx1], entities->velocitiesY[idx0]
        );
        __m256 velZ = _mm256_set_ps(
            entities->velocitiesZ[idx7], entities->velocitiesZ[idx6],
            entities->velocitiesZ[idx5], entities->velocitiesZ[idx4],
            entities->velocitiesZ[idx3], entities->velocitiesZ[idx2],
            entities->velocitiesZ[idx1], entities->velocitiesZ[idx0]
        );
        
        entities->prevPositionsX[idx0] = entities->positionsX[idx0];
        entities->prevPositionsY[idx0] = entities->positionsY[idx0];
        entities->prevPositionsZ[idx0] = entities->positionsZ[idx0];
        entities->prevPositionsX[idx1] = entities->positionsX[idx1];
        entities->prevPositionsY[idx1] = entities->positionsY[idx1];
        entities->prevPositionsZ[idx1] = entities->positionsZ[idx1];
        entities->prevPositionsX[idx2] = entities->positionsX[idx2];
        entities->prevPositionsY[idx2] = entities->positionsY[idx2];
        entities->prevPositionsZ[idx2] = entities->positionsZ[idx2];
        entities->prevPositionsX[idx3] = entities->positionsX[idx3];
        entities->prevPositionsY[idx3] = entities->positionsY[idx3];
        entities->prevPositionsZ[idx3] = entities->positionsZ[idx3];
        entities->prevPositionsX[idx4] = entities->positionsX[idx4];
        entities->prevPositionsY[idx4] = entities->positionsY[idx4];
        entities->prevPositionsZ[idx4] = entities->positionsZ[idx4];
        entities->prevPositionsX[idx5] = entities->positionsX[idx5];
        entities->prevPositionsY[idx5] = entities->positionsY[idx5];
        entities->prevPositionsZ[idx5] = entities->positionsZ[idx5];
        entities->prevPositionsX[idx6] = entities->positionsX[idx6];
        entities->prevPositionsY[idx6] = entities->positionsY[idx6];
        entities->prevPositionsZ[idx6] = entities->positionsZ[idx6];
        entities->prevPositionsX[idx7] = entities->positionsX[idx7];
        entities->prevPositionsY[idx7] = entities->positionsY[idx7];
        entities->prevPositionsZ[idx7] = entities->positionsZ[idx7];
        
        __m256 deltaX = _mm256_mul_ps(velX, dtVec);
        __m256 deltaY = _mm256_mul_ps(velY, dtVec);
        __m256 deltaZ = _mm256_mul_ps(velZ, dtVec);
        
        float dX[8], dY[8], dZ[8];
        _mm256_storeu_ps(dX, deltaX);
        _mm256_storeu_ps(dY, deltaY);
        _mm256_storeu_ps(dZ, deltaZ);
        
        entities->positionsX[idx0] += dX[0];
        entities->positionsY[idx0] += dY[0];
        entities->positionsZ[idx0] += dZ[0];
        entities->positionsX[idx1] += dX[1];
        entities->positionsY[idx1] += dY[1];
        entities->positionsZ[idx1] += dZ[1];
        entities->positionsX[idx2] += dX[2];
        entities->positionsY[idx2] += dY[2];
        entities->positionsZ[idx2] += dZ[2];
        entities->positionsX[idx3] += dX[3];
        entities->positionsY[idx3] += dY[3];
        entities->positionsZ[idx3] += dZ[3];
        entities->positionsX[idx4] += dX[4];
        entities->positionsY[idx4] += dY[4];
        entities->positionsZ[idx4] += dZ[4];
        entities->positionsX[idx5] += dX[5];
        entities->positionsY[idx5] += dY[5];
        entities->positionsZ[idx5] += dZ[5];
        entities->positionsX[idx6] += dX[6];
        entities->positionsY[idx6] += dY[6];
        entities->positionsZ[idx6] += dZ[6];
        entities->positionsX[idx7] += dX[7];
        entities->positionsY[idx7] += dY[7];
        entities->positionsZ[idx7] += dZ[7];
    }
    
    for (; i < count; i++) {
        unsigned int idx = indices[i];
        if (idx >= entities->count || !entities->active[idx] || entities->kinematic[idx]) continue;
        
        entities->prevPositionsX[idx] = entities->positionsX[idx];
        entities->prevPositionsY[idx] = entities->positionsY[idx];
        entities->prevPositionsZ[idx] = entities->positionsZ[idx];
        
        entities->positionsX[idx] += entities->velocitiesX[idx] * dt;
        entities->positionsY[idx] += entities->velocitiesY[idx] * dt;
        entities->positionsZ[idx] += entities->velocitiesZ[idx] * dt;
    }
#else
    (void)entities;
    (void)dt;
    (void)indices;
    (void)count;
#endif
}
