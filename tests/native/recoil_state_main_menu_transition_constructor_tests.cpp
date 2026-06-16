#include "Battlesport/hud.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <new>
#include <string.h>

namespace {
int __fastcall TestNewGamePanelVideoSurfaceStateNoOp(
    zVideo_SurfaceStatePartial *surfaceState
) {
    return surfaceState != 0 ? 1 : 0;
}
}

extern "C" int recoil_state_main_menu_transition_constructor_smoke(void) {
    char storage[sizeof(RecoilStateMainMenuTransition)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateMainMenuTransition *const state =
        new (storage) RecoilStateMainMenuTransition;

    if (state->m_mainMenuDialog != 0) {
        return 1;
    }

    if (state->m_savedHalfResAdjustMode != 0) {
        return 2;
    }

    if (state->m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        return 3;
    }

    if (state->m_deferredVideoModeIndex != ZVID_MODE_INVALID_COMPLEMENT) {
        return 4;
    }

    if (state->m_pausedAudioSnapshot != 0) {
        return 5;
    }

    return 0;
}

extern "C" int recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke(void) {
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex = ZVID_MODE_INVALID_COMPLEMENT;

    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        static_cast<zVidModeIndex>(5)
    );

    return g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
                   static_cast<zVidModeIndex>(5)
               ? 0
               : 1;
}

