// Checked-in focused native smoke translation unit, formerly extracted from recoil_app_message_map.cpp.
// Emits only the RecoilApp FMV-state smokes needed by functional manifests.

#include "Battlesport/recoil_app.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/player.h"
#include "Battlesport/hud.h"
#include "Battlesport/about.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <commdlg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" std::int32_t g_zSndCdFlags;
extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" const char *g_RecoilApp_WndClassNamePtr;
extern "C" int g_RecoilApp_AttractFmvReloadMode;
extern "C" unsigned int g_HudUi_InvalidateMask;
BOOL __stdcall AfxWinInit(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine,
                               int showCommand);

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits * Constructor();
    static void StaticInitAndRegisterAtExit();
    static void StaticInit();
    static void RegisterAtExit();
    void OnWndActivate(int activateCode);
    int OnTryBecomeCurrent();
    void OnDeactivate();
    ~RecoilStateCredits();
    static void QueuePush();
};

extern RecoilStateCredits g_RecoilStateCredits;

namespace {
int g_stateEnterCount;
int g_stateExitCount;
int g_stateIdleCount;
int g_stateTryBecomeCurrentCount;
int g_stateTryBecomeCurrentResult;
int g_stateUpdateShouldQuitCount;
int g_stateUpdateShouldQuitResult;
int g_stateDeactivateCount;
int g_stateSuspendCount;
int g_stateSuspendParam;
int g_stateResumeCount;
int g_stateResumeParam;
std::uint32_t g_stateIdleWParam;
std::uint32_t g_stateIdleLParam;

struct TestAppState : RecoilApp_IState {
    void OnEnter() {
        ++g_stateEnterCount;
    }

    void OnExit() {
        ++g_stateExitCount;
    }

    std::int32_t OnTryBecomeCurrent() {
        ++g_stateTryBecomeCurrentCount;
        return g_stateTryBecomeCurrentResult;
    }

    std::int32_t OnUpdateShouldQuit() {
        ++g_stateUpdateShouldQuitCount;
        return g_stateUpdateShouldQuitResult;
    }

    void OnDeactivate() {
        ++g_stateDeactivateCount;
    }

    void OnSuspend(int param) {
        ++g_stateSuspendCount;
        g_stateSuspendParam = param;
    }

    void OnResume(int param) {
        ++g_stateResumeCount;
        g_stateResumeParam = param;
    }

    std::int32_t OnIdleOrDispatch(std::uint32_t wParam, std::uint32_t lParam) {
        ++g_stateIdleCount;
        g_stateIdleWParam = wParam;
        g_stateIdleLParam = lParam;
        return 123;
    }
};

struct AppSnapshot {
    unsigned char bytes[sizeof(RecoilApp)];
};

struct TransitionSnapshot {
    unsigned char bytes[sizeof(RecoilStateMainMenuTransition)];
};

void UseTestStateVtable(RecoilApp_IState &state) {
    TestAppState prototype;
    *reinterpret_cast<void **>(&state) =
        *reinterpret_cast<void **>(&prototype);
}

void SaveApp(AppSnapshot &snapshot) {
    std::memcpy(snapshot.bytes, &g_RecoilApp, sizeof(g_RecoilApp));
}

void RestoreApp(const AppSnapshot &snapshot) {
    std::memcpy(&g_RecoilApp, snapshot.bytes, sizeof(g_RecoilApp));
}

void SaveTransition(TransitionSnapshot &snapshot) {
    std::memcpy(snapshot.bytes, &g_RecoilState_MainMenuTransition,
                sizeof(g_RecoilState_MainMenuTransition));
}

void RestoreTransition(const TransitionSnapshot &snapshot) {
    std::memcpy(&g_RecoilState_MainMenuTransition, snapshot.bytes,
                sizeof(g_RecoilState_MainMenuTransition));
}

void CleanupSingleQueuedItem(RecoilApp_StateQueue &queue) {
    if (queue.m_itemCount == 0 || queue.m_chunkBaseList == nullptr) {
        return;
    }

    RecoilApp_StateQueueItem *const item = queue.Front();
    delete item;

    RecoilApp_StateQueueItem **const chunk = queue.m_readBlock.m_chunkBegin;
    RecoilApp_StateQueueItem ***const chunkBaseList = queue.m_chunkBaseList;
    ::operator delete(chunk);
    ::operator delete(chunkBaseList);
    queue = RecoilApp_StateQueue{};
}

RecoilApp_StateQueueItem *QueueItemAt(RecoilApp_StateQueue &queue, int index) {
    if (index < 0 || index >= queue.m_itemCount || queue.m_chunkBaseList == nullptr) {
        return nullptr;
    }

    return queue.m_readBlock.m_cursor[index];
}
} // namespace

