#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "recoil/recoil_callconv.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dsound.h>

extern "C" std::int32_t g_zSndCdFlags;
extern "C" std::int32_t g_zSndCdLastPlayMode;
extern "C" std::int32_t g_zSndCdDeviceId;
extern "C" std::int32_t g_zSndCdAuxDeviceId;
extern "C" std::int32_t g_zSndCdTrackCountCached;
struct zSndCdTrackState
{
    std::int32_t track;
    std::int32_t minute;
    std::int32_t second;
};
extern "C" zSndCdTrackState g_zSndCdPlayFrom;
extern "C" zSndCdTrackState g_zSndCdCurrent;
extern "C" zSndCdTrackState g_zSndCdPlayTo;
extern "C" void *g_zSnd_BackendDevice;
extern "C" void *g_zSnd_BackendListenerHandle;
extern "C" DSCAPS g_zSnd_BackendAuxHandleOrConfig;
extern "C" LPDIRECTSOUND g_zSnd_CachedDirectSound;
extern "C" zSndCdTrackNode *g_zSndCd_TrackListHead;
extern "C" std::int32_t g_zSndCd_TrackCount;
extern "C" std::int32_t g_zSnd_UseArchiveBanksFlag;
extern "C" float g_zSndSpeedOfSoundMps;
extern "C" float g_zSndInvSpeedOfSoundMps;
extern "C" std::int32_t g_zSnd_ListenerStateValid;
extern "C" zSndListenerState g_zSnd_ListenerState;
extern "C" zVec3 g_zSnd_ListenerVelocity;

#define g_zSndCdPlayFromTrack (g_zSndCdPlayFrom.track)
#define g_zSndCdPlayFromMinute (g_zSndCdPlayFrom.minute)
#define g_zSndCdPlayFromSecond (g_zSndCdPlayFrom.second)
#define g_zSndCdCurrentTrack (g_zSndCdCurrent.track)
#define g_zSndCdCurrentMinute (g_zSndCdCurrent.minute)
#define g_zSndCdCurrentSecond (g_zSndCdCurrent.second)
#define g_zSndCdPlayToTrack (g_zSndCdPlayTo.track)
#define g_zSndCdPlayToMinute (g_zSndCdPlayTo.minute)
#define g_zSndCdPlayToSecond (g_zSndCdPlayTo.second)

namespace zSnd {
void RECOIL_FASTCALL SetUseArchiveBanksFlag(std::int32_t useArchiveBanks);
}

namespace zSndCd {
std::int32_t RECOIL_FASTCALL Init(zReader::Node *cdTracksNode);
int RECOIL_CDECL ResetTrackState();
std::int32_t RECOIL_CDECL IsStereoAuxEnabled();
std::int32_t RECOIL_FASTCALL ApplyPlaybackMode(std::int32_t playbackMode);
std::int32_t RECOIL_FASTCALL PlayTrack(std::int32_t trackIndex);
std::int32_t RECOIL_FASTCALL PlayTrackWithMode(std::int32_t trackIndex, std::int32_t playbackMode);
void RECOIL_FASTCALL OnMciNotify(std::uint32_t wParam, std::uint32_t lParam);
std::int32_t RECOIL_CDECL Stop();
std::int32_t RECOIL_CDECL Shutdown();
} // namespace zSndCd

