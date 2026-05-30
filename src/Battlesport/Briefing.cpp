#include "Battlesport/Briefing.h"

#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/Time/Time.h"

#include <windows.h>

#include <stddef.h>
#include "recoil/recoil_types.h"
#include <stdio.h>
#include <string.h>
#include <process.h>

namespace {
typedef void (RECOIL_THISCALL *BriefingSetNormalizedValue)(void *self, float value);

const size_t kBriefingActionQueueOffset = 0xa94c;
const size_t kBriefingMissionNameOffset = 0xaae8;
const size_t kBriefingObjectiveSummaryOffset = 0xad8c;
const size_t kBriefingObjectiveDescOffset = 0xb030;
const size_t kBriefingObjectivePictureOffset = 0xb2d4;
const size_t kBriefingTransmissionHaltedOffset = 0xb394;
const size_t kBriefingLocatorPanelsOffset = 0xb8f0;
const size_t kBriefingLocatorPanelStride = 0x40;
const size_t kBriefingObjectivePictureNoiseAlphaOffset = 0xbc;

typedef void (RECOIL_THISCALL *HudVirtualNoArg)(void *self);
typedef void (RECOIL_THISCALL *HudVirtualSetVisible)(void *self, int visible);
typedef void (RECOIL_CDECL *HudPanelSetTextFmt)(void *self, const char *format, ...);

template <typename Method> unsigned int MethodAddress(Method method) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(&address, &method, sizeof(method));
    return address;
}

void RECOIL_FASTCALL BriefingHudUiCommonInvalidateThunk(HudUiElement *element) {
    element->Invalidate();
}

void RECOIL_FASTCALL BriefingHudUiNoOpMethodStub(void *) {}

template <typename FTable> FTable MakeBriefingHudUiFTableWithCommonSlots() {
    FTable table = {0};
    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 2) {
        table.slots[2] = (unsigned int)(&BriefingHudUiNoOpMethodStub);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 8) {
        table.slots[8] = (unsigned int)(&BriefingHudUiCommonInvalidateThunk);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 3) {
        table.slots[3] = MethodAddress(&HudUiElement::SetPos);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 4) {
        table.slots[4] = MethodAddress(&HudUiElement::SetX);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 5) {
        table.slots[5] = MethodAddress(&HudUiElement::SetY);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 6) {
        table.slots[6] = MethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 7) {
        table.slots[7] = MethodAddress(&HudUiElement::SetClipRect);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 24) {
        table.slots[24] = MethodAddress(&HudUiElement::SetVisible);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 25) {
        table.slots[25] = MethodAddress(&HudUiElement::GetX);
    }

    if ((sizeof(table.slots) / sizeof(table.slots[0])) > 26) {
        table.slots[26] = MethodAddress(&HudUiElement::GetY);
    }

    return table;
}

struct HudUiBriefingLocatorPanel {
    HudUiCircle base;

    RECOIL_NOINLINE HudUiBriefingLocatorPanel *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL BlitDirtyRect();
    RECOIL_NOINLINE void RECOIL_THISCALL Update(float deltaSec);
};

struct HudUiBriefingObjectivePicture {
    HudUiWidget base;
    float noiseAlpha;

    RECOIL_NOINLINE void RECOIL_THISCALL DrawWithNoiseOverlay();
};

RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingObjectivePicture, noiseAlpha) ==
              kBriefingObjectivePictureNoiseAlphaOffset);

HudUiCommon_FTable MakeBriefingLocatorPanelFTable() {
    HudUiCommon_FTable table = MakeBriefingHudUiFTableWithCommonSlots<HudUiCommon_FTable>();
    table.slots[1] = MethodAddress(&HudUiCircle::DrawDirtyForwarder);
    table.slots[2] = MethodAddress(&HudUiBriefingLocatorPanel::BlitDirtyRect);
    table.slots[9] = MethodAddress(&HudUiBriefingLocatorPanel::Update);
    return table;
}

HudUiWidget_FTable MakeBriefingObjectivePictureFTable() {
    HudUiWidget_FTable table = MakeBriefingHudUiFTableWithCommonSlots<HudUiWidget_FTable>();
    table.slots[1] = MethodAddress(&HudUiBriefingObjectivePicture::DrawWithNoiseOverlay);
    table.slots[25] = MethodAddress(&HudUiWidget::GetCenterX);
    table.slots[26] = MethodAddress(&HudUiWidget::GetCenterY);
    return table;
}

