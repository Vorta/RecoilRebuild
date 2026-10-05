/* Quaternion operations. The filename is an inferred reconstruction name. */
#include "GameZRecoil/zMath/zmth.h"

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-fromeuler
 * @recoil-artifact defines .text recoil:function:0x4757c0: zMathQuatFromEuler
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 *
 *
 * Purpose: converts three Euler rotation angles into a quaternion.
 * Data: reads no authored zMath globals; VC5 materializes literal and x87
 * range-check constants while lowering the sin/cos half-angle calls.
 */
void __fastcall zMathQuatFromEuler(zQuat* outQuat, float angle0, float angle1, float angle2)
{
    float sy;
    float cy;
    float sp;
    float cp;
    float sr;
    float cr;
    zMath::SinCos(angle0 * 0.5f, &sy, &cy);
    zMath::SinCos(angle1 * 0.5f, &sp, &cp);

    const float cpcy = cp * cy;
    const float spcy = sp * cy;
    const float cpsy = cp * sy;
    const float spsy = sp * sy;

    zMath::SinCos(angle2 * 0.5f, &sr, &cr);
    outQuat->w = spsy * sr + cpcy * cr;
    outQuat->x = cpsy * sr + spcy * cr;
    outQuat->y = cpsy * cr - spcy * sr;
    outQuat->z = cpcy * sr - spsy * cr;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiply
 * @recoil-artifact defines .text recoil:function:0x475910: zMathQuatMultiply
 * @recoil-match byte
 *
 * Purpose: computes the quaternion product used by zMath rotation composition.
 */
void __fastcall zMathQuatMultiply(const zQuat* quatA, const zQuat* quatB, zQuat* outAB)
{
    outAB->w = quatB->w * quatA->w - quatA->x * quatB->x - quatA->y * quatB->y - quatA->z * quatB->z;
    const float scaledX = quatB->w * quatA->x;
    outAB->x = scaledX + quatA->w * quatB->x + quatB->z * quatA->y - quatA->z * quatB->y;
    const float scaledY = quatB->w * quatA->y;
    outAB->y = scaledY + quatA->w * quatB->y + quatA->z * quatB->x - quatB->z * quatA->x;
    const float scaledZ = quatB->w * quatA->z;
    outAB->z = scaledZ + quatA->w * quatB->z + quatB->y * quatA->x - quatA->y * quatB->x;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-multiplyinverse
 * @recoil-artifact defines .text recoil:function:0x4759d0: zMathQuatMultiplyInverse
 * @recoil-match byte
 *
 * Purpose: multiplies a quaternion by the inverse/conjugate form used by camera-view composition.
 */
void __fastcall zMathQuatMultiplyInverse(const zQuat* quatA, const zQuat* quatB, zQuat* outAConjB)
{
    outAConjB->w = quatB->w * quatA->w + quatB->x * quatA->x + quatA->y * quatB->y + quatB->z * quatA->z;
    outAConjB->x = quatB->w * quatA->x - quatA->w * quatB->x - quatB->z * quatA->y + quatA->z * quatB->y;
    outAConjB->y = quatB->w * quatA->y - quatA->w * quatB->y - quatA->z * quatB->x + quatB->z * quatA->x;
    outAConjB->z = quatB->w * quatA->z - quatA->w * quatB->z - quatB->y * quatA->x + quatA->y * quatB->x;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-quat-tomatrix
 * @recoil-artifact defines .text recoil:function:0x475a80: zMathQuatToMatrix
 *
 *
 * Purpose: expands a quaternion into the rotational part of a 4x3 matrix.
 */
void __fastcall zMathQuatToMatrix(const zQuat* quat, zMat4x3* outMatrix3x3)
{
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
 * @recoil-artifact defines .text recoil:function:0x475b80: zMathQuatFromRotationVector
 *
 *
 * Purpose: converts a rotation vector into a quaternion, returning identity for a zero vector.
 */
void __fastcall zMathQuatFromRotationVector(const zVec3* rotationVector, zQuat* outQuat)
{
    const float length = sqrt(
        rotationVector->x * rotationVector->x + rotationVector->y * rotationVector->y
        + rotationVector->z * rotationVector->z
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
