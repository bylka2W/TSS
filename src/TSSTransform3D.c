#include "TSSTransform3D.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

TSSVec3 TSSVec3_Create(float x, float y, float z) {
    TSSVec3 v = {x, y, z};
    return v;
}

TSSVec3 TSSVec3_Zero(void) {
    TSSVec3 v = {0, 0, 0};
    return v;
}

TSSVec3 TSSVec3_Add(TSSVec3 a, TSSVec3 b) {
    TSSVec3 v = {a.x + b.x, a.y + b.y, a.z + b.z};
    return v;
}

TSSVec3 TSSVec3_Sub(TSSVec3 a, TSSVec3 b) {
    TSSVec3 v = {a.x - b.x, a.y - b.y, a.z - b.z};
    return v;
}

TSSVec3 TSSVec3_Mul(TSSVec3 v, float scalar) {
    TSSVec3 result = {v.x * scalar, v.y * scalar, v.z * scalar};
    return result;
}

TSSVec3 TSSVec3_Div(TSSVec3 v, float scalar) {
    if ((scalar < 0.00001f && scalar > -0.00001f)) return v;
    TSSVec3 result = {v.x / scalar, v.y / scalar, v.z / scalar};
    return result;
}

TSSVec3 TSSVec3_Normalize(TSSVec3 v) {
    float len = TSSVec3_Length(v);
    if (len < 0.00001f) return TSSVec3_Zero();
    return TSSVec3_Div(v, len);
}

