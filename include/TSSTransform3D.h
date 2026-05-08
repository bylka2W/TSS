#ifndef TSS_TRANSFORM_3D_H
#define TSS_TRANSFORM_3D_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSS_TRANSFORM_VERSION "5.0.0"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

typedef struct {
    float x, y, z;
} TSSVec3;

typedef struct {
    float x, y;
} TSSVec2;

typedef struct {
    float x, y, z, w;
} TSSVec4;

typedef struct {
    float m[16];
} TSSMat4;

typedef struct {
    float m[9];
} TSSMat3;

typedef struct {
    float x, y, z, w;
} TSSQuat;

typedef struct {
    TSSVec3 position;
    TSSQuat rotation;
    TSSVec3 scale;
} TSSTransform;

typedef struct TSSEntity3D {
    TSSTransform current;
    TSSTransform previous;
    TSSVec3 velocity;
    TSSVec3 angularVelocity;
    TSSVec3 acceleration;
    unsigned int id;
    unsigned char type;
    char active;
    char kinematic;
    char wasTeleported;
} TSSEntity3D;

typedef struct {
    TSSEntity3D* entities;
    unsigned int maxEntities;
    unsigned int activeCount;
    float fixedDeltaTime;
    float accumulator;
    float physicsFrequency;
    float renderFrequency;
    float lastPhysicsTime;
    float lastRenderTime;
} TSSWorld3D;

typedef struct {
    TSSVec3 position;
    TSSQuat rotation;
    TSSVec3 forward;
    TSSVec3 up;
    TSSVec3 right;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    TSSMat4 viewMatrix;
    TSSMat4 projectionMatrix;
    TSSMat4 viewProjectionMatrix;
    char dirty;
} TSSCamera3D;

typedef struct {
    TSSTransform current;
    TSSTransform interpolated;
    float alpha;
    char useExtrapolation;
    char useSlerp;
} TSSRenderState3D;

typedef struct {
    TSSVec3 center;
    TSSVec3 halfExtents;
} TSSBoundingBox;

typedef struct {
    TSSVec3 center;
    float radius;
} TSSBoundingSphere;

typedef struct {
    TSSVec3 origin;
    TSSVec3 direction;
    float maxDistance;
} TSSRay;

typedef struct {
    TSSRay ray;
    float tMin;
    float tMax;
    TSSVec3 hitPoint;
    TSSVec3 hitNormal;
    float distance;
    int hit;
} TSSRayHit;

