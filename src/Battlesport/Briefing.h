#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"
#include <list>
#include <stddef.h>

/**
 * Briefing action base. BN action records at 0x404620 through 0x404bb0 share
 * the offset-zero virtual Tick dispatch used by HudUiBriefingRuntime::Update.
 */
struct BriefingAction {
    virtual int Tick(float deltaSec) = 0;
};

/**
 * Briefing-owned action sequence embedded in HudUiBriefingRuntime. The retail
 * node layout and cleanup are the official VC5 std::list implementation; the
 * queue owns list nodes while retaining non-owning BriefingAction pointers.
 */
struct Briefing_ActionQueue {
    std::list<BriefingAction *> actions;
    std::list<BriefingAction *>::iterator current;
    int active;

    Briefing_ActionQueue();
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
#if defined(_MSC_VER) && _MSC_VER == 1100
RECOIL_STATIC_ASSERT(sizeof(std::list<BriefingAction *>) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, current) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(Briefing_ActionQueue, active) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(Briefing_ActionQueue) == 0x14);
#endif

/**
 * Briefing locator child panel; BN constructor 0x403c10 and the runtime array
 * constructor prove six 0x40-byte HudUiCircle-derived elements. The retail
 * locator dispatch table is data/tier-S evidence debt, not a production source
 * scaffold.
 */
struct HudUiBriefingLocatorPanel : HudUiCircle {
    HudUiBriefingLocatorPanel();
    ~HudUiBriefingLocatorPanel();
    virtual void Draw();
    virtual void DrawBase();
    void Update(float deltaSec);
};

/**
 * Briefing objective picture widget. BN names the slot target
 * DrawWithNoiseOverlay; the source model is the HudUiWidget Draw override with
 * a briefing-only noiseAlpha member.
 * @recoil-anchor recoil:anchor:battlesport.briefing.objective-picture-type
 * @recoil-artifact emits .text recoil:function:0x403d70: VC5 compiler-generated scalar deleting-destructor contribution anchored to this complete type definition; not an authored body.
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
 * @recoil-anchor recoil:anchor:battlesport.briefing.transport-progress-type
 * @recoil-artifact emits .text recoil:function:0x403eb0: VC5 compiler-generated scalar deleting-destructor contribution anchored to this complete type definition; not an authored body.
 */
struct HudUiBriefingTransportProgress : HudUiFillBitmap {
};

/**
 * Briefing runtime owner. BN constructor/destructor and action callers prove
 * this HudUiBackground-derived member layout and embedded action queue.
 * @recoil-anchor recoil:anchor:battlesport.briefing.runtime-type
 * @recoil-artifact emits .text recoil:function:0x403d90: VC5 scalar deleting destructor for this virtual-destructor model.
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
    ~HudUiBriefingRuntime();
    int BuildObjectiveActionsFromIndex(int objectiveIndex);
    void Update(float deltaSec);
};
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, actionQueue) == 0xa94c);
#if defined(_MSC_VER) && _MSC_VER == 1100
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, transportProgress) == 0xa960);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, missionName) == 0xaae8);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, objectiveSummary) == 0xad8c);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, objectiveDesc) == 0xb030);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, objectivePicture) == 0xb2d4);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, transmissionHalted) == 0xb394);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, messagesPanel) == 0xb638);
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, locatorPanels) == 0xb8f0);
RECOIL_STATIC_ASSERT(sizeof(HudUiBriefingRuntime) == 0xba70);
#endif

namespace Briefing {
void __fastcall BuildObjectiveActionsGlobal(int objectiveIndex);
void __fastcall SampleEventCallback(int progressEventCode);
int __fastcall StartForMission(int missionId);
void __cdecl ThreadMain(void *threadParameter);
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
