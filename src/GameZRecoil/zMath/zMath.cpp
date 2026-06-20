#include "GameZRecoil/zMath/zMath.h"

#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zClass.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

float g_zMath_ProjSphereRadiusScale = 0.0f;
float g_zMath_ProjScaleX = 0.0f;
float g_zMath_ProjScaleY = 0.0f;
float g_zMath_ProjOffsetX = 0.0f;
float g_zMath_ProjOffsetY = 0.0f;
float g_zMath_InvProjScaleX = 0.0f;
float g_zMath_InvProjScaleY = 0.0f;
/**
 * Reimplements data 0x4e4880: g_zMath_ClipZLowerBound.
 * Purpose: stores the mutable lower Z clipping plane used by the zMath line
 * segment clipping helpers.
 */
float g_zMath_ClipZLowerBound = 1.0f;
/**
 * Reimplements data 0x4e4890: g_zMath_ClipZUpperBound.
 * Purpose: stores the mutable upper Z clipping plane used by the zMath line
 * segment clipping helpers.
 */
float g_zMath_ClipZUpperBound = 1.0f;
/**
 * Reimplements data 0x4d08d4: shared zMath midpoint half scalar.
 * Purpose: supplies Vec3Midpoint's component scale after summing both source
 * vectors.
 */
const float g_zMath_MidpointHalf = 0.5f;
/**
 * Reimplements data 0x4d2918: shared zMath vector zero scalar.
 * Purpose: supplies float-zero comparisons for recovered vector helpers.
 */
const float g_zMath_Vec3ZeroFloat = 0.0f;
/**
 * Reimplements data 0x4d291c: shared zMath vector unit scalar.
 * Purpose: supplies reciprocal numerator constants for recovered vector
 * helpers.
 */
const float g_zMath_Vec3UnitFloat = 1.0f;
/**
 * Reimplements data 0x4d2928: shared zMath negative unit scalar.
 * Purpose: supplies the zero-dot reflection negation multiplier.
 */
const float g_zMath_Vec3NegUnitFloat = -1.0f;
const float g_zMath_MatrixUnitFloat = 1.0f; // Reimplements data 0x4d297c: shared zMath matrix/projection unit scalar.
/**
 * Reimplements data 0x4d2960: shared zMath double zero scalar.
 * Purpose: supplies the x87 zero comparisons used by zMath vector,
 * projection, and line/sphere intersection helpers.
 */
const double g_zMath_DoubleZero = 0.0;
/**
 * Reimplements data 0x4d2970: distinct shared zMath double zero scalar.
 * Purpose: supplies zMath_SolveLinearGradient2D's x87 double-zero comparison
 * for degenerate determinants.
 */
const double g_zMath_DoubleZero2 = 0.0;
const double g_zMath_Vec3SlerpDotNegThreshold = -0.95;
const float g_zMath_Vec3SlerpPiFloat = 3.14159274f;
const double g_zMath_Vec3SlerpDotPosThreshold = 0.95;
const float g_zMath_ElevationPiFloat = 3.14159274f; // Reimplements data 0x4d2998: Euler roll adjustment pi scalar.
int g_zMath_ScreenWidthPx = 0;
int g_zMath_ScreenHeightPx = 0;
float g_zMath_FocalScaleX = 0.0f;
float g_zMath_FocalScaleY = 0.0f;
float g_zMath_InvFocalScaleX = 0.0f;
float g_zMath_InvFocalScaleY = 0.0f;
float g_zMath_HalfViewWidth = 0.0f;
float g_zMath_HalfViewHeight = 0.0f;
float g_zMath_ViewportOriginX = 0.0f;
float g_zMath_ViewportOriginY = 0.0f;
float g_zMath_ProjDepth = 0.0f;
float g_zMath_ApproxExpNegTable[256] = {0};
float g_zMath_ApproxExpNegScale = 0.0f;
int g_zMath_ApproxExpNegDirty = 1;

// Reimplements 0x472d30: zMath::CrtMatherrHandler
// (D:\Proj\GameZRecoil\zMath\zmth_main.c)
int zMath::CrtMatherrHandler(
    _exception *except
) {
    zError::ReportOld(
        0x400,
        "D:\\Proj\\GameZRecoil\\zMath\\zmth_main.c",
        376,
        "Math Exception: type=%d, [%s(%.8f, %.8f)]",
        except->type,
        except->name,
        except->arg1,
        except->arg2
    );
    fprintf(
        stderr,
        "Math Exception: type=%d, [%s(%.8f, %.8f)]\n",
        except->type,
        except->name,
        except->arg1,
        except->arg2
    );

    if (strcmp(
        except->name,
        "asin"
    ) == 0) {
        double arg = except->arg1;
        if (arg > 1.0) {
            arg = 1.0;
        } else if (arg < -1.0 || arg != arg) {
            arg = -1.0;
        }
        except->retval = asin(arg);
        return 1;
    }

    if (strcmp(
        except->name,
        "ceil"
    ) == 0) {
        except->retval = 0.0;
        return 1;
    }

    if (strcmp(
        except->name,
        "floor"
    ) == 0) {
        except->retval = 0.0;
        return 1;
    }

    return 0;
}

namespace {
int g_matrixIdentityFlagSlots[32] = {0};
float *g_matrixSlots[32] = {0};

/**
 * Original static helper observed in callers 0x4753e0 and 0x475210
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: subtract two zVec3 values for triangle-gradient and intersection
 * vector math.
 */
zVec3 Subtract(
    const zVec3 &lhs,
    const zVec3 &rhs
) {
    zVec3 result = {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
    return result;
}

/**
 * Original static helper observed in caller 0x4753e0
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: compute a zVec3 cross product for perspective texture-gradient
 * setup.
 */
zVec3 Cross(
    const zVec3 &lhs,
    const zVec3 &rhs
) {
    zVec3 result = {lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x};
    return result;
}

/**
 * Original static helper observed in zMath vector and projection callers
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: compute the zVec3 dot product used by gradient, slerp, and
 * line/sphere routines.
 */
float Dot(
    const zVec3 &lhs,
    const zVec3 &rhs
) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Original static helper observed in caller 0x473e60
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: flip a float sign bit without changing the remaining bit pattern
 * while staging the inverse camera rotation.
 */
float NegateFloatSignBit(
    float value
) {
    unsigned int bits = 0;
    memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    bits ^= 0x80000000u;
    memcpy(
        &value,
        &bits,
        sizeof(value)
    );
    return value;
}

/**
 * Original static helper observed in zMath vector/intersection callers
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: recover the original fast square-root estimate from a float bit
 * pattern for vector normalization and hit tests.
 */
float FastSqrtEstimate(
    float value
) {
    unsigned int bits = 0;
    memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    bits = (bits >> 1) + 0x1fc00000u;
    memcpy(
        &value,
        &bits,
        sizeof(value)
    );
    return value;
}

/**
 * Original static helper observed in caller 0x4753e0
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: add two zVec3 values during perspective texture-gradient plane
 * construction.
 */
zVec3 Add(
    const zVec3 &lhs,
    const zVec3 &rhs
) {
    zVec3 result = {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
    return result;
}

/**
 * Original static helper observed in caller 0x4753e0
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: scale a zVec3 value during perspective texture-gradient plane
 * construction.
 */
zVec3 Scale(
    const zVec3 &value,
    float scale
) {
    zVec3 result = {value.x * scale, value.y * scale, value.z * scale};
    return result;
}

/**
 * Original static helper observed in caller 0x4753e0
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: build one perspective-correct UV-over-Z gradient plane and base
 * term from triangle geometry and reciprocal-Z gradients.
 */
void BuildUvOverZPlane(
    const zVec3 *triVerts,
    const zVec3 &edge21,
    const zVec3 &edge01,
    float edge21LenSq,
    float edge01LenSq,
    float edgeDotScaled,
    float invGram,
    const zVec2 &recipZGrad,
    float recipZBase,
    float uv0,
    float uv1,
    float uv2,
    zVec2 *outGrad,
    float *outBase
) {
    const float delta21 = uv2 - uv1;
    const float delta01 = uv0 - uv1;
    const zVec3 plane =
        Add(Scale(edge21, delta21 * edge01LenSq * invGram - delta01 * edgeDotScaled),
            Scale(
                edge01,
                delta01 * edge21LenSq * invGram - delta21 * edgeDotScaled
            ));
    const float originDelta = uv0 - Dot(
        plane,
        triVerts[0]
    );

    outGrad->x = originDelta * recipZGrad.x + plane.x * g_zMath_InvProjScaleX;
    outGrad->y = originDelta * recipZGrad.y + plane.y * g_zMath_InvProjScaleY;
    *outBase = originDelta * recipZBase + plane.z;
}

/**
 * Original static helper observed in caller 0x474870
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: transform one bounding-box corner by a 4x3 matrix into the caller's
 * corner output storage.
 */
void TransformBBoxCorner(
    const zMat4x3 *matrix,
    float x,
    float y,
    float z,
    float *out
) {
    out[0] = x * matrix->xx + y * matrix->yx + z * matrix->zx + matrix->posX;
    out[1] = x * matrix->xy + y * matrix->yy + z * matrix->zy + matrix->posY;
    out[2] = x * matrix->xz + y * matrix->yz + z * matrix->zz + matrix->posZ;
}
} // namespace

// Reimplements 0x474710: zMath_Mat_TransformNormalBatch
// (D:\Proj\GameZRecoil\zMath\zmath_matrix.cpp)
void __fastcall zMath_Mat_TransformNormalBatch(
    const zVec3 *normals,
    zVec3 *outNormals,
    int count
) {
    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        memcpy(
            outNormals,
            normals,
            count * sizeof(zVec3)
        );
        return;
    }

    if (count == 0) {
        return;
    }

    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    for (int i = 0; i < count; ++i) {
        const zVec3 normal = normals[i];
        outNormals[i].x = normal.x * matrix->xx + normal.y * matrix->yx + normal.z * matrix->zx;
        outNormals[i].z = normal.x * matrix->xz + normal.y * matrix->yz + normal.z * matrix->zz;
        outNormals[i].y = normal.x * matrix->xy + normal.y * matrix->yy + normal.z * matrix->zy;
    }
}

// Reimplements 0x4745e0: zMath_Vec3Array_UntransformDirection
// (D:\Proj\GameZRecoil\zMath\zmath_vec.cpp)
void __fastcall zMath_Vec3Array_UntransformDirection(
    zVec3 *vectors,
    int count
) {
    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        return;
    }

