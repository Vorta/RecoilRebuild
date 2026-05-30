#pragma once

#include <cstdarg>
#include <stddef.h>
#include "recoil/recoil_types.h"

#include <windows.h>

#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "recoil/recoil_callconv.h"
#include "zClipAlt.h"

struct zVec3;
struct zTag4Partial;
struct HudUiContainer;
struct HudUiContainer_FTable;
struct HudUiBackgroundContainer_FTable;
struct HudUiTriplet_FTable;
struct HudUiTextStack4_FTable;
struct HudUiStringMenu_FTable;
struct HudUiTextInput;

struct HudUiCommon_FTable {
    unsigned int slots[29];
};

struct HudUiWidget_FTable {
    unsigned int slots[34];
};

struct HudUiBackgroundCursorWidget_FTable {
    unsigned int slots[34];
};

struct HudUiBackgroundVideoWidget_FTable {
    unsigned int slots[34];
};

struct HudUiZrdWidget_FTable {
    unsigned int slots[34];
};

struct HudUiCreditsPanel_FTable {
    unsigned int primarySlots[4];
    HudUiZrdWidget_FTable SecondaryAction;
};

struct HudUiCheckToggleWidget_FTable {
    unsigned int slots[34];
};

struct HudUiCycleSelectorWidget_FTable {
    unsigned int slots[34];
};

struct HudUiFillBitmap_FTable {
    unsigned int slots[34];
};

struct HudUiZrdWidgetEx17C_Item_FTable {
    unsigned int slots[34];
};

struct HudUiZrdWidgetEx17C_FTable {
    unsigned int slots[34];
};

struct HudCmdBindButtonBase_FTable {
    unsigned int slots[34];
};

struct HudUiMessageBoxDialog_FTable {
    unsigned int slots[5];
};

struct HudUiCounter_FTable {
    unsigned int slots[34];
};

struct HudUiMessage_FTable {
    unsigned int slots[30];
};

struct HudUiPanel_FTable {
    unsigned int slots[37];
};

struct HudCmdDialog_BackgroundPanelFTable {
    unsigned int primarySlots[4];
    HudUiPanel_FTable SecondaryAction;
};

struct HudUiOptionsPanel_BackgroundFTable {
    unsigned int primarySlots[4];
    HudUiCycleSelectorWidget_FTable SecondaryAction;
};

struct HudUiCompositePanel_FTable {
    unsigned int slots[38];
};

struct HudUiListSelectorItem_FTable {
    unsigned int slots[37];
};

struct HudUiTextLabel_FTable {
    unsigned int slots[29];
};

struct HudUiPanelSimple_FTable {
    unsigned int slots[37];
};

struct HudUiBar_FTable {
    unsigned int slots[29];
};

struct HudUiPolyline_FTable {
    unsigned int slots[29];
};

struct HudUiSliderBorder_FTable {
    unsigned int slots[29];
};

struct HudUiMeter_FTable {
    unsigned int slots[29];
};

struct HudUiTimerPanelFloat_FTable {
    unsigned int slots[37];
};

struct HudUiTimerPanel_FTable {
    unsigned int slots[37];
};

struct HudUiCounterTextPanel_FTable {
    unsigned int slots[37];
};

struct HudUiStatsListElement_FTable {
    unsigned int slots[29];
};

struct HudUiTripletPanel_FTable {
    unsigned int slots[29];
};

struct HudUiTextInput_FTable {
    unsigned int slots[9];
};

struct HudUiNumericTextInput_Base_FTable {
    unsigned int slots[36];
};

struct HudUiClampedIntTextInput_FTable {
    unsigned int slots[36];
};

struct HudUiSlot_FTable {
    unsigned int slots[29];
};

struct HudUiElement;
struct HudUiPanel;
struct HudUiCounter;
struct HudUiMessage;
struct HudUiStringMenu;
struct HudUiShieldMessageWidget;
struct HudUiStatsListElement;
struct HudUiTextStack4;
struct HudUiTimerPanel;
struct HudUiTimerPanelFloat;
struct HudUiCounterTextPanel;
struct HudUiTripletPanel;
struct HudUiNanitePanel;
struct HudUiScoreboardEntry;
struct HudUiTriplet;
struct HudUiCircle;
struct HudUiBar;
struct HudUiZrdWidgetEx17C;
struct HudUiSlot;
struct HudUiMeter;
struct GameNetPlayerRow;
struct zZbdSectionCallbackCtx;
struct zSndSample;
struct zSndPlayHandle;
struct zFMV_Stream;

extern const HudUiCommon_FTable g_HudUiCommon_FTable;
extern const HudUiCommon_FTable g_HudUiCircle_FTable;
extern const HudUiContainer_FTable g_HudUiContainer_FTable;
extern const HudUiBackgroundContainer_FTable g_HudUiBackgroundContainer_FTable;
extern const HudUiBackgroundContainer_FTable g_HudUiBackground_FTable;
extern const HudUiWidget_FTable g_HudUiWidget_FTable;
extern const HudUiBackgroundCursorWidget_FTable g_HudUiBackgroundCursorWidget_FTable;
extern const HudUiBackgroundCursorWidget_FTable g_HudUiBackgroundCursorWidget_MemberFTable;
extern const HudUiBackgroundVideoWidget_FTable g_HudUiBackgroundVideoWidget_FTable;
extern const HudUiZrdWidget_FTable g_HudUiZrdWidget_FTable;
extern const HudUiZrdWidget_FTable g_HudUiCreditsButtonQueueExitOnly_Vtbl;
extern const HudUiZrdWidget_FTable g_HudUiCreditsButtonQueueExitAndLeaveNetwork_Vtbl;
extern const HudUiCreditsPanel_FTable g_HudUiCreditsPanel_FTable;
extern const HudUiCheckToggleWidget_FTable g_HudUiCheckToggleWidget_FTable;
extern const HudUiCycleSelectorWidget_FTable g_HudUiCycleSelectorWidget_FTable;
extern const HudUiFillBitmap_FTable g_HudUiFillBitmap_FTable;
extern const HudUiZrdWidgetEx17C_Item_FTable g_HudUiZrdWidgetEx17C_Item_FTable;
extern const HudUiZrdWidgetEx17C_FTable g_HudUiZrdWidgetEx17C_FTable;
extern const HudCmdBindButtonBase_FTable g_HudCmdBindButtonBase_FTable;
extern const HudUiZrdWidget_FTable g_HudCmdResumeButton_FTable;
extern const HudUiZrdWidget_FTable g_HudCmdResetButton_FTable;
extern const HudCmdBindButtonBase_FTable g_HudCmdCommandList_FTable;
extern const HudCmdBindButtonBase_FTable g_HudCmdKeyAButton_FTable;
extern const HudCmdBindButtonBase_FTable g_HudCmdKeyBButton_FTable;
extern const HudCmdBindButtonBase_FTable g_HudCmdJoyButton_FTable;
extern const HudCmdBindButtonBase_FTable g_HudCmdMouseButton_FTable;
extern const HudUiCycleSelectorWidget_FTable g_HudCmdSetList_FTable;
extern const HudUiZrdWidget_FTable g_HudCmdNextSetButton_FTable;
extern const HudUiZrdWidget_FTable g_HudCmdPrevSetButton_FTable;
extern const HudUiZrdWidget_FTable g_HudCmdNextCommandButton_FTable;
extern const HudUiZrdWidget_FTable g_HudCmdPrevCommandButton_FTable;
extern const HudCmdDialog_BackgroundPanelFTable g_HudCmdDialog_BackgroundPanelFTable;
extern const HudUiOptionsPanel_BackgroundFTable g_HudUiOptionsPanel_FTableHeader;
extern const HudUiZrdWidget_FTable g_HudUiOptionsPanel_BackButton_Vtbl;
extern const HudUiCheckToggleWidget_FTable g_HudUiOptionsPanel_LightingToggle_Vtbl;
extern const HudUiCheckToggleWidget_FTable g_HudUiOptionsPanel_PerspectiveToggle_Vtbl;
extern const HudUiCheckToggleWidget_FTable g_HudUiOptionsPanel_FullHudToggle_Vtbl;
extern const HudUiCycleSelectorWidget_FTable g_HudUiOptionsPanel_ObjectDetailSelector_Vtbl;
extern const HudUiCycleSelectorWidget_FTable g_HudUiOptionsPanel_TextureMemorySelector_Vtbl;
extern const HudUiCycleSelectorWidget_FTable g_HudUiOptionsPanel_EffectsSelector_Vtbl;
extern const HudUiCheckToggleWidget_FTable g_HudUiOptionsPanel_SoundActiveToggle_Vtbl;
extern const HudUiCycleSelectorWidget_FTable g_HudUiOptionsPanel_SoundQualitySelector_Vtbl;
extern const HudUiFillBitmap_FTable g_HudUiOptionsPanel_SoundVolumeWidget_Vtbl;
extern const HudUiCheckToggleWidget_FTable g_HudUiOptionsPanel_MusicEnableToggle_Vtbl;
extern const HudUiFillBitmap_FTable g_HudUiOptionsPanel_MusicVolumeWidget_Vtbl;
extern const HudUiMessageBoxDialog_FTable g_HudUiMessageBoxDialog_FTable;
extern const HudUiCounter_FTable g_HudUiCounter_FTable;
extern const HudUiMessage_FTable g_HudUiMessage_FTable;
extern const HudUiPanel_FTable g_HudUiPanel_FTable;
extern const HudUiPanel_FTable g_HudUiTransitionTextPanel_FTable;
extern int g_HudCmdMouseDebounceFrames;
extern const HudUiCompositePanel_FTable g_HudUiCompositePanel_FTable;
extern const HudUiListSelectorItem_FTable g_HudUiListSelectorItem_FTable;
extern const HudUiTextLabel_FTable g_HudUiTextLabel_FTable;
extern const HudUiPanelSimple_FTable g_HudUiPanelSimple_FTable;
extern const HudUiBar_FTable g_HudUiBar_FTable;
extern const HudUiPolyline_FTable g_HudUiPolyline_FTable;
extern const HudUiSliderBorder_FTable g_HudUiSliderBorder_FTable;
extern const HudUiMeter_FTable g_HudUiMeter_FTable;
extern const HudUiMeter_FTable g_HudUiMeterEx_FTable;
extern const HudUiTimerPanelFloat_FTable g_HudUiTimerPanelFloat_FTable;
extern const HudUiTimerPanel_FTable g_HudUiTimerPanel_FTable;
extern const HudUiCounterTextPanel_FTable g_HudUiCounterTextPanel_FTable;
extern const HudUiTriplet_FTable g_HudUiTriplet_FTable;
extern const HudUiTextStack4_FTable g_HudUiTopMessageStack_FTable;
extern const HudUiTextStack4_FTable g_HudUiChatMessageStack_FTable;
extern const HudUiStringMenu_FTable g_HudUiStringMenu_FTable;
extern const HudUiStatsListElement_FTable g_HudUiStatsListElement_FTable;
extern const HudUiTripletPanel_FTable g_HudUiTripletPanel_FTable;
extern const HudUiTextInput_FTable g_HudUiTextInput_FTable;
extern const HudUiTextInput_FTable g_HudUiNumericTextInput_TextInputFTable;
extern const HudUiNumericTextInput_Base_FTable g_HudUiNumericTextInput_Base_FTable;
extern const HudUiNumericTextInput_Base_FTable g_HudUiNumericTextInput_CtorTable_FTable;
extern const HudUiClampedIntTextInput_FTable g_HudUiClampedIntTextInput_CtorTable_FTable;
extern const HudUiSlot_FTable g_HudUiSlot_FTable;
extern const HudUiZrdWidget_FTable g_HudUiMessageBoxOkButton_Vtbl;
extern const HudUiZrdWidget_FTable g_HudUiMessageBoxCancelButton_Vtbl;
struct HudUiWidget;
struct HudUiMgrSensorTrackNode;
struct PlayerProgressTargetSlotRuntime;
extern zVidImagePartial *g_HudUiWidget_ExclusiveDrawImage;
extern HudUiContainer g_HudUiMgr;
extern HudUiTimerPanel *g_HudUiMgrTimerPanel;
extern HudUiTimerPanelFloat *g_HudUiMgrTimerPanelFloat;
extern HudUiStringMenu *g_HudUiMgrStringMenu;
extern HudUiStatsListElement *g_HudUiMgrStatsList;
extern zVidImagePartial *g_HudUiMgrReticleImages[3];
extern zVidImagePartial *g_HudUiMgrSensorTargetMarkerImages[5];
extern HudUiNanitePanel g_HudUiMgrNanitePanel;
extern HudUiMessage g_HudUiMgrMessages[10];
extern int g_HudUiMgrActiveWeaponMessageIndex;
extern int g_HudUiMgrActiveWeaponSideIndex;
extern HudUiCounter g_HudUiMgrModeCounters[4];
extern HudUiSlot g_HudUiMgrWeaponSlots[32];
extern HudUiSlot *g_HudUiMgrSensorTrackedProgressSlot;
extern int g_HudUiMgrSensorRoundRobinTrackIndex;
extern HudUiMeter g_HudUiMgrObjectiveMeter;
extern int g_HudUiMgrActiveModeCounterIndex;
extern int g_HudUiMgrHudLayoutsInitialized;
extern int g_HudUiMgrHudLoaded;
extern int g_HudUiMgrLayoutDelayFrames;
extern int g_HudUiMgrSensorTargetMarkerCount;
extern int g_HudUiMgrWeaponState;
extern int g_HudUiMgrHudOriginX;
extern int g_HudUiMgrHudOriginY;