extern "C" int recoil_app_fmv_state_constructor_smoke(void) {
    RecoilApp_AttractFmvState attract{};
    auto *returnedAttract = &attract;
    auto *attractScript = &attract.m_fmv;
    if (returnedAttract != &attract ||
        attractScript->m_abortOnKey != 1 || attractScript->m_head != nullptr) {
        return 1;
    }

    RecoilApp_IntroFmvState intro{};
    auto *returnedIntro = &intro;
    auto *introScript = &intro.m_fmv;
    if (returnedIntro != &intro ||
        introScript->m_abortOnKey != 1 || introScript->m_tail != nullptr) {
        return 2;
    }

    RecoilApp_MissionFmvState mission{};
    auto *returnedMission = &mission;
    auto *missionScript = &mission.m_fmv;
    if (returnedMission != &mission ||
        mission.m_missionId != 0 || mission.m_skipMissionFmv != 0 ||
        missionScript->m_abortOnKey != 1 || missionScript->m_cur != nullptr) {
        return 3;
    }

    return 0;
}

extern "C" int recoil_app_intro_fmv_on_try_become_current_smoke(void) {
    const int oldSkipIntro = g_RecoilApp.m_skipIntroFmv;
    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    zOpt_ViewRectSection **const oldDisplayOption = g_zGame_Options_PointerCache.displaySection;
    zOpt_ViewRectSection **const oldWindowOption = g_zGame_Options_PointerCache.windowSection;
    int *const oldStrideOption = g_zGame_Options_PointerCache.videoStride;

    zOpt_ViewRectSection displaySection = {};
    zOpt_ViewRectSection windowSection = {};
    displaySection.bitsPerPixel = 16;
    windowSection.x = 10;
    windowSection.y = 20;
    windowSection.rightExclusive = 330;
    windowSection.bottomExclusive = 220;
    windowSection.width = 320;
    windowSection.height = 200;
    zOpt_ViewRectSection *displayPtr = &displaySection;
    zOpt_ViewRectSection *windowPtr = &windowSection;
    int stride = 640;
    unsigned short pixels[320 * 2] = {};

    g_zGame_Options_PointerCache.displaySection = &displayPtr;
    g_zGame_Options_PointerCache.windowSection = &windowPtr;
    g_zGame_Options_PointerCache.videoStride = &stride;
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    zRndr::g_frameBuffer = nullptr;
    zRndr::g_activeRegionWidth = 0;
    zRndr::g_activeRegionHeight = 0;
    zRndr::g_bytesPerPixel = 0;
    zRndr::g_pitchBytes = 0;
    zRndr::g_videoStrideMirror0 = 0;
    zRndr::g_videoStrideMirror1 = 0;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    g_zVid_CachedClientRectUpdateMask = 0;
    g_RecoilApp.m_skipIntroFmv = 1;

    RecoilApp_IntroFmvState intro{};
    const int result = intro.OnTryBecomeCurrent();

    const bool ok =
        result == 1 && zRndr::g_frameBuffer == pixels && zRndr::g_activeRegionWidth == 320 &&
        zRndr::g_activeRegionHeight == 200 && zRndr::g_activeRegionRect.x == 10 &&
        zRndr::g_activeRegionRect.y == 20 && zRndr::g_activeRegionRect.right == 330 &&
        zRndr::g_activeRegionRect.bottom == 220 && zRndr::g_bytesPerPixel == 2 &&
        zRndr::g_pitchBytes == 640 && zRndr::g_videoStrideMirror0 == 640 &&
        zRndr::g_videoStrideMirror1 == 640 && g_zVideo_FxSurfacePixels16 == pixels &&
        g_zVideo_FxSurfaceWidth == 320 && g_zVideo_FxSurfaceHeight == 200 &&
        g_zVideo_FxSurfacePitchBytes == 640 && g_zVideo_FxSurfacePitchPixels16 == 320 &&
        g_zVid_CachedClientRectUpdateMask == 1;

    g_RecoilApp.m_skipIntroFmv = oldSkipIntro;
    g_RecoilApp_hWndMain = oldMainHwnd;
    g_zGame_Options_PointerCache.displaySection = oldDisplayOption;
    g_zGame_Options_PointerCache.windowSection = oldWindowOption;
    g_zGame_Options_PointerCache.videoStride = oldStrideOption;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_intro_fmv_on_update_should_quit_smoke(void) {
    AppSnapshot oldApp;
    SaveApp(oldApp);

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex = -1;
    g_RecoilApp.m_skipIntroFmv = 1;
    UseTestStateVtable(g_RecoilApp.m_missionFmvState);
    UseTestStateVtable(g_RecoilApp.m_mainMenuPrepState);

    RecoilApp_IntroFmvState intro{};
    if (intro.OnUpdateShouldQuit() != 0 || g_RecoilApp.m_stateQueue.m_itemCount != 1) {
        RestoreApp(oldApp);
        return 1;
    }

    RecoilApp_StateQueueItem *item = *(g_RecoilApp.m_stateQueue.m_writeBlock.m_cursor - 1);
    const bool skipOk =
        item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent && item->m_param == 0 &&
        item->m_stateObj ==
            &g_RecoilApp.m_missionFmvState;
    CleanupSingleQueuedItem(g_RecoilApp.m_stateQueue);

    std::memset(&g_RecoilApp.m_stateQueue, 0, sizeof(g_RecoilApp.m_stateQueue));
    g_RecoilApp.m_skipIntroFmv = 0;
    auto *script = &intro.m_fmv;
    script->m_cur = nullptr;

    if (intro.OnUpdateShouldQuit() != 0 || g_RecoilApp.m_stateQueue.m_itemCount != 1) {
        RestoreApp(oldApp);
        return 2;
    }

    item = *(g_RecoilApp.m_stateQueue.m_writeBlock.m_cursor - 1);
    const bool finishedOk =
        item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent && item->m_param == 0 &&
        item->m_stateObj ==
            &g_RecoilApp.m_mainMenuPrepState;
    CleanupSingleQueuedItem(g_RecoilApp.m_stateQueue);
    RestoreApp(oldApp);

    return skipOk && finishedOk ? 0 : 3;
}

extern "C" int recoil_app_intro_fmv_on_deactivate_smoke(void) {
    RecoilApp_IntroFmvState intro{};
    auto *script = &intro.m_fmv;
    auto *action = new zFMV_Action{};
    action->next = nullptr;
    script->m_head = action;
    script->m_tail = action;
    script->m_cur = action;

    intro.OnDeactivate();
    return script->m_head == nullptr && script->m_tail == nullptr && script->m_cur == nullptr ? 0
                                                                                              : 1;
}

extern "C" int recoil_app_main_menu_prep_on_try_become_current_smoke(void) {
    unsigned short pixels[320 * 2] = {};

    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;

    RecoilApp_MainMenuPrepState state{};
    state.m_stateData04 = 0x12345678;

    const int result = state.OnTryBecomeCurrent();
    return result == 1 && state.m_stateData04 == 0 && g_zVideo_FxSurfacePixels16 == pixels &&
                   g_zVideo_FxSurfaceWidth == 320 && g_zVideo_FxSurfaceHeight == 200 &&
                   g_zVideo_FxSurfacePitchBytes == 640 && g_zVideo_FxSurfacePitchPixels16 == 320
               ? 0
               : 1;
}

extern "C" int recoil_app_main_menu_prep_on_update_should_quit_smoke(void) {
    AppSnapshot oldApp;
    SaveApp(oldApp);
    TransitionSnapshot oldTransition;
    SaveTransition(oldTransition);

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex = -1;
    g_stateEnterCount = 0;
    g_RecoilState_MainMenuTransition.m_entryRoute = static_cast<RecoilMainMenuEntryRoute>(7);
    UseTestStateVtable(g_RecoilState_MainMenuTransition);

    RecoilApp_MainMenuPrepState state{};
    const int result = state.OnUpdateShouldQuit();

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        RecoilApp_StateQueueItem *const item =
            *(queue.m_writeBlock.m_cursor - 1);
        itemOk = item->m_kind == RecoilApp_StateQueueKind_PushState && item->m_param == 0 &&
                 item->m_stateObj == &g_RecoilState_MainMenuTransition;
        CleanupSingleQueuedItem(queue);
    }

    const bool ok =
        result == 0 &&
        g_RecoilState_MainMenuTransition.m_entryRoute == RECOIL_MAINMENU_ROUTE_FRONTEND &&
        g_stateEnterCount == 1 && itemOk;

    RestoreApp(oldApp);
    RestoreTransition(oldTransition);
    return ok ? 0 : 1;
}