HudUiFillBitmap_FTable MakeBriefingTransportProgressFTable() {
    HudUiFillBitmap_FTable table =
        MakeBriefingHudUiFTableWithCommonSlots<HudUiFillBitmap_FTable>();
    table.slots[0x84 / 4] = MethodAddress(&HudUiFillBitmap::SetNormalizedValueAndRebuild);
    return table;
}

HudUiBriefingRuntimeVtable MakeBriefingRuntimeFTable() {
    HudUiBriefingRuntimeVtable table = {0};
    table.Update =
        (HudUiBriefingRuntimeUpdate)(
            MethodAddress(&HudUiBriefingRuntime::Update));
    table.SetEnabled = (HudUiBriefingRuntimeSetEnabled)(
        MethodAddress(&HudUiBackground::SetEnabled));
    table.ScalarDeletingDtor =
        (HudUiBriefingRuntimeScalarDeletingDestructor)(
            MethodAddress(&HudUiBriefingRuntime::ScalarDeletingDestructor));
    return table;
}

const HudUiCommon_FTable g_HudUiBriefingLocatorPanel_FTable = MakeBriefingLocatorPanelFTable();
const HudUiWidget_FTable g_HudUiBriefingObjectivePicture_FTable =
    MakeBriefingObjectivePictureFTable();
const HudUiFillBitmap_FTable g_HudUiBriefingTransportProgress_FTable =
    MakeBriefingTransportProgressFTable();
const HudUiBriefingRuntimeVtable g_HudUiBriefingRuntime_FTable = MakeBriefingRuntimeFTable();

struct BriefingAction {
    virtual int RECOIL_THISCALL Tick(float deltaSec) = 0;
};

struct BriefingActionNode {
    BriefingActionNode *prev;
    BriefingActionNode *next;
    BriefingAction *action;
};

struct Briefing_ActionQueue {
    int missionId;
    BriefingActionNode *headSentinel;
    int nodeCount;
    BriefingActionNode *currentNode;
    int sequenceActive;

    int InsertAction(BriefingAction *action);
    RECOIL_NOINLINE int RECOIL_THISCALL AddHideElement(void *element);
    RECOIL_NOINLINE int RECOIL_THISCALL AddShowElement(void *element);
    RECOIL_NOINLINE int RECOIL_THISCALL AddFadeInElement(void *element);
    RECOIL_NOINLINE int RECOIL_THISCALL AddSetPanelText(const char *text, void *panel);
    RECOIL_NOINLINE int RECOIL_THISCALL
    AddSetWidgetImageTimed(zVidImagePartial *imageRef, HudUiWidget *widget);
    RECOIL_NOINLINE int RECOIL_THISCALL
    AddPlaySampleByName(const char *sampleName, float gain, int useVariant,
                        int progressId);
    RECOIL_NOINLINE int RECOIL_THISCALL AddDelayUntilProgress(int progressId);
};

RECOIL_STATIC_ASSERT(sizeof(BriefingActionNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, headSentinel) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, nodeCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, currentNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, sequenceActive) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(Briefing_ActionQueue) == 0x14);

struct HudUiBriefingPanelStorage {
    unsigned char storage[0x2a4];
};

struct HudUiBriefingRuntimeLayout {
    HudUiBackground base;
    Briefing_ActionQueue actionQueue;
    HudUiFillBitmap transportProgress;
    HudUiBriefingPanelStorage missionName;
    HudUiBriefingPanelStorage objectiveSummary;
    HudUiBriefingPanelStorage objectiveDesc;
    HudUiBriefingObjectivePicture objectivePicture;
    HudUiBriefingPanelStorage transmissionHalted;
    HudUiCompositePanel messagesPanel;
    HudUiBriefingLocatorPanel locatorPanels[6];
};

RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, actionQueue) == kBriefingActionQueueOffset);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, transportProgress) == 0xa960);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, missionName) == kBriefingMissionNameOffset);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, objectiveSummary) ==
              kBriefingObjectiveSummaryOffset);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, objectiveDesc) == kBriefingObjectiveDescOffset);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, objectivePicture) ==
              kBriefingObjectivePictureOffset);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, transmissionHalted) ==
              kBriefingTransmissionHaltedOffset);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, messagesPanel) == 0xb638);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeLayout, locatorPanels) == kBriefingLocatorPanelsOffset);