float TSSVec3_Length(TSSVec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float TSSVec3_LengthSq(TSSVec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float TSSVec3_Dot(TSSVec3 a, TSSVec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

TSSVec3 TSSVec3_Cross(TSSVec3 a, TSSVec3 b) {
    TSSVec3 v;
    v.x = a.y * b.z - a.z * b.y;
    v.y = a.z * b.x - a.x * b.z;
    v.z = a.x * b.y - a.y * b.x;
    return v;
}

TSSVec3 TSSVec3_Lerp(TSSVec3 a, TSSVec3 b, float t) {
    TSSVec3 v;
    v.x = a.x + (b.x - a.x) * t;
    v.y = a.y + (b.y - a.y) * t;
    v.z = a.z + (b.z - a.z) * t;
    return v;
}

TSSVec3 TSSVec3_Extrapolate(TSSVec3 current, TSSVec3 velocity, float dt) {
    TSSVec3 result;
    result.x = current.x + velocity.x * dt;
    result.y = current.y + velocity.y * dt;
    result.z = current.z + velocity.z * dt;
    return result;
}

TSSQuat TSSQuat_Create(float x, float y, float z, float w) {
    TSSQuat q = {x, y, z, w};
    return q;
}

TSSQuat TSSQuat_Identity(void) {
    TSSQuat q = {0, 0, 0, 1};
    return q;
}

TSSQuat TSSQuat_FromEuler(float pitch, float yaw, float roll) {
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);
    
    TSSQuat q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    return q;
}

TSSQuat TSSQuat_FromAxisAngle(TSSVec3 axis, float angle) {
    float halfAngle = angle * 0.5f;
    float s = sinf(halfAngle);
    TSSQuat q;
    q.x = axis.x * s;
    q.y = axis.y * s;
    q.z = axis.z * s;
    q.w = cosf(halfAngle);
    return q;
}

TSSQuat TSSQuat_Multiply(TSSQuat a, TSSQuat b) {
    TSSQuat q;
    q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return q;
}

TSSVec3 TSSQuat_Rotate(TSSQuat q, TSSVec3 v) {
    TSSVec3 uv = TSSVec3_Create(q.x, q.y, q.z);
    TSSVec3 uuv = TSSVec3_Cross(uv, v);
    TSSVec3 qvv = TSSVec3_Mul(uv, 2.0f * TSSVec3_Dot(uv, v));
    return TSSVec3_Add(TSSVec3_Add(v, TSSVec3_Mul(uuv, q.w)), TSSVec3_Mul(uuv, q.w));
}

TSSQuat TSSQuat_Slerp(TSSQuat a, TSSQuat b, float t) {
    float dot = TSSQuat_Dot(a, b);
    
    TSSQuat bb = b;
    if (dot < 0) {
        bb.x = -b.x;
        bb.y = -b.y;
        bb.z = -b.z;
        bb.w = -b.w;
        dot = -dot;
    }
    
    if (dot > 0.9995f) {
        TSSQuat q;
        q.x = a.x + (bb.x - a.x) * t;
        q.y = a.y + (bb.y - a.y) * t;
        q.z = a.z + (bb.z - a.z) * t;
        q.w = a.w + (bb.w - a.w) * t;
        return TSSQuat_Normalize(q);
    }
    
    float theta_0 = acosf(dot);
    float theta = theta_0 * t;
    float sin_theta = sinf(theta);
    float sin_theta_0 = sinf(theta_0);
    
    float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;
    
    TSSQuat q;
    q.x = a.x * s0 + bb.x * s1;
    q.y = a.y * s0 + bb.y * s1;
    q.z = a.z * s0 + bb.z * s1;
    q.w = a.w * s0 + bb.w * s1;
    return q;
}

TSSQuat TSSQuat_Normalize(TSSQuat q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < 0.00001f) return TSSQuat_Identity();
    q.x /= len;
    q.y /= len;
    q.z /= len;
    q.w /= len;
    return q;
}

float TSSQuat_Dot(TSSQuat a, TSSQuat b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

TSSVec3 TSSQuat_ToEuler(TSSQuat q) {
    TSSVec3 euler;
    
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    euler.x = atan2f(sinr_cosp, cosr_cosp);
    
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if ((sinp >= 1.0f) || (sinp <= -1.0f)) {
        euler.y = (sinp >= 0) ? (M_PI_F / 2.0f) : (-M_PI_F / 2.0f);
    } else {
        euler.y = asinf(sinp);
    }
    
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    euler.z = atan2f(siny_cosp, cosy_cosp);
    
    return euler;
}

TSSMat3 TSSQuat_ToMat3(TSSQuat q) {
    TSSMat3 m;
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    
    m.m[0] = 1.0f - 2.0f * (yy + zz);
    m.m[1] = 2.0f * (xy + wz);
    m.m[2] = 2.0f * (xz - wy);
    m.m[3] = 2.0f * (xy - wz);
    m.m[4] = 1.0f - 2.0f * (xx + zz);
    m.m[5] = 2.0f * (yz + wx);
    m.m[6] = 2.0f * (xz + wy);
    m.m[7] = 2.0f * (yz - wx);
    m.m[8] = 1.0f - 2.0f * (xx + yy);
    
    return m;
}

TSSMat4 TSSQuat_ToMat4(TSSQuat q) {
    TSSMat3 m3 = TSSQuat_ToMat3(q);
    TSSMat4 m = TSSMat4_Identity();
    m.m[0] = m3.m[0]; m.m[1] = m3.m[1]; m.m[2] = m3.m[2];
    m.m[4] = m3.m[3]; m.m[5] = m3.m[4]; m.m[6] = m3.m[5];
    m.m[8] = m3.m[6]; m.m[9] = m3.m[7]; m.m[10] = m3.m[8];
    m.m[15] = 1.0f;
    return m;
}

TSSMat4 TSSMat4_Identity(void) {
    TSSMat4 m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

TSSMat4 TSSMat4_CreateTranslation(TSSVec3 translation) {
    TSSMat4 m = TSSMat4_Identity();
    m.m[12] = translation.x;
    m.m[13] = translation.y;
    m.m[14] = translation.z;
    return m;
}

TSSMat4 TSSMat4_CreateScale(TSSVec3 scale) {
    TSSMat4 m = {0};
    m.m[0] = scale.x;
    m.m[5] = scale.y;
    m.m[10] = scale.z;
    m.m[15] = 1.0f;
    return m;
}

TSSMat4 TSSMat4_CreateRotation(TSSQuat rotation) {
    TSSMat4 m = TSSQuat_ToMat4(rotation);
    m.m[15] = 1.0f;
    return m;
}

TSSMat4 TSSMat4_CreatePerspective(float fov, float aspect, float near, float far) {
    TSSMat4 m = {0};
    float tanHalfFov = tanf(fov * 0.5f);
    m.m[0] = 1.0f / (aspect * tanHalfFov);
    m.m[5] = 1.0f / tanHalfFov;
    m.m[10] = -(far + near) / (far - near);
    m.m[11] = -1.0f;
    m.m[14] = -(2.0f * far * near) / (far - near);
    return m;
}

TSSMat4 TSSMat4_LookAt(TSSVec3 eye, TSSVec3 target, TSSVec3 up) {
    TSSVec3 f = TSSVec3_Normalize(TSSVec3_Sub(target, eye));
    TSSVec3 r = TSSVec3_Normalize(TSSVec3_Cross(f, up));
    TSSVec3 u = TSSVec3_Cross(r, f);
    
    TSSMat4 m = TSSMat4_Identity();
    m.m[0] = r.x; m.m[4] = r.y; m.m[8] = r.z;
    m.m[1] = u.x; m.m[5] = u.y; m.m[9] = u.z;
    m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
    m.m[12] = -TSSVec3_Dot(r, eye);
    m.m[13] = -TSSVec3_Dot(u, eye);
    m.m[14] = TSSVec3_Dot(f, eye);
    return m;
}

TSSMat4 TSSMat4_Multiply(TSSMat4 a, TSSMat4 b) {
    TSSMat4 result = {0};
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result.m[i * 4 + j] = 
                a.m[i * 4 + 0] * b.m[0 * 4 + j] +
                a.m[i * 4 + 1] * b.m[1 * 4 + j] +
                a.m[i * 4 + 2] * b.m[2 * 4 + j] +
                a.m[i * 4 + 3] * b.m[3 * 4 + j];
        }
    }
    return result;
}

TSSVec3 TSSMat4_TransformPoint(TSSMat4 m, TSSVec3 p) {
    TSSVec3 result;
    result.x = m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12];
    result.y = m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13];
    result.z = m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14];
    float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];
    if ((w > 0.00001f) || (w < -0.00001f)) {
        result.x /= w; result.y /= w; result.z /= w;
    }
    return result;
}