struct HudUiRect {
    int left;
    int top;
    int right;
    int bottom;
};

extern HudUiRect g_HudUiMgrSensor_FxRectScratch;

namespace HudUiLayoutNode {
RECOIL_NOINLINE int RECOIL_FASTCALL ReadRect(zReader::Node *node, HudUiRect *outRect);
RECOIL_NOINLINE int RECOIL_FASTCALL
ReadInt3(zReader::Node *node, int *out0, int *out1, int *out2);
RECOIL_NOINLINE int RECOIL_FASTCALL
ReadRectOffsetAndSize(zReader::Node *node, HudUiRect *outRect, const int *offsetXY,
                      int *outWidth, int *outHeight);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyTextLabel(zReader::Node *layoutNode, HudUiPanel *target, int baseX,
               int baseY, const int *offsetXY);
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
ApplyImageWidget(zReader::Node *layoutNode, HudUiWidget *widget, int baseX,
                 int baseY, const int *anchorOrNull,
                 zVidImagePartial *preloadedImageOrNull, HudUiRect *outRectOrNull);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyCornerTextQuad(zReader::Node *node, HudUiBar *target, const int *offsetXY,
                    HudUiRect *outRect);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyMeterQuad(zReader::Node *node, HudUiMeter *target, int xBase, int yBase,
               const int *offsetXY, HudUiRect *outRect);
} // namespace HudUiLayoutNode

struct HudUiMgrSensorBlock {
    int state;
    HudUiRect sensorRectScaled;
    HudUiRect sensorRectRaw;
    HudUiRect sensorViewportRect;
    zClipAltFloatRect sensorPiVSrcRect;
    float sensorClampHalfW;
    float sensorClampHalfH;
    float sensorParam;
    float sensorRangeSq;
};

extern HudUiMgrSensorBlock g_HudUiMgrSensorBlock;
extern HudUiRect g_HudUiMgrSensorFxRect;
extern int g_HudUiMgrSensorFxViewportWidth;
extern int g_HudUiMgrSensorFxViewportHeight;
extern HudUiRect g_HudUiMgrHudRect;
extern HudUiRect g_HudUiMgrViewRect;
extern float g_HudUiMgrHudRectW;
extern float g_HudUiMgrHudRectH;
extern float g_HudUiMgrReticleMapBiasX;
extern float g_HudUiMgrReticleMapBiasY;
extern float g_HudUiMgrReticleMapScaleHalfW;
extern float g_HudUiMgrReticleMapScaleHalfH;
extern int g_HudUiMgrReticleSnapRadiusSq;
extern float g_HudUiMgrReticleProjection[3];
extern int g_HudUiMgrReticleWidgetHalfW;
extern int g_HudUiMgrReticleWidgetHalfH;
extern int g_HudUiMgrReticleProjectedX;
extern int g_HudUiMgrReticleProjectedY;
extern int g_HudUiMgrReticleMode;
extern HudUiWidget g_HudUiMgrReticleWidget;

struct HudLayoutBase;
typedef void (RECOIL_FASTCALL *HudLayoutOnActivatedFn)(HudLayoutBase *self);

struct HudLayoutBase_FTable {
    unsigned int slots[6];
    HudLayoutOnActivatedFn OnActivated;
};

struct HudLayoutBase {
    const HudLayoutBase_FTable *ftable;
    int enabled;
    HudUiElement *childHead;
    HudUiElement *childTail;
    HudUiRect layoutRect;
    HudUiRect activeRect;

    static void RECOIL_CDECL Shutdown_Stub();
    void RECOIL_THISCALL Destructor();
    int RECOIL_THISCALL SetActiveNoOp(int active);
    RECOIL_NOINLINE void RECOIL_THISCALL UpdateAll(float deltaSeconds);
    void RECOIL_THISCALL Enable();
    void RECOIL_THISCALL Disable();
    RECOIL_NOINLINE void RECOIL_THISCALL LoadTypeIFromZarRoot(zReader::Node *parentNode);
};

struct HudLayoutHW {
    unsigned char unknown_000[0x1a8];
    zVidImagePartial *widget1ImageDefault;
    zVidImagePartial *widget1Image320;
    zVidImagePartial *widget1Image400;
    unsigned char unknown_1b4[0xbc];
    zVidImagePartial *widget2ImageDefault;
    zVidImagePartial *widget2Image320;
    zVidImagePartial *widget2Image400;
    unsigned char unknown_27c[0xbc];
    HudUiRect reticleClipRect;
    unsigned char reticleClipInitFlags;
    unsigned char unknown_349[0x03];

    RECOIL_NOINLINE int RECOIL_THISCALL LoadTypeIIFromZarRoot(zReader::Node *parentNode);
    void RECOIL_THISCALL ReleaseImages();
    int RECOIL_THISCALL SetActive(int active);
    void RECOIL_THISCALL UpdateObjectiveDirtyRect();
    void RECOIL_THISCALL OnActivated();
    void RECOIL_THISCALL Enable();
    void RECOIL_THISCALL Disable();
};

extern HudLayoutHW g_HudLayoutHW;
extern HudLayoutBase g_HudLayoutSW;

extern HudLayoutBase *g_HudUiMgrCurrentLayout;
extern HudUiTextStack4 *g_HudUiTopMessageStack;
extern HudUiTextStack4 *g_HudUiChatMessageStack;

extern HudUiShieldMessageWidget *g_HudUiMgrShieldMessageWidget;

namespace HudLayout {
RECOIL_NOINLINE int RECOIL_FASTCALL ApplyViewportRect(HudUiRect *activeRect);
}

namespace HudUiMgrSensor {
RECOIL_NOINLINE HudUiMgrSensorTrackNode *RECOIL_FASTCALL TrackList_Add(int trackKind,
                                                                        void *payload);
RECOIL_NOINLINE int RECOIL_FASTCALL PlaceTrackCounterWidget(
    HudUiMgrSensorTrackNode *trackNode, const zVec3 *worldPoint);
RECOIL_NOINLINE int RECOIL_FASTCALL
PlaceTrackMarker(int markerMode, PlayerProgressTargetSlotRuntime *outputSlots);
RECOIL_NOINLINE void RECOIL_FASTCALL
UpdateMarkersAndProgressFromVariantTag(const zTag4Partial *requiredVariantTag);
void RECOIL_FASTCALL SetShieldMessageRatio(float ratio);
void RECOIL_FASTCALL SetViewportRect(int x, int y, int width,
                                     int height);
RECOIL_NOINLINE void RECOIL_FASTCALL GetFxRect(HudUiRect *outRect);
}

namespace HudUiMgr {
RECOIL_NOINLINE int RECOIL_FASTCALL ProjectPointToNormalizedClamped(
    const zVec3 *srcPoint, zVec3 *projectedPoint);
void RECOIL_FASTCALL ScreenToWorld(float *pointXY);
void RECOIL_CDECL TriggerCurrentLayoutOnActivated();
int RECOIL_CDECL TickLayoutDelay();
void RECOIL_FASTCALL SetNanitePanelCount(int count);
void RECOIL_FASTCALL SetModeCounterState(int counterIndex, int state);
void RECOIL_CDECL ReticleStaticAtexitStub();
void RECOIL_FASTCALL CopyReticleProjection(float *outProjection);
void RECOIL_FASTCALL SetReticleMode(int mode);
RECOIL_NOINLINE int RECOIL_FASTCALL EnsureHudLoaded(const char *entryPath);
RECOIL_NOINLINE int RECOIL_FASTCALL UpdateTargetReticleFromCursor(int reticleMode,
                                                                           zVec3 *worldHitPoint,
                                                                           float normalizedX,
                                                                           float normalizedY);
void RECOIL_FASTCALL OnViewportChanged(const HudUiRect *hudRectOrNull,
                                       const HudUiRect *viewRectOrNull);
void RECOIL_FASTCALL ActivateHud(const HudUiRect *hudRectOrNull, const HudUiRect *viewRectOrNull);
RECOIL_NOINLINE void RECOIL_CDECL DestroySensorWindow();
RECOIL_NOINLINE int RECOIL_CDECL EnableHud();
RECOIL_NOINLINE int RECOIL_CDECL DisableHud();
RECOIL_NOINLINE int RECOIL_CDECL ToggleHud();
RECOIL_NOINLINE void RECOIL_CDECL UpdateFrame();
RECOIL_NOINLINE void RECOIL_FASTCALL SwitchActiveDialog(HudLayoutBase *newDialog);
RECOIL_NOINLINE int RECOIL_FASTCALL ApplyHudModeSwitch(int hudType);
RECOIL_NOINLINE void RECOIL_FASTCALL SetFloatTimerVisible(int visible);
RECOIL_NOINLINE void RECOIL_FASTCALL HideTrackedProgressMeterIfOwnerMatches(void *ownerPayload);
RECOIL_NOINLINE void RECOIL_FASTCALL SetAuxOverlayVisible(int visible);
void RECOIL_CDECL EnableTopAndChatStacks();
void RECOIL_CDECL DisableTopAndChatStacks();
int RECOIL_FASTCALL InitHudLayouts(const HudUiRect *displaySection,
                                            const HudUiRect *windowSection);
void RECOIL_CDECL ShutdownResources();
} // namespace HudUiMgr

namespace HudUiAuxOverlay {
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateTextLine(int op, int index,
                                                    const char *format);
RECOIL_NOINLINE void RECOIL_CDECL ClearTextLines();
} // namespace HudUiAuxOverlay

namespace HudUiLoadingCheckpoint {
void RECOIL_FASTCALL AdvanceAndLog(const char *messageOrNull);
void RECOIL_CDECL InitTable();
} // namespace HudUiLoadingCheckpoint

namespace HudUi {
RECOIL_NOINLINE void RECOIL_FASTCALL SetInvalidateMode(int mode);
RECOIL_NOINLINE int RECOIL_FASTCALL ShowMessageBox(const char *messageText,
                                                   const char *titleText,
                                                   void *modalContext);
RECOIL_NOINLINE void RECOIL_FASTCALL HandleHotkeyCommand(int commandId);
void RECOIL_FASTCALL ShowTopMessageLine(const char *message, float duration);
void RECOIL_FASTCALL ShowChatLine(const char *message, float duration);
void RECOIL_FASTCALL PushTopMessageLine(const char *message, float duration);
RECOIL_NOINLINE void RECOIL_FASTCALL PlayPowerupSfx(int shouldPlay);
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshScoreboardEntryRow(GameNetPlayerRow *entryData);
RECOIL_NOINLINE void RECOIL_FASTCALL RemoveScoreboardEntryRow(GameNetPlayerRow *entryKey);
} // namespace HudUi

namespace HudScoreboard {
RECOIL_NOINLINE void RECOIL_STDCALL SetScaleAndRebuild(float scale);
RECOIL_NOINLINE void RECOIL_STDCALL DispatchSetScale(float deltaTime);
} // namespace HudScoreboard

