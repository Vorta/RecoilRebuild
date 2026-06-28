#include "Battlesport/Briefing.h"

#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <windows.h>

#include "recoil/recoil_types.h"
#include <new>
#include <process.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace {
/*
 * Briefing subsystem action records. BN shows the Add* queue helpers allocating
 * these concrete records, storing a BriefingAction base pointer in the circular
 * queue, and dispatching each Tick override through that source-level base.
 * The records are the authored action model, not a recovered VTable/FTable
 * scaffold.
 */
struct BriefingActionElementTarget : BriefingAction {
    HudUiElement *target;
};

struct BriefingActionHideElement : BriefingActionElementTarget {
    int Tick(float deltaSec);
};

struct BriefingActionShowElement : BriefingActionElementTarget {
    int Tick(float deltaSec);
};

struct BriefingActionFadeInElement : BriefingActionElementTarget {
    float alpha;

    int Tick(float deltaSec);
};

struct BriefingActionSetPanelText : BriefingAction {
    char text[0x100];
    HudUiPanel *target;

    int Tick(float deltaSec);
};

struct BriefingActionSetWidgetImageTimed : BriefingAction {
    zVidImagePartial *imageRef;
    HudUiWidget *target;
    float timer;

    int Tick(float deltaSec);
};

struct BriefingActionPlaySample : BriefingAction {
    char sampleName[0x50];
    float gain;
    int useVariant;
    int variantIndex;

    int Tick(float deltaSec);
};

struct BriefingActionDelayUntilProgress : BriefingAction {
    float requiredProgress;

    int Tick(float deltaSec);
};

RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionElementTarget,
        target
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionElementTarget) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionFadeInElement,
        alpha
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionFadeInElement) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionSetPanelText,
        target
    ) == 0x104
);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionSetPanelText) == 0x108);
RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionSetWidgetImageTimed,
        timer
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionSetWidgetImageTimed) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionPlaySample,
        gain
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionPlaySample,
        useVariant
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionPlaySample,
        variantIndex
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionPlaySample) == 0x60);
RECOIL_STATIC_ASSERT(
    offsetof(
        BriefingActionDelayUntilProgress,
        requiredProgress
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionDelayUntilProgress) == 0x08);

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x404070, 0x404180, 0x404280, and 0x404400.
 * Evidence: BN field references select offset 0xa94c from the runtime object.
 * Purpose: select the action queue embedded in the briefing runtime object.
 */
