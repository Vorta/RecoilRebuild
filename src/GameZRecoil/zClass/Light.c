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
}

namespace CZLight {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightnew
     * @recoil-artifact defines .text recoil:function:0x452fd0: CZLight::gwLightNew
     *
     *
     * Purpose: allocate and initialize a light node, its light-class data,
     * default bounds, modes, color, range, and type-list membership.
     */
    CZNodePartial *__cdecl gwLightNew() {
        CZNodePartial *node = CZClass::gwNodeNew();
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x96, "Null node pointer.");
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

        CZLightDataPartial *data =
            (CZLightDataPartial *)(calloc(1, sizeof(CZLightDataPartial)));
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
        data->isDirectional = 0;
        data->isDirectedSource = 0;
        data->isPointSource = 1;
        data->lightParam = 1;
        data->lightSubMode = 1;
        data->range1 = 32.0f;
        data->range2 = 64.0f;
        data->range2Sq = 4096.0f;
        data->invRangeDelta = 0.03125f;
        data->dirty = 1;
        CZClass::gwNodeSetActive(node, 1);
        data->attachedWorldCount = 0;
        data->attachedWorlds = 0;

        CZTypeList::Insert(9, node);
        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.deletenode
     * @recoil-artifact defines .text recoil:function:0x453110: CZLight::DeleteNode
     * @recoil-match byte
     *
     * Purpose: validate light-owned class data, reject deletion while attached
     * to worlds, release the world attachment list, and return the node storage.
     */
    int __fastcall DeleteNode(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0xf8, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data = (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0xf9, "Null class data pointer");
            return 5;
        }
        if (data->attachedWorldCount > 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR deleting light; Light attached to %d world nodes.\n",
                "D:\\Proj\\GameZRecoil\\zClass\\Light.c",
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

        return CZClass::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.removechild
     * @recoil-artifact defines .text recoil:function:0x4531c0: CZLight::RemoveChild
     * @recoil-match byte
     *
     * Purpose: validate parent and child light-node pointers before delegating
     * removal to the generic zClass child-list helper.
     */
    int __fastcall RemoveChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x127, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x128, "Null node pointer.");
            return 5;
        }
        return CZClass::RemoveChildGeneric(parent, child);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetintensity
     * @recoil-artifact defines .text recoil:function:0x453200: CZLight::gwLightSetIntensity
     * @recoil-match byte
     *
     * Purpose: validate light data, store the intensity scale, and mark the
     * light transform/state dirty.
     */
    int __fastcall gwLightSetIntensity(
        CZNodePartial * node,
        float intensity
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x157, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x158, "Null class data pointer");
            return 5;
        }
        data->dirty = 1;
        data->intensityScale = intensity;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetfalloff
     * @recoil-artifact defines .text recoil:function:0x453250: CZLight::gwLightSetFalloff
     * @recoil-match byte
     *
     * Purpose: validate light data, store the falloff value, and mark the light
     * transform/state dirty.
     */
    int __fastcall gwLightSetFalloff(
        CZNodePartial * node,
        float falloff
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x176, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x177, "Null class data pointer");
            return 5;
        }
        data->dirty = 1;
        data->falloff = falloff;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetconeangle
     * @recoil-artifact defines .text recoil:function:0x4532a0: CZLight::gwLightSetDirectional
     * @recoil-match byte
     *
     * Purpose: store the 32-bit directional flag used by LightSetDirectional
     * and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetDirectional(
        CZNodePartial * node,
        int directional
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x196, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x197, "Null class data pointer");
            return 5;
        }
        data->isDirectional = directional;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetpointmode
     * @recoil-artifact defines .text recoil:function:0x4532f0: CZLight::gwLightSetDirectedSource
     * @recoil-match byte
     *
     * Purpose: select the directed light source used by LightSetDirectedSource,
     * disable the point source, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetDirectedSource(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x1b5, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x1b6, "Null class data pointer");
            return 5;
        }
        data->isDirectedSource = 1;
        data->isPointSource = 0;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetdirectionalmode
     * @recoil-artifact defines .text recoil:function:0x453350: CZLight::gwLightSetPointSource
     * @recoil-match byte
     *
     * Purpose: select the point light source used by LightSetPointSource,
     * disable the directed source, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetPointSource(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x1d5, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x1d6, "Null class data pointer");
            return 5;
        }
        data->isDirectedSource = 0;
        data->isPointSource = 1;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetparam
     * @recoil-artifact defines .text recoil:function:0x4533b0: CZLight::gwLightSetParam
     * @recoil-match byte
     *
     * Purpose: validate light data, store the light parameter selector, and mark
     * the light transform/state dirty.
     */
    int __fastcall gwLightSetParam(
        CZNodePartial * node,
        int param
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x1f2, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x1f3, "Null class data pointer");
            return 5;
        }
        data->lightParam = param;
        ((CZLightDataPartial *)node->classData)->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetrange
     * @recoil-artifact defines .text recoil:function:0x453400: CZLight::gwLightSetRange
     * @recoil-match byte
     *
     * Purpose: validate light data, order and store the two range values, repair
     * equal ranges with the original debug path, and cache range-derived values.
     */
    int __fastcall gwLightSetRange(
        CZNodePartial * node,
        float rangeA,
        float rangeB
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x211, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x212, "Null class data pointer");
            return 5;
        }
        data->range1 = rangeA < rangeB ? rangeA : rangeB;
        data->range2 = rangeA > rangeB ? rangeA : rangeB;
        if (rangeB == rangeA) {
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR setting light ranges; Range2 can't be equal to Range1.\n",
                "D:\\Proj\\GameZRecoil\\zClass\\Light.c",
                0x21c
            );
            data->range2 = data->range1 - (-10.0f);
        }

        const float delta = data->range2 - data->range1;
        data->invRangeDelta = 1.0f / delta;
        data->range2Sq = data->range2 * data->range2;
        data->dirty = 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightgetrange
     * @recoil-artifact defines .text recoil:function:0x453500: CZLight::gwLightGetRange
     * @recoil-match byte
     *
     * Purpose: validate light data and return the cached inner and outer light
     * range values.
     */
    int __fastcall gwLightGetRange(
        CZNodePartial * node,
        float *outRange1,
        float *outRange2
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x242, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x243, "Null class data pointer");
            return 5;
        }

        *outRange1 = data->range1;
        *outRange2 = data->range2;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetposition
     * @recoil-artifact defines .text recoil:function:0x453560: CZLight::gwLightSetPosition
     * @recoil-match byte
     *
     * Purpose: validate light data, store local position components, and mark
     * the light transform/state dirty.
     */
    int __fastcall gwLightSetPosition(
        CZNodePartial * node,
        float x,
        float y,
        float z
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x266, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x267, "Null class data pointer");
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
     * @recoil-artifact defines .text recoil:function:0x4535c0: CZLight::gwLightSetRotation
     * @recoil-match byte
     *
     * Purpose: validate light data, store local rotation components, and mark
     * the light transform/state dirty.
     */
    int __fastcall gwLightSetRotation(
        CZNodePartial * node,
        float x,
        float y,
        float z
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x2da, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x2db, "Null class data pointer");
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
     * @recoil-artifact defines .text recoil:function:0x453620: CZLight::ComputeWorldTransform
     *
     *
     * Purpose: build the node-to-world transform, update world position,
     * direction, and rotation caches, then restore the zMath matrix stack.
     */
    int __fastcall ComputeWorldTransform(CZNodePartial *node, CZLightDataPartial *data) {
        zVec3 localPoints[2] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
        zVec3 worldPoints[2];
        zMat4x3 slotBuffer;
        zMath::MatStackPushPtr((float *)&slotBuffer);
        zMath::MatLoadIdentity();
        CZNode::gwNodeBuildNodeToAncestorMatrix(node, 1);
        if (data->isDirectedSource == 0 && data->isDirectional == 0) {
            if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
                worldPoints[0] = localPoints[0];
            } else {
                for (int i = 0; i < 1; ++i) {
                    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
                    const zVec3 *point = &localPoints[i];
                    zVec3 *out = &worldPoints[i];
                    out->x = point->x * matrix->xx + point->y * matrix->yx + point->z * matrix->zx + matrix->posX;
                    out->y = point->x * matrix->xy + point->y * matrix->yy + point->z * matrix->zy + matrix->posY;
                    out->z = point->x * matrix->xz + point->y * matrix->yz + point->z * matrix->zz + matrix->posZ;
                }
            }
        } else {
            if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
                memcpy(worldPoints, localPoints, sizeof(localPoints));
            } else {
                for (int i = 0; i < 2; ++i) {
                    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
                    const zVec3 *point = &localPoints[i];
                    zVec3 *out = &worldPoints[i];
                    out->x = point->x * matrix->xx + point->y * matrix->yx + point->z * matrix->zx + matrix->posX;
                    out->y = point->x * matrix->xy + point->y * matrix->yy + point->z * matrix->zy + matrix->posY;
                    out->z = point->x * matrix->xz + point->y * matrix->yz + point->z * matrix->zz + matrix->posZ;
                }
            }
            zVec3 outAngles = zMath::Vec3DirectionAnglesBetweenPoints(&worldPoints[0], &worldPoints[1]);
            outAngles.z = 0.0f;
            data->worldRotation = outAngles;
        }
        data->worldPosition = worldPoints[0];
        data->worldDir.x = -slotBuffer.zx;
        data->worldDir.y = -slotBuffer.zy;
        data->worldDir.z = -slotBuffer.zz;
        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightupdate
     * @recoil-artifact defines .text recoil:function:0x453880: CZLight::gwLightUpdate
     *
     *
     * Purpose: validate dirty light nodes, refresh world/view transform caches
     * for point, cone, and directional modes, and clear the dirty flag.
     */
    int __fastcall gwLightUpdate(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x395, "Null node pointer.");
            return 5;
        }

        if ((node->flags & 0x04) == 0) {
            return 0;
        }

        CZLightDataPartial *data = (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x39b, "Null class data pointer");
            return 5;
        }

        zMat4x3 slotBuffer = {0};
        ComputeWorldTransform(node, data);
        zMath::MatStackPushAndCloneParent((float *)(&slotBuffer));
        zMath::MatLoadCameraScratchB();

        if (data->isDirectedSource != 0 || data->isDirectional != 0) {
            zMathMatTransformNormalBatch(&data->worldDir, &data->viewDir, 1);
            data->viewDir.x = -data->viewDir.x;
            data->viewDir.y = -data->viewDir.y;
            data->viewDir.z = -data->viewDir.z;
        }

        if (data->isPointSource != 0) {
            data->worldPosScratch = data->worldPosition;
            if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
                data->viewPos = data->worldPosScratch;
                zMath::MatStackPopPtr();
                data->dirty = 0;
                return 0;
            } else {
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
        data->dirty = 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightgetspecularcolor
     * @recoil-artifact defines .text recoil:function:0x453a40: CZLight::gwLightGetSpecularColor
     * @recoil-match byte
     *
     * Purpose: validate light data and return the stored specular RGB color.
     */
    int __fastcall gwLightGetSpecularColor(
        CZNodePartial * node,
        float *outRed,
        float *outGreen,
        float *outBlue
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x3ea, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x3eb, "Null class data pointer");
            return 5;
        }

        *outRed = data->specularColor.red;
        *outGreen = data->specularColor.green;
        *outBlue = data->specularColor.blue;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.light.gwlightsetspecularcolor
     * @recoil-artifact defines .text recoil:function:0x453aa0: CZLight::gwLightSetSpecularColor
     * @recoil-match byte
     *
     * Purpose: validate light data, store clamped/staged specular RGB color
     * state, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetSpecularColor(
        CZNodePartial * node,
        float red,
        float green,
        float blue
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x40f, "Null node pointer.");
            return 5;
        }
        CZLightDataPartial *data =
            (CZLightDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Light.c", 0x410, "Null class data pointer");
            return 5;
        }

        data->dirty = 1;
        data->specularColor.red = red;
        data->specularColor.green = green;
        data->specularColor.blue = blue;
        zRndrFogTargetColorStagedSetRgb01Clamped(&data->specularColor);
        return 0;
    }

}
