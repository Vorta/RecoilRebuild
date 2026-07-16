#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

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
#include "zclass.h"

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

    int StopIfActive();
    int SetFreqScaled(float scale);
    void SetEnableScale(float scale);
    int __fastcall Update3DDispatch(
        zVec3 *worldPos,
        zVec3 *velocity,
        int velocityScaleMode
    );
    int __fastcall Update3D(
        zVec3 *worldPos,
        zVec3 *velocity,
        int velocityScaleMode
    );
    int __fastcall Update3D_A3D(
        zVec3 *worldPos,
        zVec3 *velocity,
        int velocityScaleMode
    );
    static void __fastcall PlayWithDelta_A3D(
        zSndSampleReplayFields *replayFields,
        zSndPlayHandle *playHandle,
        int restartBeforePlay,
        float gainDelta
    );
    static void __fastcall PlayWithDelta_DirectSound(
        zSndSampleReplayFields *replayFields,
        zSndPlayHandle *playHandle,
        int restartBeforePlay,
        int gainDelta
    );
    static void __fastcall PlayWithDelta_BackendDispatch(
        zSndSample *sourceSample,
        zSndPlayHandle *playHandle,
        int restartBeforePlay,
        float gainDelta
    );
};

struct zSndPlayHandleSnapshotPayload {
    zSndPlayHandle *playHandle;
    zSndSample *sourceSample;
    unsigned int volumeScaleRaw;
    unsigned int flags;
    zVec3 worldPos;
    zVec3 velocityOrDir;

    void __fastcall CaptureFromPlayHandle(zSndPlayHandle *playHandle);
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

    static zSndPlayHandleSnapshot *CreateFromActiveSamples();
    void AppendPayload(const zSndPlayHandleSnapshotPayload &payload);
    zSndPlayHandleSnapshotItem * NewNode(
        zSndPlayHandleSnapshotItem *listHead,
        zSndPlayHandleSnapshotItem *prev
    );

    int StopAllIfPlaying();
    int RestoreAllWithGlobalVolumeDelta();
    int Destroy();
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

    zSndWaveData(
        const char *path,
        int loadNow
    );
    ~zSndWaveData();
    void Destructor();
    int ParseLoadedWaveFile();
    int LoadAndParseIfNeeded();
    int LoadAndParseFromIndexArchiveIfNeeded(
        zIndexArchive *archive
    );
    int Reset();
};

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in caller 0x4a0fb0.
 * Evidence: BN emits delete-shaped cleanup through the address-backed 0x4a5440
 * cleanup routine.
 * Purpose: release wave-data storage through the recovered cleanup body.
 */
inline zSndWaveData::~zSndWaveData() {
    Destructor();
}

struct zSndSample {
    int createGuard;
    zSndSampleReplayFields replayFields;
    float rangeMin;
    float rangeMax;
    float playbackParam2;
    float playbackParam3;
    float sampleRate;
    float a3dDistanceScale;
    void(__fastcall *playbackEventHandler)(int eventCode);
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

    zSndPlayHandle * AcquirePlayHandleDispatch();
    zSndPlayHandle * AcquireA3dVoice();
    zSndPlayHandle * AcquireVoice();
    zSndPlayHandle *__fastcall PlayOnActiveBackend(
        zVec3 *worldPos,
        float gainScale,
        zVec3 *velocity,
        int backendArg
    );
    zSndPlayHandle *__fastcall PlayOnA3D(
        zVec3 *worldPos,
        float gainScale,
        zVec3 *velocity,
        int backendArg
    );
    zSndPlayHandle *__fastcall PlayOnDirectSound(
        int attenuation,
        zVec3 *worldPos,
        zVec3 *velocity,
        int backendArg
    );
    zSndPlayHandle *__fastcall PlayA3D(
        zVec3 *worldPos,
        float gainScale,
        zVec3 *velocity
    );
    zSndPlayHandle *__fastcall PlayDirectSound(
        int variantIndex,
        float gainScale,
        int stopMarkerIndex
    );
    zSndPlayHandle * PlayA3DSimple(float gainScale);
    int StopActiveVoicesIfPlaying();
    int __fastcall InitFromWaveData(zSndWaveData *waveData);
    int __fastcall InitFromWaveData_DirectSound(zSndWaveData *waveData);
    int __fastcall InitFromWaveData_A3D(zSndWaveData *waveData);
    int __fastcall LockBackendBuffers(
        unsigned int offset,
        unsigned int bytes,
        void **buffer1,
        int *buffer1Bytes,
        void **buffer2,
        int *buffer2Bytes
    );
    int __fastcall UnlockBackendBuffers(
        void *buffer1,
        int buffer1Bytes,
        void *buffer2,
        int buffer2Bytes
    );
    unsigned int GetPlayCursorBytes();
    void __fastcall SetPlaybackEventHandler(
        void(__fastcall *callback)(int eventCode)
    );
    int DestroyOwnedData();
    void Destroy();
};

struct zSndSampleSet {
    char *setName;
    int sampleCount;
    zSndSample *samples;
    int resourcesLoaded;

