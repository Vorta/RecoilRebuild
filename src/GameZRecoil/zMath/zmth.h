#pragma once

#include "GameZRecoil/zMath/zmth_decls.h"

namespace zMath {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
#pragma optimize("", off)
#pragma warning(disable: 4035)
#endif
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-zmath-vec3normalize
 * @recoil-artifact defines .text recoil:function:0x402f60: zMath::Vec3Normalize.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vec3-normalize
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vec3-normalize
 * @recoil-match byte
 *
 * Source placement audit: authored zmth.h header inline helper whose COMDAT
 * is physically observed in the retail ai_net.cpp contribution.
 * Raw assembly: keeps the VC5 x87 normalization byte shape after source-level
 * C/C++ variants could not preserve the retail FPU stack ordering in the
 * ai_net.cpp contribution block.
 * Purpose: Normalizes a nonzero vector in place and returns the original 3D length.
 */
inline float __fastcall Vec3Normalize(
    zVec3 *vec
) {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    float vecLength;

    __asm {
        mov     ecx, vec
        fld     dword ptr [ecx]zVec3.x
        fmul    st, st
        fld     dword ptr [ecx]zVec3.y
        fld     dword ptr [ecx]zVec3.z
        fmul    st, st
        fxch    st(1)
        fmul    st, st
        fxch    st(1)
        faddp   st(2), st
        faddp   st(1), st
        fsqrt
        fst     dword ptr vecLength
        test    vecLength, 07fffffffh
        je      vec3_normalize_zero_length
        fld1
        fdivrp  st(1), st
        fld     st(0)
        fmul    dword ptr [ecx]zVec3.x
        fld     st(1)
        fmul    dword ptr [ecx]zVec3.y
        fxch    st(2)
        fmul    dword ptr [ecx]zVec3.z
        fxch    st(1)
        fstp    dword ptr [ecx]zVec3.x
        fxch    st(1)
        fstp    dword ptr [ecx]zVec3.y
        fstp    dword ptr [ecx]zVec3.z
    }
    return vecLength;

vec3_normalize_zero_length:
    __asm {
        fstp    st(0)
    }
    return vecLength;
#else
    zVec3 *const localVec = vec;
    float length = sqrt(
        localVec->x * localVec->x +
        localVec->y * localVec->y +
        localVec->z * localVec->z
    );
    const unsigned int *lengthBits = (const unsigned int *)&length;
    if ((*lengthBits & 0x7fffffffu) != 0) {
        const float reciprocalLength = 1.0f / length;
        localVec->x *= reciprocalLength;
        localVec->y *= reciprocalLength;
        localVec->z *= reciprocalLength;
    }
    return length;
#endif
}
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
#pragma optimize("", on)
#endif
} // namespace zMath

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vector-direction
 *
 * Raw assembly: reviewed after VC5 C++ direction probes failed.
 * The normal wrapper captures pointers once; BOUND uses existing pointer locals.
 * Historical macro syntax, identifier and header ownership remain unresolved.
 * Purpose: Normalize (to - from) with x87-resident intermediates and
 * (z*z + y*y) + x*x summation, then store z/x/y as binary32. There is
 * no zero-length check. Arithmetic precision and rounding follow the
 * ambient x87 control word; no intermediate m32/m64 stores are added.
 */