extern int g_HudUiMgrObjectivePhase;
extern int g_HudUiMgrObjectiveState;
extern int g_HudUiMgrObjectiveChatComposeActive;
extern float g_HudUiMgrObjectivePhaseTimerSec;
extern float g_HudUiMgrObjectivePhaseDurationSec;
extern float g_HudUiMgrObjectiveAutoHideDelaySec;
extern int g_HudUiMgrObjectiveShowResetUnused;
extern int g_HudUiMgrObjectiveWidgetRightX;
extern float g_HudUiMgrObjectiveMeterFillAnimTimerSec;
extern unsigned int g_HudUiMgrObjectiveMeterFillAnimEnabled;
extern HudUiWidget g_HudUiMgrSensorPanel;
extern HudUiWidget g_HudUiMgrObjectiveWidget;
extern HudUiBar g_HudUiMgrObjectiveBar;
extern HudUiWidget g_HudUiMgrObjectiveSensorRect;
extern HudUiWidget g_HudUiMgrSensorOverlay;
extern HudUiMeter g_HudUiMgrSensorMeter;
extern HudUiPanel *g_HudUiMgrObjectiveSummaryTextPanel;
extern HudUiPanel *g_HudUiMgrObjectiveDescTextPanel;
extern HudUiPanel *g_HudUiMgrObjectiveLabelTextPanel;
extern HudUiCounterTextPanel *g_HudUiMgrObjectiveCounterTextPanel;
extern HudUiTextInput g_HudUiMgrObjectiveChatComposeTextInput;
extern int g_HudUi_AuxOverlayEnabled;

namespace HudUiMgrObjective {
RECOIL_NOINLINE void RECOIL_FASTCALL RefreshCounterText(int counterValue);
void RECOIL_FASTCALL SetVisibleAndResetMeterFill(int visible);
RECOIL_NOINLINE void RECOIL_CDECL TickMeterFillAnimation();
RECOIL_NOINLINE void RECOIL_CDECL UpdateMeterXPoints();
int RECOIL_FASTCALL Show(zVidImagePartial *objectiveImage, const char *summaryFormat,
                                  const char *descText, float autoHideDelay);
void RECOIL_CDECL Begin();
RECOIL_NOINLINE void RECOIL_CDECL StartHide();
void RECOIL_CDECL Update();
} // namespace HudUiMgrObjective

namespace HudUiMgrTarget {
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateSelectedProgressMeter(int clearSelectedTrack);
} // namespace HudUiMgrTarget

struct HudUiElement {
    const HudUiCommon_FTable *ftable;
    HudUiElement *next;
    void *parent;
    unsigned int flags;
    float timer;
    int x;
    int y;
    void *bltSource;
    HudUiRect clipRect;
    unsigned short state;
    unsigned short padding32;

    HudUiElement *RECOIL_THISCALL Constructor(int x, int y);
    RECOIL_NOINLINE HudUiElement *RECOIL_THISCALL CopyConstructor(const HudUiElement *source);
    RECOIL_NOINLINE HudUiElement *RECOIL_THISCALL CopyFrom(const HudUiElement *source);
    HudUiElement *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    RECOIL_NOINLINE void RECOIL_THISCALL Invalidate();
    void RECOIL_THISCALL SetPos(int x, int y);
    void RECOIL_THISCALL SetX(int x);
    void RECOIL_THISCALL SetY(int y);
    void RECOIL_THISCALL SetVisible(int visible);
    void RECOIL_THISCALL ResetCommonFTable();
    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL SetTimer(float duration);
    RECOIL_NOINLINE void RECOIL_THISCALL SetBltSourceAndClipRect(void *bltSourceOrNull,
                                                                 const HudUiRect *rectOrNull);
    void RECOIL_THISCALL SetClipRect(const HudUiRect *clipRect);
    void RECOIL_THISCALL GetRect(HudUiRect *outRect);
    unsigned char RECOIL_THISCALL HitTestTrue(int px, int py);
    int RECOIL_THISCALL GetX();
    int RECOIL_THISCALL GetY();
};

struct StdPtrVector {
    void RECOIL_THISCALL ClearNoOpDestroy(int *begin, int *end);
};

struct HudUiPrimitiveBindTarget {
    HudUiElement base;
    int endX;
    int endY;
    unsigned int color565;

    void RECOIL_THISCALL SetSegmentEndpoints(int startX, int startY,
                                             int endX, int endY);
};

struct HudUiTextLabel {
    HudUiElement base;
    char textBuffer[0x100];
    int fontHandle;
    int centerText;
    int centerBoundsLeft;
    int centerBoundsRight;
    int alignMode;

    RECOIL_NOINLINE HudUiTextLabel *RECOIL_THISCALL ConstructorWithPosAndFlags(const char *text,
                                                                               int x,
                                                                               int y,
                                                                               int flags);
    HudUiTextLabel *RECOIL_THISCALL CopyConstructor(const HudUiTextLabel *source);
    HudUiTextLabel *RECOIL_THISCALL Constructor(const HudUiTextLabel *source);
    void SetTextFmt(const char *format, ...);
    RECOIL_NOINLINE void RECOIL_THISCALL RebuildTextBounds();
    RECOIL_NOINLINE int RECOIL_THISCALL MeasureTextWidth();
    void RECOIL_THISCALL UpdateTextExtents();
};

struct HudUiCircle {
    HudUiElement base;
    int radius;
    int radiusSquared;
    unsigned int color565;

    HudUiCircle *RECOIL_THISCALL Constructor(int x, int y,
                                             int circleRadius,
                                             unsigned int circleColor565);
    int RECOIL_THISCALL HitTest(int px, int py);
    int RECOIL_THISCALL HitTestCore(int px, int py);
};

struct HudUiContainer {
    const HudUiContainer_FTable *vptr;
    int enabled;
    HudUiElement *childHead;
    HudUiElement *childTail;

    HudUiContainer *RECOIL_THISCALL ConstructorDefault();
    RECOIL_NOINLINE void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL SetEnabled(int enabled);
    RECOIL_NOINLINE int RECOIL_THISCALL AddChild(HudUiElement *child);
    RECOIL_NOINLINE int RECOIL_THISCALL FindChildWithPrev(HudUiElement *child,
                                                                   HudUiElement **previousOut);
    int RECOIL_THISCALL RemoveChild(HudUiElement *child);
    void RECOIL_THISCALL SetChildFlags(unsigned int childFlags);
    void RECOIL_THISCALL UpdateAll(float deltaSeconds);
    void RECOIL_THISCALL InvalidateChildren();
};

typedef void (HudUiContainer::*HudUiContainerUpdateAllFn)(float deltaSeconds);
typedef void (HudUiContainer::*HudUiContainerSetEnabledFn)(int enabled);

struct HudUiContainer_FTable {
    HudUiContainerUpdateAllFn updateAll;
    HudUiContainerSetEnabledFn setEnabled;
};

struct HudUiBackgroundContainer_FTable {
    HudUiContainerUpdateAllFn updateAll;
    HudUiContainerSetEnabledFn setEnabled;
};

struct HudUiTriplet_FTable {
    HudUiContainerUpdateAllFn updateAll;
    HudUiContainerSetEnabledFn setEnabled;
};

struct HudUiTextStack4_FTable {
    HudUiContainerUpdateAllFn updateAll;
    HudUiContainerSetEnabledFn setEnabled;
};

struct HudUiStringMenu_FTable {
    HudUiContainerUpdateAllFn updateAll;
    HudUiContainerSetEnabledFn setEnabled;
};

struct HudUiBackgroundContainer {
    HudUiContainer base;
    HudUiElement *inputFocusElement;
    unsigned char reserved14[0x2c];
    int captureTransitionMask;

    HudUiBackgroundContainer *RECOIL_THISCALL Constructor(int initFlag);
    void RECOIL_THISCALL Destructor();
    void RECOIL_THISCALL SetEnabled(int enabled);
    void RECOIL_THISCALL SetInputFocus(HudUiElement *element);
    HudUiElement *RECOIL_THISCALL GetInputFocus();
};

struct HudUiBackgroundSoundEntry {
    zSndSample *sample;
    float volume;
    zSndPlayHandle *playHandle;
};

struct HudUiDialogController {
    unsigned char reserved00[0x114];
    zVidImagePartial *capturedImage;

    void RECOIL_THISCALL BlitOwnedSurfaceToPrimary();
};

struct HudUiRectDirty {
    unsigned int framesRemaining;
    int drawX;
    int drawY;
    int srcLeft;
    int srcTop;
    int srcRight;
    int srcBottom;
};

struct HudUiWidget {
    const HudUiWidget_FTable *ftable;
    HudUiElement *next;
    void *parent;
    unsigned int flags;
    float timer;
    int x;
    int y;
    void *bltSource;
    HudUiRect clipRect;
    unsigned short state;
    unsigned short padding32;
    unsigned int ownsImage;
    unsigned int dirtyRectCount;
    zVidImagePartial *image;
    unsigned int imageStateWord;
    HudUiRect *bltClipRectOrNull;
    unsigned int alignFlags;
    HudUiRectDirty dirtyRects[4];

    HudUiWidget *RECOIL_THISCALL Constructor(unsigned int alignFlags);
    HudUiWidget *RECOIL_THISCALL CtorDefaultThunk();
    RECOIL_NOINLINE void RECOIL_THISCALL DestructorCore();
    RECOIL_NOINLINE void RECOIL_THISCALL ReleaseImageIfOwned();
    RECOIL_NOINLINE zVidImagePartial *RECOIL_THISCALL SetImageByPathOwned(const char *imagePath);
    RECOIL_NOINLINE zVidImagePartial *RECOIL_THISCALL
    SetImageBorrowedAndInvalidate(zVidImagePartial *image);
    void RECOIL_THISCALL InvalidateRect(const HudUiRect *dirtyRect);
    HudUiWidget *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    int RECOIL_THISCALL GetCenterX();
    int RECOIL_THISCALL GetCenterY();
    RECOIL_NO_GS void RECOIL_THISCALL RebuildBltRectFromImage();
    void RECOIL_THISCALL Draw();
};

struct HudUiBackgroundCursorWidget {
    HudUiWidget base;
    zVidImagePartial *capturedImage;
    int captureEnabled;
    int captureSourceSelector;
    int reservedC8;
    int reservedCC;

    HudUiBackgroundCursorWidget *RECOIL_THISCALL
    MemberConstructorLocal(const char *imagePath, int captureEnabled);
    void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL SetImageByPathOwnedAndRefresh(const char *imagePath);
    void RECOIL_THISCALL SetImageBorrowedAndRefreshIfChanged(zVidImagePartial *image);
    void RECOIL_THISCALL SetImageOwnedAndRefresh(int captureEnabled);
    void RECOIL_THISCALL SetImageBorrowedAndRefresh();
    void RECOIL_THISCALL SetPos(int x, int y);
    void RECOIL_THISCALL RebuildCapturedImage(int x, int y);
    void RECOIL_THISCALL Draw();
    void RECOIL_THISCALL DrawBase();
};

struct HudUiBackgroundVideoWidget {
    HudUiElement base;
    zFMV_Stream *stream;
    float elapsedTimeSec;
    unsigned short colorKey565;
    char mediaPath[0x106];

    HudUiBackgroundVideoWidget *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Destructor();
    void RECOIL_THISCALL SetMediaPathOwnedAndRefresh(const char *path);
    void RECOIL_THISCALL SetColorKey565(unsigned short colorKey);
    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL Draw();
    void RECOIL_THISCALL DrawBase();
    void RECOIL_THISCALL RebuildBltRect();
};

struct HudUiPanelPtrVector {
    char allocatorProxy;
    char padding[3];
    HudUiPanel **begin;
    HudUiPanel **end;
    HudUiPanel **capacityEnd;

    HudUiPanel **RECOIL_THISCALL EraseRange(HudUiPanel **first, HudUiPanel **last);
    void RECOIL_THISCALL InsertN(HudUiPanel **position, unsigned int count,
                                 HudUiPanel **valueSource);
};

struct HudUiZrdWidget {
    HudUiWidget base;
    int originX;
    int originY;
    int modeOrEnabled;
    void *owner;
    HudUiRect boundsRect;
    zVidImagePartial *defaultImage;
    zVidImagePartial *disabledImage;
    zVidImagePartial *rolloverImage;
    zSndSample *rolloverSound;
    zSndPlayHandle *rolloverPlayHandle;
    float rolloverSoundScale;
    zVidImagePartial *activateImage;
    zSndSample *activateSound;
    zSndPlayHandle *activatePlayHandle;
    float activateSoundScale;
    zSndSample *disabledSound;
    float disabledSoundScale;
    HudUiPanelPtrVector labelPanels;
    HudUiPanelPtrVector rolloverLabelPanels;
    HudUiPanelPtrVector activateLabelPanels;
    HudUiPanelPtrVector disabledLabelPanels;

