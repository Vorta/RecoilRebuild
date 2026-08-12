#include "GameZRecoil/zMath/zmth.h"

#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zclass.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/**
 * Purpose: stores the projection scale, offset, inverse scale, and sphere
 * radius cache values derived by zMath projection setup.
 */
float g_zMath_ProjSphereRadiusScale = 0.0f;
float g_zMath_ProjScaleX = 0.0f;
float g_zMath_ProjScaleY = 0.0f;
float g_zMath_ProjOffsetX = 0.0f;
float g_zMath_ProjOffsetY = 0.0f;
float g_zMath_InvProjScaleX = 0.0f;
float g_zMath_InvProjScaleY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-clipzlowerbound
 * @recoil-artifact defines .data recoil:data:0x4e4880: g_zMath_ClipZLowerBound.
 * Purpose: stores the mutable lower Z clipping plane used by the zMath line
 * segment clipping helpers.
 */
float g_zMath_ClipZLowerBound = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-clipzupperbound
 * @recoil-artifact defines .data recoil:data:0x4e4890: g_zMath_ClipZUpperBound.
 * Purpose: stores the mutable upper Z clipping plane used by the zMath line
 * segment clipping helpers.
 */
float g_zMath_ClipZUpperBound = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-shared-zmath-midpoint-half-scalar
 * @recoil-artifact defines .rdata recoil:data:0x4d08d4: shared zMath midpoint half scalar.
 * Purpose: supplies Vec3Midpoint's component scale after summing both source
 * vectors.
 */
const float g_zMath_MidpointHalf = 0.5f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-shared-zmath-vector-zero-scalar
 * @recoil-artifact defines .rdata recoil:data:0x4d2918: shared zMath vector zero scalar.
 * Purpose: supplies float-zero comparisons for recovered vector helpers.
 */
const float g_zMath_Vec3ZeroFloat = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-shared-zmath-vector-unit-scalar
 * @recoil-artifact defines .rdata recoil:data:0x4d291c: shared zMath vector unit scalar.
 * Purpose: supplies reciprocal numerator constants for recovered vector
 * helpers.
 */
const float g_zMath_Vec3UnitFloat = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-shared-zmath-negative-unit-scalar
 * @recoil-artifact defines .rdata recoil:data:0x4d2928: shared zMath negative unit scalar.
 * Purpose: supplies the zero-dot reflection negation multiplier.
 */
const float g_zMath_Vec3NegUnitFloat = -1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-matrixunitfloat
 * @recoil-artifact defines .rdata recoil:data:0x4d297c: g_zMath_MatrixUnitFloat.
 * Purpose: supplies the shared zMath matrix/projection unit scalar.
 */
const float g_zMath_MatrixUnitFloat = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-doublezero
 * @recoil-artifact defines .rdata recoil:data:0x4d2920: g_zMath_DoubleZero.
 * Purpose: supplies the x87 zero comparisons used by zMath vector,
 * projection, and line/sphere intersection helpers.
 */
const double g_zMath_DoubleZero = 0.0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-distinct-shared-zmath-double-zero-scalar
 * @recoil-artifact defines .rdata recoil:data:0x4d2970: distinct shared zMath double zero scalar.
 * Purpose: supplies zMath_SolveLinearGradient2D's x87 double-zero comparison
 * for degenerate determinants.
 */
const double g_zMath_DoubleZero2 = 0.0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vector-direction-negative-dot-threshold
 * @recoil-artifact defines .rdata recoil:data:0x4d2930: zMath vector direction negative dot threshold.
 * Purpose: selects the antiparallel Vec3Slerp branch before building a
 * perpendicular direction.
 */
const double g_zMath_Vec3DirectionDotNegThreshold = -0.95;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-direction-pi-scalar
 * @recoil-artifact defines .rdata recoil:data:0x4d2938: zMath direction pi scalar.
 * Purpose: converts the Vec3Slerp antiparallel interpolation amount to
 * radians.
 */
const float g_zMath_DirectionToPiFloat = 3.14159274f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vector-direction-positive-dot-threshold
 * @recoil-artifact defines .rdata recoil:data:0x4d2948: zMath vector direction positive dot threshold.
 * Purpose: selects the near-linear Vec3Slerp branch for nearly aligned
 * vectors.
 */
const double g_zMath_Vec3DirectionDotPosThreshold = 0.95;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-elevationpifloat
 * @recoil-artifact defines .rdata recoil:data:0x4d2998: g_zMath_ElevationPiFloat.
 * Purpose: supplies the Euler roll adjustment pi scalar.
 */