#define ZMTH_VECTOR_DIRECTION_BODY(destVar, fromVar, toVar) \
        __asm { \
            __asm mov eax, toVar \
            __asm mov ebx, fromVar \
            __asm mov edx, destVar \
            __asm fld dword ptr [eax]zVec3.x \
            __asm fsub dword ptr [ebx]zVec3.x \
            __asm fld dword ptr [eax]zVec3.y \
            __asm fsub dword ptr [ebx]zVec3.y \
            __asm fld dword ptr [eax]zVec3.z \
            __asm fsub dword ptr [ebx]zVec3.z \
            __asm fld st(0) \
            __asm fmul st, st(1) \
            __asm fxch st(2) \
            __asm fld st(0) \
            __asm fmul st, st(1) \
            __asm faddp st(3), st \
            __asm fxch st(3) \
            __asm fld st(0) \
            __asm fmul st, st(1) \
            __asm faddp st(3), st \
            __asm fxch st(2) \
            __asm fsqrt \
            __asm fld1 \
            __asm fdivrp st(1), st \
            __asm fmul st(1), st \
            __asm fxch st(1) \
            __asm fstp dword ptr [edx]zVec3.z \
            __asm fmul st(1), st \
            __asm fxch st(1) \
            __asm fstp dword ptr [edx]zVec3.x \
            __asm fmulp st(1), st \
            __asm fstp dword ptr [edx]zVec3.y \
        }
#define ZMTH_VECTOR_DIRECTION(destination, from, to) \
    do { \
        zVec3 *const directionDest = (destination); \
        const zVec3 *const directionFrom = (from); \
        const zVec3 *const directionTo = (to); \
        ZMTH_VECTOR_DIRECTION_BODY(directionDest, directionFrom, directionTo); \
    } while (0)
// Only named pointer objects may be passed to the direct binding wrapper.
#define ZMTH_VECTOR_DIRECTION_BOUND(destVar, fromVar, toVar) \
    do { ZMTH_VECTOR_DIRECTION_BODY(destVar, fromVar, toVar); } while (0)
#else
#define ZMTH_VECTOR_DIRECTION(destination, from, to) \
    do { \
        zVec3 *const directionDest = (destination); \
        const zVec3 *const directionFrom = (from); \
        const zVec3 *const directionTo = (to); \
        const double dx = (double)directionTo->x - directionFrom->x; \
        const double dy = (double)directionTo->y - directionFrom->y; \
        const double dz = (double)directionTo->z - directionFrom->z; \
        const double scale = 1.0 / sqrt((dz * dz + dy * dy) + dx * dx); \
        directionDest->z = (float)(dz * scale); \
        directionDest->x = (float)(dx * scale); \
        directionDest->y = (float)(dy * scale); \
    } while (0)
#define ZMTH_VECTOR_DIRECTION_BOUND(destVar, fromVar, toVar) \
    ZMTH_VECTOR_DIRECTION(destVar, fromVar, toVar)
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vector-length-xz
 *
 * Raw assembly: Reviewed at [0x405219,0x40522d) and [0x405a73,0x405a87).
 * Each consumer has an independent review; C++ lost the float boundary.
 * Purpose: Calculate a vector's horizontal length, rounded to a float.
 * The spelling and historical header ownership remain inferred.
 */
#define ZMTH_VECTOR_LENGTH_XZ(result, vector) \
    do { \
        const zVec3 *const lengthVector = (vector); \
        __asm mov ecx, lengthVector \
        __asm fld dword ptr [ecx]zVec3.x \
        __asm fmul dword ptr [ecx]zVec3.x \
        __asm fld dword ptr [ecx]zVec3.z \
        __asm fmul dword ptr [ecx]zVec3.z \
        __asm faddp st(1), st \
        __asm fsqrt \
        __asm fstp result \
    } while (0)

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vector-dot-xz
 *
 * Raw assembly: Reviewed for retail 0x405040 at [0x40532d,0x405342).
 * Other consumers and historical assembly provenance remain unproved here.
 * Purpose: Calculate the horizontal dot product, rounded to a float.
 * The spelling and historical header ownership remain inferred.
 */
#define ZMTH_VECTOR_DOT_XZ(result, left, right) \
    do { \
        const zVec3 *const dotRight = (right); \
        const zVec3 *const dotLeft = (left); \
        __asm mov ecx, dotLeft \
        __asm mov edx, dotRight \
        __asm fld dword ptr [ecx]zVec3.x \
        __asm fmul dword ptr [edx]zVec3.x \
        __asm fld dword ptr [ecx]zVec3.z \
        __asm fmul dword ptr [edx]zVec3.z \
        __asm faddp st(1), st \
        __asm fstp result \
    } while (0)
