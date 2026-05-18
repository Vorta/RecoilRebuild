#include "Battlesport/RecoilApp.h"

#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdlib>
#include <new>

extern "C" std::int32_t g_zSndCdFlags;
extern "C" const char *g_RecoilApp_WndClassNamePtr;

namespace {
int g_stateEnterCount;
int g_stateExitCount;
int g_stateIdleCount;
std::uint32_t g_stateIdleWParam;
std::uint32_t g_stateIdleLParam;
int g_startEngineCount;
int g_shutdownEngineCount;
int g_exitInstanceCount;
int g_startEngineResult;
RecoilPtr32 g_lastStartEngineHwnd;

struct TestAppState : RecoilApp_IState {
    void RECOIL_THISCALL OnEnter() {
        ++g_stateEnterCount;
    }

    void RECOIL_THISCALL OnExit() {
        ++g_stateExitCount;
    }

    std::int32_t RECOIL_THISCALL OnIdleOrDispatch(std::uint32_t wParam, std::uint32_t lParam) {
        ++g_stateIdleCount;
        g_stateIdleWParam = wParam;
        g_stateIdleLParam = lParam;
        return 123;
    }
};

RecoilFn32 StateMethodToFn(void (RECOIL_THISCALL TestAppState::*method)()) {
    union MemberToFunction {
        void (RECOIL_THISCALL TestAppState::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 StateIdleMethodToFn(
    std::int32_t (RECOIL_THISCALL TestAppState::*method)(std::uint32_t, std::uint32_t)) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL TestAppState::*member)(std::uint32_t, std::uint32_t);
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilApp_IState_Vtbl MakeTestAppStateVtable() {
    RecoilApp_IState_Vtbl vtable{};
    vtable.OnEnter = StateMethodToFn(&TestAppState::OnEnter);
    vtable.OnExit = StateMethodToFn(&TestAppState::OnExit);
    vtable.OnIdleOrDispatch = StateIdleMethodToFn(&TestAppState::OnIdleOrDispatch);
    return vtable;
}

RecoilApp_IState_Vtbl g_testAppStateVtable = MakeTestAppStateVtable();

struct TestRecoilApp : RecoilApp {
    std::int32_t RECOIL_THISCALL StartEngine(RecoilPtr32 hwnd) {
        ++g_startEngineCount;
        g_lastStartEngineHwnd = hwnd;
        return g_startEngineResult;
    }

    void RECOIL_THISCALL ShutdownEngine() {
        ++g_shutdownEngineCount;
    }

    std::int32_t RECOIL_THISCALL ExitInstance() {
        ++g_exitInstanceCount;
        return 77;
    }
};

RecoilFn32 AppStartMethodToFn(std::int32_t (RECOIL_THISCALL TestRecoilApp::*method)(RecoilPtr32)) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL TestRecoilApp::*member)(RecoilPtr32);
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 AppVoidMethodToFn(void (RECOIL_THISCALL TestRecoilApp::*method)()) {
    union MemberToFunction {
        void (RECOIL_THISCALL TestRecoilApp::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 AppIntMethodToFn(std::int32_t (RECOIL_THISCALL TestRecoilApp::*method)()) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL TestRecoilApp::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 g_testRecoilAppVtable[0x2d];

void InitTestRecoilAppVtable() {
    g_testRecoilAppVtable[0x70 / 4] = AppIntMethodToFn(&TestRecoilApp::ExitInstance);
    g_testRecoilAppVtable[0xac / 4] = AppStartMethodToFn(&TestRecoilApp::StartEngine);
    g_testRecoilAppVtable[0xb0 / 4] = AppVoidMethodToFn(&TestRecoilApp::ShutdownEngine);
}

void CleanupSingleQueuedItem(RecoilApp_StateQueue &queue) {
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
}
} // namespace

extern "C" int recoil_app_get_message_map_smoke(void) {
    const MfcMsgMap *messageMap = g_RecoilApp.GetMessageMap();
    if (messageMap != &g_RecoilApp_MessageMap) {
        return 1;
    }

    if (messageMap->getBaseMap != 0x004428a0 || messageMap->entries != 0x004d0998) {
        return 2;
    }

    const zMfcMsgMapEntry &sentinel = g_RecoilApp_MessageEntries[0];
    return sentinel.nMessage == 0 && sentinel.nCode == 0 && sentinel.nID == 0 &&
                   sentinel.nLastID == 0 && sentinel.nSig == 0 && sentinel.pfn == 0
               ? 0
               : 3;
}

extern "C" int recoil_app_mfc_ole_module_constructor_smoke(void) {
    RecoilApp app{};
    app.m_stateQueue_118.m_ctorTag_00 = 0x11223344;
    app.m_pendingState_0c4 = 0x12345678;
    app.m_currentStateIndex_0c8 = 7;
    app.m_skipWait_0d0 = 9;
    for (RecoilPtr32 &state : app.m_stateStack_0d8) {
        state = 0x87654321;
    }

    RecoilApp *returned = app.MfcOleModuleConstructor();
    int result = 0;
    if (returned != &app || app.vftable != kRecoilApp_MfcOleModule_VtblAddress) {
        result = 1;
    }

    if (result == 0 && (app.m_pendingState_0c4 != 0 || app.m_currentStateIndex_0c8 != -1 ||
                        app.m_skipWait_0d0 != 0)) {
        result = 2;
    }

    const unsigned char expectedCtorByte =
        static_cast<unsigned char>((reinterpret_cast<std::uintptr_t>(&app) >> 24) & 0xff);
    const auto *const ctorTagBytes =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_ctorTag_00);
    if (result == 0 && (ctorTagBytes[0] != expectedCtorByte || ctorTagBytes[1] != 0x33 ||
                        ctorTagBytes[2] != 0x22 || ctorTagBytes[3] != 0x11)) {
        result = 3;
    }

    const auto *const queueTail =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_readBlock);
    for (std::size_t i = 0; result == 0 && i < 0x2c; ++i) {
        if (queueTail[i] != 0) {
            result = 4;
        }
    }

    for (RecoilPtr32 state : app.m_stateStack_0d8) {
        if (result == 0 && state != 0) {
            result = 5;
        }
    }

    app.MfcOleModuleDestructor();
    return result;
}

extern "C" int recoil_app_accessor_and_skip_wait_smoke(void) {
    RecoilApp app{};
    app.m_pMainWnd = 0x12345678;
    if (app.GetMainWnd() != 0x12345678) {
        return 1;
    }

    app.m_skipWait_0d0 = 7;
    if (app.TakeSkipWaitMessage() != 7 || app.m_skipWait_0d0 != 0) {
        return 2;
    }

    if (app.MarkSkipWaitMessage() != 0 || app.m_skipWait_0d0 != 1) {
        return 3;
    }

    return app.MarkSkipWaitMessage() == 1 && app.m_skipWait_0d0 == 1 ? 0 : 4;
}

extern "C" int recoil_app_activate_existing_instance_absent_smoke(void) {
    const char *const oldClassName = g_RecoilApp_WndClassNamePtr;
    g_RecoilApp_WndClassNamePtr = "RecoilNativeAbsentWindowClass";
    const std::int32_t result = g_RecoilApp.ActivateExistingInstance();
    g_RecoilApp_WndClassNamePtr = oldClassName;
    return result == 1 ? 0 : 1;
}

extern "C" int recoil_app_get_current_state_smoke(void) {
    RecoilApp app{};
    app.m_stateStack_0d8[0] = 0x11111111;
    app.m_stateStack_0d8[15] = 0x22222222;

    app.m_currentStateIndex_0c8 = -1;
    if (app.GetCurrentState() != 0) {
        return 1;
    }

    app.m_currentStateIndex_0c8 = 0;
    if (app.GetCurrentState() != 0x11111111) {
        return 2;
    }

    app.m_currentStateIndex_0c8 = 15;
    if (app.GetCurrentState() != 0x22222222) {
        return 3;
    }

    app.m_currentStateIndex_0c8 = 16;
    return app.GetCurrentState() == 0 ? 0 : 4;
}

extern "C" int recoil_app_state_queue_block_init_from_cursor_smoke(void) {
    RecoilPtr32 chunkBaseSlot = 0x12340000;
    RecoilApp_StateQueueBlock block{};
    RecoilApp_StateQueueBlock *returned = block.InitFromCursor(
        0x12340800, static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&chunkBaseSlot)));

