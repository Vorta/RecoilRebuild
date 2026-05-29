#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" std::int32_t g_zFMV_ActionImage_BlitRectW;
extern "C" std::int32_t g_zFMV_ActionImage_BlitRectH;

namespace {
int g_deletedCount;
std::uint32_t g_lastDeleteFlags;
int g_beginCallCount;
int g_updateCallCount;
int g_endCallCount;
double g_lastBeginTimeSec;
double g_lastUpdateTimeSec;
int g_nextUpdateResult;

struct TestAction : zFMV_Action {
    zFMV_Action *RECOIL_THISCALL Delete(std::uint32_t flags) {
        ++g_deletedCount;
        g_lastDeleteFlags = flags;
        return this;
    }

    void RECOIL_THISCALL Begin(double timeSec) {
        ++g_beginCallCount;
        g_lastBeginTimeSec = timeSec;
    }

    int RECOIL_THISCALL Update(double timeSec) {
        ++g_updateCallCount;
        g_lastUpdateTimeSec = timeSec;
        return g_nextUpdateResult;
    }

    void RECOIL_THISCALL End() {
        ++g_endCallCount;
    }
};

zFMV_Action_Vtbl MakeTestActionVtable() {
    union DeleteMemberToFunction {
        zFMV_Action *(RECOIL_THISCALL TestAction::*member)(std::uint32_t);
        zFMV_Action *(RECOIL_THISCALL *function)(zFMV_Action *, std::uint32_t);
    };

    union BeginMemberToFunction {
        void (RECOIL_THISCALL TestAction::*member)(double);
        void (RECOIL_THISCALL *function)(zFMV_Action *, double);
    };

    union UpdateMemberToFunction {
        int (RECOIL_THISCALL TestAction::*member)(double);
        int (RECOIL_THISCALL *function)(zFMV_Action *, double);
    };

    union EndMemberToFunction {
        void (RECOIL_THISCALL TestAction::*member)();
        void (RECOIL_THISCALL *function)(zFMV_Action *);
    };

    DeleteMemberToFunction deleteThunk{};
    deleteThunk.member = &TestAction::Delete;
    BeginMemberToFunction beginThunk{};
    beginThunk.member = &TestAction::Begin;
    UpdateMemberToFunction updateThunk{};
    updateThunk.member = &TestAction::Update;
    EndMemberToFunction endThunk{};
    endThunk.member = &TestAction::End;

    zFMV_Action_Vtbl vtable{};
    vtable.ScalarDeletingDestructor = deleteThunk.function;
    vtable.Update = updateThunk.function;
    vtable.Begin = beginThunk.function;
    vtable.End = endThunk.function;
    return vtable;
}

zFMV_Action_Vtbl g_testActionVtable = MakeTestActionVtable();

void SetupFmvBeginDependencies(zSndSampleSetRegistry &oldRegistry,
                               zSndSampleSet *(&oldBegin)[1],
                               zSndSampleSet &fmvSet) {
    oldRegistry = g_zSnd_SampleSetRegistry;
    oldBegin[0] = &fmvSet;
    fmvSet = {};
    fmvSet.setName = const_cast<char *>("FMV");
    g_zSnd_SampleSetRegistry.begin = oldBegin;
    g_zSnd_SampleSetRegistry.end = oldBegin + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = oldBegin + 1;
    g_zSnd_SampleSetRegistry.useArchiveBanksFlag = 0;
    g_zVideo_PrimarySurfaceState.pixels = reinterpret_cast<void *>(0x12340000);
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    g_zInput_KbdSystemReady = 0;
    g_beginCallCount = 0;
    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_lastBeginTimeSec = -1.0;
    g_lastUpdateTimeSec = -1.0;
    g_nextUpdateResult = 1;
}

void RestoreFmvBeginDependencies(const zSndSampleSetRegistry &oldRegistry) {
    g_zSnd_SampleSetRegistry = oldRegistry;
}

void WriteU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteBytes(HANDLE file, const char *value, std::uint32_t length) {
    DWORD written = 0;
    WriteFile(file, value, length, &written, nullptr);
}

void WriteStringNode(HANDLE file, const char *value) {
    const std::uint32_t length = static_cast<std::uint32_t>(std::strlen(value));
    WriteU32(file, zReader::ZRDR_NODE_STRING);
    WriteU32(file, length);
    WriteBytes(file, value, length);
}

void WriteIntNode(HANDLE file, std::int32_t value) {
    WriteU32(file, zReader::ZRDR_NODE_INT);
    WriteU32(file, static_cast<std::uint32_t>(value));
}

void WriteFloatNode(HANDLE file, float value) {
    union FloatBits {
        float f32;
        std::uint32_t u32;
    };

    FloatBits bits{};
    bits.f32 = value;
    WriteU32(file, zReader::ZRDR_NODE_FLOAT);
    WriteU32(file, bits.u32);
}

void WriteArrayHeader(HANDLE file, std::int32_t count) {
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, static_cast<std::uint32_t>(count));
}

