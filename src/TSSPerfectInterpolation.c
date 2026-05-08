#include "TSSPerfectInterpolation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TSSPhysicsEngine* TSSCreatePhysicsEngine(uint32_t maxObjects, float frequency) {
    TSSPhysicsEngine* engine = (TSSPhysicsEngine*)calloc(1, sizeof(TSSPhysicsEngine));
    if (!engine) return NULL;
    
    engine->objects = (TSSPhysicsObject*)calloc(maxObjects, sizeof(TSSPhysicsObject));
    if (!engine->objects) {
        free(engine);
        return NULL;
    }
    
    engine->maxObjects = maxObjects;
    engine->physicsFrequency = frequency;
    engine->fixedDeltaTime = 1.0f / frequency;
    engine->accumulator = 0.0f;
    
    return engine;
}

void TSSDestroyPhysicsEngine(TSSPhysicsEngine* engine) {
    if (!engine) return;
    if (engine->objects) free(engine->objects);
    free(engine);
}

void TSSAddObject(TSSPhysicsEngine* engine, TSSVec2 pos, TSSVec2 vel) {
    if (!engine) return;
    
    uint32_t i;
    for (i = 0; i < engine->maxObjects; i++) {
        if (!engine->objects[i].active) {
            engine->objects[i].position = pos;
            engine->objects[i].velocity = vel;
            engine->objects[i].acceleration.x = 0;
            engine->objects[i].acceleration.y = 0;
            engine->objects[i].rotation = 0;
            engine->objects[i].angularVelocity = 0;
            engine->objects[i].scale = 1.0f;
            engine->objects[i].lifetime = -1;
            engine->objects[i].id = i;
            engine->objects[i].active = true;
            engine->objectCount++;
            break;
        }
    }
}

void TSSRemoveObject(TSSPhysicsEngine* engine, uint32_t id) {
    if (!engine) return;
    if (id < engine->maxObjects && engine->objects[id].active) {
        engine->objects[id].active = false;
        engine->objectCount--;
    }
}

void TSSUpdatePhysics(TSSPhysicsEngine* engine, float deltaTime) {
    if (!engine) return;
    
    engine->accumulator += deltaTime;
    
    while (engine->accumulator >= engine->fixedDeltaTime) {
        uint32_t i;
        for (i = 0; i < engine->maxObjects; i++) {
            if (!engine->objects[i].active) continue;
            
            TSSPhysicsObject* obj = &engine->objects[i];
            
            obj->velocity.x += obj->acceleration.x * engine->fixedDeltaTime;
            obj->velocity.y += obj->acceleration.y * engine->fixedDeltaTime;
            
            obj->position.x += obj->velocity.x * engine->fixedDeltaTime;
            obj->position.y += obj->velocity.y * engine->fixedDeltaTime;
            
            obj->rotation += obj->angularVelocity * engine->fixedDeltaTime;
            
            if (obj->lifetime > 0) {
                obj->lifetime -= engine->fixedDeltaTime;
                if (obj->lifetime <= 0) {
                    obj->active = false;
                    engine->objectCount--;
                }
            }
        }
        
        engine->accumulator -= engine->fixedDeltaTime;
    }
}

TSSVec2 TSSPerfectInterpolation(TSSVec2 currentPos, TSSVec2 nextPos, float alpha) {
    TSSVec2 result;
    result.x = currentPos.x + (nextPos.x - currentPos.x) * alpha;
    result.y = currentPos.y + (nextPos.y - currentPos.y) * alpha;
    return result;
}

TSSVec2 TSSExtrapolatePosition(
    TSSVec2 currentPos,
    TSSVec2 velocity,
    float acceleration,
    float alpha,
    float dt
) {
    TSSVec2 result;
    float t = alpha * dt;
    result.x = currentPos.x + velocity.x * t + 0.5f * acceleration * t * t;
    result.y = currentPos.y + velocity.y * t + 0.5f * acceleration * t * t;
    return result;
}