RECOIL_STATIC_ASSERT(sizeof(HudUiBriefingRuntimeLayout) == 0xba70);

struct BriefingActionElementTarget : BriefingAction {
    void *target;
};

struct BriefingActionHideElement : BriefingActionElementTarget {
    int RECOIL_THISCALL Tick(float deltaSec);
};

struct BriefingActionShowElement : BriefingActionElementTarget {
    int RECOIL_THISCALL Tick(float deltaSec);
};

struct BriefingActionFadeInElement : BriefingActionElementTarget {
    float alpha;

    int RECOIL_THISCALL Tick(float deltaSec);
};

struct BriefingActionSetPanelText : BriefingAction {
    char text[0x100];
    void *target;

    int RECOIL_THISCALL Tick(float deltaSec);
};

struct BriefingActionSetWidgetImageTimed : BriefingAction {
    zVidImagePartial *imageRef;
    HudUiWidget *target;
    float timer;

    int RECOIL_THISCALL Tick(float deltaSec);
};

struct BriefingActionPlaySample : BriefingAction {
    char sampleName[0x50];
    float gain;
    int useVariant;
    int variantIndex;

    int RECOIL_THISCALL Tick(float deltaSec);
};

struct BriefingActionDelayUntilProgress : BriefingAction {
    float requiredProgress;

    int RECOIL_THISCALL Tick(float deltaSec);
};

RECOIL_STATIC_ASSERT(offsetof(BriefingActionElementTarget, target) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionElementTarget) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(BriefingActionFadeInElement, alpha) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionFadeInElement) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(BriefingActionSetPanelText, target) == 0x104);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionSetPanelText) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(BriefingActionSetWidgetImageTimed, timer) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionSetWidgetImageTimed) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(BriefingActionPlaySample, gain) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(BriefingActionPlaySample, useVariant) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(BriefingActionPlaySample, variantIndex) == 0x5c);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionPlaySample) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(BriefingActionDelayUntilProgress, requiredProgress) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(BriefingActionDelayUntilProgress) == 0x08);

template <typename T> T *BriefingField(HudUiBriefingRuntime *runtime, size_t offset) {
    return (T *)((unsigned char *)(runtime) + offset);
}

HudUiBriefingRuntimeLayout *BriefingLayout(HudUiBriefingRuntime *runtime) {
    return (HudUiBriefingRuntimeLayout *)(runtime);
}

HudUiPanel *BriefingPanel(HudUiBriefingPanelStorage *panelStorage) {
    return (HudUiPanel *)(panelStorage);
}

Briefing_ActionQueue *BriefingActionQueue(HudUiBriefingRuntime *runtime) {
    return BriefingField<Briefing_ActionQueue>(runtime, kBriefingActionQueueOffset);
}

void *BriefingMissionNamePanel(HudUiBriefingRuntime *runtime) {
    return BriefingField<void>(runtime, kBriefingMissionNameOffset);
}

void *BriefingObjectiveSummaryPanel(HudUiBriefingRuntime *runtime) {
    return BriefingField<void>(runtime, kBriefingObjectiveSummaryOffset);
}

void *BriefingObjectiveDescPanel(HudUiBriefingRuntime *runtime) {
    return BriefingField<void>(runtime, kBriefingObjectiveDescOffset);
}

HudUiWidget *BriefingObjectivePicture(HudUiBriefingRuntime *runtime) {
    return BriefingField<HudUiWidget>(runtime, kBriefingObjectivePictureOffset);
}

void *BriefingTransmissionHaltedPanel(HudUiBriefingRuntime *runtime) {
    return BriefingField<void>(runtime, kBriefingTransmissionHaltedOffset);
}

void *BriefingLocatorPanel(HudUiBriefingRuntime *runtime, int objectiveIndex) {
    return BriefingField<void>(runtime,
                               kBriefingLocatorPanelsOffset +
                                   (size_t)(objectiveIndex) *
                                       kBriefingLocatorPanelStride);
}

const unsigned int *HudVtable(void *element) {
    return *(const unsigned int *const *)(element);
}

void HudCallNoArg(void *element, size_t vtableOffset) {
    HudVirtualNoArg const method = (HudVirtualNoArg)(HudVtable(element)[vtableOffset / 4]);
    method(element);
}