template <typename T> T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}
} // namespace

extern "C" int zfmv_script_reset_smoke(void) {
    g_deletedCount = 0;
    g_lastDeleteFlags = 0;

    TestAction action1{{&g_testActionVtable, nullptr}};
    TestAction action2{{&g_testActionVtable, nullptr}};
    action1.next = &action2;

    zFMV_Script script{};
    script.m_head = &action1;
    script.m_tail = &action2;
    script.m_cur = nullptr;

    script.Reset(0);
    if (script.m_head != &action1 || script.m_tail != &action2 || script.m_cur != &action1 ||
        g_deletedCount != 0) {
        return 1;
    }

    script.Reset(1);
    if (script.m_head != nullptr || script.m_tail != nullptr || script.m_cur != nullptr) {
        return 2;
    }

    return g_deletedCount == 2 && g_lastDeleteFlags == 1 ? 0 : 3;
}

extern "C" int zfmv_script_init_null_path_smoke(void) {
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);

    zFMV_Script script{};
    script.m_fmvPath = reinterpret_cast<char *>(0x11111111);
    script.m_hWnd = reinterpret_cast<HWND>(0x22222222);
    script.m_abortOnKey = 0;
    script.m_head = reinterpret_cast<zFMV_Action *>(0x33333333);
    script.m_tail = reinterpret_cast<zFMV_Action *>(0x44444444);
    script.m_cur = reinterpret_cast<zFMV_Action *>(0x55555555);

    zFMV_Script *returned = script.Init(nullptr, nullptr, nullptr);
    if (returned != &script) {
        return 1;
    }

    if (script.m_hWnd != reinterpret_cast<HWND>(0x12345678) || script.m_abortOnKey != 1 ||
        script.m_fmvPath != nullptr || script.m_head != nullptr || script.m_tail != nullptr ||
        script.m_cur != nullptr) {
        return 2;
    }

    returned = script.Init(nullptr, nullptr, reinterpret_cast<HWND>(0x87654321));
    return returned == &script && script.m_hWnd == reinterpret_cast<HWND>(0x87654321) ? 0 : 3;
}

extern "C" int zfmv_script_cleanup_smoke(void) {
    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_fmvPath = static_cast<char *>(std::malloc(4));
    script.m_head = &action;
    script.m_tail = &action;
    script.m_cur = &action;

    if (script.m_fmvPath == nullptr) {
        return 1;
    }

    g_deletedCount = 0;
    script.Cleanup();

    if (script.m_fmvPath != nullptr || script.m_head != nullptr || script.m_tail != nullptr ||
        script.m_cur != nullptr) {
        return 2;
    }

    return g_deletedCount == 1 && g_lastDeleteFlags == 1 ? 0 : 3;
}

