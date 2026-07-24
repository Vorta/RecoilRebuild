#include "Battlesport/hud.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/recoil_state_main_menu_transition.h"

#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <new>

namespace {
struct TestCurrentState : RecoilApp_IState {
    int exitCalls;

    TestCurrentState() : exitCalls(0) {}
    void OnExit() {
        ++exitCalls;
    }
};

void DestroyTestQueue(RecoilApp_StateQueue &queue) {
    if (queue.m_itemCount != 0 && queue.m_readBlock.m_cursor != 0) {
        delete *queue.m_readBlock.m_cursor;
    }

    if (queue.m_chunkBaseList != 0) {
        RecoilApp_StateQueueItem ***slot = queue.m_readBlock.m_chunkBaseSlot;
        RecoilApp_StateQueueItem ***const last = queue.m_writeBlock.m_chunkBaseSlot;
        while (slot != 0 && slot <= last) {
            ::operator delete(*slot);
            ++slot;
        }
        ::operator delete(queue.m_chunkBaseList);
    }
    queue = RecoilApp_StateQueue{};
}

struct TestAppQueueScope {
    RecoilApp_StateQueue savedQueue;
    int savedCurrentStateIndex;
    RecoilApp_IState *savedState0;
    TestCurrentState currentState;

    TestAppQueueScope()
        : savedQueue(g_RecoilApp.m_stateQueue),
          savedCurrentStateIndex(g_RecoilApp.m_currentStateIndex),
          savedState0(g_RecoilApp.m_stateStack[0]) {
        g_RecoilApp.m_stateQueue = RecoilApp_StateQueue{};
        g_RecoilApp.m_currentStateIndex = 0;
        g_RecoilApp.m_stateStack[0] = &currentState;
    }

    ~TestAppQueueScope() {
        DestroyTestQueue(g_RecoilApp.m_stateQueue);
        g_RecoilApp.m_stateQueue = savedQueue;
        g_RecoilApp.m_currentStateIndex = savedCurrentStateIndex;
        g_RecoilApp.m_stateStack[0] = savedState0;
    }

    RecoilApp_StateQueueItem *OnlyItem() const {
        const RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
        return queue.m_itemCount == 1 && queue.m_readBlock.m_cursor != 0
                   ? *queue.m_readBlock.m_cursor
                   : 0;
    }
};

bool IsQueueItem(
    RecoilApp_StateQueueItem *item,
    RecoilApp_StateQueueKind kind,
    const void *state,
    int param = 0
) {
    return item != 0 &&
           item->m_type == 0 &&
           item->m_kind == kind &&
           reinterpret_cast<const void *>(item->m_stateObj) == state &&
           item->m_param == param;
}

template <typename Button>
bool ActivatedImage(Button &button, zVidImagePartial *activateImage) {
    return button.image == activateImage;
}

int __fastcall TestSurfaceStateNoOp(zVideo_SurfaceStatePartial *) {
    return 0;
}

struct MainMenuDialogEnvironment {
    int rendererPath;
    zVideo_BltRectDirectProc bltDirect;
    zVideo_SurfaceStatePartial swSurface;
    zVideo_SurfaceStatePartial primarySurface;
    zVideo_SurfaceStatePartial displaySurface;
    zVideo_SurfaceStateProc lockSurface;
    zVideo_SurfaceStateProc unlockSurface;
    int *networkEnabledOption;
    zInput_GameStateOrMapTablePartial *gameState;
    int networkEnabled;

    MainMenuDialogEnvironment()
        : rendererPath(g_zVideo_ActiveRendererPath),
          bltDirect(g_zVideo_pfnBltSwToPrimaryRectDirect),
          swSurface(g_zVideo_SwSurfaceState),
          primarySurface(g_zVideo_PrimarySurfaceState),
          displaySurface(g_zVideo_DisplayModeSurfaceState),
          lockSurface(g_zVideo_pfnLockSurfaceState),
          unlockSurface(g_zVideo_pfnUnlockSurfaceState),
          networkEnabledOption(g_zGame_Options_PointerCache.networkEnabled),
          gameState(g_GameStateOrMapTable),
          networkEnabled(0) {
        g_zVideo_ActiveRendererPath = 0;
        g_zVideo_pfnBltSwToPrimaryRectDirect = 0;
        g_zVideo_pfnLockSurfaceState = TestSurfaceStateNoOp;
        g_zVideo_pfnUnlockSurfaceState = TestSurfaceStateNoOp;
        g_zVideo_SwSurfaceState = zVideo_SurfaceStatePartial{};
        g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial{};
        g_zVideo_DisplayModeSurfaceState = zVideo_SurfaceStatePartial{};
        g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
        g_GameStateOrMapTable = 0;
    }