void HudCallSetVisible(void *element, int visible) {
    HudVirtualSetVisible const method = (HudVirtualSetVisible)(HudVtable(element)[0x60 / 4]);
    method(element, visible);
}

void HudCallPanelSetText(void *panel, const char *text) {
    HudPanelSetTextFmt const method = (HudPanelSetTextFmt)(HudVtable(panel)[0x74 / 4]);
    method(panel, text);
}

void BriefingObjectivePictureSetNoiseAlpha(HudUiWidget *widget, float alpha) {
    *(float *)((unsigned char *)(widget) +
                               kBriefingObjectivePictureNoiseAlphaOffset) = alpha;
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
RECOIL_NOINLINE HudUiBriefingRuntime *RECOIL_THISCALL
HudUiBriefingRuntime::Constructor(int missionId) {
    HudUiBriefingRuntimeLayout *const layout = BriefingLayout(this);
    layout->base.Constructor();

    layout->actionQueue.missionId = missionId & 0xff;
    BriefingActionNode *const sentinel = new BriefingActionNode;
    sentinel->prev = sentinel;
    sentinel->next = sentinel;
    layout->actionQueue.headSentinel = sentinel;
    layout->actionQueue.nodeCount = 0;
    layout->actionQueue.sequenceActive = 0;
    g_Briefing_ProgressEventCode = -1;

    layout->transportProgress.Constructor();
    layout->transportProgress.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiBriefingTransportProgress_FTable);

    BriefingPanel(&layout->missionName)->ConstructorDefault(0, 0, 0);
    BriefingPanel(&layout->objectiveSummary)->ConstructorDefault(0, 0, 0);
    BriefingPanel(&layout->objectiveDesc)->ConstructorDefault(0, 0, 0);

    layout->objectivePicture.base.Constructor(0);
    layout->objectivePicture.base.ftable = &g_HudUiBriefingObjectivePicture_FTable;
    layout->objectivePicture.noiseAlpha = 0.0f;
    ((HudUiElement *)(&layout->objectivePicture.base))->Invalidate();

    BriefingPanel(&layout->transmissionHalted)->ConstructorDefault(0, 0, 0);
    layout->messagesPanel.ConstructorWithEntryCount(0x19);
    {
    for (int index = 0; index < 6; ++index) {
        layout->locatorPanels[index].Constructor();
    }
    }

    layout->base.base.base.vptr =
        (const HudUiContainer_FTable *)(&g_HudUiBriefingRuntime_FTable);

    char campaignSection[0x20];
    sprintf(campaignSection, "CAMPAIGN%1d", missionId);
    zReader::Node *const loadedRoot = layout->base.LoadFromZrd("briefing.zrd", campaignSection, 0);
    if (loadedRoot != 0) {
        layout->base.BindWidgetByName(loadedRoot, (HudUiWidget *)(
                                                      &layout->transportProgress),
                                      "TRANSPORT_PROGRESS");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->missionName), "MISSION_NAME");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->objectiveSummary),
            "OBJECTIVE_SUMMARY");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->objectiveDesc),
            "OBJECTIVE_DESC");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->objectivePicture),
            "OBJECTIVE_PICT");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->messagesPanel), "MESSAGES");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->transmissionHalted),
            "TRANSMISSION_HALTED");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->locatorPanels[0]), "LOCATOR1");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->locatorPanels[1]), "LOCATOR2");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->locatorPanels[2]), "LOCATOR3");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->locatorPanels[3]), "LOCATOR4");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->locatorPanels[4]), "LOCATOR5");
        layout->base.BindPrimitiveNodeToElement(
            loadedRoot, (HudUiElement *)(&layout->locatorPanels[5]), "LOCATOR6");
        layout->base.FreeLoadedTreeRoots(0);
    }

    HudCallSetVisible(&layout->missionName, 0);
    HudCallSetVisible(&layout->messagesPanel, 1);
    layout->base.SetEnabled(1);

    Time::Tick();
    zSnd_Tick(1);
    zVideo::RunPostprocessOnPrimaryBuffer();
    Update(g_FrameDeltaTimeSec);
    zVideo::Dispatch_UnlockPrimarySurfaceState();
    zVideo::AdjustSurfacesIfEnabled(0, 0, 1, 1);
    return this;
}

