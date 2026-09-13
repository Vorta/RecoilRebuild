#include "zclass.h"

#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistanceadjustenabled
 * @recoil-artifact defines .data recoil:data:0x4ddd14: g_CZClass_CameraAutoClipDistanceAdjustEnabled.
 *
 * Purpose: enable adaptive camera clip-distance changes during scene render.
 */
int g_CZClass_CameraAutoClipDistanceAdjustEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistancethreshold
 * @recoil-artifact defines .data recoil:data:0x4ddd18: g_CZClass_CameraAutoClipDistanceThreshold.
 * Purpose: frame-time threshold used by adaptive camera clip-distance scaling.
 */
float g_CZClass_CameraAutoClipDistanceThreshold = 0.04f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistancescale
 * @recoil-artifact defines .data recoil:data:0x4ddd1c: g_CZClass_CameraAutoClipDistanceScale.
 * Purpose: current adaptive camera clip-distance scale.
 */
float g_CZClass_CameraAutoClipDistanceScale = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistancestep
 * @recoil-artifact defines .data recoil:data:0x4ddd20: g_CZClass_CameraAutoClipDistanceStep.
 * Purpose: per-frame adaptive camera clip-distance scale step.
 */
float g_CZClass_CameraAutoClipDistanceStep = 0.05f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistanceminscale
 * @recoil-artifact defines .data recoil:data:0x4ddd24: g_CZClass_CameraAutoClipDistanceMinScale.
 * Purpose: minimum adaptive camera clip-distance scale clamp.
 */
float g_CZClass_CameraAutoClipDistanceMinScale = 0.6f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-objecthsetestenabled
 * @recoil-artifact defines .data recoil:data:0x4ddd10: g_CZClass_ObjectHseTestEnabled.
 * Purpose: enable projected object visibility testing during tiled render.
 */
int g_CZClass_ObjectHseTestEnabled = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-currentcamera
 * @recoil-artifact defines .data recoil:data:0x4ddd34: g_CZClass_CurrentCamera.
 * Purpose: track the current active camera node.
 */
CZNodePartial *g_CZClass_CurrentCamera = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameratargetnode
 * @recoil-artifact defines .data recoil:data:0x4ddd38: g_CZClass_CameraTargetNode.
 * Purpose: track the current camera target node.
 */
CZNodePartial *g_CZClass_CameraTargetNode = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-camera-prevlistenerposx
 * @recoil-artifact defines .data recoil:data:0x4f4988: g_Camera_PrevListenerPosX.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position X storage.
 */
float g_Camera_PrevListenerPosX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-camera-prevlistenerposy
 * @recoil-artifact defines .data recoil:data:0x4f498c: g_Camera_PrevListenerPosY.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position Y storage.
 */
float g_Camera_PrevListenerPosY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-camera-prevlistenerposz
 * @recoil-artifact defines .data recoil:data:0x4f4990: g_Camera_PrevListenerPosZ.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position Z storage.
 */
float g_Camera_PrevListenerPosZ = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-findconvexhullunexpectedreturnmsg
 * @recoil-artifact defines .data recoil:data:0x4dddbc: g_CZClass_FindConvexHullUnexpectedReturnMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x37].
 * Purpose: report the unexpected convex-hull exit path during frustum-grid
 * footprint construction.
 */
char g_CZClass_FindConvexHullUnexpectedReturnMsg[0x37] =
    "Returning from find_convex_hull_xz in unexpected line.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-diamondtilerneedmoreringsmsg
 * @recoil-artifact defines .data recoil:data:0x4dddf4: g_CZClass_DiamondTilerNeedMoreRingsMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x26].
 * Purpose: report overflow of camera frustum-grid diamond ring buckets.
 */
char g_CZClass_DiamondTilerNeedMoreRingsMsg[0x26] =
    "Error: Need more diamond tiler rings.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-diamondtilerneedmorecellsperringmsg
 * @recoil-artifact defines .data recoil:data:0x4dde1c: g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x2f].
 * Purpose: report overflow of a camera frustum-grid diamond ring's cell list.
 */
char g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg[0x2f] =
    "Error: Need more diamond tiler cells per ring.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-lineerrorpointinpolygoninitcamerafrustumfmt
 * @recoil-artifact defines .data recoil:data:0x4dde4c: g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt.
 * BN data inventory declares writable Camera.c diagnostic format char[0x53].
 * Purpose: format the camera frustum-footprint mesh-face filter failure
 * diagnostic with the legacy source file and line.
 */
char g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt[0x53] =
    "%s: Line %d: ERROR from gModDIPointInPolygonInit() for camera "
    "frustrum footprint.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-vapstaticsnodename
 * @recoil-artifact defines .data recoil:data:0x4ddea0: g_CZClass_VapStaticsNodeName.
 * BN data inventory declares the shared writable zClass VAP statics node-name
 * literal char[0xc], referenced by Camera.c render filtering and cls_world.c
 * virtual-area partition creation.
 * Purpose: name generated virtual-area statics nodes and identify them during
 * offset-tile camera rendering.
 */
char g_CZClass_VapStaticsNodeName[0x0c] = "VAP_statics";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zcamera-frustumfootprintpoints
 * @recoil-artifact defines .data recoil:data:0x56cc40: g_zCamera_FrustumFootprintPoints.
 * Purpose: cache the frustum origin plus four corner points used by camera
 * grid-tile construction; BN bounds this zero-initialized array to five zVec3
 * entries, with the adjacent zero gaps outside this symbol.
 */
zVec3 g_zCamera_FrustumFootprintPoints[5] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zcamera-frustumfootprintpointcount
 * @recoil-artifact defines .data recoil:data:0x56ccac: g_zCamera_FrustumFootprintPointCount.
 * Purpose: count active frustum footprint points for grid-tile construction.
 */
int g_zCamera_FrustumFootprintPointCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zcamera-frustumgridtilerings
 * @recoil-artifact defines .data recoil:data:0x56ccc0: g_zCamera_FrustumGridTileRings.
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

    /**
     * Original inline validation construct observed in camera setter/getter
     * callers (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: validate a camera node, recover its camera data pointer, and
     * preserve the caller-specific legacy source-line diagnostics.
     */
#define ValidateCameraNode(node, outData, nullLine, dataLine, classLine) \
    ((node) == 0 \
        ? (zError::ReportOld( \
              0x400, \
              "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", \
              (nullLine), \
              "Null node pointer." \
          ), 5) \
        : ((node)->classData == 0 \
              ? (zError::ReportOld( \
                    0x400, \
                    "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", \
                    (dataLine), \
                    "Null class data pointer" \
                ), 5) \
              : ((node)->classId != kZClassNodeCamera \
                    ? (zError::ReportOld( \
                          0x400, \
                          "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", \
                          (classLine), \
                          "Bad Class Found.\n Wanted (%d)\n Found (%d)", \
                          (node)->classId, \
                          kZClassNodeCamera \
                      ), 3) \
                    : (*(outData) = \
                           (CZCameraDataPartial *)((node)->classData), \
                       0))))

    /**
     * Original inline construct observed in camera target callers
     * (D:\Proj\GameZRecoil\zClass\Camera.c).
     * Purpose: select the active target vector field based on the camera flag
     * that switches between world target and Euler/target storage.
     */
#define GetSelectedTargetVector(data) \
    (((data)->cameraFlags & 0x02) != 0 \
        ? &(data)->worldTarget \
        : &(data)->targetOrEuler)
}