extern "C" int hud_ui_new_game_panel_constructor_cluster_smoke(void) {
    zOptionEntryPartial *const oldPlayerNameOption = ZOPT_PLAYER_NAME;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zOptionEntryPartial *const oldOptionListHead = g_zGame_Options_OptionListHead;
    void *const oldRawCallback = g_zInput_KbdRawEventCallback;
    void *const oldRawCallbackCtx = g_zInput_KbdRawEventCallbackCtx;
    const int oldRendererType = g_zVideo_RendererType;
    const int oldHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;

    unsigned short pixels[4] = {};
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 5;
    vmodeOption.name = vmodeName;
    vmodeOption.next = 0;
    char playerName[32] = "Ace";
    zOptionEntryPartial playerNameOption = {};
    playerNameOption.payloadOrBuffer = (int)(unsigned int)(playerName);
    playerNameOption.dataSize = sizeof(playerName);
    int difficulty = 2;
    g_zGame_Options_OptionListHead = &vmodeOption;
    ZOPT_PLAYER_NAME = &playerNameOption;
    g_zOpt_GameDifficultyOption = &difficulty;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;

    unsigned char panelStorage[sizeof(HudUiNewGamePanel)];
    memset(panelStorage, 0, sizeof(panelStorage));
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)(panelStorage);
    HudUiNewGamePanel *const returned = new (panel) HudUiNewGamePanel;
    const bool constructed =
        returned == panel &&
        panel->inputFocusElement == 0 &&
        panel->backWidget.owner == 0 &&
        panel->startWidget.owner == 0 &&
        panel->nameInput.owner == 0 &&
        panel->nameInput.textInput.owner == &panel->nameInput &&
        panel->nameInput.sliderBorder.inputActive == 1 &&
        panel->nameInput.sliderBorder.sliderVisibleWhenInputActive == 0 &&
        panel->nameInput.sliderBorder.rawKeyFilterEnabled == 0 &&
        panel->intensity.optionCount == 0 &&
        panel->intensity.options[0] == 0 &&
        panel->loadedRoot == 0 && panel->cfgRoot == 0 &&
        panel->uiOriginX == 0 && panel->uiOriginY == 0 &&
        panel->nameInput.textInput.buffer != 0 &&
        panel->nameInput.textInput.capacity == 0x100 &&
        strcmp(panel->nameInput.textInput.buffer, "Ace") == 0 &&
        panel->nameInput.textInput.cursor == 3;

    difficulty = 4;
    panel->SyncIntensityFromDifficulty();
    const bool synced = panel->intensity.selectedIndex == 4;

    strcpy(playerName, "Ranger");
    panel->nameInput.HudUiNewGamePanel_NameInput::OnActivate();
    const bool nameActivated =
        panel->nameInput.textInput.capacity == 21 &&
        strcmp(panel->nameInput.textInput.buffer, "Ranger") == 0 &&
        panel->nameInput.textInput.cursor == 6 &&
        panel->nameInput.sliderBorder.inputActive == 1 &&
        panel->nameInput.sliderBorder.sliderVisibleWhenInputActive == 1 &&
        g_zInput_KbdRawEventCallback ==
            (void *)(&HudUiNumericTextInput::RawKeyboardCallback) &&
        g_zInput_KbdRawEventCallbackCtx == &panel->nameInput;
    int nameActivationFailure = 0;
    if (!nameActivated) {
        if (panel->nameInput.textInput.capacity != 21) {
            if (panel->nameInput.textInput.capacity == 0x100) {
                nameActivationFailure = 127;
            } else if (panel->nameInput.textInput.capacity == 22) {
                nameActivationFailure = 128;
            } else {
                nameActivationFailure = 120;
            }
        } else if (strcmp(panel->nameInput.textInput.buffer, "Ranger") != 0) {
            nameActivationFailure = 121;
        } else if (panel->nameInput.textInput.cursor != 6) {
            nameActivationFailure = 122;
        } else if (panel->nameInput.sliderBorder.inputActive != 1) {
            nameActivationFailure = 123;
        } else if (panel->nameInput.sliderBorder.sliderVisibleWhenInputActive != 1) {
            nameActivationFailure = 124;
        } else if (g_zInput_KbdRawEventCallback !=
                   (void *)(&HudUiNumericTextInput::RawKeyboardCallback)) {
            nameActivationFailure = 125;
        } else if (g_zInput_KbdRawEventCallbackCtx != &panel->nameInput) {
            nameActivationFailure = 126;
        } else {
            nameActivationFailure = 12;
        }
    }
    panel->nameInput.SetRawKeyboardCapture(0);

    HudUiBackground *const noDeleteResult = panel->ScalarDeletingDestructor(0);
    const bool noDeleteScalar = noDeleteResult == panel;

    HudUiNewGamePanel *const heapPanel =
        (HudUiNewGamePanel *)(::operator new(sizeof(HudUiNewGamePanel)));
    new (heapPanel) HudUiNewGamePanel;
    HudUiBackground *const heapScalarResult = heapPanel->ScalarDeletingDestructor(1);
    const bool heapScalar = heapScalarResult == heapPanel;

    HudUiZrdWidget *const zrdWidget =
        (HudUiZrdWidget *)(::operator new(sizeof(HudUiZrdWidget)));
    zrdWidget->Constructor();
    HudUiZrdWidget *const zrdThunkResult = zrdWidget->ScalarDeletingDestructorThunk(1);
    const bool zrdThunk = zrdThunkResult == zrdWidget;

    HudUiZrdWidgetEx17C *const selector =
        (HudUiZrdWidgetEx17C *)(::operator new(sizeof(HudUiZrdWidgetEx17C)));
    selector->Constructor();
    HudUiZrdWidgetEx17C *const selectorThunkResult =
        selector->ScalarDeletingDestructorThunk(1);
    const bool selectorThunk = selectorThunkResult == selector;

    ZOPT_PLAYER_NAME = oldPlayerNameOption;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zGame_Options_OptionListHead = oldOptionListHead;
    g_zInput_KbdRawEventCallback = oldRawCallback;
    g_zInput_KbdRawEventCallbackCtx = oldRawCallbackCtx;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_UseHalfResBackbuffer = oldHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;

    if (!constructed) {
        return 10;
    }
    if (!synced) {
        return 11;
    }
    if (nameActivationFailure != 0) {
        return nameActivationFailure;
    }
    if (!noDeleteScalar) {
        return 13;
    }
    if (!heapScalar) {
        return 14;
    }
    if (!zrdThunk) {
        return 15;
    }
    if (!selectorThunk) {
        return 16;
    }

    return 0;
}

