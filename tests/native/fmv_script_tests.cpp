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
#include <vfw.h>

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" std::int32_t g_zFMV_ActionImage_BlitRectW;
extern "C" std::int32_t g_zFMV_ActionImage_BlitRectH;

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

int g_deletedCount;
std::uint32_t g_lastDeleteFlags;
int g_beginCallCount;
int g_updateCallCount;
int g_endCallCount;
double g_lastBeginTimeSec;
double g_lastUpdateTimeSec;
int g_nextUpdateResult;
int g_fakeFmvMciSendCommandCount;
MCIDEVICEID g_fakeFmvMciDevices[4];
UINT g_fakeFmvMciMessages[4];
DWORD_PTR g_fakeFmvMciFlags[4];
DWORD_PTR g_fakeFmvMciParams[4];
MCIERROR g_fakeFmvMciReturns[4];
zFMV_Playback *g_fakeFmvExpectedClosePlayback;
int g_fakeFmvCloseParamOk;
int g_fakeAviStreamReadCount;
PAVISTREAM g_fakeAviStreams[4];
LONG g_fakeAviStarts[4];
LONG g_fakeAviSamples[4];
void *g_fakeAviBuffers[4];
LONG g_fakeAviBufferBytes[4];
HRESULT g_fakeAviReturn;
int g_fakeIcDecompressCount;
HIC g_fakeIcLastCodec;
DWORD g_fakeIcLastFlags;
BITMAPINFOHEADER *g_fakeIcLastSrcFormat;
void *g_fakeIcLastCompressedFrame;
BITMAPINFOHEADER *g_fakeIcLastDstFormat;
void *g_fakeIcLastPixels;
LRESULT g_fakeIcReturn;
int g_fakeFmvLockCount;
int g_fakeFmvUnlockCount;
int g_fakeFmvGetCurrentPositionCount;
unsigned int g_fakeFmvLastLockOffset;
unsigned int g_fakeFmvLastLockBytes;
unsigned int g_fakeFmvLastLockFlags;
int g_fakeFmvLockResult;
int g_fakeFmvUnlockResult;
void *g_fakeFmvLockPtr1;
void *g_fakeFmvLockPtr2;
int g_fakeFmvLockBytes1;
int g_fakeFmvLockBytes2;
std::uint32_t g_fakeFmvPlayCursor;
void *g_fakeFmvUnlockPtr1;
void *g_fakeFmvUnlockPtr2;
int g_fakeFmvUnlockBytes1;
int g_fakeFmvUnlockBytes2;

MCIERROR WINAPI FakeFmvMciSendCommandA(MCIDEVICEID deviceId, UINT message, DWORD_PTR flags,
                                       DWORD_PTR params) {
    const int index = g_fakeFmvMciSendCommandCount;
    if (index < 4) {
        g_fakeFmvMciDevices[index] = deviceId;
        g_fakeFmvMciMessages[index] = message;
        g_fakeFmvMciFlags[index] = flags;
        g_fakeFmvMciParams[index] = params;
    }
    ++g_fakeFmvMciSendCommandCount;

    if (message == 0x804 && params != 0) {
        zFMV_Playback *const closePlayback =
            *reinterpret_cast<zFMV_Playback *const *>(params);
        g_fakeFmvCloseParamOk =
            closePlayback == g_fakeFmvExpectedClosePlayback ? 1 : 0;
    }

    return index < 4 ? g_fakeFmvMciReturns[index] : 0;
}

HRESULT WINAPI FakeFmvAVIStreamRead(PAVISTREAM stream, LONG start, LONG samples, void *buffer,
                                    LONG bufferBytes, LONG *, LONG *) {
    const int index = g_fakeAviStreamReadCount;
    if (index < 4) {
        g_fakeAviStreams[index] = stream;
        g_fakeAviStarts[index] = start;
        g_fakeAviSamples[index] = samples;
        g_fakeAviBuffers[index] = buffer;
        g_fakeAviBufferBytes[index] = bufferBytes;
    }
    ++g_fakeAviStreamReadCount;
    return g_fakeAviReturn;
}