TSSVec3 TSSVec3_Create(float x, float y, float z);
TSSVec3 TSSVec3_Zero(void);
TSSVec3 TSSVec3_Add(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Sub(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Mul(TSSVec3 v, float scalar);
TSSVec3 TSSVec3_Div(TSSVec3 v, float scalar);
TSSVec3 TSSVec3_Normalize(TSSVec3 v);
float TSSVec3_Length(TSSVec3 v);
float TSSVec3_LengthSq(TSSVec3 v);
float TSSVec3_Dot(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Cross(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Lerp(TSSVec3 a, TSSVec3 b, float t);
TSSVec3 TSSVec3_Extrapolate(TSSVec3 current, TSSVec3 velocity, float dt);

TSSQuat TSSQuat_Create(float x, float y, float z, float w);
TSSQuat TSSQuat_Identity(void);
TSSQuat TSSQuat_FromEuler(float pitch, float yaw, float roll);
TSSQuat TSSQuat_FromAxisAngle(TSSVec3 axis, float angle);
TSSQuat TSSQuat_Multiply(TSSQuat a, TSSQuat b);
TSSVec3 TSSQuat_Rotate(TSSQuat q, TSSVec3 v);
TSSQuat TSSQuat_Slerp(TSSQuat a, TSSQuat b, float t);
TSSQuat TSSQuat_Normalize(TSSQuat q);
float TSSQuat_Dot(TSSQuat a, TSSQuat b);
TSSVec3 TSSQuat_ToEuler(TSSQuat q);
TSSMat3 TSSQuat_ToMat3(TSSQuat q);
TSSMat4 TSSQuat_ToMat4(TSSQuat q);

TSSMat4 TSSMat4_Identity(void);
TSSMat4 TSSMat4_CreateTranslation(TSSVec3 translation);
TSSMat4 TSSMat4_CreateScale(TSSVec3 scale);
TSSMat4 TSSMat4_CreateRotation(TSSQuat rotation);
TSSMat4 TSSMat4_CreateRotationX(float angle);
TSSMat4 TSSMat4_CreateRotationY(float angle);
TSSMat4 TSSMat4_CreateRotationZ(float angle);
TSSMat4 TSSMat4_CreatePerspective(float fov, float aspect, float near, float far);
TSSMat4 TSSMat4_CreateOrthographic(float left, float right, float bottom, float top, float near, float far);
TSSMat4 TSSMat4_LookAt(TSSVec3 eye, TSSVec3 target, TSSVec3 up);
TSSMat4 TSSMat4_Multiply(TSSMat4 a, TSSMat4 b);
TSSVec3 TSSMat4_TransformPoint(TSSMat4 m, TSSVec3 p);
TSSVec3 TSSMat4_TransformVector(TSSMat4 m, TSSVec3 v);
TSSMat4 TSSMat4_Invert(TSSMat4 m);
TSSMat4 TSSMat4_Transpose(TSSMat4 m);

void TSSTransform_SetIdentity(TSSTransform* t);
void TSSTransform_Set(TSSTransform* t, TSSVec3 pos, TSSQuat rot, TSSVec3 scale);
TSSMat4 TSSTransform_ToMatrix(TSSTransform* t);
TSSVec3 TSSTransform_Forward(TSSTransform* t);
TSSVec3 TSSTransform_Up(TSSTransform* t);
TSSVec3 TSSTransform_Right(TSSTransform* t);

void TSSEntity3D_Init(TSSEntity3D* entity, unsigned int id);
void TSSEntity3D_SetPosition(TSSEntity3D* entity, TSSVec3 pos);
void TSSEntity3D_SetRotation(TSSEntity3D* entity, TSSQuat rot);
void TSSEntity3D_SetVelocity(TSSEntity3D* entity, TSSVec3 vel);
void TSSEntity3D_ApplyForce(TSSEntity3D* entity, TSSVec3 force, float mass);
void TSSEntity3D_UpdatePhysics(TSSEntity3D* entity, float dt);
void TSSEntity3D_SaveState(TSSEntity3D* entity);
void TSSEntity3D_DetectTeleport(TSSEntity3D* entity, float threshold);

TSSWorld3D* TSSWorld3D_Create(unsigned int maxEntities, float physicsHz, float renderHz);
void TSSWorld3D_Destroy(TSSWorld3D* world);
TSSEntity3D* TSSWorld3D_AddEntity(TSSWorld3D* world, TSSVec3 pos, TSSQuat rot, TSSVec3 scale);
void TSSWorld3D_RemoveEntity(TSSWorld3D* world, unsigned int id);
void TSSWorld3D_UpdatePhysics(TSSWorld3D* world, float deltaTime);
void TSSWorld3D_UpdateRender(TSSWorld3D* world, float alpha);

TSSCamera3D* TSSCamera3D_Create(void);
void TSSCamera3D_Destroy(TSSCamera3D* camera);
void TSSCamera3D_SetPosition(TSSCamera3D* camera, TSSVec3 pos);
void TSSCamera3D_SetRotation(TSSCamera3D* camera, TSSQuat rot);
void TSSCamera3D_SetFov(TSSCamera3D* camera, float fov);
void TSSCamera3D_SetAspectRatio(TSSCamera3D* camera, float aspect);
void TSSCamera3D_SetPlanes(TSSCamera3D* camera, float near, float far);
void TSSCamera3D_LookAt(TSSCamera3D* camera, TSSVec3 target);
void TSSCamera3D_Move(TSSCamera3D* camera, TSSVec3 delta);
void TSSCamera3D_Rotate(TSSCamera3D* camera, TSSQuat delta);
void TSSCamera3D_UpdateMatrices(TSSCamera3D* camera);
TSSVec3 TSSCamera3D_ScreenToWorld(TSSCamera3D* camera, TSSVec2 screenPos, float depth);
TSSVec2 TSSCamera3D_WorldToScreen(TSSCamera3D* camera, TSSVec3 worldPos);

TSSTransform TSSWorld3D_InterpolateTransform(TSSEntity3D* entity, float alpha, int useSlerp);

struct TSSFrameGenerator3DImpl;
typedef struct TSSFrameGenerator3DImpl* TSSFrameGenerator3D;

TSSFrameGenerator3D TSSCreateFrameGenerator3D(unsigned int maxEntities, float physicsHz, float renderHz);
void TSSDestroyFrameGenerator3D(TSSFrameGenerator3D fg);

void TSSFG3D_AddEntity(TSSFrameGenerator3D fg, TSSVec3 pos, TSSVec3 vel, float mass);
void TSSFG3D_SetEntityPosition(TSSFrameGenerator3D fg, unsigned int id, TSSVec3 pos);
void TSSFG3D_SetEntityRotation(TSSFrameGenerator3D fg, unsigned int id, TSSQuat rot);
void TSSFG3D_SetEntityVelocity(TSSFrameGenerator3D fg, unsigned int id, TSSVec3 vel);
void TSSFG3D_UpdatePhysics(TSSFrameGenerator3D fg, float deltaTime);
void TSSFG3D_Interpolate(TSSFrameGenerator3D fg, float alpha);

TSSEntity3D* TSSFG3D_GetEntity(TSSFrameGenerator3D fg, unsigned int index);
int TSSFG3D_GetEntityCount(TSSFrameGenerator3D fg);
float TSSFG3D_GetLatencyMs(TSSFrameGenerator3D fg);
int TSSFG3D_GetPhysicsHz(TSSFrameGenerator3D fg);
int TSSFG3D_GetRenderHz(TSSFrameGenerator3D fg);

TSSCamera3D* TSSFG3D_GetCamera(TSSFrameGenerator3D fg);
void TSSFG3D_SetCameraTransform(TSSFrameGenerator3D fg, TSSVec3 pos, TSSQuat rot);
void TSSFG3D_UpdateCamera(TSSFrameGenerator3D fg, float alpha);

TSSMat4 TSSFG3D_GetViewMatrix(TSSFrameGenerator3D fg);
TSSMat4 TSSFG3D_GetProjectionMatrix(TSSFrameGenerator3D fg);
TSSMat4 TSSFG3D_GetViewProjectionMatrix(TSSFrameGenerator3D fg);

#ifdef __cplusplus
}
#endif

#endif