    HudUiZrdWidget *RECOIL_THISCALL Constructor();
    HudUiZrdWidget *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    HudUiZrdWidget *RECOIL_THISCALL ScalarDeletingDestructorThunk(unsigned int flags);
    RECOIL_NOINLINE void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL Invalidate();
    HudUiRect *RECOIL_THISCALL GetBoundsRectOrNull();
    void RECOIL_THISCALL ShowPreview();
    void RECOIL_THISCALL OnActivate();
    void RECOIL_THISCALL OnActivateQueueExitCurrentState();
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
    void RECOIL_THISCALL RefreshState();
    void RECOIL_THISCALL HidePreview();
    static void *RECOIL_STDCALL DeleteChildIfPresent(void *childWidgetOrNull);
};

struct HudUiCheckToggleWidget {
    HudUiZrdWidget base;
    int checked;
    zVidImagePartial *disabledCheckedImage;
    zVidImagePartial *disabledCheckedFallbackImage;
    zVidImagePartial *uncheckedImage;
    zVidImagePartial *checkedImage;
    HudUiPanel *checkedLabelPanel;

    RECOIL_NOINLINE HudUiCheckToggleWidget *RECOIL_THISCALL Constructor();
    HudUiCheckToggleWidget *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    HudUiCheckToggleWidget *RECOIL_THISCALL ScalarDeletingDestructorThunk(unsigned int flags);
    void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL DestructorCoreThunk();
    HudUiRect *RECOIL_THISCALL GetBoundsRectOrNull();
    void RECOIL_THISCALL RefreshState();
    void RECOIL_THISCALL ShowPreview();
    void RECOIL_THISCALL HidePreview();
    void RECOIL_THISCALL OnActivate();
    void RECOIL_THISCALL OnActivateThunk();
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
    RECOIL_NOINLINE int RECOIL_THISCALL SetChecked(int newChecked);
};

struct HudUiCycleSelectorWidget {
    HudUiZrdWidget base;
    int selectedIndex;
    int itemCount;
    int firstIndex;
    int visibleCount;
    void *fontStyleRef;
    int textOffsetX;
    int textOffsetY;
    HudUiWidget *entriesA[20];
    HudUiWidget *entriesB[20];

    HudUiCycleSelectorWidget *RECOIL_THISCALL Constructor();
    HudUiCycleSelectorWidget *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    HudUiCycleSelectorWidget *RECOIL_THISCALL ScalarDeletingDestructorThunk(unsigned int flags);
    void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL DestructorCoreThunk();
    void RECOIL_THISCALL AdvanceSelectionAndActivate();
    void RECOIL_THISCALL SetIndexClamped(int index);
    void RECOIL_THISCALL SetVisibleRange(int first, int last);
    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL AddTextEntry(int index, const char *text, int posX,
                                      int posY);
    void RECOIL_THISCALL ApplyFontStyleForEntry(int index, int styleIndex);
    void RECOIL_THISCALL AddBitmapEntry(int index, const char *imagePath,
                                        int posX, int posY);
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
};

struct HudUiFillBitmap {
    HudUiZrdWidget base;
    float normalizedValue;
    zVidImagePartial *fillImage;
    HudUiRect fillRect;
    int fillOffsetX;
    int fillOffsetY;
    zVidImagePartial *previewImage;
    HudUiRect previewRect;
    int previewOffsetX;
    int previewOffsetY;

    HudUiFillBitmap *RECOIL_THISCALL Constructor();
    HudUiFillBitmap *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL DestructorCoreThunk();
    void RECOIL_THISCALL Draw();
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
    void RECOIL_THISCALL UpdateNormalizedFromCursor();
    void RECOIL_THISCALL SetNormalizedValue(float value);
    void RECOIL_THISCALL SetNormalizedValueAndRebuild(float value);
};

struct HudUiZrdWidgetEx17C_Item {
    HudUiZrdWidget base;
    int selected;
    HudUiZrdWidgetEx17C *ownerSelector;
    int itemIndex;
    int mouseRectValid;
    HudUiRect mouseRect;
    zVidImagePartial *selectedImage;
    zVidImagePartial *unselectedImage;
    zVidImagePartial *selectedRolloverImage;
    zVidImagePartial *unselectedRolloverImage;

    HudUiZrdWidgetEx17C_Item *RECOIL_THISCALL Constructor();
    HudUiZrdWidgetEx17C_Item *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL ShowPreviewIfNotSelected();
    void RECOIL_THISCALL HidePreviewIfNotSelected();
    void RECOIL_THISCALL OnActivateSelectSelf();
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
    void RECOIL_THISCALL SetSelected(int selectedValue);
    HudUiRect *RECOIL_THISCALL GetMouseRectOrBounds();
};

struct HudUiZrdWidgetEx17C {
    HudUiZrdWidget base;
    int optionCount;
    HudUiZrdWidgetEx17C_Item *options[10];
    int selectedIndex;

    HudUiZrdWidgetEx17C *RECOIL_THISCALL Constructor();
    HudUiZrdWidgetEx17C *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    HudUiZrdWidgetEx17C *RECOIL_THISCALL ScalarDeletingDestructorThunk(unsigned int flags);
    void RECOIL_THISCALL DestructorCore();
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
    void RECOIL_THISCALL EnableChildAtIndex(int childIndex);
    int RECOIL_THISCALL SetSelectedIndex(int index);
};

struct HudUiListSelectorItem {
    unsigned char panelStorage[0x2a4];
    unsigned int unknown_2a4;
    void *owner;

    HudUiListSelectorItem *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Draw();
};

struct HudCmdBindingVector {
    unsigned char allocator;
    unsigned char padding_01[3];
    void *begin;
    void *end;
    void *capacity;
};

struct HudCmdBindingEntry {
    char *displayText;
    int commandId;

    HudCmdBindingEntry *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    static HudCmdBindingEntry *RECOIL_STDCALL DeleteAndReturnNull(HudCmdBindingEntry *entry);
    static HudCmdBindingEntry **RECOIL_FASTCALL
    CopyRange(HudCmdBindingEntry **sourceBegin, HudCmdBindingEntry **sourceEnd,
              HudCmdBindingEntry **dest);
};

struct HudCmdBinding {
    char *displayText;

    static HudCmdBinding **RECOIL_FASTCALL
    DestroyRange(HudCmdBinding **first, HudCmdBinding **last,
                 HudCmdBinding **dest, void *unusedAlloc);
};

struct HudCmdBindButtonBase {
    HudUiCheckToggleWidget base;
    int bindingSlotTotalCount;
    int visibleBindingSlotCount;
    HudUiListSelectorItem bindPanel;
    HudUiListSelectorItem *bindingSlotPanels;
    HudCmdBindingVector bindingVec;
    int bindingSlotSpacing;
    int selectedBindingIndex;
    float visibleListOffsetX;
    float visibleListOffsetY;
    float overflowListOffsetX;
    float overflowListOffsetY;
    int selectedFontStyleRef;
    int listFontStyleRef;

    HudCmdBindButtonBase *RECOIL_THISCALL Constructor();
    int RECOIL_THISCALL AddBindingEntry(const char *displayText, int commandId);
    void RECOIL_THISCALL SetSelectedEntry(int selectedIndex);
    void RECOIL_THISCALL OnSelectionChangedRefresh(int selectedIndex);
    void RECOIL_THISCALL ClearBindingEntries();
    void RECOIL_THISCALL DestructorCore();
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
    void RECOIL_THISCALL RebuildBindingSlotWidgets(int totalCount,
                                                   int visibleCount);
};

struct HudCmdCommandList {
    HudCmdBindButtonBase base;

    void RECOIL_THISCALL Destructor();
    HudCmdCommandList *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};

struct HudCmdKeyAButton {
    HudCmdBindButtonBase base;

    void RECOIL_THISCALL Destructor();
    HudCmdKeyAButton *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL OnBeginCapture();
    void RECOIL_THISCALL OnClearBinding();
};

struct HudCmdKeyBButton {
    HudCmdBindButtonBase base;

    void RECOIL_THISCALL Destructor();
    HudCmdKeyBButton *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL OnBeginCapture();
    void RECOIL_THISCALL OnClearBinding();
};

struct HudCmdJoyButton {
    HudCmdBindButtonBase base;

    void RECOIL_THISCALL Destructor();
    HudCmdJoyButton *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL OnBeginCapture();
    void RECOIL_THISCALL OnClearBinding();
};

struct HudCmdMouseButton {
    HudCmdBindButtonBase base;

    void RECOIL_THISCALL Destructor();
    HudCmdMouseButton *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL OnBeginCapture();
    void RECOIL_THISCALL OnClearBinding();
};

RECOIL_NOINLINE void **RECOIL_FASTCALL zUtil_StdPtrVector_Clear(HudCmdBindingVector *self);
RECOIL_NOINLINE void RECOIL_FASTCALL
zUtil_StdPtrVector_FreeBufferAndReset(HudCmdBindingVector *self);

struct HudUiMessageBoxDialog;

struct HudUiMessageBoxOkButton {
    HudUiZrdWidget base;

    void RECOIL_THISCALL OnActivate();
};

struct HudUiMessageBoxCancelButton {
    HudUiZrdWidget base;

    void RECOIL_THISCALL OnActivate();
};

struct HudUiTripletPanel {
    HudUiElement base;
    int visibleCount;
    unsigned char unknown_38[0x04];
    HudUiWidget items[3];

    HudUiTripletPanel *RECOIL_THISCALL Constructor();
    HudUiTripletPanel *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL Draw();
    RECOIL_NOINLINE void RECOIL_THISCALL SetVisibleCount(int count);
    void RECOIL_THISCALL ShutdownItems_Stub();
    void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL UnwindDestructFirstItem();
};

struct HudUiNanitePanel : HudUiTripletPanel {
    RECOIL_NOINLINE void RECOIL_THISCALL InitLayout(zReader::Node *layoutRoot);
};

struct HudUiPanelFull {
    unsigned char storage[0x2a4];
    int unknown_2a4;
    int layoutX;
    int layoutY;
};

struct HudUiPanelFontParams {
    const char *faceName;
    int height;
    int weight;
    int width;
};

struct HudUiMessage {
    HudUiWidget base;
    zVidImagePartial *variantImages[5];
    unsigned char unknown_0d0[0x08];
    zVidImagePartial *sideImageSwaps[2];
    HudUiPanelFull panel;
    HudUiWidget widget;

    void RECOIL_THISCALL Destructor();
    HudUiMessage *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    HudUiMessage *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL ReleaseImages();
    RECOIL_NOINLINE void RECOIL_THISCALL RebuildWeaponLayout();
    RECOIL_NOINLINE int RECOIL_THISCALL
    LoadWeaponLayoutFromNode(zReader::Node *layoutNode, const HudUiPanelFontParams *fontParams);
    static void RECOIL_FASTCALL SelectVariantDisplay(int messageIndex,
                                                     int variantIndex);
    static void RECOIL_FASTCALL ApplySideImageSwap(int messageIndex,
                                                   int sideIndex);
    static void RECOIL_FASTCALL ClearDisplay(int messageIndex);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    SetValueIfOwnerMatches(int messageIndex, int ownerSideIndex, float valueOrClearToken);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    UpdateSelectedWeaponDisplay(int weaponBankIndex, int weaponSideIndex,
                                float valueOrClearToken);
};

struct HudUiBarPoint {
    float x;
    float y;
    int reserved;
};

struct HudUiPolylinePoint {
    int x;
    int y;
};

struct HudUiPolyline {
    HudUiElement base;
    HudUiPolylinePoint points[21];
    int pointCount;
    int color565;
    const RECT *clipRect;

    RECOIL_NOINLINE HudUiPolyline *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Draw();
    RECOIL_NOINLINE void RECOIL_THISCALL SetPoint(int index, int x,
                                                  int y);
};

struct HudUiSliderBorder {
    HudUiPolyline base;
    int originX;
    int originY;
    int halfWidth;
    int height;
    int blinkEnabled;
    float blinkPeriodSec;
    float blinkTimeRemainingSec;
    int blinkDirSign;
    int caretHalfWidth;
    int inputActive;
    char sliderVisibleWhenInputActive;
    char rawKeyFilterEnabled;
    char unknown112[2];

    HudUiSliderBorder *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL SetBounds(int originX, int originY,
                                   int halfWidth, int height);
};

struct HudUiCounter {
    unsigned char storage[0xe0];

    HudUiCounter *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE int RECOIL_THISCALL ApplyFromLayoutNode(zReader::Node *layoutNode);
    void RECOIL_THISCALL ReleaseStateImages();
    void RECOIL_THISCALL UpdateLayoutPosition();
};

struct HudUiBar {
    const HudUiBar_FTable *ftable;
    HudUiElement *next;
    void *parent;
    unsigned int flags;
    float timer;
    int x;
    int y;
    void *bltSource;
    HudUiRect clipRect;
    unsigned short state;
    unsigned short padding32;
    HudUiBarPoint points[21];
    int drawVertexCount;
    int drawParam;
    int quadHeight;
    float quadLeftX;