#else
#define ZMTH_VECTOR_LENGTH_XZ(result, vector) \
    do { \
        const zVec3 *const lengthVector = (vector); \
        (result) = (float)sqrt(lengthVector->x * lengthVector->x + lengthVector->z * lengthVector->z); \
    } while (0)
#define ZMTH_VECTOR_DOT_XZ(result, left, right) \
    do { \
        const zVec3 *const dotRight = (right); \
        const zVec3 *const dotLeft = (left); \
        (result) = dotLeft->x * dotRight->x + dotLeft->z * dotRight->z; \
    } while (0)
#endif

namespace zMath {
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 *
 * Inferred original inline helper: four expansions in 0x405040 and one in 0x4059a0.
 * Raw assembly: Five separately scoped, reviewed MOV/ADD/MOV bridges only.
 * Purpose: Form the approximate exponential's float representation from its
 * scaled integer exponent. C++ retains both arithmetic and float conversion.
 * The spelling and historical header ownership remain inferred.
 */
inline float FastExp(float value) {
    int fastExpBits = (int)(value * 12102200.0f);
    float result;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, fastExpBits
        add eax, 03f800000h
        mov result, eax
    }
#else
    *(int *)&result = fastExpBits + 0x3f800000;
#endif
    return result;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.sin-cos
 *
 * Inferred original inline helper: a repeated retail FSINCOS normal arm.
 * Raw assembly: Reviewed consumers 0x405040 and 0x474010; exact ranges are in the allowlist.
 * Purpose: Use FSINCOS for ordered |angle| <= 9.22e18 and unordered values.
 * Separate C++ sine/cosine expressions handle larger magnitudes.
 * Outputs must be distinct; the raw arm stores cosine, then sine as floats.
 * The spelling, inline syntax and historical header ownership are inferred.
 */
inline void SinCos(double angle, float *sinOut, float *cosOut) {
    if (fabs(angle) > 9.22e18) {
        *sinOut = (float)sin(angle);
        *cosOut = (float)cos(angle);
    } else {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
            mov ebx, sinOut
            mov edx, cosOut
            fld angle
            fsincos
            fstp dword ptr [edx]
            fstp dword ptr [ebx]
        }
#else
        *sinOut = (float)sin(angle);
        *cosOut = (float)cos(angle);
#endif
    }
}
} // namespace zMath

