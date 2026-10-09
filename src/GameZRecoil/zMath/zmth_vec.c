// zMath compilation unit between zin_joystick.cpp and zmth_main.c, inferred
// from the retail object boundary [0x472670, 0x472d30): its .rdata pooled
// constants [0x4d2918, 0x4d2950) repeat values that zmth_main.c pools again at
// [0x4d2950, 0x4d29b0) (0.0f, 1.0f, 0.0, -1.0f, pi and the 9.22e18 SinCos
// threshold), and its .bss [0x566420, 0x56642c) holds the delta scratch
// vector. The 1998-09-28 demo has the same two pools. Original filename
// unresolved; zmth_vec.c is a provisional name (2026-10-06).
// The include block repeats zmth_main.c's prelude.
#include "GameZRecoil/zMath/zmth.h"

#include "GameZRecoil/zError/zerr.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

zVec3 g_zMath_Vec3DeltaScratch = { 0 };

// Retail keeps an EBP frame for this leaf under the VC5SP3 /O2 profile.
#pragma optimize("y", off)

#pragma optimize("", on)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3deltalengthsq-gamezrecoil-zmath-cpp
 * @recoil-artifact defines .text recoil:function:0x472670: zMath::Vec3DeltaLengthSq.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.delta-square-sum
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.delta-square-sum
 * @recoil-match byte
 *
 * Purpose: Use reviewed raw assembly for retail's rounded XYZ deltas and squared-length result.
 */