    HudUiBar *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL SetPointXY(int pointIndex, float x, float y);
};

struct HudUiMeter {
    const HudUiMeter_FTable *ftable;
    HudUiElement *next;
    void *parent;
    unsigned int flags;
    float timer;
    int x;
    int y;
    void *bltSource;
    HudUiRect clipRect;
    unsigned short state;
    unsigned short padding32;
    HudUiBarPoint points[21];
    unsigned int drawVertexCount;
    unsigned int color565;
    int fillPixelsMax;
    unsigned int meterFlags;

    HudUiMeter *RECOIL_THISCALL Constructor();
    HudUiMeter *RECOIL_THISCALL ConstructorEx();
};

struct HudUiObjectiveBar {
    const HudUiBar_FTable *ftable;
    HudUiElement *next;
    void *parent;
    unsigned int flags;
    float timer;
    int x;
    int y;
    void *bltSource;
    HudUiRect clipRect;
    unsigned short state;
    unsigned short padding32;
    HudUiBarPoint points[21];
    unsigned int drawVertexCount;
    unsigned int drawParam;
    float slideRangeX;
    unsigned int chatComposeActive;
};

struct HudUiTextInput {
    const HudUiTextInput_FTable *ftable;
    char *buffer;
    unsigned int capacity;
    unsigned int cursor;
    char keyActionMap[0x100];

    HudUiTextInput *RECOIL_THISCALL Constructor(unsigned int bufferSize);
    void RECOIL_THISCALL AllocTextBuffer(unsigned int bufferSize);
    void RECOIL_THISCALL DestructorCore();
    void RECOIL_THISCALL SetContents(const char *source);
    RECOIL_NOINLINE char *RECOIL_THISCALL GetBuffer();
    void RECOIL_THISCALL SetCursorPosition(int position);
    RECOIL_NOINLINE void RECOIL_THISCALL DispatchKeyAction(int key);
    void RECOIL_THISCALL InsertCharAtCursor(int ch);
    void RECOIL_THISCALL BackspaceDeleteChar();
    void RECOIL_THISCALL DeleteCharForward();
    void RECOIL_THISCALL MoveCursorLeft();
    void RECOIL_THISCALL MoveCursorRight();
    int RECOIL_THISCALL ShiftTextRight(int count, int startPos);
    int RECOIL_THISCALL ShiftTextLeft(int count, int startPos);
};

struct HudUiOwnedTextInput {
    HudUiTextInput base;
    void *owner;

    void RECOIL_THISCALL OnAcceptNotifyOwner();
};

struct HudUiNumericTextInput {
    HudUiZrdWidget base;
    HudUiTextInput textInput;
    HudUiNumericTextInput *owner;
    HudUiSliderBorder sliderBorder;

    HudUiNumericTextInput *RECOIL_THISCALL Constructor(unsigned int maxDigits);
    HudUiNumericTextInput *RECOIL_THISCALL BaseConstructor();
    HudUiNumericTextInput *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    HudUiNumericTextInput *RECOIL_THISCALL ScalarDeletingDestructorThunk(unsigned int flags);
    void RECOIL_THISCALL Destructor();
    void RECOIL_THISCALL DestructorThunk();
    void RECOIL_THISCALL AllocTextBuffer(unsigned int bufferSize);
    char *RECOIL_THISCALL GetBuffer();
    void RECOIL_THISCALL Update(const char *text);
    RECOIL_NO_GS void RECOIL_THISCALL UpdateCaptureUiAndClip(float deltaSeconds);
    int RECOIL_THISCALL SetInputActive(int active);
    void RECOIL_THISCALL SetRawKeyboardCapture(int enable);
    int RECOIL_THISCALL OnRawKeyboardChar(int key);
    int RECOIL_THISCALL OnAcceptForwardToCommit();
    void RECOIL_THISCALL OnActivate();
    static int RECOIL_FASTCALL RawKeyboardCallback(int key,
                                                            HudUiNumericTextInput *callbackCtx);
};

struct HudUiNetGameSetupTextInput : HudUiNumericTextInput {
    void RECOIL_THISCALL OnActivateFocusAndCursor();
};

struct HudUiClampedIntTextInput : HudUiNumericTextInput {
    int minValue;
    int maxValue;

    HudUiClampedIntTextInput *RECOIL_THISCALL Constructor(unsigned int maxDigits);
    int RECOIL_THISCALL OnRawKeyboardDigitOnly(int key);
    int RECOIL_THISCALL CommitAndGetValue();
};

struct HudUiSlot {
    const HudUiSlot_FTable *ftable;
    HudUiElement *next;
    void *parent;
    unsigned int flags;
    float timer;
    unsigned char unknown_14[0x20];
    unsigned int screenEdgeCode;
    void *trackNode;
    float screenX;
    float screenY;
    unsigned char unknown_44[0x04];
    HudUiWidget slotWidget;
    HudUiWidget trackMarkerWidget;

    HudUiSlot *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Destructor();
    void RECOIL_THISCALL Draw();
    HudUiSlot *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiMgrObjectiveBlock {
    unsigned int state;
    unsigned int phase;
    float phaseTimerSec;
    float phaseDurationSec;
    float autoHideDelaySec;
    unsigned int showResetUnused;
    int objectiveWidgetRightX;
    HudUiWidget objectiveWidget;
    HudUiWidget objectiveSensorRect;
    HudUiPanel *objectiveSummaryTextPanel;
    HudUiPanel *objectiveLabelTextPanel;
    HudUiMeter objectiveMeter;
    float objectiveMeterFillAnimTimerSec;
    unsigned int objectiveMeterFillAnimEnabled;
    HudUiPanel *objectiveDescTextPanel;
    HudUiObjectiveBar objectiveBar;
    HudUiTextInput chatComposeTextInput;
    HudUiCounterTextPanel *counterTextPanel;

    void RECOIL_THISCALL Destructor();
};

struct HudUiPanel {
    const void *vtbl;
    unsigned char reserved004[0x144];
    zVidImagePartial *textPick;
    unsigned char reserved14c[0x08];
    HGDIOBJ hFont;
    unsigned char reserved158[0x14c];

    HudUiPanel *RECOIL_THISCALL ConstructorDefault(const char *text, int x,
                                                   int y);
    HudUiPanel *RECOIL_THISCALL CopyConstructCore(const HudUiPanel *source);
    HudUiPanel *RECOIL_THISCALL ConstructorCopy(const HudUiPanel *source);
    void RECOIL_THISCALL Invalidate();
    HGDIOBJ RECOIL_THISCALL GetFont();
    void RECOIL_THISCALL SetFontHandle(HGDIOBJ fontHandle);
    void RECOIL_THISCALL EnableWordWrapWithRect(const HudUiRect *rect);
    void RECOIL_THISCALL Draw();
    void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE void RECOIL_THISCALL DestructorThunk();
    static void RECOIL_STDCALL DestructorCallback(HudUiPanel *panel);
    unsigned int RECOIL_THISCALL SetTextColor(unsigned int color);
    void RECOIL_THISCALL SetTextColorsAndMarkDirty(unsigned int color0, unsigned int color1);
    unsigned int RECOIL_THISCALL SetShadow(unsigned int shadowEnabled, int shadowOffsetX,
                                            int shadowOffsetY);
    void RECOIL_THISCALL SetFont(const char *faceName, int height, int weight,
                                 int width, int italic, int charSet,
                                 int pitchAndFamily);
    void SetTextFmt(const char *format, ...);
    void RECOIL_THISCALL SetTextFmtV(const char *format, va_list args);
    void RECOIL_THISCALL SetText(const char *text);
    void RECOIL_THISCALL RebuildTextRect();
    void RECOIL_THISCALL UpdateTextBoundsFromContent();
    RECOIL_NOINLINE int RECOIL_THISCALL MeasureTextPrefixRect(int maxChars,
                                                                       RECT *outRect);
    int RECOIL_THISCALL QueryTextHeight();
    HudUiRect *RECOIL_THISCALL GetWrapRect();
    int RECOIL_THISCALL HitTest(int px, int py);
    char *RECOIL_THISCALL GetLastTextPtr();
    void RECOIL_THISCALL GetTextRect(HudUiRect *outRect);
    HudUiPanel *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};

struct HudUtil {
    void *fieldPtr;

    void RECOIL_THISCALL FreeFieldPtr();
};

struct HudUiTransitionTextPanel {
    unsigned char panelStorage[0x2a4];
    float flashCountdown;
    float flashResetValue;
    int flashAltColor0;
    int flashAltColor1;
    int flashEnabled;
    int flashMode;
    int flashDirectionSign;

    HudUiTransitionTextPanel *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL TickFlash(float deltaSeconds);
    void RECOIL_THISCALL ResetFlashState(float flashRate);
    void RECOIL_THISCALL SetFlashRate(float flashRate);
    void RECOIL_THISCALL SetFlashColorAndRate(unsigned int flashColor, float flashRate);
};

struct HudUiFlashPanel {
    static unsigned int RECOIL_FASTCALL ComputeFlashBlendColor(unsigned int color0,
                                                                unsigned int color1, float blend);
};

struct HudUiCompositePanelEntry {
    HudUiTransitionTextPanel panel;

    HudUiCompositePanelEntry *RECOIL_THISCALL AssignCopy(const HudUiCompositePanelEntry *source);
    HudUiCompositePanelEntry *RECOIL_THISCALL
    ConstructorCopy(const HudUiCompositePanelEntry *source);
    static HudUiCompositePanelEntry *RECOIL_FASTCALL ConstructorCopyRange(
        const HudUiCompositePanelEntry *sourceBegin, const HudUiCompositePanelEntry *sourceEnd,
        HudUiCompositePanelEntry *destBegin);
};

struct HudUiCompositePanelVector {
    unsigned int allocatorStorage;
    HudUiCompositePanelEntry *begin;
    HudUiCompositePanelEntry *end;
    HudUiCompositePanelEntry *capacityEnd;

    void RECOIL_THISCALL Clear();
    RECOIL_NOINLINE void RECOIL_THISCALL InsertCopies(HudUiCompositePanelEntry *insertPos,
                                                      unsigned int insertCount,
                                                      const HudUiCompositePanelEntry *templateEntry);
};

struct HudUiCompositePanel {
    unsigned char panelStorage[0x2a4];
    int activeEntryCount;
    HudUiCompositePanelVector entryVector;

    RECOIL_NOINLINE HudUiCompositePanel *RECOIL_THISCALL
    ConstructorWithEntryCount(int entryCount);
    RECOIL_NOINLINE void RECOIL_THISCALL LayoutEntries(int x, int y);
    RECOIL_NOINLINE void RECOIL_THISCALL ResizeEntryVectorAndRelayout(int entryCount);
    RECOIL_NOINLINE void RECOIL_THISCALL ReapplyEntryCount();
    RECOIL_NOINLINE void RECOIL_THISCALL ResizeEntryCount(int oldCount,
                                                          int entryCount);
    void SetTextFmt(const char *format, ...);
    RECOIL_NOINLINE void RECOIL_THISCALL SetTextFmtV(const char *format, va_list args);
    RECOIL_NOINLINE void RECOIL_THISCALL ScrollHistory();
    RECOIL_NOINLINE void RECOIL_THISCALL SetFont(const char *faceName, int height,
                                                 int weight, int width,
                                                 int italic, int charSet,
                                                 int pitchAndFamily);
};

struct HudFontStyle {
    unsigned int validMarker;
    const char *fontName;
    int fontSize;
    unsigned int textColor;
    unsigned int bkColor;
    unsigned int bkMode;
    unsigned int shadowEnabled;
    int fontWeight;
    unsigned int alignMode;

    HudFontStyle *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Destructor();
};

struct HudUiBackground {
    HudUiBackgroundContainer base;
    HudUiBackgroundCursorWidget cursorWidget;
    zVidImagePartial *primaryClipImage;
    zVidImagePartial *capturedCompositeImage;
    HudUiWidget backgroundImageWidgets[20];
    HudUiBackgroundVideoWidget backgroundVideoWidgets[10];
    HudUiBackgroundSoundEntry backgroundSounds[10];
    HudFontStyle fontStyles[20];
    HudUiTransitionTextPanel backgroundTextPanels[50];
    zReader::Node *loadedRoot;
    zReader::Node *cfgRoot;
    int uiOriginX;
    int uiOriginY;