inline Briefing_ActionQueue *BriefingActionQueue(
    HudUiBriefingRuntime *runtime
) {
    return &runtime->actionQueue;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x404070, 0x404280, and 0x404400.
 * Evidence: BN field references select offset 0xaae8 from the runtime object.
 * Purpose: select the mission-name panel embedded in the briefing runtime.
 */
inline HudUiPanel *BriefingMissionNamePanel(
    HudUiBriefingRuntime *runtime
) {
    return &runtime->missionName;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x404070, 0x404280, and 0x404400.
 * Evidence: BN field references select offset 0xad8c from the runtime object.
 * Purpose: select the objective-summary panel embedded in the briefing runtime.
 */
inline HudUiPanel *BriefingObjectiveSummaryPanel(
    HudUiBriefingRuntime *runtime
) {
    return &runtime->objectiveSummary;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x404070, 0x404280, and 0x404400.
 * Evidence: BN field references select offset 0xb030 from the runtime object.
 * Purpose: select the objective-description panel embedded in the briefing runtime.
 */
inline HudUiPanel *BriefingObjectiveDescPanel(
    HudUiBriefingRuntime *runtime
) {
    return &runtime->objectiveDesc;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x404070, 0x404280, and 0x404400.
 * Evidence: BN field references select offset 0xb2d4 from the runtime object.
 * Purpose: select the objective-picture widget embedded in the briefing runtime.
 */
inline HudUiBriefingObjectivePicture *BriefingObjectivePicture(
    HudUiBriefingRuntime *runtime
) {
    return &runtime->objectivePicture;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x404070 and 0x404280.
 * Evidence: BN field references select offset 0xb394 from the runtime object.
 * Purpose: select the transmission-halted panel embedded in the briefing runtime.
 */
inline HudUiPanel *BriefingTransmissionHaltedPanel(
    HudUiBriefingRuntime *runtime
) {
    return &runtime->transmissionHalted;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in caller 0x404400.
 * Evidence: BN indexes the locator panel array at offset 0xb8f0 with 0x40-byte elements.
 * Purpose: select one briefing locator panel by objective index.
 */
inline HudUiBriefingLocatorPanel *BriefingLocatorPanel(
    HudUiBriefingRuntime *runtime,
    int objectiveIndex
) {
    return &runtime->locatorPanels[objectiveIndex];
}

} // namespace

extern "C" {

/**
 * Reimplements data 0x4e5c60: g_Briefing_ThreadRunFlag.
 * Purpose: control the lifetime of the briefing update thread.
 */
int g_Briefing_ThreadRunFlag = 0;

/**
 * Reimplements data 0x4e5c64: g_Briefing_ThreadExitedFlag.
 * Purpose: signal that the briefing update thread has left its loop.
 */
int g_Briefing_ThreadExitedFlag = 0;

/**
 * Reimplements data 0x4e5c6c: g_Briefing_SequenceActiveFlag.
 * Purpose: expose whether the queued briefing sequence is still active.
 */
int g_Briefing_SequenceActiveFlag = 0;
}

/**
 * Reimplements data 0x4e5c70: g_Briefing_SndSetName.
 * Purpose: store the mission briefing sample-set name for thread startup and shutdown.
 */
char g_Briefing_SndSetName[0x40] = {0};

/**
 * Reimplements data 0x4e5cb0: g_Briefing_CurrentSndHandle.
 * Purpose: retain the currently playing briefing voice sample so later actions can stop it.
 */
zSndPlayHandle *g_Briefing_CurrentSndHandle = 0;

/**
 * Reimplements data 0x4e5cb4: g_Briefing_Runtime.
 * Purpose: hold the active briefing UI runtime while the mission briefing thread is alive.
 */
HudUiBriefingRuntime *g_Briefing_Runtime = 0;

extern "C" {

/**
 * Reimplements data 0x4e5cb8: g_Briefing_AllowAdvanceFlag.
 * Purpose: gate user input that can advance or halt the current briefing sequence.
 */
int g_Briefing_AllowAdvanceFlag = 0;

/**
 * Reimplements data 0x56bbf8: g_Briefing_SystemActiveFlag.
 * Purpose: indicate that the briefing subsystem is currently active.
 */
int g_Briefing_SystemActiveFlag = 0;

/**
 * Reimplements data 0x4da24c: g_Briefing_ProgressEventCode.
 * Purpose: track the most recent briefing sample progress event, initialized to no event.
 */
int g_Briefing_ProgressEventCode = -1;
}

/**
 * Original inline constructor; no standalone retail function exists.
 * Observed in caller 0x403930 as the first runtime member construction state.
 * Purpose: initialize the mission-owned briefing action queue and empty node ring.
 */
inline Briefing_ActionQueue::Briefing_ActionQueue(
    int missionIdValue
) {
    missionId = (unsigned char)(missionIdValue);
    BriefingActionNode *const sentinel = new BriefingActionNode;
    sentinel->prev = sentinel;
    sentinel->next = sentinel;
    headSentinel = sentinel;
    nodeCount = 0;
    sequenceActive = 0;
    g_Briefing_ProgressEventCode = -1;
}

/**
 * Original inline destructor; no standalone retail function exists.
 * Observed in 0x403930 constructor unwind state after action-queue construction.
 * Purpose: release the briefing action queue's sentinel and queued action records.
 */
inline Briefing_ActionQueue::~Briefing_ActionQueue() {
    BriefingActionNode *node = headSentinel;
    if (node == 0) {
        return;
    }

    while (nodeCount > 0) {
        BriefingActionNode *const previous = node->prev;
        delete previous->action;
        delete previous;
        --nodeCount;
    }

    delete node;
    headSentinel = 0;
    currentNode = 0;
    sequenceActive = 0;
}

/**
 * Original inline constructor; no standalone retail function exists.
 * Observed in caller 0x403930 as the transport-progress member construction.
 * Purpose: construct the briefing progress bar through its fill-bitmap base.
 */
inline HudUiBriefingTransportProgress::HudUiBriefingTransportProgress()
    : HudUiFillBitmap() {
}

/**
 * Original inline constructor; no standalone retail function exists.
 * Observed in caller 0x403930 as the objective-picture member construction.
 * Purpose: construct the briefing picture widget and clear its noise overlay state.
 */
inline HudUiBriefingObjectivePicture::HudUiBriefingObjectivePicture()
    : HudUiWidget(0) {
    noiseAlpha = 0.0f;
    Invalidate();
}

/**
 * Reimplements 0x403930: HudUiBriefingRuntime::HudUiBriefingRuntime.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: construct the briefing UI runtime, bind its ZRD widgets, and run the first frame.
 */
HudUiBriefingRuntime::HudUiBriefingRuntime(
    int missionId
) : HudUiBackground(),
    actionQueue(missionId),
    transportProgress(),
    missionName(
        0,
        0,
        0
    ),
    objectiveSummary(
        0,
        0,
        0
    ),
    objectiveDesc(
        0,
        0,
        0
    ),
    objectivePicture(),
    transmissionHalted(
        0,
        0,
        0
    ),
    messagesPanel(25) {
    char campaignSection[0x20];
    sprintf(
        campaignSection,
        "CAMPAIGN%1d",
        missionId
    );
    zReader::Node *const loadedRoot = LoadFromZrd(
        "briefing.zrd",
        campaignSection,
        0
    );
    if (loadedRoot != 0) {
        BindWidgetByName(
            loadedRoot,
            (HudUiZrdWidget *)(&transportProgress),
            "TRANSPORT_PROGRESS"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&missionName),
            "MISSION_NAME"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&objectiveSummary),
            "OBJECTIVE_SUMMARY"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&objectiveDesc),
            "OBJECTIVE_DESC"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&objectivePicture),
            "OBJECTIVE_PICT"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&messagesPanel),
            "MESSAGES"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&transmissionHalted),
            "TRANSMISSION_HALTED"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&locatorPanels[0]),
            "LOCATOR1"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&locatorPanels[1]),
            "LOCATOR2"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&locatorPanels[2]),
            "LOCATOR3"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&locatorPanels[3]),
            "LOCATOR4"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&locatorPanels[4]),
            "LOCATOR5"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&locatorPanels[5]),
            "LOCATOR6"
        );
        FreeLoadedTreeRoots((int)(unsigned int)loadedRoot);
    }

    missionName.SetVisible(0);
    messagesPanel.SetVisible(1);
    ((HudUiContainer *)(this))->SetEnabled(1);

    Time::Tick();
    zSnd_Tick(1);
    zVideo::RunPostprocessOnPrimaryBuffer();
    Update(g_FrameDeltaTimeSec);
    zVideo::Dispatch_UnlockPrimarySurfaceState();
    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: retained for native callers that construct into explicit storage
 * while retail caller 0x404180 uses the C++ new-expression constructor path.
 * Purpose: preserve storage-based construction for source-level tests.
 */