namespace {
using TestBackendSimpleFn = std::int32_t(__stdcall *)(void *self);
using TestBackendGetStatusFn = std::int32_t(__stdcall *)(void *self, std::int32_t *status);
using TestBackendGetUint32Fn = std::int32_t(__stdcall *)(void *self, std::uint32_t *value);
using TestBackendSetIntFn = std::int32_t(__stdcall *)(void *self, std::int32_t value);
using TestBackendSetVecFn = std::int32_t(__stdcall *)(void *self, float x, float y, float z);
using TestBackendSetFloatFn = std::int32_t(__stdcall *)(void *self, float value);
using TestBackendPlayDirectSoundFn = std::int32_t(__stdcall *)(void *self, std::uint32_t reserved1,
                                                               std::uint32_t reserved2,
                                                               std::uint32_t flags);
using TestBackendPlayA3dFn = std::int32_t(__stdcall *)(void *self, std::uint32_t flags);
using TestBackendLockFn = std::int32_t(__stdcall *)(void *self, std::uint32_t offset,
                                                    std::uint32_t bytes, void **outPtr1,
                                                    std::int32_t *outBytes1, void **outPtr2,
                                                    std::int32_t *outBytes2, std::uint32_t flags);
using TestBackendUnlockFn = std::int32_t(__stdcall *)(void *self, void *ptr1, std::int32_t bytes1,
                                                      void *ptr2, std::int32_t bytes2);
using TestBackendCreateBufferFn = std::int32_t(__stdcall *)(void *self, void *desc,
                                                            zSndBuffer **outBuffer,
                                                            void *outerUnknown);
using TestDirectSoundGetCapsFn = HRESULT(__stdcall *)(void *self, DSCAPS *caps);
using TestA3dCreateBufferFn = std::int32_t(__stdcall *)(void *self, std::int32_t bufferKind,
                                                        zSndBuffer **outBuffer);
using TestA3dSetWaveFormatFn = std::int32_t(__stdcall *)(void *self, WAVEFORMATEX *format);
using TestA3dSetRangeFn = std::int32_t(__stdcall *)(void *self, float rangeMin, float rangeMax,
                                                    std::int32_t enabled);

struct TestDirectSoundBufferVTable {
    void *slots00_1c[8];
    TestBackendGetUint32Fn GetFrequency;
    TestBackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    TestBackendPlayDirectSoundFn Play;
    TestBackendSetIntFn SetCurrentPosition;
    void *slot38;
    TestBackendSetIntFn SetVolume;
    TestBackendSetIntFn SetPan;
    TestBackendSetIntFn SetFrequency;
    TestBackendSimpleFn Stop;
    void *slot4c;
    TestBackendSimpleFn Restore;
};

struct TestDirectSoundBuffer {
    TestDirectSoundBufferVTable *vtable;
};

struct TestCreateDirectSoundBufferVTable {
    void *slots00_20[9];
    TestBackendGetStatusFn GetStatus;
    void *Initialize;
    TestBackendLockFn Lock;
    void *Play;
    TestBackendSetIntFn SetCurrentPosition;
    void *slots38_48[5];
    TestBackendUnlockFn Unlock;
    TestBackendSimpleFn Restore;
};

struct TestCreateDirectSoundBuffer {
    TestCreateDirectSoundBufferVTable *vtable;
};

struct TestCreateDirectSoundDeviceVTable {
    void *slots00_08[3];
    TestBackendCreateBufferFn CreateSoundBuffer;
};

struct TestCreateDirectSoundDevice {
    TestCreateDirectSoundDeviceVTable *vtable;
};

struct TestCachedDirectSoundVTable {
    void *slots00_0c[4];
    TestDirectSoundGetCapsFn GetCaps;
};

struct TestCachedDirectSound {
    TestCachedDirectSoundVTable *vtable;
};

struct TestA3dSourceVTable {
    void *slots00_30[13];
    TestBackendPlayA3dFn Play;
    TestBackendSimpleFn Stop;
    void *slots3c_44[3];
    TestBackendSetIntFn SetMode;
    void *slot4c;
    TestBackendSetVecFn SetPosition;
    void *slots54_7c[11];
    TestBackendSetVecFn SetVelocity;
    void *slots84_9c[7];
    TestBackendSetFloatFn SetGain;
    void *slotsa4_ac[3];
    TestBackendSetFloatFn SetDopplerScale;
    void *slotsb4_cc[7];
    TestBackendSetIntFn SetSpatializationEnabled;
    void *slotsd4_dc[3];
    TestBackendGetStatusFn GetStatus;
};

struct TestA3dSource {
    TestA3dSourceVTable *vtable;
};

struct TestA3dListenerVTable {
    void *slots00_08[3];
    TestBackendSetVecFn SetPosition;
    void *slots10_28[7];
    std::int32_t(__stdcall *SetOrientation)(void *self, float forwardX, float forwardY,
                                            float forwardZ, float upX, float upY, float upZ);
    void *slots30_38[3];
    TestBackendSetVecFn SetVelocity;
};

struct TestA3dListener {
    TestA3dListenerVTable *vtable;
};

struct TestA3dDeviceVTable {
    void *slots00_2c[12];
    TestBackendSimpleFn Tick;
    TestBackendSimpleFn CommitDeferredSettings;
    void *slots38_44[4];
    void *DuplicateBufferA3D;
};

struct TestA3dDevice {
    TestA3dDeviceVTable *vtable;
};

int g_testGetStatusCount = 0;
int g_testRestoreCount = 0;
int g_testStopCount = 0;
int g_testSetPositionCount = 0;
int g_testSetVelocityCount = 0;
int g_testSetGainCount = 0;
int g_testSetDopplerScaleCount = 0;
int g_testSetPanCount = 0;
int g_testSetVolumeCount = 0;
int g_testSetFrequencyCount = 0;
int g_testSetCurrentPositionCount = 0;
int g_testPlayDirectSoundCount = 0;
int g_testPlayA3dCount = 0;
int g_testSetModeCount = 0;
int g_testSetSpatializationEnabledCount = 0;
int g_testA3dDeviceTickCount = 0;
int g_testCommitDeferredSettingsCount = 0;
int g_testMarkerCallbackCount = 0;
int g_testLastMarkerEvent = -1;
int g_testListenerSetPositionCount = 0;
int g_testListenerSetOrientationCount = 0;
int g_testListenerSetVelocityCount = 0;
int g_testA3dCreateBufferCount = 0;
int g_testA3dSetWaveFormatCount = 0;
int g_testA3dSetSampleDataSizeCount = 0;
int g_testA3dCommitWriteCount = 0;
int g_testA3dResetCount = 0;
int g_testA3dSetRangeCount = 0;
int g_testA3dSetDistanceScaleCount = 0;
int g_testReleaseCount = 0;
int g_testGetCapsCount = 0;
std::uint32_t g_testLastLockOffset = 0;
std::uint32_t g_testLastLockBytes = 0;
std::uint32_t g_testLastLockFlags = 0;
std::int32_t g_testStatusValue = 0;
std::uint32_t g_testFrequencyValue = 22050;
std::int32_t g_testLastA3dBufferKind = -1;
std::int32_t g_testLastA3dSampleDataSize = 0;
std::int32_t g_testLastA3dRangeEnabled = 0;

void InitFadeTestLists() {
    g_zSndFadeActiveListFlags = 0;
    g_zSndFadeDispatchListFlags = 0;
    g_zSndFadeActiveListSentinel =
        static_cast<zSndFadeListNode *>(::operator new(sizeof(zSndFadeListNode)));
    g_zSndFadeActiveListSentinel->next = g_zSndFadeActiveListSentinel;
    g_zSndFadeActiveListSentinel->prev = g_zSndFadeActiveListSentinel;
    g_zSndFadeActiveListSentinel->fadeEntry = nullptr;
    g_zSndFadeActiveListCount = 0;

    g_zSndFadeDispatchListSentinel =
        static_cast<zSndFadeListNode *>(::operator new(sizeof(zSndFadeListNode)));
    g_zSndFadeDispatchListSentinel->next = g_zSndFadeDispatchListSentinel;
    g_zSndFadeDispatchListSentinel->prev = g_zSndFadeDispatchListSentinel;
    g_zSndFadeDispatchListSentinel->fadeEntry = nullptr;
    g_zSndFadeDispatchListCount = 0;
}

void ClearFadeListNodes(zSndFadeListNode *sentinel) {
    zSndFadeListNode *node = sentinel->next;
    while (node != sentinel) {
        zSndFadeListNode *const next = node->next;
        ::operator delete(node);
        node = next;
    }
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
}

void ClearFadeTestLists() {
    if (g_zSndFadeActiveListSentinel != nullptr) {
        ClearFadeListNodes(g_zSndFadeActiveListSentinel);
        ::operator delete(g_zSndFadeActiveListSentinel);
        g_zSndFadeActiveListSentinel = nullptr;
    }

    if (g_zSndFadeDispatchListSentinel != nullptr) {
        ClearFadeListNodes(g_zSndFadeDispatchListSentinel);
        ::operator delete(g_zSndFadeDispatchListSentinel);
        g_zSndFadeDispatchListSentinel = nullptr;
    }

    g_zSndFadeActiveListCount = 0;
    g_zSndFadeDispatchListCount = 0;
}

zSndFadeListNode *AllocateFadeNode(zSndFadeEntry *fadeEntry) {
    zSndFadeListNode *const node =
        static_cast<zSndFadeListNode *>(::operator new(sizeof(zSndFadeListNode)));
    node->next = nullptr;
    node->prev = nullptr;
    node->fadeEntry = fadeEntry;
    return node;
}

void AppendFadeNode(zSndFadeListNode *sentinel, zSndFadeListNode *node, std::int32_t &count) {
    node->next = sentinel;
    node->prev = sentinel->prev;
    sentinel->prev->next = node;
    sentinel->prev = node;
    ++count;
}
zVec3 g_testLastPosition{};
zVec3 g_testLastVelocity{};
zVec3 g_testLastListenerForward{};
zVec3 g_testLastListenerUp{};
WAVEFORMATEX *g_testLastA3dWaveFormat = nullptr;
zSndBuffer *g_testA3dCreateBufferResult = nullptr;
std::int32_t g_testLastPan = 0;
std::int32_t g_testLastVolume = 0;
std::int32_t g_testLastFrequency = 0;
std::int32_t g_testLastCurrentPosition = 0;
std::int32_t g_testLastMode = 0;
std::int32_t g_testLastSpatializationEnabled = 0;
std::uint32_t g_testLastPlayFlags = 0;
float g_testLastGain = -1.0f;
float g_testLastDopplerScale = -1.0f;
float g_testLastRangeMin = -1.0f;
float g_testLastRangeMax = -1.0f;
float g_testLastDistanceScale = -1.0f;
int g_testCreateSoundBufferCount = 0;
int g_testLockCount = 0;
int g_testUnlockCount = 0;
std::uint32_t g_testCreateDescFlags = 0;
std::uint32_t g_testCreateDescBytes = 0;
WAVEFORMATEX *g_testCreateDescFormat = nullptr;
std::uint8_t g_testLockedBytes[8] = {};
std::int32_t g_testUnlockBytes1 = 0;
std::int32_t g_testUnlockBytes2 = 0;

struct TestDirectSoundBufferDesc {
    std::uint32_t size;
    std::uint32_t flags;
    std::uint32_t bufferBytes;
    std::uint32_t reserved;
    WAVEFORMATEX *format;
};

std::int32_t __stdcall TestDirectSoundGetStatus(void *, std::int32_t *status) {
    ++g_testGetStatusCount;
    *status = g_testStatusValue;
    return 0;
}

std::int32_t __stdcall TestDirectSoundGetFrequency(void *, std::uint32_t *value) {
    *value = g_testFrequencyValue;
    return 0;
}

ULONG __stdcall TestRelease(void *) {
    ++g_testReleaseCount;
    return 0;
}

HRESULT __stdcall TestDirectSoundGetCaps(void *, DSCAPS *caps) {
    ++g_testGetCapsCount;
    caps->dwFreeHwMemBytes = 0x1234;
    return static_cast<HRESULT>(0x12345678);
}

std::int32_t __stdcall TestDirectSoundSetPan(void *, std::int32_t value) {
    ++g_testSetPanCount;
    g_testLastPan = value;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetVolume(void *, std::int32_t value) {
    ++g_testSetVolumeCount;
    g_testLastVolume = value;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetFrequency(void *, std::int32_t value) {
    ++g_testSetFrequencyCount;
    g_testLastFrequency = value;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetCurrentPosition(void *, std::int32_t value) {
    ++g_testSetCurrentPositionCount;
    g_testLastCurrentPosition = value;
    return 0;
}

std::int32_t __stdcall TestDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                           std::uint32_t flags) {
    ++g_testPlayDirectSoundCount;
    g_testLastPlayFlags = flags;
    return 0;
}

std::int32_t __stdcall TestRestore(void *) {
    ++g_testRestoreCount;
    return 0;
}

std::int32_t __stdcall TestLockSoundBuffer(void *, std::uint32_t, std::uint32_t, void **,
                                           std::int32_t *, void **, std::int32_t *, std::uint32_t);

std::int32_t __stdcall TestUnlockSoundBuffer(void *, void *, std::int32_t, void *, std::int32_t);

std::int32_t __stdcall TestCreateSoundBuffer(void *, void *desc, zSndBuffer **outBuffer, void *) {
    ++g_testCreateSoundBufferCount;
    auto *typedDesc = static_cast<TestDirectSoundBufferDesc *>(desc);
    g_testCreateDescFlags = typedDesc->flags;
    g_testCreateDescBytes = typedDesc->bufferBytes;
    g_testCreateDescFormat = typedDesc->format;

    static TestCreateDirectSoundBufferVTable bufferVTable = {};
    static TestCreateDirectSoundBuffer buffer = {&bufferVTable};
    bufferVTable.GetStatus = &TestDirectSoundGetStatus;
    bufferVTable.Lock = &TestLockSoundBuffer;
    bufferVTable.SetCurrentPosition = &TestDirectSoundSetCurrentPosition;
    bufferVTable.Unlock = &TestUnlockSoundBuffer;
    bufferVTable.Restore = &TestRestore;
    *outBuffer = reinterpret_cast<zSndBuffer *>(&buffer);
    return 0;
}

std::int32_t __stdcall TestA3dCreateBufferByKind(void *, std::int32_t bufferKind,
                                                 zSndBuffer **outBuffer) {
    ++g_testA3dCreateBufferCount;
    g_testLastA3dBufferKind = bufferKind;
    *outBuffer = g_testA3dCreateBufferResult;
    return 0;
}

std::int32_t __stdcall TestA3dSetWaveFormat(void *, WAVEFORMATEX *format) {
    ++g_testA3dSetWaveFormatCount;
    g_testLastA3dWaveFormat = format;
    return 0;
}

std::int32_t __stdcall TestA3dSetSampleDataSize(void *, std::int32_t value) {
    ++g_testA3dSetSampleDataSizeCount;
    g_testLastA3dSampleDataSize = value;
    return 0;
}

std::int32_t __stdcall TestLockSoundBuffer(void *, std::uint32_t offset, std::uint32_t bytes,
                                           void **outPtr1, std::int32_t *outBytes1, void **outPtr2,
                                           std::int32_t *outBytes2, std::uint32_t flags) {
    ++g_testLockCount;
    g_testLastLockOffset = offset;
    g_testLastLockBytes = bytes;
    g_testLastLockFlags = flags;
    outPtr1[0] = &g_testLockedBytes[0];
    *outBytes1 = 2;
    outPtr2[0] = &g_testLockedBytes[2];
    *outBytes2 = 2;
    return 0;
}

std::int32_t __stdcall TestUnlockSoundBuffer(void *, void *, std::int32_t bytes1, void *,
                                             std::int32_t bytes2) {
    ++g_testUnlockCount;
    g_testUnlockBytes1 = bytes1;
    g_testUnlockBytes2 = bytes2;
    return 0;
}

std::int32_t __stdcall TestA3dCommitWrite(void *, void *, std::int32_t bytes1, void *,
                                          std::int32_t bytes2) {
    ++g_testA3dCommitWriteCount;
    g_testUnlockBytes1 = bytes1;
    g_testUnlockBytes2 = bytes2;
    return 0;
}

std::int32_t __stdcall TestA3dReset(void *) {
    ++g_testA3dResetCount;
    return 0;
}

std::int32_t __stdcall TestA3dSetRange(void *, float rangeMin, float rangeMax,
                                       std::int32_t enabled) {
    ++g_testA3dSetRangeCount;
    g_testLastRangeMin = rangeMin;
    g_testLastRangeMax = rangeMax;
    g_testLastA3dRangeEnabled = enabled;
    return 0;
}

std::int32_t __stdcall TestA3dSetDistanceScale(void *, float value) {
    ++g_testA3dSetDistanceScaleCount;
    g_testLastDistanceScale = value;
    return 0;
}

std::int32_t __stdcall TestStop(void *) {
    ++g_testStopCount;
    return 0;
}

std::int32_t __stdcall TestA3dSetPosition(void *, float x, float y, float z) {
    ++g_testSetPositionCount;
    g_testLastPosition = {x, y, z};
    return 0;
}

std::int32_t __stdcall TestA3dSetVelocity(void *, float x, float y, float z) {
    ++g_testSetVelocityCount;
    g_testLastVelocity = {x, y, z};
    return 0;
}

std::int32_t __stdcall TestA3dListenerSetPosition(void *, float x, float y, float z) {
    ++g_testListenerSetPositionCount;
    g_testLastPosition = {x, y, z};
    return 0;
}

std::int32_t __stdcall TestA3dListenerSetOrientation(void *, float forwardX, float forwardY,
                                                     float forwardZ, float upX, float upY,
                                                     float upZ) {
    ++g_testListenerSetOrientationCount;
    g_testLastListenerForward = {forwardX, forwardY, forwardZ};
    g_testLastListenerUp = {upX, upY, upZ};
    return 0;
}

std::int32_t __stdcall TestA3dListenerSetVelocity(void *, float x, float y, float z) {
    ++g_testListenerSetVelocityCount;
    g_testLastVelocity = {x, y, z};
    return 0;
}

std::int32_t __stdcall TestA3dSetGain(void *, float value) {
    ++g_testSetGainCount;
    g_testLastGain = value;
    return 0;
}

std::int32_t __stdcall TestA3dSetDopplerScale(void *, float value) {
    ++g_testSetDopplerScaleCount;
    g_testLastDopplerScale = value;
    return 0;
}

std::int32_t __stdcall TestA3dSetMode(void *, std::int32_t value) {
    ++g_testSetModeCount;
    g_testLastMode = value;
    return 0;
}

std::int32_t __stdcall TestA3dSetSpatializationEnabled(void *, std::int32_t value) {
    ++g_testSetSpatializationEnabledCount;
    g_testLastSpatializationEnabled = value;
    return 0;
}

std::int32_t __stdcall TestA3dPlay(void *, std::uint32_t flags) {
    ++g_testPlayA3dCount;
    g_testLastPlayFlags = flags;
    return 0;
}

std::int32_t __stdcall TestCommitDeferredSettings(void *) {
    ++g_testCommitDeferredSettingsCount;
    return 0;
}

std::int32_t __stdcall TestA3dDeviceTick(void *) {
    ++g_testA3dDeviceTickCount;
    return 0;
}

void RECOIL_FASTCALL TestMarkerEvent(std::int32_t eventCode) {
    ++g_testMarkerCallbackCount;
    g_testLastMarkerEvent = eventCode;
}

void ResetStopBackendCounters() {
    g_testGetStatusCount = 0;
    g_testRestoreCount = 0;
    g_testStopCount = 0;
    g_testSetPositionCount = 0;
    g_testSetVelocityCount = 0;
    g_testSetGainCount = 0;
    g_testSetDopplerScaleCount = 0;
    g_testSetPanCount = 0;
    g_testSetVolumeCount = 0;
    g_testSetFrequencyCount = 0;
    g_testSetCurrentPositionCount = 0;
    g_testPlayDirectSoundCount = 0;
    g_testPlayA3dCount = 0;
    g_testSetModeCount = 0;
    g_testSetSpatializationEnabledCount = 0;
    g_testA3dDeviceTickCount = 0;
    g_testCommitDeferredSettingsCount = 0;
    g_testMarkerCallbackCount = 0;
    g_testLastMarkerEvent = -1;
    g_testListenerSetPositionCount = 0;
    g_testListenerSetOrientationCount = 0;
    g_testListenerSetVelocityCount = 0;
    g_testA3dCreateBufferCount = 0;
    g_testA3dSetWaveFormatCount = 0;
    g_testA3dSetSampleDataSizeCount = 0;
    g_testA3dCommitWriteCount = 0;
    g_testA3dResetCount = 0;
    g_testA3dSetRangeCount = 0;
    g_testA3dSetDistanceScaleCount = 0;
    g_testStatusValue = 0;
    g_testFrequencyValue = 22050;
    g_testLastA3dBufferKind = -1;
    g_testLastA3dSampleDataSize = 0;
    g_testLastA3dRangeEnabled = 0;
    g_testLastPosition = {};
    g_testLastVelocity = {};
    g_testLastListenerForward = {};
    g_testLastListenerUp = {};
    g_testLastA3dWaveFormat = nullptr;
    g_testA3dCreateBufferResult = nullptr;
    g_testLastPan = 0;
    g_testLastVolume = 0;
    g_testLastFrequency = 0;
    g_testLastCurrentPosition = 0;
    g_testLastMode = 0;
    g_testLastSpatializationEnabled = 0;
    g_testLastPlayFlags = 0;
    g_testLastGain = -1.0f;
    g_testLastDopplerScale = -1.0f;
    g_testLastRangeMin = -1.0f;
    g_testLastRangeMax = -1.0f;
    g_testLastDistanceScale = -1.0f;
}

void EnsureZrdrFreePool() {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }
}
} // namespace

extern "C" int zsnd_set_use_archive_banks_flag_smoke(void) {
    g_zSnd_UseArchiveBanksFlag = 0;
    zSnd::SetUseArchiveBanksFlag(1);
    if (g_zSnd_UseArchiveBanksFlag != 1) {
        return 1;
    }

    zSnd::SetUseArchiveBanksFlag(0);
    return g_zSnd_UseArchiveBanksFlag == 0 ? 0 : 2;
}

extern "C" int zsnd_sample_set_registry_init_shutdown_smoke(void) {
    zSndSampleSet *stackSlots[2] = {};
    g_zSnd_SampleSetRegistry.useArchiveBanksFlag = 0;
    g_zSnd_SampleSetRegistry.begin = stackSlots;
    g_zSnd_SampleSetRegistry.end = stackSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = stackSlots + 2;

    zSnd_SetUseArchiveBanks(1);
    if (g_zSnd_SampleSetRegistry.useArchiveBanksFlag != 1 ||
        g_zSnd_SampleSetRegistry.begin != nullptr || g_zSnd_SampleSetRegistry.end != nullptr ||
        g_zSnd_SampleSetRegistry.capacityEnd != nullptr) {
        return 1;
    }

    zSnd_SetUseArchiveBanksAndRegisterAtExit(0);
    if (g_zSnd_SampleSetRegistry.useArchiveBanksFlag != 0 ||
        g_zSnd_SampleSetRegistry.begin != nullptr || g_zSnd_SampleSetRegistry.end != nullptr ||
        g_zSnd_SampleSetRegistry.capacityEnd != nullptr) {
        return 2;
    }

    zSndSampleSet **const heapSlots =
        static_cast<zSndSampleSet **>(::operator new(sizeof(zSndSampleSet *) * 2));
    g_zSnd_SampleSetRegistry.begin = heapSlots;
    g_zSnd_SampleSetRegistry.end = heapSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = heapSlots + 2;

    zSndSampleSetRegistry_Shutdown();
    return g_zSnd_SampleSetRegistry.begin == nullptr && g_zSnd_SampleSetRegistry.end == nullptr &&
                   g_zSnd_SampleSetRegistry.capacityEnd == nullptr
               ? 0
               : 3;
}

extern "C" int zsnd_sample_set_registry_lookup_destroy_smoke(void) {
    zSndSampleSet first = {};
    zSndSampleSet second = {};
    first.setName = const_cast<char *>("first");
    second.setName = const_cast<char *>("second");

    zSndSampleSet *slots[2] = {&first, &second};
    g_zSnd_SampleSetRegistry.begin = slots;
    g_zSnd_SampleSetRegistry.end = slots + 2;
    g_zSnd_SampleSetRegistry.capacityEnd = slots + 2;

    if (zSndSampleSetRegistry_GetCount() != 2 || zSndSampleSetRegistry_GetByIndex(0) != &first ||
        zSndSampleSetRegistry_GetByIndex(1) != &second ||
        zSndSampleSetRegistry_GetByIndex(2) != nullptr ||
        zSndSampleSetRegistry_GetByIndex(-1) != nullptr ||
        zSndSampleSetRegistry_FindByName("second") != &second ||
        zSndSampleSetRegistry_FindByName("missing") != nullptr) {
        g_zSnd_SampleSetRegistry.begin = nullptr;
        g_zSnd_SampleSetRegistry.end = nullptr;
        g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
        return 1;
    }

    zSndSample sample = {};
    second.resourcesLoaded = 1;
    second.sampleCount = 1;
    second.samples = &sample;
    if (zSndSampleSet_DestroyByName("second") != 1 || second.resourcesLoaded != 0 ||
        zSndSampleSet_DestroyByName("missing") != 0) {
        g_zSnd_SampleSetRegistry.begin = nullptr;
        g_zSnd_SampleSetRegistry.end = nullptr;
        g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
        return 2;
    }

    zSndSampleSet **heapSlots =
        static_cast<zSndSampleSet **>(::operator new(sizeof(zSndSampleSet *) * 2));
    zSndSampleSet *heapSet = static_cast<zSndSampleSet *>(::operator new(sizeof(zSndSampleSet)));
    *heapSet = {};
    heapSet->setName = _strdup("heap");
    heapSet->samples = static_cast<zSndSample *>(std::calloc(1, sizeof(zSndSample)));
    heapSet->sampleCount = 1;
    heapSet->resourcesLoaded = 0;
    heapSlots[0] = heapSet;
    heapSlots[1] = nullptr;
    g_zSnd_SampleSetRegistry.begin = heapSlots;
    g_zSnd_SampleSetRegistry.end = heapSlots + 2;
    g_zSnd_SampleSetRegistry.capacityEnd = heapSlots + 2;

    zSndSampleSetRegistry_DestroyAll();
    const bool destroyAllOk = g_zSnd_SampleSetRegistry.end == g_zSnd_SampleSetRegistry.begin &&
                              heapSlots[0] == nullptr && heapSlots[1] == nullptr;

    ::operator delete(heapSlots);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    return destroyAllOk ? 0 : 3;
}

extern "C" int zsnd_option_accessors_smoke(void) {
    std::int32_t audioApi = 0;
    std::int32_t cdAudio = 0;
    ZOPT_AUDIO_API = &audioApi;
    ZOPT_SOUND_CDAUDIO = &cdAudio;

    g_zSnd_IsInitialized = 0;
    g_zSnd_ActiveBackend = 0;
    if (zSnd::SetAudioApiOption(1) != 1 || audioApi != 1 || g_zSnd_ActiveBackend != 1 ||
        zSnd::GetAudioApiOption() != 1 || zSnd::GetActiveBackend() != 1) {
        return 1;
    }

    g_zSnd_IsInitialized = 1;
    if (zSnd::SetActiveBackendPreInit(0) != 0 || g_zSnd_ActiveBackend != 1) {
        return 2;
    }

    zSnd::SetCDAudioOption(1);
    return cdAudio == 1 && zSnd::GetCDAudioOption() == 1 ? 0 : 3;
}

extern "C" int zsnd_speed_of_sound_smoke(void) {
    zSnd::SetSpeedOfSoundMps(250.0f);
    return g_zSndSpeedOfSoundMps == 250.0f && g_zSndInvSpeedOfSoundMps == (1.0f / 250.0f) ? 0 : 1;
}

extern "C" int zsnd_backend_error_reporters_smoke(void) {
    const char *sourceFile = "zsnd_backend_error_reporters_smoke";
    if (zSnd::ReportDirectSoundError(0, sourceFile, 1) != 1 ||
        zSnd::ReportA3DError(0, sourceFile, 2) != 1) {
        return 1;
    }

    if (zSnd::ReportDirectSoundError(static_cast<std::int32_t>(0x8878000a), sourceFile, 3) != 0 ||
        zSnd::ReportDirectSoundError(static_cast<std::int32_t>(0x12345678), sourceFile, 4) != 0) {
        return 2;
    }

    if (zSnd::ReportA3DError(static_cast<std::int32_t>(0x80040001), sourceFile, 5) != 0 ||
        zSnd::ReportA3DError(7, sourceFile, 6) != 0) {
        return 3;
    }

    return 0;
}

extern "C" int zsnd_cached_directsound_get_caps_smoke(void) {
    TestCachedDirectSoundVTable vtable = {};
    vtable.GetCaps = &TestDirectSoundGetCaps;
    TestCachedDirectSound device{&vtable};
    g_zSnd_CachedDirectSound = reinterpret_cast<LPDIRECTSOUND>(&device);

    DSCAPS caps = {};
    caps.dwSize = 1;
    g_testGetCapsCount = 0;
    const HRESULT result = zSnd::CachedDirectSound_GetCaps(&caps);

    g_zSnd_CachedDirectSound = nullptr;
    return result == static_cast<HRESULT>(0x12345678) && g_testGetCapsCount == 1 &&
                   caps.dwSize == sizeof(DSCAPS) && caps.dwFreeHwMemBytes == 0x1234
               ? 0
               : 1;
}

extern "C" int zsnd_cd_reset_track_state_smoke(void) {
    g_zSndCdPlayFromTrack = 10;
    g_zSndCdPlayFromMinute = 11;
    g_zSndCdPlayFromSecond = 12;
    g_zSndCdCurrentTrack = 20;
    g_zSndCdCurrentMinute = 21;
    g_zSndCdCurrentSecond = 22;
    g_zSndCdPlayToTrack = 30;
    g_zSndCdPlayToMinute = 31;
    g_zSndCdPlayToSecond = 32;

    zSndCd::ResetTrackState();

    return g_zSndCdPlayFromTrack == 1 && g_zSndCdPlayFromMinute == 0 &&
                   g_zSndCdPlayFromSecond == 0 && g_zSndCdCurrentTrack == 1 &&
                   g_zSndCdCurrentMinute == 0 && g_zSndCdCurrentSecond == 0 &&
                   g_zSndCdPlayToTrack == 1 && g_zSndCdPlayToMinute == 0 &&
                   g_zSndCdPlayToSecond == 0
               ? 0
               : 1;
}

extern "C" int zsnd_cd_is_stereo_aux_enabled_smoke(void) {
    g_zSndCdFlags = 0;
    g_zSndCdAuxDeviceId = 3;
    if (zSndCd::IsStereoAuxEnabled() != 0) {
        return 1;
    }

    g_zSndCdFlags = 3;
    g_zSndCdAuxDeviceId = -1;
    if (zSndCd::IsStereoAuxEnabled() != 0) {
        return 2;
    }

    g_zSndCdAuxDeviceId = 3;
    return zSndCd::IsStereoAuxEnabled() == 1 ? 0 : 3;
}

extern "C" int zsnd_cd_not_ready_playback_smoke(void) {
    g_zSndCdFlags = 0;
    g_zSndCdLastPlayMode = 5;
    g_zSndCdDeviceId = 0x12345678;
    g_zSndCdCurrentTrack = 7;
    g_zSndCdTrackCountCached = 12;
    g_zSndCdPlayToTrack = 99;

    if (zSndCd::ApplyPlaybackMode(5) != 0 || zSndCd::PlayTrack(7) != 0 ||
        zSndCd::PlayTrackWithMode(7, 5) != 0 || zSndCd::Stop() != 0) {
        return 1;
    }

    zSndCd::OnMciNotify(1, 0x5678);
    return g_zSndCdPlayToTrack == 99 ? 0 : 2;
}

extern "C" int zsnd_cd_init_ready_guard_smoke(void) {
    g_zSndCdFlags = 2;
    g_zSndCdTrackCountCached = 77;
    return zSndCd::Init(nullptr) == 1 && g_zSndCdTrackCountCached == 77 ? 0 : 1;
}

extern "C" int zsnd_cd_get_track_count_ready_guard_smoke(void) {
    g_zSndCdTrackCountCached = 77;
    g_zSndCdFlags = 0;
    if (zSndCd::GetTrackCount() != 0) {
        return 1;
    }

    g_zSndCdFlags = 2;
    return zSndCd::GetTrackCount() == 77 ? 0 : 2;
}

extern "C" int zsnd_cd_shutdown_track_list_smoke(void) {
    zSndCdTrackNode *head = static_cast<zSndCdTrackNode *>(::operator new(sizeof(zSndCdTrackNode)));
    zSndCdTrackNode *nodeA =
        static_cast<zSndCdTrackNode *>(::operator new(sizeof(zSndCdTrackNode)));
    zSndCdTrackNode *nodeB =
        static_cast<zSndCdTrackNode *>(::operator new(sizeof(zSndCdTrackNode)));
    zSndCdTrackEntry *entryA =
        static_cast<zSndCdTrackEntry *>(::operator new(sizeof(zSndCdTrackEntry)));
    zSndCdTrackEntry *entryB =
        static_cast<zSndCdTrackEntry *>(::operator new(sizeof(zSndCdTrackEntry)));

    entryA->archiveName = _strdup("track_a");
    entryB->archiveName = _strdup("track_b");
    head->next = nodeA;
    head->prev = nodeB;
    head->entry = nullptr;
    nodeA->next = nodeB;
    nodeA->prev = head;
    nodeA->entry = entryA;
    nodeB->next = head;
    nodeB->prev = nodeA;
    nodeB->entry = entryB;

    g_zSndCd_TrackListHead = head;
    g_zSndCd_TrackCount = 2;
    g_zSndCdFlags = 2;
    g_zSndCdDeviceId = 0;

    const bool ok = zSndCd::Shutdown() == 1 && (g_zSndCdFlags & 2) == 0 &&
                    g_zSndCd_TrackCount == 0 && head->next == head && head->prev == head;

    ::operator delete(head);
    g_zSndCd_TrackListHead = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zsnd_sample_set_get_sample_at_smoke(void) {
    zSndSample samples[4] = {};
    samples[0].createGuard = 10;
    samples[1].createGuard = 20;
    samples[2].createGuard = 30;
    samples[3].createGuard = 40;

    zSndSampleSet set = {};
    set.sampleCount = 2;
    set.samples = &samples[1];

    if (set.GetSampleAt(0) != &samples[1] || set.GetSampleAt(1) != &samples[2] ||
        set.GetSampleAt(2) != nullptr) {
        return 1;
    }

    return set.GetSampleAt(-1) == &samples[0] ? 0 : 2;
}

extern "C" int zsnd_sample_set_registry_add_entry_smoke(void) {
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;

    zSndSampleSet first = {};
    zSndSampleSet second = {};

    if (first.RegistryAddEntry("first", 2) != &first || first.sampleCount != 2 ||
        first.resourcesLoaded != 0 || first.samples == nullptr ||
        first.samples[0].createGuard != 0 || std::strcmp(first.setName, "first") != 0) {
        return 1;
    }

    if (g_zSnd_SampleSetRegistry.begin == nullptr ||
        g_zSnd_SampleSetRegistry.end != g_zSnd_SampleSetRegistry.begin + 1 ||
        g_zSnd_SampleSetRegistry.capacityEnd != g_zSnd_SampleSetRegistry.begin + 1 ||
        g_zSnd_SampleSetRegistry.begin[0] != &first) {
        return 2;
    }

    if (second.RegistryAddEntry(nullptr, 1) != &second || second.setName != nullptr ||
        second.samples == nullptr ||
        g_zSnd_SampleSetRegistry.end != g_zSnd_SampleSetRegistry.begin + 2 ||
        g_zSnd_SampleSetRegistry.capacityEnd != g_zSnd_SampleSetRegistry.begin + 2 ||
        g_zSnd_SampleSetRegistry.begin[0] != &first ||
        g_zSnd_SampleSetRegistry.begin[1] != &second) {
        return 3;
    }

    std::free(first.setName);
    std::free(first.samples);
    std::free(second.samples);
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    return 0;
}

extern "C" int zsnd_find_sample_by_name_smoke(void) {
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    g_zSndStream_PendingList = nullptr;

    g_zSnd_IsInitialized = 0;
    if (zSnd::FindSampleByName("hit") != nullptr || zSnd::FindSampleByName(nullptr) != nullptr) {
        return 1;
    }

    zSndSample samples[2] = {};
    samples[0].replayFields.sampleId = "hit";
    samples[0].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1111);
    samples[1].replayFields.sampleId = "silent";
    samples[1].primaryVoice.backendBuffer = nullptr;

    zSndSampleSet set = {};
    set.sampleCount = 2;
    set.samples = samples;

    g_zSnd_SampleSetRegistry.begin =
        static_cast<zSndSampleSet **>(::operator new(sizeof(zSndSampleSet *)));
    g_zSnd_SampleSetRegistry.end = g_zSnd_SampleSetRegistry.begin + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = g_zSnd_SampleSetRegistry.begin + 1;
    g_zSnd_SampleSetRegistry.begin[0] = &set;

    g_zSnd_IsInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    if (set.FindSampleByName("hit") != &samples[0] || set.FindSampleByName("silent") != nullptr ||
        zSnd::FindSampleByName("hit") != &samples[0]) {
        return 2;
    }

    g_zSnd_ActiveBackend = 2;
    if (set.FindSampleByName("hit") != nullptr) {
        return 3;
    }

    zSndGroup pendingGroup = {};
    pendingGroup.groupName = "pending";
    g_zSnd_ActiveBackend = 0;
    EnsureZrdrFreePool();
    g_zSndStream_PendingList = zArchiveList_CreateEmpty();
    zArchiveList_PushFrontPayload(g_zSndStream_PendingList, &pendingGroup);
    const auto *const pendingAsSample = reinterpret_cast<zSndSample *>(&pendingGroup);
    if (zSndPendingList_FindByName("pending") != pendingAsSample ||
        zSndPendingList_FindByName("missing") != nullptr ||
        zSnd::FindSampleByName("pending") != pendingAsSample) {
        return 4;
    }

    zArchiveList_PopFrontPayload(g_zSndStream_PendingList);
    std::free(g_zSndStream_PendingList);
    g_zSndStream_PendingList = nullptr;
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    return 0;
}

extern "C" int zsnd_gain_scale_to_directsound_attenuation_smoke(void) {
    if (zSnd::GainScaleToDirectSoundAttenuation(1.0f) != 0 ||
        zSnd::GainScaleToDirectSoundAttenuation(2.0f) != 0) {
        return 1;
    }

    if (zSnd::GainScaleToDirectSoundAttenuation(0.0009765625f) != -10000 ||
        zSnd::GainScaleToDirectSoundAttenuation(0.0f) != -10000) {
        return 2;
    }

    if (zSnd::GainScaleToDirectSoundAttenuation(0.5f) != -602 ||
        zSnd::GainScaleToDirectSoundAttenuation(0.25f) != -1204) {
        return 3;
    }

    return 0;
}

extern "C" int zsnd_is_muted_smoke(void) {
    g_zSnd_IsInitialized = 0;
    g_zSnd_MuteDepth = 7;
    if (zSnd::IsMuted() != 0) {
        return 1;
    }

    g_zSnd_IsInitialized = 1;
    g_zSnd_MuteDepth = 0;
    if (zSnd::IsMuted() != 0) {
        return 2;
    }

    g_zSnd_MuteDepth = 3;
    if (zSnd::IsMuted() != 1) {
        return 3;
    }

    g_zSnd_MuteDepth = -1;
    const int result = zSnd::IsMuted() == 0 ? 0 : 4;
    g_zSnd_IsInitialized = 0;
    return result;
}

extern "C" int zsnd_sample_play_simple_smoke(void) {
    return zSndSample_PlaySimple(0.375f) == 0.375f ? 0 : 1;
}

extern "C" int zsnd_sample_acquire_play_handle_smoke(void) {
    ResetStopBackendCounters();

    TestDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.GetStatus = &TestDirectSoundGetStatus;
    TestDirectSoundBuffer directSoundBuffer{&directSoundVTable};
    zSndSample directSoundSample = {};
    directSoundSample.primaryVoice.backendBuffer =
        reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    g_testStatusValue = 0;
    g_zSnd_ActiveBackend = 0;
    if (directSoundSample.AcquireVoice() != &directSoundSample.primaryVoice ||
        directSoundSample.AcquirePlayHandleDispatch() != &directSoundSample.primaryVoice ||
        g_testGetStatusCount != 2) {
        return 1;
    }

    TestA3dSourceVTable a3dVTable = {};
    a3dVTable.GetStatus = &TestDirectSoundGetStatus;
    TestA3dSource a3dSource{&a3dVTable};
    zSndSample a3dSample = {};
    a3dSample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&a3dSource);
    g_zSnd_ActiveBackend = 1;
    if (a3dSample.AcquireA3dVoice() != &a3dSample.primaryVoice ||
        a3dSample.AcquirePlayHandleDispatch() != &a3dSample.primaryVoice) {
        return 2;
    }