// Reimplements 0x403d90: HudUiBriefingRuntime::ScalarDeletingDestructor
HudUiBriefingRuntime *RECOIL_THISCALL
HudUiBriefingRuntime::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x403ed0: HudUiBriefingRuntime::Destructor (D:\Proj\Battlesport\Briefing.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBriefingRuntime::Destructor() {
    HudUiBriefingRuntimeLayout *const layout = BriefingLayout(this);
    layout->base.base.base.vptr =
        (const HudUiContainer_FTable *)(&g_HudUiBriefingRuntime_FTable);

    layout->base.SetEnabled(0);
    for (int index = 5; index >= 0; --index) {
        layout->locatorPanels[index].base.base.ResetCommonFTable();
    }

    HudUiCompositePanelEntry *entry = layout->messagesPanel.entryVector.begin;
    while (entry != layout->messagesPanel.entryVector.end) {
        typedef HudUiCompositePanelEntry * (RECOIL_THISCALL *ScalarDeletingDestructorFn)(HudUiCompositePanelEntry * self,
                                                          unsigned int flags);
        const unsigned int *const ftable = *(const unsigned int *const *)(entry);
        ((ScalarDeletingDestructorFn)(ftable[0]))(entry, 0);
        ++entry;
    }
    ::operator delete(layout->messagesPanel.entryVector.begin);
    layout->messagesPanel.entryVector.begin = 0;
    layout->messagesPanel.entryVector.end = 0;
    layout->messagesPanel.entryVector.capacityEnd = 0;

    ((HudUiPanel *)(&layout->messagesPanel))->Destructor();
    BriefingPanel(&layout->transmissionHalted)->Destructor();
    layout->objectivePicture.base.DestructorCore();
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
    layout->base.Destructor();
}

// Reimplements 0x4038a0: HudUiBriefingObjectivePicture::DrawWithNoiseOverlay
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBriefingObjectivePicture::DrawWithNoiseOverlay() {
    base.Draw();
    if (noiseAlpha <= 0.0) {
        return;
    }

    zVidRect32 rect;
    rect.left = base.GetCenterX();
    rect.top = base.GetCenterY();
    const zVidImagePartial *const image = base.image;
    rect.right = rect.left + (image != 0 ? image->width : 0);
    rect.bottom = rect.top + (image != 0 ? image->height : 0);
    zVid::DrawNoiseRect(&rect, noiseAlpha);
}

// Reimplements 0x403c10: HudUiBriefingLocatorPanel::Constructor
RECOIL_NOINLINE HudUiBriefingLocatorPanel *RECOIL_THISCALL
HudUiBriefingLocatorPanel::Constructor() {
    const unsigned short color =
        (unsigned short)(zVid_PackColorRGB(0xff, 0, 0));
    base.Constructor(0x64, 0x6e, 0x1e, color);
    base.base.ftable = &g_HudUiBriefingLocatorPanel_FTable;
    base.base.SetVisible(0);
    return this;
}

// Reimplements 0x403c90: HudUiBriefingLocatorPanel::BlitDirtyRect
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBriefingLocatorPanel::BlitDirtyRect() {
    if (base.base.bltSource != 0) {
        zVid_Image::BlitToActiveTarget((zVidImagePartial *)(base.base.bltSource),
                                       base.base.clipRect.left, base.base.clipRect.top, 0,
                                       (zVidRect32 *)(&base.base.clipRect));
    }
}

// Reimplements 0x403cb0: HudUiBriefingLocatorPanel::Update
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBriefingLocatorPanel::Update(float deltaSec) {
    if ((base.base.flags & 0x10) == 0) {
        return;
    }

    base.base.clipRect.left = base.base.GetX() - base.radius;
    base.base.clipRect.top = base.base.GetY() - base.radius;
    base.base.clipRect.right = base.base.GetX() + base.radius + 1;
    base.base.clipRect.bottom = base.base.GetY() + base.radius + 1;

    if (base.radius > 3) {
        float radiusStep = deltaSec * 20.0f;
        if (radiusStep < 1.0f) {
            radiusStep = 1.0f;
        }

        const int newRadius = base.radius - (int)(radiusStep);
        base.radius = newRadius;
        base.radiusSquared = newRadius * newRadius;
        if (base.radius < 3) {
            base.radius = 3;
            base.radiusSquared = 9;
        }
    }

    base.base.Update(deltaSec);
    base.base.Invalidate();
}

int Briefing_ActionQueue::InsertAction(BriefingAction *action) {
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
int RECOIL_THISCALL BriefingActionHideElement::Tick(float) {
    HudCallSetVisible(target, 0);
    return 1;
}

// Reimplements 0x4046b0: BriefingAction_ShowElement::Tick
int RECOIL_THISCALL BriefingActionShowElement::Tick(float) {
    HudCallSetVisible(target, 1);
    HudCallNoArg(target, 0x20);
    return 1;
}

// Reimplements 0x404740: BriefingAction_FadeInElement::Tick
int RECOIL_THISCALL BriefingActionFadeInElement::Tick(float) {
    alpha += 0.5f;

    HudUiWidget *const widget = (HudUiWidget *)(target);
    BriefingObjectivePictureSetNoiseAlpha(widget, alpha);
    HudCallNoArg(widget, 0x20);

    return alpha >= 1.0f ? 1 : 0;
}

// Reimplements 0x404850: BriefingAction_SetPanelText::Tick
int RECOIL_THISCALL BriefingActionSetPanelText::Tick(float) {
    HudCallPanelSetText(target, text);
    HudCallNoArg(target, 0x78);
    HudCallSetVisible(target, 1);
    HudCallNoArg(target, 0x20);
    return 1;
}

// Reimplements 0x404960: BriefingAction_SetWidgetImageTimed::Tick
int RECOIL_THISCALL BriefingActionSetWidgetImageTimed::Tick(float) {
    HudCallNoArg(target, 0x08);
    target->SetImageBorrowedAndInvalidate(imageRef);
    HudCallNoArg(target, 0x74);
    HudCallSetVisible(target, 1);
    BriefingObjectivePictureSetNoiseAlpha(target, timer);
    HudCallNoArg(target, 0x20);

    timer -= 0.5f;
    return timer < 0.0f ? 1 : 0;
}

// Reimplements 0x404aa0: BriefingAction_PlaySample::Tick
int RECOIL_THISCALL BriefingActionPlaySample::Tick(float) {
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
    g_Briefing_CurrentSndHandle = sample->PlayDirectSound(variantIndex, gain, 0x3e7);
    return 1;
}

// Reimplements 0x404bb0: BriefingAction_DelayUntilProgress::Tick
int RECOIL_THISCALL BriefingActionDelayUntilProgress::Tick(float) {
    return (float)(g_Briefing_ProgressEventCode) >= requiredProgress ? 1 : 0;
}

// Reimplements 0x4045b0: Briefing_ActionQueue::AddHideElement
RECOIL_NOINLINE int RECOIL_THISCALL Briefing_ActionQueue::AddHideElement(void *element) {
    BriefingActionHideElement *const action = new BriefingActionHideElement;
    if (action != 0) {
        action->target = element;
    }

    return InsertAction(action);
}

// Reimplements 0x404640: Briefing_ActionQueue::AddShowElement
RECOIL_NOINLINE int RECOIL_THISCALL Briefing_ActionQueue::AddShowElement(void *element) {
    BriefingActionShowElement *const action = new BriefingActionShowElement;
    if (action != 0) {
        action->target = element;
    }

    return InsertAction(action);
}

// Reimplements 0x4046d0: Briefing_ActionQueue::AddFadeInElement
RECOIL_NOINLINE int RECOIL_THISCALL Briefing_ActionQueue::AddFadeInElement(
    void *element) {
    BriefingActionFadeInElement *const action = new BriefingActionFadeInElement;
    if (action != 0) {
        action->target = element;
        action->alpha = 0.0f;
    }

    return InsertAction(action);
}

// Reimplements 0x404780: Briefing_ActionQueue::AddSetPanelText
RECOIL_NOINLINE int RECOIL_THISCALL
Briefing_ActionQueue::AddSetPanelText(const char *text, void *panel) {
    BriefingActionSetPanelText *const action = new BriefingActionSetPanelText;
    if (action != 0) {
        strncpy(action->text, text, sizeof(action->text));
        action->target = panel;
        HudCallSetVisible(panel, 0);
    }

    return InsertAction(action);
}

// Reimplements 0x4048a0: Briefing_ActionQueue::AddSetWidgetImageTimed
RECOIL_NOINLINE int RECOIL_THISCALL
Briefing_ActionQueue::AddSetWidgetImageTimed(zVidImagePartial *imageRef, HudUiWidget *widget) {
    BriefingActionSetWidgetImageTimed *const action =
        new BriefingActionSetWidgetImageTimed;
    if (action != 0) {
        action->imageRef = imageRef;
        action->target = widget;
        HudCallSetVisible(widget, 0);
        action->timer = 1.0f;
    }

    return InsertAction(action);
}

// Reimplements 0x4049d0: Briefing_ActionQueue::AddPlaySampleByName
RECOIL_NOINLINE int RECOIL_THISCALL
Briefing_ActionQueue::AddPlaySampleByName(const char *sampleName, float gain,
                                          int useVariant, int progressId) {
    BriefingActionPlaySample *const action = new BriefingActionPlaySample;
    if (action != 0) {
        strncpy(action->sampleName, sampleName, sizeof(action->sampleName));
        action->gain = gain;
        action->useVariant = useVariant;
        action->variantIndex = progressId;
    }

    return InsertAction(action);
}

// Reimplements 0x404b40: Briefing_ActionQueue::AddDelayUntilProgress
RECOIL_NOINLINE int RECOIL_THISCALL
Briefing_ActionQueue::AddDelayUntilProgress(int progressId) {
    BriefingActionDelayUntilProgress *const action =
        new BriefingActionDelayUntilProgress;
    if (action != 0) {
        action->requiredProgress = (float)(progressId);
    }

    return InsertAction(action);
}

namespace Briefing {
// Reimplements 0x404b30: Briefing::SampleEventCallback
RECOIL_NOINLINE void RECOIL_FASTCALL SampleEventCallback(int progressEventCode) {
    g_Briefing_ProgressEventCode = progressEventCode;
}

// Reimplements 0x404c80: Briefing::BuildObjectiveActionsGlobal
RECOIL_NOINLINE void RECOIL_FASTCALL BuildObjectiveActionsGlobal(int objectiveIndex) {
    if (g_Briefing_Runtime != 0) {
        g_Briefing_Runtime->BuildObjectiveActionsFromIndex(objectiveIndex);
    }
}

// Reimplements 0x404180: Briefing::StartForMission (D:\Proj\Battlesport\Briefing.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL StartForMission(int missionId) {
    g_Briefing_SystemActiveFlag = 1;

    HudUiBriefingRuntime *runtime =
        (HudUiBriefingRuntime *)(::operator new(sizeof(HudUiBriefingRuntimeLayout)));
    if (runtime != 0) {
        runtime = runtime->Constructor(missionId);
    }

    g_Briefing_Runtime = runtime;
    sprintf(g_Briefing_SndSetName, "BRIEFING%d", missionId);
    zSndSampleSet_InitByName(g_Briefing_SndSetName);

    g_Briefing_ThreadExitedFlag = 0;
    g_Briefing_ThreadRunFlag = 0;
    if (_beginthread(Briefing::ThreadMain, 0, 0) == (unsigned long)-1L) {
        zError::ReportOld(0x200, "D:\\Proj\\Battlesport\\Briefing.cpp", 0x202,
                          "Failed to create Briefing thread (%s)", strerror(0));
    }

    while (g_Briefing_ThreadRunFlag == 0) {
        Sleep(100);
    }

    return 1;
}

// Reimplements 0x404280: Briefing::ThreadMain (D:\Proj\Battlesport\Briefing.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ThreadMain(void *) {
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

            HudCallSetVisible(BriefingMissionNamePanel(runtime), 0);
            HudCallSetVisible(BriefingObjectiveSummaryPanel(runtime), 0);
            HudCallSetVisible(BriefingObjectiveDescPanel(runtime), 0);

            HudUiWidget *const objectivePicture = BriefingObjectivePicture(runtime);
            BriefingObjectivePictureSetNoiseAlpha(objectivePicture, 1.0f);
            HudCallNoArg(objectivePicture, 0x20);

            void *const transmissionHalted = BriefingTransmissionHaltedPanel(runtime);
            HudCallPanelSetText(transmissionHalted, zLoc::GetMessageString(0x110));
            HudCallSetVisible(transmissionHalted, 1);
        }

        Time::Tick();
        if (g_Briefing_Runtime != 0) {
            zSnd_Tick(1);
            zVideo::RunPostprocessOnPrimaryBuffer();
            ((HudUiContainer *)(g_Briefing_Runtime))->UpdateAll(g_FrameDeltaTimeSec);
            zVideo::Dispatch_UnlockPrimarySurfaceState();
        }

        zVideo::AdjustSurfacesIfEnabled(0, 0, 1, 1);
    }

    zSndSampleSet_DestroyByName(g_Briefing_SndSetName);
    zVideo::SetHalfResAdjustMode(previousHalfResMode);
    HudUi::SetInvalidateMode(previousHalfResMode);
    g_Briefing_ThreadExitedFlag = 1;
}