HudUiBriefingRuntime * HudUiBriefingRuntime::Constructor(
    int missionId
) {
    return new (this) HudUiBriefingRuntime(missionId);
}

/**
 * Reimplements 0x403ed0: HudUiBriefingRuntime::Destructor.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: tear down briefing UI children, queued actions, and the background base.
 */
void HudUiBriefingRuntime::Destructor() {
    SetEnabled(0);
    for (int index = 5; index >= 0; --index) {
        locatorPanels[index].~HudUiBriefingLocatorPanel();
    }

    messagesPanel.entryVector.Clear();

    ((HudUiPanel *)(&messagesPanel))->~HudUiPanel();
    transmissionHalted.~HudUiPanel();
    objectivePicture.DestructorCore();
    objectiveDesc.~HudUiPanel();
    objectiveSummary.~HudUiPanel();
    missionName.~HudUiPanel();
    transportProgress.DestructorCore();

    BriefingActionNode *const head = actionQueue.headSentinel;
    BriefingActionNode *node = head->prev;
    while (node != head) {
        BriefingActionNode *const prev = node->prev;
        node->next->prev = node->prev;
        node->prev->next = node->next;
        ::operator delete(node);
        --actionQueue.nodeCount;
        node = prev;
    }
    ::operator delete(head);

    actionQueue.headSentinel = 0;
    actionQueue.nodeCount = 0;
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x4038a0: HudUiBriefingObjectivePicture::DrawWithNoiseOverlay.
 * BN names this slot target DrawWithNoiseOverlay; the vtable slot is the
 * HudUiWidget::Draw override for the briefing objective picture.
 * Purpose: draw the objective picture and overlay transition noise while active.
 */
void HudUiBriefingObjectivePicture::Draw() {
    HudUiWidget::Draw();
    if (noiseAlpha <= 0.0) {
        return;
    }

    zVidRect32 rect;
    rect.left = GetCenterX();
    rect.top = GetCenterY();
    const zVidImagePartial *const image = this->image;
    rect.right = rect.left + (image != 0 ? image->width : 0);
    rect.bottom = rect.top + (image != 0 ? image->height : 0);
    zVid::DrawNoiseRect(
        &rect,
        noiseAlpha
    );
}

/**
 * Reimplements 0x403c10: HudUiBriefingLocatorPanel::HudUiBriefingLocatorPanel.
 * Original file: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: construct a briefing locator circle with the original red marker style.
 */
HudUiBriefingLocatorPanel::HudUiBriefingLocatorPanel()
    : HudUiCircle(
        0x64,
        0x6e,
        0x1e,
        (unsigned short)(zVid_PackColorRGB(
            0xff,
            0,
            0
        ))
    ) {
    SetVisible(0);
}

/**
 * Reimplements 0x403c90: HudUiBriefingLocatorPanel::BlitDirtyRect.
 * Original file: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: redraw the locator panel's clipped background region.
 */
void HudUiBriefingLocatorPanel::BlitDirtyRect() {
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(bltSource),
            clipRect.left,
            clipRect.top,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed as the locator draw-base target reached through briefing UI dispatch.
 * Purpose: share locator draw-base behavior with the address-backed dirty-rect blit.
 */
void HudUiBriefingLocatorPanel::DrawBase() {
    BlitDirtyRect();
}

/**
 * Reimplements 0x403cb0: HudUiBriefingLocatorPanel::Update.
 * Original file: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: animate the locator pulse radius and refresh the element state.
 */
void HudUiBriefingLocatorPanel::Update(
    float deltaSec
) {
    if ((flags & 0x10) == 0) {
        return;
    }

    clipRect.left = GetCenterX() - radius;
    clipRect.top = GetCenterY() - radius;
    clipRect.right = GetCenterX() + radius + 1;
    clipRect.bottom = GetCenterY() + radius + 1;

    if (radius > 3) {
        float radiusStep = deltaSec * 20.0;
        if (radiusStep < 1.0) {
            radiusStep = 1.0f;
        }

        const int newRadius = radius - (int)(radiusStep);
        radius = newRadius;
        radiusSquared = newRadius * newRadius;
    }

    if (radius < 3) {
        radius = 3;
        radiusSquared = 9;
    }

    HudUiElement::Update(deltaSec);
    Invalidate();
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x4045b0, 0x404640, 0x4046d0, 0x404780,
 * 0x4048a0, 0x4049d0, and 0x404b40.
 * Evidence: BN shows each Add* helper allocating an action record, then
 * linking one 0x0c-byte queue node at the action-queue sentinel.
 * Purpose: insert one action at the tail sentinel and return the updated queue count.
 */
inline int Briefing_ActionQueue::InsertAction(
    BriefingAction *action
) {
    BriefingActionNode *const head = headSentinel;
    BriefingActionNode *const next = head->next;
    BriefingActionNode *const node = new BriefingActionNode;
    node->prev = head;
    node->next = next;
    head->next = node;
    node->next->prev = node;
    node->action = action;

    ++nodeCount;
    return nodeCount;
}

/**
 * Reimplements 0x404620: BriefingAction_HideElement::Tick.
 * Purpose: hide a queued briefing UI element and complete the action.
 */
int BriefingActionHideElement::Tick(
    float
) {
    target->SetVisible(0);
    return 1;
}

/**
 * Reimplements 0x4046b0: BriefingAction_ShowElement::Tick.
 * Purpose: show and invalidate a queued briefing UI element.
 */
int BriefingActionShowElement::Tick(
    float
) {
    target->SetVisible(1);
    target->Invalidate();
    return 1;
}

/**
 * Reimplements 0x404740: BriefingAction_FadeInElement::Tick.
 * Purpose: advance the objective picture fade/noise effect until it completes.
 */
int BriefingActionFadeInElement::Tick(
    float
) {
    alpha += 0.5f;

    HudUiBriefingObjectivePicture *const widget = (HudUiBriefingObjectivePicture *)(target);
    widget->noiseAlpha = alpha;
    widget->Invalidate();

    return alpha >= 1.0f ? 1 : 0;
}

/**
 * Reimplements 0x404850: BriefingAction_SetPanelText::Tick.
 * Purpose: apply queued text to a briefing panel and make it visible.
 */
int BriefingActionSetPanelText::Tick(
    float
) {
    target->SetTextFmt(text);
    target->UpdateTextBoundsFromContent();
    target->SetVisible(1);
    target->Invalidate();
    return 1;
}

/**
 * Reimplements 0x404960: BriefingAction_SetWidgetImageTimed::Tick.
 * Purpose: install an objective image and advance its timed noise transition.
 */
int BriefingActionSetWidgetImageTimed::Tick(
    float
) {
    target->DrawBase();
    target->SetImageBorrowedAndInvalidate(imageRef);
    target->RebuildBltRectFromImage();
    target->SetVisible(1);
    ((HudUiBriefingObjectivePicture *)(target))->noiseAlpha = timer;
    target->Invalidate();

    timer -= 0.5f;
    return timer < 0.0f ? 1 : 0;
}

/**
 * Reimplements 0x404aa0: BriefingAction_PlaySample::Tick.
 * Purpose: stop any current briefing voice sample and start the queued sample.
 */
int BriefingActionPlaySample::Tick(
    float
) {
    if (g_Briefing_CurrentSndHandle != 0) {
        g_Briefing_CurrentSndHandle->StopIfActive();
    }

    zSndSample *const sample = zSnd::FindSampleByName(sampleName);
    if (sample == 0) {
        if (useVariant != 0) {
            g_Briefing_ProgressEventCode = 0x3e7;
        }

        return 1;
    }

    if (useVariant == 0) {
        g_Briefing_CurrentSndHandle = sample->PlayA3DSimple(gain);
        return 1;
    }

    sample->SetPlaybackEventHandler(Briefing::SampleEventCallback);
    g_Briefing_CurrentSndHandle = sample->PlayDirectSound(
        variantIndex,
        gain,
        0x3e7
    );
    return 1;
}

/**
 * Reimplements 0x404bb0: BriefingAction_DelayUntilProgress::Tick.
 * Purpose: wait until sample progress reaches the queued briefing progress id.
 */
int BriefingActionDelayUntilProgress::Tick(
    float
) {
    return (float)(g_Briefing_ProgressEventCode) >= requiredProgress ? 1 : 0;
}

/**
 * Reimplements 0x4045b0: Briefing_ActionQueue::AddHideElement.
 * Purpose: enqueue an action that hides one briefing UI element.
 */
int Briefing_ActionQueue::AddHideElement(
    HudUiElement *element
) {
    BriefingActionHideElement *const action = new BriefingActionHideElement;
    if (action != 0) {
        action->target = element;
    }

    return InsertAction(action);
}

/**
 * Reimplements 0x404640: Briefing_ActionQueue::AddShowElement.
 * Purpose: enqueue an action that shows one briefing UI element.
 */
int Briefing_ActionQueue::AddShowElement(
    HudUiElement *element
) {
    BriefingActionShowElement *const action = new BriefingActionShowElement;
    if (action != 0) {
        action->target = element;
    }

    return InsertAction(action);
}

/**
 * Reimplements 0x4046d0: Briefing_ActionQueue::AddFadeInElement.
 * Purpose: enqueue an objective picture fade-in action.
 */
int Briefing_ActionQueue::AddFadeInElement(
    HudUiElement *element
) {
    BriefingActionFadeInElement *const action = new BriefingActionFadeInElement;
    if (action != 0) {
        action->target = element;
        action->alpha = 0.0f;
    }

    return InsertAction(action);
}

/**
 * Reimplements 0x404780: Briefing_ActionQueue::AddSetPanelText.
 * Purpose: enqueue text replacement for a briefing panel.
 */
int Briefing_ActionQueue::AddSetPanelText(
    const char *text,
    HudUiPanel *panel
) {
    BriefingActionSetPanelText *const action = new BriefingActionSetPanelText;
    if (action != 0) {
        strncpy(
            action->text,
            text,
            sizeof(action->text)
        );
        action->target = panel;
        panel->SetVisible(0);
    }

    return InsertAction(action);
}

/**
 * Reimplements 0x4048a0: Briefing_ActionQueue::AddSetWidgetImageTimed.
 * Purpose: enqueue image replacement for a briefing widget with a timed transition.
 */
int Briefing_ActionQueue::AddSetWidgetImageTimed(
    zVidImagePartial *imageRef,
    HudUiWidget *widget
) {
    BriefingActionSetWidgetImageTimed *const action = new BriefingActionSetWidgetImageTimed;
    if (action != 0) {
        action->imageRef = imageRef;
        action->target = widget;
        widget->SetVisible(0);
        action->timer = 1.0f;
    }

    return InsertAction(action);
}

/**
 * Reimplements 0x4049d0: Briefing_ActionQueue::AddPlaySampleByName.
 * Purpose: enqueue a briefing sample playback request.
 */
int Briefing_ActionQueue::AddPlaySampleByName(
    const char *sampleName,
    float gain,
    int useVariant,
    int progressId
) {
    BriefingActionPlaySample *const action = new BriefingActionPlaySample;
    if (action != 0) {
        strncpy(
            action->sampleName,
            sampleName,
            sizeof(action->sampleName)
        );
        action->gain = gain;
        action->useVariant = useVariant;
        action->variantIndex = progressId;
    }

    return InsertAction(action);
}

/**
 * Reimplements 0x404b40: Briefing_ActionQueue::AddDelayUntilProgress.
 * Purpose: enqueue a wait action tied to the briefing sample progress event.
 */
int Briefing_ActionQueue::AddDelayUntilProgress(
    int progressId
) {
    BriefingActionDelayUntilProgress *const action = new BriefingActionDelayUntilProgress;
    if (action != 0) {
        action->requiredProgress = (float)(progressId);
    }

    return InsertAction(action);
}

namespace Briefing {
/**
 * Reimplements 0x404b30: Briefing::SampleEventCallback.
 * Purpose: record the latest briefing sample progress event code.
 */
void __fastcall SampleEventCallback(
    int progressEventCode
) {
    g_Briefing_ProgressEventCode = progressEventCode;
}

/**
 * Reimplements 0x404c80: Briefing::BuildObjectiveActionsGlobal.
 * Purpose: forward the global briefing action-build callback to the active runtime.
 */
void __fastcall BuildObjectiveActionsGlobal(
    int objectiveIndex
) {
    if (g_Briefing_Runtime != 0) {
        g_Briefing_Runtime->BuildObjectiveActionsFromIndex(objectiveIndex);
    }
}

/**
 * Reimplements 0x404180: Briefing::StartForMission.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: create the briefing runtime, load its sound set, and start the briefing thread.
 */
int __fastcall StartForMission(
    int missionId
) {
    g_Briefing_SystemActiveFlag = 1;

    HudUiBriefingRuntime *const runtime = new HudUiBriefingRuntime(missionId);

    g_Briefing_Runtime = runtime;
    sprintf(
        g_Briefing_SndSetName,
        "BRIEFING%d",
        missionId
    );
    zSndSampleSet_InitByName(g_Briefing_SndSetName);

    g_Briefing_ThreadExitedFlag = 0;
    g_Briefing_ThreadRunFlag = 0;
    if (_beginthread(
        Briefing::ThreadMain,
        0,
        0
    ) == (unsigned long)-1L) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\Briefing.cpp",
            0x202,
            "Failed to create Briefing thread (%s)",
            strerror(0)
        );
    }

    while (g_Briefing_ThreadRunFlag == 0) {
        Sleep(100);
    }

    return 1;
}

/**
 * Reimplements 0x404280: Briefing::ThreadMain.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: run the briefing input, audio, video, and UI update loop.
 */
void ThreadMain(
    void *
) {
    g_Briefing_ThreadRunFlag = 1;
    g_Briefing_AllowAdvanceFlag = 1;
    HudUi::SetInvalidateMode(0);
    const int previousHalfResMode = zVideo::SetHalfResAdjustMode(0);

    while (g_Briefing_ThreadRunFlag != 0) {
        if (zOpt::GetNetworkEnabled() == 0 && g_Briefing_AllowAdvanceFlag != 0 &&
            zInput_WaitForAnyKeyPressWithTimeoutMs(100) != 0) {
            /**
             * Reimplements data 0x4e5cb4: g_Briefing_Runtime.
             * Purpose: snapshot the active runtime while input cancellation resets the visible panels.
             */
            HudUiBriefingRuntime *const runtime = g_Briefing_Runtime;
            if (g_Briefing_CurrentSndHandle != 0) {
                g_Briefing_CurrentSndHandle->StopIfActive();
            }

            Briefing_ActionQueue *const actionQueue = BriefingActionQueue(runtime);
            actionQueue->sequenceActive = 0;
            g_Briefing_SequenceActiveFlag = 0;
            actionQueue->currentNode = actionQueue->headSentinel;

            BriefingMissionNamePanel(runtime)->SetVisible(0);
            BriefingObjectiveSummaryPanel(runtime)->SetVisible(0);
            BriefingObjectiveDescPanel(runtime)->SetVisible(0);

            HudUiBriefingObjectivePicture *const objectivePicture =
                BriefingObjectivePicture(runtime);
            objectivePicture->noiseAlpha = 1.0f;
            objectivePicture->Invalidate();

            HudUiPanel *const transmissionHalted = BriefingTransmissionHaltedPanel(runtime);
            transmissionHalted->SetTextFmt(zLoc::GetMessageString(0x110));
            transmissionHalted->SetVisible(1);
        }

        Time::Tick();
        if (g_Briefing_Runtime != 0) {
            zSnd_Tick(1);
            zVideo::RunPostprocessOnPrimaryBuffer();
            ((HudUiContainer *)(g_Briefing_Runtime))->UpdateAll(g_FrameDeltaTimeSec);
            zVideo::Dispatch_UnlockPrimarySurfaceState();
        }

        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            1,
            1
        );
    }

    zSndSampleSet_DestroyByName(g_Briefing_SndSetName);
    zVideo::SetHalfResAdjustMode(previousHalfResMode);
    HudUi::SetInvalidateMode(previousHalfResMode);
    g_Briefing_ThreadExitedFlag = 1;
}

/**
 * Reimplements 0x404bd0: Briefing::StopAndShutdownThread.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: stop the briefing thread and destroy the active briefing runtime.
 */
void __fastcall StopAndShutdownThread(
    int waitForInput
) {
    if (waitForInput != 0 && g_Briefing_SequenceActiveFlag != 0) {
        do {
            if (zInput_WaitForAnyKeyPressWithTimeoutMs(100) != 0) {
                break;
            }

            if (g_Briefing_AllowAdvanceFlag == 0) {
                break;
            }
        } while (g_Briefing_SequenceActiveFlag != 0);
    }

    /**
     * Reimplements data 0x4e5c64: g_Briefing_ThreadExitedFlag.
     * Purpose: preserve the pre-stop thread-exit state before clearing the run flag.
     */
    const int threadExited = g_Briefing_ThreadExitedFlag;
    g_Briefing_ThreadRunFlag = 0;
    if (threadExited == 0) {
        do {
            Sleep(100);
        } while (g_Briefing_ThreadExitedFlag == 0);
    }

    /**
     * Reimplements data 0x4e5cb4: g_Briefing_Runtime.
     * Purpose: destroy and clear the active runtime after the briefing thread has stopped.
     */
    HudUiBriefingRuntime *const runtime = g_Briefing_Runtime;
    if (runtime != 0) {
        runtime->Destructor();
        ::operator delete(runtime);
        g_Briefing_Runtime = 0;
    }

    g_Briefing_SystemActiveFlag = 0;
}

/**
 * Reimplements 0x404c50: Briefing::SetProgressAndSleep.
 * Purpose: update the transport progress widget and sleep between progress frames.
 */
void __stdcall SetProgressAndSleep(
    float progressValue
) {
    if (g_Briefing_Runtime != 0) {
        HudUiBriefingTransportProgress *const transportProgress =
            &g_Briefing_Runtime->transportProgress;
        transportProgress->SetNormalizedValueAndRebuild(progressValue);
    }

    Sleep(100);
}
} // namespace Briefing