    g_zSnd_ActiveBackend = 2;
    return directSoundSample.AcquirePlayHandleDispatch() == nullptr ? 0 : 3;
}

extern "C" int zsnd_system_named_sets_syntax_smoke(void) {
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    g_zSndCdFlags = 2;
    g_zSndSpeedOfSoundMps = 0.0f;
    g_zSndInvSpeedOfSoundMps = 0.0f;

    zReader::Node medFormatNodes[4] = {};
    medFormatNodes[0].type = zReader::ZRDR_NODE_INT;
    medFormatNodes[0].value.i32 = 4;
    medFormatNodes[1].type = zReader::ZRDR_NODE_INT;
    medFormatNodes[1].value.i32 = 32000;
    medFormatNodes[2].type = zReader::ZRDR_NODE_INT;
    medFormatNodes[2].value.i32 = 12;
    medFormatNodes[3].type = zReader::ZRDR_NODE_INT;
    medFormatNodes[3].value.i32 = 1;

    zReader::Node rangeNodes[3] = {};
    rangeNodes[0].type = zReader::ZRDR_NODE_INT;
    rangeNodes[0].value.i32 = 3;
    rangeNodes[1].type = zReader::ZRDR_NODE_FLOAT;
    rangeNodes[1].value.f32 = 12.5f;
    rangeNodes[2].type = zReader::ZRDR_NODE_FLOAT;
    rangeNodes[2].value.f32 = 34.5f;

    zReader::Node sampleNodes[17] = {};
    sampleNodes[0].type = zReader::ZRDR_NODE_INT;
    sampleNodes[0].value.i32 = 17;
    sampleNodes[1].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[1].value.str = const_cast<char *>("sample_a");
    sampleNodes[2].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[2].value.str = const_cast<char *>("resource_a.wav");
    sampleNodes[3].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[3].value.str = const_cast<char *>("3D");
    sampleNodes[4].type = zReader::ZRDR_NODE_INT;
    sampleNodes[4].value.i32 = 1;
    sampleNodes[5].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[5].value.str = const_cast<char *>("LOOPED");
    sampleNodes[6].type = zReader::ZRDR_NODE_INT;
    sampleNodes[6].value.i32 = 1;
    sampleNodes[7].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[7].value.str = const_cast<char *>("VOLUME");
    sampleNodes[8].type = zReader::ZRDR_NODE_FLOAT;
    sampleNodes[8].value.f32 = 1.5f;
    sampleNodes[9].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[9].value.str = const_cast<char *>("A3DDIST");
    sampleNodes[10].type = zReader::ZRDR_NODE_FLOAT;
    sampleNodes[10].value.f32 = 2.25f;
    sampleNodes[11].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[11].value.str = const_cast<char *>("RANGE");
    sampleNodes[12].type = zReader::ZRDR_NODE_ARRAY;
    sampleNodes[12].value.nodes = rangeNodes;
    sampleNodes[13].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[13].value.str = const_cast<char *>("HIGH");
    sampleNodes[14].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[14].value.str = const_cast<char *>("hi_variant.wav");
    sampleNodes[15].type = zReader::ZRDR_NODE_STRING;
    sampleNodes[15].value.str = const_cast<char *>("MED");
    sampleNodes[16].type = zReader::ZRDR_NODE_ARRAY;
    sampleNodes[16].value.nodes = medFormatNodes;

    zReader::Node sampleListNodes[2] = {};
    sampleListNodes[0].type = zReader::ZRDR_NODE_INT;
    sampleListNodes[0].value.i32 = 2;
    sampleListNodes[1].type = zReader::ZRDR_NODE_ARRAY;
    sampleListNodes[1].value.nodes = sampleNodes;

    zReader::Node setsNodes[3] = {};
    setsNodes[0].type = zReader::ZRDR_NODE_INT;
    setsNodes[0].value.i32 = 3;
    setsNodes[1].type = zReader::ZRDR_NODE_STRING;
    setsNodes[1].value.str = const_cast<char *>("SET_A");
    setsNodes[2].type = zReader::ZRDR_NODE_ARRAY;
    setsNodes[2].value.nodes = sampleListNodes;

    zReader::Node rootNodes[5] = {};
    rootNodes[0].type = zReader::ZRDR_NODE_INT;
    rootNodes[0].value.i32 = 5;
    rootNodes[1].type = zReader::ZRDR_NODE_STRING;
    rootNodes[1].value.str = const_cast<char *>("SETS");
    rootNodes[2].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[2].value.nodes = setsNodes;
    rootNodes[3].type = zReader::ZRDR_NODE_STRING;
    rootNodes[3].value.str = const_cast<char *>("SPEED_OF_SOUND");
    rootNodes[4].type = zReader::ZRDR_NODE_FLOAT;
    rootNodes[4].value.f32 = 345.0f;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootNodes;
    g_zSnd_ConfigRootNode = &root;

    const bool initOk = zSndSystem_InitNamedSetsSyntax(&root) == 1 &&
                        g_zSndSpeedOfSoundMps == 345.0f &&
                        g_zSnd_SampleSetRegistry.begin != nullptr &&
                        g_zSnd_SampleSetRegistry.end == g_zSnd_SampleSetRegistry.begin + 1;
    if (!initOk) {
        return 1;
    }

    zSndSampleSet *set = g_zSnd_SampleSetRegistry.begin[0];
    zSndSample *sample = set->samples;
    const bool sampleOk =
        std::strcmp(set->setName, "SET_A") == 0 && set->sampleCount == 1 &&
        std::strcmp(sample->replayFields.sampleId, "sample_a") == 0 &&
        std::strcmp(sample->replayFields.resourceName, "resource_a.wav") == 0 &&
        sample->replayFields.flags == 0x05 && sample->replayFields.gain == 1.0f &&
        sample->a3dDistanceScale == 2.25f && sample->rangeMin == 12.5f &&
        sample->rangeMax == 34.5f &&
        std::strcmp(sample->highVariant.sampleName, "hi_variant.wav") == 0 &&
        sample->highVariant.samplesPerSec == 0 && sample->medVariant.samplesPerSec == 32000 &&
        sample->medVariant.bitsPerSample == 12 && sample->medVariant.channelCount == 1 &&
        sample->lowVariant.sampleName == nullptr && sample->lowVariant.samplesPerSec == 11025 &&
        sample->lowVariant.bitsPerSample == 8 && sample->lowVariant.channelCount == 1 &&
        sample->playbackParam2 == 90000.0f && sample->playbackParam3 == 20000.0f;

    std::free(const_cast<char *>(sample->highVariant.sampleName));
    std::free(set->setName);
    std::free(set->samples);
    ::operator delete(set);
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    g_zSnd_ConfigRootNode = nullptr;
    return sampleOk ? 0 : 2;
}