// Reimplements 0x404bd0: Briefing::StopAndShutdownThread (D:\Proj\Battlesport\Briefing.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL StopAndShutdownThread(int waitForInput) {
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
        runtime->vptr->ScalarDeletingDtor(runtime, 1);
        g_Briefing_Runtime = 0;
    }

    g_Briefing_SystemActiveFlag = 0;
}

// Reimplements 0x404c50: Briefing::SetProgressAndSleep
RECOIL_NOINLINE void RECOIL_STDCALL SetProgressAndSleep(float progressValue) {
    if (g_Briefing_Runtime != 0) {
        HudUiBriefingTransportProgress *const transportProgress =
            &g_Briefing_Runtime->transportProgress;
        const unsigned int *const ftable = (const unsigned int *)transportProgress->vptr;
        BriefingSetNormalizedValue const setNormalizedValue = (BriefingSetNormalizedValue)(ftable[0x84 / 4]);
        setNormalizedValue(transportProgress, progressValue);
    }

    Sleep(100);
}
} // namespace Briefing

// Reimplements 0x404070: HudUiBriefingRuntime::Update (D:\Proj\Battlesport\Briefing.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBriefingRuntime::Update(float deltaSec) {
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

    HudCallNoArg(BriefingObjectivePicture(this), 0x20);
    HudCallNoArg(BriefingTransmissionHaltedPanel(this), 0x20);
    HudCallNoArg(BriefingMissionNamePanel(this), 0x20);
    HudCallNoArg(&transportProgress, 0x20);
    HudCallNoArg(BriefingObjectiveSummaryPanel(this), 0x20);
    HudCallNoArg(BriefingObjectiveDescPanel(this), 0x20);
    ((HudUiBackground *)(this))->Update(deltaSec);
}

