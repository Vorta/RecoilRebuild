#pragma once

#include "GameZRecoil/zMath/zmth_decls.h"

namespace zMath {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
#pragma optimize("", off)
#pragma warning(disable: 4035)
#endif
/**
 * Reimplements 0x402f60: zMath::Vec3Normalize.
 * Source placement audit: authored zmth.h header inline helper whose COMDAT
 * is physically observed in the retail ai_net.cpp contribution between
 * 0x402f10 and 0x402fd0.
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
        fld     dword ptr [ecx]
        fmul    st, st
        fld     dword ptr [ecx+4]
        fld     dword ptr [ecx+8]
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
        fmul    dword ptr [ecx]
        fld     st(1)
        fmul    dword ptr [ecx+4]
        fxch    st(2)
        fmul    dword ptr [ecx+8]
        fxch    st(1)
        fstp    dword ptr [ecx]
        fxch    st(1)
        fstp    dword ptr [ecx+4]
        fstp    dword ptr [ecx+8]
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
