// Battlesport compilation unit between turret.cpp and util.cpp, inferred from
// the retail object boundary [0x437e60, 0x438350): its .rdata float
// constants, its .data and .bss runs and its single CRT initializer are
// separate from the neighbouring objects and from Object3d.c. Original
// filename unresolved; tween.cpp is a provisional name (2026-10-02). A leaked
// build log names GameZRecoil\zQueue\zqueue.cpp, which is not adopted because
// this object links among the Battlesport objects. Retail .data also begins
// with an unreferenced int initialized to 1 (0x4dd1bc) that is not modeled.

#include "GameZRecoil/include/zclass.h"

#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdlib.h>
#include <string.h>

namespace CZNode {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setcontextrecursive
 * @recoil-artifact defines .text recoil:function:0x437e60: CZNode::SetContextRecursive
 * @recoil-match byte
 *
 * BN evidence: fastcall self/context, stack flagMask, callbackContext at
 * 0x40, flags at 0x24, signed listCountB at 0x5c, listB at 0x60,
 * recursive self-call only, and no global data references.
 * Purpose: assign a callback context and OR flag bits through a node
 * subtree using the zClass child-list links.
 */
void __fastcall SetContextRecursive(CZNodePartial* self, CZNodePartial* context, int flagMask)
{
    self->callbackContext = context;
    self->flags |= flagMask;

    for (int i = 0; i < self->listCountB; ++i) {
        SetContextRecursive(self->listB[i], context, flagMask);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setdiflagbit0recursive
 * @recoil-artifact defines .text recoil:function:0x437ea0: CZNode::SetDiFlagBit0Recursive
 * @recoil-match byte
 *
 * BN evidence: fastcall node/enabled, gwNodeGetUserData for the typed
 * userDataOrDiRef display-instance reference, zDi::SetFlagBit0 when
 * non-null, signed listCountB at 0x5c, listB at 0x60, recursive self-call
 * only, and no global data references.
 * Purpose: set display-instance flag bit 0 for each display instance
 * reachable through a node's child-list subtree.
 */
void __fastcall SetDiFlagBit0Recursive(CZNodePartial* node, int enabled)
{
    unsigned int userData;
    CZClass::gwNodeGetUserData(node, &userData);
    zDiPartial* di = (zDiPartial*)(userData);
    if (di != 0) {
        zDi::SetFlagBit0(di, enabled);
    }

    for (int i = 0; i < node->listCountB; ++i) {
        SetDiFlagBit0Recursive(node->listB[i], enabled);
    }
}

}

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-softwaremodehotkeyenabled
 * @recoil-artifact defines .data recoil:data:0x4dd1c0: g_zVideo_SoftwareModeHotkeyEnabled.
 * Retail initializes the authored zVideo debug/software-mode hotkey gate enabled.
 * Purpose: gate the software-mode hotkey command.
 */
int g_zVideo_SoftwareModeHotkeyEnabled = 1;
}

namespace zVideo {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-handlesoftwaremodehotkeycommand
 * @recoil-artifact defines .text recoil:function:0x437ef0: zVideo::HandleSoftwareModeHotkeyCommand.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: cycle the software-mode hotkey presets while preserving HUD state.
 *
 * Evidence: BN dispatches on GetVideoModeIndexFromOptions() - 2 and cycles
 * modes 2->4, 3->5, 4->2, and 5->3; only the downscale paths request
 * half-resolution adjustment disablement.
 * The saved HUD type is restored on every path through the switch,
 * including modes outside these presets. Case order follows the retail
 * dispatch bodies.
 */
void __fastcall HandleSoftwareModeHotkeyCommand(int)
{
    if (g_zVideo_SoftwareModeHotkeyEnabled == 0) {
        return;
    }

    const int previousHudType = zOpt::SetHudTypeForCurrentHwMode(1);
    const int currentModeIndex = zVid::GetVideoModeIndexFromOptions();

    switch (currentModeIndex) {
    case 2:
        if (InitApplyModeIndex(4) == 0) {
            zVid::SetVideoModeIndex(4);
            if (zVid::GetAccelerationOption() == 0) {
                SetHalfResAdjustMode(1);
            }
        }
        break;

    case 4:
        if (InitApplyModeIndex(2) == 0) {
            zVid::SetVideoModeIndex(2);
            if (zVid::GetAccelerationOption() == 0) {
                SetHalfResAdjustMode(0);
            }
        }
        break;

    case 3:
        if (InitApplyModeIndex(5) == 0) {
            zVid::SetVideoModeIndex(5);
            if (zVid::GetAccelerationOption() == 0) {
                SetHalfResAdjustMode(1);
            }
        }
        break;

    case 5:
        if (InitApplyModeIndex(3) == 0) {
            zVid::SetVideoModeIndex(3);
            if (zVid::GetAccelerationOption() == 0) {
                SetHalfResAdjustMode(0);
            }
        }
        break;
    }
    zOpt::SetHudTypeForCurrentHwMode(previousHudType);
}

}

/**
 * Purpose: initialize an empty model-reference lerp queue. Retail startup
 * 0x437ff0 reaches the constructor inlined into global initialization.
 */
inline CZObject3DModelRefLerpQueueState::CZObject3DModelRefLerpQueueState()
{
    listAux = 0;
    tail = 0;
    head = 0;
    count = 0;
}

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.clearglobalstate
 * @recoil-artifact emits .text recoil:function:0x438000: Global queue initialization.
 * Purpose: own the queue whose native construction is registered in CRT startup.
 */
CZObject3DModelRefLerpQueueState g_ModelRefLerpQueueState;
}