extern "C" int hud_ui_new_game_panel_overlay_owner_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    HudUiNewGamePanelOverlayOwner::QueueEnter();
    return g_RecoilApp.m_stateQueue.m_itemCount == oldCount + 1 ? 0 : 1;
}

extern "C" int hud_ui_new_game_panel_overlay_owner_on_try_become_current_smoke(void) {
    zOptionEntryPartial *const oldPlayerNameOption = ZOPT_PLAYER_NAME;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zOptionEntryPartial *const oldOptionListHead = g_zGame_Options_OptionListHead;
    void *const oldRawCallback = g_zInput_KbdRawEventCallback;
    void *const oldRawCallbackCtx = g_zInput_KbdRawEventCallbackCtx;
    const int oldRendererType = g_zVideo_RendererType;
    const int oldHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;

    unsigned short pixels[4] = {};
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 5;
    vmodeOption.name = vmodeName;
    vmodeOption.next = 0;
    char playerName[32] = "Ace";
    zOptionEntryPartial playerNameOption = {};
    playerNameOption.payloadOrBuffer = (int)(unsigned int)(playerName);
    playerNameOption.dataSize = sizeof(playerName);
    int difficulty = 3;
    g_zGame_Options_OptionListHead = &vmodeOption;
    ZOPT_PLAYER_NAME = &playerNameOption;
    g_zOpt_GameDifficultyOption = &difficulty;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;

    HudUiNewGamePanelOverlayOwner state;
    const int accepted = state.OnTryBecomeCurrent();
    HudUiNewGamePanel *const panel = state.m_panel;
    const bool ok =
        accepted == 1 &&
        panel != 0 &&
        panel->enabled == 1 &&
        panel->intensity.selectedIndex == 3 &&
        panel->nameInput.textInput.buffer != 0 &&
        strcmp(panel->nameInput.textInput.buffer, "Ace") == 0;

    if (panel != 0) {
        panel->ScalarDeletingDestructor(1);
        state.m_panel = 0;
    }

    ZOPT_PLAYER_NAME = oldPlayerNameOption;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zGame_Options_OptionListHead = oldOptionListHead;
    g_zInput_KbdRawEventCallback = oldRawCallback;
    g_zInput_KbdRawEventCallbackCtx = oldRawCallbackCtx;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_UseHalfResBackbuffer = oldHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;

    return ok ? 0 : 1;
}