float __fastcall Vec3DeltaLengthSq(const zVec3* a, const zVec3* b)
{
    float lengthSq;
    Vec3Subtract(a, b, &g_zMath_Vec3DeltaScratch);
    /**
     * Purpose: Store the grouped XYZ square sum as a float; ECX clobbered, x87 depths 0/3/0.
     */
    __asm {
    mov ecx, offset g_zMath_Vec3DeltaScratch
    fld dword ptr [ecx]zVec3.x
    fmul dword ptr [ecx]zVec3.x
    fld dword ptr [ecx]zVec3.y
    fmul dword ptr [ecx]zVec3.y
    fld dword ptr [ecx]zVec3.z
    fmul dword ptr [ecx]zVec3.z
    fxch st(1)
    faddp st(2), st(0)
    faddp st(1), st(0)
    fstp lengthSq
    }
    return lengthSq;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vec.vector-length
 *
 * Purpose: return FSQRT of the grouped (x*x + y*y) + z*z sum as binary32.
 * Reconstruction: zmth_vec.c-resident copy of the Camera.c inline helper,
 * following the zmth_quat.c and zwep_ammo.c precedent.
 * Raw assembly: identical body to the reviewed Camera.c Vec3Length island;
 * retail inlines it at [0x472702,0x472722) with the constant scratch
 * address and at [0x472809,0x472827) bound to its parameter home.
 * Original inline helper evidence: no standalone retail function.
 */
__inline float Vec3Length(const zVec3* vec)
{
    float vecLength;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
    mov ecx, vec
    fld dword ptr [ecx]zVec3.x
    fmul dword ptr [ecx]zVec3.x
    fld dword ptr [ecx]zVec3.y
    fmul dword ptr [ecx]zVec3.y
    fld dword ptr [ecx]zVec3.z
    fmul dword ptr [ecx]zVec3.z
    fxch st(1)
    faddp st(2), st
    faddp st(1), st
    fsqrt
    fstp vecLength
    }
#else
    vecLength = (float)sqrt((vec->x * vec->x + vec->y * vec->y) + vec->z * vec->z);
#endif
    return vecLength;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vec.vector-length-sq-xz
 *
 * Purpose: return the x*x + z*z square sum rounded to binary32.
 * Raw assembly: retail [0x47274a,0x47275e) inside 0x472730. Contract: uses
 * and clobbers ECX; x87 entry/peak/exit depth 0/2/0 on normal completion;
 * reads X and Z only and stores the result as binary32. The C fallback is
 * the arithmetic reference, not a code-generation match.
 * Retail inline-expansion evidence: the consumer contains the reload, the
 * X/Z products and the result store without a call; spelling and header
 * ownership are inferred.
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x472730.
 */
__inline float Vec3LengthSqXZ(const zVec3* vec)
{
    float lengthSq;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
    mov ecx, vec
    fld dword ptr [ecx]zVec3.x
    fmul dword ptr [ecx]zVec3.x
    fld dword ptr [ecx]zVec3.z
    fmul dword ptr [ecx]zVec3.z
    faddp st(1), st
    fstp lengthSq
    }
#else
    lengthSq = vec->x * vec->x + vec->z * vec->z;
#endif
    return lengthSq;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3deltalength-gamezrecoil-zmath-cpp
 * @recoil-artifact defines .text recoil:function:0x4726d0: zMath::Vec3DeltaLength (GameZRecoil/zMath.cpp).
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vec.vector-length
 * @recoil-match byte
 *
 * Purpose: Stores the vector delta in the shared scratch vector and returns its length.
 */
float __fastcall Vec3DeltaLength(const zVec3* a, const zVec3* b)
{
    Vec3Subtract(a, b, &g_zMath_Vec3DeltaScratch);
    return Vec3Length(&g_zMath_Vec3DeltaScratch);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3distsqxz-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472730: zMath::Vec3DistSqXZ (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vec.vector-length-sq-xz
 * @recoil-match byte
 *
 * Purpose: Stores the XZ delta in the shared scratch vector and returns squared XZ-plane distance.
 */
float __fastcall Vec3DistSqXZ(const zVec3* a, const zVec3* b)
{
    g_zMath_Vec3DeltaScratch.x = a->x - b->x;
    g_zMath_Vec3DeltaScratch.z = a->z - b->z;

    return Vec3LengthSqXZ(&g_zMath_Vec3DeltaScratch);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3scaleadd-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472770: zMath::Vec3ScaleAdd (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-match byte
 *
 * Purpose: Computes out = vec + scale * delta for each vector component.
 * Data: reads only caller-supplied vector/scalar inputs and writes only the
 * caller-supplied output vector.
 */
void __fastcall Vec3ScaleAdd(const zVec3* vec, const zVec3* delta, float scale, zVec3* out)
{
    // Unused snapshots retained to reproduce the retail VC5 operand order.
    float savedScale, savedX;
    out->x = (savedScale = scale) * delta->x + (savedX = vec->x);
    out->y = vec->y + delta->y * scale;
    out->z = vec->z + delta->z * scale;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-divscalar-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4727a0: zMathVec3DivScalar (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-match byte
 *
 * Purpose: Divides a vector by a scalar while preserving the input vector for zero divisors.
 * Data: reads shared zMath scalar constants 0x4d2918 and 0x4d291c; writes
 * only the caller-supplied output vector.
 */
void __fastcall zMathVec3DivScalar(const zVec3* vec, zVec3* out, float scalar)
{
    float savedInverse; // Unused afterward but proven by byte matching.
    float inverseScalar;
    if (scalar == 0.0f) {
        if (out != vec) {
            *out = *vec;
        }
        return;
    }
    inverseScalar = 1.0f / scalar;
    out->x = (savedInverse = inverseScalar) * vec->x;
    out->y = vec->y * inverseScalar;
    out->z = vec->z * inverseScalar;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3normalizexz-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4727f0: zMath::Vec3NormalizeXZ (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vec.vector-length
 * @recoil-match source
 *
 * Purpose: Normalizes a vector in the XZ plane while preserving the input Y value and leaving output Y untouched.
 */
void __fastcall Vec3NormalizeXZ(zVec3* vec, zVec3* out)
{
    const float savedY = vec->y;
    float length;
    float scale;
    vec->y = 0.0f;
    length = Vec3Length(vec);
    vec->y = savedY;

    scale = length;
    if (length != 0.0) {
        scale = 1.0f / length;
    }

    out->x = vec->x * scale;
    out->z = vec->z * scale;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3reflect-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472860: zMath::Vec3Reflect (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Purpose: Reflects an incident vector around a normal, with the zero-dot case negating the incident vector.
 * Data: reads shared zMath scalar constants 0x4d2918 and 0x4d2928; writes
 * only the caller-supplied output vector.
 */
void __fastcall Vec3Reflect(zVec3* normal, zVec3* incident, zVec3* reflected)
{
    float dot;
    zVec3 scaledNormal;
    zVec3 halfReflected;
    float negDot;
    ZMTH_VECTOR_DOT_BOUND(dot, normal, incident);
    if (dot == 0.0f) {
        reflected->x = incident->x * -1.0f;
        reflected->y = incident->y * -1.0f;
        reflected->z = incident->z * -1.0f;
        return;
    }

    scaledNormal.x = (negDot = -dot) * normal->x;
    scaledNormal.y = normal->y * negDot;
    scaledNormal.z = normal->z * negDot;
    Vec3Add(incident, &scaledNormal, &halfReflected);
    Vec3Add(&scaledNormal, &halfReflected, reflected);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3lerp-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472960: zMath::Vec3BlendByFirstWeight (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-match source
 *
 * Purpose: Blends the first vector in place with a second vector using a*t + b*(1-t).
 */
void __fastcall Vec3BlendByFirstWeight(zVec3* inOut, const zVec3* other, float t)
{
    const float otherScale = 1.0f - t;
    inOut->x = t * inOut->x + otherScale * other->x;
    inOut->y = t * inOut->y + otherScale * other->y;
    inOut->z = t * inOut->z + otherScale * other->z;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3directionto-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4729b0: zMath::Vec3DirectionTo (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-match byte
 *
 * Purpose: Writes the normalized direction from one point to another and returns the original distance.
 * Data: writes only the caller-supplied output vector before delegating
 * normalization to zMath::Vec3Normalize.
 */
float __fastcall Vec3DirectionTo(const zVec3* from, const zVec3* to, zVec3* outDir)
{
    Vec3Subtract(to, from, outDir);
    return Vec3Normalize(outDir);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3lerpnormalize-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x4729f0: zMath::Vec3BlendByFirstWeightNormalize (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-match byte
 *
 * Purpose: Blends the first vector toward a second vector and normalizes the result.
 */
void __fastcall Vec3BlendByFirstWeightNormalize(zVec3* inOut, const zVec3* other, float t)
{
    Vec3BlendByFirstWeight(inOut, other, t);
    Vec3Normalize(inOut);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3slerp-gamezrecoil-zmath-zmath-vec3-cpp
 * @recoil-artifact defines .text recoil:function:0x472a10: zMath::Vec3Slerp (GameZRecoil/zMath/zmath_vec3.cpp).
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vec3-slerp.fast-sqrt-estimate recoil:function:0x472a10
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vec3-slerp.fast-sqrt-estimate
 * @recoil-match source
 *
 * Raw assembly: the reviewed full-XYZ dot island bound directly to the a/b
 * parameters (retail loads ECX/EDX from their prologue homes), and one
 * in-body 13-byte fast-sqrt estimate island at retail [0x472c0e,0x472c1b).
 *
 *
 * Purpose: Interpolates between two unit vectors with endpoint, near-linear, antiparallel, and spherical paths.
 */
void __fastcall Vec3Slerp(const zVec3* a, const zVec3* b, float t, zVec3* out)
{
    float sinAngle;
    float cosAngle;
    float dot;
    float sinOmegaEstimate;
    zVec3 scaled;
    float aScale;
    float bScale;
    float invSinOmega;
    float omega;

    if (t == 0.0f) {
        *out = *a;
        return;
    }

    if (t == 1.0f) {
        *out = *b;
        return;
    }

    ZMTH_VECTOR_DOT_BOUND(dot, a, b);
    if (dot < -0.95) {
        Vec3Perp2D(a, &scaled);
        SinCos(3.14159274f * t, &sinAngle, &cosAngle);
        scaled.x *= sinAngle;
        scaled.y *= sinAngle;
        scaled.z *= sinAngle;
        out->x = cosAngle * a->x;
        out->y = a->y * cosAngle;
        out->z = a->z * cosAngle;
        Vec3Add(&scaled, out, out);
        return;
    }

    if (dot > 0.95) {
        out->x = (aScale = 1.0f - t) * a->x;
        out->y = a->y * aScale;
        out->z = a->z * aScale;
        scaled.x = t * b->x;
        scaled.y = b->y * t;
        scaled.z = b->z * t;
        Vec3Add(out, &scaled, out);
        return;
    }

    // sinAngle and cosAngle are reused here for sin(omega) and its square.
    cosAngle = 1.0f - dot * dot;
    if (cosAngle <= 0.0f) {
        sinAngle = 0.0f;
    } else {
        // Raw-assembly fast square-root estimate: retail transforms the named cosAngle
        // bits through EAX ((bits >> 1) + 0x1fc00000) into the named result local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
            mov eax, cosAngle
            sar eax, 1
            add eax, 01fc00000h
            mov sinOmegaEstimate, eax
        }
#else
        {
            int estimateBits;
            memcpy(&estimateBits, &cosAngle, sizeof estimateBits);
            estimateBits = (estimateBits >> 1) + 0x1fc00000;
            memcpy(&sinOmegaEstimate, &estimateBits, sizeof sinOmegaEstimate);
        }
#endif
        sinAngle = sinOmegaEstimate;
    }
    omega = atan2(sinAngle, dot);

    out->x = (aScale = sin((1.0f - t) * omega)) * a->x;
    out->y = a->y * aScale;
    out->z = a->z * aScale;
    scaled.x = (bScale = sin(omega * t)) * b->x;
    scaled.y = b->y * bScale;
    scaled.z = b->z * bScale;
    Vec3Add(out, &scaled, out);

    out->x *= (invSinOmega = 1.0f / sinAngle);
    out->y *= invSinOmega;
    out->z *= invSinOmega;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3perp2d-gamezrecoil-zmath-zmath-vec2-cpp
 * @recoil-artifact defines .text recoil:function:0x472cc0: zMath::Vec3Perp2D (GameZRecoil/zMath/zmath_vec2.cpp).
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vec3-perp2d.fast-sqrt-estimate recoil:function:0x472cc0
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vec3-perp2d.fast-sqrt-estimate
 * @recoil-match source
 *
 * Raw assembly: one in-body 13-byte fast-sqrt estimate island at retail
 * [0x472d03,0x472d10).
 *
 * Purpose: Computes a unit XY-plane perpendicular using the recovered fast square-root estimate.
 */
void __fastcall Vec3Perp2D(const zVec3* in, zVec3* out)
{
    float lengthSq;
    float lengthEstimate;
    float invLength;
    out->z = 0.0f;
    if (in->x == 0.0f) {
        out->x = 1.0f;
        out->y = 0.0f;
        return;
    }

    lengthSq = in->x * in->x + in->y * in->y;
    // Raw-assembly fast square-root estimate: retail transforms the named lengthSq
    // bits through EAX ((bits >> 1) + 0x1fc00000) into the named result local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, lengthSq
        sar eax, 1
        add eax, 01fc00000h
        mov lengthEstimate, eax
    }
#else
    {
        int estimateBits;
        memcpy(&estimateBits, &lengthSq, sizeof estimateBits);
        estimateBits = (estimateBits >> 1) + 0x1fc00000;
        memcpy(&lengthEstimate, &estimateBits, sizeof lengthEstimate);
    }
#endif
    invLength = 1.0f / lengthEstimate;
    out->x = in->y * invLength;
    out->y = -(in->x * invLength);
}