TSSVec3 TSSMat4_TransformVector(TSSMat4 m, TSSVec3 v) {
    TSSVec3 result;
    result.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z;
    result.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z;
    result.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z;
    return result;
}

TSSMat4 TSSMat4_Invert(TSSMat4 m) {
    TSSMat4 result = {0};
    float inv[16], det;
    int i;
    
    inv[0] = m.m[5]*m.m[10]*m.m[15] - m.m[5]*m.m[11]*m.m[14] - m.m[9]*m.m[6]*m.m[15] + m.m[9]*m.m[7]*m.m[14] + m.m[13]*m.m[6]*m.m[11] - m.m[13]*m.m[7]*m.m[10];
    inv[4] = -m.m[4]*m.m[10]*m.m[15] + m.m[4]*m.m[11]*m.m[14] + m.m[8]*m.m[6]*m.m[15] - m.m[8]*m.m[7]*m.m[14] - m.m[12]*m.m[6]*m.m[11] + m.m[12]*m.m[7]*m.m[10];
    inv[8] = m.m[4]*m.m[9]*m.m[15] - m.m[4]*m.m[11]*m.m[13] - m.m[8]*m.m[5]*m.m[15] + m.m[8]*m.m[7]*m.m[13] + m.m[12]*m.m[5]*m.m[11] - m.m[12]*m.m[7]*m.m[9];
    inv[12] = -m.m[4]*m.m[9]*m.m[14] + m.m[4]*m.m[10]*m.m[13] + m.m[8]*m.m[5]*m.m[14] - m.m[8]*m.m[6]*m.m[13] - m.m[12]*m.m[5]*m.m[10] + m.m[12]*m.m[6]*m.m[9];
    inv[1] = -m.m[1]*m.m[10]*m.m[15] + m.m[1]*m.m[11]*m.m[14] + m.m[9]*m.m[2]*m.m[15] - m.m[9]*m.m[3]*m.m[14] - m.m[13]*m.m[2]*m.m[11] + m.m[13]*m.m[3]*m.m[10];
    inv[5] = m.m[0]*m.m[10]*m.m[15] - m.m[0]*m.m[11]*m.m[14] - m.m[8]*m.m[2]*m.m[15] + m.m[8]*m.m[3]*m.m[14] + m.m[12]*m.m[2]*m.m[11] - m.m[12]*m.m[3]*m.m[10];
    inv[9] = -m.m[0]*m.m[9]*m.m[15] + m.m[0]*m.m[11]*m.m[13] + m.m[8]*m.m[1]*m.m[15] - m.m[8]*m.m[3]*m.m[13] - m.m[12]*m.m[1]*m.m[11] + m.m[12]*m.m[3]*m.m[9];
    inv[13] = m.m[0]*m.m[9]*m.m[14] - m.m[0]*m.m[10]*m.m[13] - m.m[8]*m.m[1]*m.m[14] + m.m[8]*m.m[2]*m.m[13] + m.m[12]*m.m[1]*m.m[10] - m.m[12]*m.m[2]*m.m[9];
    inv[2] = m.m[1]*m.m[6]*m.m[15] - m.m[1]*m.m[7]*m.m[14] - m.m[5]*m.m[2]*m.m[15] + m.m[5]*m.m[3]*m.m[14] + m.m[13]*m.m[2]*m.m[7] - m.m[13]*m.m[3]*m.m[6];
    inv[6] = -m.m[0]*m.m[6]*m.m[15] + m.m[0]*m.m[7]*m.m[14] + m.m[4]*m.m[2]*m.m[15] - m.m[4]*m.m[3]*m.m[14] - m.m[12]*m.m[2]*m.m[7] + m.m[12]*m.m[3]*m.m[6];
    inv[10] = m.m[0]*m.m[5]*m.m[15] - m.m[0]*m.m[7]*m.m[13] - m.m[4]*m.m[1]*m.m[15] + m.m[4]*m.m[3]*m.m[13] + m.m[12]*m.m[1]*m.m[7] - m.m[12]*m.m[3]*m.m[5];
    inv[14] = -m.m[0]*m.m[5]*m.m[14] + m.m[0]*m.m[6]*m.m[13] + m.m[4]*m.m[1]*m.m[14] - m.m[4]*m.m[2]*m.m[13] - m.m[12]*m.m[1]*m.m[6] + m.m[12]*m.m[2]*m.m[5];
    inv[3] = -m.m[1]*m.m[6]*m.m[11] + m.m[1]*m.m[7]*m.m[10] + m.m[5]*m.m[2]*m.m[11] - m.m[5]*m.m[3]*m.m[10] - m.m[9]*m.m[2]*m.m[7] + m.m[9]*m.m[3]*m.m[6];
    inv[7] = m.m[0]*m.m[6]*m.m[11] - m.m[0]*m.m[7]*m.m[10] - m.m[4]*m.m[2]*m.m[11] + m.m[4]*m.m[3]*m.m[10] + m.m[8]*m.m[2]*m.m[7] - m.m[8]*m.m[3]*m.m[6];
    inv[11] = -m.m[0]*m.m[5]*m.m[11] + m.m[0]*m.m[7]*m.m[9] + m.m[4]*m.m[1]*m.m[11] - m.m[4]*m.m[3]*m.m[9] - m.m[8]*m.m[1]*m.m[7] + m.m[8]*m.m[3]*m.m[5];
    inv[15] = m.m[0]*m.m[5]*m.m[10] - m.m[0]*m.m[6]*m.m[9] - m.m[4]*m.m[1]*m.m[10] + m.m[4]*m.m[2]*m.m[9] + m.m[8]*m.m[1]*m.m[6] - m.m[8]*m.m[2]*m.m[5];
    
    det = m.m[0]*inv[0] + m.m[1]*inv[4] + m.m[2]*inv[8] + m.m[3]*inv[12];
    if (((det > -0.00001f) && (det < 0.00001f))) return TSSMat4_Identity();
    
    det = 1.0f / det;
    for (i = 0; i < 16; i++) result.m[i] = inv[i] * det;
    return result;
}

