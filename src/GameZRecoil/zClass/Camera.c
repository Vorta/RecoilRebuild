#include "zClass.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/include/zClipAlt.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
int g_zClass_CameraAutoClipDistanceAdjustEnabled = 0;
float g_zClass_CameraAutoClipDistanceThreshold = 0.04f;
float g_zClass_CameraAutoClipDistanceScale = 1.0f;
float g_zClass_CameraAutoClipDistanceStep = 0.05f;
float g_zClass_CameraAutoClipDistanceMinScale = 0.6f;
int g_zClass_ObjectHseTestEnabled = 1;
zClass_NodePartial *g_zClass_CurrentCamera = 0;
zClass_NodePartial *g_zClass_CameraTargetNode = 0;
zVec3 g_zCamera_FrustumFootprintPoints[9] = {0};
int g_zCamera_FrustumFootprintPointCount = 0;
zCamera_FrustumGridTileRingPartial g_zCamera_FrustumGridTileRings[50] = {0};
}

namespace {
    const int kZClassNodeCamera = 1;
    const int kZClassNodeWorld = 2;
    const char *kCameraSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Camera.c";

    void ReportCameraError(int sourceLine, const char *message) {
        zError::ReportOld(0x400, kCameraSourceFile, sourceLine, message);
    }

    int ValidateCameraNode(zClass_NodePartial * node, zClass_CameraDataPartial * *outData,
                                    int nullLine, int dataLine, int classLine) {
        if (node == 0) {
            ReportCameraError(nullLine, "Null node pointer.");
            return 5;
        }

        if (node->classData == 0) {
            ReportCameraError(dataLine, "Null class data pointer");
            return 5;
        }

        if (node->classId != kZClassNodeCamera) {
            zError::ReportOld(0x400, kCameraSourceFile, classLine,
                              "Bad Class Found.\n Wanted (%d)\n Found (%d)", node->classId,
                              kZClassNodeCamera);
            return 3;
        }

        *outData = static_cast<zClass_CameraDataPartial *>(node->classData);
        return 0;
    }

    zVec3 *GetSelectedTargetVector(zClass_CameraDataPartial * data) {
        return (data->cameraFlags & 0x02) != 0 ? &data->worldTarget : &data->targetOrEuler;
    }

    float NegateFloatSignBit(float value) {
        unsigned int bits = 0;
        memcpy(&bits, &value, sizeof(bits));
        bits ^= 0x80000000u;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }

    int AbsInt(int value) {
        return value < 0 ? -value : value;
    }

    void ClearFrustumGridTileRings() {
        {
        for (int ringIndex = 0; ringIndex < 50; ++ringIndex) {
            g_zCamera_FrustumGridTileRings[ringIndex].count = 0;
        }
        }
    }

    void CopyCurrentCameraFrustumFootprint(zClass_CameraDataPartial * data,
                                           int pointCount) {
        memcpy(g_zCamera_FrustumFootprintPoints, &data->frustumOrigin,
                    pointCount * sizeof(zVec3));

        if (zMath_Mat_IsCurrentIdentity() == 0) {
            zMath::MatTransformPointBatchInPlace(g_zCamera_FrustumFootprintPoints, pointCount);
        }
    }

    int BuildCameraFrustumFootprint(zClass_CameraDataPartial * data,
                                             int filterErrorLine) {
        zMath::MatLoadIdentity();
        zMath::MatTranslate(data->cameraPos.x, data->cameraPos.y, data->cameraPos.z);
        zMath::MatRotateY(data->eulerAngles.y);

        int pointCount;
        if (fabs(data->eulerAngles.x) < 0.174533 &&
            fabs(data->eulerAngles.z) < 0.174533) {
            pointCount = 3;
        } else {
            pointCount = 5;
            zMath::MatRotateX(data->eulerAngles.x);
            zMath::MatRotateZ(data->eulerAngles.z);
        }

        g_zCamera_FrustumFootprintPointCount = pointCount;
        CopyCurrentCameraFrustumFootprint(data, pointCount);

        if (pointCount > 3) {
            pointCount =
                zClass_Camera::FindConvexHullXZ(g_zCamera_FrustumFootprintPoints, pointCount);
            g_zCamera_FrustumFootprintPointCount = pointCount;
        }

        if (zClass_cls_di::FilterRegionsAgainstMeshFaces(g_zCamera_FrustumFootprintPoints,
                                                         pointCount) == 0) {
            sprintf(g_zError_DebugMsgBuffer,
                         "%s: Line %d: ERROR from gModDIPointInPolygonInit() for camera "
                         "frustrum footprint.\n",
                         kCameraSourceFile, filterErrorLine);
            zError::EmitDebugBuffer(1);
        }

        return pointCount;
    }

    void GetFrustumFootprintBounds(int pointCount, float *minX, float *maxX,
                                   float *minZ, float *maxZ) {
        *minX = g_zCamera_FrustumFootprintPoints[0].x;
        *maxX = g_zCamera_FrustumFootprintPoints[0].x;
        *minZ = g_zCamera_FrustumFootprintPoints[0].z;
        *maxZ = g_zCamera_FrustumFootprintPoints[0].z;

        for (int i = 1; i < pointCount; ++i) {
            const zVec3 *point = &g_zCamera_FrustumFootprintPoints[i];
            if (point->x < *minX) {
                *minX = point->x;
            }
            if (point->x > *maxX) {
                *maxX = point->x;
            }
            if (point->z < *minZ) {
                *minZ = point->z;
            }
            if (point->z > *maxZ) {
                *maxZ = point->z;
            }
        }
    }

    void AddFrustumGridTile(int col, int row, int originCol,
                            int originRow, int clipMask,
                            int hasPosOffset, float posOffsetX, float posOffsetZ,
                            int cellErrorLine, int ringErrorLine) {
        const int ringIndex = AbsInt(col - originCol) + AbsInt(row - originRow);
        if (ringIndex >= 50) {
            zError::ReportOld(0x200, kCameraSourceFile, ringErrorLine,
                              "Error: Need more diamond tiler rings.");
            return;
        }

        zCamera_FrustumGridTileRingPartial *ring = &g_zCamera_FrustumGridTileRings[ringIndex];
        const int tileIndex = ring->count;
        if (tileIndex >= 30) {
            zError::ReportOld(0x200, kCameraSourceFile, cellErrorLine,
                              "Error: Need more diamond tiler cells per ring.");
            return;
        }

        ring->count = tileIndex + 1;
        zCamera_FrustumGridTilePartial *tile = &ring->tiles[tileIndex];
        tile->col = col;
        tile->row = row;
        tile->hasPosOffset = hasPosOffset;
        tile->posOffsetX = posOffsetX;
        tile->posOffsetZ = posOffsetZ;
        tile->clipMask = clipMask;
    }
}

