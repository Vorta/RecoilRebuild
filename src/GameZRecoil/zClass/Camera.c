#include "zclass.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRndr/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * Reimplements data 0x4ddd14: g_zClass_CameraAutoClipDistanceAdjustEnabled.
 * Purpose: enable adaptive camera clip-distance changes during scene render.
 */
int g_zClass_CameraAutoClipDistanceAdjustEnabled = 0;
/**
 * Reimplements data 0x4ddd18: g_zClass_CameraAutoClipDistanceThreshold.
 * Purpose: frame-time threshold used by adaptive camera clip-distance scaling.
 */
float g_zClass_CameraAutoClipDistanceThreshold = 0.04f;
/**
 * Reimplements data 0x4ddd1c: g_zClass_CameraAutoClipDistanceScale.
 * Purpose: current adaptive camera clip-distance scale.
 */
float g_zClass_CameraAutoClipDistanceScale = 1.0f;
/**
 * Reimplements data 0x4ddd20: g_zClass_CameraAutoClipDistanceStep.
 * Purpose: per-frame adaptive camera clip-distance scale step.
 */
float g_zClass_CameraAutoClipDistanceStep = 0.05f;
/**
 * Reimplements data 0x4ddd24: g_zClass_CameraAutoClipDistanceMinScale.
 * Purpose: minimum adaptive camera clip-distance scale clamp.
 */
float g_zClass_CameraAutoClipDistanceMinScale = 0.6f;
/**
 * Reimplements data 0x4ddd10: g_zClass_ObjectHseTestEnabled.
 * Purpose: enable projected object visibility testing during tiled render.
 */
int g_zClass_ObjectHseTestEnabled = 1;
/**
 * Reimplements data 0x4ddd34: g_zClass_CurrentCamera.
 * Purpose: track the current active camera node.
 */
zClass_NodePartial *g_zClass_CurrentCamera = 0;
/**
 * Reimplements data 0x4ddd38: g_zClass_CameraTargetNode.
 * Purpose: track the current camera target node.
 */
zClass_NodePartial *g_zClass_CameraTargetNode = 0;
/**
 * Reimplements data 0x4f4988: g_Camera_PrevListenerPosX.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position X storage.
 */
float g_Camera_PrevListenerPosX = 0.0f;
/**
 * Reimplements data 0x4f498c: g_Camera_PrevListenerPosY.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position Y storage.
 */
float g_Camera_PrevListenerPosY = 0.0f;
/**
 * Reimplements data 0x4f4990: g_Camera_PrevListenerPosZ.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position Z storage.
 */
float g_Camera_PrevListenerPosZ = 0.0f;
/**
 * Reimplements data 0x4dddbc: g_zClass_FindConvexHullUnexpectedReturnMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x37].
 * Purpose: report the unexpected convex-hull exit path during frustum-grid
 * footprint construction.
 */
char g_zClass_FindConvexHullUnexpectedReturnMsg[0x37] =
    "Returning from find_convex_hull_xz in unexpected line.";
/**
 * Reimplements data 0x4dddf4: g_zClass_DiamondTilerNeedMoreRingsMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x26].
 * Purpose: report overflow of camera frustum-grid diamond ring buckets.
 */
char g_zClass_DiamondTilerNeedMoreRingsMsg[0x26] =
    "Error: Need more diamond tiler rings.";
/**
 * Reimplements data 0x4dde1c: g_zClass_DiamondTilerNeedMoreCellsPerRingMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x2f].
 * Purpose: report overflow of a camera frustum-grid diamond ring's cell list.
 */
char g_zClass_DiamondTilerNeedMoreCellsPerRingMsg[0x2f] =
    "Error: Need more diamond tiler cells per ring.";
/**
 * Reimplements data 0x4dde4c:
 * g_zClass_LineErrorPointInPolygonInitCameraFrustumFmt.
 * BN data inventory declares writable Camera.c diagnostic format char[0x53].
 * Purpose: format the camera frustum-footprint mesh-face filter failure
 * diagnostic with the legacy source file and line.
 */
char g_zClass_LineErrorPointInPolygonInitCameraFrustumFmt[0x53] =
    "%s: Line %d: ERROR from gModDIPointInPolygonInit() for camera "
    "frustrum footprint.\n";
/**
 * Reimplements data 0x4ddea0: g_zClass_VapStaticsNodeName.
 * BN data inventory declares the shared writable zClass VAP statics node-name
 * literal char[0xc], referenced by Camera.c render filtering and cls_world.c
 * virtual-area partition creation.
 * Purpose: name generated virtual-area statics nodes and identify them during
 * offset-tile camera rendering.
 */
char g_zClass_VapStaticsNodeName[0x0c] = "VAP_statics";
/**
 * Reimplements data 0x56cc40: g_zCamera_FrustumFootprintPoints.
 * Purpose: cache the frustum origin plus four corner points used by camera
 * grid-tile construction; BN bounds this zero-initialized array to five zVec3
 * entries, with the adjacent zero gaps outside this symbol.
 */
zVec3 g_zCamera_FrustumFootprintPoints[5] = {0};
/**
 * Reimplements data 0x56ccac: g_zCamera_FrustumFootprintPointCount.
 * Purpose: count active frustum footprint points for grid-tile construction.
 */
int g_zCamera_FrustumFootprintPointCount = 0;
/**
 * Reimplements data 0x56ccc0: g_zCamera_FrustumGridTileRings.
 * Purpose: cache up to 50 diamond-ring buckets of camera frustum grid tiles
 * for the scene render pass.
 */
zCamera_FrustumGridTileRingPartial g_zCamera_FrustumGridTileRings[50] = {0};
}

namespace {
    const int kZClassNodeCamera = 1;
    const int kZClassNodeWorld = 2;

    /*
     * BN diagnostic string data used by 0x44a760 gwCameraGetFOV:
     * 0x4dd9d4 null-node text, 0x4dd9bc null-class-data text,
     * 0x4ddd44 Camera.c source path, and 0x4ddd68 bad-class format.
     * The generic diagnostic text/format strings are pooled across zClass
     * callers, so their owner is shared rather than Camera.c-only.
     */
    const char kCameraSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\Camera.c";