    HudUiBackground *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Destructor();
    zReader::Node *RECOIL_THISCALL LoadFromZrd(const char *zrdPath, const char *sectionName,
                                               int capturePrimary);
    zReader::Node *RECOIL_THISCALL LoadZrdAndSection(zReader::Node *loadedRootNode,
                                                     const char *sectionName,
                                                     int capturePrimary);
    void RECOIL_THISCALL SetEnabled(int enabled);
    RECOIL_NOINLINE unsigned char RECOIL_FASTCALL
    BindButtonsNodeToWidgetByName(zReader::Node *parentNode, HudUiWidget *widget,
                                  const char *name);
    RECOIL_NOINLINE int RECOIL_THISCALL
    BindWidgetByName(zReader::Node *loadedSectionNode, HudUiWidget *widget, const char *name);
    RECOIL_NOINLINE int RECOIL_THISCALL
    BindPrimitiveNodeToElement(zReader::Node *loadedSectionNode, HudUiElement *element,
                               const char *name);
    RECOIL_NOINLINE void RECOIL_THISCALL FreeLoadedTreeRoots(int unused);
    RECOIL_NOINLINE void RECOIL_THISCALL Update(float deltaSeconds);
};

struct HudCmdSimpleWidget {
    HudUiZrdWidget base;
};

struct HudCmdDialogCallback {
    HudUiZrdWidget base;

    void RECOIL_THISCALL NextSet();
    void RECOIL_THISCALL PrevSet();
    void RECOIL_THISCALL NextCommand();
    void RECOIL_THISCALL PrevCommand();
};

struct HudCmdResetButton {
    HudUiZrdWidget base;

    void RECOIL_THISCALL OnActivate();
};

struct HudCmdSetListWidget {
    HudUiCycleSelectorWidget base;

    void RECOIL_THISCALL OnActivate();
};

struct HudCmdPromptPanel {
    HudUiTransitionTextPanel base;
};

struct HudCmdDescriptionPanel {
    HudUiPanel base;
    int captureState;
};

struct HudCmdDialog {
    HudUiBackground base;
    HudCmdSimpleWidget resumeButton;
    HudCmdResetButton resetButton;
    HudCmdCommandList commandList;
    HudCmdKeyAButton keyAButton;
    HudCmdKeyBButton keyBButton;
    HudCmdJoyButton joyButton;
    HudCmdMouseButton mouseButton;
    HudCmdSetListWidget setList;
    HudCmdDialogCallback nextSetButton;
    HudCmdDialogCallback prevSetButton;
    HudCmdDialogCallback nextCommandButton;
    HudCmdDialogCallback prevCommandButton;
    HudCmdPromptPanel promptPanel;
    HudCmdDescriptionPanel descriptionPanel;

    HudCmdDialog *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Destructor();
    HudCmdDialog *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL UpdateCaptureState(float deltaTime);
    int RECOIL_THISCALL SelectGroupRelative(int delta);
    int RECOIL_THISCALL SelectCommandRelative(int delta);
    void RECOIL_THISCALL RebuildCommandBindingListsForGroup(int groupIndex);
    void RECOIL_THISCALL OnCommandSelectionChanged(int commandIndex);
    int RECOIL_THISCALL ApplyPrimaryKeyRebind(int keyCode, int commandIndex);
    int RECOIL_THISCALL ApplySecondaryKeyRebind(int keyCode, int commandIndex);
    int RECOIL_THISCALL ApplyJoystickButtonRebind(int buttonCode, int commandIndex);
    int RECOIL_THISCALL ApplyMouseButtonRebind(int buttonCode, int commandIndex);
};

struct HudOptionsDialog;

struct HudUiOptionsPanelBackButton {
    HudUiZrdWidget base;

    void RECOIL_THISCALL OnActivate();
};

struct HudUiOptionsPanel_Lighting {
    HudUiCheckToggleWidget base;

    void RECOIL_THISCALL InitFromOptions();
    void RECOIL_THISCALL SyncFromOptions();
};

struct HudUiOptionsPanel_Perspective {
    HudUiCheckToggleWidget base;

    void RECOIL_THISCALL InitFromOptions();
    void RECOIL_THISCALL SyncFromOptions();
};

struct HudUiOptionsPanel_FullHud {
    HudUiCheckToggleWidget base;

    void RECOIL_THISCALL InitFromOptions();
};

struct HudUiOptionsPanel_ObjectDetail {
    HudUiCycleSelectorWidget base;

    void RECOIL_THISCALL InitFromOptions();
    void RECOIL_THISCALL SyncFromOptions();
};

struct HudUiOptionsPanel_TextureMemory {
    HudUiCycleSelectorWidget base;

    void RECOIL_THISCALL InitFromOptions();
    void RECOIL_THISCALL SyncFromOptions();
};

struct HudUiOptionsPanel_Effects {
    HudUiCycleSelectorWidget base;

    void RECOIL_THISCALL InitFromOptions();
    void RECOIL_THISCALL SyncFromOptions();
};

struct HudUiOptionsPanel_SoundActive {
    HudUiCheckToggleWidget base;

    void RECOIL_THISCALL InitFromOptions();
    void RECOIL_THISCALL SyncFromOptions();
};

struct HudUiOptionsPanel_SoundQuality {
    HudUiCycleSelectorWidget base;

    void RECOIL_THISCALL InitFromOptions();
    void RECOIL_THISCALL SyncFromOptions();
};

struct HudUiOptionsPanel_SoundVolume {
    HudUiFillBitmap base;

    void RECOIL_THISCALL SyncFromOptions();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiOptionsPanel_MusicEnable {
    HudUiCheckToggleWidget base;

    void RECOIL_THISCALL SyncFromOptions();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiOptionsPanel_MusicVolume {
    HudUiFillBitmap base;

    void RECOIL_THISCALL SyncFromOptions();
    void RECOIL_THISCALL OnActivate();
};

struct HudUiOptionsPanel_Resolution {
    HudUiCycleSelectorWidget base;

    void RECOIL_THISCALL SyncFromOptions();
    void RECOIL_THISCALL OnActivate();
};

struct HudOptionsDialog {
    HudUiBackground base;
    HudUiOptionsPanelBackButton backButton;
    HudUiOptionsPanel_Lighting lightingToggle;
    HudUiOptionsPanel_Perspective perspectiveToggle;
    HudUiOptionsPanel_FullHud fullHudToggle;
    HudUiOptionsPanel_ObjectDetail objectDetailSelector;
    HudUiOptionsPanel_TextureMemory textureMemorySelector;
    HudUiOptionsPanel_Effects effectsSelector;
    HudUiOptionsPanel_SoundActive soundActiveToggle;
    HudUiOptionsPanel_SoundQuality soundQualitySelector;
    HudUiOptionsPanel_SoundVolume soundVolumeWidget;
    HudUiOptionsPanel_MusicEnable musicEnableToggle;
    HudUiOptionsPanel_MusicVolume musicVolumeWidget;
    HudUiOptionsPanel_Resolution resolutionSelector;

    HudOptionsDialog *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL DestructorCore();
    HudOptionsDialog *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};

struct RecoilApp_IState_Vtbl;

struct HudCmdDialogState {
    unsigned int vftable;  // RecoilApp_IState_Vtbl*
    unsigned int m_dialog; // HudCmdDialog*

    static void RECOIL_CDECL StaticInitAndRegisterAtExit();
    static HudCmdDialogState *RECOIL_CDECL StaticInit();
    static void RECOIL_CDECL RegisterAtExit();
    static void RECOIL_CDECL AtExitDestructor();
    static void RECOIL_CDECL QueueEnter();
    HudCmdDialogState *RECOIL_THISCALL Constructor();
    int RECOIL_THISCALL OnTryBecomeCurrent();
    void RECOIL_THISCALL OnDeactivate();
    void RECOIL_THISCALL DestructorCore();
    HudCmdDialogState *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(HudCmdDialogState) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialogState, m_dialog) == 0x04);

extern RecoilApp_IState_Vtbl g_HudCmdDialogState_Vtbl;
extern HudCmdDialogState g_HudCmdDialogState;

struct HudUiMessageBoxDialog : HudUiBackground {
    zVidRect32 blitRect;
    int modalResult;
    int modalFrameCountdown;
    int fallbackWidth;
    int fallbackHeight;
    zVidImagePartial *backgroundImage;
    zVidImagePartial *okButtonNormalImage;
    zVidImagePartial *okButtonPressedImage;
    HudUiWidget backdropWidget;
    HudUiPanel messagePanel;
    HudUiPanel titlePanel;
    HudUiMessageBoxOkButton okButton;
    HudUiMessageBoxCancelButton cancelButton;

    HudUiMessageBoxDialog *RECOIL_THISCALL Constructor(const char *zrdPath,
                                                       const char *sectionName);
    HudUiMessageBoxDialog *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    void RECOIL_THISCALL Destructor();
    int RECOIL_THISCALL RunModal(const char *messageText, const char *titleText,
                                 void *modalContext = 0, float timeoutSeconds = -1.0f);
    void RECOIL_THISCALL OnOk();
    void RECOIL_THISCALL OnCancel();
};

struct HudUiPanelLayoutEntry {
    HudUiPanel panel;
    int layoutX;
    int layoutY;

    HudUiPanelLayoutEntry *RECOIL_THISCALL CopyConstruct(const HudUiPanelLayoutEntry *source);
    HudUiPanelLayoutEntry *RECOIL_THISCALL CopyAssign(const HudUiPanelLayoutEntry *source);
    static HudUiPanelLayoutEntry *RECOIL_FASTCALL
    CopyAssignRange(const HudUiPanelLayoutEntry *sourceStart,
                    const HudUiPanelLayoutEntry *sourceEnd,
                    HudUiPanelLayoutEntry *dest);
    static void RECOIL_STDCALL DestroyRange(HudUiPanelLayoutEntry *start,
                                            HudUiPanelLayoutEntry *end);
};

struct HudUiPanelSpan {
    unsigned int allocatorProxy;
    HudUiPanelLayoutEntry *begin;
    HudUiPanelLayoutEntry *end;
    HudUiPanelLayoutEntry *cap;

    void RECOIL_THISCALL Clear();
    HudUiPanelSpan *RECOIL_THISCALL CopyInit(const HudUiPanelSpan *source);
    HudUiPanelSpan *RECOIL_THISCALL CopyFrom(const HudUiPanelSpan *source);
    void RECOIL_THISCALL InsertN(HudUiPanelLayoutEntry *insertPos, unsigned int count,
                                 const HudUiPanelLayoutEntry *templatePanel);
    void RECOIL_THISCALL DestroyAndFree();
};

struct HudUiPanelSpanVec {
    unsigned int allocatorProxy;
    HudUiPanelSpan *begin;
    HudUiPanelSpan *end;
    HudUiPanelSpan *cap;

    void RECOIL_THISCALL InsertN(HudUiPanelSpan *insertPos, unsigned int count,
                                 const HudUiPanelSpan *templateSpan);
};

struct HudUiZrdScrollingText {
    HudUiZrdWidget base;
    HudUiPanelSpanVec rows;
    HudUiRect rect;
    int totalHeight;