TSSMat4 TSSMat4_Transpose(TSSMat4 m) {
    TSSMat4 result;
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result.m[i * 4 + j] = m.m[j * 4 + i];
        }
    }
    return result;
}

void TSSTransform_SetIdentity(TSSTransform* t) {
    t->position = TSSVec3_Zero();
    t->rotation = TSSQuat_Identity();
    t->scale = TSSVec3_Create(1, 1, 1);
}

void TSSTransform_Set(TSSTransform* t, TSSVec3 pos, TSSQuat rot, TSSVec3 scale) {
    t->position = pos;
    t->rotation = rot;
    t->scale = scale;
}

TSSMat4 TSSTransform_ToMatrix(TSSTransform* t) {
    TSSMat4 scale = TSSMat4_CreateScale(t->scale);
    TSSMat4 rot = TSSMat4_CreateRotation(t->rotation);
    TSSMat4 trans = TSSMat4_CreateTranslation(t->position);
    TSSMat4 temp = TSSMat4_Multiply(rot, scale);
    return TSSMat4_Multiply(trans, temp);
}

TSSVec3 TSSTransform_Forward(TSSTransform* t) {
    return TSSQuat_Rotate(t->rotation, TSSVec3_Create(0, 0, -1));
}

TSSVec3 TSSTransform_Up(TSSTransform* t) {
    return TSSQuat_Rotate(t->rotation, TSSVec3_Create(0, 1, 0));
}

TSSVec3 TSSTransform_Right(TSSTransform* t) {
    return TSSQuat_Rotate(t->rotation, TSSVec3_Create(1, 0, 0));
}

void TSSEntity3D_Init(TSSEntity3D* entity, unsigned int id) {
    memset(entity, 0, sizeof(TSSEntity3D));
    entity->id = id;
    entity->active = 1;
    entity->kinematic = 0;
    entity->wasTeleported = 0;
    entity->current.position = TSSVec3_Zero();
    entity->current.rotation = TSSQuat_Identity();
    entity->current.scale = TSSVec3_Create(1, 1, 1);
    entity->previous = entity->current;
    entity->velocity = TSSVec3_Zero();
    entity->angularVelocity = TSSVec3_Zero();
    entity->acceleration = TSSVec3_Zero();
}

void TSSEntity3D_SetPosition(TSSEntity3D* entity, TSSVec3 pos) {
    entity->previous.position = entity->current.position;
    entity->current.position = pos;
}