namespace zMath {
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vector-add
 *
 * Purpose: Add all three components before storing x/y/z as binary32.
 * Inferred original inline helper at retail 0x405674, 0x405b9c and 0x405f76; spelling/header unknown.
 * Raw assembly: Pro-reviewed per consumer after VC5 C/C++ failed retail ordering.
 * C++ evaluates simple pointer arguments once and owns all homes and saves.
 * EBX/ECX/EDX hold left/right/destination; x87 depths are 0/3/0.
 * Integer flags and x87 control word are unchanged; x87 status follows retail.
 */
inline void Vec3Add(const zVec3 *left, const zVec3 *right, zVec3 *dest) {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov ebx, left
        mov ecx, right
        mov edx, dest
        fld dword ptr [ebx]zVec3.x
        fadd dword ptr [ecx]zVec3.x
        fld dword ptr [ebx]zVec3.y
        fadd dword ptr [ecx]zVec3.y
        fld dword ptr [ebx]zVec3.z
        fadd dword ptr [ecx]zVec3.z
        fxch st(2)
        fstp dword ptr [edx]zVec3.x
        fstp dword ptr [edx]zVec3.y
        fstp dword ptr [edx]zVec3.z
    }
#else
    const float x = left->x + right->x;
    const float y = left->y + right->y;
    const float z = left->z + right->z;
    dest->x = x;
    dest->y = y;
    dest->z = z;
#endif
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 *
 * Purpose: Subtract all three components before storing x/y/z as binary32.
 * Inferred original inline helper at retail 0x405fa5; spelling/header unknown.
 * Raw assembly: Pro-reviewed per consumer after VC5 C/C++ failed retail ordering.
 * C++ evaluates simple pointer arguments once and owns all homes and saves.
 * EBX/ECX/EDX hold left/right/destination; x87 depths are 0/3/0.
 * Integer flags and x87 control word are unchanged; x87 status follows retail.
 */
inline void Vec3Subtract(const zVec3 *left, const zVec3 *right, zVec3 *dest) {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov ebx, left
        mov ecx, right
        mov edx, dest
        fld dword ptr [ebx]zVec3.x
        fsub dword ptr [ecx]zVec3.x
        fld dword ptr [ebx]zVec3.y
        fsub dword ptr [ecx]zVec3.y
        fld dword ptr [ebx]zVec3.z
        fsub dword ptr [ecx]zVec3.z
        fxch st(2)
        fstp dword ptr [edx]zVec3.x
        fstp dword ptr [edx]zVec3.y
        fstp dword ptr [edx]zVec3.z
    }
#else
    const float x = left->x - right->x;
    const float y = left->y - right->y;
    const float z = left->z - right->z;
    dest->x = x;
    dest->y = y;
    dest->z = z;
#endif
}
} // namespace zMath

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vector-transform-direction
 *
 * Inferred original inline helper: grouped six-deep retail x87 transform.
 * Raw assembly: reviewed only for 0x4059a0 at [0x405b37,0x405b8a).
 * Purpose: Transform a direction by the matrix's 3x3 part, without translation.
 * Capture simple, side-effect-free matrix/destination/vector pointers once.
 * All input reads precede the z/y/x result stores; x87 entry and exit are empty.
 * The spelling, capture order and original header ownership remain inferred.
 */
#define ZMTH_VECTOR_TRANSFORM_DIRECTION(matrix, destination, vector) \
    do { \
        const zMat4x3 *const transformMatrix = (matrix); \
        zVec3 *const transformDest = (destination); \
        const zVec3 *const transformSource = (vector); \
        __asm mov eax, transformSource \
        __asm mov ebx, transformMatrix \
        __asm mov edx, transformDest \
        __asm fld dword ptr [eax]zVec3.x \
        __asm fmul dword ptr [ebx]zMat4x3.xx \
        __asm fld dword ptr [eax]zVec3.x \
        __asm fmul dword ptr [ebx]zMat4x3.xy \
        __asm fld dword ptr [eax]zVec3.x \
        __asm fmul dword ptr [ebx]zMat4x3.xz \
        __asm fld dword ptr [eax]zVec3.y \
        __asm fmul dword ptr [ebx]zMat4x3.yx \
        __asm fld dword ptr [eax]zVec3.y \
        __asm fmul dword ptr [ebx]zMat4x3.yy \
        __asm fld dword ptr [eax]zVec3.y \
        __asm fmul dword ptr [ebx]zMat4x3.yz \
        __asm fxch st(2) \
        __asm faddp st(5), st \
        __asm faddp st(3), st \
        __asm faddp st(1), st \
        __asm fld dword ptr [eax]zVec3.z \
        __asm fmul dword ptr [ebx]zMat4x3.zx \
        __asm fld dword ptr [eax]zVec3.z \
        __asm fmul dword ptr [ebx]zMat4x3.zy \
        __asm fld dword ptr [eax]zVec3.z \
        __asm fmul dword ptr [ebx]zMat4x3.zz \
        __asm fxch st(2) \
        __asm faddp st(5), st \
        __asm faddp st(3), st \
        __asm faddp st(1), st \
        __asm fstp dword ptr [edx]zVec3.z \
        __asm fstp dword ptr [edx]zVec3.y \
        __asm fstp dword ptr [edx]zVec3.x \
    } while (0)