    if (returned != &block) {
        return 1;
    }

    return block.m_chunkBegin == 0x12340000 && block.m_chunkEnd == 0x12341000 &&
                   block.m_cursor == 0x12340800 &&
                   block.m_chunkPtrSlot ==
                       static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&chunkBaseSlot))
               ? 0
               : 2;
}

extern "C" int recoil_app_state_queue_grow_chunk_base_list_smoke(void) {
    RecoilApp_StateQueue queue{};
    auto *oldList = static_cast<RecoilPtr32 *>(::operator new(3 * sizeof(RecoilPtr32)));
    oldList[0] = 0x11111111;
    oldList[1] = 0x22222222;
    oldList[2] = 0x33333333;

    queue.m_readBlock.m_chunkPtrSlot =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldList[0]));
    queue.m_writeBlock.m_chunkPtrSlot =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldList[2]));
    queue.m_chunkPtrList = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(oldList));
    queue.m_chunkPtrCapacity = 3;

    const RecoilPtr32 centerSlotValue = queue.GrowAndCenterChunkBaseList(8);
    auto *const newList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    auto *const centerSlot =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(centerSlotValue));

    const bool ok = queue.m_chunkPtrCapacity == 8 && centerSlot == &newList[2] &&
                    centerSlot[0] == 0x11111111 && centerSlot[1] == 0x22222222 &&
                    centerSlot[2] == 0x33333333;
    ::operator delete(newList);

    return ok ? 0 : 1;
}