// Reimplements 0x404400: Briefing::BuildObjectiveActionsFromIndex
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiBriefingRuntime::BuildObjectiveActionsFromIndex(int objectiveIndex) {
    if (zOpt::GetNetworkEnabled() != 0) {
        return 0;
    }

    Briefing_ActionQueue *const actionQueue = BriefingActionQueue(this);
    const int firstProgressId = objectiveIndex * 2;

    char sampleName[0x50];
    sprintf(sampleName, "snd_briefing_c%d", g_HudSensorTracker.GetMissionId());
    actionQueue->AddPlaySampleByName(sampleName, 1.0f, 1, firstProgressId);

    int progressId = firstProgressId;
    {
    for (int index = objectiveIndex; index < g_HudSensorTracker.objectiveCount; ++index) {
        char *objectiveSummaryText = 0;
        char *objectiveDescText = 0;
        zVidImagePartial *objectiveImage = 0;
        g_HudSensorTracker.GetObjectiveBriefingStringsAndImageRef(
            index, &objectiveSummaryText, &objectiveDescText, &objectiveImage);

        char objectiveTitle[0x20];
        zLoc::FormatMessage(objectiveTitle, sizeof(objectiveTitle), 0x244, index + 1);

        void *const missionNamePanel = BriefingMissionNamePanel(this);
        void *const objectiveSummaryPanel = BriefingObjectiveSummaryPanel(this);
        void *const objectiveDescPanel = BriefingObjectiveDescPanel(this);
        HudUiWidget *const objectivePicture = BriefingObjectivePicture(this);
        void *const locatorPanel = BriefingLocatorPanel(this, index);

        actionQueue->AddDelayUntilProgress(progressId);
        actionQueue->AddSetPanelText(objectiveTitle, missionNamePanel);
        actionQueue->AddSetPanelText(objectiveSummaryText, objectiveSummaryPanel);
        actionQueue->AddSetWidgetImageTimed(objectiveImage, objectivePicture);
        actionQueue->AddShowElement(locatorPanel);
        actionQueue->AddSetPanelText(objectiveDescText, objectiveDescPanel);
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
