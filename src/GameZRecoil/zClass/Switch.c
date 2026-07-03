#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"

extern "C" {
/**
 * Reimplements data 0x4dec88: g_zClass_SourceFile_SwitchC.
 * BN data inventory declares writable Switch.c source path char[0x24], and
 * Switch.c parent/child validation callers reference it for zError reports.
 * Purpose: preserve the legacy source-file literal for switch-node diagnostics.
 */
char g_zClass_SourceFile_SwitchC[0x24] =
    "D:\\Proj\\GameZRecoil\\zClass\\Switch.c";
}

namespace {
    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x44bfb0 (D:\Proj\GameZRecoil\zClass\Switch.c);
     * BN keeps the switch-mask traversal cull, bounds refresh, and sphere
     * clip-mask sequence inline in the Switch traversal body.
     * Purpose: update switch-node bounds when needed and run the sphere
     * frustum cull used by switch render traversal.
     */
    int CullNodeForRender(
        zClass_NodePartial * node,
        int siblingCountHint,
        int *clipMask
    ) {
        int result = 0;
        if (*clipMask != 0 && siblingCountHint > 1) {
            if ((node->boundsFlags & 0x04) != 0 || g_zClass_RenderBoundsContextActive != 0) {
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
                node->boundsFlags &= ~0x04;
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
}

namespace zClass_Switch {
    int __fastcall
    /**
     * Reimplements 0x44bfb0: zClass_Switch::RenderTraverse
     * (D:\Proj\GameZRecoil\zClass\Switch.c).
     *
     * Purpose: cull the switch node, push the clip mask, and render only the
     * active child-mask entries.
     */
    RenderTraverse(
        zClass_NodePartial * node,
        int siblingCountHint
    ) {
        int boundsContextPushed = 0;
        const int flags = node->flags;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        zClass_SwitchDataPartial *data = (zClass_SwitchDataPartial *)(node->classData);
        node->flags = flags & ~0x02000000;
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
            node->flags |= 0x80000000;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            const unsigned int activeMask = data->childMasks[data->activeMaskIndex];
            for (int i = 0; i < node->listCountB; ++i) {
                if (((activeMask >> i) & 1U) != 0) {
                    zClass_Class::gwNodeRenderDispatch(
                        node->listB[i],
                        node->listCountB
                    );
                }
            }
            --gModel_ClipMaskStackTop;
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        return result;
    }
}
