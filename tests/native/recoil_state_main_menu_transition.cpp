#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include <cstring>
#include <new>

#include "Battlesport/hud.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"

extern "C" int g_RecoilState_MainMenuSkipExitDelay;

extern "C" int(RECOIL_FASTCALL *g_zVideo_pfnLockSurfaceState)(
    zVideo_SurfaceStatePartial *surfaceState);
extern "C" int(RECOIL_FASTCALL *g_zVideo_pfnUnlockSurfaceState)(
    zVideo_SurfaceStatePartial *surfaceState);

struct RecoilStateCredits {
    RecoilPtr32 vftable;

    static void RECOIL_CDECL QueuePush();
};

extern RecoilStateCredits g_RecoilStateCredits;

namespace {
int g_queueEnterOnEnterCalls;
int g_queueEnterOnExitCalls;
int g_mainMenuLayoutActivatedCount;

struct TestQueueEnterState : RecoilApp_IState {
    void RECOIL_THISCALL OnEnter() {
        ++g_queueEnterOnEnterCalls;
    }

    void RECOIL_THISCALL OnExit() {
        ++g_queueEnterOnExitCalls;
    }
};

RecoilFn32 QueueEnterMethodToFn(void (RECOIL_THISCALL TestQueueEnterState::*method)()) {
    union MemberToFunction {
        void (RECOIL_THISCALL TestQueueEnterState::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilApp_IState_Vtbl MakeQueueEnterVtable() {
    RecoilApp_IState_Vtbl vtable{};
    vtable.OnEnter = QueueEnterMethodToFn(&TestQueueEnterState::OnEnter);
    vtable.OnExit = QueueEnterMethodToFn(&TestQueueEnterState::OnExit);
    return vtable;
}

RecoilApp_IState_Vtbl g_queueEnterVtable = MakeQueueEnterVtable();

void RECOIL_FASTCALL TestMainMenuLayoutOnActivated(HudLayoutBase *) {
    ++g_mainMenuLayoutActivatedCount;
}

int RECOIL_FASTCALL TestVideoSurfaceStateNoOp(zVideo_SurfaceStatePartial *surfaceState) {
    (void)surfaceState;
    return 0;
}

void CleanupGlobalAppQueue() {
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    auto *const chunkList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    auto *const chunk = reinterpret_cast<void *>(static_cast<std::uintptr_t>(chunkList[1]));
    ::operator delete(item);
    ::operator delete(chunk);
    ::operator delete(chunkList);
    queue = {};
}

zSndPlayHandleSnapshot *NewEmptySnapshot() {
    auto *const snapshot =
        static_cast<zSndPlayHandleSnapshot *>(::operator new(sizeof(zSndPlayHandleSnapshot)));
    auto *const listHead = static_cast<zSndPlayHandleSnapshotItem *>(
        ::operator new(sizeof(zSndPlayHandleSnapshotItem)));
    listHead->next = listHead;
    listHead->prev = listHead;
    snapshot->backendTag = 0;
    snapshot->listHead = listHead;
    snapshot->itemCount = 0;
    return snapshot;
}
} // namespace

extern "C" int recoil_state_main_menu_transition_constructor_smoke(void) {
    RecoilStateMainMenuTransition state;
    std::memset(&state, 0xcc, sizeof(state));

    RecoilStateMainMenuTransition *const returned = state.Constructor();
    if (returned != &state) {
        return 1;
    }

    if (state.vftable != kRecoilStateMainMenuTransition_VtblAddress) {
        return 2;
    }

    if (state.m_mainMenuDialog != 0 || state.m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND ||
        state.m_deferredVideoModeIndex != ZVID_MODE_INVALID_COMPLEMENT ||
        state.m_pausedAudioSnapshot != 0) {
        return 3;
    }

    if (state.m_savedHalfResAdjustMode != static_cast<int>(0xcccccccc)) {
        return 4;
    }

    return 0;
}

extern "C" int recoil_state_main_menu_transition_clear_paused_audio_snapshot_smoke(void) {
    zSndPlayHandleSnapshot *const snapshot = NewEmptySnapshot();
    g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot =
        reinterpret_cast<RecoilPtr32>(snapshot);

    RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();

    if (g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot != 0) {
        return 1;
    }

    RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
    return 0;
}

extern "C" int recoil_state_main_menu_transition_queue_enter_smoke(void) {
    g_queueEnterOnEnterCalls = 0;
    g_RecoilApp.m_stateQueue_118 = {};
    g_RecoilApp.m_currentStateIndex_0c8 = -1;
    g_RecoilState_MainMenuTransition.m_entryRoute = RECOIL_MAINMENU_ROUTE_FRONTEND;
    g_RecoilState_MainMenuTransition.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));

    RecoilStateMainMenuTransition::QueueEnter(static_cast<RecoilMainMenuEntryRoute>(7));

    if (g_RecoilState_MainMenuTransition.m_entryRoute != static_cast<RecoilMainMenuEntryRoute>(7)) {
        return 1;
    }

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    if (queue.m_itemCount != 1 || g_queueEnterOnEnterCalls != 1) {
        return 2;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    const bool itemOk =
        item->m_kind == RecoilApp_StateQueueKind_PushState &&
        item->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                                &g_RecoilState_MainMenuTransition)) &&
        item->m_param == 0;
    CleanupGlobalAppQueue();