namespace CZObject3DModelRefLerpQueue {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.add
 * @recoil-artifact defines .text recoil:function:0x438020: CZObject3DModelRefLerpQueue::Add
 *
 *
 * Purpose: allocate and append a model-reference lerp task, normalize fade
 * direction/rate, and enable the node's lit/model-reference flag.
 */
void __fastcall
Add(CZNodePartial* node,
    void* callbackCtx,
    void* onComplete,
    float startModelRef,
    float targetModelRef,
    float durationSec)
{
    CZObject3DModelRefLerpTask* task = new CZObject3DModelRefLerpTask;
    memset(task, 0, sizeof(*task));

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

    targetModelRef = targetModelRef > 1.0f ? 1.0f : (targetModelRef < 0.0f ? 0.0f : targetModelRef);
    task->targetModelRef = targetModelRef;
    startModelRef = startModelRef > 1.0f ? 1.0f : (startModelRef < 0.0f ? 0.0f : startModelRef);
    const float delta = targetModelRef - startModelRef;
    task->currentModelRef = startModelRef;
    if (durationSec == 0.0f) {
        task->modelRefDeltaPerSec = 99999997952.0f;
    } else {
        task->modelRefDeltaPerSec = delta / durationSec;
    }
    if (delta < 0.0f) {
        task->targetModelRef = 1.0f - targetModelRef;
        task->invertModelRef = 1;
        task->currentModelRef = 1.0f - startModelRef;
        task->modelRefDeltaPerSec = -task->modelRefDeltaPerSec;
    } else {
        task->invertModelRef = 0;
    }

    CZObject3D::gwObject3DSetLitFlag(node, 1);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.reset
 * @recoil-artifact defines .text recoil:function:0x438180: CZObject3DModelRefLerpQueue::Reset
 * @recoil-match byte
 *
 * Purpose: delete all queued model-reference lerp tasks and zero the global
 * queue state.
 */
void __cdecl Reset()
{
    CZObject3DModelRefLerpTask* task = g_ModelRefLerpQueueState.head;
    while (task != 0) {
        CZObject3DModelRefLerpTask* const next = task != 0 ? task->next : 0;
        ::operator delete(task);
        task = next;
    }

    g_ModelRefLerpQueueState.listAux = 0;
    g_ModelRefLerpQueueState.tail = 0;
    g_ModelRefLerpQueueState.head = 0;
    g_ModelRefLerpQueueState.count = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.update
 * @recoil-artifact defines .text recoil:function:0x4381d0: CZObject3DModelRefLerpQueue::Update
 *
 *
 * Purpose: advance queued model-reference fades by frame time, apply alpha
 * scale, invoke completion callbacks, and unlink finished tasks.
 */
void __cdecl Update()
{
    if (g_ModelRefLerpQueueState.count == 0) {
        return;
    }

    CZObject3DModelRefLerpTask* task = g_ModelRefLerpQueueState.head;
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

        CZObject3D::gwObject3DSetAlphaScale(task->node, alphaScale);

        if (task->currentModelRef >= task->targetModelRef) {
            union {
                void* raw;
                CZObject3DModelRefLerpCallback callback;
            } onComplete = { 0 };
            onComplete.raw = task->onComplete;
            if (onComplete.callback != 0) {
                onComplete.callback(task->callbackCtx);
            }

            if (alphaScale == 1.0f) {
                CZObject3D::gwObject3DSetLitFlag(task->node, 0);
            }

            CZObject3DModelRefLerpTask* const nextTask = task != 0 ? task->next : 0;
            if (task != 0) {
                if (g_ModelRefLerpQueueState.count != 0) {
                    CZObject3DModelRefLerpTask* prevTask = g_ModelRefLerpQueueState.head;
                    if (task == prevTask) {
                        --g_ModelRefLerpQueueState.count;
                        g_ModelRefLerpQueueState.head = task->next;
                        if (g_ModelRefLerpQueueState.head == 0) {
                            g_ModelRefLerpQueueState.listAux = 0;
                            g_ModelRefLerpQueueState.tail = 0;
                        }
                        ::operator delete(task);
                    } else {
                        while (prevTask != 0) {
                            if (prevTask->next == task) {
                                --g_ModelRefLerpQueueState.count;
                                prevTask->next = task->next;
                                if (g_ModelRefLerpQueueState.tail == task) {
                                    g_ModelRefLerpQueueState.tail = prevTask;
                                }
                                ::operator delete(task);
                                break;
                            }
                            prevTask = prevTask->next;
                        }
                    }
                }
            }
            task = nextTask;
        } else {
            task = task != 0 ? task->next : 0;
        }
    }
}
}