void TSSEntity3D_SetRotation(TSSEntity3D* entity, TSSQuat rot) {
    entity->previous.rotation = entity->current.rotation;
    entity->current.rotation = rot;
}

void TSSEntity3D_SetVelocity(TSSEntity3D* entity, TSSVec3 vel) {
    entity->velocity = vel;
}

void TSSEntity3D_ApplyForce(TSSEntity3D* entity, TSSVec3 force, float mass) {
    entity->acceleration = TSSVec3_Add(entity->acceleration, TSSVec3_Div(force, mass));
}

void TSSEntity3D_UpdatePhysics(TSSEntity3D* entity, float dt) {
    if ((!entity->active) || (entity->kinematic)) return;
    
    entity->velocity = TSSVec3_Add(entity->velocity, TSSVec3_Mul(entity->acceleration, dt));
    entity->velocity = TSSVec3_Mul(entity->velocity, 0.995f);
    
    entity->previous.position = entity->current.position;
    entity->current.position = TSSVec3_Add(entity->current.position, TSSVec3_Mul(entity->velocity, dt));
    
    entity->acceleration = TSSVec3_Zero();
}

void TSSEntity3D_SaveState(TSSEntity3D* entity) {
    entity->previous = entity->current;
}

void TSSEntity3D_DetectTeleport(TSSEntity3D* entity, float threshold) {
    float dist = TSSVec3_Length(TSSVec3_Sub(entity->current.position, entity->previous.position));
    entity->wasTeleported = (dist > threshold) ? 1 : 0;
}

TSSTransform TSSWorld3D_InterpolateTransform(TSSEntity3D* entity, float alpha, int useSlerp) {
    TSSTransform result;
    
    result.position = TSSVec3_Lerp(entity->previous.position, entity->current.position, alpha);
    
    if ((entity->wasTeleported) || (!useSlerp)) {
        result.rotation = TSSQuat_Slerp(entity->previous.rotation, entity->current.rotation, alpha);
    } else {
        result.rotation = entity->current.rotation;
    }
    
    result.scale = TSSVec3_Lerp(entity->previous.scale, entity->current.scale, alpha);
    
    if ((!entity->wasTeleported) && (!entity->kinematic)) {
        TSSVec3 extrapolatedPos = TSSVec3_Extrapolate(
            result.position, entity->velocity, alpha * entity->velocity.x * 0.01f
        );
        float weight = alpha * 0.3f;
        result.position = TSSVec3_Lerp(result.position, extrapolatedPos, weight);
    }
    
    return result;
}

TSSWorld3D* TSSWorld3D_Create(unsigned int maxEntities, float physicsHz, float renderHz) {
    TSSWorld3D* world = (TSSWorld3D*)calloc(1, sizeof(TSSWorld3D));
    if (!world) return NULL;
    
    world->entities = (TSSEntity3D*)calloc(maxEntities, sizeof(TSSEntity3D));
    if (!world->entities) {
        free(world);
        return NULL;
    }
    
    world->maxEntities = maxEntities;
    world->physicsFrequency = physicsHz;
    world->renderFrequency = renderHz;
    world->fixedDeltaTime = 1.0f / physicsHz;
    world->accumulator = 0.0f;
    
    {
        unsigned int i;
        for (i = 0; i < maxEntities; i++) {
            TSSEntity3D_Init(&world->entities[i], i);
            world->entities[i].active = 0;
        }
    }
    
    return world;
}

void TSSWorld3D_Destroy(TSSWorld3D* world) {
    if (!world) return;
    if (world->entities) free(world->entities);
    free(world);
}

TSSEntity3D* TSSWorld3D_AddEntity(TSSWorld3D* world, TSSVec3 pos, TSSQuat rot, TSSVec3 scale) {
    if (!world) return NULL;
    
    {
        unsigned int i;
        for (i = 0; i < world->maxEntities; i++) {
            if (!world->entities[i].active) {
                TSSEntity3D* e = &world->entities[i];
                e->active = 1;
                e->current.position = pos;
                e->current.rotation = rot;
                e->current.scale = scale;
                e->previous = e->current;
                world->activeCount++;
                return e;
            }
        }
    }
    return NULL;
}

void TSSWorld3D_RemoveEntity(TSSWorld3D* world, unsigned int id) {
    if (!world || id >= world->maxEntities) return;
    if (world->entities[id].active) {
        world->entities[id].active = 0;
        world->activeCount--;
    }
}