namespace CZCamera {

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.setviewdistance
     * @recoil-artifact defines .text recoil:function:0x449ba0: CZCamera::SetViewDistance.
     * Purpose: configure adaptive camera clip-distance scaling from view distance.
     */
    void __fastcall SetViewDistance(
        int enableAutoClip,
        float distance
    ) {
        g_CZClass_CameraAutoClipDistanceAdjustEnabled = enableAutoClip;
        if (distance == 0.0f) {
            g_CZClass_CameraAutoClipDistanceThreshold = 0.04f;
        } else {
            g_CZClass_CameraAutoClipDistanceThreshold = 1.0f / distance;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameranew
     * @recoil-artifact defines .text recoil:function:0x449be0: CZCamera::gwCameraNew.
     * Purpose: allocate and initialize a camera node and its class data.
     */
    CZNodePartial *__cdecl gwCameraNew() {
        CZNodePartial *node = CZClass::gwNodeNew();
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x1e8, "Null node pointer.");
            return 0;
        }

        node->classId = kZClassNodeCamera;
        CZCameraDataPartial *data =
            (CZCameraDataPartial *)(calloc(1, sizeof(CZCameraDataPartial)));
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
        CZTypeList::Insert(8, node);
        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameraaddchild
     * @recoil-artifact defines .text recoil:function:0x449c90: CZCamera::gwCameraAddChild.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: validate camera parent/child inputs before using the generic
     * zClass listA/listB child-link routine.
     */
    int __fastcall gwCameraAddChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x239, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x23a, "Null node pointer.");
            return 5;
        }

        return CZClass::AddChildGeneric(parent, child);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameraremovechild
     * @recoil-artifact defines .text recoil:function:0x449cd0: CZCamera::gwCameraRemoveChild.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Purpose: validate camera parent/child inputs before using the generic
     * zClass listA/listB child-unlink routine.
     */
    int __fastcall gwCameraRemoveChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x251, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x252, "Null node pointer.");
            return 5;
        }

        return CZClass::RemoveChildGeneric(parent, child);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetactive
     * @recoil-artifact defines .text recoil:function:0x449d10: CZCamera::gwCameraSetActive.
     * @recoil-match byte
     *
     * Purpose: route the camera active-state update through the generic node helper.
     */
    int __fastcall gwCameraSetActive(
        CZNodePartial * node,
        int active
    ) {
        return CZClass::gwNodeSetActive(node, active);
    }
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetflagbit0
     * @recoil-artifact defines .text recoil:function:0x449d20: CZCamera::gwCameraSetFlagBit0.
     * Camera data flag bit 0 gates the zSound listener-state update in
     * BuildWorldTransform.
     * Purpose: validate a camera node and set or clear camera flag bit 0.
     */
    int __fastcall gwCameraSetFlagBit0(
        CZNodePartial * node,
        int enabled
    ) {
        CZCameraDataPartial *data = 0;
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.settargetnode
     * @recoil-artifact defines .text recoil:function:0x449da0: CZCamera::SetTargetNode.
     * @recoil-match byte
     *
     * Purpose: store the current global camera target node and report success.
     */
    int __fastcall SetTargetNode(CZNodePartial * target) {
        g_CZClass_CameraTargetNode = target;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.setactivecamera
     * @recoil-artifact defines .text recoil:function:0x449db0: CZCamera::SetActiveCamera.
     * @recoil-match byte
     *
     * Purpose: store the current global camera node and return it.
     */
    CZNodePartial *__fastcall SetActiveCamera(
        CZNodePartial * camera
    ) {
        g_CZClass_CurrentCamera = camera;
        return camera;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.setobjecthsetestenabled
     * @recoil-artifact defines .text recoil:function:0x449dc0: CZCamera::SetObjectHseTestEnabled.
     * @recoil-match byte
     *
     * Purpose: store the object HSE test enable flag and report success.
     */
    int __fastcall SetObjectHseTestEnabled(int enabled) {
        g_CZClass_ObjectHseTestEnabled = enabled;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetworld
     * @recoil-artifact defines .text recoil:function:0x449dd0: CZCamera::gwCameraSetWorld.
     * Purpose: validate camera and world nodes before assigning the camera world.
     */
    gwCameraSetWorld(
        CZNodePartial * camera,
        CZNodePartial * world
    ) {
        if (camera == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2be, "Null node pointer.");
            return 5;
        }
        if (world == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2bf, "Null node pointer.");
            return 5;
        }

        if (camera->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2c1, "Null class data pointer");
            return 5;
        }
        if (world->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2c2, "Null class data pointer");
            return 5;
        }

        if (camera->classId != kZClassNodeCamera) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
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
                "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                0x2c5,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                world->classId,
                kZClassNodeWorld
            );
            return 3;
        }

        ((CZCameraDataPartial *)(camera->classData))->worldNode = world;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetworld
     * @recoil-artifact defines .text recoil:function:0x449e80: CZCamera::gwCameraGetWorld.
     * @recoil-match byte
     *
     * Purpose: return the world node currently assigned to the camera.
     */
    CZNodePartial *__fastcall gwCameraGetWorld(
        CZNodePartial * camera
    ) {
        return ((CZCameraDataPartial *)(camera->classData))->worldNode;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetwindow
     * @recoil-artifact defines .text recoil:function:0x449e90: CZCamera::gwCameraSetWindow.
     * @recoil-match byte
     *
     * Purpose: assign the window node used by the camera view context.
     */
    gwCameraSetWindow(
        CZNodePartial * camera,
        CZNodePartial * window
    ) {
        ((CZCameraDataPartial *)(camera->classData))->windowNode = window;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetposition
     * @recoil-artifact defines .text recoil:function:0x449ea0: CZCamera::gwCameraSetPosition.
     * Purpose: set the camera position offset and dirty dependent transforms.
     */
    gwCameraSetPosition(
        CZNodePartial * camera,
        float x,
        float y,
        float z
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x3a7, 0x3a8, 0x3a9);
        if (result != 0) {
            return result;
        }

        data->transformDirty = 1;
        data->posOffset.x = x;
        data->posOffset.y = y;
        data->posOffset.z = z;
        data->cameraFlags &= ~0x02;
        if (camera->listCountA > 0) {
            ActivateChildren(camera, data);
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.activatechildren
     * @recoil-artifact defines .text recoil:function:0x449f50: CZCamera::ActivateChildren.
     * @recoil-match byte
     *
     * Purpose: mark camera children dirty and register the active camera node.
     */
    int __fastcall ActivateChildren(CZNodePartial *camera, CZCameraDataPartial *data) {
        data->cameraFlags |= 0x04;
        if ((camera->flags & 0x01) == 0) {
            CZTypeList::Insert(7, camera);
            camera->flags |= 0x01;
        }
        camera->flags |= 0x02;

        if (camera->listCountB > 0) {
            for (int i = 0; i < camera->listCountB; ++i) {
                CZNode::PropagateTransformDirtyRecursive(camera->listB[i]);
            }
        }

        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameratranslate
     * @recoil-artifact defines .text recoil:function:0x449fb0: CZCamera::gwCameraTranslate.
     * Purpose: translate the camera position offset and dirty dependent transforms.
     */
    gwCameraTranslate(
        CZNodePartial * camera,
        float dx,
        float dy,
        float dz
    ) {
        CZCameraDataPartial *data = 0;
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

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetposition
     * @recoil-artifact defines .text recoil:function:0x44a060: CZCamera::gwCameraGetPosition.
     * Purpose: return the camera position offset components.
     */
    gwCameraGetPosition(
        CZNodePartial * camera,
        float *outX,
        float *outY,
        float *outZ
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x414, 0x415, 0x416);
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasettarget
     * @recoil-artifact defines .text recoil:function:0x44a0f0: CZCamera::gwCameraSetTarget.
     * Purpose: set the selected camera target vector and update children.
     */
    gwCameraSetTarget(
        CZNodePartial * camera,
        float x,
        float y,
        float z
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x43c, 0x43d, 0x43e);
        if (result != 0) {
            return result;
        }

        zVec3 *target = GetSelectedTargetVector(data);
        target->x = x;
        target->y = y;
        target->z = z;
        if (camera->listCountA > 0) {
            ActivateChildren(camera, data);
        }

        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameratranslatetarget
     * @recoil-artifact defines .text recoil:function:0x44a1a0: CZCamera::gwCameraTranslateTarget.
     * Purpose: translate the selected camera target vector and update children.
     */
    gwCameraTranslateTarget(
        CZNodePartial * camera,
        float dx,
        float dy,
        float dz
    ) {
        CZCameraDataPartial *data = 0;
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

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragettarget
     * @recoil-artifact defines .text recoil:function:0x44a250: CZCamera::gwCameraGetTarget.
     * Purpose: return the selected camera target vector components.
     */
    gwCameraGetTarget(
        CZNodePartial * camera,
        float *outX,
        float *outY,
        float *outZ
    ) {
        CZCameraDataPartial *data = 0;
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

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetnearfarclip
     * @recoil-artifact defines .text recoil:function:0x44a2f0: CZCamera::gwCameraSetNearFarClip.
     * Purpose: store near/far clip distances and dirty frustum vectors.
     */
    gwCameraSetNearFarClip(
        CZNodePartial * camera,
        float nearClip,
        float farClip
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x509, 0x50a, 0x50b);
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetnearfarclip
     * @recoil-artifact defines .text recoil:function:0x44a380: CZCamera::gwCameraGetNearFarClip.
     * Purpose: return the camera near/far clip distances.
     */
    gwCameraGetNearFarClip(
        CZNodePartial * camera,
        float *outNear,
        float *outFar
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x52f, 0x530, 0x531);
        if (result != 0) {
            return result;
        }

        *outNear = data->nearClip;
        *outFar = data->farClip;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetviewport
     * @recoil-artifact defines .text recoil:function:0x44a410: CZCamera::gwCameraSetViewport.
     * Purpose: update viewport dimensions and derived frustum scale values.
     */
    gwCameraSetViewport(
        CZNodePartial * camera,
        float viewportWidth,
        float viewportHeight
    ) {
        CZCameraDataPartial *data = 0;
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetviewport
     * @recoil-artifact defines .text recoil:function:0x44a580: CZCamera::gwCameraGetViewport.
     * Purpose: return the camera viewport dimensions.
     */
    gwCameraGetViewport(
        CZNodePartial * camera,
        float *outWidth,
        float *outHeight
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x58e, 0x58f, 0x590);
        if (result != 0) {
            return result;
        }

        *outWidth = data->viewportWidth;
        *outHeight = data->viewportHeight;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetfov
     * @recoil-artifact defines .text recoil:function:0x44a610: CZCamera::gwCameraSetFOV.
     * Purpose: set camera frustum dimensions and derived projection scale values.
     */
    gwCameraSetFOV(
        CZNodePartial * camera,
        float fovX,
        float fovY
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x5b2, 0x5b3, 0x5b4);
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetfov
     * @recoil-artifact defines .text recoil:function:0x44a760: CZCamera::gwCameraGetFOV.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
     * Touched diagnostic string data: 0x4dd9d4, 0x4dd9bc, 0x4ddd44,
     * and 0x4ddd68.
     * Purpose: return the camera frustum FOV pair after legacy camera-node
     * validation diagnostics.
     */
    int __fastcall gwCameraGetFOV(
        CZNodePartial * camera,
        float *outFovX,
        float *outFovY
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x5e7, 0x5e8, 0x5e9);
        if (result != 0) {
            return result;
        }

        *outFovX = data->frustumWidth;
        *outFovY = data->frustumHeight;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetclipdistance
     * @recoil-artifact defines .text recoil:function:0x44a7f0: CZCamera::gwCameraGetClipDistance.
     * Purpose: return the camera clip distance.
     */
    gwCameraGetClipDistance(
        CZNodePartial * camera,
        float *outClipDistance
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x609, 0x60a, 0x60b);
        if (result != 0) {
            return result;
        }

        *outClipDistance = data->clipDistance;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetclipdistance
     * @recoil-artifact defines .text recoil:function:0x44a870: CZCamera::gwCameraSetClipDistance.
     * Purpose: store the camera clip distance and inverse squared distance.
     */
    gwCameraSetClipDistance(
        CZNodePartial * camera,
        float clipDistance
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x62a, 0x62b, 0x62c);
        if (result != 0) {
            return result;
        }

        data->clipDistance = clipDistance;
        data->invClipDistanceSq = 1.0f / (clipDistance * clipDistance);
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasethorizon
     * @recoil-artifact defines .text recoil:function:0x44a910: CZCamera::gwCameraSetHorizon.
     * Purpose: assign the horizon node that follows the camera position.
     */
    gwCameraSetHorizon(
        CZNodePartial * camera,
        CZNodePartial * horizonNode
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x64d, 0x64e, 0x64f);
        if (result != 0) {
            return result;
        }

        data->horizonNode = horizonNode;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasethorizonxz
     * @recoil-artifact defines .text recoil:function:0x44a980: CZCamera::gwCameraSetHorizonXZ.
     * Purpose: assign the horizon node that follows camera X/Z position.
     */
    gwCameraSetHorizonXZ(
        CZNodePartial * camera,
        CZNodePartial * horizonXZNode
    ) {
        CZCameraDataPartial *data = 0;
        const int result = ValidateCameraNode(camera, &data, 0x66e, 0x66f, 0x670);
        if (result != 0) {
            return result;
        }

        data->horizonXZNode = horizonXZNode;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameraupdate
     * @recoil-artifact defines .text recoil:function:0x44a9f0: CZCamera::gwCameraUpdate.
     * Purpose: validate the camera node and run the camera update implementation.
     */
    int __fastcall gwCameraUpdate(CZNodePartial * camera) {
        if (camera == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x75c, "Null node pointer.");
            return 5;
        }

        if (camera->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x75d, "Null class data pointer");
            return 5;
        }

        return UpdateImpl(camera, 0);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.updateimpl
     * @recoil-artifact defines .text recoil:function:0x44aa30: CZCamera::UpdateImpl.
     * Purpose: rebuild camera transforms, frustum planes, and clip centers.
     */
    int __fastcall UpdateImpl(
        CZNodePartial * camera,
        zVec3 * posOffset
    ) {
        CZCameraDataPartial *data = (CZCameraDataPartial *)(camera->classData);

        BuildWorldTransform(camera, data, posOffset);

        data->transformDirty = 1;
        if (data->localFrustumNormalsDirty != 0) {
            data->localFrustumNormalsDirty = 0;
            zVideoUpdateProjectionStateFromCameraData(data);
            data->transformDirty = 1;
        }

        data->transformDirty = 0;
        zClipAltBuildFrustumPlanes(data);

        if (data->frustumVectorsDirty != 0) {
            const float farClip = data->farClip;
            const float halfWidth = (float)(tan(data->fovX * 0.5f)) * farClip;
            const float halfHeight = (float)(tan(data->fovY * 0.5f)) * farClip;
            const float negHalfHeight = -halfHeight;
            const float negFarClip = -farClip;
            const float negHalfWidth = -halfWidth;

            data->frustumVectorsDirty = 0;
            data->frustumOrigin.x = 0.0f;
            data->frustumOrigin.y = 0.0f;
            data->frustumOrigin.z = 0.0f;
            data->frustumCorners[0].x = halfWidth;
            data->frustumCorners[0].y = negHalfHeight;
            data->frustumCorners[0].z = negFarClip;
            data->frustumCorners[1].x = negHalfWidth;
            data->frustumCorners[1].y = negHalfHeight;
            data->frustumCorners[1].z = negFarClip;
            data->frustumCorners[2].x = halfWidth;
            data->frustumCorners[2].y = halfHeight;
            data->frustumCorners[2].z = negFarClip;
            data->frustumCorners[3].x = negHalfWidth;
            data->frustumCorners[3].y = halfHeight;
            data->frustumCorners[3].z = negFarClip;
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.buildworldtransform
     * @recoil-artifact defines .text recoil:function:0x44abf0: CZCamera::BuildWorldTransform.
     * Purpose: build the camera world transform and update the zSound
     * listener bridge previous-position state.
     */
    int __fastcall BuildWorldTransform(
        CZNodePartial * camera,
        CZCameraDataPartial * data,
        zVec3 * posOffset
    ) {
        zMath::MatLoadIdentity();
        CZNode::gwNodeBuildNodeToAncestorMatrix(camera, 1);

        zMat4x3 *matrix = zMathMatGetCurrent();
        if (posOffset != 0) {
            matrix->posX += posOffset->x;
            matrix->posY += posOffset->y;
            matrix->posZ += posOffset->z;
        }

        data->cameraPos.x = matrix->posX;
        data->cameraPos.y = matrix->posY;
        data->cameraPos.z = matrix->posZ;
        data->forwardDir.x = -matrix->zx;
        data->forwardDir.y = -matrix->zy;
        data->forwardDir.z = -matrix->zz;

        memcpy(data->worldTransform, matrix, sizeof(zMat4x3));
        zMathMatExtractEulerAngles(matrix, &data->eulerAngles);
        zMath::MatLoadIdentity();
        zMathCameraStageInverseRotation((zMat4x3 *)(data->worldTransform));

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
                if (zSndGetSpeedOfSoundMps() <= listenerSpeed) {
                    listenerVelocity.x = 0.0f;
                    listenerVelocity.y = 0.0f;
                    listenerVelocity.z = 0.0f;
                }
            }

            g_zSnd_PreviousListenerPos.x = data->worldTransform[9];
            g_zSnd_PreviousListenerPos.y = data->worldTransform[10];
            g_zSnd_PreviousListenerPos.z = data->worldTransform[11];
            zSndUpdateListenerState(
                (zSndListenerState *)(data->worldTransform),
                &listenerVelocity
            );
        }

        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44ada0
     * @recoil-artifact defines .text recoil:function:0x44ada0: CZCamera::RenderTraverse.
     * Purpose: frustum-test and render a camera node traversal branch.
     */
    RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        CZCameraDataPartial *data = (CZCameraDataPartial *)(node->classData);
        zVec3 *viewSphereCenter = (zVec3 *)node->cachedSphereCenter;
        float *viewSphereRadius = &node->cachedSphereCenter[3];

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
            if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0 ||
                (node->flags & 0x00080000) == 0) {
                zBBoxCorners corners = {0};
                CZClass::gwNodeGetViewBBoxCorners(node, &corners);
                CZBBox::CornersToBoundingSphere(&corners, viewSphereCenter, viewSphereRadius);
                if ((node->flags & 0x00080000) != 0) {
                    node->boundsFlags &= ~0x04;
                }
                if (g_CZClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_CZClass_RenderBoundsContextActive = 1;
                }
            }
            result = zVideoFrustumTestSphereClipMask(
                viewSphereCenter,
                *viewSphereRadius,
                &clipMask
            );
            if ((node->flags & 0x80) != 0) {
                if (result == 0x20) {
                    result = 0;
                }
                clipMask &= ~0x20;
            }
        }

        if (result == 0) {
            const zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            node->flags |= 0x80000000;
            zMath::MatStackPushAndCloneParent(data->worldTransform);
            zMath::MatApplyLocalTRS(&data->posOffset, &data->targetOrEuler, &unitScale);
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
            zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
            if (di != 0) {
                if (g_CZClass_RenderRangeFadeActive != 0) {
                    di->flags |= 0x08;
                    di->blendScale = g_CZClass_RenderRangeFadeScale;
                }
                gModel_RenderFn(node, clipMask);
            }
            if (node->listCountB > 0) {
                ++gModel_ClipMaskStackTop;
                *gModel_ClipMaskStackTop = clipMask;
                for (int i = 0; i < node->listCountB; ++i) {
                    CZClass::gwNodeRenderDispatch(node->listB[i], node->listCountB);
                }
                --gModel_ClipMaskStackTop;
            }
            zMath::MatStackPopPtr();
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

}

namespace CZSound {

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44af60
     * @recoil-artifact defines .text recoil:function:0x44af60: CZSound::RenderTraverse
     *
     * Purpose: cull a sound node, push its local transform, render the node and
     * children, and restore traversal state.
     */
    RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        CZSoundDataPartial *data = (CZSoundDataPartial *)(node->classData);
        zVec3 *viewSphereCenter = (zVec3 *)node->cachedSphereCenter;
        float *viewSphereRadius = &node->cachedSphereCenter[3];

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
            if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0 ||
                (node->flags & 0x00080000) == 0) {
                zBBoxCorners corners = {0};
                CZClass::gwNodeGetViewBBoxCorners(node, &corners);
                CZBBox::CornersToBoundingSphere(&corners, viewSphereCenter, viewSphereRadius);
                if ((node->flags & 0x00080000) != 0) {
                    node->boundsFlags &= ~0x04;
                }
                if (g_CZClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_CZClass_RenderBoundsContextActive = 1;
                }
            }
            result = zVideoFrustumTestSphereClipMask(
                viewSphereCenter,
                *viewSphereRadius,
                &clipMask
            );
            if ((node->flags & 0x80) != 0) {
                if (result == 0x20) {
                    result = 0;
                }
                clipMask &= ~0x20;
            }
        }

        if (result == 0) {
            const zVec3 angles = {0.0f, 0.0f, 0.0f};
            const zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            node->flags |= 0x80000000;
            zMath::MatStackPushAndCloneParent(data->savedParentMatrix);
            zMath::MatApplyLocalTRS(&angles, &data->localPosition, &unitScale);
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
            zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
            if (di != 0) {
                if (g_CZClass_RenderRangeFadeActive != 0) {
                    di->flags |= 0x08;
                    di->blendScale = g_CZClass_RenderRangeFadeScale;
                }
                gModel_RenderFn(node, clipMask);
            }
            if (node->listCountB > 0) {
                ++gModel_ClipMaskStackTop;
                *gModel_ClipMaskStackTop = clipMask;
                for (int i = 0; i < node->listCountB; ++i) {
                    CZClass::gwNodeRenderDispatch(node->listB[i], node->listCountB);
                }
                --gModel_ClipMaskStackTop;
            }
            zMath::MatStackPopPtr();
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

}

namespace CZLight {

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b140
     * @recoil-artifact defines .text recoil:function:0x44b140: CZLight::RenderTraverse
     * Purpose: cull an enabled light node, push render-bounds context when
     * needed, apply local transform, render the node subtree, and restore state.
     */
    int __fastcall RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        CZLightDataPartial *data = (CZLightDataPartial *)(node->classData);
        zVec3 *viewSphereCenter = (zVec3 *)node->cachedSphereCenter;
        float *viewSphereRadius = &node->cachedSphereCenter[3];

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
            if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0 ||
                (node->flags & 0x00080000) == 0) {
                zBBoxCorners corners = {0};
                CZClass::gwNodeGetViewBBoxCorners(node, &corners);
                CZBBox::CornersToBoundingSphere(&corners, viewSphereCenter, viewSphereRadius);
                if ((node->flags & 0x00080000) != 0) {
                    node->boundsFlags &= ~0x04;
                }
                if (g_CZClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_CZClass_RenderBoundsContextActive = 1;
                }
            }
            result = zVideoFrustumTestSphereClipMask(
                viewSphereCenter,
                *viewSphereRadius,
                &clipMask
            );
            if ((node->flags & 0x80) != 0) {
                if (result == 0x20) {
                    result = 0;
                }
                clipMask &= ~0x20;
            }
        }

        if (result == 0) {
            const zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            node->flags |= 0x80000000;
            zMath::MatStackPushAndCloneParent(data->savedParentMatrix);
            zMath::MatApplyLocalTRS(&data->localRotation, &data->localPosition, &unitScale);
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
            zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
            if (di != 0) {
                if (g_CZClass_RenderRangeFadeActive != 0) {
                    di->flags |= 0x08;
                    di->blendScale = g_CZClass_RenderRangeFadeScale;
                }
                gModel_RenderFn(node, clipMask);
            }
            if (node->listCountB > 0) {
                ++gModel_ClipMaskStackTop;
                *gModel_ClipMaskStackTop = clipMask;
                for (int i = 0; i < node->listCountB; ++i) {
                    CZClass::gwNodeRenderDispatch(node->listB[i], node->listCountB);
                }
                --gModel_ClipMaskStackTop;
            }
            zMath::MatStackPopPtr();
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

}

namespace CZObject3D {

    namespace {
    const int kZClassNodeObject3D = 5;
    const int kObject3DVisibleFlag = 0x04;
    const int kObject3DTransformDirtyFlag = 0x20;
    const int kNodeBoundsDirtyFlag = 0x04;
    const int kSingleParentFlag = 0x00080000;
    const int kNodeTransformDirtyPropagatedFlag = 0x02000000;

    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
     * BN keeps the bounds refresh, sphere test, and far-clip repair as
     * caller-local render traversal code rather than a separate call target.
     * Purpose: decide whether object render culling is needed, refresh the view
     * bounding sphere, and run the frustum sphere clip-mask test.
     */
    inline int CullNodeForRender(
        CZNodePartial * node,
        int siblingCountHint,
        int *clipMask
    ) {
        int testNeeded = 0;
        if (g_CZClass_ObjectHseTestEnabled == 0) {
            testNeeded =
                ((*clipMask != 0 || g_CZClass_RenderFrustumGridTileIndex > 0) &&
                    siblingCountHint > 1);
        } else {
            testNeeded = (*clipMask != 0 && siblingCountHint > 1);
        }

        if (testNeeded == 0 && (node->flags & kSingleParentFlag) != 0) {
            return 0;
        }

        if ((node->boundsFlags & kNodeBoundsDirtyFlag) != 0 ||
            g_CZClass_RenderBoundsContextActive != 0 || (node->flags & kSingleParentFlag) == 0) {
            zBBoxCorners corners = {0};
            CZClass::gwNodeGetViewBBoxCorners(node, &corners);
            CZBBox::CornersToBoundingSphere(
                &corners,
                zClassNodeViewSphereCenter(node),
                zClassNodeViewSphereRadius(node)
            );
            if ((node->flags & kSingleParentFlag) != 0) {
                node->boundsFlags &= ~kNodeBoundsDirtyFlag;
            }
        }

        int result = zVideoFrustumTestSphereClipMask(
            zClassNodeViewSphereCenter(node),
            *zClassNodeViewSphereRadius(node),
            clipMask
        );
        if ((node->flags & 0x80) != 0 && result == 0x20) {
            result = 0;
            *clipMask &= ~0x20;
        }
        return result;
    }

    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
     * BN shows the Object3D matrix-selection branches in the render traversal
     * body, with only direct zMath provider calls inside the pattern.
     * Purpose: push the correct object matrix onto the zMath stack, recomputing
     * cached world matrix state when the transform is dirty.
     */
    inline void PushObjectMatrix(
        CZObject3DDataPartial * data,
        int *pushed
    ) {
        const int flags = data->flags;
        if ((flags & 0x08) != 0) {
            *pushed = 0;
            return;
        }

        *pushed = 1;
        if ((flags & kObject3DTransformDirtyFlag) != 0) {
            zMath::MatStackPushAndCloneParent(data->cachedWorldMatrix);
            zMath::MatMultiply((const zMat4x3 *)data->localMatrix, 3);
            data->flags &= ~kObject3DTransformDirtyFlag;
        } else if ((flags & kSingleParentFlag) == 0) {
            zMath::MatStackPushAndCloneParent(data->cachedWorldMatrix);
            zMath::MatMultiply((const zMat4x3 *)data->localMatrix, 3);
        } else {
            zMath::MatStackPushPtr(data->cachedWorldMatrix);
        }
    }

    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
     * BN keeps the render-state stack pushes and zModel setter calls in the
     * Object3D render traversal body.
     * Purpose: push vertex-alpha, alpha-scale, and software color override
     * render state for an Object3D node.
     */
    inline void PushObjectRenderState(
        CZNodePartial * node,
        CZObject3DDataPartial * data,
        int *pushedVertexAlpha,
        int *pushedAlphaScale,
        int *pushedSoftwareState
    ) {
        *pushedVertexAlpha = 0;
        *pushedAlphaScale = 0;
        *pushedSoftwareState = 0;

        if ((node->flags & 0x00800000) != 0 && g_CZClass_RenderVertexAlphaOverrideActive == 0) {
            *pushedVertexAlpha = 1;
            g_CZClass_RenderVertexAlphaOverrideActive = 1;
            zModelRenderVertexAlphaEnabledSetCurrent(1);
        }

        if ((data->flags & 0x02) != 0) {
            *pushedAlphaScale = 1;
            ++g_CZClass_RenderAlphaScaleStackTop;
            g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop] = data->alphaScale;
            zModelRenderAlphaScaleSetCurrent(data->alphaScale);
        }

        if ((data->flags & 0x04) != 0) {
            *pushedSoftwareState = 1;
            ++g_CZClass_SoftwarePathStateStackTop;
            g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color =
                data->color;
            g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].alpha =
                data->colorAlpha;
            zModelFogTargetColorOverrideSetCurrent(
                &g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color,
                data->colorAlpha
            );
        }
    }

    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
     * BN keeps the vertex-alpha, alpha-scale, and software-state restore
     * sequence in the Object3D render traversal epilogue.
     * Purpose: restore Object3D render-state stacks after rendering a node
     * subtree.
     */
    inline void PopObjectRenderState(
        int pushedVertexAlpha,
        int pushedAlphaScale,
        int pushedSoftwareState
    ) {
        if (pushedVertexAlpha != 0) {
            g_CZClass_RenderVertexAlphaOverrideActive = 0;
            zModelRenderVertexAlphaEnabledSetCurrent(0);
        }

        if (pushedAlphaScale != 0) {
            --g_CZClass_RenderAlphaScaleStackTop;
            const float scale =
                g_CZClass_RenderAlphaScaleStackTop >= 0
                    ? g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop]
                    : 1.0f;
            zModelRenderAlphaScaleSetCurrent(scale);
        }

        if (pushedSoftwareState != 0) {
            --g_CZClass_SoftwarePathStateStackTop;
            if (g_CZClass_SoftwarePathStateStackTop >= 0) {
                zModelFogTargetColorOverrideSetCurrent(
                    &g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop]
                        .color,
                    g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].alpha
                );
            } else {
                zModelFogTargetColorOverrideSetCurrent(0, 0.0f);
            }
        }
    }

    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
     * BN shows the child-loop variant check, recursive Object3D call, and
     * generic dispatch call inline in the traversal body.
     * Purpose: render Object3D children through variant filtering and dispatch
     * non-Object3D children through the generic node renderer.
     */
    inline void RenderObjectChildren(
        CZNodePartial * node,
        int clipMask
    ) {
        if (node->listCountB <= 0) {
            return;
        }

        ++gModel_ClipMaskStackTop;
        *gModel_ClipMaskStackTop = clipMask;
        for (int i = 0; i < node->listCountB; ++i) {
            CZNodePartial *child = node->listB[i];
            if (child != 0 && child->classId == kZClassNodeObject3D) {
                if (VariantTag::CurrentAllowsId(child->nodeType) != 0) {
                    CZObject3D::RenderTraverse(child, node->listCountB);
                }
            } else if (child != 0) {
                CZClass::gwNodeRenderDispatch(child, node->listCountB);
            }
        }
        --gModel_ClipMaskStackTop;
    }

    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b300
     * @recoil-artifact defines .text recoil:function:0x44b300: CZObject3D::RenderTraverse
     * @recoil-match byte
     *
     * Purpose: cull Object3D nodes, manage transforms and render state, and render children.
     */
    RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & kObject3DVisibleFlag) == 0) {
            return 0;
        }

        node->flags = flags & ~kNodeTransformDirtyPropagatedFlag;
        int altClipReset;
        if (gAltClipPassEnabled != 0 && node == g_CZClass_CameraTargetNode) {
            altClipReset = 1;
            gAltClipPassEnabled = 0;
        } else {
            altClipReset = 0;
        }

        CZObject3DDataPartial *data = (CZObject3DDataPartial *)(node->classData);

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        int testNeeded = 0;
        if (g_CZClass_ObjectHseTestEnabled != 0) {
            testNeeded =
                ((clipMask != 0 || g_CZClass_RenderFrustumGridTileIndex > 0) &&
                    siblingCountHint > 1);
        } else {
            testNeeded = (clipMask != 0 && siblingCountHint > 1);
        }

        if (testNeeded != 0 || (node->flags & kSingleParentFlag) == 0) {
            if ((node->boundsFlags & kNodeBoundsDirtyFlag) != 0 ||
                g_CZClass_RenderBoundsContextActive != 0 ||
                (node->flags & kSingleParentFlag) == 0) {
                if ((node->flags & 0x100) == 0) {
                    if (altClipReset != 0) {
                        gAltClipPassEnabled = 1;
                    }
                    return 0;
                }
                zBBoxCorners corners;
                CZClass::gwNodeGetViewBBoxCorners(node, &corners);
                CZBBox::CornersToBoundingSphere(&corners, (zVec3 *)node->cachedSphereCenter, (&node->cachedSphereCenter[3]));
                if ((node->flags & kSingleParentFlag) != 0) {
                    node->boundsFlags &= ~kNodeBoundsDirtyFlag;
                }
                if (g_CZClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_CZClass_RenderBoundsContextActive = 1;
                }
            }

            result = zVideoFrustumTestSphereClipMask(
                (zVec3 *)node->cachedSphereCenter,
                node->cachedSphereCenter[3],
                &clipMask
            );
            if ((node->flags & 0x80) != 0) {
                if (result == 0x20) {
                    result = 0;
                }
                clipMask &= ~0x20;
            }
        }
        if (result == 0) {
            int matrixPushed;
            if ((data->flags & 0x08) == 0) {
                matrixPushed = 1;
                if ((node->flags & kSingleParentFlag) != 0) {
                    if ((data->flags & kObject3DTransformDirtyFlag) != 0) {
                        zMath::MatStackPushAndCloneParent(data->cachedWorldMatrix);
                        zMath::MatMultiply((const zMat4x3 *)data->localMatrix, 3);
                        data->flags &= ~kObject3DTransformDirtyFlag;
                        if (g_CZClass_RenderBoundsContextActive == 0) {
                            boundsContextPushed = 1;
                            g_CZClass_RenderBoundsContextActive = 1;
                        }
                    } else {
                        zMath::MatStackPushPtr(data->cachedWorldMatrix);
                    }
                } else {
                    zMath::MatStackPushAndCloneParent(data->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)data->localMatrix, 3);
                    if (g_CZClass_RenderBoundsContextActive == 0) {
                        boundsContextPushed = 1;
                        g_CZClass_RenderBoundsContextActive = 1;
                    }
                }
            } else {
                matrixPushed = 0;
            }

            int pushedVertexAlpha;
            int pushedAlphaScale;
            int pushedSoftwareState;

            if ((node->flags & 0x00800000) != 0 &&
                g_CZClass_RenderVertexAlphaOverrideActive == 0) {
                pushedVertexAlpha = 1;
                g_CZClass_RenderVertexAlphaOverrideActive = 1;
                zModelRenderVertexAlphaEnabledSetCurrent(1);
            } else {
                pushedVertexAlpha = 0;
            }

            if ((data->flags & 0x02) != 0) {
                pushedAlphaScale = 1;
                ++g_CZClass_RenderAlphaScaleStackTop;
                g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop] =
                    data->alphaScale;
                zModelRenderAlphaScaleSetCurrent(data->alphaScale);
            } else {
                pushedAlphaScale = 0;
            }

            if ((data->flags & 0x04) != 0) {
                pushedSoftwareState = 1;
                ++g_CZClass_SoftwarePathStateStackTop;
                g_CZClass_SoftwarePathRenderStateStack[
                    g_CZClass_SoftwarePathStateStackTop
                ].color = data->color;
                float colorAlpha;
                g_CZClass_SoftwarePathRenderStateStack[
                    g_CZClass_SoftwarePathStateStackTop
                ].alpha = colorAlpha = data->colorAlpha;
                zModelFogTargetColorOverrideSetCurrent(
                    &g_CZClass_SoftwarePathRenderStateStack[
                        g_CZClass_SoftwarePathStateStackTop
                    ].color,
                    colorAlpha
                );
            } else {
                pushedSoftwareState = 0;
            }

            int visibleByProjectedSphere;
            if (g_CZClass_ObjectHseTestEnabled != 0 && g_CZClass_RenderFrustumGridTileIndex > 0 &&
                siblingCountHint != 1 && g_CZClass_RenderVertexAlphaOverrideActive == 0) {
                visibleByProjectedSphere = zScene::TestProjectedSphereVisible(
                    (zVec3 *)node->cachedSphereCenter,
                    node->cachedSphereCenter[3]
                );
            } else {
                visibleByProjectedSphere = 1;
            }
            if (visibleByProjectedSphere != 0) {
                node->flags |= 0x80000000;
                zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
                if (di != 0) {
                    if (g_CZClass_RenderRangeFadeActive != 0) {
                        di->flags |= 0x08;
                        ((zDiPartial *)(unsigned int)node->userDataOrDiRef)->blendScale =
                        g_CZClass_RenderRangeFadeScale;
                    }
                    gModel_RenderFn(node, clipMask);
                }
                if (node->listCountB > 0) {
                    ++gModel_ClipMaskStackTop;
                    *gModel_ClipMaskStackTop = clipMask;
                    for (int i = 0; i < node->listCountB; ++i) {
                        CZNodePartial *child = node->listB[i];
                        if (child != 0) {
                            if (child->classId == kZClassNodeObject3D) {
                                if (VariantTag::CurrentAllowsId(child->nodeType) != 0) {
                                    CZObject3D::RenderTraverse(node->listB[i], node->listCountB);
                                }
                            } else {
                                CZClass::gwNodeRenderDispatch(child, node->listCountB);
                            }
                        }
                    }
                    --gModel_ClipMaskStackTop;
                }
            }

            if (pushedVertexAlpha != 0) {
                g_CZClass_RenderVertexAlphaOverrideActive = 0;
                zModelRenderVertexAlphaEnabledSetCurrent(0);
            }

            if (pushedAlphaScale != 0) {
                --g_CZClass_RenderAlphaScaleStackTop;
                if (g_CZClass_RenderAlphaScaleStackTop < 0) {
                    zModelRenderAlphaScaleSetCurrent(1.0f);
                } else {
                    zModelRenderAlphaScaleSetCurrent(
                        g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop]);
                }
            }

            if (pushedSoftwareState != 0) {
                --g_CZClass_SoftwarePathStateStackTop;
                if (g_CZClass_SoftwarePathStateStackTop < 0) {
                    zModelFogTargetColorOverrideSetCurrent(0, 0.0f);
                } else {
                    zModelFogTargetColorOverrideSetCurrent(
                        &g_CZClass_SoftwarePathRenderStateStack[
                            g_CZClass_SoftwarePathStateStackTop
                        ].color,
                        g_CZClass_SoftwarePathRenderStateStack[
                            g_CZClass_SoftwarePathStateStackTop
                        ].alpha
                    );
                }
            }
            if (matrixPushed != 0) {
                zMath::MatStackPopPtr();
            }
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        if (altClipReset != 0) {
            gAltClipPassEnabled = 1;
        }
        return result;
    }

}