    ~MainMenuDialogEnvironment() {
        g_GameStateOrMapTable = gameState;
        g_zGame_Options_PointerCache.networkEnabled = networkEnabledOption;
        g_zVideo_ActiveRendererPath = rendererPath;
        g_zVideo_pfnBltSwToPrimaryRectDirect = bltDirect;
        g_zVideo_pfnLockSurfaceState = lockSurface;
        g_zVideo_pfnUnlockSurfaceState = unlockSurface;
        g_zVideo_SwSurfaceState = swSurface;
        g_zVideo_PrimarySurfaceState = primarySurface;
        g_zVideo_DisplayModeSurfaceState = displaySurface;
    }
};

int g_layoutActivatedCalls;

struct TestLayout : HudLayoutBase {
    void OnActivated() {
        ++g_layoutActivatedCalls;
    }
};
} // namespace

extern "C" int hud_ui_main_menu_dialog_destructor_smoke(void) {
    MainMenuDialogEnvironment environment;
    HudUiMainMenuDialog *const dialog =
        new HudUiMainMenuDialog(RECOIL_MAINMENU_ROUTE_FRONTEND);
    if (dialog == 0) {
        return 1;
    }
    delete dialog;
    return 0;
}

extern "C" int hud_ui_main_menu_credits_button_on_activate_smoke(void) {
    TestAppQueueScope app;
    HudUiMainMenuDialog_CreditsButton button;
    zVidImagePartial image = {};
    button.activateImage = &image;
    button.OnActivate();
    return IsQueueItem(
               app.OnlyItem(),
               RecoilApp_StateQueueKind_PushState,
               &g_RecoilStateCredits
           ) &&
                   ActivatedImage(button, &image)
               ? 0
               : 1;
}

extern "C" int hud_ui_main_menu_save_button_on_activate_smoke(void) {
    TestAppQueueScope app;
    const RecoilSaveLoadDialogKind savedKind =
        g_RecoilStateSaveLoadTransition.m_dialogKind;
    const RecoilSaveLoadPresentationCaptureMode savedCapture =
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode;

    HudUiMainMenuDialog_SaveButton button;
    zVidImagePartial image = {};
    button.activateImage = &image;
    button.OnActivate();
    const bool ok =
        IsQueueItem(
            app.OnlyItem(),
            RecoilApp_StateQueueKind_PushState,
            &g_RecoilStateSaveLoadTransition
        ) &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE &&
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode ==
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED &&
        ActivatedImage(button, &image);

    g_RecoilStateSaveLoadTransition.m_dialogKind = savedKind;
    g_RecoilStateSaveLoadTransition.m_capturePresentationMode = savedCapture;
    return ok ? 0 : 1;
}

extern "C" int hud_ui_main_menu_load_button_on_activate_smoke(void) {
    const RecoilMainMenuEntryRoute savedRoute =
        g_RecoilState_MainMenuTransition.m_entryRoute;
    const RecoilSaveLoadDialogKind savedKind =
        g_RecoilStateSaveLoadTransition.m_dialogKind;
    const RecoilSaveLoadTransitionMode savedMode =
        g_RecoilStateSaveLoadTransition.m_transitionMode;

    bool frontendOk = false;
    {
        TestAppQueueScope app;
        g_RecoilState_MainMenuTransition.m_entryRoute =
            RECOIL_MAINMENU_ROUTE_FRONTEND;
        HudUiMainMenuDialog_LoadButton button;
        zVidImagePartial image = {};
        button.activateImage = &image;
        button.OnActivate();
        frontendOk =
            IsQueueItem(
                app.OnlyItem(),
                RecoilApp_StateQueueKind_PushState,
                &g_RecoilStateSaveLoadTransition
            ) &&
            g_RecoilStateSaveLoadTransition.m_dialogKind ==
                RECOIL_SAVELOAD_DIALOG_LOAD &&
            g_RecoilStateSaveLoadTransition.m_transitionMode ==
                RECOIL_SAVELOAD_MODE_STANDARD &&
            ActivatedImage(button, &image);
    }

    bool ingameOk = false;
    {
        TestAppQueueScope app;
        g_RecoilState_MainMenuTransition.m_entryRoute =
            RECOIL_MAINMENU_ROUTE_INGAME;
        HudUiMainMenuDialog_LoadButton button;
        button.OnActivate();
        ingameOk =
            IsQueueItem(
                app.OnlyItem(),
                RecoilApp_StateQueueKind_PushState,
                &g_RecoilStateSaveLoadTransition
            ) &&
            g_RecoilStateSaveLoadTransition.m_transitionMode ==
                RECOIL_SAVELOAD_MODE_FADE;
    }

    g_RecoilState_MainMenuTransition.m_entryRoute = savedRoute;
    g_RecoilStateSaveLoadTransition.m_dialogKind = savedKind;
    g_RecoilStateSaveLoadTransition.m_transitionMode = savedMode;
    return frontendOk && ingameOk ? 0 : 1;
}

