#pragma once

#include "recoil/recoil_callconv.h"

#include <stddef.h>
#include "recoil/recoil_types.h"

struct zVec3;

struct zVec2 {
    float x;
    float y;
};

struct zMat4x3 {
    float xx;
    float xy;
    float xz;
    float yx;
    float yy;
    float yz;
    float zx;
    float zy;
    float zz;
    float posX;
    float posY;
    float posZ;
};

RECOIL_STATIC_ASSERT(sizeof(zMat4x3) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zMat4x3, posX) == 0x24);

struct zQuat {
    float w;
    float x;
    float y;
    float z;
};

RECOIL_STATIC_ASSERT(sizeof(zQuat) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zQuat, x) == 0x04);

struct zBBox3f {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

struct zBBoxCorners {
    float values[24];
};

struct zProjectedPoint {
    float x;
    float y;
    float reciprocalZ;
};

struct zProjectedSphere {
    float x;
    float y;
    float screenRadius;
};

RECOIL_STATIC_ASSERT(sizeof(zBBox3f) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zBBoxCorners) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(zVec2) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zProjectedPoint) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zProjectedSphere) == 0x0c);

extern float g_zMath_ProjSphereRadiusScale;
extern float g_zMath_ProjScaleX;
extern float g_zMath_ProjScaleY;
extern float g_zMath_ProjOffsetX;
extern float g_zMath_ProjOffsetY;
extern float g_zMath_InvProjScaleX;
extern float g_zMath_InvProjScaleY;
extern int g_zMath_ScreenWidthPx;
extern int g_zMath_ScreenHeightPx;
extern float g_zMath_FocalScaleX;
extern float g_zMath_FocalScaleY;
extern float g_zMath_InvFocalScaleX;
extern float g_zMath_InvFocalScaleY;
extern float g_zMath_HalfViewWidth;
extern float g_zMath_HalfViewHeight;
extern float g_zMath_ViewportOriginX;
extern float g_zMath_ViewportOriginY;
extern float g_zMath_ProjDepth;
extern float g_zMath_ApproxExpNegTable[256];
extern float g_zMath_ApproxExpNegScale;
extern int g_zMath_ApproxExpNegDirty;

RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Mat_TransformBBoxToCorners(const zMat4x3 *matrix,
                                                                      const zBBox3f *bbox,
                                                                      zBBoxCorners *outCorners);

RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Vec3_DivScalar(const zVec3 *vec, zVec3 *out,
                                                          float scalar);

RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Vec3Array_AddScaled(zVec3 *outArray,
                                                               const zVec3 *biasArray,
                                                               const zVec3 *srcArray,
                                                               int count, float scale);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Vec3_TriangleNormal(const zVec3 *p0, const zVec3 *p1,
                                                               const zVec3 *p2, zVec3 *outNormal);
RECOIL_NOINLINE float RECOIL_FASTCALL zMath_Vec3_ElevationAngleBetweenPoints(const zVec3 *pointA,
                                                                             const zVec3 *pointB);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_SolveLinearGradient2D(float *outDuDx, float *outDuDy,
                                                                 float ax, float ay, float bx,
                                                                 float by, float cx, float cy,
                                                                 float ua, float ub, float uc);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_BuildPerspectiveTextureInterpolants(
    const zVec3 *triVerts, const zVec2 *triUVs, zVec2 *outRecipZGrad, float *outRecipZBase,
    zVec2 *outUOverZGrad, float *outUOverZBase, zVec2 *outVOverZGrad, float *outVOverZBase);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_UnprojectPointBatch(
    const zProjectedPoint *projectedPoints, zVec3 *outPoints, int count);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_UnprojectPointBatchZBuf(
    const zProjectedPoint *projectedPoints, zVec3 *outPoints, int count);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Mat_TransformNormalBatch(const zVec3 *normals,
                                                                    zVec3 *outNormals,
                                                                    int count);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Vec3Array_UntransformDirection(zVec3 *vectors,
                                                                          int count);
RECOIL_NOINLINE void RECOIL_STDCALL zMath_SetScreenSize(int screenWidthPx,
                                                        int screenHeightPx);
