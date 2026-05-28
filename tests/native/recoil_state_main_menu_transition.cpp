#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include <cstring>
#include <new>

#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"

extern "C" int g_RecoilState_MainMenuSkipExitDelay;

extern "C" int(RECOIL_FASTCALL *g_zVideo_pfnLockSurfaceState)(
    zVideo_SurfaceStatePartial *surfaceState);
extern "C" int(RECOIL_FASTCALL *g_zVideo_pfnUnlockSurfaceState)(
    zVideo_SurfaceStatePartial *surfaceState);

namespace {
int g_queueEnterOnEnterCalls;

struct TestQueueEnterState : RecoilApp_IState {
    void RECOIL_THISCALL OnEnter() {
        ++g_queueEnterOnEnterCalls;
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
    return vtable;
}

RecoilApp_IState_Vtbl g_queueEnterVtable = MakeQueueEnterVtable();

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