    if (!itemOk) {
        return 3;
    }

    return 0;
}

extern "C" int recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke(void) {
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex = ZVID_MODE_INVALID_COMPLEMENT;

    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(static_cast<zVidModeIndex>(5));

    return g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
                   static_cast<zVidModeIndex>(5)
               ? 0
               : 1;
}

extern "C" int hud_ui_main_menu_dialog_constructor_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;

    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};

    int networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = nullptr;

    HudUiMainMenuDialog frontendDialog{};
    HudUiMainMenuDialog *const frontendResult =
        frontendDialog.Constructor(RECOIL_MAINMENU_ROUTE_FRONTEND);
    const bool frontendConstructed =
        frontendResult == &frontendDialog &&
        frontendDialog.base.base.base.vptr !=
            reinterpret_cast<const HudUiContainer_FTable *>(&g_HudUiBackground_FTable) &&
        frontendDialog.base.base.base.enabled == 0 &&
        frontendDialog.base.base.captureTransitionMask == 1 &&
        frontendDialog.creditsButton.base.ftable != nullptr &&
        frontendDialog.backButton.base.ftable != nullptr &&
        frontendDialog.saveGameButton.base.ftable != nullptr &&
        frontendDialog.loadGameButton.base.ftable != nullptr &&
        frontendDialog.newGameButton.base.ftable != nullptr &&
        frontendDialog.optionsButton.base.ftable != nullptr &&
        frontendDialog.quitButton.base.ftable != nullptr &&
        frontendDialog.controlsButton.base.ftable != nullptr &&
        frontendDialog.creditsButton.base.ftable != frontendDialog.backButton.base.ftable &&
        frontendDialog.loadGameButton.modeOrEnabled == 1;

    zUtil_PlayerStateStorage playerState{};
    zInput_GameStateOrMapTablePartial gameState{};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    playerState.lifecycleState = 4;
    HudUiMainMenuDialog resumeDialog{};
    HudUiMainMenuDialog *const resumeResult =
        resumeDialog.Constructor(RECOIL_MAINMENU_ROUTE_INGAME);
    const bool resumeConstructed =
        resumeResult == &resumeDialog && resumeDialog.saveGameButton.modeOrEnabled == 1 &&
        resumeDialog.loadGameButton.modeOrEnabled == 1 &&
        resumeDialog.quitButton.base.ftable != nullptr;

    playerState.lifecycleState = 3;
    *reinterpret_cast<int *>(playerState.bytes + 0x25c) = 1;
    HudUiMainMenuDialog blockedDialog{};
    HudUiMainMenuDialog *const blockedResult =
        blockedDialog.Constructor(RECOIL_MAINMENU_ROUTE_INGAME);
    const bool blockedConstructed =
        blockedResult == &blockedDialog && blockedDialog.saveGameButton.modeOrEnabled == 0 &&
        blockedDialog.loadGameButton.modeOrEnabled == 0;

    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    if (!frontendConstructed) {
        return 1;
    }
    if (!resumeConstructed) {
        return 2;
    }
    if (!blockedConstructed) {
        return 3;
    }
    return 0;
}