    if (count == 0) {
        return;
    }

    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    for (int i = 0; i < count; ++i) {
        const zVec3 vector = vectors[i];
        vectors[i].z = vector.x * matrix->xz + vector.y * matrix->yz + vector.z * matrix->zz;
        vectors[i].y = vector.x * matrix->xy + vector.y * matrix->yy + vector.z * matrix->zy;
        vectors[i].x = vector.x * matrix->xx + vector.y * matrix->yx + vector.z * matrix->zx;
    }
}

/**
 * Reimplements 0x4743e0: zMath_SetScreenSize (GameZRecoil/zMath/zmath_proj.cpp).
 * Purpose: Stores the active projection screen width and height globals.
 */
void __stdcall zMath_SetScreenSize(
    int screenWidthPx,
    int screenHeightPx
) {
    g_zMath_ScreenWidthPx = screenWidthPx;
    g_zMath_ScreenHeightPx = screenHeightPx;
}

/**
 * Reimplements 0x474400: zMath_Setup_Projection (GameZRecoil/zMath/zmath_proj.cpp).
 * Purpose: Derives cached projection scale, inverse scale, viewport, offset, radius-scale, and depth globals.
 */
void __stdcall zMath_Setup_Projection(
    float viewportOriginX,
    float viewportOriginY,
    float halfViewWidthPx,
    float halfViewHeightPx,
    float focalScaleX,
    float focalScaleY,
    float clipDistance,
    float projDepth
) {
    g_zMath_FocalScaleX = focalScaleX;
    g_zMath_FocalScaleY = focalScaleY;
    g_zMath_InvFocalScaleX = g_zMath_MatrixUnitFloat / focalScaleX;
    g_zMath_ProjScaleX = focalScaleX * halfViewWidthPx;
    g_zMath_ProjScaleY = focalScaleY * halfViewHeightPx;
    g_zMath_InvFocalScaleY = g_zMath_MatrixUnitFloat / focalScaleY;
    g_zMath_InvProjScaleX = g_zMath_MatrixUnitFloat / g_zMath_ProjScaleX;
    g_zMath_InvProjScaleY = g_zMath_MatrixUnitFloat / g_zMath_ProjScaleY;
    g_zMath_ProjOffsetX = halfViewWidthPx + viewportOriginX;
    g_zMath_ProjOffsetY = halfViewHeightPx + viewportOriginY;
    g_zMath_HalfViewWidth = halfViewWidthPx;
    g_zMath_HalfViewHeight = halfViewHeightPx;
    g_zMath_ViewportOriginX = viewportOriginX;
    g_zMath_ViewportOriginY = viewportOriginY;
    g_zMath_ProjSphereRadiusScale = clipDistance;
    g_zMath_ProjDepth = projDepth;
}

// Retail code keeps an EBP frame for this perspective-gradient helper under the
// VC5SP3 /O2 profile; disable only frame-pointer omission for the function.
#pragma optimize("y", off)
/**
 * Reimplements 0x4753e0: zMath_BuildPerspectiveTextureInterpolants
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: recovers perspective-correct reciprocal-Z and UV-over-Z plane gradients for a triangle.
 */
void __fastcall zMath_BuildPerspectiveTextureInterpolants(
    const zVec3 *triVerts,
    const zVec2 *triUVs,
    zVec2 *outRecipZGrad,
    float *outRecipZBase,
    zVec2 *outUOverZGrad,
    float *outUOverZBase,
    zVec2 *outVOverZGrad,
    float *outVOverZBase
) {
    const zVec3 edge21 = Subtract(
        triVerts[2],
        triVerts[1]
    );
    const zVec3 edge01 = Subtract(
        triVerts[0],
        triVerts[1]
    );
    const zVec3 normal = Cross(
        edge21,
        edge01
    );
    const float normalDotOrigin = Dot(
        normal,
        triVerts[0]
    );

    if (normalDotOrigin == 0.0f) {
        outRecipZGrad->x = 0.0f;
        outRecipZGrad->y = 0.0f;
        *outRecipZBase = 1000.0f;
    } else {
        const float reciprocalNormalDot = 1.0f / normalDotOrigin;
        outRecipZGrad->x = reciprocalNormalDot * normal.x * g_zMath_InvProjScaleX;
        outRecipZGrad->y = reciprocalNormalDot * normal.y * g_zMath_InvProjScaleY;
        *outRecipZBase = reciprocalNormalDot * normal.z;
    }

    const float edge21LenSq = Dot(
        edge21,
        edge21
    );
    const float edge01LenSq = Dot(
        edge01,
        edge01
    );
    const float edgeDot = Dot(
        edge21,
        edge01
    );
    const float gramDeterminant = edge01LenSq * edge21LenSq - edgeDot * edgeDot;
    if (gramDeterminant == 0.0f) {
        outUOverZGrad->x = 0.0f;
        outUOverZGrad->y = 0.0f;
        *outUOverZBase = 0.0f;
        outVOverZGrad->x = 0.0f;
        outVOverZGrad->y = 0.0f;
        *outVOverZBase = 0.0f;
        return;
    }

    const float invGram = 1.0f / gramDeterminant;
    const float edgeDotScaled = edgeDot * invGram;

    const float uDelta21 = triUVs[2].x - triUVs[1].x;
    const float uDelta01 = triUVs[0].x - triUVs[1].x;
    const float uScale21 = uDelta21 * edge01LenSq * invGram - uDelta01 * edgeDotScaled;
    const float uScale01 = uDelta01 * edge21LenSq * invGram - uDelta21 * edgeDotScaled;
    const zVec3 uPlane = {edge21.x * uScale21 + edge01.x * uScale01,
        edge21.y * uScale21 + edge01.y * uScale01,
        edge21.z * uScale21 + edge01.z * uScale01};
    const float uOriginDelta = triUVs[0].x - Dot(
        uPlane,
        triVerts[0]
    );

    outUOverZGrad->x = uOriginDelta * outRecipZGrad->x + uPlane.x * g_zMath_InvProjScaleX;
    outUOverZGrad->y = uOriginDelta * outRecipZGrad->y + uPlane.y * g_zMath_InvProjScaleY;
    *outUOverZBase = uOriginDelta * *outRecipZBase + uPlane.z;

    const float vDelta21 = triUVs[2].y - triUVs[1].y;
    const float vDelta01 = triUVs[0].y - triUVs[1].y;
    const float vScale21 = vDelta21 * edge01LenSq * invGram - vDelta01 * edgeDotScaled;
    const float vScale01 = vDelta01 * edge21LenSq * invGram - vDelta21 * edgeDotScaled;
    const zVec3 vPlane = {edge21.x * vScale21 + edge01.x * vScale01,
        edge21.y * vScale21 + edge01.y * vScale01,
        edge21.z * vScale21 + edge01.z * vScale01};
    const float vOriginDelta = triUVs[0].y - Dot(
        vPlane,
        triVerts[0]
    );

    outVOverZGrad->x = vOriginDelta * outRecipZGrad->x + vPlane.x * g_zMath_InvProjScaleX;
    outVOverZGrad->y = vOriginDelta * outRecipZGrad->y + vPlane.y * g_zMath_InvProjScaleY;
    *outVOverZBase = vOriginDelta * *outRecipZBase + vPlane.z;
}
#pragma optimize("y", on)

