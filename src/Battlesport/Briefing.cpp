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

HudUiBriefingRuntime *BriefingLayout(
    HudUiBriefingRuntime *runtime
) {
    return runtime;
}

HudUiPanel *BriefingPanel(
    HudUiPanel *panel
) {
    return panel;
}

Briefing_ActionQueue *BriefingActionQueue(
    HudUiBriefingRuntime *runtime
) {
    return &BriefingLayout(runtime)->actionQueue;
}

HudUiPanel *BriefingMissionNamePanel(
    HudUiBriefingRuntime *runtime
) {
    return BriefingPanel(&BriefingLayout(runtime)->missionName);
}

HudUiPanel *BriefingObjectiveSummaryPanel(
    HudUiBriefingRuntime *runtime
) {
    return BriefingPanel(&BriefingLayout(runtime)->objectiveSummary);
}

HudUiPanel *BriefingObjectiveDescPanel(
    HudUiBriefingRuntime *runtime
) {
    return BriefingPanel(&BriefingLayout(runtime)->objectiveDesc);
}

HudUiBriefingObjectivePicture *BriefingObjectivePicture(
    HudUiBriefingRuntime *runtime
) {
    return &BriefingLayout(runtime)->objectivePicture;
}

HudUiPanel *BriefingTransmissionHaltedPanel(
    HudUiBriefingRuntime *runtime
) {
    return BriefingPanel(&BriefingLayout(runtime)->transmissionHalted);
}

HudUiBriefingLocatorPanel *BriefingLocatorPanel(
    HudUiBriefingRuntime *runtime,
    int objectiveIndex
) {
    return &BriefingLayout(runtime)->locatorPanels[objectiveIndex];
}

} // namespace

HudUiBriefingRuntime *g_Briefing_Runtime = 0;
zSndPlayHandle *g_Briefing_CurrentSndHandle = 0;
char g_Briefing_SndSetName[0x40] = {0};
extern "C" {
int g_Briefing_ThreadRunFlag = 0;
int g_Briefing_ThreadExitedFlag = 0;
int g_Briefing_SequenceActiveFlag = 0;
int g_Briefing_AllowAdvanceFlag = 0;
int g_Briefing_SystemActiveFlag = 0;
int g_Briefing_ProgressEventCode = 0;
}

// Reimplements 0x403930: HudUiBriefingRuntime::Constructor (D:\Proj\Battlesport\Briefing.cpp)
HudUiBriefingRuntime * HudUiBriefingRuntime::Constructor(
    int missionId
) {
    HudUiBriefingRuntime *const layout = BriefingLayout(this);
    new ((HudUiBackground *)layout) HudUiBackground;

    layout->actionQueue.missionId = missionId & 0xff;
    BriefingActionNode *const sentinel = new BriefingActionNode;
    sentinel->prev = sentinel;
    sentinel->next = sentinel;
    layout->actionQueue.headSentinel = sentinel;
    layout->actionQueue.nodeCount = 0;
    layout->actionQueue.sequenceActive = 0;
    g_Briefing_ProgressEventCode = -1;

    layout->transportProgress.Constructor();

    BriefingPanel(&layout->missionName)->ConstructorDefault(
        0,
        0,
        0
    );
    BriefingPanel(&layout->objectiveSummary)->ConstructorDefault(
        0,
        0,
        0
    );
    BriefingPanel(&layout->objectiveDesc)->ConstructorDefault(
        0,
        0,
        0
    );

    layout->objectivePicture.Constructor(0);
    layout->objectivePicture.noiseAlpha = 0.0f;
    layout->objectivePicture.Invalidate();

    BriefingPanel(&layout->transmissionHalted)->ConstructorDefault(
        0,
        0,
        0
    );
    layout->messagesPanel.ConstructorWithEntryCount(0x19);
    {
        for (int index = 0; index < 6; ++index) {
            layout->locatorPanels[index].Constructor();
        }
    }

    char campaignSection[0x20];
    sprintf(
        campaignSection,
        "CAMPAIGN%1d",
        missionId
    );
    zReader::Node *const loadedRoot = layout->LoadFromZrd(
        "briefing.zrd",
        campaignSection,
        0
    );
    if (loadedRoot != 0) {
        layout->BindWidgetByName(
            loadedRoot,
            (HudUiWidget *)(&layout->transportProgress),
            "TRANSPORT_PROGRESS"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->missionName),
            "MISSION_NAME"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->objectiveSummary),
            "OBJECTIVE_SUMMARY"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->objectiveDesc),
            "OBJECTIVE_DESC"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->objectivePicture),
            "OBJECTIVE_PICT"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->messagesPanel),
            "MESSAGES"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->transmissionHalted),
            "TRANSMISSION_HALTED"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->locatorPanels[0]),
            "LOCATOR1"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->locatorPanels[1]),
            "LOCATOR2"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->locatorPanels[2]),
            "LOCATOR3"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->locatorPanels[3]),
            "LOCATOR4"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->locatorPanels[4]),
            "LOCATOR5"
        );
        layout->BindPrimitiveNodeToElement(
            loadedRoot,
            (HudUiElement *)(&layout->locatorPanels[5]),
            "LOCATOR6"
        );
        layout->FreeLoadedTreeRoots(0);
    }

    layout->missionName.SetVisible(0);
    layout->messagesPanel.SetVisible(1);
    layout->SetEnabled(1);

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
    return this;
}

