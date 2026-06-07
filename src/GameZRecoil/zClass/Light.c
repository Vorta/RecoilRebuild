#include "zClass.h"

#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
    const int kZClassNodeLight = 9;
    const char *kLightSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Light.c";

    // Source-faithful helper recovered from address-backed callers in this source file.
    zClass_LightDataPartial *GetLightData(
        zClass_NodePartial * node,
        int nullLine,
        int dataLine
    ){
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                nullLine,
                "Null node pointer."
            );
            return 0;
        }

        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kLightSourceFile,
                dataLine,
                "Null class data pointer"
            );
            return 0;
        }

        return (zClass_LightDataPartial *)(node->classData);
    }

    /**
     * Original static helper observed in callers 0x453620 and 0x453880
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: transform a point through the active zMath matrix unless the
     * active matrix identity flag says the point can be reused unchanged.
     */
    zVec3 TransformPoint(const zVec3 &point) {
        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            return point;
        }

        const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
        zVec3 out = {0};
        out.x = point.x * matrix->xx + point.y * matrix->yx + point.z * matrix->zx + matrix->posX;
        out.y = point.x * matrix->xy + point.y * matrix->yy + point.z * matrix->zy + matrix->posY;
        out.z = point.x * matrix->xz + point.y * matrix->yz + point.z * matrix->zz + matrix->posZ;
        return out;
    }

    /**
     * Original static helper observed in caller 0x44b140
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: refresh a node's view bounding sphere when needed, run the
     * frustum sphere clip-mask test, and honor no-near-clip render flags.
     */
    int CullNodeForRender(
        zClass_NodePartial * node,
        int siblingCountHint,
        int *clipMask
    ){
        int result = 0;
        if ((*clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
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
                clipMask,
                *zClass_NodeViewSphereRadius(node)
            );
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                *clipMask &= ~0x20;
            }
        }
        return result;
    }

    /**
     * Original static helper observed in caller 0x44b140
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: mark the node as rendered, apply range-fade DI state, render it,
     * and recurse through child render dispatch while preserving clip masks.
     */
    void RenderNodeAndChildren(
        zClass_NodePartial * node,
        int clipMask
    ){
        node->flags |= 0x80000000;
        zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
        if (di != 0 && g_zClass_RenderRangeFadeActive != 0) {
            di->flags |= 0x08;
            di->blendScale = g_zClass_RenderRangeFadeScale;
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
    }
}

namespace Light {
    /**
     * Reimplements 0x4b2160: Light::InitThermalGlowPool
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: allocate the fixed eight-node thermal glow light pool, initialize
     * names, positions, and ranges, then link every node onto the free list.
     */
    int InitThermalGlowPool() {
        for (int i = 0; i < 8; ++i) {
            zClass_NodePartial *const light = zClass_Light::gwLightNew();
            zClass_Class::gwNodeSetName(
                light,
                "Thermal glow"
            );
            zClass_Light::gwLightSetPosition(
                light,
                0.0f,
                0.0f,
                0.0f
            );
            zClass_Light::gwLightSetRange(
                light,
                0.1f,
                0.2f
            );
            light->callbackContext = g_OptCatalogThermalGlowFreeList;
            g_OptCatalogThermalGlowFreeList = light;
        }

        return 1;
    }

    /**
     * Reimplements 0x4b21e0: Light::DestroyThermalGlowPool
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: delete every thermal glow light still on the free list and clear
     * the pool head.
     */
    int DestroyThermalGlowPool() {
        zClass_NodePartial *node = g_OptCatalogThermalGlowFreeList;
        while (node != 0) {
            zClass_NodePartial *next = node->callbackContext;
            node->callbackContext = 0;
            zClass_Class::DeleteNodeByType(node);
            node = next;
        }

        g_OptCatalogThermalGlowFreeList = 0;
        return 1;
    }

    /**
     * Reimplements 0x4b2520: Light::AllocFromFreeListAndAttach
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: pop a thermal glow light from the free list, reset its range and
     * specular color, and attach it to the active runtime world.
     */
    zClass_NodePartial *__fastcall AllocFromFreeListAndAttach(
        zColorRgb * specularColor
    ) {
        zClass_NodePartial *const light = g_OptCatalogThermalGlowFreeList;
        if (light == 0) {
            return 0;
        }

        g_OptCatalogThermalGlowFreeList = light->callbackContext;
        zClass_Light::gwLightSetRange(
            light,
            0.1f,
            0.2f
        );
        zClass_Light::gwLightSetSpecularColor(
            light,
            specularColor->red,
            specularColor->green,
            specularColor->blue
        );
        zClass_World::AddLight(
            g_OptCatalogRuntimeWorld,
            light
        );
        return light;
    }

