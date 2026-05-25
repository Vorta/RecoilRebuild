#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

// clang-format off
#include <windows.h>
#include <mmsystem.h>
#if !defined(_DWORD_PTR_DEFINED) && !defined(DWORD_PTR)
typedef DWORD DWORD_PTR;
#define _DWORD_PTR_DEFINED
#endif
#include <dsound.h>
// clang-format on

#include "recoil/recoil_callconv.h"
#include "zClass.h"

struct zSndBuffer;
struct zSndSample;
struct zArchiveList;
struct zIndexArchive;

enum zSndPlayHandleKind {
    ZSND_PLAYHANDLE_BACKEND = 0,
    ZSND_PLAYHANDLE_STREAM_REQUEST = 1,
};

struct zSndSampleReplayFields {
    const char *resourceName;
    const char *sampleId;
    int flags;
    float gain;
};

struct zSndPlayHandle {
    int isActive;
    zSndPlayHandleKind handleKind;
    zSndBuffer *backendBuffer;
    zVec3 worldPos;
    zVec3 velocityOrDir;
    int gainScaled;
    int hasWorldPos;
    struct zSndSample *ownerSample;
    int backendState0;
    int backendState1;
    int backendState2;

    RECOIL_NOINLINE int RECOIL_THISCALL StopIfActive();
    RECOIL_NOINLINE int RECOIL_THISCALL SetFreqScaled(float scale);
    RECOIL_NOINLINE void RECOIL_THISCALL SetEnableScale(float scale);
    RECOIL_NOINLINE int RECOIL_FASTCALL Update3DDispatch(zVec3 *worldPos, zVec3 *velocity,
                                                                  int velocityScaleMode);
    RECOIL_NOINLINE int RECOIL_FASTCALL Update3D(zVec3 *worldPos, zVec3 *velocity,
                                                          int velocityScaleMode);
    RECOIL_NOINLINE int RECOIL_FASTCALL Update3D_A3D(zVec3 *worldPos, zVec3 *velocity,
                                                              int velocityScaleMode);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    PlayWithDelta_A3D(zSndSampleReplayFields *replayFields, zSndPlayHandle *playHandle,
                      int restartBeforePlay, float gainDelta);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    PlayWithDelta_DirectSound(zSndSampleReplayFields *replayFields,
                              zSndPlayHandle *playHandle, int restartBeforePlay,
                              int gainDelta);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    PlayWithDelta_BackendDispatch(zSndSample *sourceSample, zSndPlayHandle *playHandle,
                                  int restartBeforePlay, float gainDelta);
};

struct zSndPlayHandleSnapshotPayload {
    zSndPlayHandle *playHandle;
    zSndSample *sourceSample;
    unsigned int volumeScaleRaw;
    unsigned int flags;
    zVec3 worldPos;
    zVec3 velocityOrDir;

    RECOIL_NOINLINE void RECOIL_FASTCALL CaptureFromPlayHandle(zSndPlayHandle *playHandle);
};

struct zSndPlayHandleSnapshotItem {
    zSndPlayHandleSnapshotItem *next;
    zSndPlayHandleSnapshotItem *prev;
    zSndPlayHandleSnapshotPayload payload;
};

struct zSndPlayHandleSnapshot {
    unsigned char backendTag;
    unsigned char unknown_01[3];
    zSndPlayHandleSnapshotItem *listHead;
    int itemCount;

    zSndPlayHandleSnapshot(unsigned char backendTag);

    static RECOIL_NOINLINE zSndPlayHandleSnapshot *RECOIL_CDECL CreateFromActiveSamples();
    void AppendPayload(const zSndPlayHandleSnapshotPayload &payload);
    RECOIL_NOINLINE zSndPlayHandleSnapshotItem *RECOIL_THISCALL
    NewNode(zSndPlayHandleSnapshotItem *listHead, zSndPlayHandleSnapshotItem *prev);

