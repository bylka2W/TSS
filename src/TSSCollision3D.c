#include "TSSCollision3D.h"
#include <math.h>

int TSSRayIntersectsBox(TSSRay ray, TSSBoundingBox box, TSSRayHit* hit) {
    if (!hit) return 0;
    
    TSSVec3 invDir;
    invDir.x = 1.0f / (ray.direction.x != 0.0f ? ray.direction.x : 0.00001f);
    invDir.y = 1.0f / (ray.direction.y != 0.0f ? ray.direction.y : 0.00001f);
    invDir.z = 1.0f / (ray.direction.z != 0.0f ? ray.direction.z : 0.00001f);
    
    TSSVec3 minBounds = TSSVec3_Sub(box.center, box.halfExtents);
    TSSVec3 maxBounds = TSSVec3_Add(box.center, box.halfExtents);
    
    float t1 = (minBounds.x - ray.origin.x) * invDir.x;
    float t2 = (maxBounds.x - ray.origin.x) * invDir.x;
    float t3 = (minBounds.y - ray.origin.y) * invDir.y;
    float t4 = (maxBounds.y - ray.origin.y) * invDir.y;
    float t5 = (minBounds.z - ray.origin.z) * invDir.z;
    float t6 = (maxBounds.z - ray.origin.z) * invDir.z;
    
    float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
    float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));
    
    if (tmax < 0.0f || tmin > tmax || tmin > ray.maxDistance) {
        return 0;
    }
    
    float t = (tmin < 0.0f) ? tmax : tmin;
    
    hit->tMin = t;
    hit->distance = t;
    hit->hitPoint.x = ray.origin.x + ray.direction.x * t;
    hit->hitPoint.y = ray.origin.y + ray.direction.y * t;
    hit->hitPoint.z = ray.origin.z + ray.direction.z * t;
    
    TSSVec3 relPoint;
    relPoint.x = hit->hitPoint.x - box.center.x;
    relPoint.y = hit->hitPoint.y - box.center.y;
    relPoint.z = hit->hitPoint.z - box.center.z;
    
    float absX = (relPoint.x < 0.0f) ? -relPoint.x : relPoint.x;
    float absY = (relPoint.y < 0.0f) ? -relPoint.y : relPoint.y;
    float absZ = (relPoint.z < 0.0f) ? -relPoint.z : relPoint.z;
    
    if (absX > absY && absX > absZ) {
        hit->hitNormal.x = (relPoint.x < 0.0f) ? -1.0f : 1.0f;
        hit->hitNormal.y = 0.0f;
        hit->hitNormal.z = 0.0f;
    } else if (absY > absZ) {
        hit->hitNormal.x = 0.0f;
        hit->hitNormal.y = (relPoint.y < 0.0f) ? -1.0f : 1.0f;
        hit->hitNormal.z = 0.0f;
    } else {
        hit->hitNormal.x = 0.0f;
        hit->hitNormal.y = 0.0f;
        hit->hitNormal.z = (relPoint.z < 0.0f) ? -1.0f : 1.0f;
    }
    
    hit->hit = 1;
    return 1;
}

int TSSRayIntersectsSphere(TSSRay ray, TSSBoundingSphere sphere, TSSRayHit* hit) {
    if (!hit) return 0;
    
    TSSVec3 oc;
    oc.x = ray.origin.x - sphere.center.x;
    oc.y = ray.origin.y - sphere.center.y;
    oc.z = ray.origin.z - sphere.center.z;
    
    float a = TSSVec3_Dot(ray.direction, ray.direction);
    float b = 2.0f * TSSVec3_Dot(oc, ray.direction);
    float c = TSSVec3_Dot(oc, oc) - sphere.radius * sphere.radius;
    
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) return 0;
    
    float t = (-b - sqrtf(discriminant)) / (2.0f * a);
    
    if (t < 0.0f) t = (-b + sqrtf(discriminant)) / (2.0f * a);
    if (t < 0.0f) return 0;
    if (t > ray.maxDistance) return 0;
    
    hit->tMin = t;
    hit->distance = t;
    hit->hitPoint.x = ray.origin.x + ray.direction.x * t;
    hit->hitPoint.y = ray.origin.y + ray.direction.y * t;
    hit->hitPoint.z = ray.origin.z + ray.direction.z * t;
    
    hit->hitNormal.x = (hit->hitPoint.x - sphere.center.x) / sphere.radius;
    hit->hitNormal.y = (hit->hitPoint.y - sphere.center.y) / sphere.radius;
    hit->hitNormal.z = (hit->hitPoint.z - sphere.center.z) / sphere.radius;
    
    hit->hit = 1;
    return 1;
}