extern "C" int hud_ui_main_menu_new_game_button_on_activate_smoke(void) {
    TestAppQueueScope app;
    HudUiMainMenuDialog_NewGameButton button;
    zVidImagePartial image = {};
    button.activateImage = &image;
    button.OnActivate();
    return IsQueueItem(
               app.OnlyItem(),
               RecoilApp_StateQueueKind_PushState,
               &g_HudUiNewGamePanelOverlayOwner
           ) &&
                   ActivatedImage(button, &image)
               ? 0
               : 1;
}

extern "C" int hud_ui_menu_back_button_on_activate_smoke(void) {
    HudLayoutBase *const savedLayout = g_HudUiMgrCurrentLayout;
    TestAppQueueScope app;
    TestLayout layout;
    g_layoutActivatedCalls = 0;
    g_HudUiMgrCurrentLayout = &layout;

    HudUiMenuBackButton button;
    zVidImagePartial image = {};
    button.activateImage = &image;
    button.OnActivate();
    const bool ok =
        IsQueueItem(
            app.OnlyItem(),
            RecoilApp_StateQueueKind_ExitCurrent,
            0
        ) &&
        app.currentState.exitCalls == 1 &&
        g_layoutActivatedCalls == 1 &&
        ActivatedImage(button, &image);

    g_HudUiMgrCurrentLayout = savedLayout;
    return ok ? 0 : 1;
}

extern "C" int hud_ui_main_menu_options_button_on_activate_smoke(void) {
    TestAppQueueScope app;
    HudUiMainMenuDialog_OptionsButton button;
    zVidImagePartial image = {};
    button.activateImage = &image;
    button.OnActivate();
    return IsQueueItem(
               app.OnlyItem(),
               RecoilApp_StateQueueKind_PushState,
               &g_HudUiOptionsPanelOverlayOwner
           ) &&
                   ActivatedImage(button, &image)
               ? 0
               : 1;
}

extern "C" int hud_ui_main_menu_quit_button_on_activate_smoke(void) {
    TestAppQueueScope app;
    HudUiMainMenuDialog_QuitButton button;
    zVidImagePartial image = {};
    button.activateImage = &image;
    button.OnActivate();
    return IsQueueItem(
               app.OnlyItem(),
               RecoilApp_StateQueueKind_PushState,
               &g_RecoilState_ConfirmQuit
           ) &&
                   ActivatedImage(button, &image)
               ? 0
               : 1;
}

extern "C" int hud_ui_main_menu_controls_button_on_activate_smoke(void) {
    TestAppQueueScope app;
    HudUiMainMenuDialog_ControlsButton button;
    zVidImagePartial image = {};
    button.activateImage = &image;
    button.OnActivate();
    return IsQueueItem(
               app.OnlyItem(),
               RecoilApp_StateQueueKind_PushState,
               &g_RecoilStateControls
           ) &&
                   ActivatedImage(button, &image)
               ? 0
               : 1;
}

extern "C" int recoil_state_main_menu_transition_scalar_deleting_destructor_smoke(void) {
    RecoilStateMainMenuTransition *const state =
        new RecoilStateMainMenuTransition;
    if (state == 0 || state->m_mainMenuDialog != 0) {
        delete state;
        return 1;
    }

    /*
     * The scalar-deleting wrapper is compiler output in the current source
     * model. Exercise the source-level lifecycle that generates it.
     */
    delete state;
    return 0;
}
