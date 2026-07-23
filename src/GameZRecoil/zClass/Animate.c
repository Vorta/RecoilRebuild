#include "zclass.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

namespace {
    const char kAnimateSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\Animate.c";
    const short kAnimateStateStopped = 2;
    const short kAnimateAdvanceActive = 1;
    const short kAnimateLoopDisabled = -1;

    /**
     * Original-source helper evidence: no standalone retail function exists.
     * Observed in caller 0x453d20 (zClass_Animate::SampleTransform); BN keeps
     * the three component interpolation and output-scale multiplies inline at
     * each sampled transform channel.
     *
     * Purpose: linearly interpolate a three-component keyframe value and apply
     * the corresponding output scale.
     */
    inline void SampleVec3(
        zVec3 * dest,
        const zVec3 *start,
        const zVec3 *end,
        float fraction,
        const zVec3 *scale
    ) {
        dest->x = (end->x - start->x) * fraction + start->x;
        dest->y = (end->y - start->y) * fraction + start->y;
        dest->z = (end->z - start->z) * fraction + start->z;
        dest->x *= scale->x;
        dest->y *= scale->y;
        dest->z *= scale->z;
    }

}

namespace zClass_Animate {
    /**
     * Reimplements 0x453b10: zClass_Animate::DeleteNode
     * (D:\Proj\GameZRecoil\zClass\Animate.c).
     *
     * Purpose: validate the animate node pointer and return the node to the
     * shared zClass free-list machinery.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kAnimateSourceFile,
                0x72,
                "Null node pointer."
            );
            return 5;
        }

        return zClass_Class::TryFreeNode(node);
    }

    int __fastcall
    /**
     * Reimplements 0x453b40: zClass_Animate::AddChild
     * (D:\Proj\GameZRecoil\zClass\Animate.c).
     *
     * Purpose: validate animate parent and child nodes, then append the child
     * through the shared zClass child-list helper.
     */
    AddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kAnimateSourceFile,
                0x80,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kAnimateSourceFile,
                0x81,
                "Null node pointer."
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
     * Reimplements 0x453b80: zClass_Animate::RemoveChild
     * (D:\Proj\GameZRecoil\zClass\Animate.c).
     *
     * Purpose: validate animate parent, child, and class-data pointers, then
     * remove the child through the shared zClass child-list helper.
     */
    RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kAnimateSourceFile,
                0x97,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kAnimateSourceFile,
                0x98,
                "Null node pointer."
            );
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(
                0x400,
                kAnimateSourceFile,
                0x99,
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
     * Reimplements 0x453bd0: zClass_Animate::UpdateNode
     * (D:\Proj\GameZRecoil\zClass\Animate.c).
     *
     * Purpose: update active animation runtime state, sample transforms, and
     * enqueue the node for type-list processing when it becomes dirty.
     */
    int __fastcall UpdateNode(zClass_NodePartial * node) {
        const char *message;
        int line;
        zClass_AnimateDataPartial *data;

        if (node == 0) {
            line = 0x1a9;
            message = "Null node pointer.";
            goto reportError;
        }

        data = (zClass_AnimateDataPartial *)(node->classData);
        if (data == 0) {
            line = 0x1aa;
            message = "Null class data pointer";
            goto reportError;
        }

        if ((data->statusFlags & 0x04) != 0) {
            if (AdvanceTime(
                &data->runtime,
                g_FrameDeltaTimeSec
            ) == kAnimateStateStopped) {
                data->statusFlags &= ~0x04;
                return 0;
            }

            SampleTransform(&data->runtime);
            data->flags |= 0x01;
            if ((node->flags & 0x01) == 0) {
                if (zClass_TypeList::Insert(
                    7,
                    node
                ) == 0) {
                    node->flags |= 0x01;
                }
            }
            node->flags |= 0x02;
        }

        return 0;

    reportError:
        zError::ReportOld(
            0x400,
            kAnimateSourceFile,
            line,
            message
        );
        return 5;
    }

    short __fastcall
    /**
     * Reimplements 0x453c90: zClass_Animate::AdvanceTime
     * (D:\Proj\GameZRecoil\zClass\Animate.c).
     *
     * Purpose: advance the animation clock, stop non-looping animations at the
     * end, and wrap looping animations back to their loop base.
     */
    AdvanceTime(
        zClass_AnimateRuntimePartial * runtime,
        float deltaTime
    ) {
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

    /**
     * Reimplements 0x453d20: zClass_Animate::SampleTransform
     * (D:\Proj\GameZRecoil\zClass\Animate.c).
     *
     * Purpose: sample interpolated rotation, position, and scale keyframe data
     * for the current animation time.
     */
    short __fastcall SampleTransform(zClass_AnimateRuntimePartial * runtime) {
        if (runtime->state == kAnimateStateStopped) {
            return kAnimateStateStopped;
        }

        const float frame =
            runtime->currentTime * (float)(runtime->maxFrameIndex - 1) / runtime->duration;
        const int frameIndex = (int)(frame);
        const float fraction = frame - (float)(frameIndex);
        const zClass_AnimateKeyframePartial *key0 = &runtime->keyframes[frameIndex];
        const zClass_AnimateKeyframePartial *key1 = &runtime->keyframes[frameIndex + 1];

        SampleVec3(
            &runtime->sampledRotation,
            &key0->rotation,
            &key1->rotation,
            fraction,
            &runtime->outputRotationScale
        );
        SampleVec3(
            &runtime->sampledPosition,
            &key0->position,
            &key1->position,
            fraction,
            &runtime->outputPositionScale
        );
        SampleVec3(
            &runtime->sampledScale,
            &key0->scale,
            &key1->scale,
            fraction,
            &runtime->outputScaleScale
        );

        return kAnimateAdvanceActive;
    }

}
