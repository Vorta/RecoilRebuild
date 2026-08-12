#include "zclass.h"

#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
    const int kZClassNodeLight = 9;
    const char kLightSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\Light.c";
}

namespace Light {
    /**
     * Purpose: provenance marker for the thermal-pool body emitted in the
     * literal-backed zwep_init.c physical contribution.
     */
    /**
     * Purpose: provenance marker for the thermal-pool body emitted in the
     * literal-backed zwep_init.c physical contribution.
     */
    /**
     * Purpose: provenance marker for the thermal-pool body emitted in the
     * literal-backed zwep_init.c physical contribution.
     */
    /**
     * Purpose: provenance marker for the thermal-pool body emitted in the
     * literal-backed zwep_init.c physical contribution.
     */



}

namespace zClass_Light {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightnew
     * @recoil-artifact defines .text recoil:function:0x452fd0: zClass_Light::gwLightNew
     * Purpose: allocate and initialize a light node, its light-class data,
     * default bounds, modes, color, range, and type-list membership.
     */
    zClass_NodePartial *__cdecl gwLightNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                0x96,
                "Null node pointer."
            );
            return 0;
        }

        node->cachedBounds[0] = 1.0f;
        node->cachedBounds[1] = 1.0f;
        node->cachedBounds[2] = -2.0f;
        node->cachedBounds[3] = 2.0f;
        node->cachedBounds[4] = 2.0f;
        node->cachedBounds[5] = -1.0f;
        node->flags |= 0x100;
        node->classId = kZClassNodeLight;

        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(calloc(
                1,
                sizeof(zClass_LightDataPartial)
            ));
        node->classData = data;

        data->worldDir.x = 0.0f;
        data->worldDir.y = 1.0f;
        data->worldDir.z = 0.0f;
        data->worldPosScratch.x = 0.0f;
        data->worldPosScratch.y = 0.0f;
        data->worldPosScratch.z = 0.0f;
        data->specularColor.red = 1.0f;
        data->specularColor.green = 1.0f;
        data->specularColor.blue = 1.0f;
        data->falloff = 0.0f;
        data->intensityScale = 1.0f;
        data->enabled = 1;
        data->coneAngle = 0.0f;
        data->isPointMode = 0;
        data->isDirectionalMode = 1;
        data->lightParam = 1;
        data->lightSubMode = 1;
        data->range1 = 32.0f;
        data->range2 = 64.0f;
        data->range2Sq = 4096.0f;
        data->invRangeDelta = 0.03125f;
        data->dirty = 1;
        zClass_Class::gwNodeSetActive(
            node,
            1
        );
        data->attachedWorldCount = 0;
        data->attachedWorlds = 0;

        zClass_TypeList::Insert(
            9,
            node
        );
        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.deletenode
     * @recoil-artifact defines .text recoil:function:0x453110: zClass_Light::DeleteNode
     * Purpose: validate light-owned class data, reject deletion while attached
     * to worlds, release the world attachment list, and return the node storage.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                0xf8,
                "Null node pointer."
            );
            return 5;
        }

        zClass_LightDataPartial *data = (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                0xf9,
                "Null class data pointer"
            );
            return 5;
        }

        if (data->attachedWorldCount > 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR deleting light; Light attached to %d world nodes.\n",
                kLightSourceFile,
                0x101,
                data->attachedWorldCount
            );
            zError::EmitDebugBuffer(1);
            return 1;
        }

        if (data->attachedWorlds != 0) {
            free(data->attachedWorlds);
            data->attachedWorlds = 0;
        }

        return zClass_Class::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.removechild
     * @recoil-artifact defines .text recoil:function:0x4531c0: zClass_Light::RemoveChild
     * Purpose: validate parent and child light-node pointers before delegating
     * removal to the generic zClass child-list helper.
     */
    int __fastcall RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                0x127,
                "Null node pointer."
            );
            return 5;
        }

        if (child == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                0x128,
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetintensity
     * @recoil-artifact defines .text recoil:function:0x453200: zClass_Light::gwLightSetIntensity
     * Purpose: validate light data, store the intensity scale, and mark the
     * light transform/state dirty.
     */
    int __fastcall gwLightSetIntensity(
        zClass_NodePartial * node,
        float intensity
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x157, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x158, "Null class data pointer");
            return 5;
        }

        data->dirty = 1;
        data->intensityScale = intensity;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetfalloff
     * @recoil-artifact defines .text recoil:function:0x453250: zClass_Light::gwLightSetFalloff
     * Purpose: validate light data, store the falloff value, and mark the light
     * transform/state dirty.
     */
    int __fastcall gwLightSetFalloff(
        zClass_NodePartial * node,
        float falloff
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x176, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x177, "Null class data pointer");
            return 5;
        }

        data->dirty = 1;
        data->falloff = falloff;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetconeangle
     * @recoil-artifact defines .text recoil:function:0x4532a0: zClass_Light::gwLightSetConeAngle
     * Purpose: validate light data, preserve the incoming cone-angle bit pattern
     * as a float, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetConeAngle(
        zClass_NodePartial * node,
        unsigned int coneAngleBits
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x196, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x197, "Null class data pointer");
            return 5;
        }

        memcpy(
            &data->coneAngle,
            &coneAngleBits,
            sizeof(data->coneAngle)
        );
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetpointmode
     * @recoil-artifact defines .text recoil:function:0x4532f0: zClass_Light::gwLightSetPointMode
     * Purpose: validate light data, enable point-light mode, disable directional
     * mode, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetPointMode(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x1b5, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x1b6, "Null class data pointer");
            return 5;
        }

        data->isPointMode = 1;
        data->isDirectionalMode = 0;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetdirectionalmode
     * @recoil-artifact defines .text recoil:function:0x453350: zClass_Light::gwLightSetDirectionalMode
     * Purpose: validate light data, enable directional-light mode, disable point
     * mode, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetDirectionalMode(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x1d5, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x1d6, "Null class data pointer");
            return 5;
        }

        data->isPointMode = 0;
        data->isDirectionalMode = 1;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetparam
     * @recoil-artifact defines .text recoil:function:0x4533b0: zClass_Light::gwLightSetParam
     * Purpose: validate light data, store the light parameter selector, and mark
     * the light transform/state dirty.
     */
    int __fastcall gwLightSetParam(
        zClass_NodePartial * node,
        int param
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x1f2, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x1f3, "Null class data pointer");
            return 5;
        }

        data->lightParam = param;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetrange
     * @recoil-artifact defines .text recoil:function:0x453400: zClass_Light::gwLightSetRange
     * Purpose: validate light data, order and store the two range values, repair
     * equal ranges with the original debug path, and cache range-derived values.
     */
    int __fastcall gwLightSetRange(
        zClass_NodePartial * node,
        float rangeA,
        float rangeB
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x211, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x212, "Null class data pointer");
            return 5;
        }

        data->range1 = rangeA < rangeB ? rangeA : rangeB;
        data->range2 = rangeA > rangeB ? rangeA : rangeB;
        if (data->range1 == data->range2) {
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR setting light ranges; Range2 can't be equal to Range1.\n",
                kLightSourceFile,
                0x21c
            );
            data->range2 = data->range1 + 10.0f;
        }

        const float delta = data->range2 - data->range1;
        data->invRangeDelta = 1.0f / delta;
        data->range2Sq = data->range2 * data->range2;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightgetrange
     * @recoil-artifact defines .text recoil:function:0x453500: zClass_Light::gwLightGetRange
     * Purpose: validate light data and return the cached inner and outer light
     * range values.
     */
    int __fastcall gwLightGetRange(
        zClass_NodePartial * node,
        float *outRange1,
        float *outRange2
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x242, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x243, "Null class data pointer");
            return 5;
        }

        *outRange1 = data->range1;
        *outRange2 = data->range2;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetposition
     * @recoil-artifact defines .text recoil:function:0x453560: zClass_Light::gwLightSetPosition
     * Purpose: validate light data, store local position components, and mark
     * the light transform/state dirty.
     */
    int __fastcall gwLightSetPosition(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x266, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x267, "Null class data pointer");
            return 5;
        }

        data->localPosition.x = x;
        data->localPosition.y = y;
        data->localPosition.z = z;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetrotation
     * @recoil-artifact defines .text recoil:function:0x4535c0: zClass_Light::gwLightSetRotation
     * Purpose: validate light data, store local rotation components, and mark
     * the light transform/state dirty.
     */
    int __fastcall gwLightSetRotation(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x2da, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x2db, "Null class data pointer");
            return 5;
        }

        data->localRotation.x = x;
        data->localRotation.y = y;
        data->localRotation.z = z;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.computeworldtransform
     * @recoil-artifact defines .text recoil:function:0x453620: zClass_Light::ComputeWorldTransform
     * Purpose: build the node-to-world transform, update world position,
     * direction, and rotation caches, then restore the zMath matrix stack.
     */
    int __fastcall ComputeWorldTransform(
        zClass_NodePartial * node,
        zClass_LightDataPartial * data
    ) {
        zVec3 localPointA = {0.0f, 0.0f, 0.0f};
        zVec3 localPointB = {0.0f, 0.0f, -1.0f};
        zMat4x3 slotBuffer = {0};

        zMath::MatStackPushPtr((float *)(&slotBuffer));
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(
            node,
            1
        );

        zVec3 pointA = localPointA;
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            const zMat4x3 *matrix =
                (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
            pointA.x = localPointA.x * matrix->xx + localPointA.y * matrix->yx
                + localPointA.z * matrix->zx + matrix->posX;
            pointA.y = localPointA.x * matrix->xy + localPointA.y * matrix->yy
                + localPointA.z * matrix->zy + matrix->posY;
            pointA.z = localPointA.x * matrix->xz + localPointA.y * matrix->yz
                + localPointA.z * matrix->zz + matrix->posZ;
        }

        if (data->isPointMode != 0 || data->coneAngle != 0.0f) {
            zVec3 pointB = localPointB;
            if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
                const zMat4x3 *matrix =
                    (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
                pointB.x = localPointB.x * matrix->xx + localPointB.y * matrix->yx
                    + localPointB.z * matrix->zx + matrix->posX;
                pointB.y = localPointB.x * matrix->xy + localPointB.y * matrix->yy
                    + localPointB.z * matrix->zy + matrix->posY;
                pointB.z = localPointB.x * matrix->xz + localPointB.y * matrix->yz
                    + localPointB.z * matrix->zz + matrix->posZ;
            }
            zVec3 outAngles = {0};
            zMath::Vec3DirectionAnglesBetweenPoints(
                &pointA,
                &pointB,
                &outAngles
            );
            outAngles.z = 0.0f;
            data->worldRotation = outAngles;
        }

        data->worldPosition = pointA;
        data->worldDir.x = -slotBuffer.zx;
        data->worldDir.y = -slotBuffer.zy;
        data->worldDir.z = -slotBuffer.zz;

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightupdate
     * @recoil-artifact defines .text recoil:function:0x453880: zClass_Light::gwLightUpdate
     * Purpose: validate dirty light nodes, refresh world/view transform caches
     * for point, cone, and directional modes, and clear the dirty flag.
     */
    int __fastcall gwLightUpdate(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                0x395,
                "Null node pointer."
            );
            return 5;
        }

        if ((node->flags & 0x04) == 0) {
            return 0;
        }

        zClass_LightDataPartial *data = (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                0x39b,
                "Null class data pointer"
            );
            return 5;
        }

        zMat4x3 slotBuffer = {0};
        ComputeWorldTransform(
            node,
            data
        );
        zMath::MatStackPushAndCloneParent((float *)(&slotBuffer));
        zMath::MatLoadCameraScratchB();

        if (data->isPointMode != 0 || data->coneAngle != 0.0f) {
            zMath_Mat_TransformNormalBatch(
                &data->worldDir,
                &data->viewDir,
                1
            );
            data->viewDir.x = -data->viewDir.x;
            data->viewDir.y = -data->viewDir.y;
            data->viewDir.z = -data->viewDir.z;
        }

        if (data->isDirectionalMode != 0) {
            data->worldPosScratch = data->worldPosition;
            data->viewPos = data->worldPosScratch;
            if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
                const zMat4x3 *matrix =
                    (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
                data->viewPos.x = data->worldPosScratch.x * matrix->xx
                    + data->worldPosScratch.y * matrix->yx
                    + data->worldPosScratch.z * matrix->zx + matrix->posX;
                data->viewPos.y = data->worldPosScratch.x * matrix->xy
                    + data->worldPosScratch.y * matrix->yy
                    + data->worldPosScratch.z * matrix->zy + matrix->posY;
                data->viewPos.z = data->worldPosScratch.x * matrix->xz
                    + data->worldPosScratch.y * matrix->yz
                    + data->worldPosScratch.z * matrix->zz + matrix->posZ;
            }
        }

        zMath::MatStackPopPtr();
        zMath::MatStackPopPtr();
        data->dirty = 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightgetspecularcolor
     * @recoil-artifact defines .text recoil:function:0x453a40: zClass_Light::gwLightGetSpecularColor
     * Purpose: validate light data and return the stored specular RGB color.
     */
    int __fastcall gwLightGetSpecularColor(
        zClass_NodePartial * node,
        float *outRed,
        float *outGreen,
        float *outBlue
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x3ea, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x3eb, "Null class data pointer");
            return 5;
        }

        *outRed = data->specularColor.red;
        *outGreen = data->specularColor.green;
        *outBlue = data->specularColor.blue;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetspecularcolor
     * @recoil-artifact defines .text recoil:function:0x453aa0: zClass_Light::gwLightSetSpecularColor
     * Purpose: validate light data, store clamped/staged specular RGB color
     * state, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetSpecularColor(
        zClass_NodePartial * node,
        float red,
        float green,
        float blue
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x40f, "Null node pointer.");
            return 5;
        }
        zClass_LightDataPartial *data =
            (zClass_LightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kLightSourceFile, 0x410, "Null class data pointer");
            return 5;
        }

        data->dirty = 1;
        data->specularColor.red = red;
        data->specularColor.green = green;
        data->specularColor.blue = blue;
        zRndr_FogTargetColorStaged_SetRgb01Clamped(&data->specularColor);
        return 0;
    }

}