    /**
     * Original static helper observed in camera validation callers
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: emit the legacy zError camera-source diagnostic for null or
     * invalid camera node state.
     */
    void ReportCameraError(
        int sourceLine,
        const char *message
    ) {
        zError::ReportOld(
            0x400,
            kCameraSourceFile,
            sourceLine,
            message
        );
    }

    /**
     * Original static helper observed in camera setter/getter callers
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: validate a camera node, recover its camera data pointer, and
     * preserve the caller-specific legacy source-line diagnostics.
     */
    int ValidateCameraNode(
        zClass_NodePartial * node,
        zClass_CameraDataPartial * *outData,
        int nullLine,
        int dataLine,
        int classLine
    ) {
        if (node == 0) {
            ReportCameraError(
                nullLine,
                "Null node pointer."
            );
            return 5;
        }

        if (node->classData == 0) {
            ReportCameraError(
                dataLine,
                "Null class data pointer"
            );
            return 5;
        }

        if (node->classId != kZClassNodeCamera) {
            zError::ReportOld(
                0x400,
                kCameraSourceFile,
                classLine,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeCamera
            );
            return 3;
        }

        *outData = (zClass_CameraDataPartial *)(node->classData);
        return 0;
    }

    /**
     * Original static helper observed in camera target callers
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: select the active target vector field based on the camera flag
     * that switches between world target and Euler/target storage.
     */
    zVec3 *GetSelectedTargetVector(zClass_CameraDataPartial * data) {
        return (data->cameraFlags & 0x02) != 0 ? &data->worldTarget : &data->targetOrEuler;
    }

    /**
     * Original static helper observed in caller 0x44abf0
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: flip a float sign bit without changing the remaining bit pattern
     * while deriving the camera forward vector.
     */
    float NegateFloatSignBit(float value) {
        unsigned int bits = 0;
        memcpy(
            &bits,
            &value,
            sizeof(bits)
        );
        bits ^= 0x80000000u;
        memcpy(
            &value,
            &bits,
            sizeof(value)
        );
        return value;
    }

    /**
     * Original static helper observed in frustum-grid tiling callers
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: compute the absolute integer delta used for diamond ring
     * indexing.
     */
    int AbsInt(int value) {
        return value < 0 ? -value : value;
    }

    /**
     * Original static helper observed in callers 0x44c3c0 and 0x44c8e0
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: reset all cached frustum-grid tile ring counts before rebuilding
     * visible world-cell candidates.
     */
    void ClearFrustumGridTileRings() {
        {
            for (int ringIndex = 0; ringIndex < 50; ++ringIndex) {
                g_zCamera_FrustumGridTileRings[ringIndex].count = 0;
            }
        }
    }

    /**
     * Original static helper observed in camera frustum footprint callers
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: copy the camera frustum footprint points and transform them by
     * the active matrix when the matrix is not identity.
     */
    void CopyCurrentCameraFrustumFootprint(
        zClass_CameraDataPartial * data,
        int pointCount
    ) {
        memcpy(
            g_zCamera_FrustumFootprintPoints,
            &data->frustumOrigin,
            pointCount * sizeof(zVec3)
        );

        if (zMath_Mat_IsCurrentIdentity() == 0) {
            zMath::MatTransformPointBatchInPlace(
                g_zCamera_FrustumFootprintPoints,
                pointCount
            );
        }
    }

    /**
     * Original static helper observed in callers 0x44c3c0 and 0x44c8e0
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: build and optionally convex-hull/filter the camera frustum
     * footprint used by world-cell culling.
     */
    int BuildCameraFrustumFootprint(
        zClass_CameraDataPartial * data,
        int filterErrorLine
    ) {
        zMath::MatLoadIdentity();
        zMath::MatTranslate(
            data->cameraPos.x,
            data->cameraPos.y,
            data->cameraPos.z
        );
        zMath::MatRotateY(data->eulerAngles.y);

        int pointCount;
        if (fabs(data->eulerAngles.x) < 0.174533 && fabs(data->eulerAngles.z) < 0.174533) {
            pointCount = 3;
        } else {
            pointCount = 5;
            zMath::MatRotateX(data->eulerAngles.x);
            zMath::MatRotateZ(data->eulerAngles.z);
        }

        g_zCamera_FrustumFootprintPointCount = pointCount;
        CopyCurrentCameraFrustumFootprint(
            data,
            pointCount
        );

        if (pointCount > 3) {
            pointCount =
                zClass_Camera::FindConvexHullXZ(
                    g_zCamera_FrustumFootprintPoints,
                    pointCount
                );
            g_zCamera_FrustumFootprintPointCount = pointCount;
        }

        if (zClass_cls_di::FilterRegionsAgainstMeshFaces(
                g_zCamera_FrustumFootprintPoints,
                pointCount
            ) == 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_zClass_LineErrorPointInPolygonInitCameraFrustumFmt,
                kCameraSourceFile,
                filterErrorLine
            );
            zError::EmitDebugBuffer(1);
        }