namespace zMath {
/**
 * Reimplements data 0x5668e8: g_zMath_CameraScratchB
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: stores the camera inverse-rotation scratch matrix loaded by the
 * camera setup/projection/view helpers.
 */
zMat4x3 g_zMath_CameraScratchB = {0};
/**
 * Reimplements data 0x566920: g_zMath_CameraScratchA
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: stores the staged camera world matrix before the inverse-rotation
 * transpose is copied into camera scratch B.
 */
zMat4x3 g_zMath_CameraScratchA = {0};
/**
 * Reimplements data 0x5669d8: zMath::g_zMath_Vec3Zero
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: shared writable zero vector read by zMath view-matrix setup and
 * projectile runtime initialization.
 */
zVec3 g_zMath_Vec3Zero = {0};
zVec3 g_zMath_Vec3DeltaScratch = {0};
int *g_currentMatrixIdentityFlagSlot = &g_matrixIdentityFlagSlots[0];
float **g_currentMatrixPtrSlot = &g_matrixSlots[0];

// Reimplements 0x472f30: zMath::MatStackPushPtr
void __fastcall MatStackPushPtr(
    float *matrix
) {
    ++g_currentMatrixIdentityFlagSlot;
    ++g_currentMatrixPtrSlot;
    *g_currentMatrixPtrSlot = matrix;
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x472ef0: zMath::MatStackPushAndCloneParent (GameZRecoil/zMath/zmath_matstack.cpp)
void __fastcall MatStackPushAndCloneParent(
    float *newSlotBuffer
) {
    ++g_currentMatrixIdentityFlagSlot;
    ++g_currentMatrixPtrSlot;
    *g_currentMatrixIdentityFlagSlot = g_currentMatrixIdentityFlagSlot[-1];
    *g_currentMatrixPtrSlot = newSlotBuffer;
    memcpy(
        *g_currentMatrixPtrSlot,
        g_currentMatrixPtrSlot[-1],
        sizeof(zMat4x3)
    );
}

// Reimplements 0x472f60: zMath::MatStackPopPtr
void MatStackPopPtr() {
    --g_currentMatrixIdentityFlagSlot;
    --g_currentMatrixPtrSlot;
}

/**
 * Reimplements 0x472f90: zMath::MatLoadCameraScratchB
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: loads camera scratch B into the current matrix stack slot.
 */
void MatLoadCameraScratchB() {
    MatLoadCurrentFrom(&g_zMath_CameraScratchB);
}

/**
 * Reimplements 0x472fa0: zMath::MatLoadCameraScratchA
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: loads camera scratch A into the current matrix stack slot.
 */
void MatLoadCameraScratchA() {
    MatLoadCurrentFrom(&g_zMath_CameraScratchA);
}

// Reimplements 0x4732f0: zMath::MatLoadIdentity
void MatLoadIdentity() {
    float *matrix = *g_currentMatrixPtrSlot;
    *matrix++ = 1.0f;
    *matrix++ = 0.0f;
    *matrix++ = 0.0f;
    *matrix++ = 0.0f;
    *matrix++ = 1.0f;
    *matrix++ = 0.0f;
    *matrix++ = 0.0f;
    *matrix++ = 0.0f;
    *matrix++ = 1.0f;
    *matrix++ = 0.0f;
    *matrix++ = 0.0f;
    *matrix = 0.0f;
    *g_currentMatrixIdentityFlagSlot = 1;
}

#pragma optimize("y", off)
/**
 * Reimplements 0x402f60: zMath::Vec3Normalize.
 * Purpose: Normalizes a nonzero vector in place and returns the original 3D length.
 */
float __fastcall Vec3Normalize(
    zVec3 *vec
) {
    zVec3 *inOut = vec;
    float length = sqrt(inOut->x * inOut->x + inOut->y * inOut->y + inOut->z * inOut->z);
    const unsigned int *lengthBits = (const unsigned int *)&length;
    if ((*lengthBits & 0x7fffffffu) != 0) {
        const float reciprocalLength = 1.0f / length;
        inOut->x *= reciprocalLength;
        inOut->y *= reciprocalLength;
        inOut->z *= reciprocalLength;
    }
    return length;
}
#pragma optimize("", on)

/**
 * Reimplements 0x4727f0: zMath::Vec3NormalizeXZ (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Normalizes a vector in the XZ plane while preserving the input Y value and leaving output Y untouched.
 */
void __fastcall Vec3NormalizeXZ(
    zVec3 *vec,
    zVec3 *out
) {
    const float savedY = vec->y;
    vec->y = 0.0f;
    const float length = sqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
    vec->y = savedY;

    float scale = length;
    if (length != g_zMath_DoubleZero) {
        scale = g_zMath_Vec3UnitFloat / length;
    }

    out->x = vec->x * scale;
    out->z = vec->z * scale;
}

/**
 * Reimplements 0x472cc0: zMath::Vec3Perp2D (GameZRecoil/zMath/zmath_vec2.cpp).
 * Purpose: Computes a unit XY-plane perpendicular using the recovered fast square-root estimate.
 */
void __fastcall Vec3Perp2D(
    const zVec3 *in,
    zVec3 *out
) {
    out->z = 0.0f;
    if (in->x == g_zMath_Vec3ZeroFloat) {
        out->x = 1.0f;
        out->y = 0.0f;
        return;
    }

    const float lengthSq = in->x * in->x + in->y * in->y;
    const float invLength = g_zMath_Vec3UnitFloat / FastSqrtEstimate(lengthSq);
    out->x = in->y * invLength;
    out->y = -(in->x * invLength);
}

// Reimplements 0x4745c0: zMath::Vec3PerpXZ
// (D:\Proj\GameZRecoil\zMath\zmath_vec.cpp)
void __fastcall Vec3PerpXZ(
    const zVec3 *in,
    zVec3 *out
) {
    out->x = -in->z;
    out->z = in->x;
    out->y = 0.0f;
}

/**
 * Reimplements 0x472770: zMath::Vec3ScaleAdd (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Computes out = vec + scale * delta for each vector component.
 * Data: reads only caller-supplied vector/scalar inputs and writes only the
 * caller-supplied output vector.
 */
void __fastcall Vec3ScaleAdd(
    const zVec3 *vec,
    const zVec3 *delta,
    float scale,
    zVec3 *out
) {
    out->x = scale * delta->x + vec->x;
    out->y = vec->y + delta->y * scale;
    out->z = vec->z + delta->z * scale;
}

/**
 * Reimplements 0x472860: zMath::Vec3Reflect (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Reflects an incident vector around a normal, with the zero-dot case negating the incident vector.
 * Data: reads shared zMath scalar constants 0x4d2918 and 0x4d2928; writes
 * only the caller-supplied output vector.
 */
void __fastcall Vec3Reflect(
    zVec3 *normal,
    zVec3 *incident,
    zVec3 *reflected
) {
    const float dot = normal->x * incident->x + normal->y * incident->y + normal->z * incident->z;
    if (dot == g_zMath_Vec3ZeroFloat) {
        reflected->x = incident->x * g_zMath_Vec3NegUnitFloat;
        reflected->y = incident->y * g_zMath_Vec3NegUnitFloat;
        reflected->z = incident->z * g_zMath_Vec3NegUnitFloat;
        return;
    }

    zVec3 scaledNormal;
    zVec3 *scaledNormalPtr = &scaledNormal;
    scaledNormalPtr->x = -dot * normal->x;
    scaledNormalPtr->y = normal->y * -dot;
    scaledNormalPtr->z = normal->z * -dot;

    zVec3 halfReflected;
    zVec3 *halfReflectedPtr = &halfReflected;
    halfReflectedPtr->x = incident->x + scaledNormalPtr->x;
    halfReflectedPtr->y = incident->y + scaledNormalPtr->y;
    halfReflectedPtr->z = incident->z + scaledNormalPtr->z;

    reflected->x = scaledNormalPtr->x + halfReflectedPtr->x;
    reflected->y = scaledNormalPtr->y + halfReflectedPtr->y;
    reflected->z = scaledNormalPtr->z + halfReflectedPtr->z;
}

/**
 * Reimplements 0x472960: zMath::Vec3Lerp (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Blends the first vector in place with a second vector using a*t + b*(1-t).
 */
void __fastcall Vec3Lerp(
    zVec3 *inOut,
    const zVec3 *other,
    float t
) {
    const float otherScale = g_zMath_Vec3UnitFloat - t;
    inOut->x = t * inOut->x + otherScale * other->x;
    inOut->y = t * inOut->y + otherScale * other->y;
    inOut->z = t * inOut->z + otherScale * other->z;
}

/**
 * Reimplements 0x4729f0: zMath::Vec3LerpNormalize (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Blends the first vector toward a second vector and normalizes the result.
 */
void __fastcall Vec3LerpNormalize(
    zVec3 *inOut,
    const zVec3 *other,
    float t
) {
    Vec3Lerp(
        inOut,
        other,
        t
    );
    Vec3Normalize(inOut);
}

/**
 * Reimplements 0x4729b0: zMath::Vec3DirectionTo (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Writes the normalized direction from one point to another and returns the original distance.
 * Data: writes only the caller-supplied output vector before delegating
 * normalization to zMath::Vec3Normalize.
 */
float __fastcall Vec3DirectionTo(
    const zVec3 *from,
    const zVec3 *to,
    zVec3 *outDir
) {
    const float dx = to->x - from->x;
    const float dy = to->y - from->y;
    const float dz = to->z - from->z;
    outDir->x = dx;
    outDir->y = dy;
    outDir->z = dz;
    return Vec3Normalize(outDir);
}

/**
 * Reimplements 0x472a10: zMath::Vec3Slerp (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Interpolates between two unit vectors with endpoint, near-linear, antiparallel, and spherical paths.
 */
void __fastcall Vec3Slerp(
    const zVec3 *a,
    const zVec3 *b,
    float t,
    zVec3 *out
) {
    if (t == g_zMath_Vec3ZeroFloat) {
        *out = *a;
        return;
    }

    if (t == g_zMath_Vec3UnitFloat) {
        *out = *b;
        return;
    }

    const float dot = Dot(
        *a,
        *b
    );
    if (dot < g_zMath_Vec3SlerpDotNegThreshold) {
        zVec3 perpendicular;
        Vec3Perp2D(
            a,
            &perpendicular
        );

        const float angle = g_zMath_Vec3SlerpPiFloat * t;
        const float sinAngle = sin(angle);
        const float cosAngle = cos(angle);
        out->x = a->x * cosAngle + perpendicular.x * sinAngle;
        out->y = a->y * cosAngle + perpendicular.y * sinAngle;
        out->z = a->z * cosAngle + perpendicular.z * sinAngle;
        return;
    }

    if (dot > g_zMath_Vec3SlerpDotPosThreshold) {
        const float aScale = g_zMath_Vec3UnitFloat - t;
        out->x = a->x * aScale + b->x * t;
        out->y = a->y * aScale + b->y * t;
        out->z = a->z * aScale + b->z * t;
        return;
    }

    const float sinOmegaSq = g_zMath_Vec3UnitFloat - dot * dot;
    const float sinOmega = sinOmegaSq <= g_zMath_Vec3ZeroFloat ? 0.0f : FastSqrtEstimate(sinOmegaSq);
    const float omega = atan2(
        sinOmega,
        dot
    );
    const float aScale = sin((g_zMath_Vec3UnitFloat - t) * omega);
    const float bScale = sin(t * omega);

    out->x = a->x * aScale + b->x * bScale;
    out->y = a->y * aScale + b->y * bScale;
    out->z = a->z * aScale + b->z * bScale;

    const float invSinOmega = g_zMath_Vec3UnitFloat / sinOmega;
    out->x *= invSinOmega;
    out->y *= invSinOmega;
    out->z *= invSinOmega;
}

/**
 * Reimplements 0x475210: zMath::LineVsSphereHit
 * (D:\Proj\GameZRecoil\zMath\zMathGeom.cpp).
 * Purpose: tests a segment direction against a sphere and writes the
 * normalized inward hit normal when the hit lies in front of the segment
 * origin.
 * Data: reads the shared zMath zero scalar at 0x4d2960 for x87 comparisons;
 * writes only the caller-supplied output normal.
 */
int __fastcall LineVsSphereHit(
    const zVec3 *segA,
    const zVec3 *segB,
    float radius,
    const zVec3 *sphereCenterRelSegB,
    zVec3 *outInwardNormal
) {
    zVec3 lineDelta = {segA->x - segB->x, segA->y - segB->y, segA->z - segB->z};

    const float lineLengthSq = Dot(
        lineDelta,
        lineDelta
    );
    if (lineLengthSq == 0.0f) {
        return 0;
    }

    const float centerDotLine = Dot(
        *sphereCenterRelSegB,
        lineDelta
    );
    float centerDistMinusRadius = Dot(
        *sphereCenterRelSegB,
        *sphereCenterRelSegB
    ) - radius * radius;

    float hitScale = 0.0f;
    if (centerDistMinusRadius == 0.0f) {
        if (centerDotLine <= 0.0f) {
            return 0;
        }
        hitScale = (centerDotLine + centerDotLine) / lineLengthSq;
    } else {
        const float discriminant =
            centerDotLine * centerDotLine - lineLengthSq * centerDistMinusRadius;
        if (discriminant < 0.0f) {
            return 0;
        }

        const float discriminantRoot = FastSqrtEstimate(discriminant);
        float rootNumerator = centerDotLine;
        if (centerDistMinusRadius < 0.0f) {
            centerDistMinusRadius = -centerDistMinusRadius;
            rootNumerator = -centerDotLine;
        }

        float denominator = rootNumerator - discriminantRoot;
        if (rootNumerator <= discriminantRoot) {
            denominator = rootNumerator + discriminantRoot;
            if (denominator <= 0.0f) {
                return 0;
            }
        }

        hitScale = centerDistMinusRadius / denominator;
    }

    lineDelta.x *= hitScale;
    lineDelta.y *= hitScale;
    lineDelta.z *= hitScale;

    outInwardNormal->x = sphereCenterRelSegB->x - lineDelta.x;
    outInwardNormal->y = sphereCenterRelSegB->y - lineDelta.y;
    outInwardNormal->z = sphereCenterRelSegB->z - lineDelta.z;
    Vec3Normalize(outInwardNormal);
    return 1;
}

/**
 * Reimplements 0x42d560: zMath::Vec3Midpoint (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Writes the component-wise midpoint of two vectors and returns the output pointer.
 * Data: reads shared zMath scalar constant 0x4d08d4 and writes only the
 * caller-supplied output vector.
 */
zVec3 *__fastcall Vec3Midpoint(
    const zVec3 *a,
    const zVec3 *b,
    zVec3 *outMidpoint
) {
    const float sumX = a->x + b->x;
    const float sumY = a->y + b->y;
    const float sumZ = a->z + b->z;
    outMidpoint->x = sumX;
    outMidpoint->y = sumY;
    outMidpoint->z = sumZ;
    outMidpoint->x *= g_zMath_MidpointHalf;
    outMidpoint->y *= g_zMath_MidpointHalf;
    outMidpoint->z *= g_zMath_MidpointHalf;
    return outMidpoint;
}

/**
 * Reimplements 0x4726d0: zMath::Vec3DeltaLength (GameZRecoil/zMath.cpp).
 * Purpose: Stores the vector delta in the shared scratch vector and returns its length.
 */
float __fastcall Vec3DeltaLength(
    const zVec3 *a,
    const zVec3 *b
) {
    g_zMath_Vec3DeltaScratch.x = a->x - b->x;
    g_zMath_Vec3DeltaScratch.y = a->y - b->y;
    g_zMath_Vec3DeltaScratch.z = a->z - b->z;

    const float lengthSq = g_zMath_Vec3DeltaScratch.x * g_zMath_Vec3DeltaScratch.x +
                           g_zMath_Vec3DeltaScratch.y * g_zMath_Vec3DeltaScratch.y +
                           g_zMath_Vec3DeltaScratch.z * g_zMath_Vec3DeltaScratch.z;
    return sqrt(lengthSq);
}

/**
 * Reimplements 0x472670: zMath::Vec3DeltaLengthSq (GameZRecoil/zMath.cpp).
 * Purpose: Stores the vector delta in the shared scratch vector and returns its squared length.
 */
float __fastcall Vec3DeltaLengthSq(
    const zVec3 *a,
    const zVec3 *b
) {
    g_zMath_Vec3DeltaScratch.x = a->x - b->x;
    g_zMath_Vec3DeltaScratch.y = a->y - b->y;
    g_zMath_Vec3DeltaScratch.z = a->z - b->z;

    return g_zMath_Vec3DeltaScratch.x * g_zMath_Vec3DeltaScratch.x +
           g_zMath_Vec3DeltaScratch.y * g_zMath_Vec3DeltaScratch.y +
           g_zMath_Vec3DeltaScratch.z * g_zMath_Vec3DeltaScratch.z;
}

/**
 * Reimplements 0x472730: zMath::Vec3DistSqXZ (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Stores the XZ delta in the shared scratch vector and returns squared XZ-plane distance.
 */
float __fastcall Vec3DistSqXZ(
    const zVec3 *a,
    const zVec3 *b
) {
    g_zMath_Vec3DeltaScratch.x = a->x - b->x;
    g_zMath_Vec3DeltaScratch.z = a->z - b->z;

    return g_zMath_Vec3DeltaScratch.x * g_zMath_Vec3DeltaScratch.x +
           g_zMath_Vec3DeltaScratch.z * g_zMath_Vec3DeltaScratch.z;
}

// Reimplements 0x473210: zMath::MatCopyCurrentTo
zMat4x3 *__stdcall MatCopyCurrentTo(
    zMat4x3 *out
) {
    memcpy(
        out,
        *g_currentMatrixPtrSlot,
        sizeof(zMat4x3)
    );
    return out;
}

// Reimplements 0x473250: zMath::MatLoadCurrentFrom
void __fastcall MatLoadCurrentFrom(
    const zMat4x3 *src
) {
    memcpy(
        *g_currentMatrixPtrSlot,
        src,
        sizeof(zMat4x3)
    );
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x473280: zMath::MatLoadRotationFrom3x3
// (D:\Proj\GameZRecoil\zMath\zmath_matload.cpp)
void __fastcall MatLoadRotationFrom3x3(
    const zMat4x3 *src
) {
    unsigned int *matrix = (unsigned int *)(*g_currentMatrixPtrSlot);
    const unsigned int *source = (const unsigned int *)src;

    *matrix++ = *source++;
    *matrix++ = *source++;
    *matrix++ = *source++;
    *matrix++ = *source++;
    *matrix++ = *source++;
    *matrix++ = *source++;
    *matrix++ = *source++;
    *matrix++ = *source++;
    *matrix = *source;
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x473370: zMath::MatMultiply
void __fastcall MatMultiply(
    const zMat4x3 *src,
    int mode
) {
    zMat4x3 *current = (zMat4x3 *)(*g_currentMatrixPtrSlot);
    if (*g_currentMatrixIdentityFlagSlot != 0) {
        memcpy(
            current,
            src,
            sizeof(zMat4x3)
        );
        *g_currentMatrixIdentityFlagSlot = 0;
        return;
    }

    const zMat4x3 lhs = *current;
    zMat4x3 out = {0};

    out.xx = lhs.xx * src->xx + lhs.yx * src->xy + lhs.zx * src->xz;
    out.xy = lhs.xy * src->xx + lhs.yy * src->xy + lhs.zy * src->xz;
    out.xz = lhs.xz * src->xx + lhs.yz * src->xy + lhs.zz * src->xz;
    out.yx = lhs.xx * src->yx + lhs.yx * src->yy + lhs.zx * src->yz;
    out.yy = lhs.xy * src->yx + lhs.yy * src->yy + lhs.zy * src->yz;
    out.yz = lhs.xz * src->yx + lhs.yz * src->yy + lhs.zz * src->yz;
    out.zx = lhs.xx * src->zx + lhs.yx * src->zy + lhs.zx * src->zz;
    out.zy = lhs.xy * src->zx + lhs.yy * src->zy + lhs.zy * src->zz;
    out.zz = lhs.xz * src->zx + lhs.yz * src->zy + lhs.zz * src->zz;

    if (mode != 2) {
        out.posX = lhs.xx * src->posX + lhs.yx * src->posY + lhs.zx * src->posZ + lhs.posX;
        out.posY = lhs.xy * src->posX + lhs.yy * src->posY + lhs.zy * src->posZ + lhs.posY;
        out.posZ = lhs.xz * src->posX + lhs.yz * src->posY + lhs.zz * src->posZ + lhs.posZ;
    } else {
        out.posX = lhs.posX;
        out.posY = lhs.posY;
        out.posZ = lhs.posZ;
    }

    *current = out;
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x4737e0: zMath::MatTranslate (GameZRecoil/zMath/zmath_matrix.cpp)
void __stdcall MatTranslate(
    float tx,
    float ty,
    float tz
) {
    zMat4x3 *matrix = (zMat4x3 *)(*g_currentMatrixPtrSlot);

    if (*g_currentMatrixIdentityFlagSlot != 0) {
        matrix->posX = tx;
        matrix->posY = ty;
        matrix->posZ = tz;
        *g_currentMatrixIdentityFlagSlot = 0;
        return;
    }

    matrix->posX += tx * matrix->xx + ty * matrix->yx + tz * matrix->zx;
    matrix->posY += tx * matrix->xy + ty * matrix->yy + tz * matrix->zy;
    matrix->posZ += tx * matrix->xz + ty * matrix->yz + tz * matrix->zz;
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x473970: zMath::MatRotateX (GameZRecoil/zMath/zmath_matrix.cpp)
void __stdcall MatRotateX(
    float angleRad
) {
    const float sinAngle = sin(angleRad);
    const float cosAngle = cos(angleRad);
    zMat4x3 *matrix = (zMat4x3 *)(*g_currentMatrixPtrSlot);

    if (*g_currentMatrixIdentityFlagSlot != 0) {
        matrix->yy = cosAngle;
        matrix->yz = sinAngle;
        matrix->zy = -sinAngle;
        matrix->zz = cosAngle;
        *g_currentMatrixIdentityFlagSlot = 0;
        return;
    }

    const float oldYx = matrix->yx;
    const float oldYy = matrix->yy;
    const float oldYz = matrix->yz;
    const float oldZx = matrix->zx;
    const float oldZy = matrix->zy;
    const float oldZz = matrix->zz;

    matrix->yx = cosAngle * oldYx + sinAngle * oldZx;
    matrix->yy = cosAngle * oldYy + sinAngle * oldZy;
    matrix->yz = cosAngle * oldYz + sinAngle * oldZz;
    matrix->zx = cosAngle * oldZx - sinAngle * oldYx;
    matrix->zy = cosAngle * oldZy - sinAngle * oldYy;
    matrix->zz = cosAngle * oldZz - sinAngle * oldYz;
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x473b10: zMath::MatRotateY
void __stdcall MatRotateY(
    float angleRad
) {
    const float sinAngle = sin(angleRad);
    const float cosAngle = cos(angleRad);
    zMat4x3 *matrix = (zMat4x3 *)(*g_currentMatrixPtrSlot);

    if (*g_currentMatrixIdentityFlagSlot != 0) {
        matrix->xx = cosAngle;
        matrix->xz = -sinAngle;
        matrix->zx = sinAngle;
        matrix->zz = cosAngle;
        *g_currentMatrixIdentityFlagSlot = 0;
        return;
    }

    const float oldXx = matrix->xx;
    const float oldXy = matrix->xy;
    const float oldXz = matrix->xz;
    const float oldYx = matrix->yx;
    const float oldYy = matrix->yy;
    const float oldYz = matrix->yz;
    const float oldZx = matrix->zx;
    const float oldZy = matrix->zy;
    const float oldZz = matrix->zz;
    const float oldPosX = matrix->posX;
    const float oldPosY = matrix->posY;
    const float oldPosZ = matrix->posZ;

    matrix->xx = cosAngle * oldXx - sinAngle * oldZx;
    matrix->xy = cosAngle * oldXy - sinAngle * oldZy;
    matrix->xz = cosAngle * oldXz - sinAngle * oldZz;
    matrix->yx = oldYx;
    matrix->yy = oldYy;
    matrix->yz = oldYz;
    matrix->zx = sinAngle * oldXx + cosAngle * oldZx;
    matrix->zy = sinAngle * oldXy + cosAngle * oldZy;
    matrix->zz = sinAngle * oldXz + cosAngle * oldZz;
    matrix->posX = oldPosX;
    matrix->posY = oldPosY;
    matrix->posZ = oldPosZ;
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x473cc0: zMath::MatRotateZ (GameZRecoil/zMath/zmath_matrix.cpp)
void __stdcall MatRotateZ(
    float angleRad
) {
    const float sinAngle = sin(angleRad);
    const float cosAngle = cos(angleRad);
    zMat4x3 *matrix = (zMat4x3 *)(*g_currentMatrixPtrSlot);

    if (*g_currentMatrixIdentityFlagSlot != 0) {
        matrix->xx = cosAngle;
        matrix->xy = sinAngle;
        matrix->yx = -sinAngle;
        matrix->yy = cosAngle;
        *g_currentMatrixIdentityFlagSlot = 0;
        return;
    }

    const float oldXx = matrix->xx;
    const float oldXy = matrix->xy;
    const float oldXz = matrix->xz;
    const float oldYx = matrix->yx;
    const float oldYy = matrix->yy;
    const float oldYz = matrix->yz;

    matrix->xx = cosAngle * oldXx + sinAngle * oldYx;
    matrix->xy = cosAngle * oldXy + sinAngle * oldYy;
    matrix->xz = cosAngle * oldXz + sinAngle * oldYz;
    matrix->yx = cosAngle * oldYx - sinAngle * oldXx;
    matrix->yy = cosAngle * oldYy - sinAngle * oldXy;
    matrix->yz = cosAngle * oldYz - sinAngle * oldXz;
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x474010: zMath::MatApplyLocalTRS
void __fastcall MatApplyLocalTRS(
    const zVec3 *angles,
    const zVec3 *position,
    const zVec3 *scale
) {
    const float sx = sin(angles->x);
    const float cx = cos(angles->x);
    const float sy = sin(angles->y);
    const float cy = cos(angles->y);
    const float sz = sin(angles->z);
    const float cz = cos(angles->z);

    const float sySx = sy * sx;
    const float szCy = sz * cy;
    const float czCy = cz * cy;

    zMat4x3 local = {0};
    local.xx = sySx * sz + cz * cy;
    local.xy = sz * cx;
    local.xz = szCy * sx - cz * sy;
    local.yx = sySx * cz - szCy;
    local.yy = cz * cx;
    local.yz = czCy * sx + sz * sy;
    local.zx = sy * cx;
    local.zy = -sx;
    local.zz = cy * cx;

    if (position->x != 0.0f) {
        local.posX = position->x;
    }
    if (position->y != 0.0f) {
        local.posY = position->y;
    }
    if (position->z != 0.0f) {
        local.posZ = position->z;
    }

    if (scale->x != 1.0f) {
        local.xx *= scale->x;
        local.xy *= scale->x;
        local.xz *= scale->x;
    }
    if (scale->y != 1.0f) {
        local.yx *= scale->y;
        local.yy *= scale->y;
        local.yz *= scale->y;
    }
    if (scale->z != 1.0f) {
        local.zx *= scale->z;
        local.zy *= scale->z;
        local.zz *= scale->z;
    }

    MatMultiply(
        &local,
        1
    );
    *g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x474260: zMath::MatBuildEulerRotation3x3
// (D:\Proj\GameZRecoil\zMath\zmath_mat.cpp)
void __fastcall MatBuildEulerRotation3x3(
    zMat4x3 *outBasis,
    float angleX,
    float angleY,
    float angleZ
) {
    const float sx = sin(angleX);
    const float cx = cos(angleX);
    const float sy = sin(angleY);
    const float cy = cos(angleY);
    const float sz = sin(angleZ);
    const float cz = cos(angleZ);

    const float sySx = sy * sx;
    const float szCy = sz * cy;
    const float czCy = cz * cy;

    outBasis->xx = sySx * sz + cz * cy;
    outBasis->xy = sz * cx;
    outBasis->xz = szCy * sx - cz * sy;
    outBasis->yx = sySx * cz - szCy;
    outBasis->yy = cz * cx;
    outBasis->yz = czCy * sx + sz * sy;
    outBasis->zx = sy * cx;
    outBasis->zy = -sx;
    outBasis->zz = cy * cx;
    outBasis->posX = 0.0f;
    outBasis->posY = 0.0f;
    outBasis->posZ = 0.0f;
}

// Reimplements 0x474d10: zMath::Vec3DirectionAnglesBetweenPoints
zVec3 *__fastcall Vec3DirectionAnglesBetweenPoints(
    const zVec3 *pointA,
    const zVec3 *pointB,
    zVec3 *outAngles
) {
    const float dx = pointA->x - pointB->x;
    const float dy = pointB->y - pointA->y;
    const float dz = pointA->z - pointB->z;
    outAngles->x = atan2(
        dy,
        sqrt(dx * dx + dz * dz)
    );
    outAngles->y = atan2(
        dx,
        dz
    );
    outAngles->z = 0.0f;
    return outAngles;
}

// Reimplements 0x473fc0: zMath::Vec3ArrayProjectToCachedY
void __fastcall Vec3ArrayProjectToCachedY(
    const zVec3 *points,
    float *outValues,
    int count
) {
    for (int i = 0; i < count; ++i) {
        outValues[i] = points[i].x * g_zMath_CameraScratchA.xy +
                       points[i].y * g_zMath_CameraScratchA.yy +
                       points[i].z * g_zMath_CameraScratchA.zy + g_zMath_CameraScratchA.posY;
    }
}

/**
 * Reimplements 0x474f40: zMath::Vec3RotateY (GameZRecoil/zMath/zmath_vec.cpp).
 * Purpose: Rotates an input vector around the Y axis and copies the original Y component to the output.
 */
void __fastcall Vec3RotateY(
    zVec3 *outVec,
    const zVec3 *inVec,
    float yawAngle
) {
    const float sinAngle = sin(yawAngle);
    const float cosAngle = cos(yawAngle);
    outVec->x = sinAngle * inVec->z + cosAngle * inVec->x;
    outVec->y = inVec->y;
    outVec->z = cosAngle * inVec->z - sinAngle * inVec->x;
}

// Reimplements 0x474670: zMath::Vec3ArrayTransformDirection
// (D:\Proj\GameZRecoil\zMath\zmath_vec.cpp)
void __fastcall Vec3ArrayTransformDirection(
    zVec3 *vectors,
    int count
) {
    if (*g_currentMatrixIdentityFlagSlot != 0 || count <= 0) {
        return;
    }

    const zMat4x3 *const matrix = (const zMat4x3 *)(*g_currentMatrixPtrSlot);
    for (int i = 0; i < count; ++i) {
        const zVec3 vec = vectors[i];
        vectors[i].x = vec.x * matrix->xx + vec.y * matrix->yx + vec.z * matrix->zx;
        vectors[i].y = vec.x * matrix->xy + vec.y * matrix->yy + vec.z * matrix->zy;
        vectors[i].z = vec.x * matrix->xz + vec.y * matrix->yz + vec.z * matrix->zz;
    }
}

// Reimplements 0x4747d0: zMath::MatTransformPointBatchInPlace
void __fastcall MatTransformPointBatchInPlace(
    zVec3 *points,
    int count
) {
    if (*g_currentMatrixIdentityFlagSlot != 0 || count == 0) {
        return;
    }

    const zMat4x3 *matrix = (const zMat4x3 *)(*g_currentMatrixPtrSlot);
    for (int i = 0; i < count; ++i) {
        const zVec3 point = points[i];
        points[i].x =
            point.x * matrix->xx + point.y * matrix->yx + point.z * matrix->zx + matrix->posX;
        points[i].z =
            point.x * matrix->xz + point.y * matrix->yz + point.z * matrix->zz + matrix->posZ;
        points[i].y =
            point.x * matrix->xy + point.y * matrix->yy + point.z * matrix->zy + matrix->posY;
    }
}

// Reimplements 0x474b20: zMath::ProjectPointBatch
void __fastcall ProjectPointBatch(
    const zVec3 *viewPoints,
    zProjectedPoint *projectedPoints,
    int count
) {
    for (int i = 0; i < count; ++i) {
        const float reciprocalZ = 1.0f / viewPoints[i].z;
        projectedPoints[i].reciprocalZ = reciprocalZ;
        projectedPoints[i].x =
            viewPoints[i].x * g_zMath_ProjScaleX * reciprocalZ + g_zMath_ProjOffsetX;
        projectedPoints[i].y =
            viewPoints[i].y * g_zMath_ProjScaleY * reciprocalZ + g_zMath_ProjOffsetY;
    }
}

/**
 * Reimplements 0x4bd800: zMath::ClipLineSegmentPointToZ
 * (D:\Proj\GameZ\z_math.cpp).
 * Purpose: moves one segment endpoint onto the caller-supplied Z clip plane by
 * interpolating toward the other endpoint.
 * Data: writes only the caller-supplied endpoint and reads no authored globals.
 */
void __fastcall ClipLineSegmentPointToZ(
    zVec3 *pointToClip,
    const zVec3 *otherPoint,
    float clipZ
) {
    const float t = (clipZ - pointToClip->z) / (otherPoint->z - pointToClip->z);

    pointToClip->x = (otherPoint->x - pointToClip->x) * t + pointToClip->x;
    pointToClip->y = (otherPoint->y - pointToClip->y) * t + pointToClip->y;
    pointToClip->z = clipZ;
}

/**
 * Reimplements 0x4bd720: zMath::ClipLineSegmentToZRange
 * (D:\Proj\GameZ\z_math.cpp).
 * Purpose: clips a mutable segment against the current zMath lower and upper
 * Z clipping planes, rejecting segments fully outside the range.
 * Data: reads g_zMath_ClipZLowerBound at 0x4e4880 and
 * g_zMath_ClipZUpperBound at 0x4e4890.
 */
int __fastcall ClipLineSegmentToZRange(
    zVec3 *pointA,
    zVec3 *pointB
) {
    if (pointA->z > g_zMath_ClipZUpperBound && pointB->z > g_zMath_ClipZUpperBound) {
        return 0;
    }

    if (pointA->z < g_zMath_ClipZLowerBound && pointB->z < g_zMath_ClipZLowerBound) {
        return 0;
    }

    if (pointA->z < g_zMath_ClipZLowerBound) {
        ClipLineSegmentPointToZ(
            pointA,
            pointB,
            g_zMath_ClipZLowerBound
        );
    }
    if (pointB->z < g_zMath_ClipZLowerBound) {
        ClipLineSegmentPointToZ(
            pointB,
            pointA,
            g_zMath_ClipZLowerBound
        );
    }

    if (pointB->z > g_zMath_ClipZUpperBound) {
        ClipLineSegmentPointToZ(
            pointB,
            pointA,
            g_zMath_ClipZUpperBound
        );
    }
    if (pointA->z > g_zMath_ClipZUpperBound) {
        ClipLineSegmentPointToZ(
            pointA,
            pointB,
            g_zMath_ClipZUpperBound
        );
    }

    return 1;
}

// Reimplements 0x476480: zMath::ProjectPointAndClampToScreenClip
// (GameZRecoil/zMath/zMath.cpp)
int __fastcall ProjectPointAndClampToScreenClip(
    const zVec3 *srcPoint,
    zVec3 *dstPoint
) {
    zMat4x3 slotBuffer = {0};
    MatStackPushPtr((float *)(&slotBuffer));
    MatLoadCameraScratchB();

    if (*g_currentMatrixIdentityFlagSlot != 0) {
        *dstPoint = *srcPoint;
    } else {
        const zMat4x3 *const matrix = (const zMat4x3 *)(*g_currentMatrixPtrSlot);
        dstPoint->x = srcPoint->x * matrix->xx + srcPoint->y * matrix->yx +
                      srcPoint->z * matrix->zx + matrix->posX;
        dstPoint->z = srcPoint->x * matrix->xz + srcPoint->y * matrix->yz +
                      srcPoint->z * matrix->zz + matrix->posZ;
        dstPoint->y = srcPoint->x * matrix->xy + srcPoint->y * matrix->yy +
                      srcPoint->z * matrix->zy + matrix->posY;
    }

    MatStackPopPtr();

    if (dstPoint->z <= gClipRect_Primary.zMin) {
        int result = 8;
        if (-gClipRect_Primary.zMin <= dstPoint->z) {
            dstPoint->z = gClipRect_Primary.zMin;
        } else {
            dstPoint->z = -dstPoint->z;
        }

        ProjectPointBatch(
            dstPoint,
            (zProjectedPoint *)(dstPoint),
            1
        );
        if (dstPoint->x < -5000.0f) {
            dstPoint->x = -5000.0f;
        } else if (dstPoint->x > 5000.0f) {
            dstPoint->x = 5000.0f;
        }

        dstPoint->y = g_zVideo_ProjectClipBottom;
        dstPoint->x = (dstPoint->x + g_zVideo_ProjectClipLeft + 5000.0f) /
                      (10000.0f / (gClipRect_Primary.xMaxAlt - g_zVideo_ProjectClipLeft));
        return result;
    }

    ProjectPointBatch(
        dstPoint,
        (zProjectedPoint *)(dstPoint),
        1
    );

    int result = 0;
    if (dstPoint->x < g_zVideo_ProjectClipLeft) {
        dstPoint->x = g_zVideo_ProjectClipLeft;
        result = 1;
    } else if (dstPoint->x > g_zVideo_ProjectClipRight) {
        dstPoint->x = g_zVideo_ProjectClipRight;
        result = 2;
    }

    if (dstPoint->y < g_zVideo_ProjectClipTop) {
        dstPoint->y = g_zVideo_ProjectClipTop;
        return 4;
    }
    if (dstPoint->y >= g_zVideo_ProjectClipBottom) {
        dstPoint->y = g_zVideo_ProjectClipBottom - 1.0f;
        return 8;
    }

    return result;
}

// Reimplements 0x474fc0: zMath::ApproxExpNeg
// (D:\Proj\GameZRecoil\zMath\zmath.cpp)
float __stdcall ApproxExpNeg(
    float x
) {
    if (g_zMath_ApproxExpNegDirty != 0) {
        g_zMath_ApproxExpNegScale = 51.0f;
        for (int i = 0; i < 256; ++i) {
            g_zMath_ApproxExpNegTable[i] = expf(-(float)(i) * 0.0196078438f);
        }
        g_zMath_ApproxExpNegDirty = 0;
    }

    if (x > 5.0f) {
        return 0.0f;
    }
    if (x < 0.0f) {
        return 1.0f;
    }

    const int tableIndex = (int)(g_zMath_ApproxExpNegScale * x);
    return g_zMath_ApproxExpNegTable[tableIndex];
}
} // namespace zMath

/**
 * Reimplements 0x473690: zMath_Mat_Scale (GameZRecoil/zMath/zmath_matrix.cpp).
 * Purpose: Applies per-axis scale to the current matrix basis while preserving translation.
 */
void zMath_Mat_Scale(
    float sx,
    float sy,
    float sz
) {
    zMat4x3 scaled;
    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        ((zMat4x3 *)*zMath::g_currentMatrixPtrSlot)->xx = sx;
        ((zMat4x3 *)*zMath::g_currentMatrixPtrSlot)->yy = sy;
        ((zMat4x3 *)*zMath::g_currentMatrixPtrSlot)->zz = sz;
    } else {
        zMat4x3 *matrix = (zMat4x3 *)*zMath::g_currentMatrixPtrSlot;
        scaled.xx = matrix->xx * sx;
        scaled.xy = matrix->xy * sx;
        scaled.xz = matrix->xz * sx;
        scaled.yx = matrix->yx * sy;
        scaled.yy = matrix->yy * sy;
        scaled.yz = matrix->yz * sy;
        scaled.zx = matrix->zx * sz;
        scaled.zy = matrix->zy * sz;
        scaled.zz = matrix->zz * sz;
        scaled.posX = matrix->posX;
        scaled.posY = matrix->posY;
        scaled.posZ = matrix->posZ;
        matrix->xx = scaled.xx;
        matrix->xy = scaled.xy;
        matrix->xz = scaled.xz;
        matrix->yx = scaled.yx;
        matrix->yy = scaled.yy;
        matrix->yz = scaled.yz;
        matrix->zx = scaled.zx;
        matrix->zy = scaled.zy;
        matrix->zz = scaled.zz;
        matrix->posX = scaled.posX;
        matrix->posY = scaled.posY;
        matrix->posZ = scaled.posZ;
    }
    *zMath::g_currentMatrixIdentityFlagSlot = 0;
}

// Reimplements 0x472ed0: zMath_Project_GetLastScreenScaleXY
// (GameZRecoil/zMath/zmath_project.cpp)
zVec2 zMath_Project_GetLastScreenScaleXY() {
    zVec2 scale;
    scale.x = g_zMath_ProjScaleX;
    scale.y = g_zMath_ProjScaleY;
    return scale;
}

// Reimplements 0x474d90: zMath_Vec3_ElevationAngleBetweenPoints
// (GameZRecoil/zMath/Math.c)
float __fastcall zMath_Vec3_ElevationAngleBetweenPoints(
    const zVec3 *pointA,
    const zVec3 *pointB
) {
    const float dx = pointA->x - pointB->x;
    const float dy = pointB->y - pointA->y;
    const float dz = pointA->z - pointB->z;
    return atan2(
        sqrt(dx * dx + dz * dz),
        dy
    );
}

/**
 * Reimplements 0x4731f0: zMath_Mat_SetupCamera
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: loads camera scratch B and composes it through the parent matrix
 * stack slot.
 */
void zMath_Mat_SetupCamera() {
    zMath::MatLoadCameraScratchB();
    zMath::MatMultiply(
        (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]),
        1
    );
}

/**
 * Reimplements 0x472fb0: zMath_Mat_LoadProjection
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: builds the current projection-node matrix from the parent slot,
 * camera scratch B, and a caller-supplied yaw/Z offset.
 */
void __stdcall zMath_Mat_LoadProjection(
    float zOffset
) {
    float parentYaw = 0.0f;
    if (zMath::g_currentMatrixIdentityFlagSlot[-1] == 0) {
        parentYaw = zMath_Mat_ExtractYaw((const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]));
    }

    zMath::MatLoadIdentity();
    zMath::MatRotateY(zOffset - parentYaw);
    zMath::MatMultiply(
        (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]),
        1
    );

    zMat4x3 *current = (zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    const zMat4x3 *parent = (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]);
    current->posX = parent->posX;
    current->posY = parent->posY;
    current->posZ = parent->posZ;

    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadCameraScratchB();
    zMath::MatMultiply(
        (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]),
        1
    );
    zMath::MatStackPopPtr();
    zMath::MatLoadCurrentFrom(&slotBuffer);
}

/**
 * Reimplements 0x474bc0: zMath_UnprojectPointBatch
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: converts projected screen coordinates with reciprocal Z back into view-space points.
 */
void __fastcall zMath_UnprojectPointBatch(
    const zProjectedPoint *projectedPoints,
    zVec3 *outPoints,
    int count
) {
    for (int i = 0; i < count; ++i) {
        const float z = 1.0f / projectedPoints[i].reciprocalZ;
        outPoints[i].z = z;
        outPoints[i].x = (projectedPoints[i].x - g_zMath_ProjOffsetX) * g_zMath_InvProjScaleX * z;
        outPoints[i].y = (projectedPoints[i].y - g_zMath_ProjOffsetY) * g_zMath_InvProjScaleY * z;
    }
}

/**
 * Reimplements 0x474c20: zMath_UnprojectPointBatchZBuf
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: unprojects projected points and transforms them through the staged camera inverse matrix.
 */
void __fastcall zMath_UnprojectPointBatchZBuf(
    const zProjectedPoint *projectedPoints,
    zVec3 *outPoints,
    int count
) {
    zVec3 *viewPoints = outPoints;
    zMath_UnprojectPointBatch(
        projectedPoints,
        viewPoints,
        count
    );

    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadCameraScratchA();

    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        for (int i = 0; i < count; ++i) {
            outPoints[i] = viewPoints[i];
        }
    } else {
        const zMat4x3 *const matrix =
            (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
        for (int i = 0; i < count; ++i) {
            const zVec3 viewPoint = viewPoints[i];
            outPoints[i].x =
                viewPoint.x * matrix->xx + viewPoint.y * matrix->yx +
                viewPoint.z * matrix->zx + matrix->posX;
            outPoints[i].z =
                viewPoint.x * matrix->xz + viewPoint.y * matrix->yz +
                viewPoint.z * matrix->zz + matrix->posZ;
            outPoints[i].y =
                viewPoint.x * matrix->xy + viewPoint.y * matrix->yy +
                viewPoint.z * matrix->zy + matrix->posY;
        }
    }

    zMath::MatStackPopPtr();
}

/**
 * Reimplements 0x473060: zMath_Mat_LoadView
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: builds the current view matrix from camera and parent transforms.
 */
void zMath_Mat_LoadView() {
    zVec3 parentEuler = zMath::g_zMath_Vec3Zero;
    if (zMath::g_currentMatrixIdentityFlagSlot[-1] == 0) {
        zMath_Mat_ExtractEulerAngles(
            (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]),
            &parentEuler
        );
    }

    zMath::MatLoadCameraScratchA();
    zMat4x3 *current = (zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    current->yx = -current->yx;
    current->yy = -current->yy;
    current->yz = -current->yz;
    current->zx = -current->zx;
    current->zy = -current->zy;
    current->zz = -current->zz;

    zVec3 cameraEuler = zMath::g_zMath_Vec3Zero;
    zMath_Mat_ExtractEulerAngles(
        current,
        &cameraEuler
    );

    zQuat parentQuat = {0};
    zMath_Quat_FromEuler(
        &parentQuat,
        parentEuler.y,
        parentEuler.x,
        parentEuler.z
    );

    zQuat cameraQuat = {0};
    zMath_Quat_FromEuler(
        &cameraQuat,
        cameraEuler.y,
        cameraEuler.x,
        cameraEuler.z
    );

    zQuat relativeQuat = {0};
    zMath_Quat_MultiplyInverse(
        &cameraQuat,
        &parentQuat,
        &relativeQuat
    );

    zMat4x3 viewMatrix = {0};
    zMath_Quat_ToMatrix(
        &relativeQuat,
        &viewMatrix
    );
    zMath::MatLoadCurrentFrom(&viewMatrix);
    zMath::MatMultiply(
        (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]),
        1
    );

    current = (zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    const zMat4x3 *parent = (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]);
    current->posX = parent->posX;
    current->posY = parent->posY;
    current->posZ = parent->posZ;

    zMath::MatStackPushPtr((float *)(&viewMatrix));
    zMath::MatLoadCameraScratchB();
    zMath::MatMultiply(
        (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]),
        1
    );
    zMath::MatStackPopPtr();
    zMath::MatLoadCurrentFrom(&viewMatrix);
}

// Reimplements 0x473230: zMath_Mat_GetCurrent
zMat4x3 *zMath_Mat_GetCurrent() {
    return (zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
}

// Reimplements 0x473240: zMath_Mat_IsCurrentIdentity
int zMath_Mat_IsCurrentIdentity() {
    return *zMath::g_currentMatrixIdentityFlagSlot;
}

// Reimplements 0x474de0: zMath_Mat_ExtractYaw
float __fastcall zMath_Mat_ExtractYaw(
    const zMat4x3 *matrix
) {
    if (matrix->zx == 0.0f && matrix->zz == 0.0f) {
        return 0.0f;
    }

    return atan2(
        matrix->zx,
        matrix->zz
    );
}

/**
 * Reimplements 0x474e10: zMath_Mat_ExtractEulerAngles
 * (D:\Proj\GameZRecoil\zMath\zmath_matrix.cpp).
 * Purpose: extracts pitch, yaw, and roll from a 4x3 rotation matrix.
 */
void __fastcall zMath_Mat_ExtractEulerAngles(
    const zMat4x3 *matrix,
    zVec3 *outEuler
) {
    const float yaw = zMath_Mat_ExtractYaw(matrix);
    const float horizontalLength = sqrt(matrix->zx * matrix->zx + matrix->zz * matrix->zz);
    const float pitch = atan2(
        -matrix->zy,
        horizontalLength
    );

    zVec3 rowX = {0};
    zMath::Vec3RotateY(
        &rowX,
        (const zVec3 *)(matrix),
        -yaw
    );

    zVec3 flattenedRowX = {0};
    zMath_Vec3_RotateX(
        &flattenedRowX,
        &rowX,
        -pitch
    );

    const float rollHorizontalLength =
        sqrt(flattenedRowX.x * flattenedRowX.x + flattenedRowX.z * flattenedRowX.z);
    float roll = atan2(
        flattenedRowX.y,
        rollHorizontalLength
    );
    if (matrix->yy < 0.0f) {
        roll = g_zMath_ElevationPiFloat - roll;
    }

    outEuler->x = pitch;
    outEuler->y = yaw;
    outEuler->z = roll;
}

// Reimplements 0x474ec0: zMath_Vec3_RotateX
void __fastcall zMath_Vec3_RotateX(
    zVec3 *outVec,
    const zVec3 *inVec,
    float angleX
) {
    const float sinAngle = sin(angleX);
    const float cosAngle = cos(angleX);
    outVec->x = inVec->x;
    outVec->y = cosAngle * inVec->y - sinAngle * inVec->z;
    outVec->z = sinAngle * inVec->y + cosAngle * inVec->z;
}

/**
 * Reimplements 0x474580: zMath_Vec3_DirFromYaw
 * (D:\Proj\GameZRecoil\zMath\zmath_vec.cpp).
 * Purpose: Clears the output vector, stages the canonical forward direction,
 * and rotates it around Y to produce a unit XZ direction from yaw.
 */
void __fastcall zMath_Vec3_DirFromYaw(
    zVec3 *outDir,
    float yawAngle
) {
    outDir->x = 0.0f;
    outDir->y = 0.0f;
    outDir->z = 0.0f;

    const zVec3 forward = {0.0f, 0.0f, -1.0f};
    zMath::Vec3RotateY(
        outDir,
        &forward,
        yawAngle
    );
}

/**
 * Reimplements 0x473e60: zMath_Camera_StageInverseRotation
 * (D:\Proj\GameZRecoil\zMath\zmath_camera.cpp).
 * Purpose: stages camera scratch matrices for inverse rotation and translated camera position.
 */
void __fastcall zMath_Camera_StageInverseRotation(
    const zMat4x3 *worldMatrix
) {
    memcpy(
        &zMath::g_zMath_CameraScratchA,
        worldMatrix,
        sizeof(zMat4x3)
    );

    zMath::g_zMath_CameraScratchA.yx = NegateFloatSignBit(zMath::g_zMath_CameraScratchA.yx);
    zMath::g_zMath_CameraScratchA.yy = NegateFloatSignBit(zMath::g_zMath_CameraScratchA.yy);
    zMath::g_zMath_CameraScratchA.yz = NegateFloatSignBit(zMath::g_zMath_CameraScratchA.yz);
    zMath::g_zMath_CameraScratchA.zx = NegateFloatSignBit(zMath::g_zMath_CameraScratchA.zx);
    zMath::g_zMath_CameraScratchA.zy = NegateFloatSignBit(zMath::g_zMath_CameraScratchA.zy);
    zMath::g_zMath_CameraScratchA.zz = NegateFloatSignBit(zMath::g_zMath_CameraScratchA.zz);

    memcpy(
        &zMath::g_zMath_CameraScratchB,
        &zMath::g_zMath_CameraScratchA,
        sizeof(zMat4x3)
    );

    const float yx = zMath::g_zMath_CameraScratchA.yx;
    const float zx = zMath::g_zMath_CameraScratchB.zx;
    const float xy = zMath::g_zMath_CameraScratchA.xy;
    const float zy = zMath::g_zMath_CameraScratchB.zy;
    zMath::g_zMath_CameraScratchB.zx = zMath::g_zMath_CameraScratchB.xz;
    zMath::g_zMath_CameraScratchB.xy = yx;
    zMath::g_zMath_CameraScratchB.zy = zMath::g_zMath_CameraScratchB.yz;
    zMath::g_zMath_CameraScratchB.yx = xy;
    zMath::g_zMath_CameraScratchB.xz = zx;
    zMath::g_zMath_CameraScratchB.yz = zy;

    zMath::g_zMath_CameraScratchB.posX = NegateFloatSignBit(zMath::g_zMath_CameraScratchB.posX);
    zMath::g_zMath_CameraScratchB.posY = NegateFloatSignBit(zMath::g_zMath_CameraScratchB.posY);
    zMath::g_zMath_CameraScratchB.posZ = NegateFloatSignBit(zMath::g_zMath_CameraScratchB.posZ);

    const zVec3 pos = {zMath::g_zMath_CameraScratchB.posX,
        zMath::g_zMath_CameraScratchB.posY,
        zMath::g_zMath_CameraScratchB.posZ};
    zMath::g_zMath_CameraScratchB.posZ = pos.x * zMath::g_zMath_CameraScratchB.xz +
                                         pos.y * zMath::g_zMath_CameraScratchB.yz +
                                         pos.z * zMath::g_zMath_CameraScratchB.zz;
    zMath::g_zMath_CameraScratchB.posY = pos.x * zMath::g_zMath_CameraScratchB.xy +
                                         pos.y * zMath::g_zMath_CameraScratchB.yy +
                                         pos.z * zMath::g_zMath_CameraScratchB.zy;
    zMath::g_zMath_CameraScratchB.posX = pos.x * zMath::g_zMath_CameraScratchB.xx +
                                         pos.y * zMath::g_zMath_CameraScratchB.yx +
                                         pos.z * zMath::g_zMath_CameraScratchB.zx;
}

/**
 * Reimplements 0x4757c0: zMath_Quat_FromEuler
 * (D:\Proj\GameZRecoil\zMath\zMath_Quat.cpp).
 * Purpose: converts three Euler rotation angles into a quaternion.
 * Data: reads no authored zMath globals; VC5 materializes literal and x87
 * range-check constants while lowering the sin/cos half-angle calls.
 */
void __fastcall zMath_Quat_FromEuler(
    zQuat *outQuat,
    float angle0,
    float angle1,
    float angle2
) {
    const float sin0 = sin(angle0 * 0.5f);
    const float cos0 = cos(angle0 * 0.5f);
    const float sin1 = sin(angle1 * 0.5f);
    const float cos1 = cos(angle1 * 0.5f);
    const float sin2 = sin(angle2 * 0.5f);
    const float cos2 = cos(angle2 * 0.5f);

    const float cos1Cos0 = cos1 * cos0;
    const float sin1Cos0 = sin1 * cos0;
    const float cos1Sin0 = cos1 * sin0;
    const float sin1Sin0 = sin1 * sin0;

    outQuat->w = sin1Sin0 * sin2 + cos1Cos0 * cos2;
    outQuat->x = cos1Sin0 * sin2 + sin1Cos0 * cos2;
    outQuat->y = cos1Sin0 * cos2 - sin1Cos0 * sin2;
    outQuat->z = cos1Cos0 * sin2 - sin1Sin0 * cos2;
}

/**
 * Reimplements 0x475910: zMath_Quat_Multiply
 * (D:\Proj\GameZRecoil\zMath\zMath_Quat.cpp).
 * Purpose: computes the quaternion product used by zMath rotation composition.
 */
void __fastcall zMath_Quat_Multiply(
    const zQuat *quatA,
    const zQuat *quatB,
    zQuat *outAB
) {
    outAB->w =
        quatB->w * quatA->w - quatA->x * quatB->x - quatA->y * quatB->y - quatA->z * quatB->z;
    outAB->x =
        quatB->w * quatA->x + quatA->w * quatB->x + quatB->z * quatA->y - quatA->z * quatB->y;
    outAB->y =
        quatB->w * quatA->y + quatA->w * quatB->y + quatA->z * quatB->x - quatB->z * quatA->x;
    outAB->z =
        quatB->w * quatA->z + quatA->w * quatB->z + quatB->y * quatA->x - quatA->y * quatB->x;
}

/**
 * Reimplements 0x4759d0: zMath_Quat_MultiplyInverse
 * (D:\Proj\GameZRecoil\zMath\zMath_Quat.cpp).
 * Purpose: multiplies a quaternion by the inverse/conjugate form used by camera-view composition.
 */
void __fastcall zMath_Quat_MultiplyInverse(
    const zQuat *quatA,
    const zQuat *quatB,
    zQuat *outAConjB
) {
    outAConjB->w =
        quatB->z * quatA->z + quatA->y * quatB->y + quatB->w * quatA->w + quatB->x * quatA->x;
    outAConjB->x =
        quatB->w * quatA->x - quatA->w * quatB->x - quatB->z * quatA->y + quatA->z * quatB->y;
    outAConjB->y =
        quatB->w * quatA->y - quatA->w * quatB->y - quatA->z * quatB->x + quatB->z * quatA->x;
    outAConjB->z =
        quatB->w * quatA->z - quatA->w * quatB->z - quatB->y * quatA->x + quatA->y * quatB->x;
}

/**
 * Reimplements 0x475a80: zMath_Quat_ToMatrix
 * (D:\Proj\GameZRecoil\zMath\zMath_Quat.cpp).
 * Purpose: expands a quaternion into the rotational part of a 4x3 matrix.
 */
void __fastcall zMath_Quat_ToMatrix(
    const zQuat *quat,
    zMat4x3 *outMatrix3x3
) {
    const float x2 = quat->x + quat->x;
    const float y2 = quat->y + quat->y;
    const float z2 = quat->z + quat->z;

    const float xx2 = x2 * quat->x;
    const float yy2 = y2 * quat->y;
    const float zz2 = z2 * quat->z;
    const float xy2 = y2 * quat->x;
    const float yz2 = z2 * quat->y;
    const float xz2 = x2 * quat->z;
    const float xw2 = x2 * quat->w;
    const float yw2 = y2 * quat->w;
    const float zw2 = z2 * quat->w;

    outMatrix3x3->xx = 1.0f - yy2 - zz2;
    outMatrix3x3->xy = zw2 + xy2;
    outMatrix3x3->xz = xz2 - yw2;
    outMatrix3x3->yx = xy2 - zw2;
    outMatrix3x3->yy = 1.0f - zz2 - xx2;
    outMatrix3x3->yz = xw2 + yz2;
    outMatrix3x3->zx = yw2 + xz2;
    outMatrix3x3->zy = yz2 - xw2;
    outMatrix3x3->zz = 1.0f - xx2 - yy2;
}

/**
 * Reimplements 0x475b80: zMath_Quat_FromRotationVector
 * (D:\Proj\GameZRecoil\zMath\zMath_Quat.cpp).
 * Purpose: converts a rotation vector into a quaternion, returning identity for a zero vector.
 */
void __fastcall zMath_Quat_FromRotationVector(
    const zVec3 *rotationVector,
    zQuat *outQuat
) {
    const float length = sqrt(
        rotationVector->x * rotationVector->x + rotationVector->y * rotationVector->y +
        rotationVector->z * rotationVector->z
    );

    if (length == 0.0f) {
        outQuat->w = 1.0f;
        outQuat->x = 0.0f;
        outQuat->y = 0.0f;
        outQuat->z = 0.0f;
        return;
    }

    const float sinLength = sin(length);
    const float scale = sinLength / length;
    outQuat->w = cos(length);
    outQuat->x = scale * rotationVector->x;
    outQuat->y = scale * rotationVector->y;
    outQuat->z = scale * rotationVector->z;
}

// Reimplements 0x4744f0: zMath_Vec3Array_AddScaled
void __fastcall zMath_Vec3Array_AddScaled(
    zVec3 *outArray,
    const zVec3 *biasArray,
    const zVec3 *srcArray,
    int count,
    float scale
) {
    for (int i = 0; i < count; ++i) {
        outArray[i].x = biasArray[i].x + srcArray[i].x * scale;
        outArray[i].y = biasArray[i].y + srcArray[i].y * scale;
        outArray[i].z = biasArray[i].z + srcArray[i].z * scale;
    }
}

/**
 * Reimplements 0x475070: zMath_Vec3_TriangleNormal.
 * Purpose: Computes a normalized triangle normal from the triangle edge cross product.
 */
void __fastcall zMath_Vec3_TriangleNormal(
    const zVec3 *p0,
    const zVec3 *p1,
    const zVec3 *p2,
    zVec3 *outNormal
) {
    const zVec3 edge01 = {p1->x - p0->x, p1->y - p0->y, p1->z - p0->z};
    const zVec3 edge02 = {p2->x - p0->x, p2->y - p0->y, p2->z - p0->z};

    outNormal->x = edge01.y * edge02.z - edge01.z * edge02.y;
    outNormal->y = edge01.z * edge02.x - edge01.x * edge02.z;
    outNormal->z = edge01.x * edge02.y - edge01.y * edge02.x;
    zMath::Vec3Normalize(outNormal);
}

/**
 * Reimplements 0x475130: zMath_SolveLinearGradient2D
 * (D:\Proj\GameZRecoil\zMath\zMathMisc.cpp).
 * Purpose: solves the screen-space linear gradient of a scalar over a triangle.
 * Data: reads the distinct shared zMath zero double at 0x4d2970
 * (g_zMath_DoubleZero2) and unit float at 0x4d297c; writes only the two
 * caller-supplied output floats.
 */
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
) {
    const float dxAB = ax - bx;
    const float dyAB = ay - by;
    const float dxCB = cx - bx;
    const float dyCB = cy - by;
    const float determinant = dyCB * dxAB - dxCB * dyAB;

    if (determinant == g_zMath_DoubleZero2) {
        *outDuDx = 0.0f;
        *outDuDy = 0.0f;
        return;
    }

    const float duAB = ua - ub;
    const float duCB = uc - ub;
    const float invDeterminant = g_zMath_MatrixUnitFloat / determinant;
    *outDuDx = (dyCB * duAB - duCB * dyAB) * invDeterminant;
    *outDuDy = (duCB * dxAB - dxCB * duAB) * invDeterminant;
}

/**
 * Reimplements 0x4727a0: zMath_Vec3_DivScalar (GameZRecoil/zMath/zmath_vec3.cpp).
 * Purpose: Divides a vector by a scalar while preserving the input vector for zero divisors.
 * Data: reads shared zMath scalar constants 0x4d2918 and 0x4d291c; writes
 * only the caller-supplied output vector.
 */
void __fastcall zMath_Vec3_DivScalar(
    const zVec3 *vec,
    zVec3 *out,
    float scalar
) {
    if (scalar == g_zMath_Vec3ZeroFloat) {
        if (out != vec) {
            *out = *vec;
        }
        return;
    }

    const float inverseScalar = g_zMath_Vec3UnitFloat / scalar;
    out->x = inverseScalar * vec->x;
    out->y = vec->y * inverseScalar;
    out->z = vec->z * inverseScalar;
}

// Reimplements 0x474b70: zMath_ProjectSphereBatch
void __fastcall zMath_ProjectSphereBatch(
    const zVec3 *spherePoints,
    zProjectedSphere *projectedSpheres,
    int count
) {
    for (int i = 0; i < count; ++i) {
        const float reciprocalZ = 1.0f / spherePoints[i].z;
        projectedSpheres[i].x =
            spherePoints[i].x * reciprocalZ * g_zMath_ProjScaleX + g_zMath_ProjOffsetX;
        projectedSpheres[i].y =
            spherePoints[i].y * reciprocalZ * g_zMath_ProjScaleY + g_zMath_ProjOffsetY;
        projectedSpheres[i].screenRadius = reciprocalZ * g_zMath_ProjSphereRadiusScale;
    }
}

namespace zFloat {
/**
 * Reimplements 0x490330: zFloat::Set255f
 * (Battlesport/zMath/zfloat.c).
 * Purpose: stores the byte-maximum float constant into the caller-supplied
 * output slot.
 * Data: writes only the caller-supplied float pointer.
 */
void __fastcall Set255f(
    float *value
) {
    *value = 255.0f;
}
} // namespace zFloat

/**
 * Reimplements 0x474870: zMath_Mat_TransformBBoxToCorners
 * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
 * Purpose: transforms a bounding box into its eight output corner positions.
 */
void __fastcall zMath_Mat_TransformBBoxToCorners(
    const zMat4x3 *matrix,
    const zBBox3f *bbox,
    zBBoxCorners *outCorners
) {
    TransformBBoxCorner(
        matrix,
        bbox->minX,
        bbox->minY,
        bbox->maxZ,
        &outCorners->values[0]
    );
    TransformBBoxCorner(
        matrix,
        bbox->maxX,
        bbox->minY,
        bbox->maxZ,
        &outCorners->values[3]
    );
    TransformBBoxCorner(
        matrix,
        bbox->maxX,
        bbox->minY,
        bbox->minZ,
        &outCorners->values[6]
    );
    TransformBBoxCorner(
        matrix,
        bbox->minX,
        bbox->minY,
        bbox->minZ,
        &outCorners->values[9]
    );
    TransformBBoxCorner(
        matrix,
        bbox->minX,
        bbox->maxY,
        bbox->maxZ,
        &outCorners->values[12]
    );
    TransformBBoxCorner(
        matrix,
        bbox->maxX,
        bbox->maxY,
        bbox->maxZ,
        &outCorners->values[15]
    );
    TransformBBoxCorner(
        matrix,
        bbox->maxX,
        bbox->maxY,
        bbox->minZ,
        &outCorners->values[18]
    );
    TransformBBoxCorner(
        matrix,
        bbox->minX,
        bbox->maxY,
        bbox->minZ,
        &outCorners->values[21]
    );
}