    RECOIL_NOINLINE int RECOIL_THISCALL StopAllIfPlaying();
    RECOIL_NOINLINE int RECOIL_THISCALL RestoreAllWithGlobalVolumeDelta();
    RECOIL_NOINLINE int RECOIL_THISCALL Destroy();
};

struct zSndListenerState {
    zVec3 right;
    zVec3 up;
    zVec3 forward;
    zVec3 position;
};

struct zSndQualityVariant {
    const char *sampleName;
    int samplesPerSec;
    int bitsPerSample;
    int channelCount;
};

struct zSndCuePoint {
    unsigned int identifier;
    unsigned int position;
    unsigned int fccChunk;
    unsigned int chunkStart;
    unsigned int blockStart;
    unsigned int sampleOffset;
};

struct zSndWaveData {
    int parsedOk;
    char *nameOrPath;
    int fileSize;
    void *fileData;
    int pcmByteCount;
    WAVEFORMATEX *fmt;
    int cuePointCount;
    zSndCuePoint *cuePoints;
    void *pcmData;

    RECOIL_NOINLINE zSndWaveData *RECOIL_THISCALL ConstructorFromPath(const char *path,
                                                                      int loadNow);
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE int RECOIL_THISCALL ParseLoadedWaveFile();
    RECOIL_NOINLINE int RECOIL_THISCALL LoadAndParseIfNeeded();
    RECOIL_NOINLINE int RECOIL_THISCALL
    LoadAndParseFromIndexArchiveIfNeeded(zIndexArchive *archive);
    RECOIL_NOINLINE int RECOIL_THISCALL Reset();
};

struct zSndSample {
    int createGuard;
    zSndSampleReplayFields replayFields;
    float rangeMin;
    float rangeMax;
    float playbackParam2;
    float playbackParam3;
    float sampleRate;
    float a3dDistanceScale;
    void(RECOIL_FASTCALL *playbackEventHandler)(int eventCode);
    float markerBaseTime;
    int markerCount;
    float *markerTimes;
    float *markerValues;
    int *markerAux;
    zSndPlayHandle primaryVoice;
    int duplicateVoiceCount;
    zSndPlayHandle **duplicateVoices;
    zSndQualityVariant highVariant;
    zSndQualityVariant medVariant;
    zSndQualityVariant lowVariant;

    RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL AcquirePlayHandleDispatch();
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL AcquireA3dVoice();
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL AcquireVoice();
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL PlayOnActiveBackend(zVec3 *worldPos,
                                                                        float gainScale,
                                                                        zVec3 *velocity,
                                                                        int backendArg);
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL PlayOnA3D(zVec3 *worldPos, float gainScale,
                                                              zVec3 *velocity,
                                                              int backendArg);
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL PlayOnDirectSound(int attenuation,
                                                                      zVec3 *worldPos,
                                                                      zVec3 *velocity,
                                                                      int backendArg);
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL PlayA3D(zVec3 *worldPos, float gainScale,
                                                            zVec3 *velocity);
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL PlayDirectSound(int variantIndex,
                                                                    float gainScale,
                                                                    int stopMarkerIndex);
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL PlayA3DSimple(float gainScale);
    RECOIL_NOINLINE int RECOIL_THISCALL StopActiveVoicesIfPlaying();
    RECOIL_NOINLINE int RECOIL_FASTCALL InitFromWaveData(zSndWaveData *waveData);
    RECOIL_NOINLINE int RECOIL_FASTCALL
    InitFromWaveData_DirectSound(zSndWaveData *waveData);
    RECOIL_NOINLINE int RECOIL_FASTCALL InitFromWaveData_A3D(zSndWaveData *waveData);
    RECOIL_NOINLINE int RECOIL_FASTCALL
    LockBackendBuffers(unsigned int offset, unsigned int bytes, void **buffer1,
                       int *buffer1Bytes, void **buffer2, int *buffer2Bytes);
    RECOIL_NOINLINE int RECOIL_FASTCALL UnlockBackendBuffers(void *buffer1,
                                                                      int buffer1Bytes,
                                                                      void *buffer2,
                                                                      int buffer2Bytes);
    RECOIL_NOINLINE unsigned int RECOIL_THISCALL GetPlayCursorBytes();
    RECOIL_NOINLINE void RECOIL_FASTCALL
    SetPlaybackEventHandler(void(RECOIL_FASTCALL *callback)(int eventCode));
    RECOIL_NOINLINE int RECOIL_THISCALL DestroyOwnedData();
    RECOIL_NOINLINE void RECOIL_THISCALL Destroy();
};