/**
 * Reimplements 0x404070: HudUiBriefingRuntime::Update.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: tick queued briefing actions, invalidate briefing panels, and update the background UI.
 */
void HudUiBriefingRuntime::Update(
    float deltaSec
) {
    Briefing_ActionQueue *const actionQueue = BriefingActionQueue(this);
    if (actionQueue->sequenceActive != 0) {
        BriefingActionNode *const currentNode = actionQueue->currentNode;
        int sequenceComplete = 0;
        if (currentNode == actionQueue->headSentinel) {
            sequenceComplete = 1;
        } else if (currentNode->action->Tick(deltaSec) != 0) {
            actionQueue->currentNode = actionQueue->currentNode->prev;
        }

        if (sequenceComplete != 0) {
            g_Briefing_AllowAdvanceFlag = 0;
        }
    }

    BriefingObjectivePicture(this)->Invalidate();
    BriefingTransmissionHaltedPanel(this)->Invalidate();
    BriefingMissionNamePanel(this)->Invalidate();
    transportProgress.Invalidate();
    BriefingObjectiveSummaryPanel(this)->Invalidate();
    BriefingObjectiveDescPanel(this)->Invalidate();
    HudUiBackground::Update(deltaSec);
}

/**
 * Reimplements 0x404400: Briefing::BuildObjectiveActionsFromIndex.
 * Purpose: build the queued per-objective briefing action sequence.
 */
