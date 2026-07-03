#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "recoil/recoil_types.h"
#include <stddef.h>

#include "recoil/recoil_callconv.h"

/**
 * Briefing action base. BN action records at 0x404620 through 0x404bb0 share
 * the offset-zero virtual Tick dispatch used by HudUiBriefingRuntime::Update.
 */
struct BriefingAction {
    virtual int Tick(float deltaSec) = 0;
};

/**
 * Briefing action queue node. BN queue helpers allocate 0x0c-byte nodes and
 * thread them through the runtime-owned circular action list.
 */
struct BriefingActionNode {
    BriefingActionNode *next;
    BriefingActionNode *prev;
    BriefingAction *action;
};

/**
 * Briefing-owned circular action queue embedded in HudUiBriefingRuntime. BN
 * field references in the runtime/action helpers prove the member offsets and
 * the action insertion/tick ownership.
 */
struct Briefing_ActionQueue {
    unsigned char missionId;
    char missionIdPadding[3];
    BriefingActionNode *headSentinel;
    int nodeCount;
    BriefingActionNode *currentNode;
    int sequenceActive;

    Briefing_ActionQueue(int missionId);
    ~Briefing_ActionQueue();
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

/**
 * Briefing objective picture widget. BN names the slot target
 * DrawWithNoiseOverlay; the source model is the HudUiWidget Draw override with
 * a briefing-only noiseAlpha member.
 */
struct HudUiBriefingObjectivePicture : HudUiWidget {
    float noiseAlpha;

    HudUiBriefingObjectivePicture();
    void Draw();

    /**
     * Original inline member helper; no standalone retail function exists.
     * Observed in caller 0x404960, where VC5 emits an x87 load/store for the
     * timer-to-noiseAlpha update before the typed virtual Invalidate call.
     * Purpose: update the briefing picture noise fade value.
     */
    void SetNoiseAlpha(float alphaValue) {
        noiseAlpha = alphaValue;
    }
};
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingObjectivePicture, noiseAlpha) == 0xbc);

/**
 * Briefing transport progress widget. BN constructor evidence keeps this as a
 * fill-bitmap-derived member owned by HudUiBriefingRuntime.
 */
struct HudUiBriefingTransportProgress : HudUiFillBitmap {
    HudUiBriefingTransportProgress();
};

/**
 * Briefing runtime owner. BN constructor/destructor and action callers prove
 * this HudUiBackground-derived member layout and embedded action queue.
 */
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

    HudUiBriefingRuntime(int missionId);
    HudUiBriefingRuntime * Constructor(int missionId);
    void Destructor();
    HudUiBackground * ScalarDeletingDestructor(unsigned int flags);
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