namespace CZAnimate {

    namespace {
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b710
     * @recoil-artifact defines .text recoil:function:0x44b710: CZAnimate::RenderTraverse
     *
     * Purpose: cull an animate node, push its animated transform when active,
     * render the node and children, and restore traversal state.
     */
    RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        CZAnimateDataPartial *data = (CZAnimateDataPartial *)(node->classData);
        zVec3 *viewSphereCenter = (zVec3 *)node->cachedSphereCenter;
        float *viewSphereRadius = &node->cachedSphereCenter[3];

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if ((clipMask != 0 && siblingCountHint > 1) ||
            (node->flags & 0x00080000) == 0) {
            if ((node->boundsFlags & 0x04) != 0 ||
                g_CZClass_RenderBoundsContextActive != 0 ||
                (node->flags & 0x00080000) == 0) {
                zBBoxCorners corners = {0};
                CZClass::gwNodeGetViewBBoxCorners(node, &corners);
                CZBBox::CornersToBoundingSphere(&corners, viewSphereCenter, viewSphereRadius);
                if ((node->flags & 0x00080000) != 0) {
                    node->boundsFlags &= ~0x04;
                }
                if (g_CZClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_CZClass_RenderBoundsContextActive = 1;
                }
            }
            result = zVideoFrustumTestSphereClipMask(
                viewSphereCenter,
                *viewSphereRadius,
                &clipMask
            );
            if ((node->flags & 0x80) != 0) {
                if (result == 0x20) {
                    result = 0;
                }
                clipMask &= ~0x20;
            }
        }

