#include "Battlesport/briefing.h"

#include "Battlesport/hud_sensor_tracker.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

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
    /**
     * Original inline constructor; no standalone retail function exists.
     * Observed in caller 0x4045b0 as the hide-element action allocation path.
     * Purpose: bind a hide action to the queued briefing UI element.
     */
    BriefingActionHideElement(HudUiElement *element) {
        target = element;
    }

    int Tick(float deltaSec);
};

struct BriefingActionShowElement : BriefingActionElementTarget {
    /**
     * Original inline constructor; no standalone retail function exists.
     * Observed in caller 0x404640 as the show-element action allocation path.
     * Purpose: bind a show action to the queued briefing UI element.
     */
    BriefingActionShowElement(HudUiElement *element) {
        target = element;
    }

    int Tick(float deltaSec);
};

struct BriefingActionFadeInElement : BriefingActionElementTarget {
    float alpha;

    /**
     * Original inline constructor; no standalone retail function exists.
     * Observed in caller 0x4046d0 as the fade-in action allocation path.
     * Purpose: bind a fade action to the target element and start alpha at zero.
     */
    BriefingActionFadeInElement(HudUiElement *element)
        : alpha(0.0f) {
        target = element;
    }

    int Tick(float deltaSec);
};

struct BriefingActionSetPanelText : BriefingAction {
    char text[0x100];
    HudUiPanel *target;

    /**
     * Original inline constructor; no standalone retail function exists.
     * Observed in caller 0x404780 as the panel-text action allocation path.
     * Purpose: copy the queued text, bind the target panel, and hide it until Tick.
     */
    BriefingActionSetPanelText(
        const char *textValue,
        HudUiPanel *panel
    ) {
        strncpy(
            text,
            textValue,
            sizeof(text)
        );
        target = panel;
        panel->SetVisible(0);
    }

    int Tick(float deltaSec);
};

struct BriefingActionSetWidgetImageTimed : BriefingAction {
    zVidImagePartial *imageRef;
    HudUiWidget *target;
    float timer;

    /**
     * Original inline constructor; no standalone retail function exists.
     * Observed in caller 0x4048a0 as the timed-image action allocation path.
     * Purpose: bind the image/widget action, hide the widget, and seed its timer.
     */
    BriefingActionSetWidgetImageTimed(
        zVidImagePartial *imageRefValue,
        HudUiWidget *widget
    ) {
        imageRef = imageRefValue;
        target = widget;
        widget->SetVisible(0);
        timer = 1.0f;
    }

    int Tick(float deltaSec);
};

struct BriefingActionPlaySample : BriefingAction {
    char sampleName[0x50];
    float gain;
    int useVariant;
    int variantIndex;

    /**
     * Original inline constructor; no standalone retail function exists.
     * Observed in caller 0x4049d0 as the sample-play action allocation path.
     * Purpose: copy the queued sample request and playback variant state.
     */
    BriefingActionPlaySample(
        const char *sampleNameValue,
        float gainValue,
        int useVariantValue,
        int progressId
    ) {
        strncpy(
            sampleName,
            sampleNameValue,
            sizeof(sampleName)
        );
        gain = gainValue;
        useVariant = useVariantValue;
        variantIndex = progressId;
    }

    int Tick(float deltaSec);
};

struct BriefingActionDelayUntilProgress : BriefingAction {
    float requiredProgress;

    /**
     * Original inline constructor; no standalone retail function exists.
     * Observed in caller 0x404b40 as the progress-delay action allocation path.
     * Purpose: convert the required progress event id into the queued threshold.
     */
    BriefingActionDelayUntilProgress(int progressId)
        : requiredProgress((float)(progressId)) {
    }

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
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
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
        BriefingActionNode *const next = node->next;
        delete next->action;
        delete next;
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
    const zVidImagePartial *image = this->image;
    const int imageWidth = image != 0 ? image->width : 0;
    rect.right = GetCenterX() + imageWidth;
    image = this->image;
    const int imageHeight = image != 0 ? image->height : 0;
    rect.bottom = GetCenterY() + imageHeight;
    zVid::DrawNoiseRect(
        &rect,
        noiseAlpha
    );
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
    HudUiBriefingLocatorPanel *const locatorPanel = this;
    locatorPanel->SetVisible(0);
}