int HudUiBriefingRuntime::BuildObjectiveActionsFromIndex(
    int objectiveIndex
) {
    if (zOpt::GetNetworkEnabled() != 0) {
        return 0;
    }

    Briefing_ActionQueue *const actionQueue = BriefingActionQueue(this);
    const int firstProgressId = objectiveIndex * 2;

    char sampleName[0x50];
    sprintf(
        sampleName,
        "snd_briefing_c%d",
        g_HudSensorTracker.GetMissionId()
    );
    actionQueue->AddPlaySampleByName(
        sampleName,
        1.0f,
        1,
        firstProgressId
    );

    int progressId = firstProgressId;
    {
        for (int index = objectiveIndex; index < g_HudSensorTracker.objectiveCount; ++index) {
            char *objectiveSummaryText = 0;
            char *objectiveDescText = 0;
            zVidImagePartial *objectiveImage = 0;
            g_HudSensorTracker.GetObjectiveBriefingStringsAndImageRef(
                index,
                &objectiveSummaryText,
                &objectiveDescText,
                &objectiveImage
            );

            char objectiveTitle[0x20];
            zLoc::FormatMessage(
                objectiveTitle,
                sizeof(objectiveTitle),
                0x244,
                index + 1
            );

            HudUiPanel *const missionNamePanel = BriefingMissionNamePanel(this);
            HudUiPanel *const objectiveSummaryPanel = BriefingObjectiveSummaryPanel(this);
            HudUiPanel *const objectiveDescPanel = BriefingObjectiveDescPanel(this);
            HudUiBriefingObjectivePicture *const objectivePicture = BriefingObjectivePicture(this);
            HudUiBriefingLocatorPanel *const locatorPanel = BriefingLocatorPanel(
                this,
                index
            );

            actionQueue->AddDelayUntilProgress(progressId);
            actionQueue->AddSetPanelText(
                objectiveTitle,
                missionNamePanel
            );
            actionQueue->AddSetPanelText(
                objectiveSummaryText,
                objectiveSummaryPanel
            );
            actionQueue->AddSetWidgetImageTimed(
                objectiveImage,
                objectivePicture
            );
            actionQueue->AddShowElement(locatorPanel);
            actionQueue->AddSetPanelText(
                objectiveDescText,
                objectiveDescPanel
            );
            actionQueue->AddDelayUntilProgress(progressId + 1);
            actionQueue->AddHideElement(locatorPanel);
            actionQueue->AddHideElement(missionNamePanel);
            actionQueue->AddHideElement(objectiveSummaryPanel);
            actionQueue->AddFadeInElement(objectivePicture);
            actionQueue->AddHideElement(objectiveDescPanel);

            progressId += 2;
        }
    }

    actionQueue->sequenceActive = 1;
    g_Briefing_SequenceActiveFlag = 1;
    actionQueue->currentNode = actionQueue->headSentinel->prev;
    return 1;
}
