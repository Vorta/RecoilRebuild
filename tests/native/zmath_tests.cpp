#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/include/zClipRect.h"
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

    zMat4x3 rotationSource{11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f,
                           17.0f, 18.0f, 19.0f, 110.0f, 120.0f, 130.0f};
    matrix.posX = 201.0f;
    matrix.posY = 202.0f;
    matrix.posZ = 203.0f;
    flags[1] = 1;
    zMath::MatLoadRotationFrom3x3(&rotationSource);
    if (flags[1] != 0 || matrix.xx != 11.0f || matrix.xy != 12.0f ||
        matrix.xz != 13.0f || matrix.yx != 14.0f || matrix.yy != 15.0f ||
        matrix.yz != 16.0f || matrix.zx != 17.0f || matrix.zy != 18.0f ||
        matrix.zz != 19.0f || matrix.posX != 201.0f || matrix.posY != 202.0f ||
        matrix.posZ != 203.0f) {
        return 14;
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

    zVec3 elevationA{1.0f, 2.0f, 3.0f};
    zVec3 elevationB{4.0f, 7.0f, -1.0f};
    if (!Near(zMath_Vec3_ElevationAngleBetweenPoints(&elevationA, &elevationB), 0.785398185f)) {
        return 13;
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

extern "C" int zmath_vec3_array_transform_direction_smoke(void) {
    std::int32_t flags[1] = {1};
    float *slots[1] = {};
    zMat4x3 matrix{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f,
                   100.0f, 200.0f, 300.0f};
    slots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &flags[0];
    zMath::g_currentMatrixPtrSlot = &slots[0];

    zVec3 vectors[2] = {{1.0f, 2.0f, 3.0f}, {-1.0f, 0.5f, 2.0f}};
    zMath::Vec3ArrayTransformDirection(vectors, 2);
    if (vectors[0].x != 1.0f || vectors[0].y != 2.0f || vectors[0].z != 3.0f ||
        vectors[1].x != -1.0f || vectors[1].y != 0.5f || vectors[1].z != 2.0f) {
        return 1;
    }

    flags[0] = 0;
    zMath::Vec3ArrayTransformDirection(vectors, 1);
    if (!Near(vectors[0].x, 30.0f) || !Near(vectors[0].y, 36.0f) ||
        !Near(vectors[0].z, 42.0f) || vectors[1].x != -1.0f || vectors[1].y != 0.5f ||
        vectors[1].z != 2.0f) {
        return 2;
    }

    vectors[0] = {4.0f, 5.0f, 6.0f};
    zMath::Vec3ArrayTransformDirection(vectors, 0);
    if (vectors[0].x != 4.0f || vectors[0].y != 5.0f || vectors[0].z != 6.0f) {
        return 3;
    }

    zMath::Vec3ArrayTransformDirection(vectors, -1);
    return vectors[0].x == 4.0f && vectors[0].y == 5.0f && vectors[0].z == 6.0f ? 0 : 4;
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

extern "C" int zmath_extract_euler_smoke(void) {
    zMat4x3 matrix{};
    if (zMath_Mat_ExtractYaw(&matrix) != 0.0f) {
        return 1;
    }

    matrix.zx = 1.0f;
    matrix.zz = 0.0f;
    if (!Near(zMath_Mat_ExtractYaw(&matrix), 1.57079637f)) {
        return 2;
    }

    zVec3 inVec{1.0f, 2.0f, 3.0f};
    zVec3 outVec{};
    zMath_Vec3_RotateX(&outVec, &inVec, 1.57079637f);
    if (!Near(outVec.x, 1.0f) || !Near(outVec.y, -3.0f) || !Near(outVec.z, 2.0f)) {
        return 3;
    }

    zMath::MatBuildEulerRotation3x3(&matrix, 0.0f, 0.0f, 0.0f);
    zVec3 euler{};
    zMath_Mat_ExtractEulerAngles(&matrix, &euler);
    if (!Near(euler.x, 0.0f) || !Near(euler.y, 0.0f) || !Near(euler.z, 0.0f)) {
        return 4;
    }

    zMath::MatBuildEulerRotation3x3(&matrix, 0.5f, -0.25f, 0.75f);
    zMath_Mat_ExtractEulerAngles(&matrix, &euler);
    return Near(euler.x, 0.5f) && Near(euler.y, -0.25f) && Near(euler.z, 0.75f) ? 0 : 5;
}

extern "C" int zmath_projection_batches_smoke(void) {
    g_zMath_ProjScaleX = 100.0f;
    g_zMath_ProjScaleY = -50.0f;
    g_zMath_ProjOffsetX = 320.0f;
    g_zMath_ProjOffsetY = 240.0f;
    g_zMath_InvProjScaleX = 0.01f;
    g_zMath_InvProjScaleY = -0.02f;
    g_zMath_ProjSphereRadiusScale = 25.0f;

    const zVec2 lastScale = zMath_Project_GetLastScreenScaleXY();
    if (!Near(lastScale.x, 100.0f) || !Near(lastScale.y, -50.0f)) {
        return 6;
    }

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

extern "C" int zmath_project_point_and_clamp_to_screen_clip_smoke(void) {
    int matrixIdentityFlags[2] = {};
    float *matrixSlots[2] = {};
    zMat4x3 baseMatrix{};
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    matrixSlots[0] = reinterpret_cast<float *>(&baseMatrix);

    zMath::g_zMath_CameraScratchB = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    g_zMath_ProjScaleX = 100.0f;
    g_zMath_ProjScaleY = -50.0f;
    g_zMath_ProjOffsetX = 320.0f;
    g_zMath_ProjOffsetY = 240.0f;
    gClipRect_Primary.zMin = 1.0f;
    gClipRect_Primary.xMaxAlt = 640.0f;
    g_zVideo_ProjectClipLeft = 0.0f;
    g_zVideo_ProjectClipTop = 0.0f;
    g_zVideo_ProjectClipRight = 640.0f;
    g_zVideo_ProjectClipBottom = 480.0f;

    zVec3 point{1.0f, 2.0f, 10.0f};
    zVec3 projected{};
    if (zMath::ProjectPointAndClampToScreenClip(&point, &projected) != 0 ||
        !Near(projected.x, 330.0f) || !Near(projected.y, 230.0f) ||
        !Near(projected.z, 0.1f)) {
        return 1;
    }

    point = {-100.0f, 0.0f, 10.0f};
    if (zMath::ProjectPointAndClampToScreenClip(&point, &projected) != 1 ||
        projected.x != 0.0f || !Near(projected.y, 240.0f)) {
        return 2;
    }

    point = {0.0f, 100.0f, 10.0f};
    if (zMath::ProjectPointAndClampToScreenClip(&point, &projected) != 4 ||
        projected.y != 0.0f) {
        return 3;
    }

    point = {0.0f, -100.0f, 10.0f};
    if (zMath::ProjectPointAndClampToScreenClip(&point, &projected) != 8 ||
        projected.y != 479.0f) {
        return 4;
    }

    point = {0.0f, 0.0f, -2.0f};
    const int result = zMath::ProjectPointAndClampToScreenClip(&point, &projected);
    return result == 8 && projected.y == 480.0f && Near(projected.x, 340.48f) ? 0 : 5;
}

extern "C" int zmath_vec3_lerp_smoke(void) {
    zVec3 inOut = {10.0f, -4.0f, 8.0f};
    zVec3 other = {2.0f, 20.0f, -2.0f};
    zMath::Vec3Lerp(&inOut, &other, 0.25f);

    return Near(inOut.x, 4.0f) && Near(inOut.y, 14.0f) && Near(inOut.z, 0.5f) &&
                   other.x == 2.0f && other.y == 20.0f && other.z == -2.0f
               ? 0
               : 1;
}

extern "C" int zmath_vec3_lerp_normalize_smoke(void) {
    zVec3 inOut = {1.0f, 0.0f, 0.0f};
    zVec3 other = {0.0f, 1.0f, 0.0f};
    zMath::Vec3LerpNormalize(&inOut, &other, 0.25f);

    const float expectedScale = 1.0f / sqrt(0.625f);
    if (!Near(inOut.x, 0.25f * expectedScale) ||
        !Near(inOut.y, 0.75f * expectedScale) || !Near(inOut.z, 0.0f)) {
        return 1;
    }

    zVec3 zeroA = {1.0f, 0.0f, 0.0f};
    zVec3 zeroB = {-1.0f, 0.0f, 0.0f};
    zMath::Vec3LerpNormalize(&zeroA, &zeroB, 0.5f);

    return zeroA.x == 0.0f && zeroA.y == 0.0f && zeroA.z == 0.0f ? 0 : 2;
}

extern "C" int zmath_vec3_direction_to_smoke(void) {
    const zVec3 from = {1.0f, 2.0f, 3.0f};
    const zVec3 to = {4.0f, 6.0f, 3.0f};
    zVec3 dir = {};

    const float distance = zMath::Vec3DirectionTo(&from, &to, &dir);
    if (!Near(distance, 5.0f) || !Near(dir.x, 0.6f) || !Near(dir.y, 0.8f) ||
        !Near(dir.z, 0.0f)) {
        return 1;
    }

    zVec3 zeroDir = {7.0f, 8.0f, 9.0f};
    const float zeroDistance = zMath::Vec3DirectionTo(&from, &from, &zeroDir);
    return zeroDistance == 0.0f && zeroDir.x == 0.0f && zeroDir.y == 0.0f &&
                   zeroDir.z == 0.0f
               ? 0
               : 2;
}

extern "C" int zmath_line_vs_sphere_hit_smoke(void) {
    const zVec3 rayStart = {10.0f, 0.0f, 0.0f};
    const zVec3 rayOrigin = {0.0f, 0.0f, 0.0f};
    zVec3 inwardNormal = {};

    const zVec3 sphereAhead = {5.0f, 0.0f, 0.0f};
    if (zMath::LineVsSphereHit(&rayStart, &rayOrigin, 1.0f, &sphereAhead, &inwardNormal) != 1 ||
        !Near(inwardNormal.x, -1.0f) || !Near(inwardNormal.y, 0.0f) ||
        !Near(inwardNormal.z, 0.0f)) {
        return 1;
    }

    const zVec3 sphereAtOriginSurface = {1.0f, 0.0f, 0.0f};
    inwardNormal = {};
    if (zMath::LineVsSphereHit(&rayStart, &rayOrigin, 1.0f, &sphereAtOriginSurface,
                               &inwardNormal) != 1 ||
        !Near(inwardNormal.x, -1.0f) || !Near(inwardNormal.y, 0.0f) ||
        !Near(inwardNormal.z, 0.0f)) {
        return 2;
    }

    const zVec3 sphereBehind = {-5.0f, 0.0f, 0.0f};
    inwardNormal = {7.0f, 8.0f, 9.0f};
    if (zMath::LineVsSphereHit(&rayStart, &rayOrigin, 1.0f, &sphereBehind, &inwardNormal) != 0 ||
        inwardNormal.x != 7.0f || inwardNormal.y != 8.0f || inwardNormal.z != 9.0f) {
        return 3;
    }

    const zVec3 zeroLine = {};
    if (zMath::LineVsSphereHit(&zeroLine, &zeroLine, 1.0f, &sphereAhead, &inwardNormal) != 0) {
        return 4;
    }

    const zVec3 sphereBeside = {5.0f, 2.0f, 0.0f};
    return zMath::LineVsSphereHit(&rayStart, &rayOrigin, 1.0f, &sphereBeside, &inwardNormal) == 0
               ? 0
               : 5;
}

extern "C" int zmath_vec3_perp2d_smoke(void) {
    zVec3 in = {0.0f, 7.0f, 9.0f};
    zVec3 out = {3.0f, 4.0f, 5.0f};
    zMath::Vec3Perp2D(&in, &out);
    if (out.x != 1.0f || out.y != 0.0f || out.z != 0.0f) {
        return 1;
    }

    in = {3.0f, 4.0f, 9.0f};
    out = {};
    zMath::Vec3Perp2D(&in, &out);
    return Near(out.x, 0.7804878f) && Near(out.y, -0.5853658f) && out.z == 0.0f ? 0 : 2;
}

extern "C" int zmath_vec3_perp_xz_smoke(void) {
    const zVec3 in = {3.0f, 7.0f, -5.0f};
    zVec3 out = {1.0f, 2.0f, 3.0f};

    zMath::Vec3PerpXZ(&in, &out);

    return out.x == 5.0f && out.y == 0.0f && out.z == 3.0f ? 0 : 1;
}

extern "C" int zmath_vec3_scale_add_smoke(void) {
    const zVec3 vec = {10.0f, -4.0f, 8.0f};
    const zVec3 delta = {2.0f, 20.0f, -2.0f};
    zVec3 out = {};

    zMath::Vec3ScaleAdd(&vec, &delta, 0.25f, &out);
    if (!Near(out.x, 10.5f) || !Near(out.y, 1.0f) || !Near(out.z, 7.5f)) {
        return 1;
    }

    out = {};
    zMath::Vec3ScaleAdd(&vec, &delta, -2.0f, &out);
    return Near(out.x, 6.0f) && Near(out.y, -44.0f) && Near(out.z, 12.0f) ? 0 : 2;
}

extern "C" int zmath_vec3_slerp_smoke(void) {
    const zVec3 a = {1.0f, 0.0f, 0.0f};
    const zVec3 b = {0.0f, 1.0f, 0.0f};
    zVec3 out = {};

    zMath::Vec3Slerp(&a, &b, 0.0f, &out);
    if (out.x != 1.0f || out.y != 0.0f || out.z != 0.0f) {
        return 1;
    }

    zMath::Vec3Slerp(&a, &b, 1.0f, &out);
    if (out.x != 0.0f || out.y != 1.0f || out.z != 0.0f) {
        return 2;
    }

    zMath::Vec3Slerp(&a, &b, 0.5f, &out);
    if (!Near(out.x, 0.70710677f) || !Near(out.y, 0.70710677f) || !Near(out.z, 0.0f)) {
        return 3;
    }

    const zVec3 nearB = {0.98f, 0.1f, 0.0f};
    zMath::Vec3Slerp(&a, &nearB, 0.25f, &out);
    if (!Near(out.x, 0.995f) || !Near(out.y, 0.025f) || !Near(out.z, 0.0f)) {
        return 4;
    }

    const zVec3 opposite = {-1.0f, 0.0f, 0.0f};
    zMath::Vec3Slerp(&a, &opposite, 0.5f, &out);
    return Near(out.x, 0.0f) && Near(out.y, -1.0f) && Near(out.z, 0.0f) ? 0 : 5;
}

extern "C" int zmath_vec3_midpoint_smoke(void) {
    zVec3 a = {10.0f, -4.0f, 8.0f};
    zVec3 b = {2.0f, 20.0f, -2.0f};
    zVec3 out = {};
    zVec3 *returned = zMath::Vec3Midpoint(&a, &b, &out);

    return returned == &out && Near(out.x, 6.0f) && Near(out.y, 8.0f) &&
                   Near(out.z, 3.0f) && a.x == 10.0f && b.y == 20.0f
               ? 0
               : 1;
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

    zVec3 reflectNormal{0.0f, 1.0f, 0.0f};
    zVec3 incident{2.0f, -3.0f, 4.0f};
    zVec3 reflected{};
    zMath::Vec3Reflect(&reflectNormal, &incident, &reflected);
    if (!Near(reflected.x, 2.0f) || !Near(reflected.y, 3.0f) ||
        !Near(reflected.z, 4.0f)) {
        return 10;
    }

    zVec3 perpendicularNormal{0.0f, 1.0f, 0.0f};
    zVec3 perpendicularIncident{3.0f, 0.0f, -2.0f};
    zMath::Vec3Reflect(&perpendicularNormal, &perpendicularIncident, &reflected);
    if (!Near(reflected.x, -3.0f) || !Near(reflected.y, 0.0f) ||
        !Near(reflected.z, 2.0f)) {
        return 11;
    }

    zVec3 deltaA{5.0f, -1.0f, 7.0f};
    zVec3 deltaB{2.0f, 3.0f, -1.0f};
    const float deltaLengthSq = zMath::Vec3DeltaLengthSq(&deltaA, &deltaB);
    if (!Near(deltaLengthSq, 89.0f) || zMath::g_zMath_Vec3DeltaScratch.x != 3.0f ||
        zMath::g_zMath_Vec3DeltaScratch.y != -4.0f || zMath::g_zMath_Vec3DeltaScratch.z != 8.0f) {
        return 7;
    }
    const float deltaLength = zMath::Vec3DeltaLength(&deltaA, &deltaB);
    if (!Near(deltaLength, sqrt(89.0f)) || zMath::g_zMath_Vec3DeltaScratch.x != 3.0f ||
        zMath::g_zMath_Vec3DeltaScratch.y != -4.0f || zMath::g_zMath_Vec3DeltaScratch.z != 8.0f) {
        return 12;
    }

    zMath::g_zMath_Vec3DeltaScratch.y = 123.0f;
    const float distSqXZ = zMath::Vec3DistSqXZ(&deltaA, &deltaB);
    if (!Near(distSqXZ, 73.0f) || zMath::g_zMath_Vec3DeltaScratch.x != 3.0f ||
        zMath::g_zMath_Vec3DeltaScratch.y != 123.0f ||
        zMath::g_zMath_Vec3DeltaScratch.z != 8.0f) {
        return 13;
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
    zMath_Vec3_DirFromYaw(&rotated, 0.0f);
    if (!Near(rotated.x, 0.0f) || !Near(rotated.y, 0.0f) || !Near(rotated.z, -1.0f)) {
        return 23;
    }
    zMath_Vec3_DirFromYaw(&rotated, 1.57079632679f);
    if (!Near(rotated.x, -1.0f) || !Near(rotated.y, 0.0f) || !Near(rotated.z, 0.0f)) {
        return 24;
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

    const zQuat identityProductQuat{1.0f, 0.0f, 0.0f, 0.0f};
    zMath_Quat_Multiply(&identityProductQuat, &quatB, &product);
    if (!Near(product.w, 0.5f) || !Near(product.x, -0.5f) || !Near(product.y, 0.5f) ||
        !Near(product.z, -0.5f)) {
        return 4;
    }

    const zVec3 zeroRotation{0.0f, 0.0f, 0.0f};
    zMath_Quat_FromRotationVector(&zeroRotation, &quat);
    if (!Near(quat.w, 1.0f) || !Near(quat.x, 0.0f) || !Near(quat.y, 0.0f) ||
        !Near(quat.z, 0.0f)) {
        return 5;
    }

    const zVec3 zQuarterRotation{0.0f, 0.0f, 1.57079632679f};
    zMath_Quat_FromRotationVector(&zQuarterRotation, &quat);
    if (!Near(quat.w, 0.0f) || !Near(quat.x, 0.0f) || !Near(quat.y, 0.0f) ||
        !Near(quat.z, 1.0f)) {
        return 6;
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
        return 7;
    }

    const float halfSqrt2 = 0.70710678118f;
    const zQuat yQuarterTurn{halfSqrt2, 0.0f, halfSqrt2, 0.0f};
    zMath_Quat_ToMatrix(&yQuarterTurn, &matrix);
    if (!Near(matrix.xx, 0.0f) || !Near(matrix.yy, 1.0f) || !Near(matrix.zz, 0.0f) ||
        !Near(matrix.xz, -1.0f) || !Near(matrix.zx, 1.0f)) {
        return 8;
    }

    return 0;
}

extern "C" int zmath_approx_exp_neg_smoke(void) {
    g_zMath_ApproxExpNegDirty = 1;
    g_zMath_ApproxExpNegScale = 0.0f;
    for (int i = 0; i < 256; ++i) {
        g_zMath_ApproxExpNegTable[i] = -1.0f;
    }

    const float value = zMath::ApproxExpNeg(2.0f);
    if (g_zMath_ApproxExpNegDirty != 0 || g_zMath_ApproxExpNegScale != 51.0f ||
        !Near(g_zMath_ApproxExpNegTable[0], 1.0f) ||
        !Near(g_zMath_ApproxExpNegTable[102], expf(-2.0f)) ||
        !Near(value, g_zMath_ApproxExpNegTable[102])) {
        return 1;
    }

    const float tableSlot25 = g_zMath_ApproxExpNegTable[25];
    if (!Near(zMath::ApproxExpNeg(0.5f), tableSlot25)) {
        return 2;
    }
    if (zMath::ApproxExpNeg(-0.25f) != 1.0f) {
        return 3;
    }
    if (zMath::ApproxExpNeg(5.25f) != 0.0f) {
        return 4;
    }
    if (!Near(zMath::ApproxExpNeg(5.0f), g_zMath_ApproxExpNegTable[255])) {
        return 5;
    }

    return 0;
}
