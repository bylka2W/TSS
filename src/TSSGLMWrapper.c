#include "TSSGLMWrapper.h"
#include <math.h>
#include <string.h>

#ifndef FLT_EPSILON
#define FLT_EPSILON 1.1920928955078125e-7f
#endif

TSSVec3 TSSVec3_Make(float x, float y, float z) {
    TSSVec3 v = {x, y, z};
    return v;
}

TSSVec3 TSSVec3_Zero(void) {
    TSSVec3 v = {0.0f, 0.0f, 0.0f};
    return v;
}

TSSVec3 TSSVec3_One(void) {
    TSSVec3 v = {1.0f, 1.0f, 1.0f};
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
    TSSVec3 result = {v.x / scalar, v.y / scalar, v.z / scalar};
    return result;
}

TSSVec3 TSSVec3_Neg(TSSVec3 v) {
    TSSVec3 result = {-v.x, -v.y, -v.z};
    return result;
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

float TSSVec3_LengthSq(TSSVec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float TSSVec3_Length(TSSVec3 v) {
    return sqrtf(TSSVec3_LengthSq(v));
}

TSSVec3 TSSVec3_Normalize(TSSVec3 v) {
    float len = TSSVec3_Length(v);
    if (len < FLT_EPSILON) {
        return TSSVec3_Zero();
    }
    return TSSVec3_Div(v, len);
}

TSSVec3 TSSVec3_NormalizeFast(TSSVec3 v) {
    float lenSq = TSSVec3_LengthSq(v);
    if (lenSq < FLT_EPSILON) {
        return TSSVec3_Zero();
    }
    float invLen = 1.0f / sqrtf(lenSq);
    return TSSVec3_Mul(v, invLen);
}

TSSVec3 TSSVec3_Lerp(TSSVec3 a, TSSVec3 b, float t) {
    TSSVec3 v;
    v.x = a.x + (b.x - a.x) * t;
    v.y = a.y + (b.y - a.y) * t;
    v.z = a.z + (b.z - a.z) * t;
    return v;
}

TSSVec3 TSSVec3_SmoothStep(TSSVec3 a, TSSVec3 b, float t) {
    float tClamped = TSSClamp(t, 0.0f, 1.0f);
    tClamped = tClamped * tClamped * (3.0f - 2.0f * tClamped);
    return TSSVec3_Lerp(a, b, tClamped);
}

TSSVec3 TSSVec3_Floor(TSSVec3 v) {
    TSSVec3 result = {floorf(v.x), floorf(v.y), floorf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Ceil(TSSVec3 v) {
    TSSVec3 result = {ceilf(v.x), ceilf(v.y), ceilf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Fract(TSSVec3 v) {
    TSSVec3 result = {v.x - floorf(v.x), v.y - floorf(v.y), v.z - floorf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Abs(TSSVec3 v) {
    TSSVec3 result = {fabsf(v.x), fabsf(v.y), fabsf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Min(TSSVec3 a, TSSVec3 b) {
    TSSVec3 result;
    result.x = a.x < b.x ? a.x : b.x;
    result.y = a.y < b.y ? a.y : b.y;
    result.z = a.z < b.z ? a.z : b.z;
    return result;
}

TSSVec3 TSSVec3_Max(TSSVec3 a, TSSVec3 b) {
    TSSVec3 result;
    result.x = a.x > b.x ? a.x : b.x;
    result.y = a.y > b.y ? a.y : b.y;
    result.z = a.z > b.z ? a.z : b.z;
    return result;
}

TSSVec3 TSSVec3_Clamp(TSSVec3 v, TSSVec3 minVal, TSSVec3 maxVal) {
    return TSSVec3_Min(TSSVec3_Max(v, minVal), maxVal);
}

TSSVec3 TSSVec3_Sqrt(TSSVec3 v) {
    TSSVec3 result = {sqrtf(v.x), sqrtf(v.y), sqrtf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Radians(TSSVec3 degrees) {
    float degToRad = M_PI_F / 180.0f;
    return TSSVec3_Mul(degrees, degToRad);
}

TSSVec3 TSSVec3_Degrees(TSSVec3 radians) {
    float radToDeg = 180.0f / M_PI_F;
    return TSSVec3_Mul(radians, radToDeg);
}

TSSVec3 TSSVec3_Sin(TSSVec3 v) {
    TSSVec3 result = {sinf(v.x), sinf(v.y), sinf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Cos(TSSVec3 v) {
    TSSVec3 result = {cosf(v.x), cosf(v.y), cosf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Exp(TSSVec3 v) {
    TSSVec3 result = {expf(v.x), expf(v.y), expf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Log(TSSVec3 v) {
    TSSVec3 result = {logf(v.x), logf(v.y), logf(v.z)};
    return result;
}

TSSVec3 TSSVec3_Exp2(TSSVec3 v) {
    TSSVec3 result = {exp2f(v.x), exp2f(v.y), exp2f(v.z)};
    return result;
}

TSSVec3 TSSVec3_Log2(TSSVec3 v) {
    TSSVec3 result = {log2f(v.x), log2f(v.y), log2f(v.z)};
    return result;
}

TSSVec3 TSSVec3_Pow(TSSVec3 v, float exponent) {
    TSSVec3 result = {powf(v.x, exponent), powf(v.y, exponent), powf(v.z, exponent)};
    return result;
}

TSSVec3 TSSVec3_PowVec(TSSVec3 base, TSSVec3 exponent) {
    TSSVec3 result = {powf(base.x, exponent.x), powf(base.y, exponent.y), powf(base.z, exponent.z)};
    return result;
}

TSSMat4 TSSMat4_Identity(void) {
    TSSMat4 m;
    memset(&m, 0, sizeof(TSSMat4));
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

TSSMat4 TSSMat4_Mul(TSSMat4 a, TSSMat4 b) {
    TSSMat4 result;
    int i, j, k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            float sum = 0.0f;
            for (k = 0; k < 4; k++) {
                sum += a.m[i][k] * b.m[k][j];
            }
            result.m[i][j] = sum;
        }
    }
    return result;
}

TSSVec4 TSSMat4_MulVec4(TSSMat4 m, TSSVec4 v) {
    TSSVec4 result;
    result.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
    result.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
    result.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
    result.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;
    return result;
}

TSSVec3 TSSMat4_MulVec3(TSSMat4 m, TSSVec3 v) {
    TSSVec3 result;
    result.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3];
    result.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3];
    result.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3];
    return result;
}

TSSMat4 TSSMat4_Transpose(TSSMat4 m) {
    TSSMat4 result;
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result.m[i][j] = m.m[j][i];
        }
    }
    return result;
}

float TSSMat4_Determinant(TSSMat4 m) {
    float det = 0.0f;
    int i;
    for (i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        int k = (i + 2) % 4;
        int l = (i + 3) % 4;
        det += m.m[0][i] * (m.m[1][j] * (m.m[2][k] * m.m[3][l] - m.m[2][l] * m.m[3][k])
                           - m.m[1][k] * (m.m[2][j] * m.m[3][l] - m.m[2][l] * m.m[3][j])
                           + m.m[1][l] * (m.m[2][j] * m.m[3][k] - m.m[2][k] * m.m[3][j]));
    }
    return det;
}

TSSMat4 TSSMat4_Invert(TSSMat4 m) {
    TSSMat4 inv;
    float invdet;
    float det = TSSMat4_Determinant(m);
    if (det < FLT_EPSILON) {
        return TSSMat4_Identity();
    }
    invdet = 1.0f / det;
    
    inv.m[0][0] = (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2])
                 - m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1])
                 + m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) * invdet;
    inv.m[0][1] = -(m.m[0][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2])
                  - m.m[0][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1])
                  + m.m[0][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) * invdet;
    inv.m[0][2] = (m.m[0][1] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2])
                 - m.m[0][2] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1])
                 + m.m[0][3] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1])) * invdet;
    inv.m[0][3] = -(m.m[0][1] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2])
                  - m.m[0][2] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1])
                  + m.m[0][3] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1])) * invdet;
    inv.m[1][0] = -(m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2])
                  - m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0])
                  + m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) * invdet;
    inv.m[1][1] = (m.m[0][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2])
                 - m.m[0][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0])
                 + m.m[0][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) * invdet;
    inv.m[1][2] = -(m.m[0][0] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2])
                  - m.m[0][2] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0])
                  + m.m[0][3] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0])) * invdet;
    inv.m[1][3] = (m.m[0][0] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2])
                 - m.m[0][2] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0])
                 + m.m[0][3] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0])) * invdet;
    inv.m[2][0] = (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1])
                 - m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0])
                 + m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) * invdet;
    inv.m[2][1] = -(m.m[0][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1])
                  - m.m[0][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0])
                  + m.m[0][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) * invdet;
    inv.m[2][2] = (m.m[0][0] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1])
                 - m.m[0][1] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0])
                 + m.m[0][3] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0])) * invdet;
    inv.m[2][3] = -(m.m[0][0] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1])
                  - m.m[0][1] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0])
                  + m.m[0][3] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0])) * invdet;
    inv.m[3][0] = -(m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])
                  - m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])
                  + m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) * invdet;
    inv.m[3][1] = (m.m[0][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])
                 - m.m[0][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])
                 + m.m[0][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) * invdet;
    inv.m[3][2] = -(m.m[0][0] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1])
                  - m.m[0][1] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0])
                  + m.m[0][2] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0])) * invdet;
    inv.m[3][3] = (m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1])
                 - m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0])
                 + m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0])) * invdet;
    
    return inv;
}