extern "C" int hud_ui_main_menu_credits_button_on_activate_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    RecoilApp oldApp = g_RecoilApp;
    const RecoilPtr32 oldCreditsVtable = g_RecoilStateCredits.vftable;

    int networkEnabled = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = nullptr;

    TestQueueEnterState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_RecoilStateCredits.vftable = oldState.vftable;
    g_queueEnterOnEnterCalls = 0;

    g_RecoilApp = {};
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    HudUiMainMenuDialog dialog{};
    dialog.Constructor(RECOIL_MAINMENU_ROUTE_FRONTEND);

    zVidImagePartial activateImage{};
    dialog.creditsButton.activateImage = &activateImage;

    typedef void(RECOIL_THISCALL *ActivateFn)(HudUiZrdWidget * self);
    ((ActivateFn)(dialog.creditsButton.base.ftable->slots[12]))(&dialog.creditsButton);

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
                 item->m_stateObj ==
                     static_cast<RecoilPtr32>(
                         reinterpret_cast<std::uintptr_t>(&g_RecoilStateCredits)) &&
                 item->m_param == 0;
        CleanupGlobalAppQueue();
    }

    const bool activated =
        dialog.creditsButton.base.image == &activateImage && g_queueEnterOnEnterCalls == 1;

    g_RecoilStateCredits.vftable = oldCreditsVtable;
    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    return itemOk && activated ? 0 : 1;
}

extern "C" int hud_ui_main_menu_save_button_on_activate_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    RecoilApp oldApp = g_RecoilApp;
    const RecoilStateSaveLoadTransition oldTransition = g_RecoilStateSaveLoadTransition;

    int networkEnabled = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    zUtil_PlayerStateStorage playerState{};
    zInput_GameStateOrMapTablePartial gameState{};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    g_RecoilStateSaveLoadTransition = RecoilStateSaveLoadTransition{};
    g_RecoilStateSaveLoadTransition.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_queueEnterOnEnterCalls = 0;

    TestQueueEnterState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_RecoilApp = {};
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    HudUiMainMenuDialog dialog{};
    dialog.Constructor(RECOIL_MAINMENU_ROUTE_INGAME);

    zVidImagePartial activateImage{};
    dialog.saveGameButton.activateImage = &activateImage;

    typedef void(RECOIL_THISCALL *ActivateFn)(HudUiZrdWidget * self);
    ((ActivateFn)(dialog.saveGameButton.base.ftable->slots[12]))(&dialog.saveGameButton);

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
                 item->m_stateObj ==
                     static_cast<RecoilPtr32>(
                         reinterpret_cast<std::uintptr_t>(&g_RecoilStateSaveLoadTransition)) &&
                 item->m_param == 0;
        CleanupGlobalAppQueue();
    }

    const bool saved =
        g_queueEnterOnEnterCalls == 1 &&
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode ==
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE;
    const bool activated = dialog.saveGameButton.base.image == &activateImage;

    g_RecoilStateSaveLoadTransition = oldTransition;
    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    return itemOk && saved && activated ? 0 : 1;
}

extern "C" int hud_ui_main_menu_new_game_button_on_activate_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    RecoilApp oldApp = g_RecoilApp;
    const HudUiNewGamePanelOverlayOwner oldNewGameState = g_HudUiNewGamePanelOverlayOwner;

    int networkEnabled = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = nullptr;

    g_HudUiNewGamePanelOverlayOwner = HudUiNewGamePanelOverlayOwner{};
    g_HudUiNewGamePanelOverlayOwner.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_queueEnterOnEnterCalls = 0;

    TestQueueEnterState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_RecoilApp = {};
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    HudUiMainMenuDialog dialog{};
    dialog.Constructor(RECOIL_MAINMENU_ROUTE_FRONTEND);

    zVidImagePartial activateImage{};
    dialog.newGameButton.activateImage = &activateImage;

    typedef void(RECOIL_THISCALL *ActivateFn)(HudUiZrdWidget * self);
    ((ActivateFn)(dialog.newGameButton.base.ftable->slots[12]))(&dialog.newGameButton);

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
                 item->m_stateObj ==
                     static_cast<RecoilPtr32>(
                         reinterpret_cast<std::uintptr_t>(&g_HudUiNewGamePanelOverlayOwner)) &&
                 item->m_param == 0;
        CleanupGlobalAppQueue();
    }

    const bool activated =
        dialog.newGameButton.base.image == &activateImage && g_queueEnterOnEnterCalls == 1;

    g_HudUiNewGamePanelOverlayOwner = oldNewGameState;
    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    return itemOk && activated ? 0 : 1;
}