struct zSndSampleSet {
    char *setName;
    int sampleCount;
    zSndSample *samples;
    int resourcesLoaded;

    RECOIL_NOINLINE zSndSampleSet *RECOIL_THISCALL RegistryAddEntry(const char *name,
                                                                    int count);
    RECOIL_NOINLINE zSndSample *RECOIL_THISCALL GetSampleAt(int index);
    RECOIL_NOINLINE zSndSample *RECOIL_THISCALL FindSampleByName(const char *sampleName);
    RECOIL_NOINLINE int RECOIL_THISCALL Init();
    RECOIL_NOINLINE int RECOIL_THISCALL
    LoadSamplesFromIndexArchive(zIndexArchive *archive);
    RECOIL_NOINLINE int RECOIL_THISCALL Destroy();
    RECOIL_NOINLINE void RECOIL_THISCALL DestroyOwnedData();
};

struct zSndSampleSetRegistry {
    int useArchiveBanksFlag;
    zSndSampleSet **begin;
    zSndSampleSet **end;
    zSndSampleSet **capacityEnd;
};

struct zSndGroupConfigBlock {
    unsigned short currentPlayCount;
    unsigned short maxPlayCount;
    float delayPlaySec;
    float weight;
    const char *streamName;
    int unknown_10;
    zSndGroupConfigBlock *child;
};

struct zSndGroupRuntimeFields {
    const char *groupName;
    int dynamicWeightsEnabled;
    int playSolo;
    float dynamicWeightScale;
    unsigned short repeatCount;
    unsigned short unknown_12;
    float delayRepeatSec;
    float delayTerminationSec;
    int configBlockCount;
    zSndGroupConfigBlock *configBlocks;
};

struct zSndGroup {
    int createGuard;
    const char *groupName;
    int dynamicWeightsEnabled;
    int playSolo;
    float dynamicWeightScale;
    unsigned short repeatCount;
    unsigned short unknown_16;
    float delayRepeatSec;
    float delayTerminationSec;
    int configBlockCount;
    zSndGroupConfigBlock *configBlocks;
    char unknown_28[0x90];

    RECOIL_NOINLINE zSndGroupConfigBlock *RECOIL_THISCALL SelectWeightedEntry();
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL QueueStreamRequest(int hasWorldPos,
                                                                       float gain, zVec3 *worldPos,
                                                                       zVec3 *velocity);
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL QueueStreamRequestSimple(float gain);
    RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL QueueStreamRequestWithWorldPos(zVec3 *worldPos,
                                                                                   float gain,
                                                                                   zVec3 *velocity);
};

struct zSndStreamRequest {
    int isActive;
    zSndPlayHandleKind handleKind;
    int hasWorldPos;
    zVec3 worldPos;
    zVec3 velocity;
    float gain;
    float elapsedSec;
    int playIndex;
    zSndGroupConfigBlock *currentEntry;
    int streamState;
    zSndGroup *group;

    RECOIL_NOINLINE int RECOIL_THISCALL StateBeginGroup();
    void RECOIL_THISCALL StateWaitTerminationDelay();
};

struct zSndFadeEntry {
    float targetValue;
    float currentValue;
    zSndPlayHandle *handle;
    int stopOnComplete;

    RECOIL_NOINLINE int RECOIL_THISCALL TickAndMaybeDispatch(float deltaTime);
};

