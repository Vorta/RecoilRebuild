#pragma once

#include "GameZRecoil/zMath/zMathTypes.h"
#include "recoil/recoil_callconv.h"

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

void __fastcall zMath_Mat_TransformBBoxToCorners(
    const zMat4x3 *matrix,
    const zBBox3f *bbox,
    zBBoxCorners *outCorners
);

void __fastcall zMath_Vec3_DivScalar(
    const zVec3 *vec,
    zVec3 *out,
    float scalar
);

void __fastcall zMath_Vec3Array_AddScaled(
    zVec3 *outArray,
    const zVec3 *biasArray,
    const zVec3 *srcArray,
    int count,
    float scale
);
void __fastcall zMath_Vec3_TriangleNormal(
    const zVec3 *p0,
    const zVec3 *p1,
    const zVec3 *p2,
    zVec3 *outNormal
);
float __fastcall zMath_Vec3_ElevationAngleBetweenPoints(
    const zVec3 *pointA,
    const zVec3 *pointB
);
void __fastcall zMath_SolveLinearGradient2D(
    float *outDuDx,
    float *outDuDy,
    float ax,
    float ay,
    float bx,
    float by,
    float cx,
    float cy,
    float ua,
    float ub,
    float uc
);
void __fastcall zMath_BuildPerspectiveTextureInterpolants(
    const zVec3 *triVerts,
    const zVec2 *triUVs,
    zVec2 *outRecipZGrad,
    float *outRecipZBase,
    zVec2 *outUOverZGrad,
    float *outUOverZBase,
    zVec2 *outVOverZGrad,
    float *outVOverZBase
);
void __fastcall zMath_UnprojectPointBatch(
    const zProjectedPoint *projectedPoints,
    zVec3 *outPoints,
    int count
);
void __fastcall zMath_UnprojectPointBatchZBuf(
    const zProjectedPoint *projectedPoints,
    zVec3 *outPoints,
    int count
);
void __fastcall zMath_Mat_TransformNormalBatch(
    const zVec3 *normals,
    zVec3 *outNormals,
    int count
);
void __fastcall zMath_Vec3Array_UntransformDirection(
    zVec3 *vectors,
    int count
);
void __stdcall zMath_SetScreenSize(
    int screenWidthPx,
    int screenHeightPx
);
void __stdcall zMath_Setup_Projection(
    float viewportOriginX,
    float viewportOriginY,
    float halfViewWidthPx,
    float halfViewHeightPx,
    float focalScaleX,
    float focalScaleY,
    float clipDistance,
    float projDepth
);

void zMath_Mat_SetupCamera();
void zMath_Mat_LoadView();
void __stdcall zMath_Mat_LoadProjection(float zOffset);
zMat4x3 *zMath_Mat_GetCurrent();
int zMath_Mat_IsCurrentIdentity();
float __fastcall zMath_Mat_ExtractYaw(const zMat4x3 *matrix);
void __fastcall zMath_Mat_ExtractEulerAngles(
    const zMat4x3 *matrix,
    zVec3 *outEuler
);
void __fastcall zMath_Vec3_RotateX(
    zVec3 *outVec,
    const zVec3 *inVec,
    float angleX
);
void __fastcall zMath_Vec3_DirFromYaw(
    zVec3 *outDir,
    float yawAngle
);
void __fastcall zMath_Camera_StageInverseRotation(const zMat4x3 *worldMatrix);
void __fastcall zMath_Quat_FromEuler(
    zQuat *outQuat,
    float angle0,
    float angle1,
    float angle2
);
void __fastcall zMath_Quat_Multiply(
    const zQuat *quatA,
    const zQuat *quatB,
    zQuat *outAB
);
void __fastcall zMath_Quat_MultiplyInverse(
    const zQuat *quatA,
    const zQuat *quatB,
    zQuat *outAConjB
);
void __fastcall zMath_Quat_ToMatrix(
    const zQuat *quat,
    zMat4x3 *outMatrix3x3
);
void __fastcall zMath_Quat_FromRotationVector(
    const zVec3 *rotationVector,
    zQuat *outQuat
);
zVec2 zMath_Project_GetLastScreenScaleXY();

void zMath_Mat_Scale(
    float sx,
    float sy,
    float sz
);

