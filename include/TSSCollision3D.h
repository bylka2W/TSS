#ifndef TSS_COLLISION_3D_H
#define TSS_COLLISION_3D_H

#include "TSSTransform3D.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_COLLISION_VERSION "1.0.0"

typedef enum {
    TSS_BOX = 0,
    TSS_SPHERE = 1,
    TSS_PLANE = 2,
    TSS_RAY = 3,
    TSS_CAPSULE = 4
} TSSColliderType;

typedef struct {
    TSSVec3 origin;
    TSSVec3 normal;
} TSSPlane;

typedef struct {
    TSSRay ray;
    float height;
    float radius;
} TSSCapsule;

int TSSRayIntersectsBox(TSSRay ray, TSSBoundingBox box, TSSRayHit* hit);
int TSSRayIntersectsSphere(TSSRay ray, TSSBoundingSphere sphere, TSSRayHit* hit);
int TSSRayIntersectsPlane(TSSRay ray, TSSPlane plane, TSSRayHit* hit);
int TSSRayIntersectsCapsule(TSSRay ray, TSSCapsule capsule, TSSRayHit* hit);

int TSSBoxIntersectsBox(TSSBoundingBox a, TSSBoundingBox b);
int TSSBoxIntersectsSphere(TSSBoundingBox box, TSSBoundingSphere sphere);
int TSSBoxContainsPoint(TSSBoundingBox box, TSSVec3 point);

float TSSDistancePointToBox(TSSVec3 point, TSSBoundingBox box);
float TSSDistancePointToSphere(TSSVec3 point, TSSBoundingSphere sphere);
float TSSDistancePointToPlane(TSSVec3 point, TSSPlane plane);

TSSVec3 TSSClosestPointOnBox(TSSVec3 point, TSSBoundingBox box);
TSSVec3 TSSClosestPointOnSphere(TSSVec3 point, TSSBoundingSphere sphere);
TSSVec3 TSSClosestPointOnPlane(TSSVec3 point, TSSPlane plane);

TSSBoundingBox TSSMergeBoxes(TSSBoundingBox a, TSSBoundingBox b);
TSSBoundingBox TSSTransformBox(TSSBoundingBox box, TSSMat4 transform);
TSSBoundingSphere TSSTransformSphere(TSSBoundingSphere sphere, TSSMat4 transform);

TSSBoundingBox TSSGetEntityBounds(TSSEntity3D* entity, TSSVec3 size);

#ifdef __cplusplus
}
#endif

#endif