/**
 * Reimplements 0x403c80: HudUiCircle::DrawDirtyForwarder.
 * Original file: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: route the briefing table dispatch slot to HudUiCircle::Draw.
 */
void HudUiCircle::DrawDirtyForwarder() {
    HudUiCircle::Draw();
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
    unsigned int currentFlags = flags;
    if ((~currentFlags & 0x10) == 0) {
        return;
    }

    int currentRadius = radius;
    clipRect.left = GetCenterX() - currentRadius;
    currentRadius = radius;
    clipRect.top = GetCenterY() - currentRadius;
    currentRadius = radius;
    clipRect.right = GetCenterX() + currentRadius + 1;
    currentRadius = radius;
    clipRect.bottom = GetCenterY() + currentRadius + 1;

    currentRadius = radius;
    if (currentRadius > 3) {
        float radiusStep = deltaSec * 20.0;
        if (radiusStep < 1.0) {
            radiusStep = 1.0f;
        }

        const int newRadius = currentRadius - (int)(radiusStep);
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
 * Reimplements 0x403d90: HudUiBriefingRuntime::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: run briefing runtime destruction and optionally free the object storage.
 */
HudUiBackground * HudUiBriefingRuntime::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Reimplements 0x403e20: HudUiCompositePanel::Destructor.
 * Physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: destroy composite-panel entries, free vector storage, and tear down
 * the inherited panel state.
 */
void HudUiCompositePanel::Destructor() {
    for (HudUiCompositePanelEntry *entry = entryVector.begin; entry != entryVector.end; ++entry) {
        entry->panel.ScalarDeletingDestructor(0);
    }

    ::operator delete(entryVector.begin);
    entryVector.begin = 0;
    entryVector.end = 0;
    entryVector.capacityEnd = 0;

    HudUiPanel::~HudUiPanel();
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
    BriefingActionNode *node = head->next;
    while (node != head) {
        BriefingActionNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ::operator delete(node);
        --actionQueue.nodeCount;
        node = next;
    }
    ::operator delete(head);

    actionQueue.headSentinel = 0;
    actionQueue.nodeCount = 0;
    this->HudUiBackground::~HudUiBackground();
}

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
        BriefingActionNode *const headSentinel = actionQueue->headSentinel;
        bool sequenceComplete = (currentNode == headSentinel);
        if (!sequenceComplete) {
            if (currentNode->action->Tick(deltaSec) != 0) {
                actionQueue->currentNode = actionQueue->currentNode->next;
            }
        }

        if (sequenceComplete) {
            g_Briefing_AllowAdvanceFlag = 0;
        }
    }

    BriefingObjectivePicture(this)->Invalidate();
    BriefingTransmissionHaltedPanel(this)->Invalidate();
    BriefingMissionNamePanel(this)->Invalidate();
    transportProgress.Invalidate();
    BriefingObjectiveSummaryPanel(this)->Invalidate();
    BriefingObjectiveDescPanel(this)->Invalidate();
    HudUiBackgroundContainer::UpdateAll(deltaSec);
}

/**
 * Reimplements 0x404140: zInput_WaitForAnyKeyPressWithTimeoutMs.
 * Original source path: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: poll keyboard input in 100 ms Sleep slices until a key arrives or
 * the caller's timeout budget expires.
 */
extern "C" int __fastcall zInput_WaitForAnyKeyPressWithTimeoutMs(
    int timeoutMs
) {
    int result = 0;
    int remainingMs = timeoutMs;
    if (timeoutMs > 0) {
        void (WINAPI *const sleepProc)(DWORD) = Sleep;
        while (remainingMs > 0) {
            if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
                result = 1;
                break;
            }

            sleepProc(100);
            remainingMs -= 100;
        }
    }

    return result;
}