const float g_zMath_ElevationPiFloat = 3.14159274f;
/**
 * Purpose: stores the mutable screen, focal, viewport, and projection-depth
 * cache values consumed by zMath projection and unprojection helpers.
 */
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
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-approxexpnegtable
 * @recoil-artifact defines .data recoil:data:0x566438: g_zMath_ApproxExpNegTable.
 * Purpose: stores the lazy approximate negative-exponential lookup table used
 * by zMath::ApproxExpNeg.
 */
float g_zMath_ApproxExpNegTable[256] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-approxexpnegscale
 * @recoil-artifact defines .data recoil:data:0x5669d0: g_zMath_ApproxExpNegScale.
 * Purpose: stores the table-index scale for zMath::ApproxExpNeg's lazy
 * approximate negative-exponential cache.
 */
float g_zMath_ApproxExpNegScale = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-approxexpnegdirty
 * @recoil-artifact defines .data recoil:data:0x4e0e8c: g_zMath_ApproxExpNegDirty.
 * Purpose: stores the rebuild flag for zMath::ApproxExpNeg's lazy lookup
 * table.
 */
int g_zMath_ApproxExpNegDirty = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfuncnamefloor
 * @recoil-artifact defines .data recoil:data:0x4e0e90: g_zMath_ExceptionFuncNameFloor.
 * Purpose: names the floor CRT math exception handled by zMath.
 */
char g_zMath_ExceptionFuncNameFloor[0x6] = "floor";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfuncnameceil
 * @recoil-artifact defines .data recoil:data:0x4e0e98: g_zMath_ExceptionFuncNameCeil.
 * Purpose: names the ceil CRT math exception handled by zMath.
 */
char g_zMath_ExceptionFuncNameCeil[0x5] = "ceil";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfuncnameasin
 * @recoil-artifact defines .data recoil:data:0x4e0ea0: g_zMath_ExceptionFuncNameAsin.
 * Purpose: names the asin CRT math exception clamped by zMath.
 */
char g_zMath_ExceptionFuncNameAsin[0x5] = "asin";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfmt
 * @recoil-artifact defines .data recoil:data:0x4e0ea8: g_zMath_ExceptionFmt.
 * Purpose: formats the stderr CRT math exception diagnostic line.
 */
char g_zMath_ExceptionFmt[0x2b] = "Math Exception: type=%d, [%s(%.8f, %.8f)]\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-sourcefile-zmthmainc
 * @recoil-artifact defines .data recoil:data:0x4e0ed4: g_zMath_SourceFile_ZmthMainC.
 * Purpose: supplies the recovered zmth_main.c source path for zError math
 * exception reports.
 */
char g_zMath_SourceFile_ZmthMainC[0x26] =
    "D:\\Proj\\GameZRecoil\\zMath\\zmth_main.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfmtnonewline
 * @recoil-artifact defines .data recoil:data:0x4e0efc: g_zMath_ExceptionFmtNoNewline.
 * Purpose: formats the zError CRT math exception diagnostic message.
 */
char g_zMath_ExceptionFmtNoNewline[0x2a] =
    "Math Exception: type=%d, [%s(%.8f, %.8f)]";


/**
 * Provider-boundary CRT hook: user-supplied _matherr installed by VC5 CRTEXE startup.
 * Purpose: exposes the zMath math exception handler to the CRT without pulling
 * the default MSVCRT merr.obj handler.
 */
extern "C" int __cdecl _matherr(
    _exception *except
) {
    return zMath::CrtMatherrHandler(except);
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

} // namespace






namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-camerascratchb
 * @recoil-artifact defines .data recoil:data:0x5668e8: g_zMath_CameraScratchB
 * Purpose: stores the camera inverse-rotation scratch matrix loaded by the
 * camera setup/projection/view helpers.
 */
zMat4x3 g_zMath_CameraScratchB = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-camerascratcha
 * @recoil-artifact defines .data recoil:data:0x566920: g_zMath_CameraScratchA
 * Purpose: stores the staged camera world matrix before the inverse-rotation
 * transpose is copied into camera scratch B.
 */
zMat4x3 g_zMath_CameraScratchA = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-g-zmath-vec3zero
 * @recoil-artifact defines .data recoil:data:0x5669d8: zMath::g_zMath_Vec3Zero
 * Purpose: shared writable zero vector read by zMath view-matrix setup and
 * projectile runtime initialization.
 */
zVec3 g_zMath_Vec3Zero = {0};
zVec3 g_zMath_Vec3DeltaScratch = {0};
int *g_currentMatrixIdentityFlagSlot = &g_matrixIdentityFlagSlots[0];
float **g_currentMatrixPtrSlot = &g_matrixSlots[0];

















// Retail keeps an EBP frame for this leaf under the VC5SP3 /O2 profile.
#pragma optimize("y", off)

#pragma optimize("", on)




















/**
 * Purpose: moves one segment endpoint onto the caller-supplied Z clip plane by
 * interpolating toward the other endpoint.
 * Data: writes only the caller-supplied endpoint and reads no authored globals.
 */