LRESULT __cdecl FakeFmvICDecompress(HIC codec, DWORD flags, BITMAPINFOHEADER *srcFormat,
                                    void *compressedFrame, BITMAPINFOHEADER *dstFormat,
                                    void *pixels) {
    ++g_fakeIcDecompressCount;
    g_fakeIcLastCodec = codec;
    g_fakeIcLastFlags = flags;
    g_fakeIcLastSrcFormat = srcFormat;
    g_fakeIcLastCompressedFrame = compressedFrame;
    g_fakeIcLastDstFormat = dstFormat;
    g_fakeIcLastPixels = pixels;
    return g_fakeIcReturn;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        patch.address = nullptr;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = nullptr;
}

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

using TestFmvGetCurrentPositionFn = std::int32_t(__stdcall *)(void *self,
                                                              std::uint32_t *playCursor,
                                                              std::uint32_t *writeCursor);
using TestFmvLockFn = std::int32_t(__stdcall *)(void *self, std::uint32_t offset,
                                                std::uint32_t bytes, void **outPtr1,
                                                std::int32_t *outBytes1, void **outPtr2,
                                                std::int32_t *outBytes2, std::uint32_t flags);
using TestFmvUnlockFn = std::int32_t(__stdcall *)(void *self, void *ptr1,
                                                  std::int32_t bytes1, void *ptr2,
                                                  std::int32_t bytes2);

struct TestFmvDirectSoundBufferVTable {
    void *slots00_0c[4];
    TestFmvGetCurrentPositionFn GetCurrentPosition;
    void *slots14_28[6];
    TestFmvLockFn Lock;
    void *slots30_48[7];
    TestFmvUnlockFn Unlock;
};

struct TestFmvDirectSoundBuffer {
    TestFmvDirectSoundBufferVTable *vtable;
};

std::int32_t __stdcall TestFmvGetCurrentPosition(void *, std::uint32_t *playCursor,
                                                 std::uint32_t *writeCursor) {
    ++g_fakeFmvGetCurrentPositionCount;
    *playCursor = g_fakeFmvPlayCursor;
    *writeCursor = 0;
    return 0;
}

std::int32_t __stdcall TestFmvLockSoundBuffer(void *, std::uint32_t offset,
                                              std::uint32_t bytes, void **outPtr1,
                                              std::int32_t *outBytes1, void **outPtr2,
                                              std::int32_t *outBytes2, std::uint32_t flags) {
    ++g_fakeFmvLockCount;
    g_fakeFmvLastLockOffset = offset;
    g_fakeFmvLastLockBytes = bytes;
    g_fakeFmvLastLockFlags = flags;
    *outPtr1 = g_fakeFmvLockPtr1;
    *outBytes1 = g_fakeFmvLockBytes1;
    *outPtr2 = g_fakeFmvLockPtr2;
    *outBytes2 = g_fakeFmvLockBytes2;
    return g_fakeFmvLockResult;
}

std::int32_t __stdcall TestFmvUnlockSoundBuffer(void *, void *ptr1, std::int32_t bytes1,
                                                void *ptr2, std::int32_t bytes2) {
    ++g_fakeFmvUnlockCount;
    g_fakeFmvUnlockPtr1 = ptr1;
    g_fakeFmvUnlockBytes1 = bytes1;
    g_fakeFmvUnlockPtr2 = ptr2;
    g_fakeFmvUnlockBytes2 = bytes2;
    return g_fakeFmvUnlockResult;
}

void ResetFmvAudioFillFakes(void *ptr1, int bytes1, void *ptr2, int bytes2) {
    g_fakeAviStreamReadCount = 0;
    std::memset(g_fakeAviStreams, 0, sizeof(g_fakeAviStreams));
    std::memset(g_fakeAviStarts, 0, sizeof(g_fakeAviStarts));
    std::memset(g_fakeAviSamples, 0, sizeof(g_fakeAviSamples));
    std::memset(g_fakeAviBuffers, 0, sizeof(g_fakeAviBuffers));
    std::memset(g_fakeAviBufferBytes, 0, sizeof(g_fakeAviBufferBytes));
    g_fakeAviReturn = 0;
    g_fakeIcDecompressCount = 0;
    g_fakeIcLastCodec = 0;
    g_fakeIcLastFlags = 0;
    g_fakeIcLastSrcFormat = 0;
    g_fakeIcLastCompressedFrame = 0;
    g_fakeIcLastDstFormat = 0;
    g_fakeIcLastPixels = 0;
    g_fakeIcReturn = 0;
    g_fakeFmvLockCount = 0;
    g_fakeFmvUnlockCount = 0;
    g_fakeFmvGetCurrentPositionCount = 0;
    g_fakeFmvLastLockOffset = 0;
    g_fakeFmvLastLockBytes = 0;
    g_fakeFmvLastLockFlags = 0xffffffffu;
    g_fakeFmvLockResult = 0;
    g_fakeFmvUnlockResult = 0;
    g_fakeFmvLockPtr1 = ptr1;
    g_fakeFmvLockPtr2 = ptr2;
    g_fakeFmvLockBytes1 = bytes1;
    g_fakeFmvLockBytes2 = bytes2;
    g_fakeFmvPlayCursor = 0;
    g_fakeFmvUnlockPtr1 = 0;
    g_fakeFmvUnlockPtr2 = 0;
    g_fakeFmvUnlockBytes1 = 0;
    g_fakeFmvUnlockBytes2 = 0;
}

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