    zSndSampleSet * RegistryAddEntry(
        const char *name,
        int count
    );
    zSndSample * GetSampleAt(int index);
    zSndSample * FindSampleByName(const char *sampleName);
    int Init();
    int LoadSamplesFromIndexArchive(zIndexArchive *archive);
    int Destroy();
    void DestroyOwnedData();
};

struct zSndSampleSetRegistry {
    unsigned char useArchiveBanksFlag;
    unsigned char unknown_01[3];
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
    zSndSample *cachedSample;
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

    zSndGroupConfigBlock * SelectWeightedEntry();
    zSndPlayHandle *__fastcall QueueStreamRequest(
        int hasWorldPos,
        float gain,
        zVec3 *worldPos,
        zVec3 *velocity
    );
    zSndPlayHandle * QueueStreamRequestSimple(float gain);
    zSndPlayHandle *__fastcall QueueStreamRequestWithWorldPos(
        zVec3 *worldPos,
        float gain,
        zVec3 *velocity
    );
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

    int StateBeginGroup();
    void StatePlayCurrentEntry();
    void StateWaitRepeatDelay();
    void StateWaitTerminationDelay();
};

struct zSndFadeEntry {
    float targetValue;
    float currentValue;
    zSndPlayHandle *handle;
    int stopOnComplete;

    int TickAndMaybeDispatch(float deltaTime);
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

    void DeleteNodeAndAdvanceCursor(
        zSndFadeListNode **outCursor,
        zSndFadeListNode *node
    );
};

struct zSndFadeListCursor {
    zSndFadeListNode *node;

    zSndFadeListNode ** PopFrontCursor(
        zSndFadeListNode **outNode,
        int unused
    );
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
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndPlayHandleSnapshotPayload,
        volumeScaleRaw
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndPlayHandleSnapshotPayload,
        worldPos
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndPlayHandleSnapshotPayload,
        velocityOrDir
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndPlayHandleSnapshotItem,
        payload
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndPlayHandleSnapshot,
        listHead
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndPlayHandleSnapshot,
        itemCount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndCdTrackNode,
        entry
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndSampleSet,
        sampleCount
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndSampleSet,
        samples
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndSampleSetRegistry,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndGroupConfigBlock,
        cachedSample
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndGroupConfigBlock,
        child
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndGroup,
        groupName
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndGroup,
        configBlocks
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndStreamRequest,
        hasWorldPos
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndStreamRequest,
        gain
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndStreamRequest,
        streamState
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndStreamRequest,
        group
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndFadeEntry,
        currentValue
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndFadeEntry,
        handle
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndFadeEntry,
        stopOnComplete
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndFadeListNode,
        fadeEntry
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndFadeList,
        sentinel
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zSndFadeList,
        count
    ) == 0x08
);

namespace zSnd {
int __fastcall ReportA3DError(
    int a3dError,
    const char *sourceFile,
    int sourceLine
);
int __fastcall ReportDirectSoundError(
    int directSoundError,
    const char *sourceFile,
    int sourceLine
);
int __fastcall SetAudioApiOption(int apiType);
void __fastcall SetSpeedOfSoundMps(float speedOfSoundMps);
int GetAudioApiOption();
void __fastcall SetCDAudioOption(int cdAudioOption);
int GetCDAudioOption();
int __fastcall SetActiveBackendPreInit(int backend);
int GetActiveBackend();
void __fastcall SetUseArchiveBanksFlag(int useArchiveBanks);
zSndSample *__fastcall FindSampleByName(const char *sampleName);
int __stdcall GainScaleToDirectSoundAttenuation(float gainScale);
int __fastcall ApplyMuteStateToActiveVoices(int enableMute);
int IsMuted();
float __stdcall MulGlobalVolumeScaleAndGetPrev(float scale);
float __stdcall SetGlobalVolumeScale(float scale);
void __fastcall SetFlag10PlaybackEnabled(int enabled);
int HasMmxMixerSupport();
LPDIRECTSOUND __fastcall AcquireCachedDirectSound(LPGUID deviceGuid);
void ReleaseCachedDirectSound();
HRESULT __fastcall CachedDirectSound_GetCaps(DSCAPS *caps);
} // namespace zSnd

namespace zSndCd {
int __fastcall Init(zReader::Node *cdTracksNode);
int Stop();
int Shutdown();
int GetTrackCount();
int __fastcall PlayTrackWithMode(
    int trackIndex,
    int playbackMode
);
int __fastcall GetVolume(
    unsigned short *primaryVolumeOut,
    unsigned short *secondaryVolumeOut
);
int __fastcall SetVolume(
    unsigned short primaryVolume,
    unsigned short secondaryVolume
);
} // namespace zSndCd

extern "C" zSndSample *__fastcall zSndSample_CreateQueuedStreamingSample(
    WAVEFORMATEX *audioFormat,
    void *audioBuffer,
    int bufferBytes
);

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
extern unsigned char g_zSndCd_TrackListCtorGuard;
extern zSndCdTrackNode *g_zSndCd_TrackListHead;
extern int g_zSndCd_TrackCount;
extern int g_zSndCdDiscLengthMinute;
extern int g_zSndCdDiscLengthSecond;
extern unsigned short g_zSndCdAuxVolumePrimary;
extern unsigned short g_zSndCdAuxVolumeSecondary;
extern float g_zSndSpeedOfSoundMps;
extern float g_zSndInvSpeedOfSoundMps;
}