extern "C" int zfmv_script_append_action_smoke(void) {
    zFMV_Script script{};
    TestAction action1{{&g_testActionVtable, reinterpret_cast<zFMV_Action *>(0x11111111)}};
    TestAction action2{{&g_testActionVtable, reinterpret_cast<zFMV_Action *>(0x22222222)}};

    if (script.AppendAction(nullptr) != 0 || script.m_head != nullptr || script.m_tail != nullptr ||
        script.m_cur != nullptr) {
        return 1;
    }

    if (script.AppendAction(&action1) != 1 || action1.next != nullptr ||
        script.m_head != &action1 || script.m_tail != &action1 || script.m_cur != &action1) {
        return 2;
    }

    if (script.AppendAction(&action2) != 1 || action1.next != &action2 || action2.next != nullptr ||
        script.m_head != &action1 || script.m_tail != &action2 || script.m_cur != &action1) {
        return 3;
    }

    return 0;
}

extern "C" int zfmv_script_run_blocking_empty_smoke(void) {
    zFMV_Script script{};
    script.m_abortOnKey = 0;

    const std::int32_t result = script.RunBlocking(1);
    const bool emptyOk = result == 1 && script.m_abortOnKey == 1 && script.m_cur == nullptr;

    zSndSampleSetRegistry oldRegistry{};
    zSndSampleSet *sampleSetSlots[1] = {};
    zSndSampleSet fmvSet{};
    SetupFmvBeginDependencies(oldRegistry, sampleSetSlots, fmvSet);

    TestAction action{{&g_testActionVtable, nullptr}};
    script = {};
    script.m_head = &action;
    script.m_tail = &action;
    script.m_cur = &action;
    g_nextUpdateResult = 0;

    const int actionResult = script.RunBlocking(0);
    const bool actionOk =
        actionResult == 1 && script.m_abortOnKey == 0 && script.m_cur == &action &&
        g_beginCallCount == 1 && g_updateCallCount == 1 && g_endCallCount == 1 &&
        fmvSet.resourcesLoaded == 1;

    RestoreFmvBeginDependencies(oldRegistry);
    if (!emptyOk) {
        return 1;
    }
    return actionOk ? 0 : 2;
}

extern "C" int zfmv_script_begin_current_action_smoke(void) {
    zFMV_Script emptyScript{};
    if (emptyScript.BeginCurrentAction(12.5) != 0) {
        return 1;
    }

    zSndSampleSetRegistry oldRegistry{};
    zSndSampleSet *sampleSetSlots[1] = {};
    zSndSampleSet fmvSet{};
    SetupFmvBeginDependencies(oldRegistry, sampleSetSlots, fmvSet);

    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_cur = &action;
    const int result = script.BeginCurrentAction(42.25);

    const bool ok = result == 1 && script.m_startTimeSec == 42.25 && g_beginCallCount == 1 &&
                    g_lastBeginTimeSec == 0.0 &&
                    g_zVideo_FxSurfacePixels16 ==
                        reinterpret_cast<unsigned short *>(0x12340000) &&
                    g_zVideo_FxSurfaceWidth == 320 && g_zVideo_FxSurfaceHeight == 200 &&
                    g_zVideo_FxSurfacePitchBytes == 640 &&
                    g_zVideo_FxSurfacePitchPixels16 == 320 && fmvSet.resourcesLoaded == 1;

    RestoreFmvBeginDependencies(oldRegistry);
    return ok ? 0 : 2;
}

extern "C" int zfmv_script_begin_at_time_smoke(void) {
    zSndSampleSetRegistry oldRegistry{};
    zSndSampleSet *sampleSetSlots[1] = {};
    zSndSampleSet fmvSet{};
    SetupFmvBeginDependencies(oldRegistry, sampleSetSlots, fmvSet);

    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_cur = &action;
    const int result = script.BeginAtTime();

    const bool ok = result == 1 && script.m_startTimeSec >= 0.0 && g_beginCallCount == 1 &&
                    g_lastBeginTimeSec == 0.0 && fmvSet.resourcesLoaded == 1;

    RestoreFmvBeginDependencies(oldRegistry);
    return ok ? 0 : 1;
}