extern "C" int zfmv_playback_stop_and_close_smoke(void) {
    CodeFunctionPatch mciPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&mciSendCommandA),
                           reinterpret_cast<void *>(&FakeFmvMciSendCommandA), mciPatch)) {
        return 1;
    }

    zFMV_Playback playback{};
    playback.mciDeviceId = 0x3456;
    g_fakeFmvExpectedClosePlayback = &playback;
    g_fakeFmvCloseParamOk = 0;
    g_fakeFmvMciSendCommandCount = 0;
    for (int index = 0; index < 4; ++index) {
        g_fakeFmvMciDevices[index] = 0;
        g_fakeFmvMciMessages[index] = 0;
        g_fakeFmvMciFlags[index] = 0;
        g_fakeFmvMciParams[index] = 0;
        g_fakeFmvMciReturns[index] = 0;
    }

    playback.StopAndClose();
    const bool successSequence =
        g_fakeFmvMciSendCommandCount == 2 && g_fakeFmvMciDevices[0] == 0x3456 &&
        g_fakeFmvMciMessages[0] == 0x808 && g_fakeFmvMciFlags[0] == 2 &&
        g_fakeFmvMciParams[0] == 0 && g_fakeFmvMciDevices[1] == 0x3456 &&
        g_fakeFmvMciMessages[1] == 0x804 && g_fakeFmvMciFlags[1] == 2 &&
        g_fakeFmvCloseParamOk == 1;

    g_fakeFmvMciSendCommandCount = 0;
    g_fakeFmvMciReturns[0] = 0x1234;
    g_fakeFmvMciReturns[1] = 0;
    playback.StopAndClose();
    const bool stopFailureSkipsClose =
        g_fakeFmvMciSendCommandCount == 1 && g_fakeFmvMciMessages[0] == 0x808;

    RestoreFunctionPatch(mciPatch);
    return successSequence && stopFailureSkipsClose ? 0 : 2;
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