    /**
     * Reimplements 0x4b2570: Light::ReturnToFreeList
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: reset a thermal glow light's range, detach it from the runtime
     * world, and push it back onto the thermal glow free list.
     */
    void __fastcall ReturnToFreeList(zClass_NodePartial * lightNode) {
        zClass_Light::gwLightSetRange(
            lightNode,
            0.1f,
            0.2f
        );
        zClass_World::RemoveLight(
            g_OptCatalogRuntimeWorld,
            lightNode
        );
        lightNode->callbackContext = g_OptCatalogThermalGlowFreeList;
        g_OptCatalogThermalGlowFreeList = lightNode;
    }
}

namespace zClass_Light {
    /**
     * Reimplements 0x452fd0: zClass_Light::gwLightNew
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: allocate and initialize a light node, its light-class data,
     * default bounds, modes, color, range, and type-list membership.
     */
    zClass_NodePartial *gwLightNew() {
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

        data->worldDir = zVec3_Make(
            0.0f,
            1.0f,
            0.0f
        );
        data->worldPosScratch = zVec3_Make(
            0.0f,
            0.0f,
            0.0f
        );
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
        node->flags |= 0x04;
        data->attachedWorldCount = 0;
        data->attachedWorlds = 0;

        zClass_TypeList::Insert(
            9,
            node
        );
        return node;
    }