int TSSRayIntersectsPlane(TSSRay ray, TSSPlane plane, TSSRayHit* hit) {
    if (!hit) return 0;
    
    float denom = TSSVec3_Dot(plane.normal, ray.direction);
    
    if (fabsf(denom) < 0.00001f) return 0;
    
    float t = TSSVec3_Dot(TSSVec3_Sub(plane.origin, ray.origin), plane.normal) / denom;
    
    if (t < 0.0f || t > ray.maxDistance) return 0;
    
    hit->tMin = t;
    hit->distance = t;
    hit->hitPoint.x = ray.origin.x + ray.direction.x * t;
    hit->hitPoint.y = ray.origin.y + ray.direction.y * t;
    hit->hitPoint.z = ray.origin.z + ray.direction.z * t;
    hit->hitNormal = plane.normal;
    hit->hit = 1;
    
    return 1;
}

int TSSBoxIntersectsBox(TSSBoundingBox a, TSSBoundingBox b) {
    TSSVec3 aMin = TSSVec3_Sub(a.center, a.halfExtents);
    TSSVec3 aMax = TSSVec3_Add(a.center, a.halfExtents);
    TSSVec3 bMin = TSSVec3_Sub(b.center, b.halfExtents);
    TSSVec3 bMax = TSSVec3_Add(b.center, b.halfExtents);
    
    return (aMin.x <= bMax.x && aMax.x >= bMin.x &&
            aMin.y <= bMax.y && aMax.y >= bMin.y &&
            aMin.z <= bMax.z && aMax.z >= bMin.z);
}

int TSSBoxContainsPoint(TSSBoundingBox box, TSSVec3 point) {
    TSSVec3 minBounds = TSSVec3_Sub(box.center, box.halfExtents);
    TSSVec3 maxBounds = TSSVec3_Add(box.center, box.halfExtents);
    
    return (point.x >= minBounds.x && point.x <= maxBounds.x &&
            point.y >= minBounds.y && point.y <= maxBounds.y &&
            point.z >= minBounds.z && point.z <= maxBounds.z);
}

float TSSDistancePointToBox(TSSVec3 point, TSSBoundingBox box) {
    TSSVec3 closest;
    closest.x = (point.x < box.center.x) ? (box.center.x - box.halfExtents.x) : (box.center.x + box.halfExtents.x);
    closest.y = (point.y < box.center.y) ? (box.center.y - box.halfExtents.y) : (box.center.y + box.halfExtents.y);
    closest.z = (point.z < box.center.z) ? (box.center.z - box.halfExtents.z) : (box.center.z + box.halfExtents.z);
    
    TSSVec3 minBounds = TSSVec3_Sub(box.center, box.halfExtents);
    TSSVec3 maxBounds = TSSVec3_Add(box.center, box.halfExtents);
    
    closest.x = (closest.x < minBounds.x) ? minBounds.x : closest.x;
    closest.x = (closest.x > maxBounds.x) ? maxBounds.x : closest.x;
    closest.y = (closest.y < minBounds.y) ? minBounds.y : closest.y;
    closest.y = (closest.y > maxBounds.y) ? maxBounds.y : closest.y;
    closest.z = (closest.z < minBounds.z) ? minBounds.z : closest.z;
    closest.z = (closest.z > maxBounds.z) ? maxBounds.z : closest.z;
    
    TSSVec3 diff = TSSVec3_Sub(closest, point);
    return TSSVec3_Length(diff);
}

TSSVec3 TSSClosestPointOnBox(TSSVec3 point, TSSBoundingBox box) {
    TSSVec3 result;
    result.x = (point.x < box.center.x) ? (box.center.x - box.halfExtents.x) : (box.center.x + box.halfExtents.x);
    result.y = (point.y < box.center.y) ? (box.center.y - box.halfExtents.y) : (box.center.y + box.halfExtents.y);
    result.z = (point.z < box.center.z) ? (box.center.z - box.halfExtents.z) : (box.center.z + box.halfExtents.z);
    
    TSSVec3 minBounds = TSSVec3_Sub(box.center, box.halfExtents);
    TSSVec3 maxBounds = TSSVec3_Add(box.center, box.halfExtents);
    
    result.x = (result.x < minBounds.x) ? minBounds.x : result.x;
    result.x = (result.x > maxBounds.x) ? maxBounds.x : result.x;
    result.y = (result.y < minBounds.y) ? minBounds.y : result.y;
    result.y = (result.y > maxBounds.y) ? maxBounds.y : result.y;
    result.z = (result.z < minBounds.z) ? minBounds.z : result.z;
    result.z = (result.z > maxBounds.z) ? maxBounds.z : result.z;
    
    return result;
}

