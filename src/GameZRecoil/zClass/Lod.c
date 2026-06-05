#include "zClass.h"

#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdlib.h>
#include <string.h>

namespace {
    const int kZClassNodeLod = 6;

    /**
     * Original static helper recovered with zClass_Lod::RenderTraverse.
     *
     * Purpose: approximate the square root used by LOD distance and fade
     * scaling with the original bit-level floating-point estimate.
     */
    float ApproximateSqrt(float value) {
        int bits = 0;
        memcpy(
            &bits,
            &value,
            sizeof(bits)
        );
        bits = (bits >> 1) + 0x1fc00000;
        float result = 0.0f;
        memcpy(
            &result,
            &bits,
            sizeof(result)
        );
        return result;
    }

    /**
     * Original static helper recovered with zClass_Lod::RenderTraverse.
     *
     * Purpose: refresh the node view-space bounds when needed, push the LOD
     * bounds context, and test the current distance against the LOD range.
     */
    int EnsureLodSphereAndDistance(
        zClass_NodePartial * node,
        zClass_LodDataPartial * data,
        int *boundsContextPushed
    ) {
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

        if (g_zClass_RenderBoundsContextActive == 0) {
            *boundsContextPushed = 1;
            g_zClass_RenderBoundsContextActive = 1;
        }

        zClass_LodDistanceState &state =
            g_zClass_LodDistanceStateStack[g_zClass_LodDistanceStateStackTop];
        if (data->computeOwnDistance != 0) {
            state.center = *zClass_NodeViewSphereCenter(node);
            zVec3 delta = {0};
            delta.x = g_zVideo_pActiveViewContext->cameraPos.x - state.center.x;
            delta.y = g_zVideo_pActiveViewContext->cameraPos.y - state.center.y;
            delta.z = g_zVideo_pActiveViewContext->cameraPos.z - state.center.z;
            state.distanceSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        }

        return state.distanceSq >= data->nearRangeSq && state.distanceSq < data->farRangeSq;
    }

    /**
     * Original static helper recovered with zClass_Lod::RenderTraverse.
     *
     * Purpose: push a render alpha-scale override and publish it to the model
     * renderer's current alpha-scale state.
     */
    void PushAlphaScale(float scale) {
        ++g_zClass_RenderAlphaScaleStackTop;
        g_zClass_RenderAlphaScaleStack[g_zClass_RenderAlphaScaleStackTop] = scale;
        zModel_RenderAlphaScale_SetCurrent(scale);
    }

    /**
     * Original static helper recovered with zClass_Lod::RenderTraverse.
     *
     * Purpose: pop the render alpha-scale override and restore the previous
     * scale, or the default opaque scale when the stack is empty.
     */
    void PopAlphaScale() {
        --g_zClass_RenderAlphaScaleStackTop;
        const float scale = g_zClass_RenderAlphaScaleStackTop >= 0
                                ? g_zClass_RenderAlphaScaleStack[g_zClass_RenderAlphaScaleStackTop]
                                : 1.0f;
        zModel_RenderAlphaScale_SetCurrent(scale);
    }
}

