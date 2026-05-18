#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include <cstring>
#include <new>

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

extern "C" int recoil_state_main_menu_transition_destructor_smoke(void) {
    RecoilStateMainMenuTransition state{};
    state.vftable = 0x11111111;
    state.m_mainMenuDialog = 0;

    state.Destructor();

    if (state.vftable != kRecoilStateBase_VtblAddress || state.m_mainMenuDialog != 0) {
        return 1;
    }

    return 0;
}