TSSMat4 TSSMat4_Translate(TSSVec3 translation) {
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][3] = translation.x;
    m.m[1][3] = translation.y;
    m.m[2][3] = translation.z;
    return m;
}

TSSMat4 TSSMat4_Scale(TSSVec3 scale) {
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = scale.x;
    m.m[1][1] = scale.y;
    m.m[2][2] = scale.z;
    return m;
}

TSSMat4 TSSMat4_ScaleF(float scale) {
    return TSSMat4_Scale(TSSVec3_Make(scale, scale, scale));
}

TSSMat4 TSSMat4_RotateX(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    TSSMat4 m = TSSMat4_Identity();
    m.m[1][1] = c;
    m.m[1][2] = s;
    m.m[2][1] = -s;
    m.m[2][2] = c;
    return m;
}

TSSMat4 TSSMat4_RotateY(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = c;
    m.m[0][2] = -s;
    m.m[2][0] = s;
    m.m[2][2] = c;
    return m;
}

TSSMat4 TSSMat4_RotateZ(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = c;
    m.m[0][1] = s;
    m.m[1][0] = -s;
    m.m[1][1] = c;
    return m;
}

TSSMat4 TSSMat4_RotateAxis(TSSVec3 axis, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1.0f - c;
    TSSVec3 a = TSSVec3_Normalize(axis);
    
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = t * a.x * a.x + c;
    m.m[0][1] = t * a.x * a.y + s * a.z;
    m.m[0][2] = t * a.x * a.z - s * a.y;
    m.m[1][0] = t * a.x * a.y - s * a.z;
    m.m[1][1] = t * a.y * a.y + c;
    m.m[1][2] = t * a.y * a.z + s * a.x;
    m.m[2][0] = t * a.x * a.z + s * a.y;
    m.m[2][1] = t * a.y * a.z - s * a.x;
    m.m[2][2] = t * a.z * a.z + c;
    return m;
}

