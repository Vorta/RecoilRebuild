#include "cls_api.h"
#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

enum { kAnimateStateStopped = 2, kAnimateAdvanceActive = 1, kAnimateLoopDisabled = -1 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.deletenode
 * @recoil-artifact defines .text recoil:function:0x453b10: CZAnimateDeleteNode
 * @recoil-match byte
 *
 * Purpose: validate the animate node pointer and return the node to the
 * shared zClass free-list machinery.
 */
int __fastcall CZAnimateDeleteNode(CZNodePartial* node)
{
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x72, "Null node pointer.");
        return 5;
    }

    return TryFreeNode(node);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.addchild
 * @recoil-artifact defines .text recoil:function:0x453b40: CZAnimateAddChild
 * @recoil-match byte
 *
 * Purpose: validate animate parent and child nodes, then append the child
 * through the shared zClass child-list helper.
 */
int __fastcall CZAnimateAddChild(CZNodePartial* parent, CZNodePartial* child)
{
    if (parent == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x80, "Null node pointer.");
        return 5;
    }
    if (child == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x81, "Null node pointer.");
        return 5;
    }

    return AddChildGeneric(parent, child);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.removechild
 * @recoil-artifact defines .text recoil:function:0x453b80: CZAnimateRemoveChild
 * @recoil-match byte
 *
 * Purpose: validate animate parent, child, and class-data pointers, then
 * remove the child through the shared zClass child-list helper.
 */
int __fastcall CZAnimateRemoveChild(CZNodePartial* parent, CZNodePartial* child)
{
    if (parent == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x97, "Null node pointer.");
        return 5;
    }
    if (child == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x98, "Null node pointer.");
        return 5;
    }
    if (parent->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x99, "Null class data pointer");
        return 5;
    }

    return RemoveChildGeneric(parent, child);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.animate.updatenode
 * @recoil-artifact defines .text recoil:function:0x453bd0: CZAnimate::UpdateNode
 * @recoil-match byte
 *
 * Purpose: update active animation runtime state, sample transforms, and
 * enqueue the node for type-list processing when it becomes dirty.
 */
int __fastcall UpdateNode(CZNodePartial* node)
{
    CZAnimateDataPartial* data;
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x1a9, "Null node pointer.");
        return 5;
    }
    data = (CZAnimateDataPartial*)(node->classData);
    if (data == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Animate.c", 0x1aa, "Null class data pointer");
        return 5;
    }
    if ((data->statusFlags & 0x04) != 0) {
        const float deltaTime = g_FrameDeltaTimeSec;
        if (AdvanceTime(&data->runtime, deltaTime) == kAnimateStateStopped) {
            data = (CZAnimateDataPartial*)node->classData;
            data->statusFlags &= ~0x04;
            return 0;
        }
        data = (CZAnimateDataPartial*)node->classData;
        SampleTransform(&data->runtime);
        data = (CZAnimateDataPartial*)node->classData;
        data->flags |= 0x01;
        if ((node->flags & 0x01) == 0) {
            if (CZTypeListInsert(7, node) == 0) {
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
short __fastcall AdvanceTime(CZAnimateRuntimePartial* runtime, float deltaTime)
{
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
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-lerp
 * @recoil-match byte
 *
 * Purpose: sample interpolated rotation, position, and scale keyframe data
 * for the current animation time.
 */
short __fastcall SampleTransform(CZAnimateRuntimePartial* runtime)
{
    const float duration = runtime->duration;
    const float currentTime = runtime->currentTime;
    const float lastFrame = (float)runtime->maxFrameIndex - 1.0f;
    float frame;
    short frameIndex;
    float fraction;
    const CZAnimateKeyframePartial* key0;
    const CZAnimateKeyframePartial* key1;
    zVec3* dest;
    const zVec3* from;
    const zVec3* to;
    if (runtime->state == kAnimateStateStopped) {
        return kAnimateStateStopped;
    }

    frame = lastFrame * currentTime / duration;
    frameIndex = (short)frame;
    fraction = frame - (float)frameIndex;
    key0 = &runtime->keyframes[frameIndex];
    key1 = &runtime->keyframes[frameIndex + 1];

    dest = &runtime->sampledRotation;
    from = &key0->rotation;
    to = &key1->rotation;
    ZMTH_VECTOR_LERP_BOUND(dest, from, to, fraction);
    dest->x *= runtime->outputRotationScale.x;
    dest->y *= runtime->outputRotationScale.y;
    dest->z *= runtime->outputRotationScale.z;

    dest = &runtime->sampledPosition;
    from = &key0->position;
    to = &key1->position;
    ZMTH_VECTOR_LERP_BOUND(dest, from, to, fraction);
    dest->x *= runtime->outputPositionScale.x;
    dest->y *= runtime->outputPositionScale.y;
    dest->z *= runtime->outputPositionScale.z;

    dest = &runtime->sampledScale;
    from = &key0->scale;
    to = &key1->scale;
    ZMTH_VECTOR_LERP_BOUND(dest, from, to, fraction);
    dest->x *= runtime->outputScaleScale.x;
    dest->y *= runtime->outputScaleScale.y;
    dest->z *= runtime->outputScaleScale.z;

    return kAnimateAdvanceActive;
}
