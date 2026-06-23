#include "zClass.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClipAlt.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdlib.h>
#include <string.h>

namespace {
    const int kZClassNodeObject3D = 5;
    const int kObject3DLitFlag = 0x02;
    const int kObject3DVisibleFlag = 0x04;
    const int kObject3DTransformDirtyFlag = 0x20;
    const int kNodeBoundsDirtyFlag = 0x04;
    const int kSingleParentFlag = 0x00080000;
    const int kNodeTransformDirtyPropagatedFlag = 0x02000000;

    const char *kObject3DSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c";

    /**
     * Original static helper observed in callers 0x438020 and 0x44dc30
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: clamp model-reference, color, and alpha inputs to the unit
     * interval used by object render state.
     */
    float ClampUnit(float value) {
        if (value < 0.0f) {
            return 0.0f;
        }
        if (value > 1.0f) {
            return 1.0f;
        }
        return value;
    }

    zClass_Object3DDataPartial *
    /**
     * Original static helper; no standalone retail function exists.
     * Observed in Object3D transform accessors/mutators at 0x44e270, 0x44e3d0,
     * 0x44e4f0, and 0x44e5b0 through the repeated validation/error-report
     * pattern in Object3d.c.
     * Purpose: validate node, class-data, and Object3D class id before returning
     * the node's Object3D data.
     */
    GetObject3DData(
        zClass_NodePartial * node,
        int nullLine,
        int dataLine,
        int classLine
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                nullLine,
                "Null node pointer."
            );
            return 0;
        }

        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                dataLine,
                "Null class data pointer"
            );
            return 0;
        }

        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                classLine,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                kZClassNodeObject3D,
                node->classId
            );
            return 0;
        }

        return (zClass_Object3DDataPartial *)(node->classData);
    }

    /**
     * Original static helper; no standalone retail function exists.
     * Observed in Object3D transform accessors/mutators at 0x44dfd0, 0x44e030,
     * 0x44e110, and 0x44e300 through the repeated validation/error-report
     * pattern in Object3d.c.
     * Purpose: validate node and class-data pointers before returning Object3D
     * data for callers that do not perform a class-id check.
     */
    zClass_Object3DDataPartial *GetObject3DDataNoClassCheck(
        zClass_NodePartial * node,
        int nullLine,
        int dataLine
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                nullLine,
                "Null node pointer."
            );
            return 0;
        }

        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                dataLine,
                "Null class data pointer"
            );
            return 0;
        }

        return (zClass_Object3DDataPartial *)(node->classData);
    }

    /**
     * Original static helper observed in Object3D transform-mutator callers
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: mark Object3D transform state dirty, propagate dirty bounds to
     * descendants, and enqueue the node on the transform-update type list.
     */
    void QueueTransformUpdate(
        zClass_NodePartial * node,
        zClass_Object3DDataPartial * data
    ) {
        data->flags |= 0x01;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(
                7,
                node
            );
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
    }

    void
    /**
     * Original static helper observed in callers 0x44e030, 0x44e170, 0x44e300,
     * and 0x44e3d0 (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: clear the identity-transform flag when any supplied component is
     * nonzero.
     */
    ClearIdentityFlagIfAnyNonZero(
        zClass_Object3DDataPartial * data,
        float x,
        float y,
        float z
    ) {
        if ((data->flags & 0x08) != 0 && (x != 0.0f || y != 0.0f || z != 0.0f)) {
            data->flags &= ~0x08;
        }
    }

    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
     * BN keeps the bounds refresh, sphere test, and far-clip repair as
     * caller-local render traversal code rather than a separate call target.
     * Purpose: decide whether object render culling is needed, refresh the view
     * bounding sphere, and run the frustum sphere clip-mask test.
     */
    int CullNodeForRender(
        zClass_NodePartial * node,
        int siblingCountHint,
        int *clipMask
    ) {
        int testNeeded = 0;
        if (g_zClass_ObjectHseTestEnabled == 0) {
            testNeeded =
                ((*clipMask != 0 || g_zClass_RenderFrustumGridTileIndex > 0) &&
                    siblingCountHint > 1);
        } else {
            testNeeded = (*clipMask != 0 && siblingCountHint > 1);
        }

        if (testNeeded == 0 && (node->flags & kSingleParentFlag) != 0) {
            return 0;
        }

        if ((node->boundsFlags & kNodeBoundsDirtyFlag) != 0 ||
            g_zClass_RenderBoundsContextActive != 0 || (node->flags & kSingleParentFlag) == 0) {
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
            if ((node->flags & kSingleParentFlag) != 0) {
                node->boundsFlags &= ~kNodeBoundsDirtyFlag;
            }
        }

        int result = zVideo_FrustumTestSphereClipMask(
            zClass_NodeViewSphereCenter(node),
            clipMask,
            *zClass_NodeViewSphereRadius(node)
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
    void PushObjectMatrix(
        zClass_Object3DDataPartial * data,
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
            zMath::MatMultiply(
                (const zMat4x3 *)data->localMatrix,
                3
            );
            data->flags &= ~kObject3DTransformDirtyFlag;
        } else if ((flags & kSingleParentFlag) == 0) {
            zMath::MatStackPushAndCloneParent(data->cachedWorldMatrix);
            zMath::MatMultiply(
                (const zMat4x3 *)data->localMatrix,
                3
            );
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
    void PushObjectRenderState(
        zClass_NodePartial * node,
        zClass_Object3DDataPartial * data,
        int *pushedVertexAlpha,
        int *pushedAlphaScale,
        int *pushedSoftwareState
    ) {
        *pushedVertexAlpha = 0;
        *pushedAlphaScale = 0;
        *pushedSoftwareState = 0;

        if ((node->flags & 0x00800000) != 0 && g_zClass_RenderVertexAlphaOverrideActive == 0) {
            *pushedVertexAlpha = 1;
            g_zClass_RenderVertexAlphaOverrideActive = 1;
            zModel_RenderVertexAlphaEnabled_SetCurrent(1);
        }

        if ((data->flags & 0x02) != 0) {
            *pushedAlphaScale = 1;
            ++g_zClass_RenderAlphaScaleStackTop;
            g_zClass_RenderAlphaScaleStack[g_zClass_RenderAlphaScaleStackTop] = data->alphaScale;
            zModel_RenderAlphaScale_SetCurrent(data->alphaScale);
        }

        if ((data->flags & 0x04) != 0) {
            *pushedSoftwareState = 1;
            ++g_zClass_SoftwarePathStateStackTop;
            g_zClass_SoftwarePathRenderStateStack[g_zClass_SoftwarePathStateStackTop].color =
                data->color;
            g_zClass_SoftwarePathRenderStateStack[g_zClass_SoftwarePathStateStackTop].alpha =
                data->colorAlpha;
            zModel_FogTargetColorOverride_SetCurrent(
                &g_zClass_SoftwarePathRenderStateStack[g_zClass_SoftwarePathStateStackTop].color,
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
    void PopObjectRenderState(
        int pushedVertexAlpha,
        int pushedAlphaScale,
        int pushedSoftwareState
    ) {
        if (pushedVertexAlpha != 0) {
            g_zClass_RenderVertexAlphaOverrideActive = 0;
            zModel_RenderVertexAlphaEnabled_SetCurrent(0);
        }

        if (pushedAlphaScale != 0) {
            --g_zClass_RenderAlphaScaleStackTop;
            const float scale =
                g_zClass_RenderAlphaScaleStackTop >= 0
                    ? g_zClass_RenderAlphaScaleStack[g_zClass_RenderAlphaScaleStackTop]
                    : 1.0f;
            zModel_RenderAlphaScale_SetCurrent(scale);
        }

        if (pushedSoftwareState != 0) {
            --g_zClass_SoftwarePathStateStackTop;
            if (g_zClass_SoftwarePathStateStackTop >= 0) {
                zModel_FogTargetColorOverride_SetCurrent(
                    &g_zClass_SoftwarePathRenderStateStack[g_zClass_SoftwarePathStateStackTop]
                        .color,
                    g_zClass_SoftwarePathRenderStateStack[g_zClass_SoftwarePathStateStackTop].alpha
                );
            } else {
                zModel_FogTargetColorOverride_SetCurrent(
                    0,
                    0.0f
                );
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
    void RenderObjectChildren(
        zClass_NodePartial * node,
        int clipMask
    ) {
        if (node->listCountB <= 0) {
            return;
        }

        ++gModel_ClipMaskStackTop;
        *gModel_ClipMaskStackTop = clipMask;
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if (child != 0 && child->classId == kZClassNodeObject3D) {
                if (VariantTag::CurrentAllowsId(child->nodeType) != 0) {
                    zClass_Object3D::RenderTraverse(
                        child,
                        node->listCountB
                    );
                }
            } else if (child != 0) {
                zClass_Class::gwNodeRenderDispatch(
                    child,
                    node->listCountB
                );
            }
        }
        --gModel_ClipMaskStackTop;
    }
}

namespace zClass_Object3D {
    int __fastcall
    /**
     * Reimplements 0x44b300: zClass_Object3D::RenderTraverse
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: cull visible Object3D nodes, manage alt-clip and render-bounds
     * state, push transform/render state, render the node, and recurse children.
     */
    RenderTraverse(
        zClass_NodePartial * node,
        int siblingCountHint
    ) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & kObject3DVisibleFlag) == 0) {
            return 0;
        }

        node->flags = flags & ~kNodeTransformDirtyPropagatedFlag;
        int altClipReset = 0;
        if (gAltClipPassEnabled != 0 && node == g_zClass_CameraTargetNode) {
            gAltClipPassEnabled = 0;
            altClipReset = 1;
        }

        zClass_Object3DDataPartial *data = (zClass_Object3DDataPartial *)(node->classData);
        int clipMask = *gModel_ClipMaskStackTop;
        const int result = CullNodeForRender(
            node,
            siblingCountHint,
            &clipMask
        );
        if (result == 0) {
            int matrixPushed = 0;
            PushObjectMatrix(
                data,
                &matrixPushed
            );
            if (g_zClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_zClass_RenderBoundsContextActive = 1;
            }

            int pushedVertexAlpha;
            int pushedAlphaScale;
            int pushedSoftwareState;
            PushObjectRenderState(
                node,
                data,
                &pushedVertexAlpha,
                &pushedAlphaScale,
                &pushedSoftwareState
            );

            int visibleByProjectedSphere = 1;
            if (g_zClass_ObjectHseTestEnabled != 0 && g_zClass_RenderFrustumGridTileIndex > 0 &&
                siblingCountHint != 1 && g_zClass_RenderVertexAlphaOverrideActive == 0) {
                visibleByProjectedSphere = zScene::TestProjectedSphereVisible(
                    zClass_NodeViewSphereCenter(node),
                    *zClass_NodeViewSphereRadius(node)
                );
            }
            if (visibleByProjectedSphere != 0) {
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
                RenderObjectChildren(
                    node,
                    clipMask
                );
            }

            PopObjectRenderState(
                pushedVertexAlpha,
                pushedAlphaScale,
                pushedSoftwareState
            );
            if (matrixPushed != 0) {
                zMath::MatStackPopPtr();
            }
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        if (altClipReset != 0) {
            gAltClipPassEnabled = 1;
        }
        return result;
    }

    /**
     * Reimplements 0x44daa0: zClass_Object3D::gwObject3DInit
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: allocate an Object3D node, attach zeroed Object3D data, and
     * initialize/queue its default transform state.
     */
    zClass_NodePartial *gwObject3DInit() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x12f,
                "Null node pointer."
            );
            return 0;
        }

        node->classId = kZClassNodeObject3D;
        node->classData = calloc(
            1,
            sizeof(zClass_Object3DDataPartial)
        );
        return PropagateTransformDirty(node) == 0 ? node : 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44db10: zClass_Object3D::gwObject3DAddChild
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate parent, child, and Object3D class data before delegating
     * to the generic child-add helper.
     */
    gwObject3DAddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x178,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x179,
                "Null node pointer."
            );
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x17a,
                "Null class data pointer"
            );
            return 5;
        }

        return zClass_Class::AddChildGeneric(
            parent,
            child
        );
    }

    int __fastcall
    /**
     * Reimplements 0x44db60: zClass_Object3D::RemoveChild
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate parent, child, and Object3D class data before delegating
     * to the generic child-removal helper.
     */
    RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x194,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x195,
                "Null node pointer."
            );
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x196,
                "Null class data pointer"
            );
            return 5;
        }

        return zClass_Class::RemoveChildGeneric(
            parent,
            child
        );
    }

    /**
     * Reimplements 0x44db00: zClass_Object3D::DeleteNode
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: release Object3D class data and return the node to the generic
     * free-list path.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        return zClass_Class::TryFreeNode(node);
    }

    /**
     * Reimplements 0x44d9e0: zClass_Object3D::PropagateTransformDirty
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: reset local Object3D transform fields to identity defaults and
     * queue a transform/bounds dirty update for the node subtree.
     */
    int __fastcall PropagateTransformDirty(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0xe8,
                "Null node pointer."
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0xe9,
                "Null class data pointer"
            );
            return 5;
        }

        zClass_Object3DDataPartial *data = (zClass_Object3DDataPartial *)(node->classData);
        /* Retail keeps the rotation-zero and scale-one dword stores paired. */
        volatile unsigned int *scaleBits = (volatile unsigned int *)(&data->scale.x);
        int count = 3;
        do {
            scaleBits[-3] = 0;
            *scaleBits = 0x3f800000;
            ++scaleBits;
            --count;
        } while (count != 0);

        memset(
            data->localMatrix,
            0,
            sizeof(data->localMatrix)
        );
        data->localMatrix[0] = 1.0f;
        data->localMatrix[4] = 1.0f;
        data->localMatrix[8] = 1.0f;
        data->flags = (data->flags & ~0x10) | 0x09;

        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(
                7,
                node
            );
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44dbb0: zClass_Object3D::gwObject3DSetVisibleFlag
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and set or clear the visible render flag.
     */
    gwObject3DSetVisibleFlag(
        zClass_NodePartial * node,
        int visible
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x1b1,
            0x1b2,
            0x1b3
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        if (visible != 0) {
            data->flags |= kObject3DVisibleFlag;
        } else {
            data->flags &= ~kObject3DVisibleFlag;
        }
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44dc30: zClass_Object3D::gwObject3DSetColorAlpha
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data, clamp alpha/color inputs, and store the
     * software color override state.
     */
    gwObject3DSetColorAlpha(
        zClass_NodePartial * node,
        zColorRgb * color,
        float alpha
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x1d9,
            0x1da,
            0x1db
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        data->colorAlpha = ClampUnit(alpha);
        if (color != 0) {
            data->color.red = ClampUnit(color->red);
            data->color.green = ClampUnit(color->green);
            data->color.blue = ClampUnit(color->blue);
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44dd90: zClass_Object3D::gwObject3DSetAlphaScale
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and store the alpha-scale render value.
     */
    gwObject3DSetAlphaScale(
        zClass_NodePartial * node,
        float alphaScale
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x21f,
            0x220,
            0x221
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        data->alphaScale = alphaScale;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44de10: zClass_Object3D::gwObject3DGetAlphaScale
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and return the stored alpha-scale value.
     */
    gwObject3DGetAlphaScale(
        zClass_NodePartial * node,
        float *outAlphaScale
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x238,
            0x239,
            0x23a
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        *outAlphaScale = data->alphaScale;
        return 0;
    }

    /**
     * Reimplements 0x44de80: zClass_Object3D::gwObject3DSetLitFlag
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and set or clear the lit/model-reference
     * render flag.
     */
    int __fastcall gwObject3DSetLitFlag(
        zClass_NodePartial * node,
        int lit
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x254,
            0x255,
            0x256
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        if (lit != 0) {
            data->flags |= kObject3DLitFlag;
        } else {
            data->flags &= ~kObject3DLitFlag;
        }
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44dfd0: zClass_Object3D::gwObject3DGetScale
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and return the local scale vector.
     */
    gwObject3DGetScale(
        zClass_NodePartial * node,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_Object3DDataPartial *data = GetObject3DDataNoClassCheck(
            node,
            0x331,
            0x332
        );
        if (data == 0) {
            return 5;
        }

        *outX = data->scale.x;
        *outY = data->scale.y;
        *outZ = data->scale.z;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44df00: zClass_Object3D::gwObject3DSetScale
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data, store local scale, update identity state,
     * and queue transform/bounds propagation.
     */
    gwObject3DSetScale(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        zClass_Object3DDataPartial *data = GetObject3DDataNoClassCheck(
            node,
            0x294,
            0x295
        );
        if (data == 0) {
            return 5;
        }

        if ((data->flags & 0x10) != 0) {
            data->flags &= ~0x10;
        }
        data->scale.x = x;
        data->scale.y = y;
        data->scale.z = z;
        if ((data->flags & 0x08) != 0 && (x != 1.0 || y != 1.0 || z != 1.0)) {
            data->flags &= ~0x08;
        }

        QueueTransformUpdate(
            node,
            data
        );
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44e110: zClass_Object3D::gwObject3DGetRotation
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and return the local rotation vector.
     */
    gwObject3DGetRotation(
        zClass_NodePartial * node,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_Object3DDataPartial *data = GetObject3DDataNoClassCheck(
            node,
            0x3a9,
            0x3aa
        );
        if (data == 0) {
            return 5;
        }

        *outX = data->rotation.x;
        *outY = data->rotation.y;
        *outZ = data->rotation.z;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44e030: zClass_Object3D::gwObject3DSetRotation
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data, store local rotation, update identity
     * state, and queue transform/bounds propagation.
     */
    gwObject3DSetRotation(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        zClass_Object3DDataPartial *data = GetObject3DDataNoClassCheck(
            node,
            0x357,
            0x358
        );
        if (data == 0) {
            return 5;
        }

        if ((data->flags & 0x10) != 0) {
            data->flags &= ~0x10;
        }
        data->rotation.x = x;
        data->rotation.y = y;
        data->rotation.z = z;
        ClearIdentityFlagIfAnyNonZero(
            data,
            x,
            y,
            z
        );

        QueueTransformUpdate(
            node,
            data
        );
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44e170: zClass_Object3D::gwObject3DTranslateRotation
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data, add local rotation deltas, update
     * identity state, and queue transform/bounds propagation.
     */
    gwObject3DTranslateRotation(
        zClass_NodePartial * node,
        float dx,
        float dy,
        float dz
    ) {
        zClass_Object3DDataPartial *data = GetObject3DDataNoClassCheck(
            node,
            0x3cf,
            0x3d0
        );
        if (data == 0) {
            return 5;
        }

        if ((data->flags & 0x10) != 0) {
            data->flags &= ~0x10;
        }
        data->rotation.x += dx;
        data->rotation.y += dy;
        data->rotation.z += dz;
        ClearIdentityFlagIfAnyNonZero(
            data,
            data->rotation.x,
            data->rotation.y,
            data->rotation.z
        );

        QueueTransformUpdate(
            node,
            data
        );
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44e270: zClass_Object3D::gwObject3DGetPosition
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and return translation components from
     * the local matrix.
     */
    gwObject3DGetPosition(
        zClass_NodePartial * node,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x41a,
            0x41b,
            0x41c
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        *outX = data->localMatrix[9];
        *outY = data->localMatrix[10];
        *outZ = data->localMatrix[11];
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44e300: zClass_Object3D::gwObject3DSetPosition
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data, store local matrix translation, update
     * identity state, and queue transform/bounds propagation.
     */
    gwObject3DSetPosition(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        zClass_Object3DDataPartial *data = GetObject3DDataNoClassCheck(
            node,
            0x441,
            0x442
        );
        if (data == 0) {
            return 5;
        }

        data->localMatrix[9] = x;
        data->localMatrix[10] = y;
        data->localMatrix[11] = z;
        ClearIdentityFlagIfAnyNonZero(
            data,
            x,
            y,
            z
        );

        QueueTransformUpdate(
            node,
            data
        );
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x44e3d0: zClass_Object3D::gwObject3DTranslatePosition
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data, add local translation deltas, update
     * identity state, and queue transform/bounds propagation.
     */
    gwObject3DTranslatePosition(
        zClass_NodePartial * node,
        float dx,
        float dy,
        float dz
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x47e,
            0x47f,
            0x480
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        data->localMatrix[9] += dx;
        data->localMatrix[10] += dy;
        data->localMatrix[11] += dz;
        ClearIdentityFlagIfAnyNonZero(
            data,
            data->localMatrix[9],
            data->localMatrix[10],
            data->localMatrix[11]
        );

        QueueTransformUpdate(
            node,
            data
        );
        return 0;
    }

    /**
     * Reimplements 0x44e5b0: zClass_Object3D::gwObject3DGetMatrixPtr
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data and return a pointer to the local matrix
     * storage.
     */
    float *__fastcall gwObject3DGetMatrixPtr(zClass_NodePartial * node) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x4fe,
            0x4ff,
            0x500
        );
        if (data == 0) {
            return 0;
        }

        return data->localMatrix;
    }

    int __fastcall
    /**
     * Reimplements 0x44e4f0: zClass_Object3D::gwObject3DSetMatrix
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: validate Object3D data, copy local matrix storage when needed,
     * mark matrix-authored transform state, and enqueue transform propagation.
     */
    gwObject3DSetMatrix(
        zClass_NodePartial * node,
        float *matrix
    ) {
        zClass_Object3DDataPartial *data = GetObject3DData(
            node,
            0x4bb,
            0x4bc,
            0x4bd
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        if (matrix != data->localMatrix) {
            memcpy(
                data->localMatrix,
                matrix,
                sizeof(data->localMatrix)
            );
        }

        data->flags = (data->flags & ~0x08) | 0x11;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(
                7,
                node
            );
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }
}

extern "C" {
zClass_Object3D_ModelRefLerpQueueState g_ModelRefLerpQueueState = {0};
}

namespace zClass_Object3D_ModelRefLerpQueue {
    /**
     * Reimplements 0x437fe4: zClass_Object3D_ModelRefLerpQueue::ClearGlobalState
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: clear the global model-reference lerp queue head, tail, aux, and
     * count fields.
     */
    void ClearGlobalState() {
        g_ModelRefLerpQueueState.listAux = 0;
        g_ModelRefLerpQueueState.tail = 0;
        g_ModelRefLerpQueueState.head = 0;
        g_ModelRefLerpQueueState.count = 0;
    }

    /**
     * Reimplements 0x438020: zClass_Object3D_ModelRefLerpQueue::Add
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: allocate and append a model-reference lerp task, normalize fade
     * direction/rate, and enable the node's lit/model-reference flag.
     */
    void __fastcall Add(
        zClass_NodePartial * node,
        void *callbackCtx,
        void *onComplete,
        float startModelRef,
        float targetModelRef,
        float durationSec
    ) {
        zClass_Object3D_ModelRefLerpTask *task = new zClass_Object3D_ModelRefLerpTask;
        memset(
            task,
            0,
            sizeof(*task)
        );

        if (task != 0) {
            task->next = 0;
            if (g_ModelRefLerpQueueState.count == 0) {
                g_ModelRefLerpQueueState.head = task;
            } else {
                g_ModelRefLerpQueueState.tail->next = task;
            }

            g_ModelRefLerpQueueState.tail = task;
            task->next = 0;
            ++g_ModelRefLerpQueueState.count;
        }

        task->node = node;
        task->onComplete = onComplete;
        task->callbackCtx = callbackCtx;

        targetModelRef = ClampUnit(targetModelRef);
        task->targetModelRef = targetModelRef;
        startModelRef = ClampUnit(startModelRef);
        task->currentModelRef = startModelRef;

        const float delta = targetModelRef - startModelRef;
        task->modelRefDeltaPerSec = durationSec == 0.0f ? 99999997952.0f : delta / durationSec;
        if (delta < 0.0f) {
            task->targetModelRef = 1.0f - targetModelRef;
            task->invertModelRef = 1;
            task->currentModelRef = 1.0f - startModelRef;
            task->modelRefDeltaPerSec = -task->modelRefDeltaPerSec;
        } else {
            task->invertModelRef = 0;
        }

        zClass_Object3D::gwObject3DSetLitFlag(
            node,
            1
        );
    }

    /**
     * Reimplements 0x4381d0: zClass_Object3D_ModelRefLerpQueue::Update
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: advance queued model-reference fades by frame time, apply alpha
     * scale, invoke completion callbacks, and unlink finished tasks.
     */
    void Update() {
        if (g_ModelRefLerpQueueState.count == 0) {
            return;
        }

        zClass_Object3D_ModelRefLerpTask *task = g_ModelRefLerpQueueState.head;
        if (task == 0) {
            return;
        }

        while (task != 0) {
            task->currentModelRef += task->modelRefDeltaPerSec * g_FrameDeltaTimeSec;
            if (task->currentModelRef > 1.0f) {
                task->currentModelRef = 1.0f;
            } else if (task->currentModelRef < 0.0f) {
                task->currentModelRef = 0.0f;
            }

            float alphaScale = task->currentModelRef;
            if (task->invertModelRef == 1) {
                alphaScale = 1.0f - alphaScale;
            }

            zClass_Object3D::gwObject3DSetAlphaScale(
                task->node,
                alphaScale
            );

            if (task->currentModelRef >= task->targetModelRef) {
                union {
                    void *raw;
                    zClass_Object3D_ModelRefLerpCallback callback;
                } onComplete = {0};
                onComplete.raw = task->onComplete;
                if (onComplete.callback != 0) {
                    onComplete.callback(task->callbackCtx);
                }

                if (alphaScale == 1.0f) {
                    zClass_Object3D::gwObject3DSetLitFlag(
                        task->node,
                        0
                    );
                }

                zClass_Object3D_ModelRefLerpTask *const nextTask = task->next;
                if (g_ModelRefLerpQueueState.count != 0) {
                    zClass_Object3D_ModelRefLerpTask *prevTask = g_ModelRefLerpQueueState.head;
                    if (task == prevTask) {
                        --g_ModelRefLerpQueueState.count;
                        g_ModelRefLerpQueueState.head = task->next;
                        if (g_ModelRefLerpQueueState.head == 0) {
                            g_ModelRefLerpQueueState.listAux = 0;
                            g_ModelRefLerpQueueState.tail = 0;
                        }
                        ::operator delete(task);
                    } else if (prevTask != 0) {
                        while (prevTask != 0) {
                            zClass_Object3D_ModelRefLerpTask *const prevNext = prevTask->next;
                            if (prevNext == task) {
                                --g_ModelRefLerpQueueState.count;
                                prevTask->next = task->next;
                                if (g_ModelRefLerpQueueState.tail == task) {
                                    g_ModelRefLerpQueueState.tail = prevTask;
                                }
                                ::operator delete(task);
                                break;
                            }
                            prevTask = prevNext;
                        }
                    }
                }
                task = nextTask;
            } else {
                task = task->next;
            }
        }
    }

    /**
     * Reimplements 0x438180: zClass_Object3D_ModelRefLerpQueue::Reset
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: delete all queued model-reference lerp tasks and zero the global
     * queue state.
     */
    void Reset() {
        zClass_Object3D_ModelRefLerpTask *task = g_ModelRefLerpQueueState.head;
        while (task != 0) {
            zClass_Object3D_ModelRefLerpTask *const next = task->next;
            ::operator delete(task);
            task = next;
        }

        memset(
            &g_ModelRefLerpQueueState,
            0,
            sizeof(g_ModelRefLerpQueueState)
        );
    }
}

namespace zClass_Node {
    /**
     * Reimplements 0x4527f0: zClass_Node::HasRenderableDiPredicate
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: test whether a node's DI reference points to a renderable display
     * instance mode without the hidden flag.
     */
    int __fastcall HasRenderableDiPredicate(zClass_NodePartial * node) {
        ::zDiPartial *di = (::zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di == 0) {
            return 0;
        }

        if (di->mode == 1 && (di->flags & 0x10) == 0) {
            return 1;
        }

        return 0;
    }

    /**
     * Reimplements 0x44d990: zClass_Node::PropagateTransformDirtyRecursive
     * (D:\Proj\GameZRecoil\zClass\Object3d.c).
     * Purpose: mark Object3D transform data, node bounds, and descendants dirty
     * for transform-dependent world/render updates.
     */
    void __fastcall PropagateTransformDirtyRecursive(
        zClass_NodePartial * self
    ) {
        if (self->classId == kZClassNodeObject3D) {
            *(int *)(self->classData) |= kObject3DTransformDirtyFlag;
        }

        self->boundsFlags |= kNodeBoundsDirtyFlag;
        self->flags |= kNodeTransformDirtyPropagatedFlag;

        for (int i = 0; i < self->listCountB; ++i) {
            zClass_NodePartial *child = self->listB[i];
            if ((child->flags & kNodeTransformDirtyPropagatedFlag) == 0) {
                PropagateTransformDirtyRecursive(child);
            }
        }
    }
}