    /**
     * Reimplements 0x453110: zClass_Light::DeleteNode
     * (D:\Proj\GameZRecoil\zClass\Light.c).
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

    int __fastcall
    /**
     * Reimplements 0x4531c0: zClass_Light::RemoveChild
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate parent and child light-node pointers before delegating
     * removal to the generic zClass child-list helper.
     */
    RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ){
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

    int __fastcall
    /**
     * Reimplements 0x453200: zClass_Light::gwLightSetIntensity
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, store the intensity scale, and mark the
     * light transform/state dirty.
     */
    gwLightSetIntensity(
        zClass_NodePartial * node,
        float intensity
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x157,
            0x158
        );
        if (data == 0) {
            return 5;
        }

        data->dirty = 1;
        data->intensityScale = intensity;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x453250: zClass_Light::gwLightSetFalloff
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, store the falloff value, and mark the light
     * transform/state dirty.
     */
    gwLightSetFalloff(
        zClass_NodePartial * node,
        float falloff
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x176,
            0x177
        );
        if (data == 0) {
            return 5;
        }

        data->dirty = 1;
        data->falloff = falloff;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x4532a0: zClass_Light::gwLightSetConeAngle
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, preserve the incoming cone-angle bit pattern
     * as a float, and mark the light transform/state dirty.
     */
    gwLightSetConeAngle(
        zClass_NodePartial * node,
        unsigned int coneAngleBits
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x196,
            0x197
        );
        if (data == 0) {
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
     * Reimplements 0x4532f0: zClass_Light::gwLightSetPointMode
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, enable point-light mode, disable directional
     * mode, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetPointMode(zClass_NodePartial * node) {
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x1b5,
            0x1b6
        );
        if (data == 0) {
            return 5;
        }

        data->isPointMode = 1;
        data->isDirectionalMode = 0;
        data->dirty = 1;
        return 0;
    }

    /**
     * Reimplements 0x453350: zClass_Light::gwLightSetDirectionalMode
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, enable directional-light mode, disable point
     * mode, and mark the light transform/state dirty.
     */
    int __fastcall gwLightSetDirectionalMode(zClass_NodePartial * node) {
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x1d5,
            0x1d6
        );
        if (data == 0) {
            return 5;
        }

        data->isPointMode = 0;
        data->isDirectionalMode = 1;
        data->dirty = 1;
        return 0;
    }

    /**
     * Reimplements 0x4533b0: zClass_Light::gwLightSetParam
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, store the light parameter selector, and mark
     * the light transform/state dirty.
     */
    int __fastcall gwLightSetParam(
        zClass_NodePartial * node,
        int param
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x1f2,
            0x1f3
        );
        if (data == 0) {
            return 5;
        }

        data->lightParam = param;
        data->dirty = 1;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x453400: zClass_Light::gwLightSetRange
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, order and store the two range values, repair
     * equal ranges with the original debug path, and cache range-derived values.
     */
    gwLightSetRange(
        zClass_NodePartial * node,
        float rangeA,
        float rangeB
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x211,
            0x212
        );
        if (data == 0) {
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

    int __fastcall
    /**
     * Reimplements 0x453500: zClass_Light::gwLightGetRange
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data and return the cached inner and outer light
     * range values.
     */
    gwLightGetRange(
        zClass_NodePartial * node,
        float *outRange1,
        float *outRange2
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x242,
            0x243
        );
        if (data == 0) {
            return 5;
        }

        *outRange1 = data->range1;
        *outRange2 = data->range2;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x453560: zClass_Light::gwLightSetPosition
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, store local position components, and mark
     * the light transform/state dirty.
     */
    gwLightSetPosition(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x266,
            0x267
        );
        if (data == 0) {
            return 5;
        }

        data->localPosition = zVec3_Make(
            x,
            y,
            z
        );
        data->dirty = 1;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x4535c0: zClass_Light::gwLightSetRotation
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, store local rotation components, and mark
     * the light transform/state dirty.
     */
    gwLightSetRotation(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x2da,
            0x2db
        );
        if (data == 0) {
            return 5;
        }

        data->localRotation = zVec3_Make(
            x,
            y,
            z
        );
        data->dirty = 1;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x453620: zClass_Light::ComputeWorldTransform
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: build the node-to-world transform, update world position,
     * direction, and rotation caches, then restore the zMath matrix stack.
     */
    ComputeWorldTransform(
        zClass_NodePartial * node,
        zClass_LightDataPartial * data
    ){
        zVec3 localPointA = {0.0f, 0.0f, 0.0f};
        zVec3 localPointB = {0.0f, 0.0f, -1.0f};
        zMat4x3 slotBuffer = {0};

        zMath::MatStackPushPtr((float *)(&slotBuffer));
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(
            node,
            1
        );

        zVec3 pointA = {0};
        if (data->isPointMode != 0 || data->coneAngle != 0.0f) {
            pointA = TransformPoint(localPointA);
            zVec3 pointB = TransformPoint(localPointB);
            zVec3 outAngles = {0};
            zMath::Vec3DirectionAnglesBetweenPoints(
                &pointA,
                &pointB,
                &outAngles
            );
            outAngles.z = 0.0f;
            data->worldRotation = outAngles;
        } else {
            pointA = TransformPoint(localPointA);
        }

        data->worldPosition = pointA;
        data->worldDir.x = -slotBuffer.zx;
        data->worldDir.y = -slotBuffer.zy;
        data->worldDir.z = -slotBuffer.zz;

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * Reimplements 0x453880: zClass_Light::gwLightUpdate
     * (D:\Proj\GameZRecoil\zClass\Light.c).
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
            data->viewPos = TransformPoint(data->worldPosScratch);
        }

        zMath::MatStackPopPtr();
        data->dirty = 0;
        return 0;
    }

    /**
     * Reimplements 0x453a40: zClass_Light::gwLightGetSpecularColor
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data and return the stored specular RGB color.
     */
    int __fastcall gwLightGetSpecularColor(
        zClass_NodePartial * node,
        float *outRed,
        float *outGreen,
        float *outBlue
    ) {
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x3ea,
            0x3eb
        );
        if (data == 0) {
            return 5;
        }

        *outRed = data->specularColor.red;
        *outGreen = data->specularColor.green;
        *outBlue = data->specularColor.blue;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x453aa0: zClass_Light::gwLightSetSpecularColor
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: validate light data, store clamped/staged specular RGB color
     * state, and mark the light transform/state dirty.
     */
    gwLightSetSpecularColor(
        zClass_NodePartial * node,
        float red,
        float green,
        float blue
    ){
        zClass_LightDataPartial *data = GetLightData(
            node,
            0x40f,
            0x410
        );
        if (data == 0) {
            return 5;
        }

        data->dirty = 1;
        data->specularColor.red = red;
        data->specularColor.green = green;
        data->specularColor.blue = blue;
        zRndr_FogTargetColorStaged_SetRgb01Clamped(&data->specularColor);
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44b140: zClass_Light::RenderTraverse
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: cull an enabled light node, push render-bounds context when
     * needed, apply local transform, render the node subtree, and restore state.
     */
    RenderTraverse(
        zClass_NodePartial * node,
        int siblingCountHint
    ){
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        zClass_LightDataPartial *data = (zClass_LightDataPartial *)(node->classData);
        int clipMask = *gModel_ClipMaskStackTop;
        const int result = CullNodeForRender(
            node,
            siblingCountHint,
            &clipMask
        );
        if (g_zClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_zClass_RenderBoundsContextActive = 1;
        }

        if (result == 0) {
            const zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            zMath::MatStackPushAndCloneParent(data->savedParentMatrix);
            zMath::MatApplyLocalTRS(
                &data->localRotation,
                &data->localPosition,
                &unitScale
            );
            RenderNodeAndChildren(
                node,
                clipMask
            );
            zMath::MatStackPopPtr();
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        return result;
    }
}