extern "C" int recoil_app_attract_fmv_on_try_become_current_smoke(void) {
    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    const int oldReloadMode = g_RecoilApp_AttractFmvReloadMode;
    HINSTANCE const instance = GetModuleHandleA(nullptr);
    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil-attract-fmv-on-try-smoke",
                                      WS_POPUP, 0, 0, 160, 90, nullptr, nullptr, instance,
                                      nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    unsigned short pixels[320 * 2] = {};
    g_RecoilApp_hWndMain = hwnd;
    g_RecoilApp_AttractFmvReloadMode = 1;
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;

    RecoilApp_AttractFmvState state{};
    state.m_clientRect[0] = -1;
    state.m_clientRect[1] = -1;
    state.m_clientRect[2] = -1;
    state.m_clientRect[3] = -1;

    const int result = state.OnTryBecomeCurrent();
    auto *const script = &state.m_fmv;
    const bool ok =
        result == 1 && g_RecoilApp_AttractFmvReloadMode == 0 && script->m_hWnd == hwnd &&
        state.m_clientRect[0] == 0 && state.m_clientRect[1] == 0 &&
        state.m_clientRect[2] > 0 && state.m_clientRect[3] > 0 &&
        g_zVideo_FxSurfacePixels16 == pixels && g_zVideo_FxSurfaceWidth == 320 &&
        g_zVideo_FxSurfaceHeight == 200 && g_zVideo_FxSurfacePitchBytes == 640 &&
        g_zVideo_FxSurfacePitchPixels16 == 320;

    state.~RecoilApp_AttractFmvState();
    new (&state) RecoilApp_AttractFmvState{};
    DestroyWindow(hwnd);
    g_RecoilApp_hWndMain = oldMainHwnd;
    g_RecoilApp_AttractFmvReloadMode = oldReloadMode;
    return ok ? 0 : 2;
}