extern "C" int zfmv_stream_fill_audio_buffer_smoke(void) {
    CodeFunctionPatch aviPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamRead),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamRead), aviPatch)) {
        return 1;
    }

    const int oldBackend = g_zSnd_ActiveBackend;
    g_zSnd_ActiveBackend = 0;

    unsigned char streamStorage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    zSndSample sample = {};
    TestFmvDirectSoundBufferVTable bufferVTable = {};
    bufferVTable.Lock = &TestFmvLockSoundBuffer;
    bufferVTable.Unlock = &TestFmvUnlockSoundBuffer;
    TestFmvDirectSoundBuffer soundBuffer{&bufferVTable};
    unsigned char span1[8] = {};
    unsigned char span2[8] = {};
    PAVISTREAM const aviStream = reinterpret_cast<PAVISTREAM>(0x13572468);

    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&soundBuffer);
    TestFieldAt<PAVISTREAM>(stream, 0x134) = aviStream;
    TestFieldAt<std::uint32_t>(stream, 0x168) = 2;
    TestFieldAt<zSndSample *>(stream, 0x1d0) = &sample;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    g_fakeFmvLockResult = 0x12345678;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 5;
    const bool lockFailure =
        stream->FillAudioBuffer(10, 12) == 0 && g_fakeFmvLockCount == 1 &&
        g_fakeFmvUnlockCount == 0 && g_fakeAviStreamReadCount == 0 &&
        TestFieldAt<std::uint32_t>(stream, 0x1d8) == 5;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 5;
    const bool firstSpan =
        stream->FillAudioBuffer(10, 12) == 1 && g_fakeFmvLockCount == 1 &&
        g_fakeFmvUnlockCount == 1 && g_fakeFmvLastLockOffset == 10 &&
        g_fakeFmvLastLockBytes == 12 && g_fakeFmvLastLockFlags == 0 &&
        g_fakeAviStreamReadCount == 1 && g_fakeAviStreams[0] == aviStream &&
        g_fakeAviStarts[0] == 5 && g_fakeAviSamples[0] == 2 &&
        g_fakeAviBuffers[0] == span1 && g_fakeAviBufferBytes[0] == 4 &&
        g_fakeFmvUnlockPtr1 == span1 && g_fakeFmvUnlockBytes1 == 4 &&
        g_fakeFmvUnlockPtr2 == nullptr && g_fakeFmvUnlockBytes2 == 0 &&
        TestFieldAt<std::uint32_t>(stream, 0x1d8) == 7;

    ResetFmvAudioFillFakes(span1, 4, span2, 6);
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 3;
    const bool wrappedSpans =
        stream->FillAudioBuffer(20, 14) == 1 && g_fakeAviStreamReadCount == 2 &&
        g_fakeAviStarts[0] == 3 && g_fakeAviSamples[0] == 2 &&
        g_fakeAviBuffers[0] == span1 && g_fakeAviBufferBytes[0] == 4 &&
        g_fakeAviStarts[1] == 5 && g_fakeAviSamples[1] == 3 &&
        g_fakeAviBuffers[1] == span2 && g_fakeAviBufferBytes[1] == 6 &&
        g_fakeFmvUnlockPtr1 == span1 && g_fakeFmvUnlockBytes1 == 4 &&
        g_fakeFmvUnlockPtr2 == span2 && g_fakeFmvUnlockBytes2 == 6 &&
        TestFieldAt<std::uint32_t>(stream, 0x1d8) == 7;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    g_fakeAviReturn = 0x80004005;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 9;
    const bool aviFailureStillUnlocks =
        stream->FillAudioBuffer(0, 4) == 1 && g_fakeAviStreamReadCount == 1 &&
        g_fakeFmvUnlockCount == 1 && TestFieldAt<std::uint32_t>(stream, 0x1d8) == 11;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    g_fakeFmvUnlockResult = 0x12345678;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 1;
    const bool unlockFailure =
        stream->FillAudioBuffer(0, 4) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeFmvUnlockCount == 1 && TestFieldAt<std::uint32_t>(stream, 0x1d8) == 3;

    g_zSnd_ActiveBackend = oldBackend;
    RestoreFunctionPatch(aviPatch);
    return lockFailure && firstSpan && wrappedSpans && aviFailureStillUnlocks && unlockFailure
               ? 0
               : 2;
}