        return pointCount;
    }

    /**
     * Original static helper observed in callers 0x44c3c0 and 0x44c8e0
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: compute X/Z bounds for the cached frustum footprint points.
     */
    void GetFrustumFootprintBounds(
        int pointCount,
        float *minX,
        float *maxX,
        float *minZ,
        float *maxZ
    ) {
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

    /**
     * Original static helper observed in callers 0x44c3c0 and 0x44c8e0
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: append one world-cell tile to the appropriate frustum-grid
     * diamond ring while preserving legacy overflow diagnostics.
     */
    void AddFrustumGridTile(
        int col,
        int row,
        int originCol,
        int originRow,
        int clipMask,
        int hasPosOffset,
        float posOffsetX,
        float posOffsetZ,
        int cellErrorLine,
        int ringErrorLine
    ) {
        const int ringIndex = AbsInt(col - originCol) + AbsInt(row - originRow);
        if (ringIndex >= 50) {
            zError::ReportOld(
                0x200,
                kCameraSourceFile,
                ringErrorLine,
                g_zClass_DiamondTilerNeedMoreRingsMsg
            );
            return;
        }

        zCamera_FrustumGridTileRingPartial *ring = &g_zCamera_FrustumGridTileRings[ringIndex];
        const int tileIndex = ring->count;
        if (tileIndex >= 30) {
            zError::ReportOld(
                0x200,
                kCameraSourceFile,
                cellErrorLine,
                g_zClass_DiamondTilerNeedMoreCellsPerRingMsg
            );
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
    /**
     * Reimplements 0x449be0: zClass_Camera::gwCameraNew.
     * Purpose: allocate and initialize a camera node and its class data.
     */
    zClass_NodePartial *gwCameraNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            ReportCameraError(
                0x1e8,
                "Null node pointer."
            );
            return 0;
        }

        node->classId = kZClassNodeCamera;
        zClass_CameraDataPartial *data =
            (zClass_CameraDataPartial *)(calloc(
                1,
                sizeof(zClass_CameraDataPartial)
        ));
        node->classData = data;
        data->targetOrEuler.x = 0.0f;
        data->targetOrEuler.y = 0.0f;
        data->targetOrEuler.z = 0.0f;
        data->posOffset.x = 0.0f;
        data->posOffset.y = 0.0f;
        data->posOffset.z = 0.0f;
        data->viewportWidth = 1.0f;
        data->viewportHeight = 1.0f;
        data->frustumVectorsDirty = 1;
        data->transformDirty = 1;
        data->localFrustumNormalsDirty = 1;
        data->variantOverrideEnabled = 0;
        zTag4::Clear(&data->variantTag);
        zClass_TypeList::Insert(
            8,
            node
        );
        return node;
    }

    /**
     * Reimplements 0x449c90: zClass_Camera::gwCameraAddChild.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: validate camera parent/child inputs before using the generic
     * zClass listA/listB child-link routine.
     */
    int __fastcall gwCameraAddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            ReportCameraError(
                0x239,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            ReportCameraError(
                0x23a,
                "Null node pointer."
            );
            return 5;
        }

        return zClass_Class::AddChildGeneric(
            parent,
            child
        );
    }

    /**
     * Reimplements 0x449cd0: zClass_Camera::gwCameraRemoveChild.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: validate camera parent/child inputs before using the generic
     * zClass listA/listB child-unlink routine.
     */
    int __fastcall gwCameraRemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kCameraSourceFile,
                0x251,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kCameraSourceFile,
                0x252,
                "Null node pointer."
            );
            return 5;
        }

        return zClass_Class::RemoveChildGeneric(
            parent,
            child
        );
    }

    /**
     * Reimplements 0x449d20: zClass_Camera::gwCameraSetFlagBit0.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Camera data flag bit 0 gates the zSound listener-state update in
     * BuildWorldTransform.
     * Purpose: validate a camera node and set or clear camera flag bit 0.
     */
    int __fastcall gwCameraSetFlagBit0(
        zClass_NodePartial * node,
        int enabled
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            node,
            &data,
            0x274,
            0x275,
            0x276
        );
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

    /**
     * Reimplements 0x449da0: zClass_Camera::SetTargetNode.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: store the current global camera target node and report success.
     */
    int __fastcall SetTargetNode(zClass_NodePartial * target) {
        g_zClass_CameraTargetNode = target;
        return 0;
    }

    /**
     * Reimplements 0x449db0: zClass_Camera::SetActiveCamera.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: store the current global camera node and return it.
     */
    zClass_NodePartial *__fastcall SetActiveCamera(
        zClass_NodePartial * camera
    ) {
        g_zClass_CurrentCamera = camera;
        return camera;
    }

    /**
     * Reimplements 0x449dc0: zClass_Camera::SetObjectHseTestEnabled.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: store the object HSE test enable flag and report success.
     */
    int __fastcall SetObjectHseTestEnabled(int enabled) {
        g_zClass_ObjectHseTestEnabled = enabled;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x449dd0: zClass_Camera::gwCameraSetWorld.
     * Purpose: validate camera and world nodes before assigning the camera world.
     */
    gwCameraSetWorld(
        zClass_NodePartial * camera,
        zClass_NodePartial * world
    ) {
        if (camera == 0) {
            ReportCameraError(
                0x2be,
                "Null node pointer."
            );
            return 5;
        }
        if (world == 0) {
            ReportCameraError(
                0x2bf,
                "Null node pointer."
            );
            return 5;
        }

        if (camera->classData == 0) {
            ReportCameraError(
                0x2c1,
                "Null class data pointer"
            );
            return 5;
        }
        if (world->classData == 0) {
            ReportCameraError(
                0x2c2,
                "Null class data pointer"
            );
            return 5;
        }

        if (camera->classId != kZClassNodeCamera) {
            zError::ReportOld(
                0x400,
                kCameraSourceFile,
                0x2c4,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                camera->classId,
                kZClassNodeCamera
            );
            return 3;
        }
        if (world->classId != kZClassNodeWorld) {
            zError::ReportOld(
                0x400,
                kCameraSourceFile,
                0x2c5,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                world->classId,
                kZClassNodeWorld
            );
            return 3;
        }

        ((zClass_CameraDataPartial *)(camera->classData))->worldNode = world;
        return 0;
    }

    /**
     * Reimplements 0x449e80: zClass_Camera::gwCameraGetWorld.
     * Purpose: return the world node currently assigned to the camera.
     */
    zClass_NodePartial *__fastcall gwCameraGetWorld(
        zClass_NodePartial * camera
    ) {
        return ((zClass_CameraDataPartial *)(camera->classData))->worldNode;
    }

    int __fastcall
    /**
     * Reimplements 0x449e90: zClass_Camera::gwCameraSetWindow.
     * Purpose: assign the window node used by the camera view context.
     */
    gwCameraSetWindow(
        zClass_NodePartial * camera,
        zClass_NodePartial * window
    ) {
        ((zClass_CameraDataPartial *)(camera->classData))->windowNode = window;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x449f50: zClass_Camera::ActivateChildren.
     * Purpose: mark camera children dirty and register the active camera node.
     */
    ActivateChildren(
        zClass_NodePartial * camera,
        zClass_CameraDataPartial * data
    ) {
        data->cameraFlags |= 0x04;
        if ((camera->flags & 0x01) == 0) {
            zClass_TypeList::Insert(
                7,
                camera
            );
            camera->flags |= 0x01;
        }
        camera->flags |= 0x02;

        for (int i = 0; i < camera->listCountB; ++i) {
            zClass_Node::PropagateTransformDirtyRecursive(camera->listB[i]);
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x449ea0: zClass_Camera::gwCameraSetPosition.
     * Purpose: set the camera position offset and dirty dependent transforms.
     */
    gwCameraSetPosition(
        zClass_NodePartial * camera,
        float x,
        float y,
        float z
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x3a7,
            0x3a8,
            0x3a9
        );
        if (result != 0) {
            return result;
        }

        data->transformDirty = 1;
        data->posOffset = zVec3_Make(
            x,
            y,
            z
        );
        data->cameraFlags &= ~0x02;
        if (camera->listCountA > 0) {
            ActivateChildren(
                camera,
                data
            );
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x449fb0: zClass_Camera::gwCameraTranslate.
     * Purpose: translate the camera position offset and dirty dependent transforms.
     */
    gwCameraTranslate(
        zClass_NodePartial * camera,
        float dx,
        float dy,
        float dz
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x3df,
            0x3e0,
            0x3e1
        );
        if (result != 0) {
            return result;
        }

        data->posOffset.x += dx;
        data->posOffset.y += dy;
        data->posOffset.z += dz;
        data->transformDirty = 1;
        if (camera->listCountA > 0) {
            ActivateChildren(
                camera,
                data
            );
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a060: zClass_Camera::gwCameraGetPosition.
     * Purpose: return the camera position offset components.
     */
    gwCameraGetPosition(
        zClass_NodePartial * camera,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x414,
            0x415,
            0x416
        );
        if (result != 0) {
            return result;
        }

        *outX = data->posOffset.x;
        *outY = data->posOffset.y;
        *outZ = data->posOffset.z;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a0f0: zClass_Camera::gwCameraSetTarget.
     * Purpose: set the selected camera target vector and update children.
     */
    gwCameraSetTarget(
        zClass_NodePartial * camera,
        float x,
        float y,
        float z
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x43c,
            0x43d,
            0x43e
        );
        if (result != 0) {
            return result;
        }

        *GetSelectedTargetVector(data) = zVec3_Make(
            x,
            y,
            z
        );
        if (camera->listCountA > 0) {
            ActivateChildren(
                camera,
                data
            );
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a1a0: zClass_Camera::gwCameraTranslateTarget.
     * Purpose: translate the selected camera target vector and update children.
     */
    gwCameraTranslateTarget(
        zClass_NodePartial * camera,
        float dx,
        float dy,
        float dz
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x46f,
            0x470,
            0x471
        );
        if (result != 0) {
            return result;
        }

        zVec3 *target = GetSelectedTargetVector(data);
        target->x += dx;
        target->y += dy;
        target->z += dz;
        if (camera->listCountA > 0) {
            ActivateChildren(
                camera,
                data
            );
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a250: zClass_Camera::gwCameraGetTarget.
     * Purpose: return the selected camera target vector components.
     */
    gwCameraGetTarget(
        zClass_NodePartial * camera,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x4a1,
            0x4a2,
            0x4a3
        );
        if (result != 0) {
            return result;
        }

        zVec3 *target = GetSelectedTargetVector(data);
        *outX = target->x;
        *outY = target->y;
        *outZ = target->z;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a2f0: zClass_Camera::gwCameraSetNearFarClip.
     * Purpose: store near/far clip distances and dirty frustum vectors.
     */
    gwCameraSetNearFarClip(
        zClass_NodePartial * camera,
        float nearClip,
        float farClip
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x509,
            0x50a,
            0x50b
        );
        if (result != 0) {
            return result;
        }

        data->nearClip = nearClip;
        data->farClip = farClip;
        data->frustumVectorsDirty = 1;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a380: zClass_Camera::gwCameraGetNearFarClip.
     * Purpose: return the camera near/far clip distances.
     */
    gwCameraGetNearFarClip(
        zClass_NodePartial * camera,
        float *outNear,
        float *outFar
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x52f,
            0x530,
            0x531
        );
        if (result != 0) {
            return result;
        }

        *outNear = data->nearClip;
        *outFar = data->farClip;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a410: zClass_Camera::gwCameraSetViewport.
     * Purpose: update viewport dimensions and derived frustum scale values.
     */
    gwCameraSetViewport(
        zClass_NodePartial * camera,
        float viewportWidth,
        float viewportHeight
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x553,
            0x554,
            0x555
        );
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
        const float tanHalfFovX = (float)(tan((double)(halfFovX)));
        const float tanHalfFovY = (float)(tan((double)(halfFovY)));

        data->localFrustumNormalsDirty = 1;
        data->frustumVectorsDirty = 1;
        data->frustumYaw = halfFovX;
        data->frustumPitch = halfFovY;
        data->viewportScaleX = 1.0f / tanHalfFovX;
        data->viewportScaleY = 1.0f / tanHalfFovY;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a580: zClass_Camera::gwCameraGetViewport.
     * Purpose: return the camera viewport dimensions.
     */
    gwCameraGetViewport(
        zClass_NodePartial * camera,
        float *outWidth,
        float *outHeight
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x58e,
            0x58f,
            0x590
        );
        if (result != 0) {
            return result;
        }

        *outWidth = data->viewportWidth;
        *outHeight = data->viewportHeight;
        return 0;
    }

    /**
     * Reimplements 0x44a760: zClass_Camera::gwCameraGetFOV.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Touched diagnostic string data: 0x4dd9d4, 0x4dd9bc, 0x4ddd44,
     * and 0x4ddd68.
     * Purpose: return the camera frustum FOV pair after legacy camera-node
     * validation diagnostics.
     */
    int __fastcall gwCameraGetFOV(
        zClass_NodePartial * camera,
        float *outFovX,
        float *outFovY
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x5e7,
            0x5e8,
            0x5e9
        );
        if (result != 0) {
            return result;
        }

        *outFovX = data->frustumWidth;
        *outFovY = data->frustumHeight;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a610: zClass_Camera::gwCameraSetFOV.
     * Purpose: set camera frustum dimensions and derived projection scale values.
     */
    gwCameraSetFOV(
        zClass_NodePartial * camera,
        float fovX,
        float fovY
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x5b2,
            0x5b3,
            0x5b4
        );
        if (result != 0) {
            return result;
        }

        const float normalizedFovX = fovX / data->viewportWidth;
        const float normalizedFovY = fovY / data->viewportHeight;
        const float halfFovX = normalizedFovX * 0.5f;
        const float halfFovY = normalizedFovY * 0.5f;
        const float tanHalfFovX = (float)(tan((double)(halfFovX)));
        const float tanHalfFovY = (float)(tan((double)(halfFovY)));

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

    int __fastcall
    /**
     * Reimplements 0x44a7f0: zClass_Camera::gwCameraGetClipDistance.
     * Purpose: return the camera clip distance.
     */
    gwCameraGetClipDistance(
        zClass_NodePartial * camera,
        float *outClipDistance
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x609,
            0x60a,
            0x60b
        );
        if (result != 0) {
            return result;
        }

        *outClipDistance = data->clipDistance;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a870: zClass_Camera::gwCameraSetClipDistance.
     * Purpose: store the camera clip distance and inverse squared distance.
     */
    gwCameraSetClipDistance(
        zClass_NodePartial * camera,
        float clipDistance
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x62a,
            0x62b,
            0x62c
        );
        if (result != 0) {
            return result;
        }

        data->clipDistance = clipDistance;
        data->invClipDistanceSq = 1.0f / (clipDistance * clipDistance);
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a910: zClass_Camera::gwCameraSetHorizon.
     * Purpose: assign the horizon node that follows the camera position.
     */
    gwCameraSetHorizon(
        zClass_NodePartial * camera,
        zClass_NodePartial * horizonNode
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x64d,
            0x64e,
            0x64f
        );
        if (result != 0) {
            return result;
        }

        data->horizonNode = horizonNode;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44a980: zClass_Camera::gwCameraSetHorizonXZ.
     * Purpose: assign the horizon node that follows camera X/Z position.
     */
    gwCameraSetHorizonXZ(
        zClass_NodePartial * camera,
        zClass_NodePartial * horizonXZNode
    ) {
        zClass_CameraDataPartial *data = 0;
        const int result = ValidateCameraNode(
            camera,
            &data,
            0x66e,
            0x66f,
            0x670
        );
        if (result != 0) {
            return result;
        }

        data->horizonXZNode = horizonXZNode;
        return 0;
    }

    /**
     * Reimplements 0x449ba0: zClass_Camera::SetViewDistance.
     * Purpose: configure adaptive camera clip-distance scaling from view distance.
     */
    void __fastcall SetViewDistance(
        int enableAutoClip,
        float distance
    ) {
        g_zClass_CameraAutoClipDistanceAdjustEnabled = enableAutoClip;
        if (distance == 0.0f) {
            g_zClass_CameraAutoClipDistanceThreshold = 0.04f;
        } else {
            g_zClass_CameraAutoClipDistanceThreshold = 1.0f / distance;
        }
    }

    /**
     * Reimplements 0x44c1b0: zClass_Camera::FastAngleXZ.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: approximate the XZ-plane angle between two points.
     */
    float __fastcall FastAngleXZ(
        zVec3 * point1,
        zVec3 * point2
    ) {
        const int deltaX = (int)(point2->x - point1->x);
        const int deltaZ = (int)(point1->z - point2->z);

        const int absX = deltaX < 0 ? -deltaX : deltaX;
        const int absZ = deltaZ < 0 ? -deltaZ : deltaZ;
        const int denom = absX + absZ;

        float angle = 0.0f;
        if (denom != 0) {
            angle = (float)(deltaZ) / (float)(denom);
        }

        if (deltaX < 0) {
            return (2.0f - angle) * 1.57079601f;
        }

        if (deltaZ < 0) {
            angle -= -4.0f;
        }

        return angle * 1.57079601f;
    }

    /**
     * Reimplements 0x44c230: zClass_Camera::FindConvexHullXZ.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: build the XZ convex hull ordering for frustum footprint points.
     */
    int __fastcall FindConvexHullXZ(
        zVec3 * points,
        int count
    ) {
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
                        const float angle = FastAngleXZ(
                            hullPoint,
                            candidate
                        );
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

        zError::ReportOld(
            0x200,
            kCameraSourceFile,
            0x1049,
            g_zClass_FindConvexHullUnexpectedReturnMsg
        );
        return 0;
    }

    /**
     * Reimplements 0x44c3c0: zClass_Camera::BuildFrustumGridTiles
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: build clamped in-world frustum grid rings from the active
     * camera footprint.
     */
    int __fastcall BuildFrustumGridTiles(
        zClass_NodePartial * world,
        zClass_WorldDataPartial * worldData,
        zClass_CameraDataPartial * cameraData
    ) {
        ClearFrustumGridTileRings();

        int originCol = 0;
        int originRow = 0;
        int result = zClass_World::WorldToGridCoordsClamped(
            world,
            &originCol,
            cameraData->cameraPos.x,
            cameraData->cameraPos.z,
            &originRow
        );
        if (result != 0) {
            return result;
        }

        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)&slotBuffer);
        BuildCameraFrustumFootprint(
            cameraData,
            0x10ea
        );

        float minX;
        float maxX;
        float minZ;
        float maxZ;
        GetFrustumFootprintBounds(
            g_zCamera_FrustumFootprintPointCount,
            &minX,
            &maxX,
            &minZ,
            &maxZ
        );

        int minCol = 0;
        int minRow = 0;
        result = zClass_World::WorldToGridCoordsClamped(
            world,
            &minCol,
            minX,
            minZ,
            &minRow
        );
        if (result != 0) {
            zMath::MatStackPopPtr();
            return result;
        }

        int maxCol = 0;
        int maxRow = 0;
        result = zClass_World::WorldToGridCoordsClamped(
            world,
            &maxCol,
            maxX,
            maxZ,
            &maxRow
        );
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
                                &center,
                                worldData->areaCellRadiusBias
                            ) == 0) {
                            continue;
                        }

                        int clipMask = 0x3f;
                        zVec3 *sphereCenter = &center;
                        float bboxRadius = -worldData->areaCellRadiusBias;
                        if ((area->areaFlags & 0x100) != 0) {
                            sphereCenter = &area->bboxCenter;
                            bboxRadius = area->bboxRadius;
                        }

                        if (zVideo_FrustumTestSphereClipMask(sphereCenter, &clipMask, bboxRadius) ==
                            0) {
                            AddFrustumGridTile(
                                col,
                                row,
                                originCol,
                                originRow,
                                clipMask,
                                0,
                                0.0f,
                                0.0f,
                                0x11a4,
                                0x11aa
                            );
                        }
                    }
                }
            }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * Reimplements 0x44c8e0: zClass_Camera::BuildFrustumGridTilesFromParams
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: build frustum grid rings while preserving raw out-of-bounds
     * grid offsets for wrapped/clamped world positions.
     */
    int __fastcall BuildFrustumGridTilesFromParams(
        zClass_NodePartial * world,
        zClass_WorldDataPartial * worldData,
        zClass_CameraDataPartial * cameraData
    ) {
        ClearFrustumGridTileRings();

        int originCol = 0;
        int originRow = 0;
        int originClampedCol = 0;
        int originClampedRow = 0;
        int originInsideBounds = 0;
        int result = zClass_World::WorldToGridCoordsClampedEx(
            world,
            &originCol,
            cameraData->cameraPos.x,
            cameraData->cameraPos.z,
            &originRow,
            &originClampedCol,
            &originClampedRow,
            &originInsideBounds
        );
        if (result != 0) {
            return result;
        }

        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)&slotBuffer);
        BuildCameraFrustumFootprint(
            cameraData,
            0x1279
        );

        float minX;
        float maxX;
        float minZ;
        float maxZ;
        GetFrustumFootprintBounds(
            g_zCamera_FrustumFootprintPointCount,
            &minX,
            &maxX,
            &minZ,
            &maxZ
        );

        int minCol = 0;
        int minRow = 0;
        int minClampedCol = 0;
        int minClampedRow = 0;
        int minInsideBounds = 0;
        result = zClass_World::WorldToGridCoordsClampedEx(
            world,
            &minCol,
            minX,
            minZ,
            &minRow,
            &minClampedCol,
            &minClampedRow,
            &minInsideBounds
        );
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
            world,
            &maxCol,
            maxX,
            maxZ,
            &maxRow,
            &maxClampedCol,
            &maxClampedRow,
            &maxInsideBounds
        );
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

        const int areaIndex = worldData->areaGridRows[originClampedRow][originClampedCol].areaIndex;

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
                            posOffsetX = (float)(col - areaCol) * worldData->areaCellSizeX;
                            posOffsetZ = (float)(row - areaRow) * worldData->areaCellSizeZ;
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
                                &center,
                                worldData->areaCellRadiusBias
                            ) == 0) {
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
                            frustumVisible = zVideo_FrustumTestSphereClipMask(
                                sphereCenter,
                                &clipMask,
                                bboxRadius
                            );
                        }

                        if (frustumVisible == 0) {
                            AddFrustumGridTile(
                                areaCol,
                                areaRow,
                                originCol,
                                originRow,
                                clipMask,
                                hasPosOffset,
                                posOffsetX,
                                posOffsetZ,
                                0x1351,
                                0x1357
                            );
                        }
                    }
                }
            }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * Reimplements 0x44ce70: zClass_Camera::RenderFrustumGridTiles.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: render world grid tiles selected by the camera frustum.
     */
    int __fastcall RenderFrustumGridTiles(
        zClass_NodePartial * world,
        zClass_NodePartial * camera,
        zClass_CameraDataPartial * cameraData
    ) {
        zClass_WorldDataPartial *worldData = (zClass_WorldDataPartial *)(world->classData);
        int result = 0;

        if (worldData->clampQueriesToBounds != 0) {
            result = BuildFrustumGridTilesFromParams(
                world,
                worldData,
                cameraData
            );
        } else {
            result = BuildFrustumGridTiles(
                world,
                worldData,
                cameraData
            );
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
            int ringIndex = 0;
            while (ringIndex < 50) {
                g_zClass_RenderFrustumGridTileIndex = ringIndex;
                zCamera_FrustumGridTileRingPartial *ring =
                    &g_zCamera_FrustumGridTileRings[ringIndex];
                {
                    for (int tileIndex = 0; tileIndex < ring->count; ++tileIndex) {
                        zCamera_FrustumGridTilePartial *tile = &ring->tiles[tileIndex];
                        zWorldAreaPartial *area = &worldData->areaGridRows[tile->row][tile->col];
                        zVec3 center = area->bboxCenter;

                        if (tile->hasPosOffset != 0) {
                            zVec3 posOffset = {-tile->posOffsetX, 0.0f, -tile->posOffsetZ};
                            UpdateImpl(
                                camera,
                                &posOffset
                            );
                            cameraAtBasePos = 0;
                        } else if (cameraAtBasePos == 0) {
                            gwCameraUpdate(camera);
                            cameraAtBasePos = 1;
                        }

                        if (g_zClass_ObjectHseTestEnabled != 0 && ringIndex > 0 &&
                            zScene::TestProjectedSphereVisible(
                                &center,
                                area->bboxRadius
                            ) == 0) {
                            continue;
                        }

                        for (int lightIndex = 0; lightIndex < worldData->lightCount; ++lightIndex) {
                            zClass_NodePartial *lightNode = worldData->lightNodes[lightIndex];
                            if ((lightNode->flags & 0x04) == 0) {
                                continue;
                            }

                            zClass_LightDataPartial *lightData =
                                worldData->lightDataList[lightIndex];
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
                            memcpy(
                                &bits,
                                &distanceSq,
                                sizeof(bits)
                            );
                            bits = (bits >> 1) + 0x1fc00000;
                            float distance = 0.0f;
                            memcpy(
                                &distance,
                                &bits,
                                sizeof(distance)
                            );
                            distance += area->bboxRadius * 1.10000002f;
                            zModel_Fog_SetEnabled(distance < fogDistanceStart ? 0 : 1);
                        }

                        *gModel_ClipMaskStackTop = tile->clipMask;
                        if (tile->hasPosOffset == 0) {
                            for (int childIndex = 0; childIndex < area->childCount; ++childIndex) {
                                zClass_Class::gwNodeRenderDispatch(
                                    area->childList[childIndex],
                                    area->childCount
                                );
                            }
                        } else {
                            for (int childIndex = 0; childIndex < area->childCount; ++childIndex) {
                                zClass_NodePartial *child = area->childList[childIndex];
                                if (strstr(
                                    child->name,
                                    g_zClass_VapStaticsNodeName
                                ) != 0) {
                                    zClass_Class::gwNodeRenderDispatch(
                                        child,
                                        area->childCount
                                    );
                                }
                            }
                        }
                    }
                }
                ++ringIndex;
                g_zClass_RenderFrustumGridTileIndex = ringIndex;
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

    /**
     * Reimplements 0x44d200: zClass_Camera::RenderOverlayNodes.
     * Source: GameZRecoil/zClass/Camera.c.
     * Purpose: render overlay child nodes from the world node.
     */
    void __fastcall RenderOverlayNodes(zClass_NodePartial * world) {
        *gModel_ClipMaskStackTop = 0x3f;
        for (int i = 0; i < world->listCountB; ++i) {
            zClass_Class::gwNodeRenderDispatch(
                world->listB[i],
                2
            );
        }
    }

    /**
     * Reimplements 0x44d240: zClass_Camera::RenderWorld.
     * Source: GameZRecoil/zClass/Camera.c.
     * Purpose: render frustum grid tiles and overlay nodes for the world.
     */
    void __fastcall RenderWorld(
        zClass_NodePartial * world,
        zClass_NodePartial * camera,
        zClass_CameraDataPartial * cameraData
    ) {
        RenderFrustumGridTiles(
            world,
            camera,
            cameraData
        );
        RenderOverlayNodes(world);
    }

    int __fastcall
    /**
     * Reimplements 0x44d260: zClass_Camera::gwCameraSetVariantTagOverride.
     * Source: GameZRecoil/zClass/Camera.c.
     * Purpose: validate and store the camera variant tag override.
     */
    gwCameraSetVariantTagOverride(
        zClass_NodePartial * camera,
        zTag4Partial * variantTag
    ) {
        zClass_CameraDataPartial *data = 0;
        const int validateResult = ValidateCameraNode(
            camera,
            &data,
            0x1527,
            0x1528,
            0x1529
        );
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

    int __fastcall
    /**
     * Reimplements 0x44d3a0: zClass_Camera::RenderScene.
     * Source: GameZRecoil/zClass/Camera.c.
     * Purpose: update camera scene state and render the active world.
     */
    RenderScene(
        zClass_NodePartial * camera,
        int updateFxPass3Local
    ) {
        const int queuedLensFlareSampleCount = zRndr_LensFlare_GetQueuedSampleCount();
        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)&slotBuffer);

        g_zVideo_pActiveViewContext = (zClass_CameraDataPartial *)(camera->classData);
        zClass_NodePartial *world = gwCameraGetWorld(camera);
        zClass_CameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
        zClass_WindowDataPartial *windowData =
            (zClass_WindowDataPartial *)(viewContext->windowNode->classData);

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
                g_zClass_CameraAutoClipDistanceScale = g_zClass_CameraAutoClipDistanceMinScale;
            }

            gwCameraSetClipDistance(
                camera,
                g_zClass_CameraAutoClipDistanceScale
            );
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
                const int clearPolyCount = windowData->clearPolyIndexFlags & 0x7fffffff;
                for (int i = 0; i < clearPolyCount; ++i) {
                    zClass_WindowClearPoly *poly = &windowData->clearPolys[i];
                    if ((poly->vertCount & 0x80000000) != 0) {
                        zRndr::SpanOcclusionAddPolygon(
                            poly->vertices,
                            poly->vertCount & 0x7fffffff
                        );
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
                zClass_cls_di::FindBestPickCandidateBelowPoint(
                    world,
                    &viewContext->cameraPos,
                    &pickCandidates
                );
                g_Variant_FilterEnabled = variantFilterEnabled;

                if (pickCandidates.candidateCount <= 0) {
                    zTag4::Clear(&g_zVideo_pActiveViewContext->variantTag);
                    g_Variant_CurrentTag = g_zVideo_pActiveViewContext->variantTag;
                } else if (pickCandidates.entries[0].variantTag.count > 0) {
                    g_zVideo_pActiveViewContext->variantTag = pickCandidates.entries[0].variantTag;
                    g_Variant_CurrentTag = pickCandidates.entries[0].variantTag;
                }
            }
            g_zVideo_ActiveViewVariantTag = g_zVideo_pActiveViewContext->variantTag;
        }

        RenderWorld(
            world,
            camera,
            g_zVideo_pActiveViewContext
        );
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

    /**
     * Reimplements 0x44abf0: zClass_Camera::BuildWorldTransform.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: build the camera world transform and update the zSound
     * listener bridge previous-position state.
     */
    int __fastcall BuildWorldTransform(
        zClass_NodePartial * camera,
        zClass_CameraDataPartial * data,
        zVec3 * posOffset
    ) {
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(
            camera,
            1
        );

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

        memcpy(
            data->worldTransform,
            matrix,
            sizeof(zMat4x3)
        );
        zMath_Mat_ExtractEulerAngles(
            matrix,
            &data->eulerAngles
        );
        zMath::MatLoadIdentity();
        zMath_Camera_StageInverseRotation((zMat4x3 *)(data->worldTransform));

        if ((data->cameraFlags & 0x01) != 0) {
            /*
             * g_zSnd_PreviousListenerPos is the camera/listener bridge state
             * for the previous camera position, consumed here before pushing
             * the current listener state into zSound.
             */
            zVec3 listenerVelocity = {0};
            if (g_FrameDeltaTimeSec != 0.0f) {
                listenerVelocity.x =
                    (data->worldTransform[9] - g_zSnd_PreviousListenerPos.x) / g_FrameDeltaTimeSec;
                listenerVelocity.y =
                    (data->worldTransform[10] - g_zSnd_PreviousListenerPos.y) / g_FrameDeltaTimeSec;
                listenerVelocity.z =
                    (data->worldTransform[11] - g_zSnd_PreviousListenerPos.z) / g_FrameDeltaTimeSec;

                const float listenerSpeed = sqrt(
                    listenerVelocity.x * listenerVelocity.x +
                    listenerVelocity.y * listenerVelocity.y +
                    listenerVelocity.z * listenerVelocity.z
                );
                if (zSnd_GetSpeedOfSoundMps() <= listenerSpeed) {
                    listenerVelocity = zVec3_Make(
                        0.0f,
                        0.0f,
                        0.0f
                    );
                }
            }

            g_zSnd_PreviousListenerPos.x = data->worldTransform[9];
            g_zSnd_PreviousListenerPos.y = data->worldTransform[10];
            g_zSnd_PreviousListenerPos.z = data->worldTransform[11];
            zSnd_UpdateListenerState(
                (zSndListenerState *)(data->worldTransform),
                &listenerVelocity
            );
        }

        return 0;
    }

    /**
     * Reimplements 0x44aa30: zClass_Camera::UpdateImpl.
     * Source: GameZRecoil/zClass/Camera.c.
     * Purpose: rebuild camera transforms, frustum planes, and clip centers.
     */
    int __fastcall UpdateImpl(
        zClass_NodePartial * camera,
        zVec3 * posOffset
    ) {
        zClass_CameraDataPartial *data = (zClass_CameraDataPartial *)(camera->classData);

        BuildWorldTransform(
            camera,
            data,
            posOffset
        );

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
            const float halfWidth = (float)(tan(data->fovX * 0.5f)) * farClip;
            const float halfHeight = (float)(tan(data->fovY * 0.5f)) * farClip;
            const float negHalfHeight = -halfHeight;
            const float negFarClip = -farClip;
            const float negHalfWidth = -halfWidth;

            data->frustumVectorsDirty = 0;
            data->frustumOrigin = zVec3_Make(
                0.0f,
                0.0f,
                0.0f
            );
            data->frustumCorners[0] = zVec3_Make(
                halfWidth,
                negHalfHeight,
                negFarClip
            );
            data->frustumCorners[1] = zVec3_Make(
                negHalfWidth,
                negHalfHeight,
                negFarClip
            );
            data->frustumCorners[2] = zVec3_Make(
                halfWidth,
                halfHeight,
                negFarClip
            );
            data->frustumCorners[3] = zVec3_Make(
                negHalfWidth,
                halfHeight,
                negFarClip
            );
        }

        data->nearClipCenter.x = data->cameraPos.x + data->forwardDir.x * data->nearClip;
        data->nearClipCenter.y = data->cameraPos.y + data->forwardDir.y * data->nearClip;
        data->nearClipCenter.z = data->cameraPos.z + data->forwardDir.z * data->nearClip;

        data->farClipCenter.x = data->cameraPos.x + data->forwardDir.x * data->farClip;
        data->farClipCenter.y = data->cameraPos.y + data->forwardDir.y * data->farClip;
        data->farClipCenter.z = data->cameraPos.z + data->forwardDir.z * data->farClip;

        return 0;
    }

    /**
     * Reimplements 0x44a9f0: zClass_Camera::gwCameraUpdate.
     * Source: GameZRecoil/zClass/Camera.c.
     * Purpose: validate the camera node and run the camera update implementation.
     */
    int __fastcall gwCameraUpdate(zClass_NodePartial * camera) {
        if (camera == 0) {
            ReportCameraError(
                0x75c,
                "Null node pointer."
            );
            return 5;
        }

        if (camera->classData == 0) {
            ReportCameraError(
                0x75d,
                "Null class data pointer"
            );
            return 5;
        }

        return UpdateImpl(
            camera,
            0
        );
    }

    /**
     * Reimplements 0x44d320: zClass_Camera::SyncViewContextPositions.
     * Source: GameZRecoil/zClass/Camera.c.
     * Purpose: synchronize horizon helper nodes with the active view context.
     */
    void SyncViewContextPositions() {
        zClass_CameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
        int updatedAnyNode = 0;

        if (viewContext->horizonNode != 0) {
            zClass_Object3D::gwObject3DSetPosition(
                viewContext->horizonNode,
                viewContext->cameraPos.x,
                viewContext->cameraPos.y,
                viewContext->cameraPos.z
            );
            viewContext = g_zVideo_pActiveViewContext;
            updatedAnyNode = 1;
        }

        if (viewContext->horizonXZNode != 0) {
            float horizonX;
            float preservedY;
            float horizonZ;
            zClass_Object3D::gwObject3DGetPosition(
                viewContext->horizonXZNode,
                &horizonX,
                &preservedY,
                &horizonZ
            );
            viewContext = g_zVideo_pActiveViewContext;
            zClass_Object3D::gwObject3DSetPosition(
                viewContext->horizonXZNode,
                viewContext->cameraPos.x,
                preservedY,
                viewContext->cameraPos.z
            );
            updatedAnyNode = 1;
        }

        if (updatedAnyNode != 0) {
            zClass_Class::gwNodeUpdateAll();
        }
    }

    int __fastcall
    /**
     * Reimplements 0x44ada0: zClass_Camera::RenderTraverse.
     * Source: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: frustum-test and render a camera node traversal branch.
     */
    RenderTraverse(
        zClass_NodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        zClass_CameraDataPartial *data = (zClass_CameraDataPartial *)(node->classData);
        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
            if ((node->boundsFlags & 0x04) != 0 || g_zClass_RenderBoundsContextActive != 0 ||
                (node->flags & 0x00080000) == 0) {
                zBBoxCorners corners = {0};
                zClass_Class::gwNodeGetViewBBoxCorners(
                    node,
                    &corners
                );
                BBox::CornersToBoundingSphere(
                    &corners,
                    zClass_NodeViewSphereCenter(node),
                    zClass_NodeViewSphereRadius(node)
                );
                if ((node->flags & 0x00080000) != 0) {
                    node->boundsFlags &= ~0x04;
                }
            }
            result = zVideo_FrustumTestSphereClipMask(
                zClass_NodeViewSphereCenter(node),
                &clipMask,
                *zClass_NodeViewSphereRadius(node)
            );
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                clipMask &= ~0x20;
            }
        }

        if (result == 0) {
            const zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            node->flags |= 0x80000000;
            zMath::MatStackPushAndCloneParent(data->worldTransform);
            zMath::MatApplyLocalTRS(
                &data->posOffset,
                &data->targetOrEuler,
                &unitScale
            );
            if (g_zClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_zClass_RenderBoundsContextActive = 1;
            }
            if (gModel_RenderFn != 0) {
                gModel_RenderFn(
                    node,
                    clipMask
                );
            }
            if (node->listCountB > 0) {
                ++gModel_ClipMaskStackTop;
                *gModel_ClipMaskStackTop = clipMask;
                for (int i = 0; i < node->listCountB; ++i) {
                    zClass_Class::gwNodeRenderDispatch(
                        node->listB[i],
                        node->listCountB
                    );
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