extern "C" int recoil_app_queue_switch_current_state_smoke(void) {
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    RecoilApp app{};
    TestAppState oldState{};
    TestAppState newState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    newState.vftable = oldState.vftable;
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    const RecoilPtr32 returned = app.QueueSwitchCurrentState(&newState, 42);
    if (returned != app.m_stateStack_0d8[0] || g_stateExitCount != 1 || g_stateEnterCount != 1) {
        return 1;
    }

    RecoilApp_StateQueue &queue = app.m_stateQueue_118;
    if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        return 2;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    const bool itemOk =
        item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
        item->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&newState)) &&
        item->m_param == 42;

    ::operator delete(item);
    auto *const chunkList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    auto *const chunk = reinterpret_cast<void *>(static_cast<std::uintptr_t>(chunkList[1]));
    ::operator delete(chunk);
    ::operator delete(chunkList);

    return itemOk ? 0 : 3;
}

extern "C" int recoil_app_queue_exit_current_state_smoke(void) {
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    RecoilApp app{};
    TestAppState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    const RecoilPtr32 returned = app.QueueExitCurrentState(17);
    if (returned != app.m_stateStack_0d8[0] || g_stateExitCount != 1 || g_stateEnterCount != 0) {
        return 1;
    }

    RecoilApp_StateQueue &queue = app.m_stateQueue_118;
    if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        return 2;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    const bool itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
                        item->m_stateObj == 0 && item->m_param == 17;
    CleanupSingleQueuedItem(queue);

    return itemOk ? 0 : 3;
}