extern "C" int zsnd_system_legacy_sets_syntax_smoke(void) {
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    g_zSndCdFlags = 2;
    g_zSndSpeedOfSoundMps = 0.0f;
    g_zSndInvSpeedOfSoundMps = 0.0f;

    zReader::Node legacySampleNodes[9] = {};
    legacySampleNodes[0].type = zReader::ZRDR_NODE_INT;
    legacySampleNodes[0].value.i32 = 9;
    legacySampleNodes[1].type = zReader::ZRDR_NODE_STRING;
    legacySampleNodes[1].value.str = const_cast<char *>("legacy_sample");
    legacySampleNodes[2].type = zReader::ZRDR_NODE_STRING;
    legacySampleNodes[2].value.str = const_cast<char *>("legacy.wav");
    legacySampleNodes[3].type = zReader::ZRDR_NODE_FLOAT;
    legacySampleNodes[3].value.f32 = 0.75f;
    legacySampleNodes[4].type = zReader::ZRDR_NODE_STRING;
    legacySampleNodes[4].value.str = const_cast<char *>("TRUE");
    legacySampleNodes[5].type = zReader::ZRDR_NODE_STRING;
    legacySampleNodes[5].value.str = const_cast<char *>("FALSE");
    legacySampleNodes[6].type = zReader::ZRDR_NODE_STRING;
    legacySampleNodes[6].value.str = const_cast<char *>("TRUE");
    legacySampleNodes[7].type = zReader::ZRDR_NODE_FLOAT;
    legacySampleNodes[7].value.f32 = 7.0f;
    legacySampleNodes[8].type = zReader::ZRDR_NODE_FLOAT;
    legacySampleNodes[8].value.f32 = 70.0f;

    zReader::Node sampleListNodes[2] = {};
    sampleListNodes[0].type = zReader::ZRDR_NODE_INT;
    sampleListNodes[0].value.i32 = 2;
    sampleListNodes[1].type = zReader::ZRDR_NODE_ARRAY;
    sampleListNodes[1].value.nodes = legacySampleNodes;

    zReader::Node setsNodes[3] = {};
    setsNodes[0].type = zReader::ZRDR_NODE_INT;
    setsNodes[0].value.i32 = 3;
    setsNodes[1].type = zReader::ZRDR_NODE_STRING;
    setsNodes[1].value.str = const_cast<char *>("LEGACY_SET");
    setsNodes[2].type = zReader::ZRDR_NODE_ARRAY;
    setsNodes[2].value.nodes = sampleListNodes;

    zReader::Node rootNodes[5] = {};
    rootNodes[0].type = zReader::ZRDR_NODE_INT;
    rootNodes[0].value.i32 = 5;
    rootNodes[1].type = zReader::ZRDR_NODE_STRING;
    rootNodes[1].value.str = const_cast<char *>("SETS");
    rootNodes[2].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[2].value.nodes = setsNodes;
    rootNodes[3].type = zReader::ZRDR_NODE_STRING;
    rootNodes[3].value.str = const_cast<char *>("SPEED_OF_SOUND");
    rootNodes[4].type = zReader::ZRDR_NODE_FLOAT;
    rootNodes[4].value.f32 = 300.0f;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootNodes;
    g_zSnd_ConfigRootNode = &root;

    const bool initOk = zSndSystem_InitLegacySetsSyntax(&root) == 1 &&
                        g_zSndSpeedOfSoundMps == 300.0f &&
                        g_zSnd_SampleSetRegistry.begin != nullptr &&
                        g_zSnd_SampleSetRegistry.end == g_zSnd_SampleSetRegistry.begin + 1;
    if (!initOk) {
        return 1;
    }

    zSndSampleSet *set = g_zSnd_SampleSetRegistry.begin[0];
    zSndSample *sample = set->samples;
    const bool sampleOk =
        std::strcmp(set->setName, "LEGACY_SET") == 0 && set->sampleCount == 1 &&
        std::strcmp(sample->replayFields.sampleId, "legacy_sample") == 0 &&
        std::strcmp(sample->replayFields.resourceName, "legacy.wav") == 0 &&
        sample->replayFields.flags == 0x03 && sample->replayFields.gain == 0.75f &&
        sample->rangeMin == 7.0f && sample->rangeMax == 70.0f &&
        sample->highVariant.sampleName == nullptr && sample->highVariant.samplesPerSec == 44100 &&
        sample->highVariant.bitsPerSample == 16 && sample->highVariant.channelCount == 2 &&
        sample->medVariant.samplesPerSec == 22050 && sample->lowVariant.samplesPerSec == 11025 &&
        sample->playbackParam2 == 90000.0f && sample->playbackParam3 == 20000.0f;

    std::free(set->setName);
    std::free(set->samples);
    ::operator delete(set);
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    g_zSnd_ConfigRootNode = nullptr;
    return sampleOk ? 0 : 2;
}

extern "C" int zsnd_system_init_gate_and_missing_config_smoke(void) {
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;
    g_zSnd_ActiveBackend = 2;
    g_zSnd_WindowHandle = 0;
    g_zSnd_ConfigRootNode = reinterpret_cast<zReader::Node *>(0x12345678);

    if (zSndSystem_Init(0xabcdef01u, "missing_sound_config.zrd") != 0 || g_zSnd_WindowHandle != 0 ||
        g_zSnd_ConfigRootNode != reinterpret_cast<zReader::Node *>(0x12345678)) {
        return 1;
    }

    g_zSnd_PreInitialized = 1;
    g_zSnd_WindowHandle = 0;
    g_zSnd_ConfigRootNode = reinterpret_cast<zReader::Node *>(0x12345678);
    if (zSndSystem_Init(0x1234u, "missing_sound_config.zrd") != 0 ||
        g_zSnd_WindowHandle != 0x1234u || g_zSnd_IsInitialized != 1 ||
        g_zSnd_ConfigRootNode != nullptr) {
        return 2;
    }

    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;
    g_zSnd_WindowHandle = 0;
    return 0;
}