extern "C" int zfmv_script_update_smoke(void) {
    zFMV_Script emptyScript{};
    if (emptyScript.Update(12.0) != 0) {
        return 1;
    }

    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_beginCallCount = 0;
    g_lastUpdateTimeSec = -1.0;
    g_lastBeginTimeSec = -1.0;
    g_nextUpdateResult = 1;

    TestAction action1{{&g_testActionVtable, nullptr}};
    TestAction action2{{&g_testActionVtable, nullptr}};
    action1.next = &action2;

    zFMV_Script script{};
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

    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_startTimeSec = 0.0;
    script.m_abortOnKey = 0;
    script.m_cur = &action;

    const int result = script.UpdateAtTime();
    return result == 1 && script.m_cur == &action && g_updateCallCount == 1 &&
                   g_lastUpdateTimeSec >= 0.0 && g_endCallCount == 0
               ? 0
               : 1;
}

extern "C" int zfmv_script_begin_now_smoke(void) {
    g_deletedCount = 0;

    TestAction action1{{&g_testActionVtable, nullptr}};
    TestAction action2{{&g_testActionVtable, nullptr}};
    action1.next = &action2;

    zFMV_Script script{};
    script.m_head = &action1;
    script.m_tail = &action2;
    script.m_cur = nullptr;

    script.BeginNow(0);
    if (script.m_head != &action1 || script.m_tail != &action2 || script.m_cur != &action1 ||
        g_deletedCount != 0) {
        return 1;
    }

    script.BeginNow(1);
    return script.m_head == nullptr && script.m_tail == nullptr && script.m_cur == nullptr &&
                   g_deletedCount == 2
               ? 0
               : 2;
}

extern "C" int zfmv_action_image_constructor_with_screen_rect_smoke(void) {
    g_zFMV_ActionImage_BlitRectW = 640;
    g_zFMV_ActionImage_BlitRectH = 480;

    zFMV_ActionImage action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.image = reinterpret_cast<void *>(0x22222222);

    zFMV_ActionImage *returned = action.ConstructorWithScreenRect("screen.raw", 7, 32, 48);

    const bool ok =
        returned == &action && reinterpret_cast<std::uintptr_t>(action.vftable) == 0x4d2598 &&
        action.next == nullptr && action.image == nullptr && action.imagePath != nullptr &&
        std::strcmp(action.imagePath, "screen.raw") == 0 && action.doAdjustSurfaces == 7 &&
        action.forcePrimaryPostprocess == 1 && action.blitRect[0] == 32 &&
        action.blitRect[1] == 48 && action.blitRect[2] == 640 && action.blitRect[3] == 480;

    std::free(action.imagePath);
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_image_constructor_scaled_smoke(void) {
    zRndr::g_activeRegionWidth = 800;
    zRndr::g_activeRegionHeight = 600;
    zRndr::g_pitchBytes = 1600;
    zRndr::g_bytesPerPixel = 2;

    zFMV_ActionImage action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.image = reinterpret_cast<void *>(0x22222222);

    zFMV_ActionImage *returned = action.ConstructorScaled("scaled.raw", 3);
    const bool ok =
        returned == &action && reinterpret_cast<std::uintptr_t>(action.vftable) == 0x4d2598 &&
        action.next == nullptr && action.image == nullptr && action.imagePath != nullptr &&
        std::strcmp(action.imagePath, "scaled.raw") == 0 && action.doAdjustSurfaces == 3 &&
        action.forcePrimaryPostprocess == 0 && action.blitRect[0] == 0 && action.blitRect[1] == 0 &&
        action.blitRect[2] == 800 && action.blitRect[3] == 600;

    std::free(action.imagePath);
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_fade_constructor_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    zFMV_ActionFade action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.reserved0e = 0x7777;
    action.startSec = 123.0;
    action.capturedFrame = reinterpret_cast<void *>(0x22222222);

    zFMV_ActionFade *returned = action.Constructor(0xff, 0x80, 0x20, 0x3fc00000, -1, 128);

    return returned == &action && reinterpret_cast<std::uintptr_t>(action.vftable) == 0x4d25b0 &&
                   action.next == nullptr && action.fadeDirectionSign == -1 &&
                   action.fadeColorPacked16 == 0xfc04 && action.reserved0e == 0x7777 &&
                   action.durationSecRaw == 0x3fc00000 && action.startSec == 123.0 &&
                   action.capturedFrame == reinterpret_cast<void *>(0x22222222) &&
                   action.maxAlpha == 128
               ? 0
               : 1;
}

extern "C" int zfmv_playback_init_smoke(void) {
    zFMV_Playback playback{};
    playback.mciPutFlags = 0x77777777;
    playback.notifyHwnd = reinterpret_cast<HWND>(0x11111111);
    playback.mediaPathDup = reinterpret_cast<char *>(0x22222222);

    zFMV_Playback *returned = playback.Init("movie.avi", reinterpret_cast<HWND>(0x12345678));

    const bool ok = returned == &playback && playback.mediaPathDup != nullptr &&
                    std::strcmp(playback.mediaPathDup, "movie.avi") == 0 &&
                    playback.notifyHwnd == reinterpret_cast<HWND>(0x12345678) &&
                    playback.mciPutFlags == 0;

    std::free(playback.mediaPathDup);
    return ok ? 0 : 1;
}

extern "C" int zfmv_playback_set_dest_rect_smoke(void) {
    zFMV_Playback playback{};
    playback.mciPutFlags = 0x10;
    const zFMV_Rect rect{1, 2, 3, 4};

    const std::int32_t result = playback.SetDestRect(&rect);

    return result == 0x40010 && playback.mciPutFlags == 0x40010 &&
                   playback.destinationRect.left == 1 && playback.destinationRect.top == 2 &&
                   playback.destinationRect.right == 3 && playback.destinationRect.bottom == 4
               ? 0
               : 1;
}

extern "C" int zfmv_playback_destructor_smoke(void) {
    zFMV_Playback playback{};
    playback.mediaPathDup = static_cast<char *>(std::malloc(4));
    if (playback.mediaPathDup == nullptr) {
        return 1;
    }

    std::strcpy(playback.mediaPathDup, "x");
    playback.Destructor();
    playback.mediaPathDup = nullptr;
    playback.Destructor();
    return 0;
}

extern "C" int zfmv_playback_report_mci_error_smoke(void) {
    zFMV_Playback playback{};
    return playback.ReportMciError(0xffffffffu) == 0 ? 0 : 1;
}

extern "C" int zfmv_stream_destructor_empty_smoke(void) {
    alignas(8) std::uint8_t storage[0x1d4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    TestFieldAt<char *>(stream, 0x38) = static_cast<char *>(std::malloc(4));
    if (TestFieldAt<char *>(stream, 0x38) == nullptr) {
        return 1;
    }

    std::strcpy(TestFieldAt<char *>(stream, 0x38), "x");
    InitializeCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));

    stream->Destructor();
    return 0;
}