extern "C" int recoil_app_start_engine_and_queue_startup_state_smoke(void) {
    InitTestRecoilAppVtable();
    g_startEngineCount = 0;
    g_shutdownEngineCount = 0;
    g_exitInstanceCount = 0;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    g_lastStartEngineHwnd = 0;

    RecoilPtr32 frameWords[9]{};
    frameWords[8] = 0x44556677;

    TestRecoilApp app{};
    TestAppState startupState{};
    startupState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    app.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(frameWords));
    app.m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&startupState));
    app.m_currentStateIndex_0c8 = -1;
    app.m_skipWait_0d0 = 0;
    app.m_reserved0d4 = 5;
    g_startEngineResult = 1;

    if (app.StartEngineAndQueueStartupState() != 1 || g_startEngineCount != 1 ||
        g_lastStartEngineHwnd != 0x44556677 || g_shutdownEngineCount != 0 ||
        g_exitInstanceCount != 0 || g_stateEnterCount != 1) {
        return 1;
    }

    if (app.m_skipWait_0d0 != 1 || app.m_reserved0d4 != 0 ||
        app.m_stateQueue_118.m_itemCount != 1) {
        return 2;
    }
    CleanupSingleQueuedItem(app.m_stateQueue_118);

    TestRecoilApp failApp{};
    failApp.vftable = app.vftable;
    failApp.m_pMainWnd = app.m_pMainWnd;
    g_startEngineResult = 0;
    if (failApp.StartEngineAndQueueStartupState() != 77 || g_startEngineCount != 2 ||
        g_shutdownEngineCount != 1 || g_exitInstanceCount != 1) {
        return 3;
    }

    return failApp.m_stateQueue_118.m_itemCount == 0 ? 0 : 4;
}

extern "C" int recoil_app_initialize_display_failure_smoke(void) {
    static std::int32_t modeIndex = 3;
    static std::int32_t fullscreen = 1;
    static std::int32_t hwApi = 0;
    static std::int32_t acceleration = 1;

    ZOPT_VIDEO_MODE = &modeIndex;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    g_zVideo_IsInitialized = 1;

    const std::int32_t result = RecoilApp::InitializeDisplay(0x12345678);
    g_zVideo_IsInitialized = 0;
    return result == 0 ? 0 : 1;
}

extern "C" int recoil_app_on_idle_or_dispatch_smoke(void) {
    g_zSndCdFlags = 0;
    g_stateIdleCount = 0;
    g_stateIdleWParam = 0;
    g_stateIdleLParam = 0;

    RecoilApp app{};
    app.m_currentStateIndex_0c8 = -1;
    if (app.OnIdleOrDispatch(0x11, 0x22) != 0 || g_stateIdleCount != 0) {
        return 1;
    }

    TestAppState state{};
    state.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&state));

    if (app.OnIdleOrDispatch(0x33, 0x44) != 123) {
        return 2;
    }

    return g_stateIdleCount == 1 && g_stateIdleWParam == 0x33 && g_stateIdleLParam == 0x44 ? 0 : 3;
}

extern "C" int recoil_app_mfc_ole_module_destructor_smoke(void) {
    RecoilApp app{};
    app.MfcOleModuleConstructor();
    void *chunk = ::operator new(0x1000);
    auto *chunkList = static_cast<RecoilPtr32 *>(::operator new(sizeof(RecoilPtr32)));
    *chunkList = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(chunk));

    const RecoilPtr32 chunkValue =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(chunk));
    const RecoilPtr32 chunkListValue =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(chunkList));
    app.m_stateQueue_118.m_readBlock.m_chunkBegin = chunkValue;
    app.m_stateQueue_118.m_readBlock.m_chunkEnd = chunkValue + 0x1000;
    app.m_stateQueue_118.m_readBlock.m_cursor = chunkValue;
    app.m_stateQueue_118.m_readBlock.m_chunkPtrSlot = chunkListValue;
    app.m_stateQueue_118.m_writeBlock.m_chunkBegin = 0x22222222;
    app.m_stateQueue_118.m_writeBlock.m_chunkEnd = 0x33333333;
    app.m_stateQueue_118.m_writeBlock.m_cursor = 0x44444444;
    app.m_stateQueue_118.m_writeBlock.m_chunkPtrSlot = 0x55555555;
    app.m_stateQueue_118.m_chunkPtrList = chunkListValue;
    app.m_stateQueue_118.m_itemCount = 1;

    app.MfcOleModuleDestructor();
    if (app.m_stateQueue_118.m_itemCount != 0) {
        return 1;
    }

    const auto *const readBytes =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_readBlock);
    const auto *const writeBytes =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_writeBlock);
    for (std::size_t i = 0; i < sizeof(app.m_stateQueue_118.m_readBlock); ++i) {
        if (readBytes[i] != 0 || writeBytes[i] != 0) {
            return 2;
        }
    }

    return 0;
}

