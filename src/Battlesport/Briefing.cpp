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

} // namespace

extern "C" {

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.g-briefing-threadrunflag
 * @recoil-artifact defines .data recoil:data:0x4e5c60: g_Briefing_ThreadRunFlag.
 * Purpose: control the lifetime of the briefing update thread.
 */
int g_Briefing_ThreadRunFlag = 0;

/**
 * Purpose: signal that the briefing update thread has left its loop.
 */
int g_Briefing_ThreadExitedFlag = 0;

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.g-briefing-sequenceactiveflag
 * @recoil-artifact defines .data recoil:data:0x4e5c6c: g_Briefing_SequenceActiveFlag.
 * Purpose: expose whether the queued briefing sequence is still active.
 */
int g_Briefing_SequenceActiveFlag = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.g-briefing-sndsetname
 * @recoil-artifact defines .data recoil:data:0x4e5c70: g_Briefing_SndSetName.
 * Purpose: store the mission briefing sample-set name for thread startup and shutdown.
 */
char g_Briefing_SndSetName[0x40] = {0};

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.g-briefing-currentsndhandle
 * @recoil-artifact defines .data recoil:data:0x4e5cb0: g_Briefing_CurrentSndHandle.
 * Purpose: retain the currently playing briefing voice sample so later actions can stop it.
 */
zSndPlayHandle *g_Briefing_CurrentSndHandle = 0;

/**
 * Purpose: hold the active briefing UI runtime while the mission briefing thread is alive.
 */
HudUiBriefingRuntime *g_Briefing_Runtime = 0;

extern "C" {

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.g-briefing-allowadvanceflag
 * @recoil-artifact defines .data recoil:data:0x4e5cb8: g_Briefing_AllowAdvanceFlag.
 * Purpose: gate user input that can advance or halt the current briefing sequence.
 */
int g_Briefing_AllowAdvanceFlag = 0;

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.g-briefing-systemactiveflag
 * @recoil-artifact defines .data recoil:data:0x56bbf8: g_Briefing_SystemActiveFlag.
 * Purpose: indicate that the briefing subsystem is currently active.
 */
int g_Briefing_SystemActiveFlag = 0;

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.g-briefing-progresseventcode
 * @recoil-artifact defines .data recoil:data:0x4da24c: g_Briefing_ProgressEventCode.
 * Purpose: track the most recent briefing sample progress event, initialized to no event.
 */
int g_Briefing_ProgressEventCode = -1;
}

/**
 * Provider-boundary 0x403db0: official VC5
 * std::list<BriefingAction *>::~list COMDAT emitted from the queue member.
 * Purpose: destroy the queue's list nodes through the compiler's canonical
 * xlist implementation while leaving the pointed-to BriefingAction objects
 * under the queue's explicit action-lifetime policy.
 */

/**
 * Original inline constructor; no standalone retail function exists.
 * Observed in caller 0x403930 as the first runtime member construction state.
 * The official VC5 std::list constructor supplies the empty sentinel; the
 * iterator remains default constructed until a sequence starts.
 * Purpose: initialize the briefing action queue's independent active state.
 */
inline Briefing_ActionQueue::Briefing_ActionQueue()
    : active(0) {
    g_Briefing_ProgressEventCode = -1;
}

/**
 * Original inline constructor; no standalone retail function exists.
 * Observed in caller 0x403930 as the objective-picture member construction.
 * Purpose: construct the briefing picture widget and clear its noise overlay state.
 */
inline HudUiBriefingObjectivePicture::HudUiBriefingObjectivePicture()
    : HudUiWidget(0) {
    noiseAlpha = 0.0f;
    ((HudUiElement *)(this))->Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefingobjectivepicture-draw
 * @recoil-artifact defines .text recoil:function:0x4038a0: HudUiBriefingObjectivePicture::DrawWithNoiseOverlay.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefingruntime-huduibriefingruntime
 * @recoil-artifact defines .text recoil:function:0x403930: HudUiBriefingRuntime::HudUiBriefingRuntime.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: construct the briefing UI runtime, bind its ZRD widgets, and run the first frame.
 */
HudUiBriefingRuntime::HudUiBriefingRuntime(
    int missionId
) : HudUiBackground(),
    actionQueue(),
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
    HudUiBriefingTransportProgress *const progress = &transportProgress;
    HudUiPanel *const namePanel = &missionName;
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
            (HudUiZrdWidget *)(progress),
            "TRANSPORT_PROGRESS"
        );
        BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(namePanel),
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

    namePanel->SetVisible(0);
    messagesPanel.SetVisible(1);
    HudUiBackground *const background = this;
    background->SetEnabled(1);

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
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefinglocatorpanel-huduibriefinglocatorpanel
 * @recoil-artifact defines .text recoil:function:0x403c10: HudUiBriefingLocatorPanel::HudUiBriefingLocatorPanel.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefinglocatorpanel-draw
 * @recoil-artifact defines .text recoil:function:0x403c80: HudUiBriefingLocatorPanel::Draw.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: draw the locator circle through the inherited circle implementation.
 */
void HudUiBriefingLocatorPanel::Draw() {
    HudUiCircle::Draw();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefinglocatorpanel-drawbase
 * @recoil-artifact defines .text recoil:function:0x403c90: HudUiBriefingLocatorPanel::DrawBase.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: redraw the locator panel's clipped background region.
 */
void HudUiBriefingLocatorPanel::DrawBase() {
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefinglocatorpanel-update
 * @recoil-artifact defines .text recoil:function:0x403cb0: HudUiBriefingLocatorPanel::Update.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduicompositepanel-destructor-huduicompositepanel
 * @recoil-artifact defines .text recoil:function:0x403e20: HudUiCompositePanel::~HudUiCompositePanel.
 * Physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: destroy the entry vector before the inherited panel base.
 */
inline HudUiCompositePanel::~HudUiCompositePanel() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefingruntime-destructor-huduibriefingruntime
 * @recoil-artifact defines .text recoil:function:0x403ed0: HudUiBriefingRuntime::~HudUiBriefingRuntime.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: tear down briefing UI children, queued actions, and the background base.
 */
HudUiBriefingRuntime::~HudUiBriefingRuntime() {
    HudUiContainer *const container = this;
    container->SetEnabled(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefingruntime-update
 * @recoil-artifact defines .text recoil:function:0x404070: HudUiBriefingRuntime::Update.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: tick queued briefing actions, invalidate briefing panels, and update the background UI.
 */
void HudUiBriefingRuntime::Update(
    float deltaSec
) {
    Briefing_ActionQueue *const actionQueue = &this->actionQueue;
    if (actionQueue->active != 0) {
        int sequenceComplete = 0;
        if (actionQueue->current != actionQueue->actions.end()) {
            if ((*actionQueue->current)->Tick(deltaSec) != 0) {
                ++actionQueue->current;
            }
        } else {
            sequenceComplete = 1;
        }

        if (sequenceComplete) {
            g_Briefing_AllowAdvanceFlag = 0;
        }
    }

    this->objectivePicture.Invalidate();
    this->transmissionHalted.Invalidate();
    this->missionName.Invalidate();
    transportProgress.Invalidate();
    this->objectiveSummary.Invalidate();
    this->objectiveDesc.Invalidate();
    HudUiBackgroundContainer::UpdateAll(deltaSec);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.zinput-waitforanykeypresswithtimeoutms
 * @recoil-artifact defines .text recoil:function:0x404140: zInput_WaitForAnyKeyPressWithTimeoutMs.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.startformission
 * @recoil-artifact defines .text recoil:function:0x404180: Briefing::StartForMission.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.threadmain
 * @recoil-artifact defines .text recoil:function:0x404280: Briefing::ThreadMain.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: run the briefing input, audio, video, and UI update loop.
 */
void __cdecl ThreadMain(
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
             * Purpose: snapshot the active runtime while input cancellation resets the visible panels.
             */
            HudUiBriefingRuntime *const runtime = g_Briefing_Runtime;
            if (g_Briefing_CurrentSndHandle != 0) {
                g_Briefing_CurrentSndHandle->StopIfActive();
            }

            Briefing_ActionQueue *const actionQueue = &runtime->actionQueue;
            actionQueue->active = 0;
            g_Briefing_SequenceActiveFlag = 0;
            actionQueue->current = actionQueue->actions.end();

            runtime->missionName.SetVisible(0);
            runtime->objectiveSummary.SetVisible(0);
            runtime->objectiveDesc.SetVisible(0);

            HudUiBriefingObjectivePicture *const objectivePicture =
                &runtime->objectivePicture;
            objectivePicture->noiseAlpha = 1.0f;
            objectivePicture->Invalidate();

            HudUiPanel *const transmissionHalted = &runtime->transmissionHalted;
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.huduibriefingruntime-buildobjectiveactionsfromindex
 * @recoil-artifact defines .text recoil:function:0x404400: Briefing::BuildObjectiveActionsFromIndex.
 * Purpose: build the queued per-objective briefing action sequence.
 */
int HudUiBriefingRuntime::BuildObjectiveActionsFromIndex(
    int objectiveIndex
) {
    HudUiBriefingRuntime *const runtime = this;
    int progressId = objectiveIndex + objectiveIndex;
    if (zOpt::GetNetworkEnabled() != 0) {
        return 0;
    }

    char sampleName[0x50];
    sprintf(
        sampleName,
        "snd_briefing_c%d",
        g_HudSensorTracker.GetMissionId()
    );

    Briefing_ActionQueue *const actionQueue = &runtime->actionQueue;
    actionQueue->AddPlaySampleByName(
        sampleName,
        1.0f,
        1,
        progressId
    );

    unsigned int index = (unsigned int)(objectiveIndex);
    if (index < (unsigned int)(g_HudSensorTracker.objectiveCount)) {
        HudUiPanel *const missionNamePanel = &runtime->missionName;
        HudUiPanel *const objectiveSummaryPanel = &runtime->objectiveSummary;
        HudUiBriefingObjectivePicture *const objectivePicture =
            &runtime->objectivePicture;
        HudUiPanel *const objectiveDescPanel = &runtime->objectiveDesc;
        HudUiBriefingLocatorPanel *locatorPanel =
            &runtime->locatorPanels[(int)(index)];

        do {
            char *objectiveSummaryText;
            char *objectiveDescText;
            zVidImagePartial *objectiveImage;
            g_HudSensorTracker.GetObjectiveBriefingStringsAndImageRef(
                (int)(index),
                &objectiveSummaryText,
                &objectiveDescText,
                &objectiveImage
            );
            ++index;

            char objectiveTitle[0x20];
            zLoc::FormatMessage(
                objectiveTitle,
                sizeof(objectiveTitle),
                0x244,
                (int)(index)
            );

            actionQueue->AddDelayUntilProgress(progressId++);
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
            actionQueue->AddDelayUntilProgress(progressId++);
            actionQueue->AddHideElement(locatorPanel);
            actionQueue->AddHideElement(missionNamePanel);
            actionQueue->AddHideElement(objectiveSummaryPanel);
            actionQueue->AddFadeInElement(objectivePicture);
            actionQueue->AddHideElement(objectiveDescPanel);

            ++locatorPanel;
        } while (index < (unsigned int)(g_HudSensorTracker.objectiveCount));
    }

    actionQueue->active = 1;
    g_Briefing_SequenceActiveFlag = 1;
    actionQueue->current = actionQueue->actions.begin();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefing-actionqueue-addhideelement
 * @recoil-artifact defines .text recoil:function:0x4045b0: Briefing_ActionQueue::AddHideElement.
 * Purpose: enqueue an action that hides one briefing UI element.
 */
int Briefing_ActionQueue::AddHideElement(
    HudUiElement *element
) {
    BriefingActionHideElement *const action = new BriefingActionHideElement(element);
    actions.push_back(action);
    return actions.size();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefingactionhideelement-tick
 * @recoil-artifact defines .text recoil:function:0x404620: BriefingAction_HideElement::Tick.
 * Purpose: hide a queued briefing UI element and complete the action.
 */
int BriefingActionHideElement::Tick(
    float
) {
    target->SetVisible(0);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefing-actionqueue-addshowelement
 * @recoil-artifact defines .text recoil:function:0x404640: Briefing_ActionQueue::AddShowElement.
 * Purpose: enqueue an action that shows one briefing UI element.
 */
int Briefing_ActionQueue::AddShowElement(
    HudUiElement *element
) {
    BriefingActionShowElement *const action = new BriefingActionShowElement(element);
    actions.push_back(action);
    return actions.size();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefingactionshowelement-tick
 * @recoil-artifact defines .text recoil:function:0x4046b0: BriefingAction_ShowElement::Tick.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefing-actionqueue-addfadeinelement
 * @recoil-artifact defines .text recoil:function:0x4046d0: Briefing_ActionQueue::AddFadeInElement.
 * Purpose: enqueue an objective picture fade-in action.
 */
int Briefing_ActionQueue::AddFadeInElement(
    HudUiElement *element
) {
    BriefingActionFadeInElement *const action = new BriefingActionFadeInElement(element);
    actions.push_back(action);
    return actions.size();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefingactionfadeinelement-tick
 * @recoil-artifact defines .text recoil:function:0x404740: BriefingAction_FadeInElement::Tick.
 * Purpose: advance the objective picture fade/noise effect until it completes.
 */
int BriefingActionFadeInElement::Tick(
    float
) {
    const float nextAlpha = alpha + 0.5f;
    alpha = nextAlpha;
    HudUiBriefingObjectivePicture *const widget = (HudUiBriefingObjectivePicture *)(target);
    widget->SetNoiseAlpha(nextAlpha);
    widget->Invalidate();

    return nextAlpha >= 1.0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefing-actionqueue-addsetpaneltext
 * @recoil-artifact defines .text recoil:function:0x404780: Briefing_ActionQueue::AddSetPanelText.
 * Purpose: enqueue text replacement for a briefing panel.
 */
int Briefing_ActionQueue::AddSetPanelText(
    const char *text,
    HudUiPanel *panel
) {
    BriefingActionSetPanelText *const action =
        new BriefingActionSetPanelText(text, panel);
    actions.push_back(action);
    return actions.size();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefingactionsetpaneltext-tick
 * @recoil-artifact defines .text recoil:function:0x404850: BriefingAction_SetPanelText::Tick.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefing-actionqueue-addsetwidgetimagetimed
 * @recoil-artifact defines .text recoil:function:0x4048a0: Briefing_ActionQueue::AddSetWidgetImageTimed.
 * Purpose: enqueue image replacement for a briefing widget with a timed transition.
 */
int Briefing_ActionQueue::AddSetWidgetImageTimed(
    zVidImagePartial *imageRef,
    HudUiWidget *widget
) {
    BriefingActionSetWidgetImageTimed *const action =
        new BriefingActionSetWidgetImageTimed(imageRef, widget);
    actions.push_back(action);
    return actions.size();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefingactionsetwidgetimagetimed-tick
 * @recoil-artifact defines .text recoil:function:0x404960: BriefingAction_SetWidgetImageTimed::Tick.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefing-actionqueue-addplaysamplebyname
 * @recoil-artifact defines .text recoil:function:0x4049d0: Briefing_ActionQueue::AddPlaySampleByName.
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
    actions.push_back(action);
    return actions.size();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefingactionplaysample-tick
 * @recoil-artifact defines .text recoil:function:0x404aa0: BriefingAction_PlaySample::Tick.
 * Purpose: stop any current briefing voice sample and start the queued sample.
 */
int BriefingActionPlaySample::Tick(
    float
) {
    zSndPlayHandle *handle = g_Briefing_CurrentSndHandle;
    if (handle != 0) {
        handle->StopIfActive();
    }

    zSndSample *sample = zSnd::FindSampleByName(sampleName);
    if (sample != 0) {
        if (useVariant != 0) {
            sample->SetPlaybackEventHandler(Briefing::SampleEventCallback);
            g_Briefing_CurrentSndHandle = sample->PlayDirectSound(
                variantIndex,
                gain,
                0x3e7
            );
            return 1;
        }

        g_Briefing_CurrentSndHandle = sample->PlayA3DSimple(gain);
        return 1;
    } else {
        if (useVariant != 0) {
            g_Briefing_ProgressEventCode = 0x3e7;
        }
    }

    return 1;
}

namespace Briefing {
/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.sampleeventcallback
 * @recoil-artifact defines .text recoil:function:0x404b30: Briefing::SampleEventCallback.
 * Purpose: record the latest briefing sample progress event code.
 */
void __fastcall SampleEventCallback(
    int progressEventCode
) {
    g_Briefing_ProgressEventCode = progressEventCode;
}

} // namespace Briefing

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefing-actionqueue-adddelayuntilprogress
 * @recoil-artifact defines .text recoil:function:0x404b40: Briefing_ActionQueue::AddDelayUntilProgress.
 * Purpose: enqueue a wait action tied to the briefing sample progress event.
 */
int Briefing_ActionQueue::AddDelayUntilProgress(
    int progressId
) {
    BriefingActionDelayUntilProgress *const action =
        new BriefingActionDelayUntilProgress(progressId);
    actions.push_back(action);
    return actions.size();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.briefingactiondelayuntilprogress-tick
 * @recoil-artifact defines .text recoil:function:0x404bb0: BriefingAction_DelayUntilProgress::Tick.
 * Purpose: wait until sample progress reaches the queued briefing progress id.
 */
int BriefingActionDelayUntilProgress::Tick(
    float
) {
    return (float)(g_Briefing_ProgressEventCode) >= requiredProgress ? 1 : 0;
}

namespace Briefing {
/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.stopandshutdownthread
 * @recoil-artifact defines .text recoil:function:0x404bd0: Briefing::StopAndShutdownThread.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\Briefing.cpp.
 * Purpose: stop the briefing thread and destroy the active briefing runtime.
 */
void __fastcall StopAndShutdownThread(
    int waitForInput
) {
    if (waitForInput != 0) {
        while (g_Briefing_SequenceActiveFlag != 0) {
            if (zInput_WaitForAnyKeyPressWithTimeoutMs(100) != 0) {
                break;
            }

            if (g_Briefing_AllowAdvanceFlag == 0) {
                break;
            }
        }
    }

    /**
     * Purpose: preserve the pre-stop thread-exit state before clearing the run flag.
     */
    const int threadExited = g_Briefing_ThreadExitedFlag;
    g_Briefing_ThreadRunFlag = 0;
    if (threadExited == 0) {
        void (WINAPI *const sleepProc)(DWORD) = Sleep;
        do {
            sleepProc(100);
        } while (g_Briefing_ThreadExitedFlag == 0);
    }

    /**
     * Purpose: destroy and clear the active runtime after the briefing thread has stopped.
     */
    HudUiBriefingRuntime *const runtime = g_Briefing_Runtime;
    if (runtime != 0) {
        delete runtime;
        g_Briefing_Runtime = 0;
    }

    g_Briefing_SystemActiveFlag = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.briefing.setprogressandsleep
 * @recoil-artifact defines .text recoil:function:0x404c50: Briefing::SetProgressAndSleep.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.buildobjectiveactionsglobal
 * @recoil-artifact defines .text recoil:function:0x404c80: Briefing::BuildObjectiveActionsGlobal.
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
