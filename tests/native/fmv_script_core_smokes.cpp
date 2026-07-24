#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdlib>

extern "C" HWND g_RecoilApp_hWndMain;

namespace {

int g_deletedCount;
int g_beginCallCount;
double g_lastBeginTimeSec;
int g_updateCallCount;
int g_endCallCount;
int g_nextUpdateResult;
double g_lastUpdateTimeSec;

struct BeginCurrentActionTestAction : zFMV_Action {
    void Begin(
        double timeSec
    ) {
        ++g_beginCallCount;
        g_lastBeginTimeSec = timeSec;
    }
};

struct TestFmvAction : zFMV_Action {
    ~TestFmvAction() {
        ++g_deletedCount;
    }
};

struct UpdateTestAction : zFMV_Action {
    int Update(
        double timeSec
    ) {
        ++g_updateCallCount;
        g_lastUpdateTimeSec = timeSec;
        return g_nextUpdateResult;
    }

    void Begin(
        double timeSec
    ) {
        ++g_beginCallCount;
        g_lastBeginTimeSec = timeSec;
    }

    void End() {
        ++g_endCallCount;
    }
};

TestFmvAction *NewLinkedAction(
    TestFmvAction *next
) {
    TestFmvAction *action = new TestFmvAction;
    action->next = next;
    return action;
}

} // namespace

extern "C" int zfmv_script_init_null_path_smoke(void) {
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);

    zFMV_Script script = {};
    script.m_fmvPath = reinterpret_cast<char *>(0x11111111);
    script.m_hWnd = reinterpret_cast<HWND>(0x22222222);
    script.m_abortOnKey = 0;
    script.m_head = reinterpret_cast<zFMV_Action *>(0x33333333);
    script.m_tail = reinterpret_cast<zFMV_Action *>(0x44444444);
    script.m_cur = reinterpret_cast<zFMV_Action *>(0x55555555);

    zFMV_Script *returned = script.Init(0, 0, 0);
    if (returned != &script) {
        return 1;
    }

    if (script.m_hWnd != reinterpret_cast<HWND>(0x12345678) || script.m_abortOnKey != 1 ||
        script.m_fmvPath != 0 || script.m_head != 0 || script.m_tail != 0 || script.m_cur != 0) {
        return 2;
    }

    returned = script.Init(
        0,
        0,
        reinterpret_cast<HWND>(0x87654321)
    );
    return returned == &script && script.m_hWnd == reinterpret_cast<HWND>(0x87654321) ? 0 : 3;
}

extern "C" int zfmv_script_reset_smoke(void) {
    g_deletedCount = 0;

    TestFmvAction *action2 = NewLinkedAction(0);
    TestFmvAction *action1 = NewLinkedAction(action2);

    zFMV_Script script = {};
    script.m_head = action1;
    script.m_tail = action2;
    script.m_cur = 0;

    script.Reset(0);
    if (script.m_head != action1 || script.m_tail != action2 || script.m_cur != action1 ||
        g_deletedCount != 0) {
        delete action1;
        delete action2;
        return 1;
    }

    script.Reset(1);
    return script.m_head == 0 && script.m_tail == 0 && script.m_cur == 0 && g_deletedCount == 2
               ? 0
               : 2;
}

extern "C" int zfmv_script_cleanup_smoke(void) {
    g_deletedCount = 0;

    TestFmvAction *action = NewLinkedAction(0);

    zFMV_Script script = {};
    script.m_fmvPath = (char *)(std::malloc(4));
    script.m_head = action;
    script.m_tail = action;
    script.m_cur = action;

    if (script.m_fmvPath == 0) {
        delete action;
        return 1;
    }

    script.Cleanup();
    return script.m_fmvPath == 0 && script.m_head == 0 && script.m_tail == 0 &&
                   script.m_cur == 0 && g_deletedCount == 1
               ? 0
               : 2;
}

extern "C" int zfmv_script_begin_now_smoke(void) {
    g_deletedCount = 0;

    TestFmvAction *action2 = NewLinkedAction(0);
    TestFmvAction *action1 = NewLinkedAction(action2);

    zFMV_Script script = {};
    script.m_head = action1;
    script.m_tail = action2;
    script.m_cur = 0;

    script.BeginNow(0);
    if (script.m_head != action1 || script.m_tail != action2 || script.m_cur != action1 ||
        g_deletedCount != 0) {
        delete action1;
        delete action2;
        return 1;
    }

    script.BeginNow(1);
    return script.m_head == 0 && script.m_tail == 0 && script.m_cur == 0 && g_deletedCount == 2
               ? 0
               : 2;
}