extern "C" int recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke(void) {
    auto *app = new RecoilApp{};
    app->MfcOleModuleConstructor();
    RecoilApp *returned = app->MfcOleModuleScalarDeletingDestructor(1);
    return returned == app ? 0 : 1;
}

extern "C" int recoil_app_play_state_constructor_smoke(void) {
    RecoilApp_PlayState playState{};
    playState.base.vftable = 0x11111111;
    playState.pWindowSection = 0x22222222;
    playState.pDisplaySection = 0x33333333;
    playState.pRenderSection = 0x44444444;
    playState.m_transitionScratch_10 = 0x55555555;
    playState.pPendingLoadGameStartPath = 0x66666666;

    RecoilApp_PlayState *returned = playState.Constructor();
    if (returned != &playState) {
        return 1;
    }

    if (playState.base.vftable != kRecoilApp_PlayState_VtblAddress ||
        playState.m_transitionScratch_10 != 0 || playState.pPendingLoadGameStartPath != 0) {
        return 2;
    }

    return playState.pWindowSection == 0x22222222 && playState.pDisplaySection == 0x33333333 &&
                   playState.pRenderSection == 0x44444444
               ? 0
               : 3;
}

extern "C" int recoil_app_fmv_state_constructor_smoke(void) {
    RecoilApp_AttractFmvState attract{};
    auto *returnedAttract = attract.Constructor();
    auto *attractScript = reinterpret_cast<zFMV_Script *>(attract.m_fmv_10);
    if (returnedAttract != &attract ||
        attract.base.vftable != kRecoilApp_AttractFmvState_VtblAddress ||
        attractScript->m_abortOnKey != 1 || attractScript->m_head != nullptr) {
        return 1;
    }

    RecoilApp_IntroFmvState intro{};
    auto *returnedIntro = intro.Constructor();
    auto *introScript = reinterpret_cast<zFMV_Script *>(intro.m_fmv_08);
    if (returnedIntro != &intro || intro.base.vftable != kRecoilApp_IntroFmvState_VtblAddress ||
        introScript->m_abortOnKey != 1 || introScript->m_tail != nullptr) {
        return 2;
    }

    RecoilApp_MissionFmvState mission{};
    auto *returnedMission = mission.Constructor();
    auto *missionScript = reinterpret_cast<zFMV_Script *>(mission.m_fmv_08);
    if (returnedMission != &mission ||
        mission.base.vftable != kRecoilApp_MissionFmvState_VtblAddress ||
        mission.m_missionId != 0 || mission.m_skipMissionFmv != 0 ||
        missionScript->m_abortOnKey != 1 || missionScript->m_cur != nullptr) {
        return 3;
    }

    return 0;
}