void TSSWorld3D_UpdatePhysics(TSSWorld3D* world, float deltaTime) {
    if (!world) return;
    
    world->accumulator += deltaTime;
    int maxSteps = 10;
    int steps = 0;
    
    while ((world->accumulator >= world->fixedDeltaTime) && (steps < maxSteps)) {
        unsigned int i;
        for (i = 0; i < world->maxEntities; i++) {
            TSSEntity3D* e = &world->entities[i];
            if (!e->active) continue;
            TSSEntity3D_UpdatePhysics(e, world->fixedDeltaTime);
        }
        world->accumulator -= world->fixedDeltaTime;
        steps++;
    }
    
    {
        unsigned int i;
        for (i = 0; i < world->maxEntities; i++) {
            if (world->entities[i].active) {
                TSSEntity3D_DetectTeleport(&world->entities[i], 100.0f);
            }
        }
    }
    
    world->lastPhysicsTime += world->fixedDeltaTime * steps;
}

void TSSWorld3D_UpdateRender(TSSWorld3D* world, float alpha) {
    if (!world) return;
    unsigned int i;
    for (i = 0; i < world->maxEntities; i++) {
        if (world->entities[i].active) {
            world->entities[i].current = TSSWorld3D_InterpolateTransform(
                &world->entities[i], alpha, 1
            );
        }
    }
    world->lastRenderTime = alpha;
}

TSSCamera3D* TSSCamera3D_Create(void) {
    TSSCamera3D* camera = (TSSCamera3D*)calloc(1, sizeof(TSSCamera3D));
    if (!camera) return NULL;
    
    camera->position = TSSVec3_Create(0, 2, 5);
    camera->rotation = TSSQuat_Identity();
    camera->forward = TSSVec3_Create(0, 0, -1);
    camera->up = TSSVec3_Create(0, 1, 0);
    camera->right = TSSVec3_Create(1, 0, 0);
    camera->fov = 60.0f * M_PI_F / 180.0f;
    camera->aspectRatio = 16.0f / 9.0f;
    camera->nearPlane = 0.1f;
    camera->farPlane = 1000.0f;
    camera->dirty = 1;
    
    return camera;
}

void TSSCamera3D_Destroy(TSSCamera3D* camera) {
    free(camera);
}

void TSSCamera3D_SetPosition(TSSCamera3D* camera, TSSVec3 pos) {
    camera->position = pos;
    camera->dirty = 1;
}

void TSSCamera3D_SetRotation(TSSCamera3D* camera, TSSQuat rot) {
    camera->rotation = rot;
    camera->dirty = 1;
}

void TSSCamera3D_SetFov(TSSCamera3D* camera, float fov) {
    camera->fov = fov * M_PI_F / 180.0f;
    camera->dirty = 1;
}

void TSSCamera3D_SetAspectRatio(TSSCamera3D* camera, float aspect) {
    camera->aspectRatio = aspect;
    camera->dirty = 1;
}

void TSSCamera3D_SetPlanes(TSSCamera3D* camera, float near, float far) {
    camera->nearPlane = near;
    camera->farPlane = far;
    camera->dirty = 1;
}

void TSSCamera3D_LookAt(TSSCamera3D* camera, TSSVec3 target) {
    TSSVec3 forward = TSSVec3_Normalize(TSSVec3_Sub(target, camera->position));
    TSSVec3 worldUp = TSSVec3_Create(0, 1, 0);
    
    if ((TSSVec3_Dot(forward, worldUp) > 0.99f) || (TSSVec3_Dot(forward, worldUp) < -0.99f)) {
        worldUp = TSSVec3_Create(0, 0, 1);
    }
    
    TSSVec3 right = TSSVec3_Normalize(TSSVec3_Cross(forward, worldUp));
    TSSVec3 up = TSSVec3_Cross(right, forward);
    
    camera->forward = forward;
    camera->up = up;
    camera->right = right;
    camera->dirty = 1;
}

void TSSCamera3D_Move(TSSCamera3D* camera, TSSVec3 delta) {
    camera->position = TSSVec3_Add(camera->position, delta);
    camera->dirty = 1;
}

void TSSCamera3D_Rotate(TSSCamera3D* camera, TSSQuat delta) {
    camera->rotation = TSSQuat_Multiply(delta, camera->rotation);
    camera->dirty = 1;
}

void TSSCamera3D_UpdateMatrices(TSSCamera3D* camera) {
    if (!camera->dirty) return;
    
    camera->forward = TSSQuat_Rotate(camera->rotation, TSSVec3_Create(0, 0, -1));
    camera->up = TSSQuat_Rotate(camera->rotation, TSSVec3_Create(0, 1, 0));
    camera->right = TSSQuat_Rotate(camera->rotation, TSSVec3_Create(1, 0, 0));
    
    TSSVec3 target = TSSVec3_Add(camera->position, camera->forward);
    camera->viewMatrix = TSSMat4_LookAt(camera->position, target, camera->up);
    
    camera->projectionMatrix = TSSMat4_CreatePerspective(
        camera->fov, camera->aspectRatio, camera->nearPlane, camera->farPlane
    );
    
    camera->viewProjectionMatrix = TSSMat4_Multiply(camera->projectionMatrix, camera->viewMatrix);
    
    camera->dirty = 0;
}