TSSMat4 TSSMat4_RotateYawPitchRoll(float yaw, float pitch, float roll) {
    TSSMat4 m = TSSMat4_Identity();
    TSSMat4 rotY = TSSMat4_RotateY(yaw);
    TSSMat4 rotX = TSSMat4_RotateX(pitch);
    TSSMat4 rotZ = TSSMat4_RotateZ(roll);
    m = TSSMat4_Mul(m, rotY);
    m = TSSMat4_Mul(m, rotX);
    m = TSSMat4_Mul(m, rotZ);
    return m;
}

TSSMat4 TSSMat4_LookAt(TSSVec3 eye, TSSVec3 center, TSSVec3 up) {
    TSSVec3 f = TSSVec3_Normalize(TSSVec3_Sub(center, eye));
    TSSVec3 s = TSSVec3_Normalize(TSSVec3_Cross(f, up));
    TSSVec3 u = TSSVec3_Cross(s, f);
    
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = s.x;
    m.m[1][0] = s.y;
    m.m[2][0] = s.z;
    m.m[0][1] = u.x;
    m.m[1][1] = u.y;
    m.m[2][1] = u.z;
    m.m[0][2] = -f.x;
    m.m[1][2] = -f.y;
    m.m[2][2] = -f.z;
    m.m[0][3] = -TSSVec3_Dot(s, eye);
    m.m[1][3] = -TSSVec3_Dot(u, eye);
    m.m[2][3] = TSSVec3_Dot(f, eye);
    
    return m;
}

