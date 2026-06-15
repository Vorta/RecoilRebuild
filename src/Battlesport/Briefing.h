#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "recoil/recoil_types.h"
#include <stddef.h>

#include "recoil/recoil_callconv.h"

struct BriefingAction {
    virtual int Tick(float deltaSec) = 0;
};

struct BriefingActionNode {
    BriefingActionNode *prev;
    BriefingActionNode *next;
    BriefingAction *action;
};

// Briefing-owned circular action queue embedded at HudUiBriefingRuntime+0xa94c.
struct Briefing_ActionQueue {
    int missionId;
    BriefingActionNode *headSentinel;
    int nodeCount;
    BriefingActionNode *currentNode;
    int sequenceActive;

    int InsertAction(BriefingAction *action);
    int AddHideElement(HudUiElement *element);
    int AddShowElement(HudUiElement *element);
    int AddFadeInElement(HudUiElement *element);
    int AddSetPanelText(
        const char *text,
        HudUiPanel *panel
    );
    int AddSetWidgetImageTimed(
        zVidImagePartial *imageRef,
        HudUiWidget *widget
    );
    int AddPlaySampleByName(
        const char *sampleName,
        float gain,
        int useVariant,
        int progressId
    );
    int AddDelayUntilProgress(int progressId);
};
RECOIL_STATIC_ASSERT(sizeof(BriefingActionNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, headSentinel) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, nodeCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, currentNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, sequenceActive) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(Briefing_ActionQueue) == 0x14);

/**
 * Briefing locator child panel; BN constructor 0x403c10 and the runtime array
 * constructor prove six 0x40-byte HudUiCircle-derived elements. The retail
 * locator dispatch table is data/tier-S evidence debt, not a production source
 * scaffold.
 */
struct HudUiBriefingLocatorPanel : HudUiCircle {
    HudUiBriefingLocatorPanel();
    void DrawBase();
    void BlitDirtyRect();
    void Update(float deltaSec);
};

// Briefing objective picture widget; derived dispatch overrides Draw with the noise-overlay pass.
struct HudUiBriefingObjectivePicture : HudUiWidget {
    float noiseAlpha;

    void Draw();
};
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingObjectivePicture, noiseAlpha) == 0xbc);

// Briefing transport progress widget; constructor installs the briefing-specific fill-bitmap vtable.
struct HudUiBriefingTransportProgress : HudUiFillBitmap {
};

// Briefing runtime owner; BN constructor/destructor prove this HudUiBackground-derived member layout.
struct HudUiBriefingRuntime : HudUiBackground {
    Briefing_ActionQueue actionQueue;
    HudUiBriefingTransportProgress transportProgress;
    HudUiPanel missionName;
    HudUiPanel objectiveSummary;
    HudUiPanel objectiveDesc;
    HudUiBriefingObjectivePicture objectivePicture;
    HudUiPanel transmissionHalted;
    HudUiCompositePanel messagesPanel;
    HudUiBriefingLocatorPanel locatorPanels[6];

    HudUiBriefingRuntime * Constructor(int missionId);
    void Destructor();
    int BuildObjectiveActionsFromIndex(int objectiveIndex);
    void Update(float deltaSec);
};
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, actionQueue) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, transportProgress) == 0xa960);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, missionName) == 0xaae8);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, objectiveSummary) == 0xad8c);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, objectiveDesc) == 0xb030);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, objectivePicture) == 0xb2d4);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, transmissionHalted) == 0xb394);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, messagesPanel) == 0xb638);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, locatorPanels) == 0xb8f0);
RECOIL_STATIC_ASSERT(sizeof(HudUiBriefingRuntime) == 0xba70);

namespace Briefing {
void __fastcall BuildObjectiveActionsGlobal(int objectiveIndex);
void __fastcall SampleEventCallback(int progressEventCode);
int __fastcall StartForMission(int missionId);
void ThreadMain(void *threadParameter);
void __fastcall StopAndShutdownThread(int waitForInput);
void __stdcall SetProgressAndSleep(float progressValue);
} // namespace Briefing

extern HudUiBriefingRuntime *g_Briefing_Runtime;

extern "C" {
extern int g_Briefing_ThreadRunFlag;
extern int g_Briefing_ThreadExitedFlag;
extern int g_Briefing_SequenceActiveFlag;
extern int g_Briefing_AllowAdvanceFlag;
extern int g_Briefing_SystemActiveFlag;
extern int g_Briefing_ProgressEventCode;
}