struct zSndFadeListNode {
    zSndFadeListNode *next;
    zSndFadeListNode *prev;
    zSndFadeEntry *fadeEntry;
};

struct zSndFadeList {
    unsigned int flags;
    zSndFadeListNode *sentinel;
    int count;

    RECOIL_NOINLINE void RECOIL_THISCALL DeleteNodeAndAdvanceCursor(zSndFadeListNode *node,
                                                                    zSndFadeListNode **outCursor);
};

struct zSndFadeListCursor {
    zSndFadeListNode *node;

    RECOIL_NOINLINE zSndFadeListNode **RECOIL_THISCALL PopFrontCursor(zSndFadeListNode **outNode,
                                                                      int unused);
};

struct zSndCdTrackEntry {
    char *archiveName;
    int trackNumber;
};

struct zSndCdTrackNode {
    zSndCdTrackNode *next;
    zSndCdTrackNode *prev;
    zSndCdTrackEntry *entry;
};

namespace zReader {
struct Node;
}

RECOIL_STATIC_ASSERT(sizeof(zSndSampleReplayFields) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndPlayHandle) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(zSndPlayHandleSnapshotPayload) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(zSndPlayHandleSnapshotItem) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zSndPlayHandleSnapshot) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zSndListenerState) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zSndQualityVariant) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndCuePoint) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zSndWaveData) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zSndSample) == 0xb8);
RECOIL_STATIC_ASSERT(sizeof(zSndSampleSet) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndSampleSetRegistry) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndGroupConfigBlock) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zSndGroupRuntimeFields) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zSndGroup) == 0xb8);
RECOIL_STATIC_ASSERT(sizeof(zSndStreamRequest) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(zSndFadeEntry) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndFadeListNode) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zSndFadeList) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zSndCdTrackNode) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zSndCdTrackEntry) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshotPayload, volumeScaleRaw) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshotPayload, worldPos) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshotPayload, velocityOrDir) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshotItem, payload) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshot, listHead) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshot, itemCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndCdTrackNode, entry) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndSampleSet, sampleCount) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndSampleSet, samples) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndSampleSetRegistry, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndGroupConfigBlock, child) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zSndGroup, groupName) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndGroup, configBlocks) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, hasWorldPos) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, gain) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, streamState) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, group) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeEntry, currentValue) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeEntry, handle) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeEntry, stopOnComplete) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeListNode, fadeEntry) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeList, sentinel) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeList, count) == 0x08);

namespace zSnd {
RECOIL_NOINLINE int RECOIL_FASTCALL ReportA3DError(int a3dError,
                                                            const char *sourceFile,
                                                            int sourceLine);
RECOIL_NOINLINE int RECOIL_FASTCALL ReportDirectSoundError(int directSoundError,
                                                                    const char *sourceFile,
                                                                    int sourceLine);
RECOIL_NOINLINE int RECOIL_FASTCALL SetAudioApiOption(int apiType);
RECOIL_NOINLINE void RECOIL_FASTCALL SetSpeedOfSoundMps(float speedOfSoundMps);
RECOIL_NOINLINE int RECOIL_CDECL GetAudioApiOption();
RECOIL_NOINLINE void RECOIL_FASTCALL SetCDAudioOption(int cdAudioOption);
RECOIL_NOINLINE int RECOIL_CDECL GetCDAudioOption();
RECOIL_NOINLINE int RECOIL_FASTCALL SetActiveBackendPreInit(int backend);
RECOIL_NOINLINE int RECOIL_CDECL GetActiveBackend();
RECOIL_NOINLINE void RECOIL_FASTCALL SetUseArchiveBanksFlag(int useArchiveBanks);
RECOIL_NOINLINE zSndSample *RECOIL_FASTCALL FindSampleByName(const char *sampleName);
RECOIL_NOINLINE int RECOIL_STDCALL GainScaleToDirectSoundAttenuation(float gainScale);
RECOIL_NOINLINE int RECOIL_FASTCALL ApplyMuteStateToActiveVoices(int enableMute);
RECOIL_NOINLINE int RECOIL_CDECL IsMuted();
RECOIL_NOINLINE float RECOIL_STDCALL MulGlobalVolumeScaleAndGetPrev(float scale);
RECOIL_NOINLINE float RECOIL_STDCALL SetGlobalVolumeScale(float scale);
RECOIL_NOINLINE void RECOIL_FASTCALL SetFlag10PlaybackEnabled(int enabled);
RECOIL_NOINLINE int RECOIL_CDECL HasMmxMixerSupport();
RECOIL_NOINLINE LPDIRECTSOUND RECOIL_FASTCALL AcquireCachedDirectSound(LPCGUID deviceGuid);
RECOIL_NOINLINE void RECOIL_CDECL ReleaseCachedDirectSound();
RECOIL_NOINLINE HRESULT RECOIL_FASTCALL CachedDirectSound_GetCaps(DSCAPS *caps);
} // namespace zSnd