extern "C" int recoil_app_attract_fmv_on_update_should_quit_smoke(void) {
    AppSnapshot oldApp;
    SaveApp(oldApp);

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex = -1;
    UseTestStateVtable(g_RecoilApp.m_mainMenuPrepState);
    g_stateEnterCount = 0;

    RecoilApp_AttractFmvState state{};
    auto *const script = &state.m_fmv;
    script->m_cur = nullptr;

    const int result = state.OnUpdateShouldQuit();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        auto *const item =
            *(queue.m_writeBlock.m_cursor - 1);
        itemOk = item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent && item->m_param == 0 &&
                 item->m_stateObj == &g_RecoilApp.m_mainMenuPrepState;
        CleanupSingleQueuedItem(queue);
    }

    const bool ok = result == 0 && g_stateEnterCount == 1 && itemOk;
    RestoreApp(oldApp);
    return ok ? 0 : 1;
}

extern "C" int recoil_app_attract_fmv_on_deactivate_smoke(void) {
    RecoilApp_AttractFmvState state{};
    auto *const script = &state.m_fmv;
    zFMV_Action *const action1 = new zFMV_Action{};
    zFMV_Action *const action2 = new zFMV_Action{};

    action1->next = action2;
    action2->next = nullptr;
    script->m_head = action1;
    script->m_tail = action2;
    script->m_cur = action2;

    state.OnDeactivate();
    return script->m_head == action1 && script->m_tail == action2 && script->m_cur == action1 &&
                   action1->next == action2 && action2->next == nullptr
               ? 0
               : 1;
}