        if (result == 0) {
            int matrixPushed = 0;
            node->flags |= 0x80000000;
            if ((data->statusFlags & 0x04) != 0) {
                matrixPushed = 1;
                zMath::MatStackPushAndCloneParent(data->savedParentMatrix);
                zMath::MatMultiply((const zMat4x3 *)data->animatedTransform, 3);
                if (g_CZClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_CZClass_RenderBoundsContextActive = 1;
                }
            }
            zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
            if (di != 0) {
                if (g_CZClass_RenderRangeFadeActive != 0) {
                    di->flags |= 0x08;
                    di->blendScale = g_CZClass_RenderRangeFadeScale;
                }
                gModel_RenderFn(node, clipMask);
            }
            if (node->listCountB > 0) {
                ++gModel_ClipMaskStackTop;
                *gModel_ClipMaskStackTop = clipMask;
                for (int i = 0; i < node->listCountB; ++i) {
                    CZClass::gwNodeRenderDispatch(node->listB[i], node->listCountB);
                }
                --gModel_ClipMaskStackTop;
            }
            if (matrixPushed != 0) {
                zMath::MatStackPopPtr();
            }
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

}

namespace CZLod {

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b8c0
     * @recoil-artifact defines .text recoil:function:0x44b8c0: CZLod::RenderTraverse
     *
     * Purpose: cull and render an LOD node, applying range, scale, alpha, and
     * vertex-alpha fades while maintaining the render traversal stacks.
     */
    RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        CZLodDataPartial *data = (CZLodDataPartial *)(node->classData);
        zVec3 *viewSphereCenter = (zVec3 *)node->cachedSphereCenter;
        float *viewSphereRadius = &node->cachedSphereCenter[3];