extern "C" int zsnd_preinitialize_runtime_state_smoke(void) {
    g_zSnd_PreInitialized = 0;
    g_zSnd_IsInitialized = 7;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_WindowHandle = 0;
    g_zSndCdFlags = 0x13;
    g_zSndCdDeviceId = 0x12345678;
    g_zSndCdAuxDeviceId = -1;
    g_zSndCdAuxVolumePrimary = 9;
    g_zSndCdAuxVolumeSecondary = 8;
    g_zSndCdLastPlayMode = 99;
    g_zSndCd_TrackCount = 3;
    g_zSnd_SearchPathList = reinterpret_cast<zArchiveList *>(0x1111);
    g_zSndLastSample = reinterpret_cast<zSndSample *>(0x2222);
    g_zSndLastVoice = reinterpret_cast<zSndSample *>(0x3333);
    g_zSndLastVoiceHandle = reinterpret_cast<zSndPlayHandle *>(0x4444);
    g_zSndLastVoiceStopMarkerIndex = 7;
    g_zSnd_Flag10PlaybackEnabled = 0;
    g_zGame_Options_OptionListHead = nullptr;

    if (zSnd_PreInitializeRuntimeState(0x1234) != 1) {
        return 1;
    }

    if (g_zSnd_PreInitialized != 1 || g_zSnd_IsInitialized != 0 || g_zSnd_WindowHandle != 0x1234 ||
        (g_zSndCdFlags & 0x03) != 0 || g_zSndCdDeviceId != 0x12340000 || g_zSndCdAuxDeviceId != 0 ||
        g_zSndCdAuxVolumePrimary != 0 || g_zSndCdAuxVolumeSecondary != 0 ||
        g_zSndCdLastPlayMode != 2 || g_zSndCd_TrackCount != 0 || g_zSnd_SearchPathList != nullptr ||
        g_zSnd_SoundLodValuePtr != &g_zSnd_SoundLodDefault ||
        g_zSnd_MuteOptionValuePtr != &g_zSnd_MuteOptionDefault || g_zSnd_MuteDepth != 0 ||
        g_zSnd_GlobalVolumeScalePtr != &g_zSnd_VolumeScaleDefault ||
        g_zSnd_VolumeScaleDefault != 1.0f || g_zSndLastSample != nullptr ||
        g_zSndLastVoice != nullptr || g_zSndLastVoiceHandle != nullptr ||
        g_zSndLastVoiceStopMarkerIndex != 999 || g_zSnd_Flag10PlaybackEnabled != 1) {
        return 2;
    }

    if (zSnd_PreInitializeRuntimeState(0x5678) != 0 || g_zSnd_WindowHandle != 0x1234) {
        return 3;
    }

    zOptionEntryPartial soundLod = {};
    zOptionEntryPartial muteSound = {};
    zOptionEntryPartial soundVolume = {};
    char soundLodName[] = "SoundLOD";
    char muteSoundName[] = "MuteSound";
    char soundVolumeName[] = "SoundVolume";
    soundLod.name = soundLodName;
    muteSound.name = muteSoundName;
    soundVolume.name = soundVolumeName;
    soundLod.payloadOrBuffer = 2;
    muteSound.payloadOrBuffer = 5;
    soundVolume.payloadOrBuffer = 0x3f000000;
    soundLod.next = &muteSound;
    muteSound.next = &soundVolume;
    g_zGame_Options_OptionListHead = &soundLod;
    g_zSnd_PreInitialized = 0;
    g_zSnd_ActiveBackend = 2;
    g_zSnd_BackendDevice = reinterpret_cast<void *>(0x4444);
    g_zSnd_BackendListenerHandle = reinterpret_cast<void *>(0x5555);

    if (zSnd_PreInitializeRuntimeState(0x9abc) != 1) {
        return 4;
    }

    return g_zSnd_SoundLodValuePtr == &soundLod && g_zSnd_MuteOptionValuePtr == &muteSound &&
                   g_zSnd_MuteDepth == 5 && g_zSnd_GlobalVolumeScalePtr == &soundVolume &&
                   g_zSnd_BackendDevice == reinterpret_cast<void *>(0x4444) &&
                   g_zSnd_BackendListenerHandle == reinterpret_cast<void *>(0x5555)
               ? 0
               : 5;
}

extern "C" int zsnd_group_load_config_block_smoke(void) {
    zReader::Node nestedNodes[4] = {};
    nestedNodes[0].type = zReader::ZRDR_NODE_INT;
    nestedNodes[0].value.i32 = 4;
    nestedNodes[1].type = zReader::ZRDR_NODE_STRING;
    nestedNodes[1].value.str = const_cast<char *>("nested_stream");
    nestedNodes[2].type = zReader::ZRDR_NODE_STRING;
    nestedNodes[2].value.str = const_cast<char *>("WEIGHT");
    nestedNodes[3].type = zReader::ZRDR_NODE_INT;
    nestedNodes[3].value.i32 = 7;

    zReader::Node rootNodes[10] = {};
    rootNodes[0].type = zReader::ZRDR_NODE_INT;
    rootNodes[0].value.i32 = 10;
    rootNodes[1].type = zReader::ZRDR_NODE_STRING;
    rootNodes[1].value.str = const_cast<char *>("stream_a");
    rootNodes[2].type = zReader::ZRDR_NODE_STRING;
    rootNodes[2].value.str = const_cast<char *>("DELAY_PLAY");
    rootNodes[3].type = zReader::ZRDR_NODE_INT;
    rootNodes[3].value.i32 = 3;
    rootNodes[4].type = zReader::ZRDR_NODE_STRING;
    rootNodes[4].value.str = const_cast<char *>("PLAY_COUNT");
    rootNodes[5].type = zReader::ZRDR_NODE_FLOAT;
    rootNodes[5].value.f32 = 2.6f;
    rootNodes[6].type = zReader::ZRDR_NODE_STRING;
    rootNodes[6].value.str = const_cast<char *>("WEIGHT");
    rootNodes[7].type = zReader::ZRDR_NODE_FLOAT;
    rootNodes[7].value.f32 = 1.25f;
    rootNodes[8].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[8].value.nodes = nestedNodes;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootNodes;

    zSndGroupRuntimeFields fields = {};
    fields.groupName = "group_a";
    zSndGroupConfigBlock block = {};

    const bool ok =
        zSndGroup_LoadConfigBlock(&root, &fields, &block) == 1 && block.currentPlayCount == 3 &&
        block.maxPlayCount == 3 && block.delayPlaySec == 3.0f && block.weight == 1.25f &&
        std::strcmp(block.streamName, "stream_a") == 0 && block.child != nullptr &&
        block.child->currentPlayCount == 0xffff && block.child->maxPlayCount == 0xffff &&
        block.child->weight == 7.0f && std::strcmp(block.child->streamName, "nested_stream") == 0;

    std::free(block.child);
    return ok ? 0 : 1;
}

extern "C" int zsnd_group_load_and_queue_smoke(void) {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }
    g_zSndStream_PendingList = nullptr;
    g_zSndStream_ActiveList = nullptr;
    g_zSndStream_FreeList = nullptr;
    g_zSndStream_RootNode = reinterpret_cast<zClass_NodePartial *>(0x1);

    zReader::Node blockANodes[2] = {};
    blockANodes[0].type = zReader::ZRDR_NODE_INT;
    blockANodes[0].value.i32 = 2;
    blockANodes[1].type = zReader::ZRDR_NODE_STRING;
    blockANodes[1].value.str = const_cast<char *>("stream_a");

    zReader::Node blockBNodes[2] = {};
    blockBNodes[0].type = zReader::ZRDR_NODE_INT;
    blockBNodes[0].value.i32 = 2;
    blockBNodes[1].type = zReader::ZRDR_NODE_STRING;
    blockBNodes[1].value.str = const_cast<char *>("stream_b");

    zReader::Node groupNodes[13] = {};
    groupNodes[0].type = zReader::ZRDR_NODE_INT;
    groupNodes[0].value.i32 = 13;
    groupNodes[1].type = zReader::ZRDR_NODE_STRING;
    groupNodes[1].value.str = const_cast<char *>("group_a");
    groupNodes[2].type = zReader::ZRDR_NODE_STRING;
    groupNodes[2].value.str = const_cast<char *>("DELAY_REPEAT");
    groupNodes[3].type = zReader::ZRDR_NODE_INT;
    groupNodes[3].value.i32 = 2;
    groupNodes[4].type = zReader::ZRDR_NODE_STRING;
    groupNodes[4].value.str = const_cast<char *>("DELAY_TERMINATION");
    groupNodes[5].type = zReader::ZRDR_NODE_FLOAT;
    groupNodes[5].value.f32 = 4.5f;
    groupNodes[6].type = zReader::ZRDR_NODE_STRING;
    groupNodes[6].value.str = const_cast<char *>("DYNAMIC_WEIGHTS");
    groupNodes[7].type = zReader::ZRDR_NODE_FLOAT;
    groupNodes[7].value.f32 = 1.5f;
    groupNodes[8].type = zReader::ZRDR_NODE_STRING;
    groupNodes[8].value.str = const_cast<char *>("PLAY_SOLO");
    groupNodes[9].type = zReader::ZRDR_NODE_STRING;
    groupNodes[9].value.str = const_cast<char *>("REPEAT");
    groupNodes[10].type = zReader::ZRDR_NODE_INT;
    groupNodes[10].value.i32 = 6;
    groupNodes[11].type = zReader::ZRDR_NODE_ARRAY;
    groupNodes[11].value.nodes = blockANodes;
    groupNodes[12].type = zReader::ZRDR_NODE_ARRAY;
    groupNodes[12].value.nodes = blockBNodes;

    zReader::Node groupNode = {};
    groupNode.type = zReader::ZRDR_NODE_ARRAY;
    groupNode.value.nodes = groupNodes;

    zSndGroup *group = zSndGroup_LoadFromConfigNode(&groupNode);
    const bool loadOk = group != nullptr && group->createGuard == 1 &&
                        std::strcmp(group->groupName, "group_a") == 0 &&
                        group->delayRepeatSec == 2.0f && group->delayTerminationSec == 4.5f &&
                        group->dynamicWeightsEnabled == 1 && group->dynamicWeightScale == 1.0f &&
                        group->playSolo == 1 && group->repeatCount == 6 &&
                        group->configBlockCount == 2 && group->configBlocks[0].weight == 50.0f &&
                        group->configBlocks[1].weight == 50.0f;

    std::free(group->configBlocks);
    std::free(group);

    zReader::Node groupListNodes[3] = {};
    groupListNodes[0].type = zReader::ZRDR_NODE_INT;
    groupListNodes[0].value.i32 = 3;
    groupListNodes[1].type = zReader::ZRDR_NODE_ARRAY;
    groupListNodes[1].value.nodes = groupNodes;
    groupListNodes[2].type = zReader::ZRDR_NODE_ARRAY;
    groupListNodes[2].value.nodes = groupNodes;

    zReader::Node groupList = {};
    groupList.type = zReader::ZRDR_NODE_ARRAY;
    groupList.value.nodes = groupListNodes;

    const bool queueOk = zSndGroup_QueuePendingLoadsFromConfigNode(&groupList) == 1 &&
                         g_zSndStream_PendingList != nullptr &&
                         zArchiveList_GetCount(g_zSndStream_PendingList) == 2;

    zSndPlayHandle request{};
    zSndPlayHandle otherRequest{};
    g_zSndStream_ActiveList = zArchiveList_CreateEmpty();
    zArchiveList_PushFrontPayload(g_zSndStream_ActiveList, &request);
    const bool stopFound =
        zSndStreamRequest_StopIfActive(&request) == 1 && request.backendState1 == 4;
    const bool stopMissing =
        zSndStreamRequest_StopIfActive(&otherRequest) == 0 && otherRequest.backendState1 == 0;

    return loadOk && queueOk && stopFound && stopMissing ? 0 : 1;
}

extern "C" int zsnd_stream_mgr_shutdown_lists_smoke(void) {
    g_zSndStream_RootNode = nullptr;
    g_zSndStream_MatchedRequest = reinterpret_cast<zSndStreamRequest *>(0x1234);
    g_zSndStream_MatchedRequestCount = 3;

    EnsureZrdrFreePool();
    g_zSndStream_ActiveList = zArchiveList_CreateEmpty();
    g_zSndStream_FreeList = zArchiveList_CreateEmpty();
    g_zSndStream_PendingList = zArchiveList_CreateEmpty();

    void *const activeRequest = std::malloc(sizeof(zSndStreamRequest));
    void *const freeRequest = std::malloc(sizeof(zSndStreamRequest));
    if (g_zSndStream_ActiveList == nullptr || g_zSndStream_FreeList == nullptr ||
        g_zSndStream_PendingList == nullptr || activeRequest == nullptr || freeRequest == nullptr) {
        std::free(activeRequest);
        std::free(freeRequest);
        return 1;
    }

    zArchiveList_PushFrontPayload(g_zSndStream_ActiveList, activeRequest);
    zArchiveList_PushFrontPayload(g_zSndStream_FreeList, freeRequest);

    zSndGroup *const pending = static_cast<zSndGroup *>(std::calloc(1, sizeof(zSndGroup)));
    if (pending == nullptr) {
        return 2;
    }
    pending->createGuard = 1;
    pending->configBlockCount = 2;
    pending->configBlocks =
        static_cast<zSndGroupConfigBlock *>(std::calloc(2, sizeof(zSndGroupConfigBlock)));
    if (pending->configBlocks == nullptr) {
        std::free(pending);
        return 3;
    }

    auto *const childA =
        static_cast<zSndGroupConfigBlock *>(std::calloc(1, sizeof(zSndGroupConfigBlock)));
    auto *const childB =
        static_cast<zSndGroupConfigBlock *>(std::calloc(1, sizeof(zSndGroupConfigBlock)));
    auto *const childC =
        static_cast<zSndGroupConfigBlock *>(std::calloc(1, sizeof(zSndGroupConfigBlock)));
    if (childA == nullptr || childB == nullptr || childC == nullptr) {
        std::free(childA);
        std::free(childB);
        std::free(childC);
        std::free(pending->configBlocks);
        std::free(pending);
        return 4;
    }

    pending->configBlocks[0].child = childA;
    childA->child = childB;
    pending->configBlocks[1].child = childC;
    zArchiveList_PushFrontPayload(g_zSndStream_PendingList, pending);

    return zSndStreamMgr::Shutdown() == 1 && g_zSndStream_ActiveList == nullptr &&
                   g_zSndStream_FreeList == nullptr && g_zSndStream_PendingList == nullptr &&
                   g_zSndStream_MatchedRequest == nullptr && g_zSndStream_MatchedRequestCount == 0
               ? 0
               : 5;
}