extern "C" int zfmv_script_begin_current_action_smoke(void) {
    zFMV_Script emptyScript = {};
    if (emptyScript.BeginCurrentAction(12.5) != 0) {
        return 1;
    }

    const zSndSampleSetRegistry oldRegistry = g_zSnd_SampleSetRegistry;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldKbdReady = g_zInput_KbdSystemReady;
    const int oldUseArchiveBanksFlag = g_zSnd_UseArchiveBanksFlag;

    zSndSampleSet fmvSet = {};
    fmvSet.setName = const_cast<char *>("FMV");
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&fmvSet);
    g_zSnd_UseArchiveBanksFlag = 0;

    g_zVideo_PrimarySurfaceState.pixels = reinterpret_cast<void *>(0x12340000);
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = 0;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    g_zInput_KbdSystemReady = 0;
    g_beginCallCount = 0;
    g_lastBeginTimeSec = -1.0;

    BeginCurrentActionTestAction action;
    zFMV_Script script = {};
    script.m_cur = &action;
    const int result = script.BeginCurrentAction(42.25);

    const bool ok = result == 1 && script.m_startTimeSec == 42.25 && g_beginCallCount == 1 &&
                    g_lastBeginTimeSec == 0.0 &&
                    g_zVideo_FxSurfacePixels16 ==
                        reinterpret_cast<unsigned short *>(0x12340000) &&
                    g_zVideo_FxSurfaceWidth == 320 && g_zVideo_FxSurfaceHeight == 200 &&
                    g_zVideo_FxSurfacePitchBytes == 640 &&
                    g_zVideo_FxSurfacePitchPixels16 == 320 && fmvSet.resourcesLoaded == 1;

    g_zSnd_SampleSetRegistry = oldRegistry;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    g_zInput_KbdSystemReady = oldKbdReady;
    g_zSnd_UseArchiveBanksFlag = oldUseArchiveBanksFlag;

    return ok ? 0 : 2;
}

extern "C" int zfmv_script_begin_at_time_smoke(void) {
    const zSndSampleSetRegistry oldRegistry = g_zSnd_SampleSetRegistry;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldKbdReady = g_zInput_KbdSystemReady;
    const int oldUseArchiveBanksFlag = g_zSnd_UseArchiveBanksFlag;

    zSndSampleSet fmvSet = {};
    fmvSet.setName = const_cast<char *>("FMV");
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&fmvSet);
    g_zSnd_UseArchiveBanksFlag = 0;

    g_zVideo_PrimarySurfaceState.pixels = reinterpret_cast<void *>(0x12340000);
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = 0;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    g_zInput_KbdSystemReady = 0;
    g_beginCallCount = 0;
    g_lastBeginTimeSec = -1.0;

    BeginCurrentActionTestAction action;
    zFMV_Script script = {};
    script.m_cur = &action;
    const int result = script.BeginAtTime();

    const bool ok = result == 1 && script.m_startTimeSec >= 0.0 && g_beginCallCount == 1 &&
                    g_lastBeginTimeSec == 0.0 && fmvSet.resourcesLoaded == 1;

    g_zSnd_SampleSetRegistry = oldRegistry;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    g_zInput_KbdSystemReady = oldKbdReady;
    g_zSnd_UseArchiveBanksFlag = oldUseArchiveBanksFlag;

    return ok ? 0 : 1;
}

extern "C" int zfmv_script_update_smoke(void) {
    zFMV_Script emptyScript = {};
    if (emptyScript.Update(12.0) != 0) {
        return 1;
    }

    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_beginCallCount = 0;
    g_lastUpdateTimeSec = -1.0;
    g_lastBeginTimeSec = -1.0;
    g_nextUpdateResult = 1;

    UpdateTestAction action1;
    UpdateTestAction action2;
    action1.next = &action2;

    zFMV_Script script = {};
    script.m_startTimeSec = 10.0;
    script.m_abortOnKey = 0;
    script.m_cur = &action1;

    if (script.Update(12.5) != 1 || script.m_cur != &action1 || g_updateCallCount != 1 ||
        g_lastUpdateTimeSec != 2.5 || g_endCallCount != 0 || g_beginCallCount != 0) {
        return 2;
    }

    g_nextUpdateResult = 0;
    if (script.Update(14.0) != 1 || script.m_cur != &action2 || g_updateCallCount != 2 ||
        g_lastUpdateTimeSec != 4.0 || g_endCallCount != 1 || g_beginCallCount != 1 ||
        g_lastBeginTimeSec != 4.0) {
        return 3;
    }

    return 0;
}

extern "C" int zfmv_script_update_at_time_smoke(void) {
    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_nextUpdateResult = 1;
    g_lastUpdateTimeSec = -1.0;

    UpdateTestAction action;
    zFMV_Script script = {};
    script.m_startTimeSec = 0.0;
    script.m_abortOnKey = 0;
    script.m_cur = &action;

    const int result = script.UpdateAtTime();
    return result == 1 && script.m_cur == &action && g_updateCallCount == 1 &&
                   g_lastUpdateTimeSec >= 0.0 && g_endCallCount == 0
               ? 0
               : 1;
}