// Reimplements 0x403ed0: HudUiBriefingRuntime::Destructor (D:\Proj\Battlesport\Briefing.cpp)
void HudUiBriefingRuntime::Destructor() {
    HudUiBriefingRuntime *const layout = BriefingLayout(this);

    layout->SetEnabled(0);
    layout->messagesPanel.entryVector.Clear();

    ((HudUiPanel *)(&layout->messagesPanel))->Destructor();
    BriefingPanel(&layout->transmissionHalted)->Destructor();
    layout->objectivePicture.DestructorCore();
    BriefingPanel(&layout->objectiveDesc)->Destructor();
    BriefingPanel(&layout->objectiveSummary)->Destructor();
    BriefingPanel(&layout->missionName)->Destructor();
    layout->transportProgress.DestructorCore();

    BriefingActionNode *const head = layout->actionQueue.headSentinel;
    BriefingActionNode *node = head->prev;
    while (node != head) {
        BriefingActionNode *const prev = node->prev;
        node->next->prev = node->prev;
        node->prev->next = node->next;
        ::operator delete(node);
        --layout->actionQueue.nodeCount;
        node = prev;
    }
    ::operator delete(head);

    layout->actionQueue.headSentinel = 0;
    layout->actionQueue.nodeCount = 0;
    layout->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x4038a0: HudUiBriefingObjectivePicture::DrawWithNoiseOverlay
void HudUiBriefingObjectivePicture::DrawWithNoiseOverlay() {
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

void HudUiBriefingObjectivePicture::Draw() {
    DrawWithNoiseOverlay();
}

// Reimplements 0x403c10: HudUiBriefingLocatorPanel::Constructor
HudUiBriefingLocatorPanel * HudUiBriefingLocatorPanel::Constructor() {
    const unsigned short color = (unsigned short)(zVid_PackColorRGB(
        0xff,
        0,
        0
    ));
    HudUiCircle::Constructor(
        0x64,
        0x6e,
        0x1e,
        color
    );
    SetVisible(0);
    return this;
}

// Reimplements 0x403c90: HudUiBriefingLocatorPanel::BlitDirtyRect
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

void HudUiBriefingLocatorPanel::DrawBase() {
    BlitDirtyRect();
}

// Reimplements 0x403cb0: HudUiBriefingLocatorPanel::Update
void HudUiBriefingLocatorPanel::Update(
    float deltaSec
) {
    if ((flags & 0x10) == 0) {
        return;
    }

    clipRect.left = GetX() - radius;
    clipRect.top = GetY() - radius;
    clipRect.right = GetX() + radius + 1;
    clipRect.bottom = GetY() + radius + 1;

    if (radius > 3) {
        float radiusStep = deltaSec * 20.0f;
        if (radiusStep < 1.0f) {
            radiusStep = 1.0f;
        }

        const int newRadius = radius - (int)(radiusStep);
        radius = newRadius;
        radiusSquared = newRadius * newRadius;
        if (radius < 3) {
            radius = 3;
            radiusSquared = 9;
        }
    }

    HudUiElement::Update(deltaSec);
    Invalidate();
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x4045b0, 0x404640, 0x4046d0, 0x404780,
 * 0x4048a0, 0x4049d0, and 0x404b40.
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

// Reimplements 0x404620: BriefingAction_HideElement::Tick
int BriefingActionHideElement::Tick(
    float
) {
    target->SetVisible(0);
    return 1;
}

// Reimplements 0x4046b0: BriefingAction_ShowElement::Tick
int BriefingActionShowElement::Tick(
    float
) {
    target->SetVisible(1);
    target->Invalidate();
    return 1;
}

// Reimplements 0x404740: BriefingAction_FadeInElement::Tick
int BriefingActionFadeInElement::Tick(
    float
) {
    alpha += 0.5f;

    HudUiBriefingObjectivePicture *const widget = (HudUiBriefingObjectivePicture *)(target);
    widget->noiseAlpha = alpha;
    widget->Invalidate();

    return alpha >= 1.0f ? 1 : 0;
}

// Reimplements 0x404850: BriefingAction_SetPanelText::Tick
int BriefingActionSetPanelText::Tick(
    float
) {
    target->SetTextFmt(text);
    target->UpdateTextBoundsFromContent();
    target->SetVisible(1);
    target->Invalidate();
    return 1;
}

// Reimplements 0x404960: BriefingAction_SetWidgetImageTimed::Tick
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

// Reimplements 0x404aa0: BriefingAction_PlaySample::Tick
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

// Reimplements 0x404bb0: BriefingAction_DelayUntilProgress::Tick
int BriefingActionDelayUntilProgress::Tick(
    float
) {
    return (float)(g_Briefing_ProgressEventCode) >= requiredProgress ? 1 : 0;
}

// Reimplements 0x4045b0: Briefing_ActionQueue::AddHideElement
int Briefing_ActionQueue::AddHideElement(
    HudUiElement *element
) {
    BriefingActionHideElement *const action = new BriefingActionHideElement;
    if (action != 0) {
        action->target = element;
    }

    return InsertAction(action);
}

// Reimplements 0x404640: Briefing_ActionQueue::AddShowElement
int Briefing_ActionQueue::AddShowElement(
    HudUiElement *element
) {
    BriefingActionShowElement *const action = new BriefingActionShowElement;
    if (action != 0) {
        action->target = element;
    }

    return InsertAction(action);
}

// Reimplements 0x4046d0: Briefing_ActionQueue::AddFadeInElement
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

// Reimplements 0x404780: Briefing_ActionQueue::AddSetPanelText
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

// Reimplements 0x4048a0: Briefing_ActionQueue::AddSetWidgetImageTimed
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

// Reimplements 0x4049d0: Briefing_ActionQueue::AddPlaySampleByName
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

// Reimplements 0x404b40: Briefing_ActionQueue::AddDelayUntilProgress
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
// Reimplements 0x404b30: Briefing::SampleEventCallback
void __fastcall SampleEventCallback(
    int progressEventCode
) {
    g_Briefing_ProgressEventCode = progressEventCode;
}

// Reimplements 0x404c80: Briefing::BuildObjectiveActionsGlobal
void __fastcall BuildObjectiveActionsGlobal(
    int objectiveIndex
) {
    if (g_Briefing_Runtime != 0) {
        g_Briefing_Runtime->BuildObjectiveActionsFromIndex(objectiveIndex);
    }
}

// Reimplements 0x404180: Briefing::StartForMission (D:\Proj\Battlesport\Briefing.cpp)
int __fastcall StartForMission(
    int missionId
) {
    g_Briefing_SystemActiveFlag = 1;

    HudUiBriefingRuntime *runtime =
        (HudUiBriefingRuntime *)(::operator new(sizeof(HudUiBriefingRuntime)));
    if (runtime != 0) {
        runtime = runtime->Constructor(missionId);
    }

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

// Reimplements 0x404280: Briefing::ThreadMain (D:\Proj\Battlesport\Briefing.cpp)
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
            if (g_Briefing_CurrentSndHandle != 0) {
                g_Briefing_CurrentSndHandle->StopIfActive();
            }

            HudUiBriefingRuntime *const runtime = g_Briefing_Runtime;
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

// Reimplements 0x404bd0: Briefing::StopAndShutdownThread (D:\Proj\Battlesport\Briefing.cpp)
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

    const int threadExited = g_Briefing_ThreadExitedFlag;
    g_Briefing_ThreadRunFlag = 0;
    if (threadExited == 0) {
        do {
            Sleep(100);
        } while (g_Briefing_ThreadExitedFlag == 0);
    }

    HudUiBriefingRuntime *const runtime = g_Briefing_Runtime;
    if (runtime != 0) {
        runtime->Destructor();
        ::operator delete(runtime);
        g_Briefing_Runtime = 0;
    }

    g_Briefing_SystemActiveFlag = 0;
}

// Reimplements 0x404c50: Briefing::SetProgressAndSleep
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

// Reimplements 0x404070: HudUiBriefingRuntime::Update (D:\Proj\Battlesport\Briefing.cpp)
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
    BriefingLayout(this)->HudUiBackground::Update(deltaSec);
}

// Reimplements 0x404400: Briefing::BuildObjectiveActionsFromIndex
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
