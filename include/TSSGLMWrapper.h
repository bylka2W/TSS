#ifndef TSS_GLM_WRAPPER_H
#define TSS_GLM_WRAPPER_H

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_SWIZZLE

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON 1.1920928955078125e-7f
#endif

#define TSS_ALIGNED(x) __declspec(align(x))

typedef struct {
    float x, y;
} TSSVec2;

typedef struct {
    float x, y, z;
} TSSVec3;

typedef struct {
    float x, y, z, w;
} TSSVec4;

typedef struct {
    float m[4][4];
} TSSMat4;

typedef struct {
    float m[3][3];
} TSSMat3;

TSSVec3 TSSVec3_Make(float x, float y, float z);
TSSVec3 TSSVec3_Zero(void);
TSSVec3 TSSVec3_One(void);
TSSVec3 TSSVec3_Add(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Sub(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Mul(TSSVec3 v, float scalar);
TSSVec3 TSSVec3_Div(TSSVec3 v, float scalar);
TSSVec3 TSSVec3_Neg(TSSVec3 v);
float TSSVec3_Dot(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Cross(TSSVec3 a, TSSVec3 b);
float TSSVec3_LengthSq(TSSVec3 v);
float TSSVec3_Length(TSSVec3 v);
TSSVec3 TSSVec3_Normalize(TSSVec3 v);
TSSVec3 TSSVec3_NormalizeFast(TSSVec3 v);
TSSVec3 TSSVec3_Lerp(TSSVec3 a, TSSVec3 b, float t);
TSSVec3 TSSVec3_SmoothStep(TSSVec3 a, TSSVec3 b, float t);
TSSVec3 TSSVec3_Floor(TSSVec3 v);
TSSVec3 TSSVec3_Ceil(TSSVec3 v);
TSSVec3 TSSVec3_Fract(TSSVec3 v);
TSSVec3 TSSVec3_Abs(TSSVec3 v);
TSSVec3 TSSVec3_Min(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Max(TSSVec3 a, TSSVec3 b);
TSSVec3 TSSVec3_Clamp(TSSVec3 v, TSSVec3 minVal, TSSVec3 maxVal);
TSSVec3 TSSVec3_Sqrt(TSSVec3 v);
TSSVec3 TSSVec3_Radians(TSSVec3 degrees);
TSSVec3 TSSVec3_Degrees(TSSVec3 radians);
TSSVec3 TSSVec3_Sin(TSSVec3 v);
TSSVec3 TSSVec3_Cos(TSSVec3 v);
TSSVec3 TSSVec3_Exp(TSSVec3 v);
TSSVec3 TSSVec3_Log(TSSVec3 v);
TSSVec3 TSSVec3_Exp2(TSSVec3 v);
TSSVec3 TSSVec3_Log2(TSSVec3 v);
TSSVec3 TSSVec3_Pow(TSSVec3 v, float exponent);
TSSVec3 TSSVec3_PowVec(TSSVec3 base, TSSVec3 exponent);

TSSMat4 TSSMat4_Identity(void);
TSSMat4 TSSMat4_Mul(TSSMat4 a, TSSMat4 b);
TSSVec4 TSSMat4_MulVec4(TSSMat4 m, TSSVec4 v);
TSSVec3 TSSMat4_MulVec3(TSSMat4 m, TSSVec3 v);
TSSMat4 TSSMat4_Transpose(TSSMat4 m);
TSSMat4 TSSMat4_Invert(TSSMat4 m);
float TSSMat4_Determinant(TSSMat4 m);

TSSMat4 TSSMat4_Translate(TSSVec3 translation);
TSSMat4 TSSMat4_Scale(TSSVec3 scale);
TSSMat4 TSSMat4_ScaleF(float scale);
TSSMat4 TSSMat4_RotateX(float angle);
TSSMat4 TSSMat4_RotateY(float angle);
TSSMat4 TSSMat4_RotateZ(float angle);
TSSMat4 TSSMat4_RotateAxis(TSSVec3 axis, float angle);
TSSMat4 TSSMat4_RotateYawPitchRoll(float yaw, float pitch, float roll);

TSSMat4 TSSMat4_LookAt(TSSVec3 eye, TSSVec3 center, TSSVec3 up);
TSSMat4 TSSMat4_Perspective(float fovY, float aspect, float near, float far);
TSSMat4 TSSMat4_Ortho(float left, float right, float bottom, float top, float near, float far);
TSSMat4 TSSMat4_InfinitePerspective(float fovY, float aspect, float near);

TSSMat4 TSSMat4_ExtrapolateMotion(TSSMat4 current, TSSMat4 previous, float deltaTime, float velocityScale);

TSSVec2 TSSVec2_Make(float x, float y);
TSSVec2 TSSVec2_Zero(void);
TSSVec2 TSSVec2_Add(TSSVec2 a, TSSVec2 b);
TSSVec2 TSSVec2_Sub(TSSVec2 a, TSSVec2 b);
TSSVec2 TSSVec2_Mul(TSSVec2 v, float scalar);
float TSSVec2_Dot(TSSVec2 a, TSSVec2 b);
float TSSVec2_Length(TSSVec2 v);
TSSVec2 TSSVec2_Normalize(TSSVec2 v);
TSSVec2 TSSVec2_Lerp(TSSVec2 a, TSSVec2 b, float t);
TSSVec2 TSSVec2_Clamp(TSSVec2 v, TSSVec2 minVal, TSSVec2 maxVal);

TSSVec4 TSSVec4_Make(float x, float y, float z, float w);
TSSVec4 TSSVec4_Zero(void);
TSSVec4 TSSVec4_Add(TSSVec4 a, TSSVec4 b);
TSSVec4 TSSVec4_Sub(TSSVec4 a, TSSVec4 b);
TSSVec4 TSSVec4_Mul(TSSVec4 v, float scalar);
float TSSVec4_Dot(TSSVec4 a, TSSVec4 b);
TSSVec4 TSSVec4_Normalize(TSSVec4 v);
TSSVec4 TSSVec4_Lerp(TSSVec4 a, TSSVec4 b, float t);

float TSSClamp(float v, float minVal, float maxVal);
float TSSLerp(float a, float b, float t);
float TSSSaturate(float v);

typedef struct {
    TSSVec3 position;
    TSSVec3 scale;
    TSSVec3 rotation;
} TSSTransform3D;

TSSTransform3D TSSTransform3D_Make(TSSVec3 position, TSSVec3 scale, TSSVec3 rotation);
TSSMat4 TSSTransform3D_ToMatrix(TSSTransform3D t);
TSSTransform3D TSSTransform3D_Lerp(TSSTransform3D a, TSSTransform3D b, float t);

#ifdef __cplusplus
}
#endif

#endif