float TSSCalculateSubPixelCoverage(
    TSSVec2 objectPos,
    TSSVec2 objectSize,
    float pixelX,
    float pixelY
) {
    float halfW = objectSize.x * 0.5f;
    float halfH = objectSize.y * 0.5f;
    
    float left = objectPos.x - halfW;
    float right = objectPos.x + halfW;
    float top = objectPos.y - halfH;
    float bottom = objectPos.y + halfH;
    
    float overlapLeft = pixelX - 0.5f;
    float overlapRight = pixelX + 0.5f;
    float overlapTop = pixelY - 0.5f;
    float overlapBottom = pixelY + 0.5f;
    
    if (right < overlapLeft || left > overlapRight) return 0.0f;
    if (bottom < overlapTop || top > overlapBottom) return 0.0f;
    
    float intersectLeft = (left > overlapLeft) ? left : overlapLeft;
    float intersectRight = (right < overlapRight) ? right : overlapRight;
    float intersectTop = (top > overlapTop) ? top : overlapTop;
    float intersectBottom = (bottom < overlapBottom) ? bottom : overlapBottom;
    
    float intersectW = intersectRight - intersectLeft;
    float intersectH = intersectBottom - intersectTop;
    
    if (intersectW <= 0 || intersectH <= 0) return 0.0f;
    
    float coverage = (intersectW * intersectH);
    
    float pixelArea = 1.0f;
    coverage /= pixelArea;
    
    return (coverage > 1.0f) ? 1.0f : coverage;
}

TSSRenderState TSSGetRenderState(
    TSSPhysicsEngine* physics,
    float alpha,
    TSSRenderConfig* config
) {
    TSSRenderState state = {0};
    
    if (!physics || physics->objectCount == 0) return state;
    
    uint32_t i;
    for (i = 0; i < physics->maxObjects; i++) {
        if (!physics->objects[i].active) continue;
        
        TSSPhysicsObject* obj = &physics->objects[i];
        
        TSSVec2 prevPos = obj->position;
        TSSVec2 nextPos;
        nextPos.x = prevPos.x + obj->velocity.x * physics->fixedDeltaTime;
        nextPos.y = prevPos.y + obj->velocity.y * physics->fixedDeltaTime;
        
        if (config->usePerfectInterpolation) {
            state.renderPosition = TSSPerfectInterpolation(prevPos, nextPos, alpha);
        } else {
            state.renderPosition = prevPos;
        }
        
        if (config->useVectorMotion) {
            state.renderVelocity = obj->velocity;
        }
        
        float prevRot = obj->rotation;
        float nextRot = prevRot + obj->angularVelocity * physics->fixedDeltaTime;
        state.renderRotation = prevRot + (nextRot - prevRot) * alpha;
        
        float speed = sqrtf(obj->velocity.x * obj->velocity.x + obj->velocity.y * obj->velocity.y);
        state.subPixelCoverage = 1.0f;
        
        if (config->useAnalyticalAA && obj->scale > 0) {
            TSSVec2 size = {50.0f * obj->scale, 50.0f * obj->scale};
            state.subPixelCoverage = TSSCalculateSubPixelCoverage(
                state.renderPosition, size, state.renderPosition.x, state.renderPosition.y
            );
        }
        
        break;
    }
    
    state.physicsAlpha = alpha;
    state.renderAlpha = alpha;
    
    return state;
}

void TSSApplyMotionBlur(
    TSSVec2* outputColor,
    TSSVec2 startPos,
    TSSVec2 endPos,
    float pixelX,
    float pixelY,
    float blurStrength
) {
    TSSVec2 velocity;
    velocity.x = endPos.x - startPos.x;
    velocity.y = endPos.y - startPos.y;
    
    float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);
    if (speed < 0.001f) return;
    
    float tStart = 0.0f;
    float tEnd = 1.0f;
    
    TSSVec2 velNorm;
    velNorm.x = velocity.x / speed;
    velNorm.y = velocity.y / speed;
    
    float t;
    for (t = tStart; t <= tEnd; t += 1.0f / blurStrength) {
        float projX = startPos.x + velocity.x * t;
        float projY = startPos.y + velocity.y * t;
        
        float dist = sqrtf((projX - pixelX) * (projX - pixelX) + 
                          (projY - pixelY) * (projY - pixelY));
        
        if (dist < 0.5f) {
            float weight = 1.0f - (dist / 0.5f);
            outputColor->x += weight * t;
        }
    }
}
