#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zMath/zMath.h"

namespace {
    int CullNodeForRender(zClass_NodePartial *node, int siblingCountHint,
                                   int *clipMask) {
        int result = 0;
        if (*clipMask != 0 && siblingCountHint > 1) {
            if ((node->boundsFlags & 0x04) != 0 || g_zClass_RenderBoundsContextActive != 0) {
                zBBoxCorners corners = {0};
                zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);
                BBox::CornersToBoundingSphere(&corners, zClass_NodeViewSphereCenter(node),
                                              zClass_NodeViewSphereRadius(node));
                node->boundsFlags &= ~0x04;
            }
            result = zVideo_FrustumTestSphereClipMask(zClass_NodeViewSphereCenter(node), clipMask,
                                                      *zClass_NodeViewSphereRadius(node));
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                *clipMask &= ~0x20;
            }
        }
        return result;
    }
}

namespace zClass_Switch {
    // Reimplements 0x44bfb0: zClass_Switch::RenderTraverse
    // (D:\Proj\GameZRecoil\zClass\Switch.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(
        zClass_NodePartial *node, int siblingCountHint) {
        int boundsContextPushed = 0;
        const int flags = node->flags;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        zClass_SwitchDataPartial *data =
            (zClass_SwitchDataPartial *)(node->classData);
        node->flags = flags & ~0x02000000;
        int clipMask = *gModel_ClipMaskStackTop;
        const int result = CullNodeForRender(node, siblingCountHint, &clipMask);
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
                    zClass_Class::gwNodeRenderDispatch(node->listB[i], node->listCountB);
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
