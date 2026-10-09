/* Quaternion operations. The filename is an inferred reconstruction name. */
#include "GameZRecoil/zMath/zmth.h"

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-fromeuler
 * @recoil-artifact defines .text recoil:function:0x4757c0: zMathQuatFromEulerYXZ
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match byte
 *
 * Purpose: converts three Euler rotation angles into a quaternion using Y-X-Z composition.
 * Data: reads no authored zMath globals; VC5 materializes literal and x87
 * range-check constants while lowering the sin/cos half-angle calls.
 */
void __fastcall zMathQuatFromEulerYXZ(zQuat* outQuat, float angle0, float angle1, float angle2)
{
    float sy;
    float cy;
    float sp;
    float cp;
    float sr;
    float cr;
    float cpcy;
    float spcy;
    float cpsy;
    float spsy;
    SinCos(angle0 * 0.5f, &sy, &cy);
    SinCos(angle1 * 0.5f, &sp, &cp);

    cpcy = cp * cy;
    spcy = sp * cy;
    cpsy = cp * sy;
    spsy = sp * sy;

    SinCos(angle2 * 0.5f, &sr, &cr);
    outQuat->w = spsy * sr + cpcy * cr;
    outQuat->x = cpsy * sr + spcy * cr;
    outQuat->y = cpsy * cr - spcy * sr;
    outQuat->z = cpcy * sr - spsy * cr;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiply
 * @recoil-artifact defines .text recoil:function:0x475910: zMathQuatMultiply
 * @recoil-match source
 *
 * Purpose: computes the quaternion product used by zMath rotation composition.
 */
void __fastcall zMathQuatMultiply(const zQuat* quatA, const zQuat* quatB, zQuat* outAB)
{
    float scaledX;
    float scaledY;
    float scaledZ;
    outAB->w = quatB->w * quatA->w - quatA->x * quatB->x - quatA->y * quatB->y - quatA->z * quatB->z;
    scaledX = quatB->w * quatA->x;
    outAB->x = scaledX + quatA->w * quatB->x + quatB->z * quatA->y - quatA->z * quatB->y;
    scaledY = quatB->w * quatA->y;
    outAB->y = scaledY + quatA->w * quatB->y + quatA->z * quatB->x - quatB->z * quatA->x;
    scaledZ = quatB->w * quatA->z;
    outAB->z = scaledZ + quatA->w * quatB->z + quatB->y * quatA->x - quatA->y * quatB->x;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiplyinverse
 * @recoil-artifact defines .text recoil:function:0x4759d0: zMathQuatMultiplyConjugate
 * @recoil-match source
 *
 * Purpose: multiplies a quaternion by the conjugate of a second quaternion (its inverse only for unit quaternions).
 */
void __fastcall zMathQuatMultiplyConjugate(const zQuat* quatA, const zQuat* quatB, zQuat* outAConjB)
{
    outAConjB->w = quatB->w * quatA->w + quatB->x * quatA->x + quatA->y * quatB->y + quatB->z * quatA->z;
    outAConjB->x = quatB->w * quatA->x - quatA->w * quatB->x - quatB->z * quatA->y + quatA->z * quatB->y;
    outAConjB->y = quatB->w * quatA->y - quatA->w * quatB->y - quatA->z * quatB->x + quatB->z * quatA->x;
    outAConjB->z = quatB->w * quatA->z - quatA->w * quatB->z - quatB->y * quatA->x + quatA->y * quatB->x;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-tomatrix
 * @recoil-artifact defines .text recoil:function:0x475a80: zMathQuatToMatrix
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Raw assembly: the inline zMath::Vec3Add expansion [0x475a9c,0x475abf)
 * (requires zmth_quat.c /Ob1).
 *
 * Purpose: expands a quaternion into the rotational part of a 4x3 matrix.
 */
void __fastcall zMathQuatToMatrix(const zQuat* quat, zMat4x3* outMatrix3x3)
{
    zVec3 twice;
    float xx2;
    float yy2;
    float zz2;
    float xy2;
    float yz2;
    float xz2;
    float xw2;
    float yw2;
    float zw2;
    float* out;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    Vec3Add((const zVec3*)&quat->x, (const zVec3*)&quat->x, &twice);
#else
    twice.x = quat->x + quat->x;
    twice.y = quat->y + quat->y;
    twice.z = quat->z + quat->z;
#endif

    xx2 = twice.x * quat->x;
    yy2 = twice.y * quat->y;
    zz2 = twice.z * quat->z;
    xy2 = twice.y * quat->x;
    yz2 = twice.z * quat->y;
    xz2 = twice.x * quat->z;
    xw2 = twice.x * quat->w;
    yw2 = twice.y * quat->w;
    zw2 = twice.z * quat->w;

    out = &outMatrix3x3->xx;
    *out++ = 1.0f - yy2 - zz2;
    *out++ = zw2 + xy2;
    *out++ = xz2 - yw2;
    *out++ = xy2 - zw2;
    *out++ = 1.0f - zz2 - xx2;
    *out++ = xw2 + yz2;
    *out++ = yw2 + xz2;
    *out++ = yz2 - xw2;
    *out = 1.0f - xx2 - yy2;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.quat.vector-length
 *
 * Purpose: return FSQRT of the grouped (x*x + y*y) + z*z sum as binary32.
 * Reconstruction: zmth_quat.c-resident copy of the Camera.c inline helper,
 * following the zwep_ammo.c-resident precedent; this /Ob1 TU inlines it at
 * 0x475b80 with the parameter argument bound to its inline parameter home.
 * Raw assembly: identical body to the reviewed Camera.c Vec3Length island.
 * Retail inline-expansion evidence: the listed consumer contains the operand reloads, arithmetic
 * sequence and result store without a call at that site; the original inline helper's header
 * ownership and declaration placement are not established (TU-resident reconstruction model).
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x475b80.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-fromrotationvector
 * @recoil-artifact defines .text recoil:function:0x475b80: zMathQuatExp
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.quat.vector-length
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match byte
 *
 * Raw assembly: inline zMath::Vec3Length [0x475b92,0x475bb0)
 * and the inline zMath::SinCos fsincos arm [0x475c01,0x475c10)
 * (requires zmth_quat.c /Ob1).
 *
 * Purpose: computes the quaternion exponential of a half-angle rotation vector, returning identity for zero.
 */
void __fastcall zMathQuatExp(const zVec3* rotationVector, zQuat* outQuat)
{
    float sinLength;
    const float length = Vec3Length(rotationVector);
    float scale;

    if (length == 0.0f) {
        outQuat->w = 1.0f;
        outQuat->x = 0.0f;
        outQuat->y = 0.0f;
        outQuat->z = 0.0f;
        return;
    }

    SinCos(length, &sinLength, &outQuat->w);
    scale = sinLength / length;
    outQuat->x = scale * rotationVector->x;
    outQuat->y = scale * rotationVector->y;
    outQuat->z = scale * rotationVector->z;
}