TSSMat4 TSSMat4_Perspective(float fovY, float aspect, float near, float far) {
    float tanHalfFov = tanf(fovY * 0.5f);
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = 1.0f / (aspect * tanHalfFov);
    m.m[1][1] = 1.0f / tanHalfFov;
    m.m[2][2] = -(far + near) / (far - near);
    m.m[2][3] = -(2.0f * far * near) / (far - near);
    m.m[3][2] = -1.0f;
    m.m[3][3] = 0.0f;
    return m;
}

TSSMat4 TSSMat4_Ortho(float left, float right, float bottom, float top, float near, float far) {
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = 2.0f / (right - left);
    m.m[1][1] = 2.0f / (top - bottom);
    m.m[2][2] = -2.0f / (far - near);
    m.m[0][3] = -(right + left) / (right - left);
    m.m[1][3] = -(top + bottom) / (top - bottom);
    m.m[2][3] = -(far + near) / (far - near);
    return m;
}

TSSMat4 TSSMat4_InfinitePerspective(float fovY, float aspect, float near) {
    float tanHalfFov = tanf(fovY * 0.5f);
    TSSMat4 m = TSSMat4_Identity();
    m.m[0][0] = 1.0f / (aspect * tanHalfFov);
    m.m[1][1] = 1.0f / tanHalfFov;
    m.m[2][2] = -1.0f;
    m.m[2][3] = -2.0f * near;
    m.m[3][2] = -1.0f;
    return m;
}

TSSMat4 TSSMat4_ExtrapolateMotion(TSSMat4 current, TSSMat4 previous, float deltaTime, float velocityScale) {
    TSSMat4 invPrev = TSSMat4_Invert(previous);
    TSSMat4 delta = TSSMat4_Mul(current, invPrev);
    
    float dt = deltaTime * velocityScale;
    
    TSSMat4 extrapolated = current;
    extrapolated.m[3][0] += delta.m[3][0] * dt;
    extrapolated.m[3][1] += delta.m[3][1] * dt;
    extrapolated.m[3][2] += delta.m[3][2] * dt;
    
    return extrapolated;
}

TSSVec2 TSSVec2_Make(float x, float y) {
    TSSVec2 v = {x, y};
    return v;
}

TSSVec2 TSSVec2_Zero(void) {
    TSSVec2 v = {0.0f, 0.0f};
    return v;
}

TSSVec2 TSSVec2_Add(TSSVec2 a, TSSVec2 b) {
    TSSVec2 v = {a.x + b.x, a.y + b.y};
    return v;
}

TSSVec2 TSSVec2_Sub(TSSVec2 a, TSSVec2 b) {
    TSSVec2 v = {a.x - b.x, a.y - b.y};
    return v;
}

TSSVec2 TSSVec2_Mul(TSSVec2 v, float scalar) {
    TSSVec2 result = {v.x * scalar, v.y * scalar};
    return result;
}