namespace zMath {
extern zMat4x3 g_zMath_CameraScratchB;
extern zMat4x3 g_zMath_CameraScratchA;
extern zVec3 g_zMath_Vec3Zero;
extern zVec3 g_zMath_Vec3DeltaScratch;
extern int *g_currentMatrixIdentityFlagSlot;
extern float **g_currentMatrixPtrSlot;

int CrtMatherrHandler(_exception *except);
void __fastcall MatStackPushPtr(float *matrix);
void __fastcall MatStackPushAndCloneParent(float *newSlotBuffer);
void MatStackPopPtr();
void MatLoadCameraScratchB();
void MatLoadCameraScratchA();
void MatLoadIdentity();
inline float __fastcall Vec3Normalize(zVec3 *vec);
void __fastcall Vec3NormalizeXZ(
    zVec3 *vec,
    zVec3 *out
);
void __fastcall Vec3Perp2D(
    const zVec3 *in,
    zVec3 *out
);
void __fastcall Vec3PerpXZ(
    const zVec3 *in,
    zVec3 *out
);
void __fastcall Vec3ScaleAdd(
    const zVec3 *vec,
    const zVec3 *delta,
    float scale,
    zVec3 *out
);
void __fastcall Vec3Reflect(
    zVec3 *normal,
    zVec3 *incident,
    zVec3 *reflected
);
void __fastcall Vec3Lerp(
    zVec3 *inOut,
    const zVec3 *other,
    float t
);
void __fastcall Vec3LerpNormalize(
    zVec3 *inOut,
    const zVec3 *other,
    float t
);
float __fastcall Vec3DirectionTo(
    const zVec3 *from,
    const zVec3 *to,
    zVec3 *outDir
);
void __fastcall Vec3Slerp(
    const zVec3 *a,
    const zVec3 *b,
    float t,
    zVec3 *out
);
int __fastcall LineVsSphereHit(
    const zVec3 *segA,
    const zVec3 *segB,
    float radius,
    const zVec3 *sphereCenterRelSegB,
    zVec3 *outInwardNormal
);
zVec3 *__fastcall Vec3Midpoint(
    const zVec3 *a,
    const zVec3 *b,
    zVec3 *outMidpoint
);
float __fastcall Vec3DeltaLength(
    const zVec3 *a,
    const zVec3 *b
);
float __fastcall Vec3DeltaLengthSq(
    const zVec3 *a,
    const zVec3 *b
);
float __fastcall Vec3DistSqXZ(
    const zVec3 *a,
    const zVec3 *b
);
zMat4x3 *__stdcall MatCopyCurrentTo(zMat4x3 *out);
void __fastcall MatLoadCurrentFrom(const zMat4x3 *src);
void __fastcall MatLoadRotationFrom3x3(const zMat4x3 *src);
void __fastcall MatMultiply(
    const zMat4x3 *src,
    int mode
);
void __stdcall MatTranslate(
    float tx,
    float ty,
    float tz
);
void __stdcall MatRotateX(float angleRad);
void __stdcall MatRotateY(float angleRad);
void __stdcall MatRotateZ(float angleRad);
void __fastcall MatApplyLocalTRS(
    const zVec3 *angles,
    const zVec3 *position,
    const zVec3 *scale
);
void __fastcall MatBuildEulerRotation3x3(
    zMat4x3 *outBasis,
    float angleX,
    float angleY,
    float angleZ
);
zVec3 *__fastcall Vec3DirectionAnglesBetweenPoints(
    const zVec3 *pointA,
    const zVec3 *pointB,
    zVec3 *outAngles
);
void __fastcall Vec3ArrayProjectToCachedY(
    const zVec3 *points,
    float *outValues,
    int count
);
void __fastcall Vec3RotateY(
    zVec3 *outVec,
    const zVec3 *inVec,
    float yawAngle
);
void __fastcall Vec3ArrayTransformDirection(
    zVec3 *vectors,
    int count
);
void __fastcall MatTransformPointBatchInPlace(
    zVec3 *points,
    int count
);
void __fastcall ProjectPointBatch(
    const zVec3 *viewPoints,
    zProjectedPoint *projectedPoints,
    int count
);
int __fastcall ClipLineSegmentToZRange(
    zVec3 *pointA,
    zVec3 *pointB
);
void __fastcall ClipLineSegmentPointToZ(
    zVec3 *pointToClip,
    const zVec3 *otherPoint,
    float clipZ
);
int __fastcall ProjectPointAndClampToScreenClip(
    const zVec3 *srcPoint,
    zVec3 *dstPoint
);
float __stdcall ApproxExpNeg(float x);
} // namespace zMath

namespace zFloat {
void __fastcall Set255f(float *value);
}

void __fastcall zMath_ProjectSphereBatch(
    const zVec3 *spherePoints,
    zProjectedSphere *projectedSpheres,
    int count
);