        node->flags = flags & ~0x02000000;
        CZLodDistanceState &state =
            g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop];
        if (data->computeOwnDistance == 0 &&
            (state.distanceSq < data->nearRangeSq || state.distanceSq >= data->farRangeSq)) {
            return 0;
        }

        if ((node->boundsFlags & 0x04) != 0 ||
            g_CZClass_RenderBoundsContextActive != 0 ||
            (node->flags & 0x00080000) == 0) {
            zBBoxCorners corners = {0};
            CZClass::gwNodeGetViewBBoxCorners(node, &corners);
            CZBBox::CornersToBoundingSphere(&corners, viewSphereCenter, viewSphereRadius);
            if ((node->flags & 0x00080000) != 0) {
                node->boundsFlags &= ~0x04;
            }
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }
        if (data->computeOwnDistance != 0) {
            state.center = *viewSphereCenter;
            zVec3 delta = {0};
            delta.x = g_zVideo_pActiveViewContext->cameraPos.x - state.center.x;
            delta.y = g_zVideo_pActiveViewContext->cameraPos.y - state.center.y;
            delta.z = g_zVideo_pActiveViewContext->cameraPos.z - state.center.z;
            state.distanceSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            state.distanceSq *= g_zVideo_pActiveViewContext->invClipDistanceSq;
        }
        if (state.distanceSq < data->nearRangeSq || state.distanceSq >= data->farRangeSq) {
            if (boundsContextPushed != 0) {
                g_CZClass_RenderBoundsContextActive = 0;
            }
            return 0;
        }

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;
        int pushScaleMatrix = 0;
        int pushAlphaScale = 0;
        float alphaScale = 1.0f;
        int distanceBits = 0;
        memcpy(&distanceBits, &state.distanceSq, sizeof(distanceBits));
        distanceBits = (distanceBits >> 1) + 0x1fc00000;
        float distance = 0.0f;
        memcpy(&distance, &distanceBits, sizeof(distance));
        if (distance > data->nearRange) {
            distance = data->nearRange;
        }

        if (data->fadeAmount.x > 0.01f) {
            const float fadeBegin = data->nearRange - data->fadeWidth.x;
            pushScaleMatrix = 1;
            scaleX = distance <= fadeBegin
                         ? 1.0f
                         : 1.0f - (fadeBegin - distance) * (data->fadeEndScale.x - 1.0f) /
                                      data->fadeWidth.x;
            if (data->active != 0) {
                scaleY = scaleX;
                scaleZ = scaleX;
            }
        }
        if (data->active == 0) {
            if (data->fadeAmount.y > 0.01f) {
                const float fadeBegin = data->nearRange - data->fadeWidth.y;
                pushScaleMatrix = 1;
                scaleY = distance <= fadeBegin
                             ? 1.0f
                             : 1.0f - (fadeBegin - distance) * (data->fadeEndScale.y - 1.0f) /
                                          data->fadeWidth.y;
            }
            if (data->fadeAmount.z > 0.01f) {
                const float fadeBegin = data->nearRange - data->fadeWidth.z;
                pushScaleMatrix = 1;
                scaleZ = distance <= fadeBegin
                             ? 1.0f
                             : 1.0f - (fadeBegin - distance) * (data->fadeEndScale.z - 1.0f) /
                                          data->fadeWidth.z;
            }
        }

        if (data->vertexShadingAmount > 0.01f && distance > data->nearRange - data->fogStartDist) {
            pushAlphaScale = 1;
            alphaScale = (data->nearRange - distance) / data->fogStartDist;
        }
        if (data->fogFadeAmount > 0.01f) {
            int nearDistanceBits = 0;
            memcpy(&nearDistanceBits, &data->nearRangeSq, sizeof(nearDistanceBits));
            nearDistanceBits = (nearDistanceBits >> 1) + 0x1fc00000;
            float nearDistance = 0.0f;
            memcpy(&nearDistance, &nearDistanceBits, sizeof(nearDistance));
            if (distance < nearDistance) {
                distance = nearDistance;
            }
            if (distance < nearDistance + data->fogFadeWidth) {
                const float fogScale = (distance - nearDistance) / data->fogFadeWidth;
                if (fogScale < alphaScale) {
                    alphaScale = fogScale;
                    pushAlphaScale = 1;
                }
            }
        }

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if (clipMask != 0 && siblingCountHint > 1) {
            result = zVideoFrustumTestSphereClipMask(
                viewSphereCenter,
                *viewSphereRadius,
                &clipMask
            );
            if ((node->flags & 0x80) != 0) {
                if (result == 0x20) {
                    result = 0;
                }
                clipMask &= ~0x20;
            }
        }

        if (result == 0) {
            node->flags |= 0x80000000;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;

            if (data->rangeNode != 0) {
                const float fadeBegin = data->farRangeSq - data->rangeSq;
                g_CZClass_RenderRangeFadeActive = 1;
                if (fadeBegin < state.distanceSq) {
                    g_CZClass_RenderRangeFadeScale =
                        (state.distanceSq - fadeBegin) / (data->farRangeSq - fadeBegin);
                } else {
                    g_CZClass_RenderRangeFadeScale = 0.0f;
                }
            }

            const int nextLodStack = g_CZClass_LodDistanceStateStackTop + 1;
            g_CZClass_LodDistanceStateStack[nextLodStack] =
                g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop];
            g_CZClass_LodDistanceStateStackTop = nextLodStack;

            zMat4x3 slotBuffer;
            if (pushScaleMatrix != 0) {
                zMath::MatStackPushAndCloneParent((float *)&slotBuffer);
                zMath_Mat_Scale(scaleX, scaleY, scaleZ);
            }
            if (pushAlphaScale != 0) {
                ++g_CZClass_RenderAlphaScaleStackTop;
                g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop] = alphaScale;
                zModelRenderAlphaScaleSetCurrent(alphaScale);
            }

            int pushedVertexAlpha = 0;
            if ((node->flags & 0x00800000) != 0 && g_CZClass_RenderVertexAlphaOverrideActive == 0) {
                pushedVertexAlpha = 1;
                g_CZClass_RenderVertexAlphaOverrideActive = 1;
                zModelRenderVertexAlphaEnabledSetCurrent(1);
            }

            for (int i = 0; i < node->listCountB; ++i) {
                CZClass::gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }

            if (pushScaleMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            if (pushAlphaScale != 0) {
                --g_CZClass_RenderAlphaScaleStackTop;
                const float previousAlphaScale =
                    g_CZClass_RenderAlphaScaleStackTop >= 0
                        ? g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop]
                        : 1.0f;
                zModelRenderAlphaScaleSetCurrent(previousAlphaScale);
            }
            if (pushedVertexAlpha != 0) {
                g_CZClass_RenderVertexAlphaOverrideActive = 0;
                zModelRenderVertexAlphaEnabledSetCurrent(0);
            }
            --g_CZClass_LodDistanceStateStackTop;
            g_CZClass_RenderRangeFadeActive = 0;
            --gModel_ClipMaskStackTop;
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

}

namespace CZSequence {

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44bea0
     * @recoil-artifact defines .text recoil:function:0x44bea0: CZSequence::RenderTraverse
     *
     * Purpose: cull an active sequence node, push traversal state, and render
     * only the currently selected child entry.
     */
    RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        int boundsContextPushed = 0;
        CZSequenceDataPartial *data;
        const int flags = node->flags;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        data = (CZSequenceDataPartial *)(node->classData);
        zVec3 *viewSphereCenter = (zVec3 *)node->cachedSphereCenter;
        float *viewSphereRadius = &node->cachedSphereCenter[3];

