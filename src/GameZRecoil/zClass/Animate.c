#include "zClass.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

namespace {
    const char *kAnimateSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Animate.c";
    const short kAnimateStateStopped = 2;
    const short kAnimateAdvanceActive = 1;
    const short kAnimateLoopDisabled = -1;

    RECOIL_FORCEINLINE void SampleVec3(zVec3 * dest, const zVec3 *start, const zVec3 *end,
                                       float fraction, const zVec3 *scale) {
        dest->x = (end->x - start->x) * fraction + start->x;
        dest->y = (end->y - start->y) * fraction + start->y;
        dest->z = (end->z - start->z) * fraction + start->z;
        dest->x *= scale->x;
        dest->y *= scale->y;
        dest->z *= scale->z;
    }

    int CullNodeForRender(zClass_NodePartial *node, int siblingCountHint,
                                   int *clipMask) {
        int result = 0;
        if ((*clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
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
            result = zVideo_FrustumTestSphereClipMask(zClass_NodeViewSphereCenter(node), clipMask,
                                                      *zClass_NodeViewSphereRadius(node));
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                *clipMask &= ~0x20;
            }
        }
        return result;
    }

    void RenderNodeAndChildren(zClass_NodePartial *node, int clipMask) {
        node->flags |= 0x80000000;
        zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
        if (di != 0 && g_zClass_RenderRangeFadeActive != 0) {
            di->flags |= 0x08;
            di->blendScale = g_zClass_RenderRangeFadeScale;
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
    }
}

namespace zClass_Animate {
    // Reimplements 0x453c90: zClass_Animate::AdvanceTime
    // (GameZRecoil/zClass/Animate.c)
    RECOIL_NOINLINE short RECOIL_FASTCALL AdvanceTime(zClass_AnimateRuntimePartial * runtime,
                                                             float deltaTime) {
        if (runtime->state == kAnimateStateStopped) {
            return kAnimateStateStopped;
        }

        deltaTime += runtime->currentTime;
        runtime->currentTime = deltaTime;
        if (deltaTime > runtime->duration) {
            if (runtime->loopCount == kAnimateLoopDisabled) {
                runtime->currentTime = 0.0f;
                runtime->state = kAnimateStateStopped;
                return kAnimateAdvanceActive;
            }

            runtime->currentTime = deltaTime - runtime->duration + runtime->loopBase;
            return kAnimateAdvanceActive;
        }

        if (runtime->loopCount != kAnimateLoopDisabled && deltaTime > runtime->startTime) {
            runtime->currentTime = deltaTime - runtime->startTime + runtime->loopBase;
        }

        return kAnimateAdvanceActive;
    }

    // Reimplements 0x453d20: zClass_Animate::SampleTransform
    // (GameZRecoil/zClass/Animate.c)
    RECOIL_NOINLINE short RECOIL_FASTCALL SampleTransform(zClass_AnimateRuntimePartial *
                                                                 runtime) {
        if (runtime->state == kAnimateStateStopped) {
            return kAnimateStateStopped;
        }

        const float frame = runtime->currentTime * static_cast<float>(runtime->maxFrameIndex - 1) /
                            runtime->duration;
        const int frameIndex = static_cast<int>(frame);
        const float fraction = frame - static_cast<float>(frameIndex);
        const zClass_AnimateKeyframePartial *key0 = &runtime->keyframes[frameIndex];
        const zClass_AnimateKeyframePartial *key1 = &runtime->keyframes[frameIndex + 1];

        SampleVec3(&runtime->sampledRotation, &key0->rotation, &key1->rotation, fraction,
                   &runtime->outputRotationScale);
        SampleVec3(&runtime->sampledPosition, &key0->position, &key1->position, fraction,
                   &runtime->outputPositionScale);
        SampleVec3(&runtime->sampledScale, &key0->scale, &key1->scale, fraction,
                   &runtime->outputScaleScale);

        return kAnimateAdvanceActive;
    }

    // Reimplements 0x453bd0: zClass_Animate::UpdateNode
    // (GameZRecoil/zClass/Animate.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL UpdateNode(zClass_NodePartial * node) {
        const char *message;
        int line;
        zClass_AnimateDataPartial *data;

        if (node == 0) {
            line = 0x1a9;
            message = "Null node pointer.";
            goto reportError;
        }

        data = static_cast<zClass_AnimateDataPartial *>(node->classData);
        if (data == 0) {
            line = 0x1aa;
            message = "Null class data pointer";
            goto reportError;
        }

        if ((data->statusFlags & 0x04) != 0) {
            if (AdvanceTime(&data->runtime, g_FrameDeltaTimeSec) == kAnimateStateStopped) {
                data->statusFlags &= ~0x04;
                return 0;
            }

            SampleTransform(&data->runtime);
            data->flags |= 0x01;
            if ((node->flags & 0x01) == 0) {
                if (zClass_TypeList::Insert(7, node) == 0) {
                    node->flags |= 0x01;
                }
            }
            node->flags |= 0x02;
        }

        return 0;

    reportError:
        zError::ReportOld(0x400, kAnimateSourceFile, line, message);
        return 5;
    }

    // Reimplements 0x453b40: zClass_Animate::AddChild
    // (GameZRecoil/zClass/Animate.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL AddChild(zClass_NodePartial * parent,
                                                          zClass_NodePartial * child) {
        if (parent == 0) {
            zError::ReportOld(0x400, kAnimateSourceFile, 0x80, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, kAnimateSourceFile, 0x81, "Null node pointer.");
            return 5;
        }

        return zClass_Class::AddChildGeneric(parent, child);
    }

    // Reimplements 0x453b10: zClass_Animate::DeleteNode
    RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, kAnimateSourceFile, 0x72, "Null node pointer.");
            return 5;
        }

        return zClass_Class::TryFreeNode(node);
    }

    // Reimplements 0x453b80: zClass_Animate::RemoveChild
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial * parent,
                                                             zClass_NodePartial * child) {
        if (parent == 0) {
            zError::ReportOld(0x400, kAnimateSourceFile, 0x97, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, kAnimateSourceFile, 0x98, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(0x400, kAnimateSourceFile, 0x99, "Null class data pointer");
            return 5;
        }

        return zClass_Class::RemoveChildGeneric(parent, child);
    }

    // Reimplements 0x44b710: zClass_Animate::RenderTraverse
    // (GameZRecoil/zClass/Animate.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(
        zClass_NodePartial *node, int siblingCountHint) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        zClass_AnimateDataPartial *data =
            static_cast<zClass_AnimateDataPartial *>(node->classData);
        int clipMask = *gModel_ClipMaskStackTop;
        const int result = CullNodeForRender(node, siblingCountHint, &clipMask);

        if (result == 0) {
            int matrixPushed = 0;
            node->flags |= 0x80000000;
            if ((data->statusFlags & 0x04) != 0) {
                matrixPushed = 1;
                zMath::MatStackPushAndCloneParent(data->savedParentMatrix);
                zMath::MatMultiply((const zMat4x3 *)data->animatedTransform, 3);
                if (g_zClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_zClass_RenderBoundsContextActive = 1;
                }
            }
            RenderNodeAndChildren(node, clipMask);
            if (matrixPushed != 0) {
                zMath::MatStackPopPtr();
            }
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        return result;
    }
}