extern "C" int hud_ui_menu_back_button_on_activate_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    HudLayoutBase *const oldLayout = g_HudUiMgrCurrentLayout;
    RecoilApp oldApp = g_RecoilApp;

    int networkEnabled = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = nullptr;

    TestQueueEnterState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_queueEnterOnEnterCalls = 0;
    g_queueEnterOnExitCalls = 0;
    g_RecoilApp = {};
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    HudLayoutBase_FTable layoutTable{};
    layoutTable.OnActivated = TestMainMenuLayoutOnActivated;
    HudLayoutBase layout{};
    layout.ftable = &layoutTable;
    g_mainMenuLayoutActivatedCount = 0;
    g_HudUiMgrCurrentLayout = &layout;

    HudUiMainMenuDialog dialog{};
    dialog.Constructor(RECOIL_MAINMENU_ROUTE_FRONTEND);

    zVidImagePartial activateImage{};
    dialog.backButton.activateImage = &activateImage;

    typedef void(RECOIL_THISCALL *ActivateFn)(HudUiZrdWidget * self);
    ((ActivateFn)(dialog.backButton.base.ftable->slots[12]))(&dialog.backButton);

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
                 item->m_stateObj == 0 && item->m_param == 0;
        CleanupGlobalAppQueue();
    }

    const bool activated = dialog.backButton.base.image == &activateImage &&
                           g_queueEnterOnExitCalls == 1 && g_queueEnterOnEnterCalls == 0 &&
                           g_mainMenuLayoutActivatedCount == 1;

    g_HudUiMgrCurrentLayout = oldLayout;
    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    return itemOk && activated ? 0 : 1;
}

extern "C" int hud_ui_main_menu_options_button_on_activate_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    RecoilApp oldApp = g_RecoilApp;
    const HudUiOptionsPanelOverlayOwner oldOptionsState = g_HudUiOptionsPanelOverlayOwner;

    int networkEnabled = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = nullptr;

    g_HudUiOptionsPanelOverlayOwner = HudUiOptionsPanelOverlayOwner{};
    g_HudUiOptionsPanelOverlayOwner.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_queueEnterOnEnterCalls = 0;

    TestQueueEnterState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_RecoilApp = {};
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    HudUiMainMenuDialog dialog{};
    dialog.Constructor(RECOIL_MAINMENU_ROUTE_FRONTEND);

    zVidImagePartial activateImage{};
    dialog.optionsButton.activateImage = &activateImage;

    typedef void(RECOIL_THISCALL *ActivateFn)(HudUiZrdWidget * self);
    ((ActivateFn)(dialog.optionsButton.base.ftable->slots[12]))(&dialog.optionsButton);

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
                 item->m_stateObj ==
                     static_cast<RecoilPtr32>(
                         reinterpret_cast<std::uintptr_t>(&g_HudUiOptionsPanelOverlayOwner)) &&
                 item->m_param == 0;
        CleanupGlobalAppQueue();
    }

    const bool activated =
        dialog.optionsButton.base.image == &activateImage && g_queueEnterOnEnterCalls == 1;

    g_HudUiOptionsPanelOverlayOwner = oldOptionsState;
    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    return itemOk && activated ? 0 : 1;
}

extern "C" int hud_ui_main_menu_quit_button_on_activate_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    RecoilApp oldApp = g_RecoilApp;
    const RecoilStateConfirmQuit oldConfirmQuitState = g_RecoilState_ConfirmQuit;

    int networkEnabled = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = nullptr;

    g_RecoilState_ConfirmQuit = RecoilStateConfirmQuit{};
    g_RecoilState_ConfirmQuit.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_queueEnterOnEnterCalls = 0;

    TestQueueEnterState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_RecoilApp = {};
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    HudUiMainMenuDialog dialog{};
    dialog.Constructor(RECOIL_MAINMENU_ROUTE_FRONTEND);

    zVidImagePartial activateImage{};
    dialog.quitButton.activateImage = &activateImage;

    typedef void(RECOIL_THISCALL *ActivateFn)(HudUiZrdWidget * self);
    ((ActivateFn)(dialog.quitButton.base.ftable->slots[12]))(&dialog.quitButton);

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
                 item->m_stateObj ==
                     static_cast<RecoilPtr32>(
                         reinterpret_cast<std::uintptr_t>(&g_RecoilState_ConfirmQuit)) &&
                 item->m_param == 0;
        CleanupGlobalAppQueue();
    }

    const bool activated =
        dialog.quitButton.base.image == &activateImage && g_queueEnterOnEnterCalls == 1;

    g_RecoilState_ConfirmQuit = oldConfirmQuitState;
    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    return itemOk && activated ? 0 : 1;
}