#else
// Mathematical fallback; exact x87 rounding and exception order are not implied.
#define ZMTH_VECTOR_TRANSFORM_DIRECTION(matrix, destination, vector) \
    do { \
        const zMat4x3 *const transformMatrix = (matrix); \
        zVec3 *const transformDest = (destination); \
        const zVec3 *const transformSource = (vector); \
        const float transformX = \
            (transformSource->x * transformMatrix->xx + \
             transformSource->y * transformMatrix->yx) + \
            transformSource->z * transformMatrix->zx; \
        const float transformY = \
            (transformSource->x * transformMatrix->xy + \
             transformSource->y * transformMatrix->yy) + \
            transformSource->z * transformMatrix->zy; \
        const float transformZ = \
            (transformSource->x * transformMatrix->xz + \
             transformSource->y * transformMatrix->yz) + \
            transformSource->z * transformMatrix->zz; \
        transformDest->z = transformZ; \
        transformDest->y = transformY; \
        transformDest->x = transformX; \
    } while (0)
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.vector-cross
 *
 * Purpose: Compute a cross product through typed captures before y/z/x stores.
 * Raw assembly: VC5 C++ controls failed; Pro reviewed [0x42b994,0x42b9d5).
 * C++ owns captures, homes and saves; original macro spelling is inferred.
 * Empty incoming x87 stack reaches depth six and returns empty on completion.
 * Integer flags and control word are unchanged; FP status follows retail.
 * All input reads precede stores; arguments must be side-effect-free pointers.
 */
#define ZMTH_VECTOR_CROSS(left, right, destination) \
    do { \
        zVec3 *const crossDest = (destination); \
        const zVec3 *const crossRight = (right); \
        const zVec3 *const crossLeft = (left); \
        __asm { \
            __asm mov ebx, crossLeft \
            __asm mov ecx, crossRight \
            __asm mov edx, crossDest \
            __asm fld dword ptr [ebx]zVec3.x \
            __asm fld st(0) \
            __asm fmul dword ptr [ecx]zVec3.y \
            __asm fld dword ptr [ebx]zVec3.y \
            __asm fld st(0) \
            __asm fmul dword ptr [ecx]zVec3.z \
            __asm fld dword ptr [ebx]zVec3.z \
            __asm fld st(0) \
            __asm fmul dword ptr [ecx]zVec3.x \
            __asm fxch st(5) \
            __asm fmul dword ptr [ecx]zVec3.z \
            __asm fxch st(3) \
            __asm fmul dword ptr [ecx]zVec3.x \
            __asm fxch st(3) \
            __asm fsubp st(5), st \
            __asm fmul dword ptr [ecx]zVec3.y \
            __asm fxch st(2) \
            __asm fsubp st(3), st \
            __asm fxch st(1) \
            __asm fsubp st(1), st \
            __asm fxch st(2) \
            __asm fstp dword ptr [edx]zVec3.y \
            __asm fstp dword ptr [edx]zVec3.z \
            __asm fstp dword ptr [edx]zVec3.x \
        } \
    } while (0)
#else
#define ZMTH_VECTOR_CROSS(left, right, destination) \
    do { \
        zVec3 *const crossDest = (destination); \
        const zVec3 *const crossRight = (right); \
        const zVec3 *const crossLeft = (left); \
        const float crossX = crossLeft->y * crossRight->z - crossLeft->z * crossRight->y; \
        const float crossY = crossLeft->z * crossRight->x - crossLeft->x * crossRight->z; \
        const float crossZ = crossLeft->x * crossRight->y - crossLeft->y * crossRight->x; \
        crossDest->y = crossY; \
        crossDest->z = crossZ; \
        crossDest->x = crossX; \
    } while (0)
#endif