        node->flags = flags & ~0x02000000;
        if (data->isActive == 0) {
            return 0;
        }

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if (clipMask != 0 && siblingCountHint > 1) {
            if ((node->boundsFlags & 0x04) != 0 ||
                g_CZClass_RenderBoundsContextActive != 0) {
                zBBoxCorners corners = {0};
                CZClass::gwNodeGetViewBBoxCorners(node, &corners);
                CZBBox::CornersToBoundingSphere(&corners, viewSphereCenter, viewSphereRadius);
                node->boundsFlags &= ~0x04;
            }
            result = zVideoFrustumTestSphereClipMask(
                viewSphereCenter,
                *viewSphereRadius,
                &clipMask
            );
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                clipMask &= ~0x20;
            }
        }
        if (g_CZClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_CZClass_RenderBoundsContextActive = 1;
        }

        if (result == 0) {
            node->flags |= 0x80000000;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            CZClass::gwNodeRenderDispatch(
                data->entries[data->currentIndex].node,
                node->listCountB
            );
            --gModel_ClipMaskStackTop;
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

}

namespace CZSwitch {

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44bfb0
     * @recoil-artifact defines .text recoil:function:0x44bfb0: CZSwitch::RenderTraverse
     *
     * Purpose: cull the switch node, push the clip mask, and render only the
     * active child-mask entries.
     */
    RenderTraverse(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        int boundsContextPushed = 0;
        const int flags = node->flags;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        CZSwitchDataPartial *data = (CZSwitchDataPartial *)(node->classData);
        zVec3 *viewSphereCenter = (zVec3 *)node->cachedSphereCenter;
        float *viewSphereRadius = &node->cachedSphereCenter[3];

        node->flags = flags & ~0x02000000;
        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if (clipMask != 0 && siblingCountHint > 1) {
            if ((node->boundsFlags & 0x04) != 0 ||
                g_CZClass_RenderBoundsContextActive != 0) {
                zBBoxCorners corners = {0};
                CZClass::gwNodeGetViewBBoxCorners(node, &corners);
                CZBBox::CornersToBoundingSphere(&corners, viewSphereCenter, viewSphereRadius);
                node->boundsFlags &= ~0x04;
            }
            result = zVideoFrustumTestSphereClipMask(
                viewSphereCenter,
                *viewSphereRadius,
                &clipMask
            );
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                clipMask &= ~0x20;
            }
        }
        if (g_CZClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_CZClass_RenderBoundsContextActive = 1;
        }

        if (result == 0) {
            node->flags |= 0x80000000;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            const unsigned int activeMask = data->childMasks[data->activeMaskIndex];
            for (int i = 0; i < node->listCountB; ++i) {
                if (((activeMask >> i) & 1U) != 0) {
                    CZClass::gwNodeRenderDispatch(node->listB[i], node->listCountB);
                }
            }
            --gModel_ClipMaskStackTop;
        }

        if (boundsContextPushed != 0) {
            g_CZClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

}

namespace CZClass {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwnoderenderdispatch
     * @recoil-artifact defines .text recoil:function:0x44c0e0: CZClass::gwNodeRenderDispatch.
     * Purpose: route visible scene nodes to the class-specific render
     * traversal after variant-tag filtering.
     */
    int __fastcall gwNodeRenderDispatch(
        CZNodePartial * node,
        int siblingCountHint
    ) {
        const int variantId = node->nodeType;
        const int variantAllowed = VariantTag::CurrentAllowsId(variantId);
        if (variantAllowed == 0) {
            return variantAllowed;
        }

        switch (node->classId - 1) {
        case 4:
            return CZObject3D::RenderTraverse(node, siblingCountHint);
        case 5:
            return CZLod::RenderTraverse(node, siblingCountHint);
        case 8:
            return CZLight::RenderTraverse(node, siblingCountHint);
        case 9:
            return CZSound::RenderTraverse(node, siblingCountHint);
        case 0:
            return CZCamera::RenderTraverse(node, siblingCountHint);
        case 7:
            return CZAnimate::RenderTraverse(node, siblingCountHint);
        case 6:
            return CZSequence::RenderTraverse(node, siblingCountHint);
        case 10:
            return CZSwitch::RenderTraverse(node, siblingCountHint);
        default:
            return fprintf(stderr, "Unrecognized node rendering type: %s\n", node->name);
        }
    }

}

namespace CZCamera {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.fastanglexz
     * @recoil-artifact defines .text recoil:function:0x44c1b0: CZCamera::FastAngleXZ.
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.findconvexhullxz
     * @recoil-artifact defines .text recoil:function:0x44c230: CZCamera::FindConvexHullXZ.
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

        zError::ReportOld(
            0x200,
            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
            0x1049,
            g_CZClass_FindConvexHullUnexpectedReturnMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.buildfrustumgridtiles
     * @recoil-artifact defines .text recoil:function:0x44c3c0: CZCamera::BuildFrustumGridTiles
     * Purpose: build clamped in-world frustum grid rings from the active
     * camera footprint.
     */
    int __fastcall BuildFrustumGridTiles(
        CZNodePartial * world,
        CZWorldDataPartial * worldData,
        CZCameraDataPartial * cameraData
    ) {
        for (int ringIndex = 0; ringIndex < 50; ++ringIndex) {
            g_zCamera_FrustumGridTileRings[ringIndex].count = 0;
        }

        int originCol = 0;
        int originRow = 0;
        int result = CZWorld::WorldToGridCoordsClamped(
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
        zMath::MatLoadIdentity();
        zMath::MatTranslate(
            cameraData->cameraPos.x,
            cameraData->cameraPos.y,
            cameraData->cameraPos.z
        );
        zMath::MatRotateY(cameraData->eulerAngles.y);

        int pointCount;
        if (fabs(cameraData->eulerAngles.x) < 0.174533 &&
            fabs(cameraData->eulerAngles.z) < 0.174533) {
            pointCount = 3;
        } else {
            pointCount = 5;
            zMath::MatRotateX(cameraData->eulerAngles.x);
            zMath::MatRotateZ(cameraData->eulerAngles.z);
        }
        g_zCamera_FrustumFootprintPointCount = pointCount;
        memcpy(
            g_zCamera_FrustumFootprintPoints,
            &cameraData->frustumOrigin,
            pointCount * sizeof(zVec3)
        );
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            const zMat4x3 *matrix =
                (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
            for (int i = 0; i < pointCount; ++i) {
                const zVec3 point = g_zCamera_FrustumFootprintPoints[i];
                g_zCamera_FrustumFootprintPoints[i].x =
                    point.x * matrix->xx + point.y * matrix->yx +
                    point.z * matrix->zx + matrix->posX;
                g_zCamera_FrustumFootprintPoints[i].z =
                    point.x * matrix->xz + point.y * matrix->yz +
                    point.z * matrix->zz + matrix->posZ;
                g_zCamera_FrustumFootprintPoints[i].y =
                    point.x * matrix->xy + point.y * matrix->yy +
                    point.z * matrix->zy + matrix->posY;
            }
        }
        if (pointCount > 3) {
            pointCount = CZCamera::FindConvexHullXZ(
                g_zCamera_FrustumFootprintPoints,
                pointCount
            );
            g_zCamera_FrustumFootprintPointCount = pointCount;
        }
        if (CZDisplayInstance::FilterRegionsAgainstMeshFaces(
                g_zCamera_FrustumFootprintPoints,
                pointCount
            ) == 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt,
                "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                0x10ea
            );
            zError::EmitDebugBuffer(1);
        }

        float minX = g_zCamera_FrustumFootprintPoints[0].x;
        float maxX = g_zCamera_FrustumFootprintPoints[0].x;
        float minZ = g_zCamera_FrustumFootprintPoints[0].z;
        float maxZ = g_zCamera_FrustumFootprintPoints[0].z;
        for (int i = 1; i < g_zCamera_FrustumFootprintPointCount; ++i) {
            const zVec3 *point = &g_zCamera_FrustumFootprintPoints[i];
            if (point->x < minX) {
                minX = point->x;
            }
            if (point->x > maxX) {
                maxX = point->x;
            }
            if (point->z < minZ) {
                minZ = point->z;
            }
            if (point->z > maxZ) {
                maxZ = point->z;
            }
        }

        int minCol = 0;
        int minRow = 0;
        result = CZWorld::WorldToGridCoordsClamped(world, &minCol, minX, minZ, &minRow);
        if (result != 0) {
            zMath::MatStackPopPtr();
            return result;
        }

        int maxCol = 0;
        int maxRow = 0;
        result = CZWorld::WorldToGridCoordsClamped(world, &maxCol, maxX, maxZ, &maxRow);
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
                        if (CZDisplayInstance::FilterRegionsAgainstHexahedronFaces(
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

                        if (zVideoFrustumTestSphereClipMask(sphereCenter, bboxRadius, &clipMask) ==
                            0) {
                            const int colDelta = col - originCol;
                            const int rowDelta = row - originRow;
                            const int ringIndex =
                                (colDelta < 0 ? -colDelta : colDelta) +
                                (rowDelta < 0 ? -rowDelta : rowDelta);
                            if (ringIndex >= 50) {
                                zError::ReportOld(
                                    0x200,
                                    "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                                    0x11aa,
                                    g_CZClass_DiamondTilerNeedMoreRingsMsg
                                );
                            } else {
                                zCamera_FrustumGridTileRingPartial *ring =
                                    &g_zCamera_FrustumGridTileRings[ringIndex];
                                const int tileIndex = ring->count;
                                if (tileIndex >= 30) {
                                    zError::ReportOld(
                                        0x200,
                                        "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                                        0x11a4,
                                        g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg
                                    );
                                } else {
                                    ring->count = tileIndex + 1;
                                    zCamera_FrustumGridTilePartial *tile =
                                        &ring->tiles[tileIndex];
                                    tile->col = col;
                                    tile->row = row;
                                    tile->hasPosOffset = 0;
                                    tile->posOffsetX = 0.0f;
                                    tile->posOffsetZ = 0.0f;
                                    tile->clipMask = clipMask;
                                }
                            }
                        }
                    }
                }
            }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.buildfrustumgridtilesfromparams
     * @recoil-artifact defines .text recoil:function:0x44c8e0: CZCamera::BuildFrustumGridTilesFromParams
     * Purpose: build frustum grid rings while preserving raw out-of-bounds
     * grid offsets for wrapped/clamped world positions.
     */
    int __fastcall BuildFrustumGridTilesFromParams(
        CZNodePartial * world,
        CZWorldDataPartial * worldData,
        CZCameraDataPartial * cameraData
    ) {
        for (int ringIndex = 0; ringIndex < 50; ++ringIndex) {
            g_zCamera_FrustumGridTileRings[ringIndex].count = 0;
        }

        int originCol = 0;
        int originRow = 0;
        int originClampedCol = 0;
        int originClampedRow = 0;
        int originInsideBounds = 0;
        int result = CZWorld::WorldToGridCoordsClampedEx(
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
        zMath::MatLoadIdentity();
        zMath::MatTranslate(
            cameraData->cameraPos.x,
            cameraData->cameraPos.y,
            cameraData->cameraPos.z
        );
        zMath::MatRotateY(cameraData->eulerAngles.y);

        int pointCount;
        if (fabs(cameraData->eulerAngles.x) < 0.174533 &&
            fabs(cameraData->eulerAngles.z) < 0.174533) {
            pointCount = 3;
        } else {
            pointCount = 5;
            zMath::MatRotateX(cameraData->eulerAngles.x);
            zMath::MatRotateZ(cameraData->eulerAngles.z);
        }
        g_zCamera_FrustumFootprintPointCount = pointCount;
        memcpy(
            g_zCamera_FrustumFootprintPoints,
            &cameraData->frustumOrigin,
            pointCount * sizeof(zVec3)
        );
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            const zMat4x3 *matrix =
                (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
            for (int i = 0; i < pointCount; ++i) {
                const zVec3 point = g_zCamera_FrustumFootprintPoints[i];
                g_zCamera_FrustumFootprintPoints[i].x =
                    point.x * matrix->xx + point.y * matrix->yx +
                    point.z * matrix->zx + matrix->posX;
                g_zCamera_FrustumFootprintPoints[i].z =
                    point.x * matrix->xz + point.y * matrix->yz +
                    point.z * matrix->zz + matrix->posZ;
                g_zCamera_FrustumFootprintPoints[i].y =
                    point.x * matrix->xy + point.y * matrix->yy +
                    point.z * matrix->zy + matrix->posY;
            }
        }
        if (pointCount > 3) {
            pointCount = CZCamera::FindConvexHullXZ(
                g_zCamera_FrustumFootprintPoints,
                pointCount
            );
            g_zCamera_FrustumFootprintPointCount = pointCount;
        }
        if (CZDisplayInstance::FilterRegionsAgainstMeshFaces(
                g_zCamera_FrustumFootprintPoints,
                pointCount
            ) == 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt,
                "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                0x1279
            );
            zError::EmitDebugBuffer(1);
        }

        float minX = g_zCamera_FrustumFootprintPoints[0].x;
        float maxX = g_zCamera_FrustumFootprintPoints[0].x;
        float minZ = g_zCamera_FrustumFootprintPoints[0].z;
        float maxZ = g_zCamera_FrustumFootprintPoints[0].z;
        for (int i = 1; i < g_zCamera_FrustumFootprintPointCount; ++i) {
            const zVec3 *point = &g_zCamera_FrustumFootprintPoints[i];
            if (point->x < minX) {
                minX = point->x;
            }
            if (point->x > maxX) {
                maxX = point->x;
            }
            if (point->z < minZ) {
                minZ = point->z;
            }
            if (point->z > maxZ) {
                maxZ = point->z;
            }
        }

        int minCol = 0;
        int minRow = 0;
        int minClampedCol = 0;
        int minClampedRow = 0;
        int minInsideBounds = 0;
        int maxCol = 0;
        int maxRow = 0;
        int maxClampedCol = 0;
        int maxClampedRow = 0;
        int maxInsideBounds = 0;
        result = CZWorld::WorldToGridCoordsClampedEx(
            world,
            &minCol,
            minX,
            minZ,
            &minRow,
            &minClampedCol,
            &minClampedRow,
            &minInsideBounds
        );
        if (result == 0) {
            result = CZWorld::WorldToGridCoordsClampedEx(
                world,
                &maxCol,
                maxX,
                maxZ,
                &maxRow,
                &maxClampedCol,
                &maxClampedRow,
                &maxInsideBounds
            );
        }
        do {
            if (result != 0) {
                break;
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
                        if (CZDisplayInstance::FilterRegionsAgainstHexahedronFaces(
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
                            frustumVisible = zVideoFrustumTestSphereClipMask(
                                sphereCenter,
                                bboxRadius,
                                &clipMask
                            );
                        }

                        if (frustumVisible == 0) {
                            const int colDelta = col - originCol;
                            const int rowDelta = row - originRow;
                            const int ringIndex =
                                (colDelta < 0 ? -colDelta : colDelta) +
                                (rowDelta < 0 ? -rowDelta : rowDelta);
                            if (ringIndex >= 50) {
                                zError::ReportOld(
                                    0x200,
                                    "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                                    0x1357,
                                    g_CZClass_DiamondTilerNeedMoreRingsMsg
                                );
                            } else {
                                zCamera_FrustumGridTileRingPartial *ring =
                                    &g_zCamera_FrustumGridTileRings[ringIndex];
                                const int tileIndex = ring->count;
                                if (tileIndex >= 30) {
                                    zError::ReportOld(
                                        0x200,
                                        "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                                        0x1351,
                                        g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg
                                    );
                                } else {
                                    ring->count = tileIndex + 1;
                                    zCamera_FrustumGridTilePartial *tile =
                                        &ring->tiles[tileIndex];
                                    tile->col = areaCol;
                                    tile->row = areaRow;
                                    tile->hasPosOffset = hasPosOffset;
                                    tile->posOffsetX = posOffsetX;
                                    tile->posOffsetZ = posOffsetZ;
                                    tile->clipMask = clipMask;
                                }
                            }
                        }
                    }
                }
            }
        }
        } while (0);