RECOIL_NOINLINE void RECOIL_STDCALL zMath_Setup_Projection(
    float viewportOriginX, float viewportOriginY, float halfViewWidthPx, float halfViewHeightPx,
    float focalScaleX, float focalScaleY, float clipDistance, float projDepth);

RECOIL_NOINLINE void RECOIL_CDECL zMath_Mat_SetupCamera();
RECOIL_NOINLINE void RECOIL_CDECL zMath_Mat_LoadView();
RECOIL_NOINLINE void RECOIL_STDCALL zMath_Mat_LoadProjection(float zOffset);
RECOIL_NOINLINE zMat4x3 *RECOIL_CDECL zMath_Mat_GetCurrent();
RECOIL_NOINLINE int RECOIL_CDECL zMath_Mat_IsCurrentIdentity();
RECOIL_NOINLINE float RECOIL_FASTCALL zMath_Mat_ExtractYaw(const zMat4x3 *matrix);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Mat_ExtractEulerAngles(const zMat4x3 *matrix,
                                                                  zVec3 *outEuler);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Vec3_RotateX(zVec3 *outVec, const zVec3 *inVec,
                                                        float angleX);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Vec3_DirFromYaw(zVec3 *outDir, float yawAngle);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Camera_StageInverseRotation(const zMat4x3 *worldMatrix);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Quat_FromEuler(zQuat *outQuat, float angle0,
                                                          float angle1, float angle2);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Quat_Multiply(const zQuat *quatA, const zQuat *quatB,
                                                         zQuat *outAB);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Quat_MultiplyInverse(const zQuat *quatA,
                                                                const zQuat *quatB,
                                                                zQuat *outAConjB);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Quat_ToMatrix(const zQuat *quat, zMat4x3 *outMatrix3x3);
RECOIL_NOINLINE void RECOIL_FASTCALL zMath_Quat_FromRotationVector(const zVec3 *rotationVector,
                                                                   zQuat *outQuat);
RECOIL_NOINLINE zVec2 RECOIL_CDECL zMath_Project_GetLastScreenScaleXY();

RECOIL_NOINLINE void RECOIL_CDECL zMath_Mat_Scale(float sx, float sy, float sz);