TSSVec3 TSSCamera3D_ScreenToWorld(TSSCamera3D* camera, TSSVec2 screenPos, float depth) {
    TSSCamera3D_UpdateMatrices(camera);
    
    float x = (2.0f * screenPos.x / 1920.0f) - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y / 1080.0f);
    
    TSSMat4 invVP = TSSMat4_Invert(camera->viewProjectionMatrix);
    
    TSSVec4 clipPos = {x, y, depth * 2.0f - 1.0f, 1.0f};
    
    TSSVec4 worldPos;
    worldPos.x = invVP.m[0] * clipPos.x + invVP.m[4] * clipPos.y + invVP.m[8] * clipPos.z + invVP.m[12] * clipPos.w;
    worldPos.y = invVP.m[1] * clipPos.x + invVP.m[5] * clipPos.y + invVP.m[9] * clipPos.z + invVP.m[13] * clipPos.w;
    worldPos.z = invVP.m[2] * clipPos.x + invVP.m[6] * clipPos.y + invVP.m[10] * clipPos.z + invVP.m[14] * clipPos.w;
    worldPos.w = invVP.m[3] * clipPos.x + invVP.m[7] * clipPos.y + invVP.m[11] * clipPos.z + invVP.m[15] * clipPos.w;
    
    if (((worldPos.w > 0.00001f) || (worldPos.w < -0.00001f))) {
        worldPos.x /= worldPos.w;
        worldPos.y /= worldPos.w;
        worldPos.z /= worldPos.w;
    }
    
    TSSVec3 result = {worldPos.x, worldPos.y, worldPos.z};
    return result;
}

TSSVec2 TSSCamera3D_WorldToScreen(TSSCamera3D* camera, TSSVec3 worldPos) {
    TSSCamera3D_UpdateMatrices(camera);
    
    TSSVec4 clipPos;
    clipPos.x = camera->viewProjectionMatrix.m[0] * worldPos.x + camera->viewProjectionMatrix.m[4] * worldPos.y + camera->viewProjectionMatrix.m[8] * worldPos.z + camera->viewProjectionMatrix.m[12];
    clipPos.y = camera->viewProjectionMatrix.m[1] * worldPos.x + camera->viewProjectionMatrix.m[5] * worldPos.y + camera->viewProjectionMatrix.m[9] * worldPos.z + camera->viewProjectionMatrix.m[13];
    clipPos.z = camera->viewProjectionMatrix.m[2] * worldPos.x + camera->viewProjectionMatrix.m[6] * worldPos.y + camera->viewProjectionMatrix.m[10] * worldPos.z + camera->viewProjectionMatrix.m[14];
    clipPos.w = camera->viewProjectionMatrix.m[3] * worldPos.x + camera->viewProjectionMatrix.m[7] * worldPos.y + camera->viewProjectionMatrix.m[11] * worldPos.z + camera->viewProjectionMatrix.m[15];
    
    TSSVec3 ndc = TSSVec3_Create(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);
    
    TSSVec2 screen;
    screen.x = (ndc.x + 1.0f) * 0.5f * 1920.0f;
    screen.y = (1.0f - ndc.y) * 0.5f * 1080.0f;
    
    return screen;
}

struct TSSFrameGenerator3DImpl {
    TSSWorld3D* world;
    TSSCamera3D* camera;
    float latencyMs;
    float cameraAlpha;
    TSSVec3 cameraPrevPos;
    TSSQuat cameraPrevRot;
};

TSSFrameGenerator3D TSSCreateFrameGenerator3D(unsigned int maxEntities, float physicsHz, float renderHz) {
    struct TSSFrameGenerator3DImpl* fg = (struct TSSFrameGenerator3DImpl*)calloc(1, sizeof(struct TSSFrameGenerator3DImpl));
    if (!fg) return NULL;
    
    fg->world = TSSWorld3D_Create(maxEntities, physicsHz, renderHz);
    if (!fg->world) {
        free(fg);
        return NULL;
    }
    
    fg->camera = TSSCamera3D_Create();
    if (!fg->camera) {
        TSSWorld3D_Destroy(fg->world);
        free(fg);
        return NULL;
    }
    
    fg->latencyMs = 1000.0f / physicsHz;
    fg->cameraAlpha = 0.0f;
    fg->cameraPrevPos = TSSVec3_Zero();
    fg->cameraPrevRot = TSSQuat_Identity();
    
    return (TSSFrameGenerator3D)fg;
}