        zMath::MatStackPopPtr();
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderfrustumgridtiles
     * @recoil-artifact defines .text recoil:function:0x44ce70: CZCamera::RenderFrustumGridTiles.
     * Purpose: render world grid tiles selected by the camera frustum.
     */
    int __fastcall RenderFrustumGridTiles(
        CZNodePartial * world,
        CZNodePartial * camera,
        CZCameraDataPartial * cameraData
    ) {
        CZWorldDataPartial *worldData = (CZWorldDataPartial *)(world->classData);
        int result = 0;

        if (worldData->clampQueriesToBounds != 0) {
            result = BuildFrustumGridTilesFromParams(world, worldData, cameraData);
        } else {
            result = BuildFrustumGridTiles(world, worldData, cameraData);
        }
        if (result != 0) {
            return result;
        }

        const int fogWasEnabled = zModelFogIsEnabled();
        float fogDistanceStart = 0.0f;
        if (fogWasEnabled != 0) {
            fogDistanceStart = zModelFogGetDistanceStart();
        }

        g_CZClass_RenderFrustumGridTileIndex = 0;
        int cameraAtBasePos = 1;
        {
            int ringIndex = 0;
            while (ringIndex < 50) {
                g_CZClass_RenderFrustumGridTileIndex = ringIndex;
                zCamera_FrustumGridTileRingPartial *ring =
                    &g_zCamera_FrustumGridTileRings[ringIndex];
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

                        if (g_CZClass_ObjectHseTestEnabled != 0 && ringIndex > 0 &&
                            zScene::TestProjectedSphereVisible(&center, area->bboxRadius) == 0) {
                            continue;
                        }

                        for (int lightIndex = 0; lightIndex < worldData->lightCount; ++lightIndex) {
                            CZNodePartial *lightNode = worldData->lightNodes[lightIndex];
                            if ((lightNode->flags & 0x04) == 0) {
                                continue;
                            }

                            CZLightDataPartial *lightData =
                                worldData->lightDataList[lightIndex];
                            if (lightData->isPointSource == 0 || lightData->enabled == 0) {
                                lightData->lightSubMode = 1;
                                continue;
                            }

                            const float dx = center.x - lightData->worldPosScratch.x;
                            const float dy = center.y - lightData->worldPosScratch.y;
                            const float dz = center.z - lightData->worldPosScratch.z;
                            const float range = lightData->range2 + area->bboxRadius;
                            const float distanceSq = dx * dx + dy * dy + dz * dz;
                            lightData->lightSubMode = range * range <= distanceSq ? 0 : 1;
                        }

                        if (fogWasEnabled != 0) {
                            const float dx = center.x - cameraData->cameraPos.x;
                            const float dy = center.y - cameraData->cameraPos.y;
                            const float dz = center.z - cameraData->cameraPos.z;
                            // Retail uses FSQRT before adding the area radius.
                            const float distanceSq = dx * dx + dy * dy + dz * dz;
                            float distance = (float)sqrt(distanceSq);
                            distance += area->bboxRadius * 1.10000002f;
                            zModelFogSetEnabled(distance < fogDistanceStart ? 0 : 1);
                        }

                        *gModel_ClipMaskStackTop = tile->clipMask;
                        if (tile->hasPosOffset == 0) {
                            for (int childIndex = 0; childIndex < area->childCount; ++childIndex) {
                                CZClass::gwNodeRenderDispatch(
                                    area->childList[childIndex],
                                    area->childCount
                                );
                            }
                        } else {
                            for (int childIndex = 0; childIndex < area->childCount; ++childIndex) {
                                CZNodePartial *child = area->childList[childIndex];
                                if (strstr(child->name, g_CZClass_VapStaticsNodeName) != 0) {
                                    CZClass::gwNodeRenderDispatch(child, area->childCount);
                                }
                            }
                        }
                    }
                }
                ++ringIndex;
                g_CZClass_RenderFrustumGridTileIndex = ringIndex;
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
            zModelFogSetEnabled(fogWasEnabled);
        }
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderoverlaynodes
     * @recoil-artifact defines .text recoil:function:0x44d200: CZCamera::RenderOverlayNodes.
     * @recoil-match byte
     *
     * Purpose: render overlay child nodes from the world node.
     */
    void __fastcall RenderOverlayNodes(CZNodePartial * world) {
        *gModel_ClipMaskStackTop = 0x3f;
        for (int i = 0; i < world->listCountB; ++i) {
            CZClass::gwNodeRenderDispatch(world->listB[i], 2);
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderworld
     * @recoil-artifact defines .text recoil:function:0x44d240: CZCamera::RenderWorld.
     * @recoil-match byte
     *
     * Purpose: render frustum grid tiles and overlay nodes for the world.
     */
    void __fastcall RenderWorld(
        CZNodePartial * world,
        CZNodePartial * camera,
        CZCameraDataPartial * cameraData
    ) {
        RenderFrustumGridTiles(world, camera, cameraData);
        RenderOverlayNodes(world);
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetvarianttagoverride
     * @recoil-artifact defines .text recoil:function:0x44d260: CZCamera::gwCameraSetVariantTagOverride.
     * Purpose: validate and store the camera variant tag override.
     */
    gwCameraSetVariantTagOverride(
        CZNodePartial * camera,
        zTag4Partial * variantTag
    ) {
        CZCameraDataPartial *data = 0;
        const int validateResult = ValidateCameraNode(camera, &data, 0x1527, 0x1528, 0x1529);
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.syncviewcontextpositions
     * @recoil-artifact defines .text recoil:function:0x44d320: CZCamera::SyncViewContextPositions.
     * @recoil-match byte
     *
     * Purpose: synchronize horizon helper nodes with the active view context.
     * Preserve the horizon-XZ helper height while updating its X and Z.
     */
    void __cdecl SyncViewContextPositions() {
        int updatedAnyNode = 0;

        if (g_zVideo_pActiveViewContext->horizonNode != 0) {
            CZObject3D::gwObject3DSetPosition(
                g_zVideo_pActiveViewContext->horizonNode,
                g_zVideo_pActiveViewContext->cameraPos.x,
                g_zVideo_pActiveViewContext->cameraPos.y,
                g_zVideo_pActiveViewContext->cameraPos.z
            );
            updatedAnyNode = 1;
        }

        if (g_zVideo_pActiveViewContext->horizonXZNode != 0) {
            float horizonX;
            float preservedY;
            float horizonZ;
            CZObject3D::gwObject3DGetPosition(
                g_zVideo_pActiveViewContext->horizonXZNode,
                &horizonX,
                &preservedY,
                &horizonZ
            );
            CZObject3D::gwObject3DSetPosition(
                g_zVideo_pActiveViewContext->horizonXZNode,
                g_zVideo_pActiveViewContext->cameraPos.x,
                preservedY,
                g_zVideo_pActiveViewContext->cameraPos.z
            );
            updatedAnyNode = 1;
        }

        if (updatedAnyNode != 0) {
            CZClass::gwNodeUpdateAll();
        }
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderscene
     * @recoil-artifact defines .text recoil:function:0x44d3a0: CZCamera::RenderScene.
     * Purpose: update camera scene state and render the active world.
     */
    RenderScene(
        CZNodePartial * camera,
        int updateFxPass3Local
    ) {
        const int queuedLensFlareSampleCount = zRndrLensFlareGetQueuedSampleCount();
        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)&slotBuffer);

        g_zVideo_pActiveViewContext = (CZCameraDataPartial *)(camera->classData);
        CZNodePartial *world = gwCameraGetWorld(camera);
        CZCameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
        CZWindowDataPartial *windowData =
            (CZWindowDataPartial *)(viewContext->windowNode->classData);

        if (g_CZClass_CameraAutoClipDistanceAdjustEnabled != 0) {
            if (g_FrameDeltaTimeSec <= g_CZClass_CameraAutoClipDistanceThreshold) {
                g_CZClass_CameraAutoClipDistanceScale += g_CZClass_CameraAutoClipDistanceStep;
            } else {
                g_CZClass_CameraAutoClipDistanceScale -= g_CZClass_CameraAutoClipDistanceStep;
            }

            if (g_CZClass_CameraAutoClipDistanceScale > 1.0f) {
                g_CZClass_CameraAutoClipDistanceScale = 1.0f;
            } else if (g_CZClass_CameraAutoClipDistanceScale <
                       g_CZClass_CameraAutoClipDistanceMinScale) {
                g_CZClass_CameraAutoClipDistanceScale = g_CZClass_CameraAutoClipDistanceMinScale;
            }

            gwCameraSetClipDistance(camera, g_CZClass_CameraAutoClipDistanceScale);
        }

        CZWorld::InitLightPointInPolygonXZ(world);
        zVideo::ReturnSuccessStub();
        gwCameraUpdate(camera);
        SyncViewContextPositions();
        zVideoSetActiveViewContext(g_zVideo_pActiveViewContext);
        CZWorld::UpdateAllLights(world);
        CZWorld::UpdateAllSounds(world);

        g_CZClass_LodDistanceStateStackTop = 0;
        if (CZTypeList::CountNodes(8) > 1) {
            zRndr::SpanOcclusionResetFrame();
            if ((windowData->clearPolyIndexFlags & 0x80000000) != 0) {
                const int clearPolyCount = windowData->clearPolyIndexFlags & 0x7fffffff;
                for (int i = 0; i < clearPolyCount; ++i) {
                    CZWindowClearPoly *poly = &windowData->clearPolys[i];
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
                CZDisplayInstance::FindBestPickCandidateBelowPoint(
                    world,
                    &viewContext->cameraPos,
                    &pickCandidates
                );
                g_Variant_FilterEnabled = variantFilterEnabled;

                if (pickCandidates.candidateCount <= 0) {
                    g_Variant_CurrentTag = g_zVideo_pActiveViewContext->variantTag;
                } else if (pickCandidates.entries[0].variantTag.count > 0) {
                    g_zVideo_pActiveViewContext->variantTag = pickCandidates.entries[0].variantTag;
                    g_Variant_CurrentTag = pickCandidates.entries[0].variantTag;
                }
            }
            g_zVideo_ActiveViewVariantTag = g_zVideo_pActiveViewContext->variantTag;
        }

        RenderWorld(world, camera, g_zVideo_pActiveViewContext);
        zMath::MatStackPopPtr();
        zRndrFlushTransparentQueue();
        if (updateFxPass3Local != 0) {
            zVideo::FxPass3UpdateLocal(g_FrameDeltaTimeSec);
        }
        zRndrFlushOverwriteQueue();
        zRndrLensFlareDrawQueuedSamples16AndBuildVisibleList(queuedLensFlareSampleCount);
        zRndrLensFlareDrawVisibleSamples();
        zRndrFlushTransparentQueue();
        zRndrOverlayRectFlushSw();
        zTag4::Clear(&g_zVideo_pActiveViewContext->variantTag);

        return 0;
    }

}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.zvideo-sw-renderframe
 * @recoil-artifact defines .text recoil:function:0x44d600: zVideo_sw::RenderFrame.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: BN writes the render-frame active view context at 0x5398fc,
 * updates the active variant tag at 0x5398f8, dispatches the three renderer
 * flush callbacks at 0x56bc6c..0x56bc74, and brackets rendering through the
 * scene-depth owner at 0x632148.
 * Purpose: provide the recovered zVideoswRenderFrame behavior.
 */
int __fastcall zVideoswRenderFrame(
    CZNodePartial *camera,
    int updateFxPass3Local
) {
    const int queuedLensFlareSampleCount = zRndrLensFlareGetQueuedSampleCount();
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)&slotBuffer);

    g_zVideo_pActiveViewContext = (CZCameraDataPartial *)(camera->classData);
    CZNodePartial *world = CZCamera::gwCameraGetWorld(camera);
    CZCameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
    CZWindowDataPartial *windowData =
        (CZWindowDataPartial *)(viewContext->windowNode->classData);

    if (g_CZClass_CameraAutoClipDistanceAdjustEnabled != 0) {
        if (g_FrameDeltaTimeSec <= g_CZClass_CameraAutoClipDistanceThreshold) {
            g_CZClass_CameraAutoClipDistanceScale += g_CZClass_CameraAutoClipDistanceStep;
        } else {
            g_CZClass_CameraAutoClipDistanceScale -= g_CZClass_CameraAutoClipDistanceStep;
        }

        if (g_CZClass_CameraAutoClipDistanceScale > 1.0f) {
            g_CZClass_CameraAutoClipDistanceScale = 1.0f;
        } else if (g_CZClass_CameraAutoClipDistanceScale < g_CZClass_CameraAutoClipDistanceMinScale) {
            g_CZClass_CameraAutoClipDistanceScale = g_CZClass_CameraAutoClipDistanceMinScale;
        }

        CZCamera::gwCameraSetClipDistance(camera, g_CZClass_CameraAutoClipDistanceScale);
    }

    CZWorld::InitLightPointInPolygonXZ(world);
    zVideo::ReturnSuccessStub();
    CZCamera::gwCameraUpdate(camera);
    CZCamera::SyncViewContextPositions();
    zVideoSetActiveViewContext(g_zVideo_pActiveViewContext);
    CZWorld::UpdateAllLights(world);
    CZWorld::UpdateAllSounds(world);

    const int variantFilterEnabled = g_Variant_FilterEnabled;
    g_CZClass_LodDistanceStateStackTop = 0;
    PlayerProbeSampleCandidateBuffer pickCandidates = {0};
    if (variantFilterEnabled != 0) {
        viewContext = g_zVideo_pActiveViewContext;
        if (viewContext->variantOverrideEnabled != 0 && variantFilterEnabled == 1) {
            g_Variant_CurrentTag = viewContext->variantTag;
        } else {
            g_Variant_FilterEnabled = 0;
            CZDisplayInstance::FindBestPickCandidateBelowPoint(
                world,
                &viewContext->cameraPos,
                &pickCandidates
            );
            g_Variant_FilterEnabled = variantFilterEnabled;

            if (pickCandidates.candidateCount <= 0) {
                zTag4::Clear(&g_zVideo_pActiveViewContext->variantTag);
                viewContext = g_zVideo_pActiveViewContext;
                g_Variant_CurrentTag = viewContext->variantTag;
            } else if (pickCandidates.entries[0].variantTag.count > 0) {
                g_zVideo_pActiveViewContext->variantTag = pickCandidates.entries[0].variantTag;
                viewContext = g_zVideo_pActiveViewContext;
                g_Variant_CurrentTag = pickCandidates.entries[0].variantTag;
            }
        }

        viewContext = g_zVideo_pActiveViewContext;
        g_zVideo_ActiveViewVariantTag = viewContext->variantTag;
    }

    zVideoD3D::SceneEnter();
    CZCamera::RenderWorld(world, camera, g_zVideo_pActiveViewContext);
    zMath::MatStackPopPtr();

    g_zVideo_pfnFlushSortedPolys();
    if (updateFxPass3Local != 0) {
        zVideo::FxPass3UpdateLocal(g_FrameDeltaTimeSec);
    }
    g_zVideo_pfnFlushSortedPolys();
    g_zVideo_pfnFlushOverwritePolys();

    const int visibleLensFlareSampleCount =
        zRndrLensFlareBuildVisibleSampleListFromQueue(queuedLensFlareSampleCount);
    for (int sampleIndex = 0; sampleIndex < visibleLensFlareSampleCount; ++sampleIndex) {
        zVec3 visibleSamplePoint = {0};
        zRndrSpanOcclusionFilterSampleList(sampleIndex, &visibleSamplePoint);
        CZDisplayInstance::SetStopAfterFirstHit(0x40000);
        CZDisplayInstance::SetBreakOnFirstCandidate(1);
        viewContext = g_zVideo_pActiveViewContext;
        const int raycastHit = CZDisplayInstance::RaycastFindClosest(
            viewContext->worldNode,
            &pickCandidates,
            viewContext->cameraPos.x,
            viewContext->cameraPos.y,
            viewContext->cameraPos.z,
            visibleSamplePoint.x,
            visibleSamplePoint.y,
            visibleSamplePoint.z
        );
        CZDisplayInstance::SetBreakOnFirstCandidate(0);
        if (raycastHit != 0 || pickCandidates.candidateCount == 0) {
            zRndrLensFlareDrawVisibleSample(sampleIndex);
        }
    }

    g_zVideo_pfnFlushSortedPolys();
    g_zVideo_pfnFlushOverwritePolys();
    g_zVideo_pfnFlushQuadBatch();
    zVideoD3D::SceneLeave();

    if (CZTypeList::CountNodes(8) > 1 && (windowData->clearPolyIndexFlags & 0x80000000) != 0) {
        const int clearPolyCount = windowData->clearPolyIndexFlags & 0x7fffffff;
        for (int i = 0; i < clearPolyCount; ++i) {
            CZWindowClearPoly *poly = &windowData->clearPolys[i];
            if ((poly->vertCount & 0x80000000) == 0) {
                continue;
            }

            const int vertexCount = poly->vertCount & 0x7fffffff;
            if (vertexCount <= 0) {
                continue;
            }

            zVidRect32 rect;
            rect.left = (int)(poly->vertices[0].x);
            rect.right = rect.left;
            rect.top = (int)(poly->vertices[0].y);
            rect.bottom = rect.top;

            for (int vertexIndex = 1; vertexIndex < vertexCount; ++vertexIndex) {
                const zVec3 *vertex = &poly->vertices[vertexIndex];
                if (rect.left > vertex->x) {
                    rect.left = (int)(vertex->x);
                }
                if (rect.right < vertex->x) {
                    rect.right = (int)(vertex->x);
                }
                if (rect.top > vertex->y) {
                    rect.top = (int)(vertex->y);
                }
                if (rect.bottom < vertex->y) {
                    rect.bottom = (int)(vertex->y);
                }
            }

            zVideo_dd3d::CallClearZBufferRect(&rect);
        }
    }

    return 0;
}

namespace CZCamera {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-camera-delete-node: CZCamera::DeleteNode
     * Purpose: route camera deletion through the generic node free path.
     */
    int __fastcall DeleteNode(CZNodePartial * node) {
        return CZClass::TryFreeNode(node);
    }
}