extern "C" int hud_ui_new_game_panel_overlay_owner_lifecycle_smoke(void) {
    zOptionEntryPartial *const oldPlayerNameOption = ZOPT_PLAYER_NAME;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zOptionEntryPartial *const oldOptionListHead = g_zGame_Options_OptionListHead;
    void *const oldRawCallback = g_zInput_KbdRawEventCallback;
    void *const oldRawCallbackCtx = g_zInput_KbdRawEventCallbackCtx;
    const int oldRendererType = g_zVideo_RendererType;
    const int oldHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;

    unsigned short pixels[4] = {};
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 5;
    vmodeOption.name = vmodeName;
    vmodeOption.next = 0;
    char playerName[32] = "Ace";
    zOptionEntryPartial playerNameOption = {};
    playerNameOption.payloadOrBuffer = (int)(unsigned int)(playerName);
    playerNameOption.dataSize = sizeof(playerName);
    int difficulty = 1;
    g_zGame_Options_OptionListHead = &vmodeOption;
    ZOPT_PLAYER_NAME = &playerNameOption;
    g_zOpt_GameDifficultyOption = &difficulty;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;

    g_HudUiNewGamePanelOverlayOwner.m_panel = (HudUiNewGamePanel *)0x22222222;
    HudUiNewGamePanelOverlayOwner *const staticInitReturned =
        HudUiNewGamePanelOverlayOwner::StaticInit();
    const bool staticInitOk =
        staticInitReturned == &g_HudUiNewGamePanelOverlayOwner &&
        g_HudUiNewGamePanelOverlayOwner.m_panel == 0;

    HudUiNewGamePanel *const atExitPanel =
        (HudUiNewGamePanel *)(::operator new(sizeof(HudUiNewGamePanel)));
    new (atExitPanel) HudUiNewGamePanel;
    atExitPanel->SetEnabled(1);
    g_HudUiNewGamePanelOverlayOwner.m_panel = atExitPanel;
    HudUiNewGamePanelOverlayOwner::AtExitDestructor();
    const bool atExitOk = g_HudUiNewGamePanelOverlayOwner.m_panel == 0;

    HudUiNewGamePanel *const destructorPanel =
        (HudUiNewGamePanel *)(::operator new(sizeof(HudUiNewGamePanel)));
    new (destructorPanel) HudUiNewGamePanel;
    destructorPanel->SetEnabled(1);
    HudUiNewGamePanelOverlayOwner state;
    state.m_panel = destructorPanel;
    state.~HudUiNewGamePanelOverlayOwner();
    const bool destructorOk = state.m_panel == 0;

    HudUiNewGamePanelOverlayOwner::RegisterAtExit();

    g_HudUiNewGamePanelOverlayOwner.m_panel = (HudUiNewGamePanel *)0x77777777;
    HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit();
    const bool staticInitRegisterOk = g_HudUiNewGamePanelOverlayOwner.m_panel == 0;

    ZOPT_PLAYER_NAME = oldPlayerNameOption;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zGame_Options_OptionListHead = oldOptionListHead;
    g_zInput_KbdRawEventCallback = oldRawCallback;
    g_zInput_KbdRawEventCallbackCtx = oldRawCallbackCtx;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_UseHalfResBackbuffer = oldHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;

    return staticInitOk && atExitOk && destructorOk && staticInitRegisterOk ? 0 : 1;
}

extern "C" int recoil_state_cheat_code_constructor_smoke(void) {
    char storage[sizeof(RecoilStateCheatCode)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateCheatCode *const rawState =
        reinterpret_cast<RecoilStateCheatCode *>(storage);
    rawState->m_prevHalfResAdjustMode = ZVIDEO_HALFRES_ADJUST_ENABLED;
    rawState->m_audioSnapshot = 0x33333333;

    RecoilStateCheatCode *const state = new (storage) RecoilStateCheatCode;

    if (state->m_dialog != 0) {
        return 1;
    }

    if (state->m_prevHalfResAdjustMode != ZVIDEO_HALFRES_ADJUST_ENABLED) {
        return 2;
    }

    if (state->m_audioSnapshot != 0x33333333) {
        return 3;
    }

    return 0;
}

extern "C" int recoil_state_controls_lifecycle_smoke(void) {
    char storage[sizeof(RecoilStateControls)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateControls *const state = new (storage) RecoilStateControls;
    if (state->m_dialog != 0) {
        return 1;
    }

    state->~RecoilStateControls();
    return state->m_dialog == 0 ? 0 : 2;
}

extern "C" int recoil_state_controls_activation_smoke(void) {
    RecoilStateControls state;
    return state.m_dialog == 0 ? 0 : 1;
}

extern "C" int recoil_state_controls_on_resume_smoke(void) {
    RecoilStateControls state;
    state.OnResume(1);
    return state.m_dialog == 0 ? 0 : 1;
}

extern "C" int recoil_state_controls_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    RecoilStateControls::QueueEnter();
    return g_RecoilApp.m_stateQueue.m_itemCount == oldCount + 1 ? 0 : 1;
}

extern "C" int recoil_state_confirm_quit_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    RecoilStateConfirmQuit::QueueEnter();
    return g_RecoilApp.m_stateQueue.m_itemCount == oldCount + 1 ? 0 : 1;
}

extern "C" int recoil_state_confirm_quit_destructor_smoke(void) {
    char storage[sizeof(RecoilStateConfirmQuit)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateConfirmQuit *const state = new (storage) RecoilStateConfirmQuit;
    if (state->m_dialog != 0) {
        return 1;
    }

    state->~RecoilStateConfirmQuit();
    return state->m_dialog == 0 ? 0 : 2;
}