namespace zClass_Lod {
    int __fastcall
    /**
     * Reimplements 0x44b8c0: zClass_Lod::RenderTraverse
     * (D:\Proj\GameZRecoil\zClass\Lod.c).
     *
     * Purpose: cull and render an LOD node, applying range, scale, alpha, and
     * vertex-alpha fades while maintaining the render traversal stacks.
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

        zClass_LodDataPartial *data = (zClass_LodDataPartial *)(node->classData);
        node->flags = flags & ~0x02000000;
        zClass_LodDistanceState &state =
            g_zClass_LodDistanceStateStack[g_zClass_LodDistanceStateStackTop];
        if (data->computeOwnDistance == 0 &&
            (state.distanceSq < data->nearRangeSq || state.distanceSq >= data->farRangeSq)) {
            return 0;
        }

        if (EnsureLodSphereAndDistance(
            node,
            data,
            &boundsContextPushed
        ) == 0) {
            if (boundsContextPushed != 0) {
                g_zClass_RenderBoundsContextActive = 0;
            }
            return 0;
        }

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;
        int pushScaleMatrix = 0;
        int pushAlphaScale = 0;
        float alphaScale = 1.0f;

        float distance = ApproximateSqrt(state.distanceSq);
        if (distance < data->nearRange) {
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

        if (data->vertexShadingAmount > 0.01f && distance < data->fogStartDist) {
            pushAlphaScale = 1;
            alphaScale = (data->nearRange - distance) / data->fogStartDist;
        }
        if (data->fogFadeAmount > 0.01f) {
            const float nearDistance = ApproximateSqrt(data->nearRangeSq);
            if (distance < nearDistance) {
                distance = nearDistance;
            }
            if (distance < nearDistance + data->fogFadeWidth) {
                const float fogScale = (distance - nearDistance) / data->fogFadeWidth;
                if (fogScale < alphaScale) {
                    alphaScale = fogScale;
                }
                pushAlphaScale = 1;
            }
        }

        int clipMask = *gModel_ClipMaskStackTop;
        int result = 0;
        if (clipMask != 0 && siblingCountHint > 1) {
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
            node->flags |= 0x80000000;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;

            if (data->rangeNode != 0) {
                const float fadeBegin = data->farRangeSq - data->rangeSq;
                g_zClass_RenderRangeFadeActive = 1;
                if (fadeBegin < state.distanceSq) {
                    g_zClass_RenderRangeFadeScale =
                        (state.distanceSq - fadeBegin) / (data->farRangeSq - fadeBegin);
                } else {
                    g_zClass_RenderRangeFadeScale = 0.0f;
                }
            }

            const int nextLodStack = g_zClass_LodDistanceStateStackTop + 1;
            g_zClass_LodDistanceStateStack[nextLodStack] =
                g_zClass_LodDistanceStateStack[g_zClass_LodDistanceStateStackTop];
            g_zClass_LodDistanceStateStackTop = nextLodStack;

            if (pushScaleMatrix != 0) {
                zMat4x3 slotBuffer = {0};
                zMath::MatStackPushAndCloneParent((float *)&slotBuffer);
                zMath_Mat_Scale(
                    scaleX,
                    scaleY,
                    scaleZ
                );
            }
            if (pushAlphaScale != 0) {
                PushAlphaScale(alphaScale);
            }

            int pushedVertexAlpha = 0;
            if ((node->flags & 0x00800000) != 0 && g_zClass_RenderVertexAlphaOverrideActive == 0) {
                pushedVertexAlpha = 1;
                g_zClass_RenderVertexAlphaOverrideActive = 1;
                zModel_RenderVertexAlphaEnabled_SetCurrent(1);
            }

            for (int i = 0; i < node->listCountB; ++i) {
                zClass_Class::gwNodeRenderDispatch(
                    node->listB[i],
                    node->listCountB
                );
            }

            if (pushScaleMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            if (pushAlphaScale != 0) {
                PopAlphaScale();
            }
            if (pushedVertexAlpha != 0) {
                g_zClass_RenderVertexAlphaOverrideActive = 0;
                zModel_RenderVertexAlphaEnabled_SetCurrent(0);
            }
            --g_zClass_LodDistanceStateStackTop;
            g_zClass_RenderRangeFadeActive = 0;
            --gModel_ClipMaskStackTop;
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        return result;
    }

    /**
     * Reimplements 0x4542a0: zClass_Lod::gwLodNew
     * (D:\Proj\GameZRecoil\zClass\Lod.c).
     *
     * Purpose: allocate an LOD node, attach zeroed LOD class data, and seed the
     * original default range and active-distance settings.
     */
    zClass_NodePartial *gwLodNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        node->classId = kZClassNodeLod;

        zClass_LodDataPartial *data =
            (zClass_LodDataPartial *)(calloc(
                1,
                sizeof(zClass_LodDataPartial)
            ));
        node->classData = data;
        data->computeOwnDistance = 1;
        data->nearRange = 1000.0f;
        data->farRangeSq = 1000000.0f;
        data->active = 1;
        return node;
    }

    int __fastcall
    /**
     * Reimplements 0x454310: zClass_Lod::gwLodAddChild
     * (D:\Proj\GameZRecoil\zClass\Lod.c).
     *
     * Purpose: append a child to an LOD node using the shared zClass child-list
     * helper.
     */
    gwLodAddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ){
        return zClass_Class::AddChildGeneric(
            parent,
            child
        );
    }

    int __fastcall
    /**
     * Reimplements 0x454320: zClass_Lod::RemoveChild
     * (D:\Proj\GameZRecoil\zClass\Lod.c).
     *
     * Purpose: remove a child from an LOD node through the shared zClass
     * child-list helper and return success.
     */
    RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ){
        zClass_Class::RemoveChildGeneric(
            parent,
            child
        );
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x454330: zClass_Lod::SetComputeOwnDistance
     * (D:\Proj\GameZRecoil\zClass\Lod.c).
     *
     * Purpose: update whether this LOD node computes its own camera distance
     * during render traversal.
     */
    SetComputeOwnDistance(
        zClass_NodePartial * node,
        int enabled
    ){
        ((zClass_LodDataPartial *)(node->classData))->computeOwnDistance = enabled;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x454340: zClass_Lod::SetTargetNodeAndRange
     * (D:\Proj\GameZRecoil\zClass\Lod.c).
     *
     * Purpose: assign the range-fade target node and cache the squared fade
     * range when a target is present.
     */
    SetTargetNodeAndRange(
        zClass_NodePartial * node,
        zClass_NodePartial * target,
        float range
    ){
        zClass_LodDataPartial *data = (zClass_LodDataPartial *)(node->classData);
        data->rangeNode = target;
        if (target != 0) {
            data->rangeSq = range * range;
        }

        return 0;
    }
}