namespace zMath {
extern zMat4x3 g_zMath_CameraScratchB;
extern zMat4x3 g_zMath_CameraScratchA;
extern zVec3 g_zMath_Vec3DeltaScratch;
extern int *g_currentMatrixIdentityFlagSlot;
extern float **g_currentMatrixPtrSlot;

RECOIL_NOINLINE void RECOIL_FASTCALL MatStackPushPtr(float *matrix);
RECOIL_NOINLINE void RECOIL_FASTCALL MatStackPushAndCloneParent(float *newSlotBuffer);
RECOIL_NOINLINE void RECOIL_CDECL MatStackPopPtr();
RECOIL_NOINLINE void RECOIL_CDECL MatLoadCameraScratchB();
RECOIL_NOINLINE void RECOIL_CDECL MatLoadCameraScratchA();
RECOIL_NOINLINE void RECOIL_CDECL MatLoadIdentity();
RECOIL_NOINLINE float RECOIL_FASTCALL Vec3Normalize(zVec3 *vec);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3NormalizeXZ(zVec3 *vec, zVec3 *out);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3Perp2D(const zVec3 *in, zVec3 *out);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3PerpXZ(const zVec3 *in, zVec3 *out);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3ScaleAdd(const zVec3 *vec, const zVec3 *delta,
                                                  float scale, zVec3 *out);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3Reflect(zVec3 *normal, zVec3 *incident,
                                                 zVec3 *reflected);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3Lerp(zVec3 *inOut, const zVec3 *other,
                                              float t);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3LerpNormalize(zVec3 *inOut, const zVec3 *other,
                                                       float t);
RECOIL_NOINLINE float RECOIL_FASTCALL Vec3DirectionTo(const zVec3 *from, const zVec3 *to,
                                                      zVec3 *outDir);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3Slerp(const zVec3 *a, const zVec3 *b, float t,
                                               zVec3 *out);
RECOIL_NOINLINE int RECOIL_FASTCALL LineVsSphereHit(const zVec3 *segA, const zVec3 *segB,
                                                    float radius,
                                                    const zVec3 *sphereCenterRelSegB,
                                                    zVec3 *outInwardNormal);
RECOIL_NOINLINE zVec3 *RECOIL_FASTCALL Vec3Midpoint(const zVec3 *a, const zVec3 *b,
                                                    zVec3 *outMidpoint);
RECOIL_NOINLINE float RECOIL_FASTCALL Vec3DeltaLength(const zVec3 *a, const zVec3 *b);
RECOIL_NOINLINE float RECOIL_FASTCALL Vec3DeltaLengthSq(const zVec3 *a, const zVec3 *b);
RECOIL_NOINLINE float RECOIL_FASTCALL Vec3DistSqXZ(const zVec3 *a, const zVec3 *b);
RECOIL_NOINLINE zMat4x3 *RECOIL_STDCALL MatCopyCurrentTo(zMat4x3 *out);
RECOIL_NOINLINE void RECOIL_FASTCALL MatLoadCurrentFrom(const zMat4x3 *src);
RECOIL_NOINLINE void RECOIL_FASTCALL MatLoadRotationFrom3x3(const zMat4x3 *src);
RECOIL_NOINLINE void RECOIL_FASTCALL MatMultiply(const zMat4x3 *src, int mode);
RECOIL_NOINLINE void RECOIL_STDCALL MatTranslate(float tx, float ty, float tz);
RECOIL_NOINLINE void RECOIL_STDCALL MatRotateX(float angleRad);
RECOIL_NOINLINE void RECOIL_STDCALL MatRotateY(float angleRad);
RECOIL_NOINLINE void RECOIL_STDCALL MatRotateZ(float angleRad);
RECOIL_NOINLINE void RECOIL_FASTCALL MatApplyLocalTRS(const zVec3 *angles, const zVec3 *position,
                                                      const zVec3 *scale);
RECOIL_NOINLINE void RECOIL_FASTCALL MatBuildEulerRotation3x3(zMat4x3 *outBasis,
                                                              float angleX, float angleY,
                                                              float angleZ);
RECOIL_NOINLINE zVec3 *RECOIL_FASTCALL Vec3DirectionAnglesBetweenPoints(const zVec3 *pointA,
                                                                        const zVec3 *pointB,
                                                                        zVec3 *outAngles);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3ArrayProjectToCachedY(const zVec3 *points,
                                                               float *outValues,
                                                               int count);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3RotateY(zVec3 *outVec, const zVec3 *inVec, float yawAngle);
RECOIL_NOINLINE void RECOIL_FASTCALL Vec3ArrayTransformDirection(zVec3 *vectors,
                                                                 int count);
RECOIL_NOINLINE void RECOIL_FASTCALL MatTransformPointBatchInPlace(zVec3 *points,
                                                                   int count);
RECOIL_NOINLINE void RECOIL_FASTCALL ProjectPointBatch(const zVec3 *viewPoints,
                                                       zProjectedPoint *projectedPoints,
                                                       int count);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipLineSegmentToZRange(zVec3 *pointA, zVec3 *pointB);
RECOIL_NOINLINE void RECOIL_FASTCALL ClipLineSegmentPointToZ(zVec3 *pointToClip,
                                                             const zVec3 *otherPoint, float clipZ);
RECOIL_NOINLINE int RECOIL_FASTCALL ProjectPointAndClampToScreenClip(const zVec3 *srcPoint,
                                                                              zVec3 *dstPoint);
RECOIL_NOINLINE float RECOIL_STDCALL ApproxExpNeg(float x);
} // namespace zMath

namespace zFloat {
RECOIL_NOINLINE void RECOIL_FASTCALL Set255f(float *value);
}

RECOIL_NOINLINE void RECOIL_FASTCALL zMath_ProjectSphereBatch(const zVec3 *spherePoints,
                                                              zProjectedSphere *projectedSpheres,
                                                              int count);