    void RECOIL_THISCALL OnActivateResetOwnerFade();
    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL UpdateScrollPositions(float scrollProgress);
    int RECOIL_THISCALL LoadFromZrd(zReader::Node *zrdSection, void *ownerDialog);
    void RECOIL_THISCALL Destructor();
    HudUiZrdScrollingText *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiCreditsPanel {
    HudUiBackground base;
    float fadeStep;
    HudUiZrdWidget backButton;
    HudUiZrdWidget quitButton;
    HudUiZrdScrollingText creditsScreen;
    float fadeProgress;

    HudUiCreditsPanel *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL UpdateFadeAndExit(float deltaSeconds);
    void RECOIL_THISCALL Destructor();
    HudUiCreditsPanel *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiPanelSimple {
    unsigned char storage[0x2a4];

    HudUiPanelSimple *RECOIL_THISCALL Constructor(const char *text, int x, int y);
    HudUiPanelSimple *RECOIL_THISCALL ConstructorDefaultThunk();
};

struct HudUiShieldMessageWidget {
    int state;
    int viewportResetFrame;
    unsigned char unknown_08[0x04];
    HudUiRect screenRect;
    HudUiWidget widget;
    HudUiPanelSimple percentTextPanel;
    HudUiMeter meter;
    unsigned char unknown_4bc[0x08];

    static RECOIL_NOINLINE int RECOIL_STDCALL ApplyLayout(zReader::Node *layoutRoot);
    void RECOIL_THISCALL Destructor();
};

typedef HudUiShieldMessageWidget HudUiShieldMessageWidgetState;

struct HudUiTimerPanelFloat {
    unsigned char storage[0x2b0];

    HudUiTimerPanelFloat *RECOIL_THISCALL ConstructorDefault();
};

struct HudUiStringMenu {
    HudUiContainer base;
    unsigned char unknown_10[0x10];
    HudUiPanelSimple items[23];

    void RECOIL_THISCALL DestructorCore();
};

struct HudUiStatsListElement {
    HudUiElement base;
    HudUiTriplet *triplet;

    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL DestructorCore();
    HudUiStatsListElement *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiTimerPanel {
    unsigned char storage[0x2b0];

    static void RECOIL_FASTCALL SetRunning(int running);
    static void RECOIL_STDCALL SetElapsedSeconds(float seconds);
    static void RECOIL_STDCALL SetSeconds(float elapsedSeconds, float secondsStep);
    static float RECOIL_CDECL GetSeconds();
    static void RECOIL_FASTCALL ZarWriteTimerDataCallback(zZbdSectionCallbackCtx *sectionCtx,
                                                          HudUiTimerPanel *userData);
    static void RECOIL_STDCALL ZarReadTimerData(const float *buffer, int byteCount,
                                                HudUiTimerPanel *userData);
    HudUiTimerPanel *RECOIL_THISCALL ConstructorDefault();
    void RECOIL_THISCALL Update(float deltaSeconds);
    void RECOIL_THISCALL UpdateHMSFromSeconds(float seconds);
    void RECOIL_THISCALL SetTimeSeconds(int hours, int minutes,
                                        int seconds);
};

struct HudUiCounterTextPanel {
    unsigned char storage[0x2a4];

    HudUiCounterTextPanel *RECOIL_THISCALL Constructor();
};

struct HudUiScoreboardEntry {
    int playerKey;
    char displayName[0x40];
    int score;
    int lapCount;
    unsigned int playerColorPackedRgb;
};

struct HudUiTripletEntries {
    unsigned char rowInitFlag;
    unsigned char padding[3];
    HudUiScoreboardEntry *begin;
    HudUiScoreboardEntry *end;
    HudUiScoreboardEntry *cap;

    RECOIL_NOINLINE int RECOIL_THISCALL GetCount();
    RECOIL_NOINLINE static HudUiScoreboardEntry *RECOIL_STDCALL
    CopyRange(HudUiScoreboardEntry *sourceBegin, HudUiScoreboardEntry *sourceEnd,
              HudUiScoreboardEntry *dest);
    RECOIL_NOINLINE static void RECOIL_STDCALL FillN(HudUiScoreboardEntry *dest,
                                                     unsigned int count,
                                                     const HudUiScoreboardEntry *sourceValue);
};

struct HudUiTriplet {
    HudUiContainer base;
    HudUiPanel *headerPanels[3];
    HudUiPanel *rowCells[24];
    HudUiTripletEntries entries;
    int baseX;
    int baseY;
    int rowPitchY;
    int lapsColumnOffsetX;
    int killsColumnOffsetX;
    int fontSize;
    int fontWeight;
    int baseXStart;
    int baseYStart;
    int baseXEnd;
    int baseYEnd;
    int rowPitchYStart;
    int rowPitchYEnd;
    int lapsColumnOffsetXStart;
    int lapsColumnOffsetXEnd;
    int killsColumnOffsetXStart;
    int killsColumnOffsetXEnd;
    int fontSizeStart;
    int fontSizeEnd;
    int fontWeightStart;
    int fontWeightEnd;

    HudUiTriplet *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL DestructorCore();
    RECOIL_NOINLINE void RECOIL_THISCALL InterpolateLayout(float t);
    RECOIL_NOINLINE void RECOIL_THISCALL RebuildDisplay();
    RECOIL_NOINLINE void RECOIL_THISCALL AddEntry(GameNetPlayerRow *entryData);
    RECOIL_NOINLINE void RECOIL_THISCALL UpdateEntryData(GameNetPlayerRow *entryData);
    RECOIL_NOINLINE void RECOIL_THISCALL RemoveEntry(GameNetPlayerRow *entryKey);
    RECOIL_NOINLINE int RECOIL_THISCALL IsLocalPlayerFirstEntry();
};

struct HudUiTextStack4 {
    HudUiContainer base;
    unsigned char lines[4][0x2a4];

    HudUiPanel *RECOIL_THISCALL PushLine(const char *message, float duration);
    void RECOIL_THISCALL Clear();
    RECOIL_NOINLINE void RECOIL_THISCALL SetFontAll(const char *faceName, int height,
                                                    int weight, int width);
    void RECOIL_THISCALL SetTextColors(unsigned int color0, unsigned int color1);
    void RECOIL_THISCALL SetXAll(int x);
    void RECOIL_THISCALL SetYDescending(int yStart);
};

struct HudUiTopMessageStack : HudUiTextStack4 {
    HudUiTopMessageStack *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL DestructorCore();
};

struct HudUiChatMessageStack : HudUiTextStack4 {
    HudUiChatMessageStack *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL DestructorCore();
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(HudUiRect) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(HudUiElement) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, next) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, parent) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, flags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, timer) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, x) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, y) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, bltSource) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, clipRect) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(HudUiElement, state) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(HudUiCircle) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(HudUiCircle, radius) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiCircle, radiusSquared) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(HudUiCircle, color565) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutBase, layoutRect) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutBase, activeRect) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, widget1ImageDefault) == 0x1a8);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, widget1Image320) == 0x1ac);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, widget1Image400) == 0x1b0);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, widget2ImageDefault) == 0x270);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, widget2Image320) == 0x274);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, widget2Image400) == 0x278);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, reticleClipRect) == 0x338);
RECOIL_STATIC_ASSERT(offsetof(HudLayoutHW, reticleClipInitFlags) == 0x348);
RECOIL_STATIC_ASSERT(sizeof(HudLayoutHW) == 0x34c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorBlock, sensorRectScaled) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorBlock, sensorRectRaw) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorBlock, sensorPiVSrcRect) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorBlock, sensorClampHalfW) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorBlock, sensorClampHalfH) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorBlock, sensorParam) == 0x4c);
RECOIL_STATIC_ASSERT(sizeof(HudUiShieldMessageWidget) == 0x4c4);
RECOIL_STATIC_ASSERT(offsetof(HudUiShieldMessageWidget, widget) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(HudUiShieldMessageWidget, percentTextPanel) == 0xd8);
RECOIL_STATIC_ASSERT(offsetof(HudUiShieldMessageWidget, meter) == 0x37c);
RECOIL_STATIC_ASSERT(sizeof(HudUiContainer) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(HudUiContainer_FTable) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundContainer_FTable) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudUiTriplet_FTable) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudUiTextStack4_FTable) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudUiStringMenu_FTable) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiContainer, enabled) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiContainer, childHead) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiContainer, childTail) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudUiRectDirty) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(HudUiWidget, ownsImage) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiWidget, dirtyRectCount) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(HudUiWidget, image) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(HudUiWidget, imageStateWord) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(HudUiPrimitiveBindTarget, endX) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiPrimitiveBindTarget, endY) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(HudUiPrimitiveBindTarget, color565) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(HudUiWidget, bltClipRectOrNull) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(HudUiWidget, alignFlags) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(HudUiWidget, dirtyRects) == 0x4c);
RECOIL_STATIC_ASSERT(sizeof(HudUiWidget) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundCursorWidget, capturedImage) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundCursorWidget, captureEnabled) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundCursorWidget, captureSourceSelector) == 0xc4);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundCursorWidget) == 0xd0);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundVideoWidget, stream) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundVideoWidget, elapsedTimeSec) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundVideoWidget, colorKey565) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundVideoWidget, mediaPath) == 0x3e);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundVideoWidget) == 0x144);
RECOIL_STATIC_ASSERT(sizeof(HudUiTextLabel) == 0x148);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextLabel, textBuffer) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextLabel, fontHandle) == 0x134);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextLabel, centerText) == 0x138);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextLabel, centerBoundsLeft) == 0x13c);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextLabel, centerBoundsRight) == 0x140);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextLabel, alignMode) == 0x144);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelPtrVector) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelPtrVector, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelPtrVector, end) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelPtrVector, capacityEnd) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdWidget) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, originX) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, originY) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, modeOrEnabled) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, owner) == 0xc8);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, boundsRect) == 0xcc);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, defaultImage) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, disabledImage) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, rolloverImage) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, rolloverSound) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, rolloverPlayHandle) == 0xec);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, rolloverSoundScale) == 0xf0);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, activateImage) == 0xf4);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, activateSound) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, activatePlayHandle) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, activateSoundScale) == 0x100);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, disabledSound) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, disabledSoundScale) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, labelPanels) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, rolloverLabelPanels) == 0x11c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, activateLabelPanels) == 0x12c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidget, disabledLabelPanels) == 0x13c);
RECOIL_STATIC_ASSERT(sizeof(HudUiCheckToggleWidget) == 0x164);
RECOIL_STATIC_ASSERT(offsetof(HudUiCheckToggleWidget, checked) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCheckToggleWidget, disabledCheckedImage) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiCheckToggleWidget, disabledCheckedFallbackImage) == 0x154);
RECOIL_STATIC_ASSERT(offsetof(HudUiCheckToggleWidget, uncheckedImage) == 0x158);
RECOIL_STATIC_ASSERT(offsetof(HudUiCheckToggleWidget, checkedImage) == 0x15c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCheckToggleWidget, checkedLabelPanel) == 0x160);
RECOIL_STATIC_ASSERT(sizeof(HudUiCycleSelectorWidget) == 0x208);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, selectedIndex) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, itemCount) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, firstIndex) == 0x154);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, visibleCount) == 0x158);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, fontStyleRef) == 0x15c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, textOffsetX) == 0x160);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, textOffsetY) == 0x164);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, entriesA) == 0x168);
RECOIL_STATIC_ASSERT(offsetof(HudUiCycleSelectorWidget, entriesB) == 0x1b8);
RECOIL_STATIC_ASSERT(sizeof(HudUiFillBitmap) == 0x188);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, normalizedValue) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, fillImage) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, fillRect) == 0x154);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, fillOffsetX) == 0x164);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, fillOffsetY) == 0x168);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, previewImage) == 0x16c);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, previewRect) == 0x170);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, previewOffsetX) == 0x180);
RECOIL_STATIC_ASSERT(offsetof(HudUiFillBitmap, previewOffsetY) == 0x184);
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdWidgetEx17C_Item) == 0x17c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, selected) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, ownerSelector) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, itemIndex) == 0x154);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, mouseRectValid) == 0x158);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, mouseRect) == 0x15c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, selectedImage) == 0x16c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, unselectedImage) == 0x170);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, selectedRolloverImage) == 0x174);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C_Item, unselectedRolloverImage) == 0x178);
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdWidgetEx17C) == 0x17c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C, optionCount) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C, options) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdWidgetEx17C, selectedIndex) == 0x178);
RECOIL_STATIC_ASSERT(sizeof(HudUiListSelectorItem) == 0x2ac);
RECOIL_STATIC_ASSERT(offsetof(HudUiListSelectorItem, owner) == 0x2a8);
RECOIL_STATIC_ASSERT(sizeof(HudCmdBindingVector) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindingVector, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindingVector, end) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindingVector, capacity) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindingEntry, displayText) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindingEntry, commandId) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(HudCmdBindingEntry) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudCmdBindButtonBase) == 0x44c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, bindingSlotTotalCount) == 0x164);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, visibleBindingSlotCount) == 0x168);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, bindPanel) == 0x16c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, bindingSlotPanels) == 0x418);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, bindingVec) == 0x41c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, bindingSlotSpacing) == 0x42c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, selectedBindingIndex) == 0x430);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, visibleListOffsetX) == 0x434);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, visibleListOffsetY) == 0x438);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, overflowListOffsetX) == 0x43c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, overflowListOffsetY) == 0x440);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, selectedFontStyleRef) == 0x444);
RECOIL_STATIC_ASSERT(offsetof(HudCmdBindButtonBase, listFontStyleRef) == 0x448);
RECOIL_STATIC_ASSERT(sizeof(HudCmdCommandList) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdKeyAButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdKeyBButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdJoyButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdMouseButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdSimpleWidget) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdDialogCallback) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdSetListWidget) == 0x208);
RECOIL_STATIC_ASSERT(sizeof(HudCmdPromptPanel) == 0x2c0);
RECOIL_STATIC_ASSERT(sizeof(HudCmdDescriptionPanel) == 0x2a8);
RECOIL_STATIC_ASSERT(sizeof(HudCmdDialog) == 0xce00);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, resumeButton) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, resetButton) == 0xaa98);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, commandList) == 0xabe4);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, keyAButton) == 0xb030);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, keyBButton) == 0xb47c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, joyButton) == 0xb8c8);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, mouseButton) == 0xbd14);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, setList) == 0xc160);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, nextSetButton) == 0xc368);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, prevSetButton) == 0xc4b4);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, nextCommandButton) == 0xc600);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, prevCommandButton) == 0xc74c);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, promptPanel) == 0xc898);
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialog, descriptionPanel) == 0xcb58);
RECOIL_STATIC_ASSERT(sizeof(HudOptionsDialog) == 0xbec4);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, backButton) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, lightingToggle) == 0xaa98);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, perspectiveToggle) == 0xabfc);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, fullHudToggle) == 0xad60);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, objectDetailSelector) == 0xaec4);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, textureMemorySelector) == 0xb0cc);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, effectsSelector) == 0xb2d4);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, soundActiveToggle) == 0xb4dc);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, soundQualitySelector) == 0xb640);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, soundVolumeWidget) == 0xb848);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, musicEnableToggle) == 0xb9d0);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, musicVolumeWidget) == 0xbb34);
RECOIL_STATIC_ASSERT(offsetof(HudOptionsDialog, resolutionSelector) == 0xbcbc);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundContainer) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundContainer, inputFocusElement) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundContainer, captureTransitionMask) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundSoundEntry) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundSoundEntry, volume) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackgroundSoundEntry, playHandle) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiDialogController, capturedImage) == 0x114);
RECOIL_STATIC_ASSERT(sizeof(HudUiDialogController) == 0x118);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, cursorWidget) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, primaryClipImage) == 0x114);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, capturedCompositeImage) == 0x118);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, backgroundImageWidgets) == 0x11c);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, backgroundVideoWidgets) == 0xfcc);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, backgroundSounds) == 0x1c74);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, fontStyles) == 0x1cec);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, backgroundTextPanels) == 0x1fbc);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, loadedRoot) == 0xa93c);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, cfgRoot) == 0xa940);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, uiOriginX) == 0xa944);
RECOIL_STATIC_ASSERT(offsetof(HudUiBackground, uiOriginY) == 0xa948);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackground) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, blitRect) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, modalResult) == 0xa95c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, modalFrameCountdown) == 0xa960);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, fallbackWidth) == 0xa964);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, fallbackHeight) == 0xa968);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, backgroundImage) == 0xa96c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, okButtonNormalImage) == 0xa970);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, okButtonPressedImage) == 0xa974);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, backdropWidget) == 0xa978);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, messagePanel) == 0xaa34);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, titlePanel) == 0xacd8);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, okButton) == 0xaf7c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessageBoxDialog, cancelButton) == 0xb0c8);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessageBoxDialog) == 0xb214);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelLayoutEntry) == 0x2ac);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelLayoutEntry, layoutX) == 0x2a4);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelLayoutEntry, layoutY) == 0x2a8);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelSpan) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelSpan, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelSpan, end) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelSpan, cap) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelSpanVec) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelSpanVec, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelSpanVec, end) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelSpanVec, cap) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdScrollingText) == 0x170);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdScrollingText, rows) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdScrollingText, rect) == 0x15c);
RECOIL_STATIC_ASSERT(offsetof(HudUiZrdScrollingText, totalHeight) == 0x16c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCreditsPanel_FTable, SecondaryAction) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiCreditsPanel, fadeStep) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCreditsPanel, backButton) == 0xa950);
RECOIL_STATIC_ASSERT(offsetof(HudUiCreditsPanel, quitButton) == 0xaa9c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCreditsPanel, creditsScreen) == 0xabe8);
RECOIL_STATIC_ASSERT(offsetof(HudUiCreditsPanel, fadeProgress) == 0xad58);
RECOIL_STATIC_ASSERT(sizeof(HudUiCreditsPanel) == 0xad5c);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessageBoxOkButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessageBoxCancelButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudUiTransitionTextPanel) == 0x2c0);
RECOIL_STATIC_ASSERT(offsetof(HudUiTransitionTextPanel, flashCountdown) == 0x2a4);
RECOIL_STATIC_ASSERT(offsetof(HudUiTransitionTextPanel, flashResetValue) == 0x2a8);
RECOIL_STATIC_ASSERT(offsetof(HudUiTransitionTextPanel, flashAltColor0) == 0x2ac);
RECOIL_STATIC_ASSERT(offsetof(HudUiTransitionTextPanel, flashAltColor1) == 0x2b0);
RECOIL_STATIC_ASSERT(offsetof(HudUiTransitionTextPanel, flashEnabled) == 0x2b4);
RECOIL_STATIC_ASSERT(offsetof(HudUiTransitionTextPanel, flashMode) == 0x2b8);
RECOIL_STATIC_ASSERT(offsetof(HudUiTransitionTextPanel, flashDirectionSign) == 0x2bc);
RECOIL_STATIC_ASSERT(sizeof(HudUiCompositePanelEntry) == 0x2c0);
RECOIL_STATIC_ASSERT(sizeof(HudUiCompositePanelVector) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiCompositePanelVector, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiCompositePanelVector, end) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiCompositePanelVector, capacityEnd) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudUiCompositePanel, activeEntryCount) == 0x2a4);
RECOIL_STATIC_ASSERT(offsetof(HudUiCompositePanel, entryVector) == 0x2a8);
RECOIL_STATIC_ASSERT(sizeof(HudFontStyle) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, validMarker) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, fontName) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, fontSize) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, textColor) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, bkColor) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, bkMode) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, shadowEnabled) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, fontWeight) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(HudFontStyle, alignMode) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(HudUiTripletPanel) == 0x270);
RECOIL_STATIC_ASSERT(offsetof(HudUiTripletPanel, visibleCount) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiTripletPanel, items) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(HudUiNanitePanel) == sizeof(HudUiTripletPanel));
RECOIL_STATIC_ASSERT(offsetof(HudUiMessage, variantImages) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessage, sideImageSwaps) == 0xd8);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessage, panel) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelFull, layoutX) == 0x2a8);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanelFull, layoutY) == 0x2ac);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelFontParams) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiMessage, widget) == 0x390);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessage) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudUiCounter) == 0xe0);
RECOIL_STATIC_ASSERT(sizeof(HudUiBarPoint) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudUiPolylinePoint) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudUiPolyline) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(HudUiPolyline, points) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiPolyline, pointCount) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(HudUiPolyline, color565) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(HudUiPolyline, clipRect) == 0xe4);
RECOIL_STATIC_ASSERT(sizeof(HudUiSliderBorder) == 0x114);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, originX) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, originY) == 0xec);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, halfWidth) == 0xf0);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, height) == 0xf4);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, blinkEnabled) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, blinkPeriodSec) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, blinkTimeRemainingSec) == 0x100);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, blinkDirSign) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, caretHalfWidth) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, inputActive) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, sliderVisibleWhenInputActive) == 0x110);
RECOIL_STATIC_ASSERT(offsetof(HudUiSliderBorder, rawKeyFilterEnabled) == 0x111);
RECOIL_STATIC_ASSERT(sizeof(HudUiBar) == 0x140);
RECOIL_STATIC_ASSERT(offsetof(HudUiBar, points) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiBar, drawVertexCount) == 0x130);
RECOIL_STATIC_ASSERT(offsetof(HudUiBar, drawParam) == 0x134);
RECOIL_STATIC_ASSERT(offsetof(HudUiBar, quadHeight) == 0x138);
RECOIL_STATIC_ASSERT(offsetof(HudUiBar, quadLeftX) == 0x13c);
RECOIL_STATIC_ASSERT(sizeof(HudUiMeter) == 0x140);
RECOIL_STATIC_ASSERT(offsetof(HudUiMeter, points) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiMeter, drawVertexCount) == 0x130);
RECOIL_STATIC_ASSERT(offsetof(HudUiMeter, color565) == 0x134);
RECOIL_STATIC_ASSERT(offsetof(HudUiMeter, fillPixelsMax) == 0x138);
RECOIL_STATIC_ASSERT(offsetof(HudUiMeter, meterFlags) == 0x13c);
RECOIL_STATIC_ASSERT(sizeof(HudUiObjectiveBar) == 0x140);
RECOIL_STATIC_ASSERT(offsetof(HudUiObjectiveBar, points) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiObjectiveBar, drawVertexCount) == 0x130);
RECOIL_STATIC_ASSERT(offsetof(HudUiObjectiveBar, drawParam) == 0x134);
RECOIL_STATIC_ASSERT(offsetof(HudUiObjectiveBar, slideRangeX) == 0x138);
RECOIL_STATIC_ASSERT(offsetof(HudUiObjectiveBar, chatComposeActive) == 0x13c);
RECOIL_STATIC_ASSERT(sizeof(HudUiTextInput) == 0x110);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextInput, buffer) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextInput, capacity) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextInput, cursor) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextInput, keyActionMap) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(HudUiOwnedTextInput) == 0x114);
RECOIL_STATIC_ASSERT(offsetof(HudUiOwnedTextInput, owner) == 0x110);
RECOIL_STATIC_ASSERT(sizeof(HudUiNumericTextInput) == 0x374);
RECOIL_STATIC_ASSERT(offsetof(HudUiNumericTextInput, textInput) == 0x14c);
RECOIL_STATIC_ASSERT(offsetof(HudUiNumericTextInput, owner) == 0x25c);
RECOIL_STATIC_ASSERT(offsetof(HudUiNumericTextInput, sliderBorder) == 0x260);
RECOIL_STATIC_ASSERT(sizeof(HudUiClampedIntTextInput) == 0x37c);
RECOIL_STATIC_ASSERT(offsetof(HudUiClampedIntTextInput, minValue) == 0x374);
RECOIL_STATIC_ASSERT(offsetof(HudUiClampedIntTextInput, maxValue) == 0x378);
RECOIL_STATIC_ASSERT(sizeof(HudUiSlot) == 0x1c0);
RECOIL_STATIC_ASSERT(offsetof(HudUiSlot, screenEdgeCode) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(HudUiSlot, trackNode) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(HudUiSlot, screenX) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(HudUiSlot, screenY) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(HudUiSlot, slotWidget) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(HudUiSlot, trackMarkerWidget) == 0x104);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrObjectiveBlock) == 0x53c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrObjectiveBlock, objectiveWidget) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrObjectiveBlock, objectiveSensorRect) == 0xd8);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrObjectiveBlock, objectiveMeter) == 0x19c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrObjectiveBlock, objectiveBar) == 0x2e8);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrObjectiveBlock, chatComposeTextInput) == 0x428);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrObjectiveBlock, counterTextPanel) == 0x538);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanel, textPick) == 0x148);
RECOIL_STATIC_ASSERT(offsetof(HudUiPanel, hFont) == 0x154);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanel) == 0x2a4);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelSimple) == 0x2a4);
RECOIL_STATIC_ASSERT(sizeof(HudUiTimerPanelFloat) == 0x2b0);
RECOIL_STATIC_ASSERT(sizeof(HudUiStringMenu) == 0x3cdc);
RECOIL_STATIC_ASSERT(offsetof(HudUiStringMenu, items) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(HudUiStatsListElement) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(HudUiStatsListElement, triplet) == 0x34);
RECOIL_STATIC_ASSERT(sizeof(HudUiTimerPanel) == 0x2b0);
RECOIL_STATIC_ASSERT(sizeof(HudUiCounterTextPanel) == 0x2a4);
RECOIL_STATIC_ASSERT(sizeof(HudUiTripletEntries) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(HudUiScoreboardEntry) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(HudUiScoreboardEntry, displayName) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiScoreboardEntry, score) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(HudUiScoreboardEntry, lapCount) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(HudUiScoreboardEntry, playerColorPackedRgb) == 0x4c);
RECOIL_STATIC_ASSERT(sizeof(HudUiTriplet) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(HudUiTriplet, headerPanels) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiTriplet, rowCells) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(HudUiTriplet, entries) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(HudUiTriplet, lapsColumnOffsetX) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(HudUiTriplet, killsColumnOffsetX) == 0x9c);
RECOIL_STATIC_ASSERT(offsetof(HudUiTriplet, fontSize) == 0xa0);
RECOIL_STATIC_ASSERT(offsetof(HudUiTriplet, fontWeight) == 0xa4);
RECOIL_STATIC_ASSERT(sizeof(HudUiTextStack4) == 0xaa0);
RECOIL_STATIC_ASSERT(offsetof(HudUiTextStack4, lines) == 0x10);
#endif