extern "C" int zfmv_stream_constructor_missing_file_smoke(void) {
    alignas(8) std::uint8_t storage[0x1d4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    TestFieldAt<char *>(stream, 0x38) = const_cast<char *>("__missing_stream_ctor__.avi");
    TestFieldAt<std::int32_t>(stream, 0x104) = 0x12345678;

    stream->Constructor();

    return TestFieldAt<std::int32_t>(stream, 0x104) == 0 &&
                   TestFieldAt<std::int32_t>(stream, 0x3c) == 0
               ? 0
               : 1;
}

extern "C" int zfmv_stream_open_audio_missing_file_smoke(void) {
    alignas(8) std::uint8_t storage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    TestFieldAt<char *>(stream, 0x38) = const_cast<char *>("__missing_stream_audio__.avi");
    TestFieldAt<void *>(stream, 0x134) = reinterpret_cast<void *>(0x11111111);
    TestFieldAt<std::int32_t>(stream, 0x130) = 0x22222222;
    TestFieldAt<std::int32_t>(stream, 0x1e0) = 5;

    stream->OpenAudio();

    return TestFieldAt<void *>(stream, 0x134) == nullptr &&
                   TestFieldAt<std::int32_t>(stream, 0x130) == 0x22222222 &&
                   TestFieldAt<std::int32_t>(stream, 0x1e0) == 5
               ? 0
               : 1;
}

extern "C" int zfmv_stream_init_missing_file_smoke(void) {
    alignas(8) std::uint8_t storage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    zFMV_Stream *const returned = stream->Init("__missing_stream_init__.avi", 7);
    const bool ok =
        returned == stream && TestFieldAt<char *>(stream, 0x38) != nullptr &&
        std::strcmp(TestFieldAt<char *>(stream, 0x38), "__missing_stream_init__.avi") == 0 &&
        TestFieldAt<std::int32_t>(stream, 0x1d4) == 1 &&
        TestFieldAt<std::int32_t>(stream, 0x1e0) == 7 &&
        TestFieldAt<std::int32_t>(stream, 0x130) == 0 &&
        TestFieldAt<std::int32_t>(stream, 0x3c) == 0 &&
        TestFieldAt<std::int32_t>(stream, 0x104) == 0;

    stream->Destructor();
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_play_avi_constructor_existing_file_smoke(void) {
    const char *fileName = "recoil_playavi_ctor_smoke.tmp";
    FILE *file = std::fopen(fileName, "wb");
    if (file == nullptr) {
        return 1;
    }
    std::fclose(file);

    zFMV_ActionPlayAvi action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);

    zFMV_ActionPlayAvi *returned = action.Constructor(".", fileName, 5);
    const bool ok = returned == &action &&
                    reinterpret_cast<std::uintptr_t>(action.vftable) == 0x4d25c8 &&
                    action.next == nullptr && action.mediaPath != nullptr &&
                    std::strcmp(action.mediaPath, ".\\recoil_playavi_ctor_smoke.tmp") == 0 &&
                    action.modeFlags == 5;

    std::free(action.mediaPath);
    std::remove(fileName);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_play_mci_constructor_smoke(void) {
    zRndr::g_activeRegionWidth = 1024;
    zRndr::g_activeRegionHeight = 768;
    zRndr::g_pitchBytes = 2048;
    zRndr::g_bytesPerPixel = 2;

    zFMV_ActionPlayMci action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);

    zFMV_ActionPlayMci *returned =
        action.Constructor("movies", "intro.mci", reinterpret_cast<HWND>(0x2468ace0));

    const bool ok =
        returned == &action && reinterpret_cast<std::uintptr_t>(action.vftable) == 0x4d25f8 &&
        action.next == nullptr && action.mediaPath != nullptr &&
        std::strcmp(action.mediaPath, "movies\\intro.mci") == 0 && action.playback != nullptr &&
        action.playback->mediaPathDup != nullptr &&
        std::strcmp(action.playback->mediaPathDup, "movies\\intro.mci") == 0 &&
        action.playback->notifyHwnd == reinterpret_cast<HWND>(0x2468ace0) &&
        action.playback->mciPutFlags == 0x40000 && action.playback->destinationRect.left == 0 &&
        action.playback->destinationRect.top == 0 &&
        action.playback->destinationRect.right == 1024 &&
        action.playback->destinationRect.bottom == 768 && g_zFMV_ActionPlayMci_DestRect.left == 0 &&
        g_zFMV_ActionPlayMci_DestRect.top == 0 && g_zFMV_ActionPlayMci_DestRect.right == 1024 &&
        g_zFMV_ActionPlayMci_DestRect.bottom == 768;

    std::free(action.playback->mediaPathDup);
    ::operator delete(action.playback);
    std::free(action.mediaPath);
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_blur_constructor_smoke(void) {
    zFMV_ActionBlur action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.framesRemaining = 0x22222222;
    action.blurPassCount = 0x33333333;
    action.swSurfaceRect = {1, 2, 3, 4};
    action.primarySurfaceRect = {5, 6, 7, 8};

    zFMV_ActionBlur *returned = action.Constructor(12, 3);

    return returned == &action && action.vftable == &g_zFMV_ActionBlur_Vtable &&
                   action.next == nullptr && action.framesRemaining == 12 &&
                   action.blurPassCount == 3 && action.swSurfaceRect.left == 1 &&
                   action.swSurfaceRect.top == 2 && action.swSurfaceRect.right == 3 &&
                   action.swSurfaceRect.bottom == 4 && action.primarySurfaceRect.left == 5 &&
                   action.primarySurfaceRect.top == 6 && action.primarySurfaceRect.right == 7 &&
                   action.primarySurfaceRect.bottom == 8
               ? 0
               : 1;
}

extern "C" int zfmv_script_load_actions_from_zrd_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "fmv", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteArrayHeader(file, 7);
    WriteStringNode(file, "FMV_PATH");
    WriteStringNode(file, "movies");
    WriteStringNode(file, "IMAGE_PATH");
    WriteStringNode(file, "images");
    WriteStringNode(file, "INTRO");
    WriteArrayHeader(file, 4);
    WriteArrayHeader(file, 3);
    WriteStringNode(file, "WAIT");
    WriteFloatNode(file, 1.25f);
    WriteArrayHeader(file, 3);
    WriteStringNode(file, "BLURH");
    WriteIntNode(file, 4);
    WriteArrayHeader(file, 3);
    WriteStringNode(file, "PLAYSOUND");
    WriteStringNode(file, "intro_whoosh");
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "fmv.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;
    g_zArchive_MountedList = &list;

    zFMV_Script script{};
    script.Init(nullptr, nullptr, nullptr);
    const std::int32_t result = script.LoadActionsFromZrd("C:\\dummy\\fmv.zrd", "INTRO");

    auto *wait = static_cast<zFMV_ActionWait *>(script.m_head);
    auto *blur = static_cast<zFMV_ActionBlur *>(wait != nullptr ? wait->next : nullptr);
    auto *sound = static_cast<zFMV_ActionPlaySound *>(blur != nullptr ? blur->next : nullptr);

    const bool ok = result == 3 && script.m_fmvPath != nullptr &&
                    std::strcmp(script.m_fmvPath, "movies") == 0 && wait != nullptr &&
                    wait->vftable == &g_zFMV_ActionWait_Vtable && wait->durationSec == 1.25f &&
                    blur != nullptr && blur->vftable == &g_zFMV_ActionBlurH_Vtable &&
                    blur->framesRemaining == 1 && blur->blurPassCount == 4 && sound != nullptr &&
                    sound->vftable == &g_zFMV_ActionPlaySound_Vtable &&
                    std::strcmp(sound->sampleName, "intro_whoosh") == 0 &&
                    sound->voice == nullptr && sound->next == nullptr;

    script.Cleanup();
    g_zArchive_MountedList = nullptr;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}

extern "C" int zfmv_action_wait_begin_update_smoke(void) {
    zFMV_ActionWait action{};
    action.durationSec = 2.5f;
    action.startSec = -1.0f;

    action.Begin(10.25);

    return action.startSec == 10.25f && action.Update(12.0) == 1 && action.Update(12.75) == 0 ? 0
                                                                                              : 1;
}

extern "C" int zfmv_action_base_destructor_smoke(void) {
    zFMV_Action action{};
    action.vftable = &g_zFMV_ActionWait_Vtable;
    action.next = reinterpret_cast<zFMV_Action *>(0x1234);

    action.Destructor();
    if (action.vftable != &g_zFMV_ActionBase_Vtable ||
        action.next != reinterpret_cast<zFMV_Action *>(0x1234)) {
        return 1;
    }

    action.vftable = &g_zFMV_ActionWait_Vtable;
    return action.ScalarDeletingDestructor(0) == &action &&
                   action.vftable == &g_zFMV_ActionBase_Vtable
               ? 0
               : 2;
}

extern "C" int zfmv_action_derived_scalar_deleting_destructor_smoke(void) {
    zFMV_ActionWait action{};
    action.vftable = &g_zFMV_ActionWait_Vtable;

    return action.DerivedScalarDeletingDestructor(0) == &action &&
                   action.vftable == &g_zFMV_ActionBase_Vtable
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_sound_begin_missing_sample_smoke(void) {
    zFMV_ActionPlaySound action{};
    action.vftable = &g_zFMV_ActionPlaySound_Vtable;
    std::strcpy(action.sampleName, "__missing_fmv_sample__");
    action.sample = reinterpret_cast<zSndSample *>(0x1234);
    action.voice = nullptr;

    action.Begin(0.0);

    return action.sample == nullptr && action.voice == nullptr ? 0 : 1;
}