extern "C" int zfmv_stream_read_and_decode_frame_smoke(void) {
    CodeFunctionPatch aviPatch{};
    CodeFunctionPatch icPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamRead),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamRead), aviPatch)) {
        return 1;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&ICDecompress),
                           reinterpret_cast<void *>(&FakeFmvICDecompress), icPatch)) {
        RestoreFunctionPatch(aviPatch);
        return 2;
    }

    const int oldBackend = g_zSnd_ActiveBackend;
    g_zSnd_ActiveBackend = 0;

    unsigned char streamStorage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    unsigned char compressedFrame[16] = {};
    unsigned char pixels[16] = {};
    BITMAPINFOHEADER srcFormat = {};
    BITMAPINFOHEADER dstFormat = {};
    PAVISTREAM const videoStream = reinterpret_cast<PAVISTREAM>(0x24681357);
    PAVISTREAM const audioStream = reinterpret_cast<PAVISTREAM>(0x13572468);
    HIC const codec = reinterpret_cast<HIC>(0x11223344);

    TestFieldAt<void *>(stream, 0x10) = pixels;
    TestFieldAt<PAVISTREAM>(stream, 0x40) = videoStream;
    TestFieldAt<BITMAPINFOHEADER *>(stream, 0x44) = &srcFormat;
    TestFieldAt<BITMAPINFOHEADER *>(stream, 0x48) = &dstFormat;
    TestFieldAt<std::uint32_t>(stream, 0x4c) = 3;
    TestFieldAt<int>(stream, 0xdc) = sizeof(compressedFrame);
    TestFieldAt<HIC>(stream, 0xe0) = codec;
    TestFieldAt<void *>(stream, 0xe4) = compressedFrame;
    InitializeCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    const bool videoDecode =
        stream->ReadAndDecodeFrame(1) == 2 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStreams[0] == videoStream && g_fakeAviStarts[0] == 1 &&
        g_fakeAviSamples[0] == 1 && g_fakeAviBuffers[0] == compressedFrame &&
        g_fakeAviBufferBytes[0] == sizeof(compressedFrame) &&
        g_fakeIcDecompressCount == 1 && g_fakeIcLastCodec == codec &&
        g_fakeIcLastFlags == 0 && g_fakeIcLastSrcFormat == &srcFormat &&
        g_fakeIcLastCompressedFrame == compressedFrame &&
        g_fakeIcLastDstFormat == &dstFormat && g_fakeIcLastPixels == pixels &&
        TestFieldAt<std::uint32_t>(stream, 0x104) == 2;

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    const bool frameWrap =
        stream->ReadAndDecodeFrame(2) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStarts[0] == 2 && g_fakeIcDecompressCount == 1 &&
        TestFieldAt<std::uint32_t>(stream, 0x104) == 0;

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    g_fakeAviReturn = 0x80004005;
    TestFieldAt<std::uint32_t>(stream, 0x104) = 0;
    const bool videoReadFailure =
        stream->ReadAndDecodeFrame(0) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeIcDecompressCount == 0 && TestFieldAt<std::uint32_t>(stream, 0x104) == 0;

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    g_fakeIcReturn = 1;
    const bool decompressFailure =
        stream->ReadAndDecodeFrame(0) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeIcDecompressCount == 1 && TestFieldAt<std::uint32_t>(stream, 0x104) == 0;
    LeaveCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));

    TestFmvDirectSoundBufferVTable bufferVTable = {};
    bufferVTable.GetCurrentPosition = &TestFmvGetCurrentPosition;
    bufferVTable.Lock = &TestFmvLockSoundBuffer;
    bufferVTable.Unlock = &TestFmvUnlockSoundBuffer;
    TestFmvDirectSoundBuffer soundBuffer{&bufferVTable};
    zSndSample sample = {};
    unsigned char audioSpan[8] = {};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&soundBuffer);
    TestFieldAt<PAVISTREAM>(stream, 0x134) = audioStream;
    TestFieldAt<int>(stream, 0x130) = 1;
    TestFieldAt<std::uint32_t>(stream, 0x168) = 2;
    TestFieldAt<zSndSample *>(stream, 0x1d0) = &sample;
    TestFieldAt<int>(stream, 0x1d4) = 0;
    TestFieldAt<int>(stream, 0x1e0) = 1;
    TestFieldAt<std::uint32_t>(stream, 0x1c8) = 8;
    TestFieldAt<std::uint32_t>(stream, 0x4c) = 0;

    ResetFmvAudioFillFakes(audioSpan, 4, nullptr, 0);
    g_fakeFmvPlayCursor = 9;
    TestFieldAt<int>(stream, 0x1dc) = 0;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 4;
    const bool firstHalfRefill =
        stream->ReadAndDecodeFrame(0xffffffffu) == 0 && g_fakeFmvGetCurrentPositionCount == 1 &&
        g_fakeFmvLockCount == 1 && g_fakeFmvLastLockOffset == 0 &&
        g_fakeFmvLastLockBytes == 8 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStreams[0] == audioStream && TestFieldAt<int>(stream, 0x1dc) == 1;

    ResetFmvAudioFillFakes(audioSpan, 4, nullptr, 0);
    g_fakeFmvPlayCursor = 4;
    TestFieldAt<int>(stream, 0x1dc) = 1;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 6;
    const bool secondHalfRefill =
        stream->ReadAndDecodeFrame(0xffffffffu) == 0 && g_fakeFmvGetCurrentPositionCount == 1 &&
        g_fakeFmvLockCount == 1 && g_fakeFmvLastLockOffset == 8 &&
        g_fakeFmvLastLockBytes == 8 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStreams[0] == audioStream && TestFieldAt<int>(stream, 0x1dc) == 0;

    DeleteCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));
    g_zSnd_ActiveBackend = oldBackend;
    RestoreFunctionPatch(icPatch);
    RestoreFunctionPatch(aviPatch);
    return videoDecode && frameWrap && videoReadFailure && decompressFailure &&
                   firstHalfRefill && secondHalfRefill
               ? 0
               : 3;
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