float TSSVec2_Dot(TSSVec2 a, TSSVec2 b) {
    return a.x * b.x + a.y * b.y;
}

float TSSVec2_Length(TSSVec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

TSSVec2 TSSVec2_Normalize(TSSVec2 v) {
    float len = TSSVec2_Length(v);
    if (len < FLT_EPSILON) {
        return TSSVec2_Zero();
    }
    return TSSVec2_Mul(v, 1.0f / len);
}

TSSVec2 TSSVec2_Lerp(TSSVec2 a, TSSVec2 b, float t) {
    TSSVec2 v;
    v.x = a.x + (b.x - a.x) * t;
    v.y = a.y + (b.y - a.y) * t;
    return v;
}

TSSVec2 TSSVec2_Clamp(TSSVec2 v, TSSVec2 minVal, TSSVec2 maxVal) {
    TSSVec2 result;
    result.x = v.x < minVal.x ? minVal.x : (v.x > maxVal.x ? maxVal.x : v.x);
    result.y = v.y < minVal.y ? minVal.y : (v.y > maxVal.y ? maxVal.y : v.y);
    return result;
}

TSSVec4 TSSVec4_Make(float x, float y, float z, float w) {
    TSSVec4 v = {x, y, z, w};
    return v;
}

TSSVec4 TSSVec4_Zero(void) {
    TSSVec4 v = {0.0f, 0.0f, 0.0f, 0.0f};
    return v;
}

TSSVec4 TSSVec4_Add(TSSVec4 a, TSSVec4 b) {
    TSSVec4 v = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    return v;
}

TSSVec4 TSSVec4_Sub(TSSVec4 a, TSSVec4 b) {
    TSSVec4 v = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    return v;
}

TSSVec4 TSSVec4_Mul(TSSVec4 v, float scalar) {
    TSSVec4 result = {v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar};
    return result;
}

float TSSVec4_Dot(TSSVec4 a, TSSVec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

TSSVec4 TSSVec4_Normalize(TSSVec4 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    if (len < FLT_EPSILON) {
        return TSSVec4_Zero();
    }
    return TSSVec4_Mul(v, 1.0f / len);
}

TSSVec4 TSSVec4_Lerp(TSSVec4 a, TSSVec4 b, float t) {
    TSSVec4 v;
    v.x = a.x + (b.x - a.x) * t;
    v.y = a.y + (b.y - a.y) * t;
    v.z = a.z + (b.z - a.z) * t;
    v.w = a.w + (b.w - a.w) * t;
    return v;
}

float TSSClamp(float v, float minVal, float maxVal) {
    return v < minVal ? minVal : (v > maxVal ? maxVal : v);
}

float TSSLerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float TSSSaturate(float v) {
    return TSSClamp(v, 0.0f, 1.0f);
}

TSSTransform3D TSSTransform3D_Make(TSSVec3 position, TSSVec3 scale, TSSVec3 rotation) {
    TSSTransform3D t;
    t.position = position;
    t.scale = scale;
    t.rotation = rotation;
    return t;
}

TSSMat4 TSSTransform3D_ToMatrix(TSSTransform3D t) {
    TSSMat4 scale = TSSMat4_Scale(t.scale);
    TSSMat4 rot = TSSMat4_RotateYawPitchRoll(t.rotation.y, t.rotation.x, t.rotation.z);
    TSSMat4 trans = TSSMat4_Translate(t.position);
    TSSMat4 m = TSSMat4_Identity();
    m = TSSMat4_Mul(m, trans);
    m = TSSMat4_Mul(m, rot);
    m = TSSMat4_Mul(m, scale);
    return m;
}

TSSTransform3D TSSTransform3D_Lerp(TSSTransform3D a, TSSTransform3D b, float t) {
    TSSTransform3D result;
    result.position = TSSVec3_Lerp(a.position, b.position, t);
    result.scale = TSSVec3_Lerp(a.scale, b.scale, t);
    result.rotation = TSSVec3_Lerp(a.rotation, b.rotation, t);
    return result;
}
