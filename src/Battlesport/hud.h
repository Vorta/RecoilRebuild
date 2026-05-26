#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"

enum RecoilSaveLoadDialogKind {
    RECOIL_SAVELOAD_DIALOG_LOAD = 0,
    RECOIL_SAVELOAD_DIALOG_SAVE = 1,
};

enum RecoilSaveLoadPresentationCaptureMode {
    RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED = 0,
    RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED = 1,
};

enum RecoilSaveLoadTransitionMode {
    RECOIL_SAVELOAD_MODE_STANDARD = 0,
    RECOIL_SAVELOAD_MODE_FADE = 1,
    RECOIL_SAVELOAD_MODE_QUICKLOAD = 2,
};

enum zVideoHalfResAdjustMode {
    ZVIDEO_HALFRES_ADJUST_DISABLED = 0,
    ZVIDEO_HALFRES_ADJUST_ENABLED = 1,
};

struct HudUiDialogController;
struct HudUiNewGamePanel;
struct HudUiOptionsPanel;
struct HudUiBackgroundConfirmQuit;
struct HudUiControlsDialog;
struct HudUiCheatCodeDialog;
struct zSndSample;
struct zSndPlayHandleSnapshot;

struct RecoilStateSaveLoadTransition : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiDialogController*
    RecoilSaveLoadDialogKind m_dialogKind;
    zVideoHalfResAdjustMode m_savedHalfResAdjustMode;
    RecoilSaveLoadPresentationCaptureMode m_capturePresentationMode;
    RecoilSaveLoadTransitionMode m_transitionMode;
    RecoilPtr32 m_pausedAudioSnapshot; // zSndPlayHandleSnapshot*

    static void RECOIL_FASTCALL
    QueueOpenSaveDialog(RecoilSaveLoadPresentationCaptureMode capturePresentationMode);
    static void RECOIL_FASTCALL
    QueueOpenLoadDialog(RecoilSaveLoadTransitionMode transitionMode);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateSaveLoadTransition) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateSaveLoadTransition, m_dialogKind) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateSaveLoadTransition, m_capturePresentationMode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateSaveLoadTransition, m_transitionMode) == 0x14);

extern RecoilStateSaveLoadTransition g_RecoilStateSaveLoadTransition;

struct HudUiNewGamePanelOverlayOwner : RecoilApp_IState {
    RecoilPtr32 m_panel; // HudUiNewGamePanel*

    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNewGamePanelOverlayOwner) == 0x08);

struct HudUiOptionsPanelOverlayOwner : RecoilApp_IState {
    RecoilPtr32 m_panel; // HudUiOptionsPanel*

    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiOptionsPanelOverlayOwner) == 0x08);

struct RecoilStateConfirmQuit : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiBackgroundConfirmQuit*

    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateConfirmQuit) == 0x08);

struct RecoilStateControls : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiControlsDialog*

    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateControls) == 0x08);

struct RecoilStateCheatCode : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiCheatCodeDialog*
    zVideoHalfResAdjustMode m_prevHalfResAdjustMode;
    RecoilPtr32 m_audioSnapshot; // zSndPlayHandleSnapshot*
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCheatCode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateCheatCode, m_prevHalfResAdjustMode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateCheatCode, m_audioSnapshot) == 0x0c);

extern HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;
extern HudUiOptionsPanelOverlayOwner g_HudUiOptionsPanelOverlayOwner;
extern RecoilStateConfirmQuit g_RecoilState_ConfirmQuit;
extern RecoilStateControls g_RecoilStateControls;
extern RecoilStateCheatCode g_RecoilStateCheatCode;
extern zSndSample *g_Hud_LowMeterBeepSample;
extern zSndSample *g_Hud_LowMeterLoopSample;
extern int g_Hud_LowMeterLoopActive;
extern float g_Hud_LowMeterBeepInterval;
extern float g_Hud_LowMeterNextBeepTime;

namespace HudUiCallback {
int RECOIL_CDECL QueueCheatCodeState();
}

namespace HudLowMeterLoopSound {
RECOIL_NOINLINE void RECOIL_FASTCALL SetLoopActive(int enabled);
RECOIL_NOINLINE void RECOIL_CDECL Disable();
}