extern "C" int recoil_app_mission_fmv_on_try_become_current_skip_smoke(void) {
    zArchiveList *const oldSearchPathList = g_zRdr_SearchPathList;
    zArchiveList *const oldScratchSearchPathList = g_zRdr_ScratchSearchPathList;
    zArchiveList *const oldMissionSearchPathList = g_zImage_MissionSearchPathList;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    HWND const oldMainHwnd = g_RecoilApp_hWndMain;

    g_zRdr_SearchPathList = nullptr;
    g_zRdr_ScratchSearchPathList = nullptr;
    g_zImage_MissionSearchPathList = nullptr;
    g_zArchive_MountedList = zArchiveList_CreateEmpty();
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);

    g_HudSensorTracker.Constructor();
    g_HudSensorTracker.missionFlags = 0;
    g_HudSensorTracker.SetMissionId(6);

    RecoilApp_MissionFmvState adoptedState{};
    adoptedState.m_missionId = 0;
    adoptedState.m_skipMissionFmv = 1;
    zFMV_Script *const adoptedScript = &adoptedState.m_fmv;
    adoptedScript->m_hWnd = nullptr;

    const int adoptedResult = adoptedState.OnTryBecomeCurrent();
    const bool adoptedOk =
        adoptedResult == 1 && adoptedState.m_missionId == 6 &&
        g_HudSensorTracker.GetMissionId() == 6 && adoptedScript->m_hWnd == nullptr &&
        g_zRdr_SearchPathList != nullptr && g_zRdr_ScratchSearchPathList != nullptr &&
        g_zImage_MissionSearchPathList != nullptr;

    RecoilApp_MissionFmvState explicitState{};
    explicitState.m_missionId = 4;
    explicitState.m_skipMissionFmv = 1;
    zFMV_Script *const explicitScript = &explicitState.m_fmv;
    explicitScript->m_hWnd = nullptr;

    const int explicitResult = explicitState.OnTryBecomeCurrent();
    const bool explicitOk =
        explicitResult == 1 && explicitState.m_missionId == 4 &&
        g_HudSensorTracker.GetMissionId() == 4 && explicitScript->m_hWnd == nullptr;

    if (g_zRdr_SearchPathList != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zRdr_SearchPathList);
    }
    if (g_zRdr_ScratchSearchPathList != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zRdr_ScratchSearchPathList);
    }
    if (g_zImage_MissionSearchPathList != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionSearchPathList);
    }
    if (g_zArchive_MountedList != nullptr) {
        zArchiveList_Destroy(g_zArchive_MountedList);
    }

    g_HudSensorTracker.Shutdown();
    g_zRdr_SearchPathList = oldSearchPathList;
    g_zRdr_ScratchSearchPathList = oldScratchSearchPathList;
    g_zImage_MissionSearchPathList = oldMissionSearchPathList;
    g_zArchive_MountedList = oldMountedList;
    g_RecoilApp_hWndMain = oldMainHwnd;

    return adoptedOk && explicitOk ? 0 : 1;
}

extern "C" int recoil_app_mission_fmv_on_deactivate_smoke(void) {
    RecoilApp_MissionFmvState skippedState{};
    skippedState.m_missionId = 9;
    skippedState.m_skipMissionFmv = 1;
    zFMV_Script *const skippedScript = &skippedState.m_fmv;
    skippedScript->m_head = nullptr;
    skippedScript->m_tail = reinterpret_cast<zFMV_Action *>(0x11111111);
    skippedScript->m_cur = reinterpret_cast<zFMV_Action *>(0x22222222);

    skippedState.OnDeactivate();
    const bool skippedOk = skippedState.m_missionId == 0 && skippedScript->m_head == nullptr &&
                           skippedScript->m_tail ==
                               reinterpret_cast<zFMV_Action *>(0x11111111) &&
                           skippedScript->m_cur ==
                               reinterpret_cast<zFMV_Action *>(0x22222222);

    RecoilApp_MissionFmvState activeState{};
    activeState.m_missionId = 7;
    activeState.m_skipMissionFmv = 0;
    zFMV_Script *const activeScript = &activeState.m_fmv;
    activeScript->m_head = nullptr;
    activeScript->m_tail = reinterpret_cast<zFMV_Action *>(0x33333333);
    activeScript->m_cur = reinterpret_cast<zFMV_Action *>(0x44444444);

    activeState.OnDeactivate();
    const bool activeOk =
        activeState.m_missionId == 0 && activeScript->m_head == nullptr &&
        activeScript->m_tail == nullptr && activeScript->m_cur == nullptr;

    return skippedOk && activeOk ? 0 : 1;
}

extern "C" int recoil_app_mission_fmv_on_update_should_quit_smoke(void) {
    struct ActiveAction : zFMV_Action {
        int Update(double) {
            return 1;
        }

        void Begin(double) {}
        void End() {}
    };

    RecoilApp_MissionFmvState activeState{};
    ActiveAction activeAction{};
    zFMV_Script *const activeScript = &activeState.m_fmv;
    activeScript->m_abortOnKey = 0;
    activeScript->m_head = &activeAction;
    activeScript->m_tail = &activeAction;
    activeScript->m_cur = &activeAction;

    const int result = activeState.OnUpdateShouldQuit();
    const bool activeOk = result == 0;
    activeScript->m_head = nullptr;
    activeScript->m_tail = nullptr;
    activeScript->m_cur = nullptr;
    return activeOk ? 0 : 1;
}
