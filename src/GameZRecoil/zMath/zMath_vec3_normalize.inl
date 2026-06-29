#pragma optimize("y", off)

/**
 * Reimplements 0x402f60: zMath::Vec3Normalize.
 * Source placement audit: authored zMath inline helper whose COMDAT is
 * physically selected from the retail ai_net.cpp contribution between
 * 0x402f10 and 0x402fd0.
 * Purpose: Normalizes a nonzero vector in place and returns the original 3D length.
 */
inline float __fastcall zMath::Vec3Normalize(
    zVec3 *vec
) {
    float length = sqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
    const unsigned int *lengthBits = (const unsigned int *)&length;
    if ((*lengthBits & 0x7fffffffu) != 0) {
        const float reciprocalLength = 1.0f / length;
        vec->x *= reciprocalLength;
        vec->y *= reciprocalLength;
        vec->z *= reciprocalLength;
    }
    return length;
}

#pragma optimize("", on)