extern "C" int recoil_app_constructor_destructor_smoke(void) {
    RecoilApp app{};
    RecoilApp *returned = app.Constructor();
    if (returned != &app || app.vftable != kRecoilApp_VtblAddress ||
        app.m_attractFmvState_160.base.vftable != kRecoilApp_AttractFmvState_VtblAddress ||
        app.m_introFmvState_1a0.base.vftable != kRecoilApp_IntroFmvState_VtblAddress ||
        app.m_mainMenuPrepState_1c8.base.vftable != kRecoilApp_MainMenuPrepState_VtblAddress ||
        app.m_leaveNetworkState_1d0.base.vftable != kRecoilApp_LeaveNetworkState_VtblAddress ||
        app.m_missionFmvState_1d8.base.vftable != kRecoilApp_MissionFmvState_VtblAddress ||
        app.m_playState_208.base.vftable != kRecoilApp_PlayState_VtblAddress ||
        app.m_mpExitDialogState_220.base.vftable != kRecoilApp_MpExitDialogState_VtblAddress ||
        app.m_transitionFadeTimer150 != 0.0f) {
        return 1;
    }

    app.Destructor();
    if (app.m_attractFmvState_160.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_introFmvState_1a0.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_mainMenuPrepState_1c8.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_leaveNetworkState_1d0.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_missionFmvState_1d8.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_playState_208.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_mpExitDialogState_220.base.vftable != kRecoilStateBase_VtblAddress) {
        return 2;
    }

    return 0;
}

extern "C" int recoil_app_istate_destructor_smoke(void) {
    RecoilApp_IState state{0x12345678};
    state.Destructor();

    return state.vftable == kRecoilStateBase_VtblAddress ? 0 : 1;
}

extern "C" int recoil_app_fmv_state_destructor_smoke(void) {
    RecoilApp_AttractFmvState attract{};
    attract.base.vftable = 0x11111111;
    auto *attractScript = reinterpret_cast<zFMV_Script *>(attract.m_fmv_10);
    attractScript->m_fmvPath = static_cast<char *>(std::malloc(4));
    if (attractScript->m_fmvPath == nullptr) {
        return 1;
    }

    attract.Destructor();
    if (attract.base.vftable != kRecoilStateBase_VtblAddress ||
        attractScript->m_fmvPath != nullptr) {
        return 2;
    }

    RecoilApp_IntroFmvState intro{};
    intro.base.vftable = 0x22222222;
    auto *introScript = reinterpret_cast<zFMV_Script *>(intro.m_fmv_08);
    introScript->m_fmvPath = static_cast<char *>(std::malloc(4));
    if (introScript->m_fmvPath == nullptr) {
        return 3;
    }

    intro.Destructor();
    return intro.base.vftable == kRecoilStateBase_VtblAddress && introScript->m_fmvPath == nullptr
               ? 0
               : 4;
}

extern "C" int recoil_app_mission_fmv_state_destructor_smoke(void) {
    RecoilApp_MissionFmvState mission{};
    mission.base.vftable = 0x33333333;
    auto *missionScript = reinterpret_cast<zFMV_Script *>(mission.m_fmv_08);
    missionScript->m_fmvPath = static_cast<char *>(std::malloc(4));
    if (missionScript->m_fmvPath == nullptr) {
        return 1;
    }

    mission.Destructor();
    if (mission.base.vftable != kRecoilStateBase_VtblAddress ||
        missionScript->m_fmvPath != nullptr) {
        return 2;
    }

    auto *heapMission = new RecoilApp_MissionFmvState{};
    heapMission->base.vftable = 0x44444444;
    auto *returned = heapMission->ScalarDeletingDestructor(1);

    return returned == heapMission ? 0 : 3;
}

extern "C" int recoil_app_scalar_deleting_destructor_smoke(void) {
    auto *state = new RecoilApp_IState{0x11111111};
    RecoilApp_IState *returnedState = state->ScalarDeletingDestructor(1);
    if (returnedState != state) {
        return 1;
    }

    auto *attract = new RecoilApp_AttractFmvState{};
    attract->base.vftable = 0x22222222;
    auto *returnedAttract = attract->ScalarDeletingDestructor(1);
    if (returnedAttract != attract) {
        return 2;
    }

    auto *intro = new RecoilApp_IntroFmvState{};
    intro->base.vftable = 0x33333333;
    auto *returnedIntro = intro->ScalarDeletingDestructor(1);
    return returnedIntro == intro ? 0 : 3;
}