extern "C" int zsnd_backend_shutdown_release_smoke(void) {
    TestCreateDirectSoundDeviceVTable deviceVTable = {};
    TestCreateDirectSoundDevice device{&deviceVTable};
    TestDirectSoundBufferVTable listenerVTable = {};
    TestDirectSoundBuffer listener{&listenerVTable};
    TestA3dDeviceVTable auxVTable = {};
    TestA3dDevice auxObject{&auxVTable};

    deviceVTable.slots00_08[2] = reinterpret_cast<void *>(&TestRelease);
    listenerVTable.slots00_1c[2] = reinterpret_cast<void *>(&TestRelease);
    auxVTable.slots00_2c[2] = reinterpret_cast<void *>(&TestRelease);

    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 1;
    if (zSndBackend::Shutdown() != 0) {
        return 1;
    }

    g_testReleaseCount = 0;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_BackendDevice = &device;
    g_zSnd_BackendListenerHandle = &listener;
    std::memset(&g_zSnd_BackendAuxHandleOrConfig, 0, sizeof(g_zSnd_BackendAuxHandleOrConfig));

    if (zSndBackend::Shutdown() != 1 || g_testReleaseCount != 2 ||
        g_zSnd_BackendDevice != nullptr || g_zSnd_BackendListenerHandle != nullptr ||
        g_zSnd_IsInitialized != 0) {
        return 2;
    }

    g_testReleaseCount = 0;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 1;
    g_zSnd_BackendDevice = &device;
    g_zSnd_BackendListenerHandle = &listener;
    *reinterpret_cast<void **>(&g_zSnd_BackendAuxHandleOrConfig) = &auxObject;

    const bool a3dOk = zSndBackend::Shutdown() == 1 && g_testReleaseCount == 3 &&
                       g_zSnd_BackendDevice == nullptr && g_zSnd_BackendListenerHandle == nullptr &&
                       *reinterpret_cast<void **>(&g_zSnd_BackendAuxHandleOrConfig) == nullptr &&
                       g_zSnd_IsInitialized == 0;

    std::memset(&g_zSnd_BackendAuxHandleOrConfig, 0, sizeof(g_zSnd_BackendAuxHandleOrConfig));
    return a3dOk ? 0 : 3;
}

extern "C" int zsnd_play_handle_stop_if_active_smoke(void) {
    zSndPlayHandle inactive{};
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 1;
    if (inactive.StopIfActive() != -1) {
        return 1;
    }

    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 2;
    zSndPlayHandle unsupported{};
    const std::int32_t unsupportedResult = unsupported.StopIfActive();
    if (unsupportedResult !=
        static_cast<std::int32_t>(reinterpret_cast<std::intptr_t>(&unsupported))) {
        return 2;
    }

    zSndPlayHandle request{};
    request.handleKind = ZSND_PLAYHANDLE_STREAM_REQUEST;
    request.backendState1 = 0;
    EnsureZrdrFreePool();
    g_zSndStream_ActiveList = zArchiveList_CreateEmpty();
    zArchiveList_PushFrontPayload(g_zSndStream_ActiveList, &request);
    if (request.StopIfActive() != 1 || request.backendState1 != 4) {
        return 3;
    }

    ResetStopBackendCounters();
    TestDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.GetStatus = &TestDirectSoundGetStatus;
    directSoundVTable.Stop = &TestStop;
    directSoundVTable.Restore = &TestRestore;
    TestDirectSoundBuffer directSoundBuffer{&directSoundVTable};
    zSndPlayHandle directSoundHandle{};
    directSoundHandle.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    directSoundHandle.ownerSample = reinterpret_cast<zSndSample *>(0x12345678);
    g_zSndLastVoice = reinterpret_cast<zSndSample *>(0x12345678);
    g_zSndLastVoiceMarkerIndex = 7;
    g_zSnd_ActiveBackend = 0;
    g_testStatusValue = 2;
    if (directSoundHandle.StopIfActive() != 0 || g_testGetStatusCount != 1 ||
        g_testRestoreCount != 1 || g_testStopCount != 1 || g_zSndLastVoice != nullptr ||
        g_zSndLastVoiceMarkerIndex != 0) {
        return 4;
    }

    ResetStopBackendCounters();
    TestA3dSourceVTable a3dVTable = {};
    a3dVTable.Stop = &TestStop;
    TestA3dSource a3dSource{&a3dVTable};
    zSndPlayHandle a3dHandle{};
    a3dHandle.backendBuffer = reinterpret_cast<zSndBuffer *>(&a3dSource);
    g_zSnd_ActiveBackend = 1;
    if (a3dHandle.StopIfActive() != 0 || g_testStopCount != 1 || g_testGetStatusCount != 0 ||
        g_testRestoreCount != 0) {
        return 5;
    }

    zSndPlayHandle missingBackend{};
    g_zSnd_ActiveBackend = 0;
    return missingBackend.StopIfActive() == -1 ? 0 : 6;
}

extern "C" int zsnd_play_handle_update3d_a3d_smoke(void) {
    ResetStopBackendCounters();

    TestA3dSourceVTable a3dVTable = {};
    a3dVTable.SetPosition = &TestA3dSetPosition;
    a3dVTable.SetVelocity = &TestA3dSetVelocity;
    a3dVTable.SetGain = &TestA3dSetGain;
    a3dVTable.SetDopplerScale = &TestA3dSetDopplerScale;
    TestA3dSource a3dSource{&a3dVTable};

    zSndSample sample = {};
    zSndPlayHandle handle = {};
    handle.backendBuffer = reinterpret_cast<zSndBuffer *>(&a3dSource);
    handle.ownerSample = &sample;
    float rawGain = 7.0f;
    std::memcpy(&handle.gainScaled, &rawGain, sizeof(handle.gainScaled));

    zVec3 worldPos{1.0f, 2.0f, 3.0f};
    zVec3 velocity{4.0f, 5.0f, 6.0f};

    handle.handleKind = ZSND_PLAYHANDLE_STREAM_REQUEST;
    if (handle.Update3D_A3D(&worldPos, &velocity, 1) != -1) {
        return 1;
    }

    handle.handleKind = ZSND_PLAYHANDLE_BACKEND;
    sample.replayFields.flags = 0;
    if (handle.Update3D_A3D(&worldPos, &velocity, 1) != 1 || g_testSetPositionCount != 0 ||
        g_testSetVelocityCount != 0 || g_testSetGainCount != 0 || g_testSetDopplerScaleCount != 0) {
        return 2;
    }

    sample.replayFields.flags = 4;
    const int oldInitialized = g_zSnd_IsInitialized;
    g_zSnd_PreInitialized = 1;
    g_zSnd_IsInitialized = 1;
    g_zSnd_MuteDepth = 0;
    if (handle.Update3D_A3D(&worldPos, &velocity, 1) != 1 || g_testSetPositionCount != 1 ||
        g_testSetVelocityCount != 1 || g_testSetGainCount != 1 || g_testSetDopplerScaleCount != 1 ||
        g_testLastPosition.x != 1.0f || g_testLastPosition.y != 2.0f ||
        g_testLastPosition.z != 3.0f || g_testLastVelocity.x != 4.0f ||
        g_testLastVelocity.y != 5.0f || g_testLastVelocity.z != 6.0f || g_testLastGain != 7.0f ||
        g_testLastDopplerScale != 1.0f) {
        g_zSnd_IsInitialized = oldInitialized;
        return 3;
    }

    g_zSnd_MuteDepth = 1;
    if (handle.Update3D_A3D(nullptr, nullptr, 0) != 1 || g_testSetPositionCount != 1 ||
        g_testSetVelocityCount != 1 || g_testSetGainCount != 2 || g_testSetDopplerScaleCount != 2 ||
        g_testLastGain != 0.0f || g_testLastDopplerScale != 0.0f) {
        g_zSnd_IsInitialized = oldInitialized;
        return 4;
    }

    handle.backendBuffer = nullptr;
    const int result = handle.Update3D_A3D(nullptr, nullptr, 0) == -1 ? 0 : 5;
    g_zSnd_IsInitialized = oldInitialized;
    return result;
}

extern "C" int zsnd_update_listener_state_smoke(void) {
    ResetStopBackendCounters();

    zSndListenerState listenerState{};
    listenerState.right = {1.0f, 0.0f, 0.0f};
    listenerState.up = {0.0f, 1.0f, 0.0f};
    listenerState.forward = {0.0f, 0.0f, 1.0f};
    listenerState.position = {7.0f, 8.0f, 9.0f};
    zVec3 listenerVelocity{2.0f, 3.0f, 4.0f};

    g_zSnd_ActiveBackend = 0;
    g_zSnd_ListenerStateValid = 0;
    if (zSnd_UpdateListenerState(&listenerState, &listenerVelocity) != 1 ||
        g_zSnd_ListenerStateValid != 1 || g_zSnd_ListenerState.position.x != 7.0f ||
        g_zSnd_ListenerState.up.y != 1.0f || g_zSnd_ListenerVelocity.z != 4.0f) {
        return 1;
    }

    g_zSnd_ActiveBackend = 1;
    g_zSnd_BackendListenerHandle = nullptr;
    if (zSnd_UpdateListenerState(&listenerState, &listenerVelocity) != -1) {
        return 2;
    }

    TestA3dListenerVTable listenerVTable = {};
    listenerVTable.SetPosition = &TestA3dListenerSetPosition;
    listenerVTable.SetOrientation = &TestA3dListenerSetOrientation;
    listenerVTable.SetVelocity = &TestA3dListenerSetVelocity;
    TestA3dListener listener{&listenerVTable};
    g_zSnd_BackendListenerHandle = &listener;
    g_zSnd_ListenerStateValid = 0;
    if (zSnd_UpdateListenerState(&listenerState, &listenerVelocity) != 1 ||
        g_zSnd_ListenerStateValid != 1 || g_testListenerSetPositionCount != 1 ||
        g_testListenerSetOrientationCount != 1 || g_testListenerSetVelocityCount != 1 ||
        g_testLastPosition.z != 9.0f || g_testLastListenerForward.z != -1.0f ||
        g_testLastListenerUp.y != 1.0f || g_testLastVelocity.x != 2.0f) {
        return 3;
    }

    g_zSnd_BackendListenerHandle = nullptr;
    return 0;
}

extern "C" int zsnd_play_handle_update3d_directsound_smoke(void) {
    ResetStopBackendCounters();

    TestDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.GetFrequency = &TestDirectSoundGetFrequency;
    directSoundVTable.SetVolume = &TestDirectSoundSetVolume;
    directSoundVTable.SetPan = &TestDirectSoundSetPan;
    directSoundVTable.SetFrequency = &TestDirectSoundSetFrequency;
    TestDirectSoundBuffer directSoundBuffer{&directSoundVTable};

    zSndSample sample = {};
    sample.replayFields.flags = 4;
    sample.rangeMin = 10.0f;
    sample.rangeMax = 100.0f;

    zSndPlayHandle handle = {};
    handle.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    handle.ownerSample = &sample;
    handle.gainScaled = -200;

    g_zSnd_ActiveBackend = 0;
    g_zSnd_PreInitialized = 1;
    g_zSnd_MuteDepth = 0;
    g_zSnd_ListenerStateValid = 0;
    if (handle.Update3D(nullptr, nullptr, 0) != 1 || g_testSetPanCount != 1 ||
        g_testSetVolumeCount != 1 || g_testLastPan != 0 || g_testLastVolume != -200) {
        return 1;
    }

    ResetStopBackendCounters();
    handle.handleKind = ZSND_PLAYHANDLE_STREAM_REQUEST;
    if (handle.Update3D(nullptr, nullptr, 0) != -1) {
        return 2;
    }

    handle.handleKind = ZSND_PLAYHANDLE_BACKEND;
    sample.replayFields.flags = 0;
    zVec3 worldPos{20.0f, 0.0f, 0.0f};
    if (handle.Update3D(&worldPos, nullptr, 0) != 1 || g_testSetPanCount != 0 ||
        g_testSetVolumeCount != 0) {
        return 3;
    }

    sample.replayFields.flags = 4;
    g_zSnd_ListenerStateValid = 1;
    g_zSnd_ListenerState.right = {1.0f, 0.0f, 0.0f};
    g_zSnd_ListenerState.position = {};
    g_zSnd_ListenerVelocity = {};
    g_zSndInvSpeedOfSoundMps = 0.0f;
    zVec3 velocity{1.0f, 0.0f, 0.0f};
    ResetStopBackendCounters();
    if (handle.Update3D(&worldPos, &velocity, 1) != 1 || handle.hasWorldPos != 1 ||
        handle.worldPos.x != 20.0f || handle.velocityOrDir.x != 1.0f || g_testSetPanCount != 1 ||
        g_testSetVolumeCount != 1 || g_testSetFrequencyCount != 1 || g_testLastPan != 1560 ||
        g_testLastVolume != -829 ||
        g_testLastFrequency != static_cast<std::int32_t>(g_testFrequencyValue)) {
        return 4;
    }

    worldPos = {200.0f, 0.0f, 0.0f};
    ResetStopBackendCounters();
    if (handle.Update3D(&worldPos, nullptr, 0) != 0 || g_testSetPanCount != 0 ||
        g_testSetVolumeCount != 1 || g_testLastVolume != -10000) {
        return 5;
    }

    g_zSnd_ActiveBackend = 2;
    return handle.Update3DDispatch(nullptr, nullptr, 0) == 0 ? 0 : 6;
}