TSSBoundingBox TSSMergeBoxes(TSSBoundingBox a, TSSBoundingBox b) {
    TSSBoundingBox result;
    TSSVec3 aMin = TSSVec3_Sub(a.center, a.halfExtents);
    TSSVec3 aMax = TSSVec3_Add(a.center, a.halfExtents);
    TSSVec3 bMin = TSSVec3_Sub(b.center, b.halfExtents);
    TSSVec3 bMax = TSSVec3_Add(b.center, b.halfExtents);
    
    TSSVec3 minPt;
    minPt.x = (aMin.x < bMin.x) ? aMin.x : bMin.x;
    minPt.y = (aMin.y < bMin.y) ? aMin.y : bMin.y;
    minPt.z = (aMin.z < bMin.z) ? aMin.z : bMin.z;
    
    TSSVec3 maxPt;
    maxPt.x = (aMax.x > bMax.x) ? aMax.x : bMax.x;
    maxPt.y = (aMax.y > bMax.y) ? aMax.y : bMax.y;
    maxPt.z = (aMax.z > bMax.z) ? aMax.z : bMax.z;
    
    result.center = TSSVec3_Mul(TSSVec3_Add(minPt, maxPt), 0.5f);
    result.halfExtents = TSSVec3_Mul(TSSVec3_Sub(maxPt, minPt), 0.5f);
    
    return result;
}

TSSBoundingBox TSSTransformBox(TSSBoundingBox box, TSSMat4 transform) {
    TSSVec3 corners[8];
    TSSVec3 min = TSSVec3_Sub(box.center, box.halfExtents);
    TSSVec3 max = TSSVec3_Add(box.center, box.halfExtents);
    
    corners[0] = TSSMat4_TransformPoint(transform, (TSSVec3){min.x, min.y, min.z});
    corners[1] = TSSMat4_TransformPoint(transform, (TSSVec3){max.x, min.y, min.z});
    corners[2] = TSSMat4_TransformPoint(transform, (TSSVec3){min.x, max.y, min.z});
    corners[3] = TSSMat4_TransformPoint(transform, (TSSVec3){max.x, max.y, min.z});
    corners[4] = TSSMat4_TransformPoint(transform, (TSSVec3){min.x, min.y, max.z});
    corners[5] = TSSMat4_TransformPoint(transform, (TSSVec3){max.x, min.y, max.z});
    corners[6] = TSSMat4_TransformPoint(transform, (TSSVec3){min.x, max.y, max.z});
    corners[7] = TSSMat4_TransformPoint(transform, (TSSVec3){max.x, max.y, max.z});
    
    TSSVec3 minBounds = corners[0];
    TSSVec3 maxBounds = corners[0];
    int i;
    for (i = 1; i < 8; i++) {
        minBounds.x = (corners[i].x < minBounds.x) ? corners[i].x : minBounds.x;
        minBounds.y = (corners[i].y < minBounds.y) ? corners[i].y : minBounds.y;
        minBounds.z = (corners[i].z < minBounds.z) ? corners[i].z : minBounds.z;
        maxBounds.x = (corners[i].x > maxBounds.x) ? corners[i].x : maxBounds.x;
        maxBounds.y = (corners[i].y > maxBounds.y) ? corners[i].y : maxBounds.y;
        maxBounds.z = (corners[i].z > maxBounds.z) ? corners[i].z : maxBounds.z;
    }
    
    TSSBoundingBox result;
    result.center = TSSVec3_Mul(TSSVec3_Add(minBounds, maxBounds), 0.5f);
    result.halfExtents = TSSVec3_Mul(TSSVec3_Sub(maxBounds, minBounds), 0.5f);
    
    return result;
}

TSSBoundingSphere TSSTransformSphere(TSSBoundingSphere sphere, TSSMat4 transform) {
    TSSBoundingSphere result;
    result.center = TSSMat4_TransformPoint(transform, sphere.center);
    result.radius = sphere.radius * fmaxf(fmaxf(transform.m[0], transform.m[5]), transform.m[10]);
    return result;
}

TSSBoundingBox TSSGetEntityBounds(TSSEntity3D* entity, TSSVec3 size) {
    TSSBoundingBox box;
    box.center = entity->current.position;
    box.halfExtents = TSSVec3_Mul(size, entity->current.scale.x * 0.5f);
    return box;
}