namespace zSndCd {
RECOIL_NOINLINE int RECOIL_FASTCALL Init(zReader::Node *cdTracksNode);
RECOIL_NOINLINE int RECOIL_CDECL Stop();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL GetTrackCount();
RECOIL_NOINLINE int RECOIL_FASTCALL PlayTrackWithMode(int trackIndex,
                                                               int playbackMode);
} // namespace zSndCd

extern "C" RECOIL_NOINLINE zSndSample *RECOIL_FASTCALL zSndSample_CreateQueuedStreamingSample(
    WAVEFORMATEX *audioFormat, void *audioBuffer, int bufferBytes);

extern "C" {
extern int *ZOPT_AUDIO_API;
extern int *ZOPT_SOUND_CDAUDIO;
extern int g_zSnd_IsInitialized;
extern int g_zSnd_PreInitialized;
extern int g_zSnd_ActiveBackend;
extern unsigned int g_zSnd_WindowHandle;
extern int g_zSnd_UseArchiveBanksFlag;
extern int g_zSnd_SoundLodDefault;
extern void *g_zSnd_SoundLodValuePtr;
extern int g_zSnd_MuteOptionDefault;
extern void *g_zSnd_MuteOptionValuePtr;
extern int g_zSnd_MuteDepth;
extern float g_zSnd_VolumeScaleDefault;
extern void *g_zSnd_GlobalVolumeScalePtr;
extern zSndSample *g_zSndLastSample;
extern zSndSample *g_zSndLastVoice;
extern zSndPlayHandle *g_zSndLastVoiceHandle;
extern int g_zSndLastVoiceMarkerIndex;
extern int g_zSndLastVoiceStopMarkerIndex;
extern int g_zSnd_Flag10PlaybackEnabled;
extern zSndSampleSetRegistry g_zSnd_SampleSetRegistry;
extern zReader::Node *g_zSnd_ConfigRootNode;
extern zArchiveList *g_zSnd_SearchPathList;
extern int g_zSnd_ListenerStateValid;
extern zSndListenerState g_zSnd_ListenerState;
extern zVec3 g_zSnd_ListenerVelocity;
extern zVec3 g_zSnd_PreviousListenerPos;
extern zArchiveList *g_zSndStream_PendingList;
extern zArchiveList *g_zSndStream_ActiveList;
extern zArchiveList *g_zSndStream_FreeList;
extern zSndStreamRequest *g_zSndStream_MatchedRequest;
extern int g_zSndStream_MatchedRequestCount;
extern zClass_NodePartial *g_zSndStream_RootNode;
extern unsigned int g_zSndFadeActiveListFlags;
extern zSndFadeListNode *g_zSndFadeActiveListSentinel;
extern int g_zSndFadeActiveListCount;
extern unsigned int g_zSndFadeDispatchListFlags;
extern zSndFadeListNode *g_zSndFadeDispatchListSentinel;
extern int g_zSndFadeDispatchListCount;
extern int g_zSndCdFlags;
extern int g_zSndCdLastPlayMode;
extern int g_zSndCdDeviceId;
extern int g_zSndCdAuxDeviceId;
extern zSndCdTrackNode *g_zSndCd_TrackListHead;
extern int g_zSndCd_TrackCount;
extern int g_zSndCdDiscLengthMinute;
extern int g_zSndCdDiscLengthSecond;
extern unsigned short g_zSndCdAuxVolumePrimary;
extern unsigned short g_zSndCdAuxVolumeSecondary;
extern float g_zSndSpeedOfSoundMps;
extern float g_zSndInvSpeedOfSoundMps;
}

extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndBackend_InitA3D();
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndBackend_InitDirectSound();
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSnd_PreInitializeRuntimeState(unsigned int hwnd);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSnd_UpdateListenerState(zSndListenerState *listenerState, zVec3 *listenerVelocity);
extern "C" RECOIL_NOINLINE float RECOIL_CDECL zSnd_GetSpeedOfSoundMps();

extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zSndSystem_Init(unsigned int hwnd,
                                                                        const char *zrdPath);
namespace zSndSystem {
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
}
namespace zSndBackend {
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
}
namespace zSndStreamMgr {
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
}
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndSampleSetRegistry_DestroyAll();
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndSampleSetRegistry_Shutdown();
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndSampleSetRegistry_RegisterAtExit();
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zSnd_SetUseArchiveBanks(int enabled);
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL
zSnd_SetUseArchiveBanksAndRegisterAtExit(int enabled);
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndSampleSetRegistry_GetCount();
extern "C" RECOIL_NOINLINE zSndSampleSet *RECOIL_FASTCALL
zSndSampleSetRegistry_GetByIndex(int index);
extern "C" RECOIL_NOINLINE zSndSampleSet *RECOIL_FASTCALL
zSndSampleSetRegistry_FindByName(const char *setName);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSampleSet_DestroyByName(const char *setName);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSampleSet_InitByName(const char *setName);
namespace zSndFadeLists {
RECOIL_NOINLINE void RECOIL_CDECL InitGlobals();
RECOIL_NOINLINE void RECOIL_CDECL StopAllAndShutdown();
} // namespace zSndFadeLists
namespace zSndFadeDispatchList {
RECOIL_NOINLINE void RECOIL_FASTCALL PushBack(zSndFadeEntry *fadeEntry);
}
extern "C" RECOIL_NOINLINE void RECOIL_STDCALL zSndFadeActiveList_TickAll(float deltaTime);
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zSnd_Tick(int skipA3dCommit);
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zSnd_TickWrapper(int skipA3dCommit);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSystem_InitNamedSetsSyntax(zReader::Node *configRootNode);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSystem_InitLegacySetsSyntax(zReader::Node *configRootNode);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndGroup_LoadConfigBlock(zReader::Node *readerNode, zSndGroupRuntimeFields *groupFields,
                          zSndGroupConfigBlock *outConfigBlock);
extern "C" RECOIL_NOINLINE zSndGroup *RECOIL_FASTCALL
zSndGroup_LoadFromConfigNode(zReader::Node *readerNode);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndGroup_QueuePendingLoadsFromConfigNode(zReader::Node *readerNode);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndStreamRequest_StopIfActive(zSndPlayHandle *request);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPlayHandle_TryEnableManaged(zSndPlayHandle *handle);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPlayHandle_TryDisableManaged(zSndPlayHandle *handle);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndStreamRequest_MatchGroupPredicate(void *payload, void *group);
extern "C" RECOIL_NOINLINE float RECOIL_STDCALL zSndSample_PlaySimple(float value);
extern "C" RECOIL_NOINLINE zSndSample *RECOIL_FASTCALL
zSndPendingList_FindByName(const char *sampleName);
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPendingList_MatchNamePredicate(void *payload, void *sampleName);
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndStreamMgr_EnsureInit();
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndStreamMgr_RecycleFinishedRequest();