namespace zClass_Camera {
    // Reimplements 0x449be0: zClass_Camera::gwCameraNew
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwCameraNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            ReportCameraError(0x1e8, "Null node pointer.");
            return 0;
        }

        node->classId = kZClassNodeCamera;
        zClass_CameraDataPartial *data = static_cast<zClass_CameraDataPartial *>(
            calloc(1, sizeof(zClass_CameraDataPartial)));
        node->classData = data;
        data->viewportWidth = 1.0f;
        data->viewportHeight = 1.0f;
        data->frustumVectorsDirty = 1;
        data->transformDirty = 1;
        data->localFrustumNormalsDirty = 1;
        data->variantOverrideEnabled = 0;
        zTag4::Clear(&data->variantTag);
        zClass_TypeList::Insert(8, node);
        return node;
    }

    // Reimplements 0x449c90: zClass_Camera::gwCameraAddChild
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraAddChild(zClass_NodePartial * parent,
                                                                  zClass_NodePartial * child) {
        if (parent == 0) {
            ReportCameraError(0x239, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            ReportCameraError(0x23a, "Null node pointer.");
            return 5;
        }

        return zClass_Class::AddChildGeneric(parent, child);
    }

    // Reimplements 0x449cd0: zClass_Camera::gwCameraRemoveChild
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraRemoveChild(zClass_NodePartial * parent,
                                                                     zClass_NodePartial * child) {
        if (parent == 0) {
            ReportCameraError(0x251, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            ReportCameraError(0x252, "Null node pointer.");
            return 5;
        }

        return zClass_Class::RemoveChildGeneric(parent, child);
    }

    // Reimplements 0x449d20: zClass_Camera::gwCameraSetFlagBit0
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetFlagBit0(zClass_NodePartial * node,
                                                                     int enabled) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(node, &data, 0x274, 0x275, 0x276);
        if (result != 0) {
            return result;
        }

        if (enabled != 0) {
            data->cameraFlags |= 0x01;
        } else {
            data->cameraFlags &= ~0x01;
        }

        return 0;
    }

    // Reimplements 0x449da0: zClass_Camera::SetTargetNode
    RECOIL_NOINLINE int RECOIL_FASTCALL SetTargetNode(zClass_NodePartial * target) {
        g_zClass_CameraTargetNode = target;
        return 0;
    }

    // Reimplements 0x449db0: zClass_Camera::SetActiveCamera
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL SetActiveCamera(zClass_NodePartial *
                                                                        camera) {
        g_zClass_CurrentCamera = camera;
        return camera;
    }

    // Reimplements 0x449dc0: zClass_Camera::SetObjectHseTestEnabled
    RECOIL_NOINLINE int RECOIL_FASTCALL SetObjectHseTestEnabled(int enabled) {
        g_zClass_ObjectHseTestEnabled = enabled;
        return 0;
    }

    // Reimplements 0x449dd0: zClass_Camera::gwCameraSetWorld
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetWorld(zClass_NodePartial * camera,
                                                                  zClass_NodePartial * world) {
        if (camera == 0) {
            ReportCameraError(0x2be, "Null node pointer.");
            return 5;
        }
        if (world == 0) {
            ReportCameraError(0x2bf, "Null node pointer.");
            return 5;
        }

        if (camera->classData == 0) {
            ReportCameraError(0x2c1, "Null class data pointer");
            return 5;
        }
        if (world->classData == 0) {
            ReportCameraError(0x2c2, "Null class data pointer");
            return 5;
        }

        if (camera->classId != kZClassNodeCamera) {
            zError::ReportOld(0x400, kCameraSourceFile, 0x2c4,
                              "Bad Class Found.\n Wanted (%d)\n Found (%d)", camera->classId,
                              kZClassNodeCamera);
            return 3;
        }
        if (world->classId != kZClassNodeWorld) {
            zError::ReportOld(0x400, kCameraSourceFile, 0x2c5,
                              "Bad Class Found.\n Wanted (%d)\n Found (%d)", world->classId,
                              kZClassNodeWorld);
            return 3;
        }

        static_cast<zClass_CameraDataPartial *>(camera->classData)->worldNode = world;
        return 0;
    }

    // Reimplements 0x449e80: zClass_Camera::gwCameraGetWorld
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwCameraGetWorld(zClass_NodePartial *
                                                                         camera) {
        return static_cast<zClass_CameraDataPartial *>(camera->classData)->worldNode;
    }

    // Reimplements 0x449e90: zClass_Camera::gwCameraSetWindow
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetWindow(zClass_NodePartial * camera,
                                                                   zClass_NodePartial * window) {
        static_cast<zClass_CameraDataPartial *>(camera->classData)->windowNode = window;
        return 0;
    }

    // Reimplements 0x449f50: zClass_Camera::ActivateChildren
    RECOIL_NOINLINE int RECOIL_FASTCALL ActivateChildren(zClass_NodePartial * camera,
                                                                  zClass_CameraDataPartial * data) {
        data->cameraFlags |= 0x04;
        if ((camera->flags & 0x01) == 0) {
            zClass_TypeList::Insert(7, camera);
            camera->flags |= 0x01;
        }
        camera->flags |= 0x02;

        for (int i = 0; i < camera->listCountB; ++i) {
            zClass_Node::PropagateTransformDirtyRecursive(camera->listB[i]);
        }

        return 0;
    }

    // Reimplements 0x449ea0: zClass_Camera::gwCameraSetPosition
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetPosition(zClass_NodePartial * camera,
                                                                     float x, float y, float z) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x3a7, 0x3a8, 0x3a9);
        if (result != 0) {
            return result;
        }

        data->transformDirty = 1;
        data->posOffset = zVec3_Make(x, y, z);
        data->cameraFlags &= ~0x02;
        if (camera->listCountA > 0) {
            ActivateChildren(camera, data);
        }

        return 0;
    }

    // Reimplements 0x449fb0: zClass_Camera::gwCameraTranslate
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraTranslate(zClass_NodePartial * camera,
                                                                   float dx, float dy, float dz) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x3df, 0x3e0, 0x3e1);
        if (result != 0) {
            return result;
        }

        data->posOffset.x += dx;
        data->posOffset.y += dy;
        data->posOffset.z += dz;
        data->transformDirty = 1;
        if (camera->listCountA > 0) {
            ActivateChildren(camera, data);
        }

        return 0;
    }

    // Reimplements 0x44a060: zClass_Camera::gwCameraGetPosition
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetPosition(
        zClass_NodePartial * camera, float *outX, float *outY, float *outZ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x414, 0x415, 0x416);
        if (result != 0) {
            return result;
        }

        *outX = data->posOffset.x;
        *outY = data->posOffset.y;
        *outZ = data->posOffset.z;
        return 0;
    }

    // Reimplements 0x44a0f0: zClass_Camera::gwCameraSetTarget
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetTarget(zClass_NodePartial * camera,
                                                                   float x, float y, float z) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x43c, 0x43d, 0x43e);
        if (result != 0) {
            return result;
        }

        *GetSelectedTargetVector(data) = zVec3_Make(x, y, z);
        if (camera->listCountA > 0) {
            ActivateChildren(camera, data);
        }

        return 0;
    }

    // Reimplements 0x44a1a0: zClass_Camera::gwCameraTranslateTarget
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraTranslateTarget(
        zClass_NodePartial * camera, float dx, float dy, float dz) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x46f, 0x470, 0x471);
        if (result != 0) {
            return result;
        }

        zVec3 *target = GetSelectedTargetVector(data);
        target->x += dx;
        target->y += dy;
        target->z += dz;
        if (camera->listCountA > 0) {
            ActivateChildren(camera, data);
        }

        return 0;
    }

    // Reimplements 0x44a250: zClass_Camera::gwCameraGetTarget
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetTarget(
        zClass_NodePartial * camera, float *outX, float *outY, float *outZ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x4a1, 0x4a2, 0x4a3);
        if (result != 0) {
            return result;
        }

        zVec3 *target = GetSelectedTargetVector(data);
        *outX = target->x;
        *outY = target->y;
        *outZ = target->z;
        return 0;
    }

    // Reimplements 0x44a2f0: zClass_Camera::gwCameraSetNearFarClip
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetNearFarClip(
        zClass_NodePartial * camera, float nearClip, float farClip) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x509, 0x50a, 0x50b);
        if (result != 0) {
            return result;
        }

        data->nearClip = nearClip;
        data->farClip = farClip;
        data->frustumVectorsDirty = 1;
        return 0;
    }

    // Reimplements 0x44a380: zClass_Camera::gwCameraGetNearFarClip
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetNearFarClip(
        zClass_NodePartial * camera, float *outNear, float *outFar) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x52f, 0x530, 0x531);
        if (result != 0) {
            return result;
        }

        *outNear = data->nearClip;
        *outFar = data->farClip;
        return 0;
    }

    // Reimplements 0x44a410: zClass_Camera::gwCameraSetViewport
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetViewport(
        zClass_NodePartial * camera, float viewportWidth, float viewportHeight) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x553, 0x554, 0x555);
        if (result != 0) {
            return result;
        }

        const float maxFov = 1.39600003f;
        data->viewportWidth = viewportWidth;
        data->viewportHeight = viewportHeight;
        data->fovX = data->frustumWidth / viewportWidth;
        data->fovY = data->frustumHeight / viewportHeight;
        if (data->fovX > maxFov) {
            data->fovX = maxFov;
        }
        if (data->fovY > maxFov) {
            data->fovY = maxFov;
        }

        const float halfFovX = data->fovX * 0.5f;
        const float halfFovY = data->fovY * 0.5f;
        const float tanHalfFovX = static_cast<float>(tan(static_cast<double>(halfFovX)));
        const float tanHalfFovY = static_cast<float>(tan(static_cast<double>(halfFovY)));

        data->localFrustumNormalsDirty = 1;
        data->frustumVectorsDirty = 1;
        data->frustumYaw = halfFovX;
        data->frustumPitch = halfFovY;
        data->viewportScaleX = 1.0f / tanHalfFovX;
        data->viewportScaleY = 1.0f / tanHalfFovY;
        return 0;
    }

    // Reimplements 0x44a580: zClass_Camera::gwCameraGetViewport
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetViewport(
        zClass_NodePartial * camera, float *outWidth, float *outHeight) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x58e, 0x58f, 0x590);
        if (result != 0) {
            return result;
        }

        *outWidth = data->viewportWidth;
        *outHeight = data->viewportHeight;
        return 0;
    }

    // Reimplements 0x44a760: zClass_Camera::gwCameraGetFOV
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetFOV(zClass_NodePartial * camera,
                                                                float *outFovX, float *outFovY) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x5e7, 0x5e8, 0x5e9);
        if (result != 0) {
            return result;
        }

        *outFovX = data->frustumWidth;
        *outFovY = data->frustumHeight;
        return 0;
    }

    // Reimplements 0x44a610: zClass_Camera::gwCameraSetFOV
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetFOV(zClass_NodePartial * camera,
                                                                float fovX, float fovY) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x5b2, 0x5b3, 0x5b4);
        if (result != 0) {
            return result;
        }

        const float normalizedFovX = fovX / data->viewportWidth;
        const float normalizedFovY = fovY / data->viewportHeight;
        const float halfFovX = normalizedFovX * 0.5f;
        const float halfFovY = normalizedFovY * 0.5f;
        const float tanHalfFovX = static_cast<float>(tan(static_cast<double>(halfFovX)));
        const float tanHalfFovY = static_cast<float>(tan(static_cast<double>(halfFovY)));

        data->localFrustumNormalsDirty = 1;
        data->frustumVectorsDirty = 1;
        data->frustumWidth = fovX;
        data->frustumHeight = fovY;
        data->fovX = normalizedFovX;
        data->fovY = normalizedFovY;
        data->frustumYaw = halfFovX;
        data->frustumPitch = halfFovY;
        data->viewportScaleX = 1.0f / tanHalfFovX;
        data->viewportScaleY = 1.0f / tanHalfFovY;
        return 0;
    }

    // Reimplements 0x44a7f0: zClass_Camera::gwCameraGetClipDistance
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetClipDistance(
        zClass_NodePartial * camera, float *outClipDistance) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x609, 0x60a, 0x60b);
        if (result != 0) {
            return result;
        }

        *outClipDistance = data->clipDistance;
        return 0;
    }

    // Reimplements 0x44a870: zClass_Camera::gwCameraSetClipDistance
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetClipDistance(
        zClass_NodePartial * camera, float clipDistance) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x62a, 0x62b, 0x62c);
        if (result != 0) {
            return result;
        }

        data->clipDistance = clipDistance;
        data->invClipDistanceSq = 1.0f / (clipDistance * clipDistance);
        return 0;
    }

    // Reimplements 0x44a910: zClass_Camera::gwCameraSetHorizon
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetHorizon(
        zClass_NodePartial * camera, zClass_NodePartial * horizonNode) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x64d, 0x64e, 0x64f);
        if (result != 0) {
            return result;
        }

        data->horizonNode = horizonNode;
        return 0;
    }

    // Reimplements 0x44a980: zClass_Camera::gwCameraSetHorizonXZ
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetHorizonXZ(
        zClass_NodePartial * camera, zClass_NodePartial * horizonXZNode) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x66e, 0x66f, 0x670);
        if (result != 0) {
            return result;
        }

        data->horizonXZNode = horizonXZNode;
        return 0;
    }

    // Reimplements 0x449ba0: zClass_Camera::SetViewDistance
    RECOIL_NOINLINE void RECOIL_FASTCALL SetViewDistance(int enableAutoClip,
                                                         float distance) {
        g_zClass_CameraAutoClipDistanceAdjustEnabled = enableAutoClip;
        if (distance == 0.0f) {
            g_zClass_CameraAutoClipDistanceThreshold = 0.04f;
        } else {
            g_zClass_CameraAutoClipDistanceThreshold = 1.0f / distance;
        }
    }

    // Reimplements 0x44c1b0: zClass_Camera::FastAngleXZ
    // (D:\Proj\GameZRecoil\zClass\Camera.c)
    RECOIL_NOINLINE float RECOIL_FASTCALL FastAngleXZ(zVec3 *point1, zVec3 *point2) {
        const int deltaX = static_cast<int>(point2->x - point1->x);
        const int deltaZ = static_cast<int>(point1->z - point2->z);

        const int absX = deltaX < 0 ? -deltaX : deltaX;
        const int absZ = deltaZ < 0 ? -deltaZ : deltaZ;
        const int denom = absX + absZ;

        float angle = 0.0f;
        if (denom != 0) {
            angle = static_cast<float>(deltaZ) / static_cast<float>(denom);
        }

        if (deltaX < 0) {
            return (2.0f - angle) * 1.57079601f;
        }

        if (deltaZ < 0) {
            angle -= -4.0f;
        }

        return angle * 1.57079601f;
    }

    // Reimplements 0x44c230: zClass_Camera::FindConvexHullXZ
    // (D:\Proj\GameZRecoil\zClass\Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL FindConvexHullXZ(zVec3 *points,
                                                                  int count) {
        int candidateIndex = 1;
        int selectedIndex = 0;

        if (count > 1) {
            float selectedZ = points[0].z;
            while (candidateIndex < count) {
                if (points[candidateIndex].z - selectedZ > 0.1) {
                    selectedIndex = candidateIndex;
                    selectedZ = points[candidateIndex].z;
                }
                ++candidateIndex;
            }
        }

        float previousAngle = 0.0f;
        points[count] = points[selectedIndex];

        if (count > 0) {
            zVec3 *hullPoint = points;
            int hullIndex = 0;
            int scanStart = 1;

            do {
                const zVec3 savedPoint = *hullPoint;
                *hullPoint = points[selectedIndex];
                selectedIndex = count;
                points[selectedIndex] = savedPoint;

                const float minAngle = previousAngle;
                previousAngle = 6.28318548f;

                if (scanStart <= count) {
                    zVec3 *candidate = hullPoint + 1;
                    int scanIndex = scanStart;
                    do {
                        const float angle = FastAngleXZ(hullPoint, candidate);
                        if (angle > minAngle && angle < previousAngle) {
                            previousAngle = angle;
                            selectedIndex = scanIndex;
                        }
                        ++scanIndex;
                        ++candidate;
                    } while (scanIndex <= count);
                }

                if (selectedIndex == count) {
                    return hullIndex + 1;
                }

                ++hullPoint;
                ++hullIndex;
                ++scanStart;
            } while (hullIndex < count);
        }

        zError::ReportOld(0x200, kCameraSourceFile, 0x1049,
                          "Returning from find_convex_hull_xz in unexpected line.");
        return 0;
    }

    // Reimplements 0x44c3c0: zClass_Camera::BuildFrustumGridTiles
    // (D:\Proj\GameZRecoil\zClass\Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildFrustumGridTiles(
        zClass_NodePartial *world, zClass_WorldDataPartial *worldData,
        zClass_CameraDataPartial *cameraData) {
        ClearFrustumGridTileRings();

        int originCol = 0;
        int originRow = 0;
        int result = zClass_World::WorldToGridCoordsClamped(
            world, &originCol, cameraData->cameraPos.x, cameraData->cameraPos.z, &originRow);
        if (result != 0) {
            return result;
        }

        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)&slotBuffer);
        BuildCameraFrustumFootprint(cameraData, 0x10ea);

        float minX;
        float maxX;
        float minZ;
        float maxZ;
        GetFrustumFootprintBounds(g_zCamera_FrustumFootprintPointCount, &minX, &maxX, &minZ,
                                  &maxZ);

        int minCol = 0;
        int minRow = 0;
        result = zClass_World::WorldToGridCoordsClamped(world, &minCol, minX, minZ, &minRow);
        if (result != 0) {
            zMath::MatStackPopPtr();
            return result;
        }

        int maxCol = 0;
        int maxRow = 0;
        result = zClass_World::WorldToGridCoordsClamped(world, &maxCol, maxX, maxZ, &maxRow);
        if (result != 0) {
            zMath::MatStackPopPtr();
            return result;
        }

        if (minRow > maxRow) {
            const int savedRow = minRow;
            minRow = maxRow;
            maxRow = savedRow;
        }

        if (minCol < 0) {
            minCol = 0;
        } else if (minCol >= worldData->areaGridColCount) {
            minCol = worldData->areaGridColCount - 1;
        }
        if (maxCol < 0) {
            maxCol = 0;
        } else if (maxCol >= worldData->areaGridColCount) {
            maxCol = worldData->areaGridColCount - 1;
        }
        if (minRow < 0) {
            minRow = 0;
        } else if (minRow >= worldData->areaGridRowCount) {
            minRow = worldData->areaGridRowCount - 1;
        }
        if (maxRow < 0) {
            maxRow = 0;
        } else if (maxRow >= worldData->areaGridRowCount) {
            maxRow = worldData->areaGridRowCount - 1;
        }

        const int areaIndex = worldData->areaGridRows[originRow][originCol].areaIndex;
        {
        for (int col = minCol; col <= maxCol; ++col) {
            {
            for (int row = minRow; row <= maxRow; ++row) {
                zWorldAreaPartial *area = &worldData->areaGridRows[row][col];
                if ((area->areaIndex & areaIndex) == 0) {
                    continue;
                }

                zVec3 center = {0};
                center.x = area->cellMinX + worldData->areaHalfSizeX;
                center.y = 0.0f;
                center.z = area->cellMinZ + worldData->areaHalfSizeZ;
                if (zClass_cls_di::FilterRegionsAgainstHexahedronFaces(
                        &center, worldData->areaCellRadiusBias) == 0) {
                    continue;
                }

                int clipMask = 0x3f;
                zVec3 *sphereCenter = &center;
                float bboxRadius = -worldData->areaCellRadiusBias;
                if ((area->areaFlags & 0x100) != 0) {
                    sphereCenter = &area->bboxCenter;
                    bboxRadius = area->bboxRadius;
                }

                if (zVideo_FrustumTestSphereClipMask(sphereCenter, &clipMask, bboxRadius) == 0) {
                    AddFrustumGridTile(col, row, originCol, originRow, clipMask, 0, 0.0f, 0.0f,
                                       0x11a4, 0x11aa);
                }
            }
            }
        }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x44c8e0: zClass_Camera::BuildFrustumGridTilesFromParams
    // (D:\Proj\GameZRecoil\zClass\Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildFrustumGridTilesFromParams(
        zClass_NodePartial *world, zClass_WorldDataPartial *worldData,
        zClass_CameraDataPartial *cameraData) {
        ClearFrustumGridTileRings();

        int originCol = 0;
        int originRow = 0;
        int originClampedCol = 0;
        int originClampedRow = 0;
        int originInsideBounds = 0;
        int result = zClass_World::WorldToGridCoordsClampedEx(
            world, &originCol, cameraData->cameraPos.x, cameraData->cameraPos.z, &originRow,
            &originClampedCol, &originClampedRow, &originInsideBounds);
        if (result != 0) {
            return result;
        }

        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)&slotBuffer);
        BuildCameraFrustumFootprint(cameraData, 0x1279);

        float minX;
        float maxX;
        float minZ;
        float maxZ;
        GetFrustumFootprintBounds(g_zCamera_FrustumFootprintPointCount, &minX, &maxX, &minZ,
                                  &maxZ);

        int minCol = 0;
        int minRow = 0;
        int minClampedCol = 0;
        int minClampedRow = 0;
        int minInsideBounds = 0;
        result = zClass_World::WorldToGridCoordsClampedEx(
            world, &minCol, minX, minZ, &minRow, &minClampedCol, &minClampedRow,
            &minInsideBounds);
        if (result != 0) {
            zMath::MatStackPopPtr();
            return result;
        }

        int maxCol = 0;
        int maxRow = 0;
        int maxClampedCol = 0;
        int maxClampedRow = 0;
        int maxInsideBounds = 0;
        result = zClass_World::WorldToGridCoordsClampedEx(
            world, &maxCol, maxX, maxZ, &maxRow, &maxClampedCol, &maxClampedRow,
            &maxInsideBounds);
        if (result != 0) {
            zMath::MatStackPopPtr();
            return result;
        }

        if (minClampedRow > maxClampedRow) {
            const int savedClampedRow = minClampedRow;
            minClampedRow = maxClampedRow;
            maxClampedRow = savedClampedRow;
        }
        if (minRow > maxRow) {
            const int savedRow = minRow;
            minRow = maxRow;
            maxRow = savedRow;
        }

        const int areaIndex =
            worldData->areaGridRows[originClampedRow][originClampedCol].areaIndex;

        {
        for (int col = minCol; col <= maxCol; ++col) {
            {
            for (int row = minRow; row <= maxRow; ++row) {
                int hasPosOffset = 0;
                int areaCol = col;
                int areaRow = row;

                if (areaCol < 0) {
                    hasPosOffset = 1;
                    areaCol = 0;
                } else if (areaCol >= worldData->areaGridColCount) {
                    hasPosOffset = 1;
                    areaCol = worldData->areaGridColCount - 1;
                }

                if (areaRow < 0) {
                    hasPosOffset = 1;
                    areaRow = 0;
                } else if (areaRow >= worldData->areaGridRowCount) {
                    hasPosOffset = 1;
                    areaRow = worldData->areaGridRowCount - 1;
                }

                float posOffsetX = 0.0f;
                float posOffsetZ = 0.0f;
                if (hasPosOffset != 0) {
                    posOffsetX =
                        static_cast<float>(col - areaCol) * worldData->areaCellSizeX;
                    posOffsetZ =
                        static_cast<float>(row - areaRow) * worldData->areaCellSizeZ;
                }

                zWorldAreaPartial *area = &worldData->areaGridRows[areaRow][areaCol];
                if ((area->areaIndex & areaIndex) == 0) {
                    continue;
                }

                zVec3 center = {0};
                center.x = area->cellMinX + worldData->areaHalfSizeX + posOffsetX;
                center.y = 0.0f;
                center.z = area->cellMinZ + worldData->areaHalfSizeZ + posOffsetZ;
                if (zClass_cls_di::FilterRegionsAgainstHexahedronFaces(
                        &center, worldData->areaCellRadiusBias) == 0) {
                    continue;
                }

                int clipMask = 0x3f;
                int frustumVisible = 0;
                if (hasPosOffset == 0) {
                    zVec3 *sphereCenter = &center;
                    float bboxRadius = -worldData->areaCellRadiusBias;
                    if ((area->areaFlags & 0x100) != 0) {
                        sphereCenter = &area->bboxCenter;
                        bboxRadius = area->bboxRadius;
                    }
                    frustumVisible =
                        zVideo_FrustumTestSphereClipMask(sphereCenter, &clipMask, bboxRadius);
                }

                if (frustumVisible == 0) {
                    AddFrustumGridTile(areaCol, areaRow, originCol, originRow, clipMask,
                                       hasPosOffset, posOffsetX, posOffsetZ, 0x1351, 0x1357);
                }
            }
            }
        }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x44ce70: zClass_Camera::RenderFrustumGridTiles
    // (D:\Proj\GameZRecoil\zClass\Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RenderFrustumGridTiles(
        zClass_NodePartial *world, zClass_NodePartial *camera,
        zClass_CameraDataPartial *cameraData) {
        zClass_WorldDataPartial *worldData =
            static_cast<zClass_WorldDataPartial *>(world->classData);
        int result = 0;

        if (worldData->clampQueriesToBounds != 0) {
            result = BuildFrustumGridTilesFromParams(world, worldData, cameraData);
        } else {
            result = BuildFrustumGridTiles(world, worldData, cameraData);
        }
        if (result != 0) {
            return result;
        }

        const int fogWasEnabled = zModel_Fog_IsEnabled();
        float fogDistanceStart = 0.0f;
        if (fogWasEnabled != 0) {
            fogDistanceStart = zModel_Fog_GetDistanceStart();
        }

        g_zClass_RenderFrustumGridTileIndex = 0;
        int cameraAtBasePos = 1;
        {
        for (int ringIndex = 0; ringIndex < 50; ++ringIndex) {
            g_zClass_RenderFrustumGridTileIndex = ringIndex;
            zCamera_FrustumGridTileRingPartial *ring = &g_zCamera_FrustumGridTileRings[ringIndex];
            {
            for (int tileIndex = 0; tileIndex < ring->count; ++tileIndex) {
                zCamera_FrustumGridTilePartial *tile = &ring->tiles[tileIndex];
                zWorldAreaPartial *area = &worldData->areaGridRows[tile->row][tile->col];
                zVec3 center = area->bboxCenter;

                if (tile->hasPosOffset != 0) {
                    zVec3 posOffset = {-tile->posOffsetX, 0.0f, -tile->posOffsetZ};
                    UpdateImpl(camera, &posOffset);
                    cameraAtBasePos = 0;
                } else if (cameraAtBasePos == 0) {
                    gwCameraUpdate(camera);
                    cameraAtBasePos = 1;
                }

                if (g_zClass_ObjectHseTestEnabled != 0 && ringIndex > 0 &&
                    zScene::TestProjectedSphereVisible(&center, area->bboxRadius) == 0) {
                    continue;
                }

                for (int lightIndex = 0; lightIndex < worldData->lightCount;
                     ++lightIndex) {
                    zClass_NodePartial *lightNode = worldData->lightNodes[lightIndex];
                    if ((lightNode->flags & 0x04) == 0) {
                        continue;
                    }

                    zClass_LightDataPartial *lightData = worldData->lightDataList[lightIndex];
                    if (lightData->isDirectionalMode == 0 || lightData->enabled == 0) {
                        lightData->lightSubMode = 1;
                        continue;
                    }

                    const float dx = center.x - lightData->worldPosScratch.x;
                    const float dy = center.y - lightData->worldPosScratch.y;
                    const float dz = center.z - lightData->worldPosScratch.z;
                    const float range = lightData->range2 + area->bboxRadius;
                    const float distanceSq = dx * dx + dy * dy + dz * dz;
                    lightData->lightSubMode = range * range < distanceSq ? 0 : 1;
                }

                if (fogWasEnabled != 0) {
                    const float dx = center.x - cameraData->cameraPos.x;
                    const float dy = center.y - cameraData->cameraPos.y;
                    const float dz = center.z - cameraData->cameraPos.z;
                    float distanceSq = dx * dx + dy * dy + dz * dz;
                    int bits = 0;
                    memcpy(&bits, &distanceSq, sizeof(bits));
                    bits = (bits >> 1) + 0x1fc00000;
                    float distance = 0.0f;
                    memcpy(&distance, &bits, sizeof(distance));
                    distance += area->bboxRadius * 1.10000002f;
                    zModel_Fog_SetEnabled(distance < fogDistanceStart ? 0 : 1);
                }

                *gModel_ClipMaskStackTop = tile->clipMask;
                if (tile->hasPosOffset == 0) {
                    for (int childIndex = 0; childIndex < area->childCount;
                         ++childIndex) {
                        zClass_Class::gwNodeRenderDispatch(area->childList[childIndex],
                                                           area->childCount);
                    }
                } else {
                    for (int childIndex = 0; childIndex < area->childCount;
                         ++childIndex) {
                        zClass_NodePartial *child = area->childList[childIndex];
                        if (strstr(child->name, "VAP_statics") != 0) {
                            zClass_Class::gwNodeRenderDispatch(child, area->childCount);
                        }
                    }
                }
            }
            }
        }
        }

        if (cameraAtBasePos == 0) {
            gwCameraUpdate(camera);
        }
        {
        for (int lightIndex = 0; lightIndex < worldData->lightCount; ++lightIndex) {
            worldData->lightDataList[lightIndex]->lightSubMode = 1;
        }
        }
        if (fogWasEnabled != 0) {
            zModel_Fog_SetEnabled(fogWasEnabled);
        }
        return result;
    }

    // Reimplements 0x44d200: zClass_Camera::RenderOverlayNodes
    // (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL RenderOverlayNodes(zClass_NodePartial *world) {
        *gModel_ClipMaskStackTop = 0x3f;
        for (int i = 0; i < world->listCountB; ++i) {
            zClass_Class::gwNodeRenderDispatch(world->listB[i], 2);
        }
    }

    // Reimplements 0x44d240: zClass_Camera::RenderWorld
    // (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL RenderWorld(zClass_NodePartial *world,
                                                     zClass_NodePartial *camera,
                                                     zClass_CameraDataPartial *cameraData) {
        RenderFrustumGridTiles(world, camera, cameraData);
        RenderOverlayNodes(world);
    }

    // Reimplements 0x44d260: zClass_Camera::gwCameraSetVariantTagOverride
    // (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    gwCameraSetVariantTagOverride(zClass_NodePartial *camera, zTag4Partial *variantTag) {
        zClass_CameraDataPartial *data = 0;
        const int validateResult =
            ValidateCameraNode(camera, &data, 0x1527, 0x1528, 0x1529);
        if (validateResult != 0) {
            return validateResult;
        }

        int validVariantTag = 1;
        for (int i = 0; i < variantTag->count; ++i) {
            if (variantTag->tags[i] == 0xff) {
                validVariantTag = 0;
            }
        }

        if (variantTag->count > 0 && validVariantTag != 0) {
            data->variantOverrideEnabled = 1;
            data->variantTag = *variantTag;
        }
        return 0;
    }

    // Reimplements 0x44d3a0: zClass_Camera::RenderScene
    // (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RenderScene(
        zClass_NodePartial *camera, int updateFxPass3Local) {
        const int queuedLensFlareSampleCount =
            zRndr_LensFlare_GetQueuedSampleCount();
        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)&slotBuffer);

        g_zVideo_pActiveViewContext =
            static_cast<zClass_CameraDataPartial *>(camera->classData);
        zClass_NodePartial *world = gwCameraGetWorld(camera);
        zClass_CameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
        zClass_WindowDataPartial *windowData =
            static_cast<zClass_WindowDataPartial *>(viewContext->windowNode->classData);

        if (g_zClass_CameraAutoClipDistanceAdjustEnabled != 0) {
            if (g_FrameDeltaTimeSec <= g_zClass_CameraAutoClipDistanceThreshold) {
                g_zClass_CameraAutoClipDistanceScale += g_zClass_CameraAutoClipDistanceStep;
            } else {
                g_zClass_CameraAutoClipDistanceScale -= g_zClass_CameraAutoClipDistanceStep;
            }

            if (g_zClass_CameraAutoClipDistanceScale > 1.0f) {
                g_zClass_CameraAutoClipDistanceScale = 1.0f;
            } else if (g_zClass_CameraAutoClipDistanceScale <
                       g_zClass_CameraAutoClipDistanceMinScale) {
                g_zClass_CameraAutoClipDistanceScale =
                    g_zClass_CameraAutoClipDistanceMinScale;
            }

            gwCameraSetClipDistance(camera, g_zClass_CameraAutoClipDistanceScale);
        }

        zClass_World::InitLightPointInPolygonXZ(world);
        zVideo::ReturnSuccessStub();
        gwCameraUpdate(camera);
        SyncViewContextPositions();
        zVideo_SetActiveViewContext(g_zVideo_pActiveViewContext);
        zClass_World::UpdateAllLights(world);
        zClass_World::UpdateAllSounds(world);

        g_zClass_LodDistanceStateStackTop = 0;
        if (zClass_TypeList::CountNodes(8) > 1) {
            zRndr::SpanOcclusionResetFrame();
            if ((windowData->clearPolyIndexFlags & 0x80000000) != 0) {
                const int clearPolyCount =
                    windowData->clearPolyIndexFlags & 0x7fffffff;
                for (int i = 0; i < clearPolyCount; ++i) {
                    zClass_WindowClearPoly *poly = &windowData->clearPolys[i];
                    if ((poly->vertCount & 0x80000000) != 0) {
                        zRndr::SpanOcclusionAddPolygon(poly->vertices,
                                                       poly->vertCount & 0x7fffffff);
                    }
                }
            }
        }
        zRndr::SpanOcclusionBuildColumnHeadTable();

        const int variantFilterEnabled = g_Variant_FilterEnabled;
        viewContext = g_zVideo_pActiveViewContext;
        if (variantFilterEnabled != 0) {
            if (viewContext->variantOverrideEnabled != 0 && variantFilterEnabled == 1) {
                g_Variant_CurrentTag = viewContext->variantTag;
            } else {
                PlayerProbeSampleCandidateBuffer pickCandidates = {0};
                g_Variant_FilterEnabled = 0;
                zClass_cls_di::FindBestPickCandidateBelowPoint(world, &viewContext->cameraPos,
                                                               &pickCandidates);
                g_Variant_FilterEnabled = variantFilterEnabled;

                if (pickCandidates.candidateCount <= 0) {
                    zTag4::Clear(&g_zVideo_pActiveViewContext->variantTag);
                    g_Variant_CurrentTag = g_zVideo_pActiveViewContext->variantTag;
                } else if (pickCandidates.entries[0].variantTag.count > 0) {
                    g_zVideo_pActiveViewContext->variantTag =
                        pickCandidates.entries[0].variantTag;
                    g_Variant_CurrentTag = pickCandidates.entries[0].variantTag;
                }
            }
            g_zVideo_ActiveViewVariantTag = g_zVideo_pActiveViewContext->variantTag;
        }

        RenderWorld(world, camera, g_zVideo_pActiveViewContext);
        zMath::MatStackPopPtr();
        zRndr_FlushTransparentQueue();
        if (updateFxPass3Local != 0) {
            zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);
        }
        zRndr_FlushOverwriteQueue();
        zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList(queuedLensFlareSampleCount);
        zRndr_LensFlare_DrawVisibleSamples();
        zRndr_FlushTransparentQueue();
        zRndr_OverlayRect_FlushSw();

        return 0;
    }

    // Reimplements 0x44abf0: zClass_Camera::BuildWorldTransform
    // (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildWorldTransform(
        zClass_NodePartial *camera, zClass_CameraDataPartial *data, zVec3 *posOffset) {
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(camera, 1);

        zMat4x3 *matrix = zMath_Mat_GetCurrent();
        if (posOffset != 0) {
            matrix->posX += posOffset->x;
            matrix->posY += posOffset->y;
            matrix->posZ += posOffset->z;
        }

        data->cameraPos.x = matrix->posX;
        data->cameraPos.y = matrix->posY;
        data->cameraPos.z = matrix->posZ;
        data->forwardDir.x = NegateFloatSignBit(matrix->zx);
        data->forwardDir.y = NegateFloatSignBit(matrix->zy);
        data->forwardDir.z = NegateFloatSignBit(matrix->zz);

        memcpy(data->worldTransform, matrix, sizeof(zMat4x3));
        zMath_Mat_ExtractEulerAngles(matrix, &data->eulerAngles);
        zMath::MatLoadIdentity();
        zMath_Camera_StageInverseRotation(reinterpret_cast<zMat4x3 *>(data->worldTransform));

        if ((data->cameraFlags & 0x01) != 0) {
            zVec3 listenerVelocity = {0};
            if (g_FrameDeltaTimeSec != 0.0f) {
                listenerVelocity.x =
                    (data->worldTransform[9] - g_zSnd_PreviousListenerPos.x) /
                    g_FrameDeltaTimeSec;
                listenerVelocity.y =
                    (data->worldTransform[10] - g_zSnd_PreviousListenerPos.y) /
                    g_FrameDeltaTimeSec;
                listenerVelocity.z =
                    (data->worldTransform[11] - g_zSnd_PreviousListenerPos.z) /
                    g_FrameDeltaTimeSec;

                const float listenerSpeed =
                    sqrt(listenerVelocity.x * listenerVelocity.x +
                              listenerVelocity.y * listenerVelocity.y +
                              listenerVelocity.z * listenerVelocity.z);
                if (zSnd_GetSpeedOfSoundMps() <= listenerSpeed) {
                    listenerVelocity = zVec3_Make(0.0f, 0.0f, 0.0f);
                }
            }

            g_zSnd_PreviousListenerPos.x = data->worldTransform[9];
            g_zSnd_PreviousListenerPos.y = data->worldTransform[10];
            g_zSnd_PreviousListenerPos.z = data->worldTransform[11];
            zSnd_UpdateListenerState(
                reinterpret_cast<zSndListenerState *>(data->worldTransform), &listenerVelocity);
        }

        return 0;
    }

    // Reimplements 0x44aa30: zClass_Camera::UpdateImpl
    // (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL UpdateImpl(zClass_NodePartial *camera,
                                                            zVec3 *posOffset) {
        zClass_CameraDataPartial *data =
            static_cast<zClass_CameraDataPartial *>(camera->classData);

        BuildWorldTransform(camera, data, posOffset);

        data->transformDirty = 1;
        if (data->localFrustumNormalsDirty != 0) {
            data->localFrustumNormalsDirty = 0;
            zVideo_UpdateProjectionStateFromCameraData(data);
            data->transformDirty = 1;
        }

        data->transformDirty = 0;
        zClipAlt_BuildFrustumPlanes(data);

        if (data->frustumVectorsDirty != 0) {
            const float farClip = data->farClip;
            const float halfWidth = static_cast<float>(tan(data->fovX * 0.5f)) * farClip;
            const float halfHeight = static_cast<float>(tan(data->fovY * 0.5f)) * farClip;
            const float negHalfHeight = -halfHeight;
            const float negFarClip = -farClip;
            const float negHalfWidth = -halfWidth;

            data->frustumVectorsDirty = 0;
            data->frustumOrigin = zVec3_Make(0.0f, 0.0f, 0.0f);
            data->frustumCorners[0] = zVec3_Make(halfWidth, negHalfHeight, negFarClip);
            data->frustumCorners[1] = zVec3_Make(negHalfWidth, negHalfHeight, negFarClip);
            data->frustumCorners[2] = zVec3_Make(halfWidth, halfHeight, negFarClip);
            data->frustumCorners[3] = zVec3_Make(negHalfWidth, halfHeight, negFarClip);
        }

        data->nearClipCenter.x = data->cameraPos.x + data->forwardDir.x * data->nearClip;
        data->nearClipCenter.y = data->cameraPos.y + data->forwardDir.y * data->nearClip;
        data->nearClipCenter.z = data->cameraPos.z + data->forwardDir.z * data->nearClip;

        data->farClipCenter.x = data->cameraPos.x + data->forwardDir.x * data->farClip;
        data->farClipCenter.y = data->cameraPos.y + data->forwardDir.y * data->farClip;
        data->farClipCenter.z = data->cameraPos.z + data->forwardDir.z * data->farClip;

        return 0;
    }

    // Reimplements 0x44a9f0: zClass_Camera::gwCameraUpdate
    // (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraUpdate(zClass_NodePartial *camera) {
        if (camera == 0) {
            ReportCameraError(0x75c, "Null node pointer.");
            return 5;
        }

        if (camera->classData == 0) {
            ReportCameraError(0x75d, "Null class data pointer");
            return 5;
        }

        return UpdateImpl(camera, 0);
    }

    // Reimplements 0x44d320: zClass_Camera::SyncViewContextPositions (GameZRecoil/zClass/Camera.c)
    RECOIL_NOINLINE void RECOIL_CDECL SyncViewContextPositions() {
        zClass_CameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
        int updatedAnyNode = 0;

        if (viewContext->horizonNode != 0) {
            zClass_Object3D::gwObject3DSetPosition(viewContext->horizonNode,
                                                   viewContext->cameraPos.x,
                                                   viewContext->cameraPos.y,
                                                   viewContext->cameraPos.z);
            viewContext = g_zVideo_pActiveViewContext;
            updatedAnyNode = 1;
        }

        if (viewContext->horizonXZNode != 0) {
            float horizonX;
            float preservedY;
            float horizonZ;
            zClass_Object3D::gwObject3DGetPosition(viewContext->horizonXZNode, &horizonX,
                                                   &preservedY, &horizonZ);
            viewContext = g_zVideo_pActiveViewContext;
            zClass_Object3D::gwObject3DSetPosition(viewContext->horizonXZNode,
                                                   viewContext->cameraPos.x, preservedY,
                                                   viewContext->cameraPos.z);
            updatedAnyNode = 1;
        }

        if (updatedAnyNode != 0) {
            zClass_Class::gwNodeUpdateAll();
        }
    }

    // Reimplements 0x44ada0: zClass_Camera::RenderTraverse
    // (D:\Proj\GameZRecoil\zClass\Camera.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(
        zClass_NodePartial *node, int siblingCountHint) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        zClass_CameraDataPartial *data = static_cast<zClass_CameraDataPartial *>(node->classData);
        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
            if ((node->boundsFlags & 0x04) != 0 || g_zClass_RenderBoundsContextActive != 0 ||
                (node->flags & 0x00080000) == 0) {
                zBBoxCorners corners = {0};
                zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);
                BBox::CornersToBoundingSphere(&corners, zClass_NodeViewSphereCenter(node),
                                              zClass_NodeViewSphereRadius(node));
                if ((node->flags & 0x00080000) != 0) {
                    node->boundsFlags &= ~0x04;
                }
            }
            result = zVideo_FrustumTestSphereClipMask(zClass_NodeViewSphereCenter(node), &clipMask,
                                                      *zClass_NodeViewSphereRadius(node));
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                clipMask &= ~0x20;
            }
        }

        if (result == 0) {
            const zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            node->flags |= 0x80000000;
            zMath::MatStackPushAndCloneParent(data->worldTransform);
            zMath::MatApplyLocalTRS(&data->targetOrEuler, &data->posOffset, &unitScale);
            if (g_zClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_zClass_RenderBoundsContextActive = 1;
            }
            if (gModel_RenderFn != 0) {
                gModel_RenderFn(node, clipMask);
            }
            if (node->listCountB > 0) {
                ++gModel_ClipMaskStackTop;
                *gModel_ClipMaskStackTop = clipMask;
                for (int i = 0; i < node->listCountB; ++i) {
                    zClass_Class::gwNodeRenderDispatch(node->listB[i], node->listCountB);
                }
                --gModel_ClipMaskStackTop;
            }
            zMath::MatStackPopPtr();
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        return result;
    }
}