extern "C" int zsnd_sample_play_a3d_simple_direct_smoke(void) {
    ResetStopBackendCounters();

    TestDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.GetStatus = &TestDirectSoundGetStatus;
    directSoundVTable.SetVolume = &TestDirectSoundSetVolume;
    directSoundVTable.SetCurrentPosition = &TestDirectSoundSetCurrentPosition;
    directSoundVTable.Play = &TestDirectSoundPlay;
    TestDirectSoundBuffer directSoundBuffer{&directSoundVTable};

    float globalVolume = 0.5f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_testStatusValue = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 2.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    zSndPlayHandle *directResult = sample.PlayA3DSimple(0.5f);
    if (directResult != &sample.primaryVoice || sample.primaryVoice.ownerSample != &sample ||
        sample.primaryVoice.gainScaled != -602 || g_testSetVolumeCount != 1 ||
        g_testLastVolume != -602 || g_testSetCurrentPositionCount != 1 ||
        g_testLastCurrentPosition != 0 || g_testPlayDirectSoundCount != 1 ||
        g_testLastPlayFlags != 0) {
        return 1;
    }

    ResetStopBackendCounters();
    TestA3dSourceVTable a3dVTable = {};
    a3dVTable.GetStatus = &TestDirectSoundGetStatus;
    a3dVTable.SetSpatializationEnabled = &TestA3dSetSpatializationEnabled;
    a3dVTable.SetGain = &TestA3dSetGain;
    a3dVTable.SetMode = &TestA3dSetMode;
    a3dVTable.Play = &TestA3dPlay;
    TestA3dSource a3dSource{&a3dVTable};

    TestA3dDeviceVTable a3dDeviceVTable = {};
    a3dDeviceVTable.CommitDeferredSettings = &TestCommitDeferredSettings;
    TestA3dDevice a3dDevice{&a3dDeviceVTable};
    g_zSnd_BackendDevice = &a3dDevice;
    globalVolume = 1.0f;
    g_zSnd_ActiveBackend = 1;
    g_testStatusValue = 0;

    zSndSample a3dSample = {};
    a3dSample.replayFields.flags = 9;
    a3dSample.replayFields.gain = 0.25f;
    a3dSample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&a3dSource);

    zSndPlayHandle *a3dResult = a3dSample.PlayA3DSimple(2.0f);
    if (a3dResult != &a3dSample.primaryVoice || g_testSetSpatializationEnabledCount != 1 ||
        g_testLastSpatializationEnabled != 1 || g_testSetGainCount != 1 || g_testLastGain != 0.5f ||
        g_testSetModeCount != 1 || g_testLastMode != 0 || g_testPlayA3dCount != 1 ||
        g_testLastPlayFlags != 1 || g_testCommitDeferredSettingsCount != 1) {
        return 2;
    }

    g_zSnd_BackendDevice = nullptr;
    g_zSnd_GlobalVolumeScalePtr = nullptr;
    return 0;
}

extern "C" int zsnd_sample_destroy_owned_data_smoke(void) {
    zSndSample guarded{};
    guarded.createGuard = 1;
    if (guarded.DestroyOwnedData() != 0) {
        return 1;
    }

    zSndSample sample{};
    sample.replayFields.flags = 0x0f;
    sample.markerTimes = static_cast<float *>(std::malloc(sizeof(float)));
    sample.markerValues = static_cast<float *>(std::malloc(sizeof(float)));
    sample.markerAux = static_cast<std::int32_t *>(std::malloc(sizeof(std::int32_t)));
    sample.highVariant.sampleName = _strdup("high");
    sample.medVariant.sampleName = _strdup("med");
    sample.lowVariant.sampleName = _strdup("low");
    sample.duplicateVoiceCount = 2;
    sample.duplicateVoices =
        static_cast<zSndPlayHandle **>(std::malloc(sizeof(zSndPlayHandle *) * 2));
    sample.duplicateVoices[0] = nullptr;
    sample.duplicateVoices[1] = nullptr;

    const std::int32_t result = sample.DestroyOwnedData();

    return result == 1 && sample.markerTimes == nullptr && sample.markerValues == nullptr &&
                   sample.markerAux == nullptr && sample.highVariant.sampleName == nullptr &&
                   sample.medVariant.sampleName == nullptr &&
                   sample.lowVariant.sampleName == nullptr && sample.duplicateVoiceCount == 0 &&
                   sample.duplicateVoices == nullptr &&
                   sample.primaryVoice.backendBuffer == nullptr &&
                   (sample.replayFields.flags & 0x08) == 0
               ? 0
               : 2;
}

extern "C" int zsnd_fade_entry_backend_and_dispatch_smoke(void) {
    ResetStopBackendCounters();
    InitFadeTestLists();

    TestDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.SetVolume = &TestDirectSoundSetVolume;
    TestDirectSoundBuffer directSoundBuffer{&directSoundVTable};

    zSndPlayHandle directSoundHandle = {};
    directSoundHandle.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    zSndFadeEntry directSoundFade{0.0f, -2500.0f, &directSoundHandle, 0};

    g_zSnd_ActiveBackend = 0;
    if (directSoundFade.TickAndMaybeDispatch(1.0f) != 1 || directSoundFade.currentValue != 0.0f ||
        g_testSetVolumeCount != 1 || g_testLastVolume != 0 || g_zSndFadeDispatchListCount != 1 ||
        g_zSndFadeDispatchListSentinel->prev->fadeEntry != &directSoundFade) {
        ClearFadeTestLists();
        return 1;
    }

    ClearFadeTestLists();
    ResetStopBackendCounters();
    InitFadeTestLists();

    TestA3dSourceVTable a3dVTable = {};
    a3dVTable.SetGain = &TestA3dSetGain;
    TestA3dSource a3dSource{&a3dVTable};

    zSndPlayHandle a3dHandle = {};
    a3dHandle.backendBuffer = reinterpret_cast<zSndBuffer *>(&a3dSource);
    zSndFadeEntry a3dFade{1.0f, 0.0f, &a3dHandle, 0};

    g_zSnd_ActiveBackend = 1;
    const std::int32_t result = a3dFade.TickAndMaybeDispatch(1.0f) == 1 &&
                                        a3dFade.currentValue == 1.0f && g_testSetGainCount == 1 &&
                                        g_testLastGain == 1.0f && g_zSndFadeDispatchListCount == 1
                                    ? 0
                                    : 2;

    ClearFadeTestLists();
    return result;
}

extern "C" int zsnd_fade_active_list_tick_compacts_smoke(void) {
    InitFadeTestLists();
    g_zSnd_ActiveBackend = 2;

    zSndPlayHandle handle = {};
    zSndFadeEntry completed{2500.0f, 0.0f, &handle, 0};
    zSndFadeEntry survivor{5000.0f, 0.0f, &handle, 0};

    zSndFadeListNode *const first = AllocateFadeNode(&completed);
    zSndFadeListNode *const second = AllocateFadeNode(&survivor);
    AppendFadeNode(g_zSndFadeActiveListSentinel, first, g_zSndFadeActiveListCount);
    AppendFadeNode(g_zSndFadeActiveListSentinel, second, g_zSndFadeActiveListCount);

    zSndFadeActiveList_TickAll(1.0f);

    const bool ok = g_zSndFadeActiveListCount == 1 && g_zSndFadeActiveListSentinel->next == first &&
                    g_zSndFadeActiveListSentinel->prev == first && first->fadeEntry == &survivor &&
                    survivor.currentValue == 2500.0f && g_zSndFadeDispatchListCount == 1 &&
                    g_zSndFadeDispatchListSentinel->next->fadeEntry == &completed;

    ClearFadeTestLists();
    return ok ? 0 : 1;
}

extern "C" int zsnd_fade_list_cursor_helpers_smoke(void) {
    InitFadeTestLists();

    zSndFadeEntry entryA = {};
    zSndFadeEntry entryB = {};
    zSndFadeListNode *const first = AllocateFadeNode(&entryA);
    zSndFadeListNode *const second = AllocateFadeNode(&entryB);
    AppendFadeNode(g_zSndFadeActiveListSentinel, first, g_zSndFadeActiveListCount);
    AppendFadeNode(g_zSndFadeActiveListSentinel, second, g_zSndFadeActiveListCount);

    zSndFadeListCursor cursor{first};
    zSndFadeListNode *popped = nullptr;
    if (cursor.PopFrontCursor(&popped, 0) != &popped || popped != first || cursor.node != second) {
        ClearFadeTestLists();
        return 1;
    }

    zSndFadeList list{0, g_zSndFadeActiveListSentinel, 2};
    zSndFadeListNode *outCursor = nullptr;
    list.DeleteNodeAndAdvanceCursor(first, &outCursor);

    const bool ok = list.count == 1 && outCursor == second &&
                    g_zSndFadeActiveListSentinel->next == second &&
                    second->prev == g_zSndFadeActiveListSentinel;

    g_zSndFadeActiveListSentinel->next = second;
    g_zSndFadeActiveListSentinel->prev = second;
    g_zSndFadeActiveListCount = 1;
    ClearFadeTestLists();
    return ok ? 0 : 2;
}

extern "C" int zsnd_fade_lists_stop_all_shutdown_smoke(void) {
    InitFadeTestLists();

    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 2;

    zSndPlayHandle handleA = {};
    zSndPlayHandle handleB = {};
    zSndFadeEntry *const fadeA =
        static_cast<zSndFadeEntry *>(::operator new(sizeof(zSndFadeEntry)));
    zSndFadeEntry *const fadeB =
        static_cast<zSndFadeEntry *>(::operator new(sizeof(zSndFadeEntry)));
    *fadeA = zSndFadeEntry{0.0f, -1.0f, &handleA, 1};
    *fadeB = zSndFadeEntry{1.0f, 0.0f, &handleB, 1};

    AppendFadeNode(g_zSndFadeActiveListSentinel, AllocateFadeNode(fadeA),
                   g_zSndFadeActiveListCount);
    AppendFadeNode(g_zSndFadeActiveListSentinel, AllocateFadeNode(fadeB),
                   g_zSndFadeActiveListCount);

    zSndFadeLists::StopAllAndShutdown();

    const bool ok = g_zSndFadeActiveListCount == 0 && g_zSndFadeDispatchListCount == 0 &&
                    g_zSndFadeActiveListSentinel->next == g_zSndFadeActiveListSentinel &&
                    g_zSndFadeDispatchListSentinel->next == g_zSndFadeDispatchListSentinel;

    ClearFadeTestLists();
    return ok ? 0 : 1;
}

extern "C" int zsnd_tick_backend_markers_smoke(void) {
    ResetStopBackendCounters();
    g_zSndFadeActiveListCount = 0;

    TestA3dDeviceVTable a3dDeviceVTable = {};
    a3dDeviceVTable.Tick = &TestA3dDeviceTick;
    a3dDeviceVTable.CommitDeferredSettings = &TestCommitDeferredSettings;
    TestA3dDevice a3dDevice{&a3dDeviceVTable};
    g_zSnd_BackendDevice = &a3dDevice;
    g_zSnd_ActiveBackend = 1;
    g_zSndLastVoice = nullptr;

    zSnd_Tick(0);
    if (g_testCommitDeferredSettingsCount != 1 || g_testA3dDeviceTickCount != 1) {
        return 1;
    }

    zSnd_Tick(1);
    if (g_testCommitDeferredSettingsCount != 1 || g_testA3dDeviceTickCount != 1) {
        return 2;
    }

    ResetStopBackendCounters();
    g_zSnd_BackendDevice = nullptr;
    g_zSnd_ActiveBackend = 2;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;

    float markerValues[2] = {5.0f, 10.0f};
    zSndSample sample = {};
    sample.playbackEventHandler = &TestMarkerEvent;
    sample.markerCount = 2;
    sample.markerValues = markerValues;

    zSndPlayHandle handle = {};
    handle.ownerSample = &sample;
    g_zSndLastVoice = &sample;
    g_zSndLastVoiceHandle = &handle;
    g_zSndLastVoiceMarkerIndex = 0;
    g_zSndLastVoiceStopMarkerIndex = 1;
    g_Time_UnscaledAccumulatedTimeSec = 4.0f;

    zSnd_Tick(1);
    if (g_testMarkerCallbackCount != 0 || g_zSndLastVoiceMarkerIndex != 0) {
        return 3;
    }

    g_Time_UnscaledAccumulatedTimeSec = 5.0f;
    zSnd_Tick(1);
    if (g_testMarkerCallbackCount != 1 || g_testLastMarkerEvent != 0 ||
        g_zSndLastVoiceMarkerIndex != 1 || g_zSndLastVoice != &sample) {
        return 4;
    }

    g_Time_UnscaledAccumulatedTimeSec = 10.0f;
    zSnd_Tick(1);
    if (g_testMarkerCallbackCount != 2 || g_testLastMarkerEvent != 1 ||
        g_zSndLastVoice != nullptr || g_zSndLastVoiceMarkerIndex != 0 ||
        g_zSndLastVoiceStopMarkerIndex != 999) {
        return 5;
    }

    g_zSndLastVoice = &sample;
    g_zSndLastVoiceMarkerIndex = 7;
    g_zSndLastVoiceStopMarkerIndex = 3;
    sample.markerValues = nullptr;
    zSnd_TickWrapper(1);
    return g_zSndLastVoice == nullptr && g_zSndLastVoiceMarkerIndex == 0 &&
                   g_zSndLastVoiceStopMarkerIndex == 999
               ? 0
               : 6;
}

extern "C" int zsnd_sample_set_init_by_name_empty_smoke(void) {
    zSndSampleSet empty = {};
    empty.setName = const_cast<char *>("empty");
    empty.sampleCount = 0;
    empty.resourcesLoaded = 0;

    zSndSampleSet *slots[1] = {&empty};
    g_zSnd_SampleSetRegistry.begin = slots;
    g_zSnd_SampleSetRegistry.end = slots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = slots + 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_zSnd_SearchPathList = nullptr;

    if (zSndSampleSet_InitByName("empty") != 1 || empty.resourcesLoaded != 1) {
        g_zSnd_SampleSetRegistry.begin = nullptr;
        g_zSnd_SampleSetRegistry.end = nullptr;
        g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
        return 1;
    }

    if (zSndSampleSet_InitByName("empty") != 0 || zSndSampleSet_InitByName("missing") != 0) {
        g_zSnd_SampleSetRegistry.begin = nullptr;
        g_zSnd_SampleSetRegistry.end = nullptr;
        g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
        return 2;
    }

    g_zSnd_SampleSetRegistry.begin = nullptr;
    g_zSnd_SampleSetRegistry.end = nullptr;
    g_zSnd_SampleSetRegistry.capacityEnd = nullptr;
    return 0;
}

extern "C" int zsnd_wave_data_load_parse_reset_smoke(void) {
    const char *fileName = "recoil_wave_data_smoke.wav";
    FILE *file = std::fopen(fileName, "wb");
    if (file == nullptr) {
        return 1;
    }

    const std::uint8_t wavBytes[] = {
        'R', 'I', 'F', 'F', 40,  0,   0,   0,   'W',  'A',  'V', 'E', 'f',  'm',  't', ' ',
        16,  0,   0,   0,   1,   0,   1,   0,   0x40, 0x1f, 0,   0,   0x80, 0x3e, 0,   0,
        2,   0,   16,  0,   'd', 'a', 't', 'a', 4,    0,    0,   0,   1,    2,    3,   4,
    };
    std::fwrite(wavBytes, sizeof(wavBytes), 1, file);
    std::fclose(file);

    zSndWaveData wave{};
    wave.ConstructorFromPath(fileName, 1);

    const bool loaded = wave.parsedOk == 1 &&
                        wave.fileSize == static_cast<std::int32_t>(sizeof(wavBytes)) &&
                        wave.fileData != nullptr && wave.fmt != nullptr &&
                        wave.fmt->wFormatTag == WAVE_FORMAT_PCM && wave.fmt->nChannels == 1 &&
                        wave.pcmData != nullptr && wave.pcmByteCount == 4;

    wave.Reset();
    const bool reset = wave.parsedOk == 0 && wave.fileData == nullptr && wave.fileSize == 0 &&
                       wave.fmt == nullptr && wave.pcmData == nullptr && wave.pcmByteCount == 0;

    wave.Destructor();
    std::remove(fileName);
    return loaded && reset ? 0 : 2;
}