void TSSDestroyFrameGenerator3D(TSSFrameGenerator3D fg) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    if (impl->camera) TSSCamera3D_Destroy(impl->camera);
    if (impl->world) TSSWorld3D_Destroy(impl->world);
    free(impl);
}

void TSSFG3D_AddEntity(TSSFrameGenerator3D fg, TSSVec3 pos, TSSVec3 vel, float mass) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    TSSEntity3D* e = TSSWorld3D_AddEntity(impl->world, pos, TSSQuat_Identity(), TSSVec3_Create(1, 1, 1));
    if (e) {
        e->velocity = vel;
    }
    (void)mass;
}

void TSSFG3D_SetEntityPosition(TSSFrameGenerator3D fg, unsigned int id, TSSVec3 pos) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    if (id < impl->world->maxEntities && impl->world->entities[id].active) {
        TSSEntity3D_SetPosition(&impl->world->entities[id], pos);
    }
}

void TSSFG3D_SetEntityRotation(TSSFrameGenerator3D fg, unsigned int id, TSSQuat rot) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    if (id < impl->world->maxEntities && impl->world->entities[id].active) {
        TSSEntity3D_SetRotation(&impl->world->entities[id], rot);
    }
}

void TSSFG3D_SetEntityVelocity(TSSFrameGenerator3D fg, unsigned int id, TSSVec3 vel) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    if (id < impl->world->maxEntities && impl->world->entities[id].active) {
        TSSEntity3D_SetVelocity(&impl->world->entities[id], vel);
    }
}

void TSSFG3D_UpdatePhysics(TSSFrameGenerator3D fg, float deltaTime) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    TSSWorld3D_UpdatePhysics(impl->world, deltaTime);
}

void TSSFG3D_Interpolate(TSSFrameGenerator3D fg, float alpha) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    
    float renderAlpha = impl->world->accumulator / impl->world->fixedDeltaTime;
    if (renderAlpha > 1.0f) renderAlpha = 1.0f;
    
    TSSWorld3D_UpdateRender(impl->world, renderAlpha);
    impl->cameraAlpha = renderAlpha;
}

TSSEntity3D* TSSFG3D_GetEntity(TSSFrameGenerator3D fg, unsigned int index) {
    if (!fg) return NULL;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    if (index >= impl->world->maxEntities) return NULL;
    return &impl->world->entities[index];
}

int TSSFG3D_GetEntityCount(TSSFrameGenerator3D fg) {
    if (!fg) return 0;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return (int)impl->world->activeCount;
}

float TSSFG3D_GetLatencyMs(TSSFrameGenerator3D fg) {
    if (!fg) return 0;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return impl->latencyMs;
}

int TSSFG3D_GetPhysicsHz(TSSFrameGenerator3D fg) {
    if (!fg) return 0;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return (int)impl->world->physicsFrequency;
}

int TSSFG3D_GetRenderHz(TSSFrameGenerator3D fg) {
    if (!fg) return 0;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return (int)impl->world->renderFrequency;
}

TSSCamera3D* TSSFG3D_GetCamera(TSSFrameGenerator3D fg) {
    if (!fg) return NULL;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return impl->camera;
}

void TSSFG3D_SetCameraTransform(TSSFrameGenerator3D fg, TSSVec3 pos, TSSQuat rot) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    impl->cameraPrevPos = impl->camera->position;
    impl->cameraPrevRot = impl->camera->rotation;
    TSSCamera3D_SetPosition(impl->camera, pos);
    TSSCamera3D_SetRotation(impl->camera, rot);
}

void TSSFG3D_UpdateCamera(TSSFrameGenerator3D fg, float alpha) {
    if (!fg) return;
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    
    TSSVec3 interpPos = TSSVec3_Lerp(impl->cameraPrevPos, impl->camera->position, alpha);
    TSSQuat interpRot = TSSQuat_Slerp(impl->cameraPrevRot, impl->camera->rotation, alpha);
    
    impl->camera->position = interpPos;
    impl->camera->rotation = interpRot;
    impl->camera->dirty = 1;
    
    TSSCamera3D_UpdateMatrices(impl->camera);
}

TSSMat4 TSSFG3D_GetViewMatrix(TSSFrameGenerator3D fg) {
    if (!fg) return TSSMat4_Identity();
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return impl->camera->viewMatrix;
}

TSSMat4 TSSFG3D_GetProjectionMatrix(TSSFrameGenerator3D fg) {
    if (!fg) return TSSMat4_Identity();
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return impl->camera->projectionMatrix;
}

TSSMat4 TSSFG3D_GetViewProjectionMatrix(TSSFrameGenerator3D fg) {
    if (!fg) return TSSMat4_Identity();
    struct TSSFrameGenerator3DImpl* impl = (struct TSSFrameGenerator3DImpl*)fg;
    return impl->camera->viewProjectionMatrix;
}