extern char g_zSndConfig_SoundGroupsKey[0x0d];
extern char g_zSndConfig_SetsKey[0x05];
extern char g_zSndConfig_SpeedOfSoundKey[0x0f];
extern char g_zSndConfig_SoundPathKey[0x0b];
extern char g_zSndConfig_CdTracksKey[0x0a];
extern char g_zSndConfig_QualityMedToken[0x04];
extern char g_zSndConfig_A3dDistanceKey[0x08];
extern char g_zSndConfig_VolumeKey[0x07];
extern char g_zSndConfig_VoiceKey[0x06];
extern char g_zSndConfig_PurgeableKey[0x0a];
extern char g_zSndConfig_HardwareKey[0x09];
extern char g_zSndConfig_FrequencyKey[0x0a];
extern char g_zSndConfig_LoopedKey[0x07];
extern char g_zSndConfig_3dKey[0x03];

extern "C" int zSndBackend_InitA3D();
extern "C" int zSndBackend_InitDirectSound();
extern "C" int __fastcall zSnd_PreInitializeRuntimeState(unsigned int hwnd);
extern "C" int __fastcall zSnd_UpdateListenerState(
    zSndListenerState *listenerState,
    zVec3 *listenerVelocity
);
extern "C" float zSnd_GetSpeedOfSoundMps();

extern "C" int __fastcall zSndSystem_Init(
    unsigned int hwnd,
    const char *zrdPath
);
namespace zSndSystem {
int Shutdown();
}
namespace zSndBackend {
int Shutdown();
}
namespace zSndCdTrackList {
void __cdecl StaticInit();
void StaticConstructor();
void RegisterAtExitDestructor();
void __cdecl StaticDestructor();
}
namespace zSndStreamMgr {
int __fastcall UpdateActiveRequestPredicate(
    void *payload,
    void *userData
);
int Shutdown();
}
extern "C" void zSndSampleSetRegistry_DestroyAll();
extern "C" void __cdecl zSndSampleSetRegistry_Shutdown();
extern "C" void zSndSampleSetRegistry_RegisterAtExit();
extern "C" void __fastcall zSnd_SetUseArchiveBanks(unsigned char enabled);
extern "C" void __fastcall zSnd_SetUseArchiveBanksAndRegisterAtExit(
    unsigned char enabled
);
extern "C" int zSndSampleSetRegistry_GetCount();
extern "C" zSndSampleSet *__fastcall zSndSampleSetRegistry_GetByIndex(
    int index
);
extern "C" zSndSampleSet *__fastcall zSndSampleSetRegistry_FindByName(
    const char *setName
);
extern "C" int __fastcall zSndSampleSet_DestroyByName(const char *setName);
extern "C" int __fastcall zSndSampleSet_InitByName(const char *setName);
namespace zSndFadeLists {
void Init();
void InitGlobals();
void RegisterShutdownAtExit();
void __cdecl ShutdownAtExit();
void StopAllAndShutdown();
} // namespace zSndFadeLists
namespace zSndFadeDispatchList {
void __fastcall PushBack(zSndFadeEntry *fadeEntry);
}
extern "C" void __stdcall zSndFadeActiveList_TickAll(float deltaTime);
extern "C" void __fastcall zSnd_Tick(int skipA3dCommit);
extern "C" int __fastcall zSndSystem_InitNamedSetsSyntax(
    zReader::Node *configRootNode
);
extern "C" int __fastcall zSndSystem_InitLegacySetsSyntax(
    zReader::Node *configRootNode
);
extern "C" int __fastcall zSndGroup_LoadConfigBlock(
    zReader::Node *readerNode,
    zSndGroupRuntimeFields *groupFields,
    zSndGroupConfigBlock *outConfigBlock
);
extern "C" zSndGroup *__fastcall zSndGroup_LoadFromConfigNode(
    zReader::Node *readerNode
);
extern "C" int __fastcall zSndGroup_QueuePendingLoadsFromConfigNode(
    zReader::Node *readerNode
);
extern "C" int __fastcall zSndStreamRequest_StopIfActive(
    zSndPlayHandle *request
);
extern "C" int __fastcall zSndPlayHandle_TryEnableManaged(
    zSndPlayHandle *handle
);
extern "C" int __fastcall zSndPlayHandle_TryDisableManaged(
    zSndPlayHandle *handle
);
extern "C" int __fastcall zSndStreamRequest_MatchGroupPredicate(
    void *payload,
    void *group
);
extern "C" float __stdcall zSndSample_PlaySimple(float value);
extern "C" zSndSample *__fastcall zSndPendingList_FindByName(
    const char *sampleName
);
extern "C" int __fastcall zSndPendingList_MatchNamePredicate(
    void *payload,
    void *sampleName
);
extern "C" int zSndStreamMgr_EnsureInit();
extern "C" void zSndStreamMgr_RecycleFinishedRequest();
