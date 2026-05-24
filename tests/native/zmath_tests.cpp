#include "GameZRecoil/zMath/zMath.h"
#include "zClass.h"

#include <cmath>

namespace {
bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.00001f;
}
} // namespace

extern "C" int zmath_matrix_stack_and_direction_smoke(void) {
    std::int32_t flags[3] = {11, 22, 33};
    float *slots[3] = {};
    zMat4x3 matrix{};
    zMath::g_currentMatrixIdentityFlagSlot = &flags[0];
    zMath::g_currentMatrixPtrSlot = &slots[0];

    zMath::MatStackPushPtr(reinterpret_cast<float *>(&matrix));
    if (zMath::g_currentMatrixIdentityFlagSlot != &flags[1] ||
        zMath::g_currentMatrixPtrSlot != &slots[1] ||
        slots[1] != reinterpret_cast<float *>(&matrix) || flags[1] != 0) {
        return 1;
    }

    zMath::MatLoadIdentity();
    if (flags[1] != 1 || matrix.xx != 1.0f || matrix.yy != 1.0f || matrix.zz != 1.0f ||
        matrix.xy != 0.0f || matrix.posZ != 0.0f) {
        return 2;
    }
    if (zMath_Mat_GetCurrent() != &matrix || zMath_Mat_IsCurrentIdentity() != 1) {
        return 12;
    }

    zMat4x3 source{2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 4.0f, 10.0f, 20.0f, 30.0f};
    zMath::MatLoadCurrentFrom(&source);
    zMat4x3 copy{};
    if (zMath::MatCopyCurrentTo(&copy) != &copy || flags[1] != 0 || copy.yy != 3.0f ||
        copy.posZ != 30.0f) {
        return 3;
    }

    zMat4x3 local{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 2.0f, 3.0f};
    zMath::MatMultiply(&local, 1);
    if (!Near(matrix.xx, 2.0f) || !Near(matrix.yy, 3.0f) || !Near(matrix.zz, 4.0f) ||
        !Near(matrix.posX, 12.0f) || !Near(matrix.posY, 26.0f) || !Near(matrix.posZ, 42.0f)) {
        return 4;
    }

    zVec3 angles{0.0f, 0.0f, 0.0f};
    zVec3 position{3.0f, 4.0f, 5.0f};
    zVec3 scale{2.0f, 3.0f, 4.0f};
    flags[1] = 1;
    zMath::MatApplyLocalTRS(&angles, &position, &scale);
    if (flags[1] != 0 || !Near(matrix.xx, 2.0f) || !Near(matrix.yy, 3.0f) ||
        !Near(matrix.zz, 4.0f) || !Near(matrix.posX, 3.0f) || !Near(matrix.posY, 4.0f) ||
        !Near(matrix.posZ, 5.0f)) {
        return 5;
    }

    zMath::MatStackPopPtr();
    if (zMath::g_currentMatrixIdentityFlagSlot != &flags[0] ||
        zMath::g_currentMatrixPtrSlot != &slots[0]) {
        return 6;
    }

    zMat4x3 cloneParent{3.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    zMat4x3 cloneSlot{};
    flags[0] = 88;
    slots[0] = reinterpret_cast<float *>(&cloneParent);
    zMath::g_currentMatrixIdentityFlagSlot = &flags[0];
    zMath::g_currentMatrixPtrSlot = &slots[0];
    zMath::MatStackPushAndCloneParent(reinterpret_cast<float *>(&cloneSlot));
    if (zMath::g_currentMatrixIdentityFlagSlot != &flags[1] ||
        zMath::g_currentMatrixPtrSlot != &slots[1] || flags[1] != 88 ||
        slots[1] != reinterpret_cast<float *>(&cloneSlot) || cloneSlot.xx != 3.0f ||
        cloneSlot.yy != 4.0f || cloneSlot.posZ != 8.0f) {
        return 11;
    }
    zMath::MatStackPopPtr();

    zMat4x3 bboxMatrix{2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 4.0f, 10.0f, 20.0f, 30.0f};
    zBBox3f bbox{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    zBBoxCorners corners{};
    zMath_Mat_TransformBBoxToCorners(&bboxMatrix, &bbox, &corners);
    if (!Near(corners.values[0], 12.0f) || !Near(corners.values[1], 26.0f) ||
        !Near(corners.values[2], 54.0f) || !Near(corners.values[6], 18.0f) ||
        !Near(corners.values[7], 26.0f) || !Near(corners.values[8], 42.0f) ||
        !Near(corners.values[21], 12.0f) || !Near(corners.values[22], 35.0f) ||
        !Near(corners.values[23], 42.0f)) {
        return 8;
    }

    zVec3 pointA{0.0f, 0.0f, 0.0f};
    zVec3 pointB{0.0f, 1.0f, -1.0f};
    zVec3 directionAngles{};
    zVec3 *returned = zMath::Vec3DirectionAnglesBetweenPoints(&pointA, &pointB, &directionAngles);
    if (returned != &directionAngles || !Near(directionAngles.x, 0.785398185f) ||
        directionAngles.y != 0.0f || directionAngles.z != 0.0f) {
        return 7;
    }

    zMath::g_zMath_CameraScratchA = {};
    zMath::g_zMath_CameraScratchA.xy = 2.0f;
    zMath::g_zMath_CameraScratchA.yy = 3.0f;
    zMath::g_zMath_CameraScratchA.zy = 4.0f;
    zMath::g_zMath_CameraScratchA.posY = 5.0f;
    zVec3 points[2] = {{1.0f, 2.0f, 3.0f}, {-1.0f, 0.5f, 2.0f}};
    float projectedY[2] = {-7.0f, -8.0f};
    zMath::Vec3ArrayProjectToCachedY(points, projectedY, 2);
    if (!Near(projectedY[0], 25.0f) || !Near(projectedY[1], 12.5f)) {
        return 9;
    }

    zMath::Vec3ArrayProjectToCachedY(points, projectedY, 0);
    return Near(projectedY[0], 25.0f) ? 0 : 10;
}

extern "C" int zmath_mat_build_euler_rotation3x3_smoke(void) {
    zMat4x3 matrix{};
    zMath::MatBuildEulerRotation3x3(&matrix, 0.0f, 0.0f, 0.0f);
    if (!Near(matrix.xx, 1.0f) || !Near(matrix.yy, 1.0f) || !Near(matrix.zz, 1.0f) ||
        !Near(matrix.posX, 0.0f) || !Near(matrix.posY, 0.0f) || !Near(matrix.posZ, 0.0f)) {
        return 1;
    }

    zMath::MatBuildEulerRotation3x3(&matrix, 0.5f, -0.25f, 0.75f);
    const float sx = sinf(0.5f);
    const float cx = cosf(0.5f);
    const float sy = sinf(-0.25f);
    const float cy = cosf(-0.25f);
    const float sz = sinf(0.75f);
    const float cz = cosf(0.75f);

    return Near(matrix.xx, sy * sx * sz + cz * cy) && Near(matrix.xy, sz * cx) &&
                   Near(matrix.xz, sz * cy * sx - cz * sy) &&
                   Near(matrix.yx, sy * sx * cz - sz * cy) && Near(matrix.yy, cz * cx) &&
                   Near(matrix.yz, cz * cy * sx + sz * sy) && Near(matrix.zx, sy * cx) &&
                   Near(matrix.zy, -sx) && Near(matrix.zz, cy * cx) &&
                   Near(matrix.posX, 0.0f) && Near(matrix.posY, 0.0f) &&
                   Near(matrix.posZ, 0.0f)
               ? 0
               : 2;
}

extern "C" int zmath_projection_batches_smoke(void) {
    g_zMath_ProjScaleX = 100.0f;
    g_zMath_ProjScaleY = -50.0f;
    g_zMath_ProjOffsetX = 320.0f;
    g_zMath_ProjOffsetY = 240.0f;
    g_zMath_InvProjScaleX = 0.01f;
    g_zMath_InvProjScaleY = -0.02f;
    g_zMath_ProjSphereRadiusScale = 25.0f;

    zVec3 viewPoints[2] = {{2.0f, 4.0f, 10.0f}, {-3.0f, 6.0f, 5.0f}};
    zProjectedPoint projectedPoints[2] = {};
    zMath::ProjectPointBatch(viewPoints, projectedPoints, 2);
    if (!Near(projectedPoints[0].reciprocalZ, 0.1f) || !Near(projectedPoints[0].x, 340.0f) ||
        !Near(projectedPoints[0].y, 220.0f) || !Near(projectedPoints[1].reciprocalZ, 0.2f) ||
        !Near(projectedPoints[1].x, 260.0f) || !Near(projectedPoints[1].y, 180.0f)) {
        return 1;
    }

    zProjectedSphere projectedSpheres[2] = {};
    zMath_ProjectSphereBatch(viewPoints, projectedSpheres, 2);
    if (!Near(projectedSpheres[0].x, 340.0f) || !Near(projectedSpheres[0].y, 220.0f) ||
        !Near(projectedSpheres[0].screenRadius, 2.5f) || !Near(projectedSpheres[1].x, 260.0f) ||
        !Near(projectedSpheres[1].y, 180.0f) || !Near(projectedSpheres[1].screenRadius, 5.0f)) {
        return 2;
    }

    projectedPoints[0].x = 123.0f;
    projectedSpheres[0].screenRadius = 456.0f;
    zMath::ProjectPointBatch(viewPoints, projectedPoints, 0);
    zMath_ProjectSphereBatch(viewPoints, projectedSpheres, 0);
    if (projectedPoints[0].x != 123.0f || projectedSpheres[0].screenRadius != 456.0f) {
        return 3;
    }

    zProjectedPoint projectedZBuf{340.0f, 220.0f, 0.1f};
    zVec3 unprojected{};
    zMath_UnprojectPointBatch(&projectedZBuf, &unprojected, 1);
    if (!Near(unprojected.x, 2.0f) || !Near(unprojected.y, 4.0f) || !Near(unprojected.z, 10.0f)) {
        return 4;
    }

    zMath::g_zMath_CameraScratchA = {1.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f,
                                     0.0f, 0.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    zMath_UnprojectPointBatchZBuf(&projectedZBuf, &unprojected, 1);
    return Near(unprojected.x, 6.0f) && Near(unprojected.y, 13.0f) && Near(unprojected.z, 36.0f)
               ? 0
               : 5;
}

extern "C" int zmath_perspective_texture_interpolants_smoke(void) {
    g_zMath_InvProjScaleX = 2.0f;
    g_zMath_InvProjScaleY = 3.0f;

    zVec3 triVerts[3] = {
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
    };
    zVec2 triUVs[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };
    zVec2 recipZGrad{};
    float recipZBase = 0.0f;
    zVec2 uOverZGrad{};
    float uOverZBase = 0.0f;
    zVec2 vOverZGrad{};
    float vOverZBase = 0.0f;

    zMath_BuildPerspectiveTextureInterpolants(triVerts, triUVs, &recipZGrad, &recipZBase,
                                              &uOverZGrad, &uOverZBase, &vOverZGrad, &vOverZBase);

    if (!Near(recipZGrad.x, 0.0f) || !Near(recipZGrad.y, 0.0f) || !Near(recipZBase, 1.0f) ||
        !Near(uOverZGrad.x, 2.0f) || !Near(uOverZGrad.y, 0.0f) || !Near(uOverZBase, 0.0f) ||
        !Near(vOverZGrad.x, 0.0f) || !Near(vOverZGrad.y, 3.0f) || !Near(vOverZBase, 0.0f)) {
        return 1;
    }

    triVerts[2] = {2.0f, 0.0f, 1.0f};
    recipZGrad = {99.0f, 99.0f};
    recipZBase = 99.0f;
    uOverZGrad = {99.0f, 99.0f};
    uOverZBase = 99.0f;
    vOverZGrad = {99.0f, 99.0f};
    vOverZBase = 99.0f;
    zMath_BuildPerspectiveTextureInterpolants(triVerts, triUVs, &recipZGrad, &recipZBase,
                                              &uOverZGrad, &uOverZBase, &vOverZGrad, &vOverZBase);

    return recipZGrad.x == 0.0f && recipZGrad.y == 0.0f && recipZBase == 1000.0f &&
                   uOverZGrad.x == 0.0f && uOverZGrad.y == 0.0f && uOverZBase == 0.0f &&
                   vOverZGrad.x == 0.0f && vOverZGrad.y == 0.0f && vOverZBase == 0.0f
               ? 0
               : 2;
}

extern "C" int zmath_vec3_normalize_and_div_scalar_smoke(void) {
    float byteMax = -1.0f;
    zFloat::Set255f(&byteMax);
    if (byteMax != 255.0f) {
        return 6;
    }

    zVec3 vec{3.0f, 4.0f, 12.0f};
    const float length = zMath::Vec3Normalize(&vec);
    if (!Near(length, 13.0f) || !Near(vec.x, 3.0f / 13.0f) || !Near(vec.y, 4.0f / 13.0f) ||
        !Near(vec.z, 12.0f / 13.0f)) {
        return 1;
    }

    zVec3 zero{};
    const float zeroLength = zMath::Vec3Normalize(&zero);
    if (zeroLength != 0.0f || zero.x != 0.0f || zero.y != 0.0f || zero.z != 0.0f) {
        return 2;
    }

    zVec3 flat{3.0f, 9.0f, 4.0f};
    zVec3 flatOut{10.0f, 11.0f, 12.0f};
    zMath::Vec3NormalizeXZ(&flat, &flatOut);
    if (flat.y != 9.0f || !Near(flatOut.x, 0.6f) || flatOut.y != 11.0f ||
        !Near(flatOut.z, 0.8f)) {
        return 8;
    }

    zVec3 flatZero{0.0f, 7.0f, 0.0f};
    flatOut.x = 5.0f;
    flatOut.y = 6.0f;
    flatOut.z = 8.0f;
    zMath::Vec3NormalizeXZ(&flatZero, &flatOut);
    if (flatZero.y != 7.0f || flatOut.x != 0.0f || flatOut.y != 6.0f ||
        flatOut.z != 0.0f) {
        return 9;
    }

    zVec3 deltaA{5.0f, -1.0f, 7.0f};
    zVec3 deltaB{2.0f, 3.0f, -1.0f};
    const float deltaLengthSq = zMath::Vec3DeltaLengthSq(&deltaA, &deltaB);
    if (!Near(deltaLengthSq, 89.0f) || zMath::g_zMath_Vec3DeltaScratch.x != 3.0f ||
        zMath::g_zMath_Vec3DeltaScratch.y != -4.0f || zMath::g_zMath_Vec3DeltaScratch.z != 8.0f) {
        return 7;
    }

    zVec3 source{8.0f, -4.0f, 2.0f};
    zVec3 out{};
    zMath_Vec3_DivScalar(&source, &out, 2.0f);
    if (!Near(out.x, 4.0f) || !Near(out.y, -2.0f) || !Near(out.z, 1.0f)) {
        return 3;
    }

    out = {};
    zMath_Vec3_DivScalar(&source, &out, 0.0f);
    if (out.x != source.x || out.y != source.y || out.z != source.z) {
        return 4;
    }

    zMath_Vec3_DivScalar(&source, &source, 0.0f);
    return source.x == 8.0f && source.y == -4.0f && source.z == 2.0f ? 0 : 5;
}

extern "C" int zmath_array_add_scaled_and_transform_smoke(void) {
    zVec3 bias[2] = {{1.0f, 2.0f, 3.0f}, {-1.0f, -2.0f, -3.0f}};
    zVec3 delta[2] = {{4.0f, 6.0f, 8.0f}, {10.0f, 12.0f, 14.0f}};
    zVec3 out[2] = {};
    zMath_Vec3Array_AddScaled(out, bias, delta, 2, 0.5f);
    if (!Near(out[0].x, 3.0f) || !Near(out[0].y, 5.0f) || !Near(out[0].z, 7.0f) ||
        !Near(out[1].x, 4.0f) || !Near(out[1].y, 4.0f) || !Near(out[1].z, 4.0f)) {
        return 1;
    }

    zVec3 tri0{0.0f, 0.0f, 0.0f};
    zVec3 tri1{1.0f, 0.0f, 0.0f};
    zVec3 tri2{0.0f, 1.0f, 0.0f};
    zVec3 normal{};
    zMath_Vec3_TriangleNormal(&tri0, &tri1, &tri2, &normal);
    if (!Near(normal.x, 0.0f) || !Near(normal.y, 0.0f) || !Near(normal.z, 1.0f)) {
        return 23;
    }

    float gradX = -1.0f;
    float gradY = -1.0f;
    zMath_SolveLinearGradient2D(&gradX, &gradY, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                                2.0f);
    if (!Near(gradX, 1.0f) || !Near(gradY, 2.0f)) {
        return 24;
    }

    zMath_SolveLinearGradient2D(&gradX, &gradY, 0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f,
                                2.0f);
    if (gradX != 0.0f || gradY != 0.0f) {
        return 25;
    }

    zVec3 points[2] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
    zVec3 original = points[0];
    static std::int32_t matrixFlags[1] = {1};
    static float *matrixSlots[1] = {};
    zMat4x3 matrix{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 20.0f, 30.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zMath::MatTransformPointBatchInPlace(points, 2);
    if (points[0].x != original.x || points[0].y != original.y || points[0].z != original.z) {
        return 2;
    }

    matrixFlags[0] = 0;
    zMath::MatTransformPointBatchInPlace(points, 1);
    if (!Near(points[0].x, 40.0f) || !Near(points[0].y, 56.0f) || !Near(points[0].z, 72.0f) ||
        points[1].x != 4.0f || points[1].y != 5.0f || points[1].z != 6.0f) {
        return 3;
    }

    points[0] = {7.0f, 8.0f, 9.0f};
    zMath::MatTransformPointBatchInPlace(points, 0);
    if (points[0].x != 7.0f || points[0].y != 8.0f || points[0].z != 9.0f) {
        return 4;
    }

    static std::int32_t setupFlags[2] = {0, 0};
    static float *setupSlots[2] = {};
    zMat4x3 parent{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};
    zMat4x3 current{};
    setupSlots[0] = reinterpret_cast<float *>(&parent);
    setupSlots[1] = reinterpret_cast<float *>(&current);
    zMath::g_currentMatrixIdentityFlagSlot = &setupFlags[1];
    zMath::g_currentMatrixPtrSlot = &setupSlots[1];
    setupFlags[1] = 42;
    if (zMath_Mat_GetCurrent() != &current || zMath_Mat_IsCurrentIdentity() != 42) {
        return 7;
    }

    current = {};
    current.xx = 1.0f;
    current.yy = 1.0f;
    current.zz = 1.0f;
    setupFlags[1] = 1;
    zMath::MatRotateY(1.57079632679f);
    if (setupFlags[1] != 0 || current.xx < -0.001f || current.xx > 0.001f || current.xz < -1.001f ||
        current.xz > -0.999f || current.zx < 0.999f || current.zx > 1.001f ||
        current.zz < -0.001f || current.zz > 0.001f) {
        return 8;
    }

    current = matrix;
    setupFlags[1] = 0;
    zMath::MatRotateY(1.57079632679f);
    if (!Near(current.xx, -7.0f) || !Near(current.xy, -8.0f) || !Near(current.xz, -9.0f) ||
        !Near(current.yx, 4.0f) || !Near(current.yy, 5.0f) || !Near(current.yz, 6.0f) ||
        !Near(current.zx, 1.0f) || !Near(current.zy, 2.0f) || !Near(current.zz, 3.0f) ||
        !Near(current.posX, 10.0f) || !Near(current.posY, 20.0f) || !Near(current.posZ, 30.0f)) {
        return 9;
    }

    current = {};
    current.xx = 1.0f;
    current.yy = 1.0f;
    current.zz = 1.0f;
    setupFlags[1] = 1;
    zMath::MatTranslate(1.0f, 2.0f, 3.0f);
    if (setupFlags[1] != 0 || !Near(current.posX, 1.0f) || !Near(current.posY, 2.0f) ||
        !Near(current.posZ, 3.0f)) {
        return 17;
    }

    current = matrix;
    setupFlags[1] = 0;
    zMath::MatTranslate(1.0f, 2.0f, 3.0f);
    if (!Near(current.posX, 40.0f) || !Near(current.posY, 56.0f) || !Near(current.posZ, 72.0f) ||
        !Near(current.xx, 1.0f) || !Near(current.zz, 9.0f)) {
        return 18;
    }

    current = {};
    current.xx = 1.0f;
    current.yy = 1.0f;
    current.zz = 1.0f;
    setupFlags[1] = 1;
    zMath::MatRotateX(1.57079632679f);
    if (setupFlags[1] != 0 || !Near(current.yy, 0.0f) || !Near(current.yz, 1.0f) ||
        !Near(current.zy, -1.0f) || !Near(current.zz, 0.0f)) {
        return 19;
    }

    current = matrix;
    setupFlags[1] = 0;
    zMath::MatRotateX(1.57079632679f);
    if (!Near(current.yx, 7.0f) || !Near(current.yy, 8.0f) || !Near(current.yz, 9.0f) ||
        !Near(current.zx, -4.0f) || !Near(current.zy, -5.0f) || !Near(current.zz, -6.0f) ||
        !Near(current.posX, 10.0f)) {
        return 20;
    }

    current = {};
    current.xx = 1.0f;
    current.yy = 1.0f;
    current.zz = 1.0f;
    setupFlags[1] = 1;
    zMath::MatRotateZ(1.57079632679f);
    if (setupFlags[1] != 0 || !Near(current.xx, 0.0f) || !Near(current.xy, 1.0f) ||
        !Near(current.yx, -1.0f) || !Near(current.yy, 0.0f)) {
        return 21;
    }

    current = matrix;
    setupFlags[1] = 0;
    zMath::MatRotateZ(1.57079632679f);
    if (!Near(current.xx, 4.0f) || !Near(current.xy, 5.0f) || !Near(current.xz, 6.0f) ||
        !Near(current.yx, -1.0f) || !Near(current.yy, -2.0f) || !Near(current.yz, -3.0f) ||
        !Near(current.zx, 7.0f) || !Near(current.posZ, 30.0f)) {
        return 22;
    }

    zMat4x3 yawMatrix = {};
    if (zMath_Mat_ExtractYaw(&yawMatrix) != 0.0f) {
        return 10;
    }
    yawMatrix.zx = 1.0f;
    yawMatrix.zz = 0.0f;
    const float yaw = zMath_Mat_ExtractYaw(&yawMatrix);
    if (yaw < 1.570f || yaw > 1.571f) {
        return 11;
    }

    zVec3 rotated{};
    zVec3 rotateSource{1.0f, 2.0f, 3.0f};
    zMath_Vec3_RotateX(&rotated, &rotateSource, 1.57079632679f);
    if (!Near(rotated.x, 1.0f) || !Near(rotated.y, -3.0f) || !Near(rotated.z, 2.0f)) {
        return 13;
    }
    zMath::Vec3RotateY(&rotated, &rotateSource, 1.57079632679f);
    if (!Near(rotated.x, 3.0f) || !Near(rotated.y, 2.0f) || !Near(rotated.z, -1.0f)) {
        return 14;
    }

    zVec3 euler{};
    zMat4x3 identityMatrix{};
    identityMatrix.xx = 1.0f;
    identityMatrix.yy = 1.0f;
    identityMatrix.zz = 1.0f;
    zMath_Mat_ExtractEulerAngles(&identityMatrix, &euler);
    if (!Near(euler.x, 0.0f) || !Near(euler.y, 0.0f) || !Near(euler.z, 0.0f)) {
        return 15;
    }
    zMat4x3 yawOnlyMatrix{};
    yawOnlyMatrix.xz = -1.0f;
    yawOnlyMatrix.yy = 1.0f;
    yawOnlyMatrix.zx = 1.0f;
    zMath_Mat_ExtractEulerAngles(&yawOnlyMatrix, &euler);
    if (euler.y < 1.570f || euler.y > 1.571f) {
        return 16;
    }

    setupFlags[1] = 0;

    zMath::g_zMath_CameraScratchA = matrix;
    zMath::MatLoadCameraScratchA();
    if (!Near(current.posX, 10.0f) || !Near(current.posY, 20.0f) || !Near(current.posZ, 30.0f)) {
        return 5;
    }

    zMath::g_zMath_CameraScratchB = {};
    zMath::g_zMath_CameraScratchB.xx = 1.0f;
    zMath::g_zMath_CameraScratchB.yy = 1.0f;
    zMath::g_zMath_CameraScratchB.zz = 1.0f;
    zMath::g_zMath_CameraScratchB.posX = 1.0f;
    zMath::g_zMath_CameraScratchB.posY = 2.0f;
    zMath::g_zMath_CameraScratchB.posZ = 3.0f;
    zMath_Mat_SetupCamera();
    if (!Near(current.posX, 11.0f) || !Near(current.posY, 22.0f) || !Near(current.posZ, 33.0f)) {
        return 6;
    }

    parent = {};
    parent.xx = 1.0f;
    parent.yy = 1.0f;
    parent.zz = 1.0f;
    parent.posX = 10.0f;
    parent.posY = 20.0f;
    parent.posZ = 30.0f;
    current = {};
    setupFlags[0] = 1;
    setupFlags[1] = 0;
    zMath::g_zMath_CameraScratchB = {};
    zMath::g_zMath_CameraScratchB.xx = 1.0f;
    zMath::g_zMath_CameraScratchB.yy = 1.0f;
    zMath::g_zMath_CameraScratchB.zz = 1.0f;
    zMath_Mat_LoadProjection(1.57079632679f);
    return current.xx > -0.001f && current.xx < 0.001f && current.xz < -0.999f &&
                   current.xz > -1.001f && current.zx > 0.999f && current.zx < 1.001f &&
                   current.zz > -0.001f && current.zz < 0.001f && Near(current.posX, 10.0f) &&
                   Near(current.posY, 20.0f) && Near(current.posZ, 30.0f)
               ? 0
               : 12;
}

extern "C" int zmath_load_view_smoke(void) {
    static std::int32_t flags[2] = {1, 0};
    static float *slots[2] = {};
    zMat4x3 parent{};
    parent.xx = 1.0f;
    parent.yy = 1.0f;
    parent.zz = 1.0f;
    parent.posX = 10.0f;
    parent.posY = 20.0f;
    parent.posZ = 30.0f;
    zMat4x3 current{};
    slots[0] = reinterpret_cast<float *>(&parent);
    slots[1] = reinterpret_cast<float *>(&current);
    zMath::g_currentMatrixIdentityFlagSlot = &flags[1];
    zMath::g_currentMatrixPtrSlot = &slots[1];

    zMath::g_zMath_CameraScratchA = {};
    zMath::g_zMath_CameraScratchA.xx = 1.0f;
    zMath::g_zMath_CameraScratchA.yy = -1.0f;
    zMath::g_zMath_CameraScratchA.zz = -1.0f;
    zMath::g_zMath_CameraScratchB = {};
    zMath::g_zMath_CameraScratchB.xx = 1.0f;
    zMath::g_zMath_CameraScratchB.yy = 1.0f;
    zMath::g_zMath_CameraScratchB.zz = 1.0f;

    zMath_Mat_LoadView();
    if (flags[1] != 0 || !Near(current.xx, 1.0f) || !Near(current.yy, 1.0f) ||
        !Near(current.zz, 1.0f) || !Near(current.posX, 10.0f) || !Near(current.posY, 20.0f) ||
        !Near(current.posZ, 30.0f)) {
        return 1;
    }

    flags[0] = 0;
    flags[1] = 0;
    parent.posX = 3.0f;
    parent.posY = 4.0f;
    parent.posZ = 5.0f;
    zMath::g_zMath_CameraScratchB.posX = 1.0f;
    zMath::g_zMath_CameraScratchB.posY = 2.0f;
    zMath::g_zMath_CameraScratchB.posZ = 3.0f;
    zMath_Mat_LoadView();
    return Near(current.posX, 4.0f) && Near(current.posY, 6.0f) && Near(current.posZ, 8.0f) ? 0 : 2;
}

extern "C" int zmath_quaternion_helpers_smoke(void) {
    zQuat quat{};
    zMath_Quat_FromEuler(&quat, 0.0f, 0.0f, 0.0f);
    if (!Near(quat.w, 1.0f) || !Near(quat.x, 0.0f) || !Near(quat.y, 0.0f) || !Near(quat.z, 0.0f)) {
        return 1;
    }

    zMath_Quat_FromEuler(&quat, 3.14159265359f, 0.0f, 0.0f);
    if (!Near(quat.w, 0.0f) || !Near(quat.x, 0.0f) || !Near(quat.y, 1.0f) || !Near(quat.z, 0.0f)) {
        return 2;
    }

    const zQuat quatA{0.5f, 0.5f, 0.5f, 0.5f};
    const zQuat quatB{0.5f, -0.5f, 0.5f, -0.5f};
    zQuat product{};
    zMath_Quat_MultiplyInverse(&quatA, &quatB, &product);
    if (!Near(product.w, 0.0f) || !Near(product.x, 1.0f) || !Near(product.y, 0.0f) ||
        !Near(product.z, 0.0f)) {
        return 3;
    }

    zMat4x3 matrix{};
    matrix.posX = 7.0f;
    matrix.posY = 8.0f;
    matrix.posZ = 9.0f;
    const zQuat identityQuat{1.0f, 0.0f, 0.0f, 0.0f};
    zMath_Quat_ToMatrix(&identityQuat, &matrix);
    if (!Near(matrix.xx, 1.0f) || !Near(matrix.yy, 1.0f) || !Near(matrix.zz, 1.0f) ||
        !Near(matrix.xy, 0.0f) || !Near(matrix.posX, 7.0f) || !Near(matrix.posY, 8.0f) ||
        !Near(matrix.posZ, 9.0f)) {
        return 4;
    }

    const float halfSqrt2 = 0.70710678118f;
    const zQuat yQuarterTurn{halfSqrt2, 0.0f, halfSqrt2, 0.0f};
    zMath_Quat_ToMatrix(&yQuarterTurn, &matrix);
    if (!Near(matrix.xx, 0.0f) || !Near(matrix.yy, 1.0f) || !Near(matrix.zz, 0.0f) ||
        !Near(matrix.xz, -1.0f) || !Near(matrix.zx, 1.0f)) {
        return 5;
    }

    return 0;
}