/**
 * Purpose: clips a mutable segment against the current zMath lower and upper
 * Z clipping planes, rejecting segments fully outside the range.
 * Data: reads g_zMath_ClipZLowerBound at 0x4e4880 and
 * g_zMath_ClipZUpperBound at 0x4e4890.
 */
} // namespace zMath



























namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3deltalengthsq-gamezrecoil-zmath-cpp
 * @recoil-artifact defines .text recoil:function:0x472670: zMath::Vec3DeltaLengthSq (GameZRecoil/zMath.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3deltalength-gamezrecoil-zmath-cpp
 * @recoil-artifact defines .text recoil:function:0x4726d0: zMath::Vec3DeltaLength (GameZRecoil/zMath.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3distsqxz-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472730: zMath::Vec3DistSqXZ (GameZRecoil/zMath/zmath_vec3.cpp).
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3scaleadd-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472770: zMath::Vec3ScaleAdd (GameZRecoil/zMath/zmath_vec3.cpp).
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-divscalar-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4727a0: zMath_Vec3_DivScalar (GameZRecoil/zMath/zmath_vec3.cpp).
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

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3normalizexz-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4727f0: zMath::Vec3NormalizeXZ (GameZRecoil/zMath/zmath_vec3.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3reflect-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472860: zMath::Vec3Reflect (GameZRecoil/zMath/zmath_vec3.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3lerp-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472960: zMath::Vec3Lerp (GameZRecoil/zMath/zmath_vec3.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3directionto-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4729b0: zMath::Vec3DirectionTo (GameZRecoil/zMath/zmath_vec3.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3lerpnormalize-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4729f0: zMath::Vec3LerpNormalize (GameZRecoil/zMath/zmath_vec3.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3slerp-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472a10: zMath::Vec3Slerp (GameZRecoil/zMath/zmath_vec3.cpp).
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

    const float dot =
        a->x * b->x +
        a->y * b->y +
        a->z * b->z;
    if (dot < g_zMath_Vec3DirectionDotNegThreshold) {
        zVec3 perpendicular;
        Vec3Perp2D(
            a,
            &perpendicular
        );

        const float angle = g_zMath_DirectionToPiFloat * t;
        const float sinAngle = sin(angle);
        const float cosAngle = cos(angle);
        out->x = a->x * cosAngle + perpendicular.x * sinAngle;
        out->y = a->y * cosAngle + perpendicular.y * sinAngle;
        out->z = a->z * cosAngle + perpendicular.z * sinAngle;
        return;
    }

    if (dot > g_zMath_Vec3DirectionDotPosThreshold) {
        const float aScale = g_zMath_Vec3UnitFloat - t;
        out->x = a->x * aScale + b->x * t;
        out->y = a->y * aScale + b->y * t;
        out->z = a->z * aScale + b->z * t;
        return;
    }

    const float sinOmegaSq = g_zMath_Vec3UnitFloat - dot * dot;
    float sinOmega = 0.0f;
    if (sinOmegaSq > g_zMath_Vec3ZeroFloat) {
        unsigned int sinOmegaBits = 0;
        memcpy(
            &sinOmegaBits,
            &sinOmegaSq,
            sizeof(sinOmegaBits)
        );
        sinOmegaBits = (sinOmegaBits >> 1) + 0x1fc00000u;
        memcpy(
            &sinOmega,
            &sinOmegaBits,
            sizeof(sinOmega)
        );
    }
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3perp2d-gamezrecoil-zmath-zmath-vec2-cpp
 * @recoil-artifact defines .text recoil:function:0x472cc0: zMath::Vec3Perp2D (GameZRecoil/zMath/zmath_vec2.cpp).
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
    unsigned int lengthBits = 0;
    memcpy(
        &lengthBits,
        &lengthSq,
        sizeof(lengthBits)
    );
    lengthBits = (lengthBits >> 1) + 0x1fc00000u;
    float length = 0.0f;
    memcpy(
        &length,
        &lengthBits,
        sizeof(length)
    );
    const float invLength = g_zMath_Vec3UnitFloat / length;
    out->x = in->y * invLength;
    out->y = -(in->x * invLength);
}
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-crtmatherrhandler
 * @recoil-artifact defines .text recoil:function:0x472d30: zMath::CrtMatherrHandler
 * Purpose: reports CRT math exceptions and supplies recovered return values
 * for zMath asin, ceil, and floor failures.
 */
int __cdecl zMath::CrtMatherrHandler(
    _exception *except
) {
    zError::ReportOld(
        0x400,
        g_zMath_SourceFile_ZmthMainC,
        376,
        g_zMath_ExceptionFmtNoNewline,
        except->type,
        except->name,
        except->arg1,
        except->arg2
    );
    fprintf(
        stderr,
        g_zMath_ExceptionFmt,
        except->type,
        except->name,
        except->arg1,
        except->arg2
    );

    if (strcmp(
        except->name,
        g_zMath_ExceptionFuncNameAsin
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
        g_zMath_ExceptionFuncNameCeil
    ) == 0) {
        except->retval = 0.0;
        return 1;
    }

    if (strcmp(
        except->name,
        g_zMath_ExceptionFuncNameFloor
    ) == 0) {
        except->retval = 0.0;
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-project-getlastscreenscalexy
 * @recoil-artifact defines .text recoil:function:0x472ed0: zMath_Project_GetLastScreenScaleXY.
 * Purpose: returns the last cached projection X/Y scale values as a zVec2.
 */
zVec2 __cdecl zMath_Project_GetLastScreenScaleXY() {
    zVec2 scale;
    scale.x = g_zMath_ProjScaleX;
    scale.y = g_zMath_ProjScaleY;
    return scale;
}

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matstackpushandcloneparent
 * @recoil-artifact defines .text recoil:function:0x472ef0: zMath::MatStackPushAndCloneParent.
 * Purpose: pushes a caller-supplied matrix slot and clones the parent matrix
 * and identity flag into the new top-of-stack slot.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matstackpushptr
 * @recoil-artifact defines .text recoil:function:0x472f30: zMath::MatStackPushPtr.
 * Purpose: pushes a caller-supplied matrix pointer onto the zMath matrix
 * stack and marks the new slot non-identity.
 */
void __fastcall MatStackPushPtr(
    float *matrix
) {
    ++g_currentMatrixIdentityFlagSlot;
    ++g_currentMatrixPtrSlot;
    *g_currentMatrixPtrSlot = matrix;
    *g_currentMatrixIdentityFlagSlot = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matstackpopptr
 * @recoil-artifact defines .text recoil:function:0x472f60: zMath::MatStackPopPtr.
 * Purpose: pops the current zMath matrix pointer and identity-flag slots.
 */
void __cdecl MatStackPopPtr() {
    --g_currentMatrixIdentityFlagSlot;
    --g_currentMatrixPtrSlot;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadcamerascratchb
 * @recoil-artifact defines .text recoil:function:0x472f90: zMath::MatLoadCameraScratchB
 * Purpose: loads camera scratch B into the current matrix stack slot.
 */
void __cdecl MatLoadCameraScratchB() {
    MatLoadCurrentFrom(&g_zMath_CameraScratchB);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadcamerascratcha
 * @recoil-artifact defines .text recoil:function:0x472fa0: zMath::MatLoadCameraScratchA
 * Purpose: loads camera scratch A into the current matrix stack slot.
 */
void __cdecl MatLoadCameraScratchA() {
    MatLoadCurrentFrom(&g_zMath_CameraScratchA);
}
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-loadprojection
 * @recoil-artifact defines .text recoil:function:0x472fb0: zMath_Mat_LoadProjection
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

    zMat4x3 slotBuffer;
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-loadview
 * @recoil-artifact defines .text recoil:function:0x473060: zMath_Mat_LoadView
 * Purpose: builds the current view matrix from camera and parent transforms.
 */
void __cdecl zMath_Mat_LoadView() {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-setupcamera
 * @recoil-artifact defines .text recoil:function:0x4731f0: zMath_Mat_SetupCamera
 * Purpose: loads camera scratch B and composes it through the parent matrix
 * stack slot.
 */
void __cdecl zMath_Mat_SetupCamera() {
    zMath::MatLoadCameraScratchB();
    zMath::MatMultiply(
        (const zMat4x3 *)(zMath::g_currentMatrixPtrSlot[-1]),
        1
    );
}

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matcopycurrentto
 * @recoil-artifact defines .text recoil:function:0x473210: zMath::MatCopyCurrentTo.
 * Purpose: copies the current matrix stack slot into caller-provided storage
 * and returns that storage pointer.
 */
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-getcurrent
 * @recoil-artifact defines .text recoil:function:0x473230: zMath_Mat_GetCurrent.
 * Purpose: returns the current zMath matrix stack slot as a 4x3 matrix.
 */
zMat4x3 *__cdecl zMath_Mat_GetCurrent() {
    return (zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-iscurrentidentity
 * @recoil-artifact defines .text recoil:function:0x473240: zMath_Mat_IsCurrentIdentity.
 * Purpose: returns the identity flag for the current zMath matrix stack slot.
 */
int __cdecl zMath_Mat_IsCurrentIdentity() {
    return *zMath::g_currentMatrixIdentityFlagSlot;
}

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadcurrentfrom
 * @recoil-artifact defines .text recoil:function:0x473250: zMath::MatLoadCurrentFrom.
 * Purpose: copies a caller-supplied 4x3 matrix into the current zMath matrix
 * stack slot and clears the identity flag.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadrotationfrom3x3
 * @recoil-artifact defines .text recoil:function:0x473280: zMath::MatLoadRotationFrom3x3.
 * Purpose: loads only the 3x3 rotation rows into the current matrix stack
 * slot and marks the slot non-identity.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadidentity
 * @recoil-artifact defines .text recoil:function:0x4732f0: zMath::MatLoadIdentity.
 * Purpose: writes an identity 4x3 matrix into the current matrix stack slot
 * and marks the slot as identity.
 */
void __cdecl MatLoadIdentity() {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matmultiply
 * @recoil-artifact defines .text recoil:function:0x473370: zMath::MatMultiply.
 * Purpose: multiplies the current matrix stack slot by a source matrix,
 * optionally preserving the current translation for mode 2.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-scale-gamezrecoil-zmath-zmath-matrix-cpp
 * @recoil-artifact defines .text recoil:function:0x473690: zMath::MatScale (GameZRecoil/zMath/zmath_matrix.cpp).
 * Purpose: Applies per-axis scale to the current matrix basis while preserving translation.
 */
void __stdcall MatScale(
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mattranslate
 * @recoil-artifact defines .text recoil:function:0x4737e0: zMath::MatTranslate.
 * Purpose: applies a local translation through the current matrix basis and
 * updates the current matrix stack slot.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matrotatex
 * @recoil-artifact defines .text recoil:function:0x473970: zMath::MatRotateX.
 * Purpose: applies an X-axis rotation to the current matrix stack slot.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matrotatey
 * @recoil-artifact defines .text recoil:function:0x473b10: zMath::MatRotateY.
 * Purpose: applies a Y-axis rotation to the current matrix stack slot while
 * preserving translation.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matrotatez
 * @recoil-artifact defines .text recoil:function:0x473cc0: zMath::MatRotateZ.
 * Purpose: applies a Z-axis rotation to the current matrix stack slot.
 */
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-camera-stageinverserotation
 * @recoil-artifact defines .text recoil:function:0x473e60: zMath_Camera_StageInverseRotation
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

    *(unsigned int *)(&zMath::g_zMath_CameraScratchA.yx) ^= 0x80000000u;
    *(unsigned int *)(&zMath::g_zMath_CameraScratchA.yy) ^= 0x80000000u;
    *(unsigned int *)(&zMath::g_zMath_CameraScratchA.yz) ^= 0x80000000u;
    *(unsigned int *)(&zMath::g_zMath_CameraScratchA.zx) ^= 0x80000000u;
    *(unsigned int *)(&zMath::g_zMath_CameraScratchA.zy) ^= 0x80000000u;
    *(unsigned int *)(&zMath::g_zMath_CameraScratchA.zz) ^= 0x80000000u;

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

    *(unsigned int *)(&zMath::g_zMath_CameraScratchB.posX) ^= 0x80000000u;
    *(unsigned int *)(&zMath::g_zMath_CameraScratchB.posY) ^= 0x80000000u;
    *(unsigned int *)(&zMath::g_zMath_CameraScratchB.posZ) ^= 0x80000000u;

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

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3arrayprojecttocachedy
 * @recoil-artifact defines .text recoil:function:0x473fc0: zMath::Vec3ArrayProjectToCachedY.
 * Purpose: projects an array of points against cached camera scratch row Y
 * into caller-provided scalar output storage.
 */
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matapplylocaltrs
 * @recoil-artifact defines .text recoil:function:0x474010: zMath::MatApplyLocalTRS.
 * Purpose: builds a local transform from Euler angles, position, and scale,
 * then composes it into the current matrix stack slot.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matbuildeulerrotation3x3
 * @recoil-artifact defines .text recoil:function:0x474260: zMath::MatBuildEulerRotation3x3.
 * Purpose: builds a 3x3 Euler rotation basis in caller-provided matrix
 * storage and clears the translation row.
 */
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-setscreensize-gamezrecoil-zmath-zmath-proj-cpp
 * @recoil-artifact defines .text recoil:function:0x4743e0: zMath_SetScreenSize (GameZRecoil/zMath/zmath_proj.cpp).
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-setup-projection-gamezrecoil-zmath-zmath-proj-cpp
 * @recoil-artifact defines .text recoil:function:0x474400: zMath_Setup_Projection (GameZRecoil/zMath/zmath_proj.cpp).
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3array-addscaled
 * @recoil-artifact defines .text recoil:function:0x4744f0: zMath_Vec3Array_AddScaled.
 * Purpose: writes bias plus scaled source vectors across a caller-provided
 * vector array.
 */
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-dirfromyaw
 * @recoil-artifact defines .text recoil:function:0x474580: zMath_Vec3_DirFromYaw
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

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3perpxz
 * @recoil-artifact defines .text recoil:function:0x4745c0: zMath::Vec3PerpXZ.
 * Purpose: builds the XZ-plane perpendicular vector with a zero Y component.
 */
void __fastcall Vec3PerpXZ(
    const zVec3 *in,
    zVec3 *out
) {
    out->x = -in->z;
    out->z = in->x;
    out->y = 0.0f;
}
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3array-untransformdirection
 * @recoil-artifact defines .text recoil:function:0x4745e0: zMath_Vec3Array_UntransformDirection.
 * Purpose: applies the current matrix rotation columns to direction vectors
 * in place when the matrix stack slot is not identity.
 */
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

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3arraytransformdirection
 * @recoil-artifact defines .text recoil:function:0x474670: zMath::Vec3ArrayTransformDirection.
 * Purpose: transforms direction vectors in place by the current matrix
 * rotation when the matrix stack slot is non-identity.
 */
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-transformnormalbatch
 * @recoil-artifact defines .text recoil:function:0x474710: zMath_Mat_TransformNormalBatch
 * Purpose: transforms normal batches through the current matrix rotation, or
 * copies the input normals unchanged when the current matrix is identity.
 */
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

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mattransformpointbatchinplace
 * @recoil-artifact defines .text recoil:function:0x4747d0: zMath::MatTransformPointBatchInPlace.
 * Purpose: transforms an array of points in place by the current 4x3 matrix
 * when the matrix stack slot is non-identity.
 */
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-transformbboxtocorners
 * @recoil-artifact defines .text recoil:function:0x474870: zMath_Mat_TransformBBoxToCorners
 * Purpose: transforms a bounding box into its eight output corner positions.
 */
void __fastcall zMath_Mat_TransformBBoxToCorners(
    const zMat4x3 *matrix,
    const zBBox3f *bbox,
    zBBoxCorners *outCorners
) {
    outCorners->values[0] = bbox->minX * matrix->xx + bbox->minY * matrix->yx +
        bbox->maxZ * matrix->zx + matrix->posX;
    outCorners->values[1] = bbox->minX * matrix->xy + bbox->minY * matrix->yy +
        bbox->maxZ * matrix->zy + matrix->posY;
    outCorners->values[2] = bbox->minX * matrix->xz + bbox->minY * matrix->yz +
        bbox->maxZ * matrix->zz + matrix->posZ;

    outCorners->values[3] = bbox->maxX * matrix->xx + bbox->minY * matrix->yx +
        bbox->maxZ * matrix->zx + matrix->posX;
    outCorners->values[4] = bbox->maxX * matrix->xy + bbox->minY * matrix->yy +
        bbox->maxZ * matrix->zy + matrix->posY;
    outCorners->values[5] = bbox->maxX * matrix->xz + bbox->minY * matrix->yz +
        bbox->maxZ * matrix->zz + matrix->posZ;

    outCorners->values[6] = bbox->maxX * matrix->xx + bbox->minY * matrix->yx +
        bbox->minZ * matrix->zx + matrix->posX;
    outCorners->values[7] = bbox->maxX * matrix->xy + bbox->minY * matrix->yy +
        bbox->minZ * matrix->zy + matrix->posY;
    outCorners->values[8] = bbox->maxX * matrix->xz + bbox->minY * matrix->yz +
        bbox->minZ * matrix->zz + matrix->posZ;

    outCorners->values[9] = bbox->minX * matrix->xx + bbox->minY * matrix->yx +
        bbox->minZ * matrix->zx + matrix->posX;
    outCorners->values[10] = bbox->minX * matrix->xy + bbox->minY * matrix->yy +
        bbox->minZ * matrix->zy + matrix->posY;
    outCorners->values[11] = bbox->minX * matrix->xz + bbox->minY * matrix->yz +
        bbox->minZ * matrix->zz + matrix->posZ;

    outCorners->values[12] = bbox->minX * matrix->xx + bbox->maxY * matrix->yx +
        bbox->maxZ * matrix->zx + matrix->posX;
    outCorners->values[13] = bbox->minX * matrix->xy + bbox->maxY * matrix->yy +
        bbox->maxZ * matrix->zy + matrix->posY;
    outCorners->values[14] = bbox->minX * matrix->xz + bbox->maxY * matrix->yz +
        bbox->maxZ * matrix->zz + matrix->posZ;

    outCorners->values[15] = bbox->maxX * matrix->xx + bbox->maxY * matrix->yx +
        bbox->maxZ * matrix->zx + matrix->posX;
    outCorners->values[16] = bbox->maxX * matrix->xy + bbox->maxY * matrix->yy +
        bbox->maxZ * matrix->zy + matrix->posY;
    outCorners->values[17] = bbox->maxX * matrix->xz + bbox->maxY * matrix->yz +
        bbox->maxZ * matrix->zz + matrix->posZ;

    outCorners->values[18] = bbox->maxX * matrix->xx + bbox->maxY * matrix->yx +
        bbox->minZ * matrix->zx + matrix->posX;
    outCorners->values[19] = bbox->maxX * matrix->xy + bbox->maxY * matrix->yy +
        bbox->minZ * matrix->zy + matrix->posY;
    outCorners->values[20] = bbox->maxX * matrix->xz + bbox->maxY * matrix->yz +
        bbox->minZ * matrix->zz + matrix->posZ;

    outCorners->values[21] = bbox->minX * matrix->xx + bbox->maxY * matrix->yx +
        bbox->minZ * matrix->zx + matrix->posX;
    outCorners->values[22] = bbox->minX * matrix->xy + bbox->maxY * matrix->yy +
        bbox->minZ * matrix->zy + matrix->posY;
    outCorners->values[23] = bbox->minX * matrix->xz + bbox->maxY * matrix->yz +
        bbox->minZ * matrix->zz + matrix->posZ;
}

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-projectpointbatch
 * @recoil-artifact defines .text recoil:function:0x474b20: zMath::ProjectPointBatch.
 * Purpose: projects view-space points to screen coordinates and reciprocal-Z
 * values using the cached zMath projection globals.
 */
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-projectspherebatch
 * @recoil-artifact defines .text recoil:function:0x474b70: zMath_ProjectSphereBatch.
 * Purpose: projects sphere centers to screen space and scales radii from
 * reciprocal Z using cached zMath projection globals.
 */
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-unprojectpointbatch
 * @recoil-artifact defines .text recoil:function:0x474bc0: zMath_UnprojectPointBatch
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-unprojectpointbatchzbuf
 * @recoil-artifact defines .text recoil:function:0x474c20: zMath_UnprojectPointBatchZBuf
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
    zMath::MatStackPopPtr();
}

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3directionanglesbetweenpoints
 * @recoil-artifact defines .text recoil:function:0x474d10: zMath::Vec3DirectionAnglesBetweenPoints.
 * Purpose: computes pitch and yaw angles from one point toward another and
 * clears roll in the output vector.
 */
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
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-elevationanglebetweenpoints
 * @recoil-artifact defines .text recoil:function:0x474d90: zMath_Vec3_ElevationAngleBetweenPoints.
 * Purpose: computes the elevation angle between two points from horizontal
 * distance and vertical delta.
 */
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-extractyaw
 * @recoil-artifact defines .text recoil:function:0x474de0: zMath_Mat_ExtractYaw.
 * Purpose: extracts yaw from the Z basis row of a 4x3 matrix, returning zero
 * for a degenerate horizontal basis.
 */
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-extracteulerangles
 * @recoil-artifact defines .text recoil:function:0x474e10: zMath_Mat_ExtractEulerAngles
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-rotatex
 * @recoil-artifact defines .text recoil:function:0x474ec0: zMath_Vec3_RotateX.
 * Purpose: rotates one vector around the X axis into caller-provided output.
 */
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

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3rotatey-gamezrecoil-zmath-zmath-vec-cpp
 * @recoil-artifact defines .text recoil:function:0x474f40: zMath::Vec3RotateY (GameZRecoil/zMath/zmath_vec.cpp).
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-approxexpneg
 * @recoil-artifact defines .text recoil:function:0x474fc0: zMath::ApproxExpNeg.
 * Purpose: lazily builds and samples the 256-entry approximate e^-x lookup
 * table with edge clamps for negative and out-of-range inputs.
 */
float __stdcall ApproxExpNeg(
    float x
) {
    if (g_zMath_ApproxExpNegDirty != 0) {
        g_zMath_ApproxExpNegScale = 51.0f;
        for (int i = 0; i < 256; ++i) {
            g_zMath_ApproxExpNegTable[i] = exp(-(float)(i) * 0.0196078438f);
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

#pragma optimize("y", off)
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-trianglenormal
 * @recoil-artifact defines .text recoil:function:0x475070: zMath_Vec3_TriangleNormal.
 * Purpose: Computes a normalized triangle normal from the triangle edge cross product.
 */
void __fastcall zMath_Vec3_TriangleNormal(
    const zVec3 *p0,
    const zVec3 *p1,
    const zVec3 *p2,
    zVec3 *outNormal
) {
    zVec3 edge01;
    edge01.x = p1->x - p0->x;
    edge01.y = p1->y - p0->y;
    edge01.z = p1->z - p0->z;

    zVec3 edge02;
    edge02.x = p2->x - p0->x;
    edge02.y = p2->y - p0->y;
    edge02.z = p2->z - p0->z;

    outNormal->x = edge01.y * edge02.z - edge01.z * edge02.y;
    outNormal->y = edge01.z * edge02.x - edge01.x * edge02.z;
    outNormal->z = edge01.x * edge02.y - edge01.y * edge02.x;
    zMath::Vec3Normalize(outNormal);
}
#pragma optimize("", on)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-solvelineargradient2d
 * @recoil-artifact defines .text recoil:function:0x475130: zMath_SolveLinearGradient2D
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

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-linevsspherehit
 * @recoil-artifact defines .text recoil:function:0x475210: zMath::LineVsSphereHit
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

    const float lineLengthSq = lineDelta.x * lineDelta.x +
        lineDelta.y * lineDelta.y + lineDelta.z * lineDelta.z;
    if (lineLengthSq == 0.0f) {
        return 0;
    }

    const float centerDotLine = sphereCenterRelSegB->x * lineDelta.x +
        sphereCenterRelSegB->y * lineDelta.y + sphereCenterRelSegB->z * lineDelta.z;
    float centerDistMinusRadius =
        sphereCenterRelSegB->x * sphereCenterRelSegB->x +
        sphereCenterRelSegB->y * sphereCenterRelSegB->y +
        sphereCenterRelSegB->z * sphereCenterRelSegB->z - radius * radius;

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

        unsigned int discriminantBits = 0;
        memcpy(
            &discriminantBits,
            &discriminant,
            sizeof(discriminantBits)
        );
        discriminantBits =
            (discriminantBits >> 1) + 0x1fc00000u;
        float discriminantRoot = 0.0f;
        memcpy(
            &discriminantRoot,
            &discriminantBits,
            sizeof(discriminantRoot)
        );
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
    return Vec3Normalize(outInwardNormal) >= 0.0f;
}
} // namespace zMath

// Retail code keeps an EBP frame for this perspective-gradient helper under the
// VC5SP3 /O2 profile; disable only frame-pointer omission for the function.
#pragma optimize("y", off)
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-buildperspectivetextureinterpolants
 * @recoil-artifact defines .text recoil:function:0x4753e0: zMath_BuildPerspectiveTextureInterpolants
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
    const zVec3 edge21 = {
        triVerts[2].x - triVerts[1].x,
        triVerts[2].y - triVerts[1].y,
        triVerts[2].z - triVerts[1].z
    };
    const zVec3 edge01 = {
        triVerts[0].x - triVerts[1].x,
        triVerts[0].y - triVerts[1].y,
        triVerts[0].z - triVerts[1].z
    };
    const zVec3 normal = {
        edge21.y * edge01.z - edge21.z * edge01.y,
        edge21.z * edge01.x - edge21.x * edge01.z,
        edge21.x * edge01.y - edge21.y * edge01.x
    };
    const float normalDotOrigin = normal.x * triVerts[0].x +
        normal.y * triVerts[0].y + normal.z * triVerts[0].z;

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

    const float edge21LenSq = edge21.x * edge21.x + edge21.y * edge21.y +
        edge21.z * edge21.z;
    const float edge01LenSq = edge01.x * edge01.x + edge01.y * edge01.y +
        edge01.z * edge01.z;
    const float edgeDot = edge21.x * edge01.x + edge21.y * edge01.y +
        edge21.z * edge01.z;
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
    const float uOriginDelta = triUVs[0].x -
        (uPlane.x * triVerts[0].x + uPlane.y * triVerts[0].y +
            uPlane.z * triVerts[0].z);

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
    const float vOriginDelta = triUVs[0].y -
        (vPlane.x * triVerts[0].x + vPlane.y * triVerts[0].y +
            vPlane.z * triVerts[0].z);

    outVOverZGrad->x = vOriginDelta * outRecipZGrad->x + vPlane.x * g_zMath_InvProjScaleX;
    outVOverZGrad->y = vOriginDelta * outRecipZGrad->y + vPlane.y * g_zMath_InvProjScaleY;
    *outVOverZBase = vOriginDelta * *outRecipZBase + vPlane.z;
}
#pragma optimize("", on)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-fromeuler
 * @recoil-artifact defines .text recoil:function:0x4757c0: zMath_Quat_FromEuler
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiply
 * @recoil-artifact defines .text recoil:function:0x475910: zMath_Quat_Multiply
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiplyinverse
 * @recoil-artifact defines .text recoil:function:0x4759d0: zMath_Quat_MultiplyInverse
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-tomatrix
 * @recoil-artifact defines .text recoil:function:0x475a80: zMath_Quat_ToMatrix
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-fromrotationvector
 * @recoil-artifact defines .text recoil:function:0x475b80: zMath_Quat_FromRotationVector
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