extern "C" int zsnd_wave_data_archive_load_smoke(void) {
    const char *fileName = "recoil_archive_wave_smoke.bin";
    const std::uint8_t wavBytes[] = {
        'R', 'I', 'F', 'F', 40,  0,   0,   0,   'W',  'A',  'V', 'E', 'f',  'm',  't', ' ',
        16,  0,   0,   0,   1,   0,   1,   0,   0x40, 0x1f, 0,   0,   0x80, 0x3e, 0,   0,
        2,   0,   16,  0,   'd', 'a', 't', 'a', 4,    0,    0,   0,   1,    2,    3,   4,
    };

    FILE *file = std::fopen(fileName, "wb");
    if (file == nullptr) {
        return 1;
    }
    std::fwrite(wavBytes, sizeof(wavBytes), 1, file);
    std::fclose(file);

    HANDLE handle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        std::remove(fileName);
        return 2;
    }

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = sizeof(wavBytes);
    std::strcpy(record.name, "arch.wav");

    zIndexArchive archive = {};
    archive.hFile = handle;
    archive.recordCount = 1;
    archive.records = &record;

    std::uint8_t readBuffer[sizeof(wavBytes)] = {};
    std::uint32_t requiredSize = 0;
    if (archive.ReadFileByName("arch.wav", readBuffer, &requiredSize) != 0x10002 ||
        requiredSize != sizeof(wavBytes)) {
        CloseHandle(handle);
        std::remove(fileName);
        return 3;
    }

    requiredSize = sizeof(readBuffer);
    if (archive.ReadFileByName("arch.wav", readBuffer, &requiredSize) != 0 ||
        requiredSize != sizeof(wavBytes) ||
        std::memcmp(readBuffer, wavBytes, sizeof(wavBytes)) != 0) {
        CloseHandle(handle);
        std::remove(fileName);
        return 4;
    }

    zSndWaveData wave = {};
    wave.ConstructorFromPath("arch.wav", 0);
    const bool archiveLoadOk =
        wave.LoadAndParseFromIndexArchiveIfNeeded(&archive) == 1 && wave.parsedOk == 1 &&
        wave.fileSize == static_cast<std::int32_t>(sizeof(wavBytes)) && wave.fmt != nullptr &&
        wave.pcmData != nullptr && wave.pcmByteCount == 4;
    wave.Destructor();

    CloseHandle(handle);
    std::remove(fileName);
    return archiveLoadOk ? 0 : 5;
}

extern "C" int zsnd_sample_backend_buffer_lock_unlock_smoke(void) {
    TestDirectSoundBufferVTable directSoundVTable = {};
    directSoundVTable.slot2c = reinterpret_cast<void *>(&TestLockSoundBuffer);
    directSoundVTable.slot4c = reinterpret_cast<void *>(&TestUnlockSoundBuffer);
    TestDirectSoundBuffer directSoundBuffer{&directSoundVTable};

    zSndSample sample = {};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    void *ptr1 = nullptr;
    void *ptr2 = nullptr;
    std::int32_t bytes1 = 0;
    std::int32_t bytes2 = 0;
    g_testLockCount = 0;
    g_testUnlockCount = 0;
    g_testA3dCommitWriteCount = 0;
    g_testLastLockOffset = 0;
    g_testLastLockBytes = 0;
    g_testLastLockFlags = 0xffffffffu;
    g_zSnd_ActiveBackend = 0;

    const bool directSoundOk =
        sample.LockBackendBuffers(4, 8, &ptr1, &bytes1, &ptr2, &bytes2) == 1 &&
        sample.UnlockBackendBuffers(ptr1, bytes1, ptr2, bytes2) == 1 && g_testLockCount == 1 &&
        g_testUnlockCount == 1 && g_testA3dCommitWriteCount == 0 && g_testLastLockOffset == 4 &&
        g_testLastLockBytes == 8 && g_testLastLockFlags == 0 && bytes1 == 2 && bytes2 == 2;

    TestA3dSourceVTable a3dVTable = {};
    a3dVTable.slots00_30[11] = reinterpret_cast<void *>(&TestLockSoundBuffer);
    a3dVTable.slots00_30[12] = reinterpret_cast<void *>(&TestA3dCommitWrite);
    TestA3dSource a3dSource{&a3dVTable};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&a3dSource);

    ptr1 = nullptr;
    ptr2 = nullptr;
    bytes1 = 0;
    bytes2 = 0;
    g_testLockCount = 0;
    g_testUnlockCount = 0;
    g_testA3dCommitWriteCount = 0;
    g_testLastLockOffset = 0;
    g_testLastLockBytes = 0;
    g_zSnd_ActiveBackend = 1;

    const bool a3dOk = sample.LockBackendBuffers(12, 16, &ptr1, &bytes1, &ptr2, &bytes2) == 1 &&
                       sample.UnlockBackendBuffers(ptr1, bytes1, ptr2, bytes2) == 1 &&
                       g_testLockCount == 1 && g_testUnlockCount == 0 &&
                       g_testA3dCommitWriteCount == 1 && g_testLastLockOffset == 12 &&
                       g_testLastLockBytes == 16 && bytes1 == 2 && bytes2 == 2;

    sample.createGuard = 1;
    const bool guardOk = sample.LockBackendBuffers(0, 0, &ptr1, &bytes1, &ptr2, &bytes2) == 0 &&
                         sample.UnlockBackendBuffers(ptr1, bytes1, ptr2, bytes2) == 0;

    sample.createGuard = 0;
    g_zSnd_ActiveBackend = 2;
    const bool fallbackOk = sample.LockBackendBuffers(0, 0, &ptr1, &bytes1, &ptr2, &bytes2) == 1 &&
                            sample.UnlockBackendBuffers(ptr1, bytes1, ptr2, bytes2) == 1;

    return directSoundOk && a3dOk && guardOk && fallbackOk ? 0 : 1;
}

extern "C" int zsnd_sample_init_from_wave_data_directsound_smoke(void) {
    g_testCreateSoundBufferCount = 0;
    g_testLockCount = 0;
    g_testUnlockCount = 0;
    g_testGetStatusCount = 0;
    g_testSetCurrentPositionCount = 0;
    g_testCreateDescFlags = 0;
    g_testCreateDescBytes = 0;
    g_testCreateDescFormat = nullptr;
    g_testStatusValue = 0;
    std::memset(g_testLockedBytes, 0, sizeof(g_testLockedBytes));

    TestCreateDirectSoundDeviceVTable deviceVTable = {};
    deviceVTable.CreateSoundBuffer = &TestCreateSoundBuffer;
    TestCreateDirectSoundDevice device{&deviceVTable};
    g_zSnd_BackendDevice = &device;
    g_zSnd_ActiveBackend = 0;

    WAVEFORMATEX fmt = {};
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = 8000;
    fmt.nAvgBytesPerSec = 16000;
    fmt.nBlockAlign = 2;
    fmt.wBitsPerSample = 16;

    std::uint8_t pcmData[4] = {1, 2, 3, 4};
    zSndCuePoint cue = {};
    cue.position = 1;

    zSndWaveData wave = {};
    wave.nameOrPath = const_cast<char *>("inline");
    wave.pcmByteCount = sizeof(pcmData);
    wave.fmt = &fmt;
    wave.cuePointCount = 1;
    wave.cuePoints = &cue;
    wave.pcmData = pcmData;

    zSndSample sample = {};
    const std::int32_t result = sample.InitFromWaveData_DirectSound(&wave);

    const bool ok =
        result == 1 && g_testCreateSoundBufferCount == 1 && g_testCreateDescFlags == 0x80 &&
        g_testCreateDescBytes == 4 && g_testCreateDescFormat == &fmt && g_testGetStatusCount == 1 &&
        g_testLockCount == 1 && g_testUnlockCount == 1 && g_testUnlockBytes1 == 4 &&
        g_testUnlockBytes2 == 2 && g_testSetCurrentPositionCount == 1 &&
        g_testLastCurrentPosition == 0 &&
        std::memcmp(g_testLockedBytes, pcmData, sizeof(pcmData)) == 0 &&
        sample.primaryVoice.handleKind == ZSND_PLAYHANDLE_BACKEND &&
        sample.primaryVoice.backendBuffer != nullptr && sample.markerCount == 2 &&
        sample.markerAux != nullptr && sample.markerAux[0] == 2 && sample.markerTimes != nullptr &&
        sample.markerTimes[0] == 0.000125f && sample.markerTimes[1] == 0.00025f &&
        sample.playbackEventHandler == nullptr && (sample.replayFields.flags & 0x80) == 0;

    std::free(sample.markerTimes);
    std::free(sample.markerValues);
    std::free(sample.markerAux);
    g_zSnd_BackendDevice = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zsnd_sample_init_from_wave_data_a3d_smoke(void) {
    ResetStopBackendCounters();
    g_testCreateSoundBufferCount = 0;
    g_testLockCount = 0;
    g_testUnlockCount = 0;
    g_testUnlockBytes1 = 0;
    g_testUnlockBytes2 = 0;
    std::memset(g_testLockedBytes, 0, sizeof(g_testLockedBytes));

    TestA3dSourceVTable a3dVTable = {};
    a3dVTable.slots00_30[5] = reinterpret_cast<void *>(&TestA3dSetSampleDataSize);
    a3dVTable.slots00_30[7] = reinterpret_cast<void *>(&TestA3dSetWaveFormat);
    a3dVTable.slots00_30[11] = reinterpret_cast<void *>(&TestLockSoundBuffer);
    a3dVTable.slots00_30[12] = reinterpret_cast<void *>(&TestA3dCommitWrite);
    a3dVTable.slots3c_44[0] = reinterpret_cast<void *>(&TestA3dReset);
    a3dVTable.slots84_9c[5] = reinterpret_cast<void *>(&TestA3dSetRange);
    a3dVTable.slotsb4_cc[1] = reinterpret_cast<void *>(&TestA3dSetDistanceScale);
    a3dVTable.SetSpatializationEnabled = &TestA3dSetSpatializationEnabled;
    TestA3dSource a3dSource{&a3dVTable};
    g_testA3dCreateBufferResult = reinterpret_cast<zSndBuffer *>(&a3dSource);

    TestA3dDeviceVTable a3dDeviceVTable = {};
    a3dDeviceVTable.slots38_44[3] = reinterpret_cast<void *>(&TestA3dCreateBufferByKind);
    TestA3dDevice a3dDevice{&a3dDeviceVTable};
    g_zSnd_BackendDevice = &a3dDevice;
    g_zSnd_ActiveBackend = 1;

    WAVEFORMATEX fmt = {};
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = 8000;
    fmt.nAvgBytesPerSec = 16000;
    fmt.nBlockAlign = 2;
    fmt.wBitsPerSample = 16;

    std::uint8_t pcmData[4] = {1, 2, 3, 4};
    zSndCuePoint cue = {};
    cue.position = 1;

    zSndWaveData wave = {};
    wave.nameOrPath = const_cast<char *>("inline");
    wave.pcmByteCount = sizeof(pcmData);
    wave.fmt = &fmt;
    wave.cuePointCount = 1;
    wave.cuePoints = &cue;
    wave.pcmData = pcmData;

    zSndSample sample = {};
    sample.replayFields.flags = 0x04;
    sample.rangeMin = 3.0f;
    sample.rangeMax = 9.0f;
    sample.a3dDistanceScale = 2.0f;
    const std::int32_t result = sample.InitFromWaveData_A3D(&wave);

    const bool ok =
        result == 1 && g_testA3dCreateBufferCount == 1 && g_testLastA3dBufferKind == 0 &&
        g_testA3dSetWaveFormatCount == 1 && g_testLastA3dWaveFormat == &fmt &&
        g_testA3dSetSampleDataSizeCount == 1 && g_testLastA3dSampleDataSize == 4 &&
        g_testLockCount == 1 && g_testA3dCommitWriteCount == 1 && g_testUnlockBytes1 == 4 &&
        g_testUnlockBytes2 == 2 && g_testA3dResetCount == 1 && g_testA3dSetRangeCount == 1 &&
        g_testLastRangeMin == 3.0f && g_testLastRangeMax == 9.0f &&
        g_testLastA3dRangeEnabled == 1 && g_testA3dSetDistanceScaleCount == 1 &&
        g_testLastDistanceScale == 2.0f && g_testSetSpatializationEnabledCount == 0 &&
        std::memcmp(g_testLockedBytes, pcmData, sizeof(pcmData)) == 0 &&
        sample.primaryVoice.backendBuffer != nullptr && sample.sampleRate == 8000.0f &&
        sample.markerCount == 2 && sample.markerAux != nullptr && sample.markerAux[0] == 2 &&
        sample.markerTimes != nullptr && sample.markerTimes[0] == 0.000125f &&
        sample.markerTimes[1] == 0.00025f && sample.playbackEventHandler == nullptr &&
        (sample.replayFields.flags & 0x80) == 0;

    std::free(sample.markerTimes);
    std::free(sample.markerValues);
    std::free(sample.markerAux);

    ResetStopBackendCounters();
    std::memset(g_testLockedBytes, 0, sizeof(g_testLockedBytes));
    sample = {};
    sample.replayFields.flags = 0;
    g_testA3dCreateBufferResult = reinterpret_cast<zSndBuffer *>(&a3dSource);
    const std::int32_t spatialResult = sample.InitFromWaveData_A3D(&wave);
    const bool spatialOk =
        spatialResult == 1 && g_testA3dSetRangeCount == 0 && g_testA3dSetDistanceScaleCount == 0 &&
        g_testSetSpatializationEnabledCount == 1 && g_testLastSpatializationEnabled == 1;

    std::free(sample.markerTimes);
    std::free(sample.markerValues);
    std::free(sample.markerAux);
    g_zSnd_BackendDevice = nullptr;
    g_testA3dCreateBufferResult = nullptr;
    return ok && spatialOk ? 0 : 1;
}
