#include "zclass.h"

#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

namespace {
    const short kAnimateStateStopped = 2;
    const short kAnimateAdvanceActive = 1;
    const short kAnimateLoopDisabled = -1;

}

namespace CZAnimate {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.deletenode
     * @recoil-artifact defines .text recoil:function:0x453b10: CZAnimate::DeleteNode
     * @recoil-match byte
     *
     * Purpose: validate the animate node pointer and return the node to the
     * shared zClass free-list machinery.
     */
    int __fastcall DeleteNode(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x72, "Null node pointer.");
            return 5;
        }

        return CZClass::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.addchild
     * @recoil-artifact defines .text recoil:function:0x453b40: CZAnimate::AddChild
     * @recoil-match byte
     *
     * Purpose: validate animate parent and child nodes, then append the child
     * through the shared zClass child-list helper.
     */
    int __fastcall AddChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x80, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x81, "Null node pointer.");
            return 5;
        }

        return CZClass::AddChildGeneric(parent, child);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.removechild
     * @recoil-artifact defines .text recoil:function:0x453b80: CZAnimate::RemoveChild
     * @recoil-match byte
     *
     * Purpose: validate animate parent, child, and class-data pointers, then
     * remove the child through the shared zClass child-list helper.
     */
    int __fastcall RemoveChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x97, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x98, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x99, "Null class data pointer");
            return 5;
        }

        return CZClass::RemoveChildGeneric(parent, child);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.updatenode
     * @recoil-artifact defines .text recoil:function:0x453bd0: CZAnimate::UpdateNode
     * @recoil-match byte
     *
     * Purpose: update active animation runtime state, sample transforms, and
     * enqueue the node for type-list processing when it becomes dirty.
     */
    int __fastcall UpdateNode(CZNodePartial * node) {
        CZAnimateDataPartial *data;
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x1a9, "Null node pointer.");
            return 5;
        }
        data = (CZAnimateDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x1aa, "Null class data pointer");
            return 5;
        }
        if ((data->statusFlags & 0x04) != 0) {
            const float deltaTime = g_FrameDeltaTimeSec;
            if (AdvanceTime(&data->runtime, deltaTime) == kAnimateStateStopped) {
                data = (CZAnimateDataPartial *)node->classData;
                data->statusFlags &= ~0x04;
                return 0;
            }
            data = (CZAnimateDataPartial *)node->classData;
            SampleTransform(&data->runtime);
            data = (CZAnimateDataPartial *)node->classData;
            data->flags |= 0x01;
            if ((node->flags & 0x01) == 0) {
                if (CZTypeList::Insert(7, node) == 0) {
                    node->flags |= 0x01;
                }
            }
            node->flags |= 0x02;
        }
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.advancetime
     * @recoil-artifact defines .text recoil:function:0x453c90: CZAnimate::AdvanceTime
     * @recoil-match byte
     *
     * Purpose: advance the animation clock, stop non-looping animations at the
     * end, and wrap looping animations back to their loop base.
     * The loop-count sentinel distinguishes one-shot playback from looping.
     */
    short __fastcall AdvanceTime(
        CZAnimateRuntimePartial * runtime,
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.sampletransform
     * @recoil-artifact defines .text recoil:function:0x453d20: CZAnimate::SampleTransform
     *
     * Purpose: sample interpolated rotation, position, and scale keyframe data
     * for the current animation time.
     */
    short __fastcall SampleTransform(CZAnimateRuntimePartial * runtime) {
        if (runtime->state == kAnimateStateStopped) {
            return kAnimateStateStopped;
        }

        const float frame =
            runtime->currentTime * (float)(runtime->maxFrameIndex - 1) / runtime->duration;
        const int frameIndex = (int)(frame);
        const float fraction = frame - (float)(frameIndex);
        const CZAnimateKeyframePartial *key0 = &runtime->keyframes[frameIndex];
        const CZAnimateKeyframePartial *key1 = &runtime->keyframes[frameIndex + 1];

        runtime->sampledRotation.x =
            ((key1->rotation.x - key0->rotation.x) * fraction + key0->rotation.x) *
            runtime->outputRotationScale.x;
        runtime->sampledRotation.y =
            ((key1->rotation.y - key0->rotation.y) * fraction + key0->rotation.y) *
            runtime->outputRotationScale.y;
        runtime->sampledRotation.z =
            ((key1->rotation.z - key0->rotation.z) * fraction + key0->rotation.z) *
            runtime->outputRotationScale.z;

        runtime->sampledPosition.x =
            ((key1->position.x - key0->position.x) * fraction + key0->position.x) *
            runtime->outputPositionScale.x;
        runtime->sampledPosition.y =
            ((key1->position.y - key0->position.y) * fraction + key0->position.y) *
            runtime->outputPositionScale.y;
        runtime->sampledPosition.z =
            ((key1->position.z - key0->position.z) * fraction + key0->position.z) *
            runtime->outputPositionScale.z;

        runtime->sampledScale.x =
            ((key1->scale.x - key0->scale.x) * fraction + key0->scale.x) *
            runtime->outputScaleScale.x;
        runtime->sampledScale.y =
            ((key1->scale.y - key0->scale.y) * fraction + key0->scale.y) *
            runtime->outputScaleScale.y;
        runtime->sampledScale.z =
            ((key1->scale.z - key0->scale.z) * fraction + key0->scale.z) *
            runtime->outputScaleScale.z;

        return kAnimateAdvanceActive;
    }

}