extern "C" int hud_ui_main_menu_controls_button_on_activate_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    RecoilApp oldApp = g_RecoilApp;
    const RecoilStateControls oldControlsState = g_RecoilStateControls;

    int networkEnabled = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = nullptr;

    g_RecoilStateControls = RecoilStateControls{};
    g_RecoilStateControls.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_queueEnterOnEnterCalls = 0;

    TestQueueEnterState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_queueEnterVtable));
    g_RecoilApp = {};
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    HudUiMainMenuDialog dialog{};
    dialog.Constructor(RECOIL_MAINMENU_ROUTE_FRONTEND);

    zVidImagePartial activateImage{};
    dialog.controlsButton.activateImage = &activateImage;

    typedef void(RECOIL_THISCALL *ActivateFn)(HudUiZrdWidget * self);
    ((ActivateFn)(dialog.controlsButton.base.ftable->slots[12]))(&dialog.controlsButton);

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
                 item->m_stateObj ==
                     static_cast<RecoilPtr32>(
                         reinterpret_cast<std::uintptr_t>(&g_RecoilStateControls)) &&
                 item->m_param == 0;
        CleanupGlobalAppQueue();
    }

    const bool activated =
        dialog.controlsButton.base.image == &activateImage && g_queueEnterOnEnterCalls == 1;

    g_RecoilStateControls = oldControlsState;
    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    return itemOk && activated ? 0 : 1;
}

// Reimplements 0x415220: RecoilStateMainMenuTransition::OnTryBecomeCurrent (frontend path).
extern "C" int recoil_state_main_menu_transition_on_try_become_current_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface = g_zVideo_DisplayModeSurfaceState;
    auto *const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    auto *const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldCdAudio = ZOPT_SOUND_CDAUDIO;
    void *const oldGlobalVolumeScale = g_zSnd_GlobalVolumeScalePtr;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;

    int networkEnabled = 0;
    int cdAudio = 0;
    float globalVolumeScale = 1.0f;
    char dialogSetName[] = "DIALOG";
    zSndSampleSet dialogSet = {};
    zSndSampleSet *sampleSetSlots[1] = {&dialogSet};
    dialogSet.setName = dialogSetName;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = nullptr;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_DisplayModeSurfaceState = {};
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    ZOPT_SOUND_CDAUDIO = &cdAudio;
    g_zSnd_GlobalVolumeScalePtr = &globalVolumeScale;
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;

    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;

    RecoilStateMainMenuTransition state{};
    state.Constructor();
    state.m_entryRoute = RECOIL_MAINMENU_ROUTE_FRONTEND;

    const int result = state.OnTryBecomeCurrent();
    int failure = 0;
    if (result != 1) {
        failure = 1;
    }
    if (failure == 0 && g_RecoilState_MainMenuSkipExitDelay != 0) {
        failure = 2;
    }
    if (failure == 0 && state.m_pausedAudioSnapshot == 0) {
        failure = 3;
    }
    if (failure == 0 && state.m_mainMenuDialog == 0) {
        failure = 4;
    }

    RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_SOUND_CDAUDIO = oldCdAudio;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    return failure;
}

extern "C" int recoil_state_main_menu_transition_destructor_smoke(void) {
    RecoilStateMainMenuTransition state{};
    state.vftable = 0x11111111;
    state.m_mainMenuDialog = 0;

    state.~RecoilStateMainMenuTransition();

    if (state.vftable != kRecoilStateBase_VtblAddress || state.m_mainMenuDialog != 0) {
        return 1;
    }

    return 0;
}

extern "C" int recoil_state_main_menu_transition_scalar_deleting_destructor_smoke(void) {
    RecoilStateMainMenuTransition state{};
    state.vftable = kRecoilStateMainMenuTransition_VtblAddress;
    state.m_mainMenuDialog = 0;

    RecoilStateMainMenuTransition *const returned = state.ScalarDeletingDestructor(0);
    if (returned != &state) {
        return 1;
    }

    if (state.vftable != kRecoilStateBase_VtblAddress || state.m_mainMenuDialog != 0) {
        return 2;
    }

    auto *const deletingState = new RecoilStateMainMenuTransition{};
    deletingState->vftable = kRecoilStateMainMenuTransition_VtblAddress;
    deletingState->m_mainMenuDialog = 0;

    RecoilStateMainMenuTransition *const deletingReturned =
        deletingState->ScalarDeletingDestructor(1);
    if (deletingReturned != deletingState) {
        return 3;
    }

    return 0;
}