namespace Briefing {
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

} // namespace Briefing

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
    actionQueue->currentNode = actionQueue->headSentinel->next;
    return 1;
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
    BriefingActionNode *const prev = head->prev;
    BriefingActionNode *const node = new BriefingActionNode;
    node->next = head != 0 ? head : node;
    node->prev = prev != 0 ? prev : node;
    head->prev = node;
    node->prev->next = node;
    new (&node->action) BriefingAction *(action);

    ++nodeCount;
    return nodeCount;
}

/**
 * Reimplements 0x4045b0: Briefing_ActionQueue::AddHideElement.
 * Purpose: enqueue an action that hides one briefing UI element.
 */
int Briefing_ActionQueue::AddHideElement(
    HudUiElement *element
) {
    BriefingActionHideElement *const action = new BriefingActionHideElement(element);
    return InsertAction(action);
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
 * Reimplements 0x404640: Briefing_ActionQueue::AddShowElement.
 * Purpose: enqueue an action that shows one briefing UI element.
 */
int Briefing_ActionQueue::AddShowElement(
    HudUiElement *element
) {
    BriefingActionShowElement *const action = new BriefingActionShowElement(element);
    return InsertAction(action);
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
 * Reimplements 0x4046d0: Briefing_ActionQueue::AddFadeInElement.
 * Purpose: enqueue an objective picture fade-in action.
 */
int Briefing_ActionQueue::AddFadeInElement(
    HudUiElement *element
) {
    BriefingActionFadeInElement *const action = new BriefingActionFadeInElement(element);
    return InsertAction(action);
}

/**
 * Reimplements 0x404740: BriefingAction_FadeInElement::Tick.
 * Purpose: advance the objective picture fade/noise effect until it completes.
 */
int BriefingActionFadeInElement::Tick(
    float
) {
    HudUiBriefingObjectivePicture *const widget = (HudUiBriefingObjectivePicture *)(target);
    widget->SetNoiseAlpha(alpha = alpha + 0.5f);
    widget->Invalidate();

    return alpha >= 1.0 ? 1 : 0;
}

/**
 * Reimplements 0x404780: Briefing_ActionQueue::AddSetPanelText.
 * Purpose: enqueue text replacement for a briefing panel.
 */
int Briefing_ActionQueue::AddSetPanelText(
    const char *text,
    HudUiPanel *panel
) {
    BriefingActionSetPanelText *const action =
        new BriefingActionSetPanelText(text, panel);
    return InsertAction(action);
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
 * Reimplements 0x4048a0: Briefing_ActionQueue::AddSetWidgetImageTimed.
 * Purpose: enqueue image replacement for a briefing widget with a timed transition.
 */
int Briefing_ActionQueue::AddSetWidgetImageTimed(
    zVidImagePartial *imageRef,
    HudUiWidget *widget
) {
    BriefingActionSetWidgetImageTimed *const action =
        new BriefingActionSetWidgetImageTimed(imageRef, widget);
    return InsertAction(action);
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
    HudUiBriefingObjectivePicture *const widget = (HudUiBriefingObjectivePicture *)(target);
    widget->SetNoiseAlpha(timer);
    widget->Invalidate();

    timer -= 0.5f;
    return timer < 0.0 ? 1 : 0;
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
    BriefingActionPlaySample *const action =
        new BriefingActionPlaySample(
            sampleName,
            gain,
            useVariant,
            progressId
        );
    return InsertAction(action);
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

} // namespace Briefing

/**
 * Reimplements 0x404b40: Briefing_ActionQueue::AddDelayUntilProgress.
 * Purpose: enqueue a wait action tied to the briefing sample progress event.
 */
int Briefing_ActionQueue::AddDelayUntilProgress(
    int progressId
) {
    BriefingActionDelayUntilProgress *const action =
        new BriefingActionDelayUntilProgress(progressId);
    return InsertAction(action);
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

namespace Briefing {
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

} // namespace Briefing
