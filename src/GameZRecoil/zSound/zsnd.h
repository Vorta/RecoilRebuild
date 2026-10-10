#pragma once
#ifdef __cplusplus
#include "recoil/recoil_types.h"
#include <list>
#include <stddef.h>
#include <vector>

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

extern "C" LPDIRECTSOUND g_zSnd_CachedDirectSound;
extern "C" const GUID* g_zSnd_CachedDirectSoundGuid;

struct zSndBuffer;
struct zSndSample;
struct zArchiveList;
struct zIndexArchive;

enum zSndPlayHandleKind {
    ZSND_PLAYHANDLE_BACKEND = 0,
    ZSND_PLAYHANDLE_STREAM_REQUEST = 1,
};

struct zSndSampleReplayFlagBits {
    unsigned int looping : 1;
    unsigned int flag02 : 1;
    unsigned int flag04 : 1;
    unsigned int loaded : 1;
    unsigned int flag10 : 1;
    unsigned int flag20 : 1;
    unsigned int flag40 : 1;
    unsigned int flag80 : 1;
    unsigned int streaming : 1;
};

struct zSndSampleReplayFields {
    const char* resourceName;
    const char* sampleId;
    union {
        int flags;
        zSndSampleReplayFlagBits flagBits;
    };
    float gain;
};

struct zSndPlayHandle {
    int isActive;
    zSndPlayHandleKind handleKind;
    zSndBuffer* backendBuffer;
    zVec3 worldPos;
    zVec3 velocityOrDir;
    int gainScaled;
    int hasWorldPos;
    struct zSndSample* ownerSample;
    int backendState0;
    int backendState1;
    int backendState2;

    /* SetFreqScaled is the C function zSndPlayHandleSetFreqScaled (zsnd_parm.cpp). */
    int __fastcall SetEnableScale(float scale);
    int __fastcall Update3D(zVec3* worldPos, zVec3* velocity, int velocityScaleMode);
    int __fastcall Update3DA3D(zVec3* worldPos, zVec3* velocity, int velocityScaleMode);
    static void __fastcall PlayWithDeltaA3D(
        zSndSampleReplayFields* replayFields,
        zSndPlayHandle* playHandle,
        int restartBeforePlay,
        float gainDelta
    );
    static void __fastcall PlayWithDeltaDirectSound(
        zSndSampleReplayFields* replayFields,
        zSndPlayHandle* playHandle,
        int restartBeforePlay,
        int gainDelta
    );
    static void __fastcall PlayWithDeltaBackendDispatch(
        zSndSample* sourceSample,
        zSndPlayHandle* playHandle,
        int restartBeforePlay,
        float gainDelta
    );
};

struct zSndPlayHandleSnapshotPayload {
    zSndPlayHandle* playHandle;
    zSndSample* sourceSample;
    unsigned int volumeScaleRaw;
    unsigned int flags;
    zVec3 worldPos;
    zVec3 velocityOrDir;

    void __fastcall CaptureFromPlayHandle(zSndPlayHandle* playHandle);
};

/**
 * Original VC5 snapshot container: a std::list of captured payloads; the first
 * entry is the global-volume anchor.
 * Evidence: retail 0x49fff0 copies the empty allocator byte from an
 * uninitialized constructor temporary, builds the sentinel with the inlined
 * list::_Buynode, appends through the inlined list::insert (out-of-line
 * _Buynode 0x4a07c0: ret 8 with an unused this) and guards each payload copy
 * with the placement-new null test of allocator::construct; retail 0x4a05f0
 * is the inlined ~list() of a plain list pointer (no derived destructor call).
 */
typedef std::list<zSndPlayHandleSnapshotPayload> zSndPlayHandleSnapshotList;

namespace zSnd {
zSndPlayHandleSnapshotList* __cdecl CreateSnapshotFromActiveSamples();
int __fastcall StopSnapshotVoicesIfPlaying(zSndPlayHandleSnapshotList* snapshot);
int __fastcall RestoreSnapshotWithGlobalVolumeDelta(zSndPlayHandleSnapshotList* snapshot);
int __fastcall DestroySnapshot(zSndPlayHandleSnapshotList* snapshot);
} // namespace zSnd

struct zSndListenerState {
    zVec3 right;
    zVec3 up;
    zVec3 forward;
    zVec3 position;
};

struct zSndQualityVariant {
    const char* sampleName;
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
    char* nameOrPath;
    int fileSize;
    void* fileData;
    int pcmByteCount;
    WAVEFORMATEX* fmt;
    int cuePointCount;
    zSndCuePoint* cuePoints;
    void* pcmData;

    zSndWaveData(const char* path, int loadNow);
    ~zSndWaveData();
    int ParseLoadedWaveFile();
    int LoadAndParseIfNeeded();
    int Reset();
    int LoadAndParseFromIndexArchiveIfNeeded(zIndexArchive* archive);
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
    void(__fastcall* playbackEventHandler)(int eventCode);
    float markerBaseTime;
    int markerCount;
    float* markerTimes;
    float* markerValues;
    int* markerAux;
    zSndPlayHandle primaryVoice;
    int duplicateVoiceCount;
    zSndPlayHandle** duplicateVoices;
    zSndQualityVariant highVariant;
    zSndQualityVariant medVariant;
    zSndQualityVariant lowVariant;

    zSndPlayHandle* AcquirePlayHandleDispatch();
    zSndPlayHandle* AcquireA3dVoice();
    zSndPlayHandle* AcquireVoice();
    zSndPlayHandle* __fastcall PlayOnActiveBackend(float gainScale, zVec3* worldPos, zVec3* velocity, int backendArg);
    zSndPlayHandle* __fastcall PlayOnA3D(zVec3* worldPos, float gainScale, zVec3* velocity, int backendArg);
    zSndPlayHandle* __fastcall PlayOnDirectSound(int attenuation, zVec3* worldPos, zVec3* velocity, int backendArg);
    zSndPlayHandle* __fastcall PlayDirectSound(int variantIndex, float gainScale, int stopMarkerIndex);
    int StopActiveVoicesIfPlaying();
    int __fastcall InitFromWaveData(zSndWaveData* waveData);
    int __fastcall InitFromWaveDataDirectSound(zSndWaveData* waveData);
    int __fastcall InitFromWaveDataA3D(zSndWaveData* waveData);
    int __fastcall LockBackendBuffers(
        unsigned int offset,
        unsigned int bytes,
        void** buffer1,
        void** buffer2,
        int* buffer1Bytes,
        int* buffer2Bytes
    );
    int __fastcall UnlockBackendBuffers(void* buffer1, void* buffer2, int buffer1Bytes, int buffer2Bytes);
    unsigned int __fastcall GetPlayCursorBytes();
    void __fastcall SetPlaybackEventHandler(void(__fastcall* callback)(int eventCode));
    int DestroyOwnedData();
    void Destroy();
};

struct zSndSampleSet {
    char* setName;
    int sampleCount;
    zSndSample* samples;
    int resourcesLoaded;

    zSndSampleSet(const char* name, int count);
    zSndSample* GetSampleAt(int index);
    zSndSample* FindSampleByName(const char* sampleName);
    int Init();
    int LoadSamplesFromIndexArchive(zIndexArchive* archive);
    int Destroy();
    void DestroyOwnedData();
};

/**
 * Original VC5 sample-set registry container.
 * Evidence: retail 0x4a09e0 is the inlined std::vector push-back/growth path,
 * including the VC5 copy, uninitialized-fill, destroy-range, allocation, and
 * deallocation COMDATs over the object stored at 0x56b290.
 * Purpose: own the ordered set of loaded zSndSampleSet pointers.
 */
typedef std::vector<zSndSampleSet*> zSndSampleSetRegistry;

struct zSndGroupConfigBlock {
    unsigned short currentPlayCount;
    unsigned short maxPlayCount;
    float delayPlaySec;
    float weight;
    const char* streamName;
    zSndSample* cachedSample;
    zSndGroupConfigBlock* child;
};

struct zSndGroupRuntimeFields {
    const char* groupName;
    int dynamicWeightsEnabled;
    int playSolo;
    float dynamicWeightScale;
    unsigned short repeatCount;
    unsigned short unknown_12;
    float delayRepeatSec;
    float delayTerminationSec;
    int configBlockCount;
    zSndGroupConfigBlock* configBlocks;
};

struct zSndGroup {
    int createGuard;
    // Retail passes &fields (lea [group+4]) to zSndGroupLoadConfigBlock.
    zSndGroupRuntimeFields fields;
    char unknown_28[0x90];

    zSndGroupConfigBlock* SelectWeightedEntry();
    zSndPlayHandle* __fastcall QueueStreamRequest(float gain, int hasWorldPos, zVec3* worldPos, zVec3* velocity);
    zSndPlayHandle* QueueStreamRequestSimple(float gain);
    zSndPlayHandle* __fastcall QueueStreamRequestWithWorldPos(zVec3* worldPos, float gain, zVec3* velocity);
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
    zSndGroupConfigBlock* currentEntry;
    int streamState;
    zSndGroup* group;

    int StateBeginGroup();
    void StatePlayCurrentEntry();
    void StateWaitRepeatDelay();
    void StateWaitTerminationDelay();
};

struct zSndFadeEntry {
    float targetValue;
    float currentValue;
    zSndPlayHandle* handle;
    int stopOnComplete;

    int TickAndMaybeDispatch(float deltaTime);
};

struct zSndCdTrackEntry {
    char* archiveName;
    int trackNumber;

    zSndCdTrackEntry(const char* name, int track)
    {
        trackNumber = track;
        archiveName = _strdup(name);
    }
};

namespace zReader {
struct Node;
}

RECOIL_STATIC_ASSERT(sizeof(zSndSampleReplayFields) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndPlayHandle) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(zSndPlayHandleSnapshotPayload) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(zSndPlayHandleSnapshotList) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zSndListenerState) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zSndQualityVariant) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndCuePoint) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zSndWaveData) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zSndSample) == 0xb8);
RECOIL_STATIC_ASSERT(sizeof(zSndSampleSet) == 0x10);
#if defined(_MSC_VER) && _MSC_VER < 1200
RECOIL_STATIC_ASSERT(sizeof(zSndSampleSetRegistry) == 0x10);
#endif
RECOIL_STATIC_ASSERT(sizeof(zSndGroupConfigBlock) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zSndGroupRuntimeFields) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zSndGroup) == 0xb8);
RECOIL_STATIC_ASSERT(sizeof(zSndStreamRequest) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(zSndFadeEntry) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zSndCdTrackEntry) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshotPayload, volumeScaleRaw) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshotPayload, worldPos) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zSndPlayHandleSnapshotPayload, velocityOrDir) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zSndSampleSet, sampleCount) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndSampleSet, samples) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndGroupConfigBlock, cachedSample) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zSndGroupConfigBlock, child) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zSndGroup, fields) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndGroupRuntimeFields, configBlocks) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, hasWorldPos) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, gain) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, streamState) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zSndStreamRequest, group) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeEntry, currentValue) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeEntry, handle) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zSndFadeEntry, stopOnComplete) == 0x0c);

namespace zSnd {
// Audio API option and active sound backend values: the Options audio-API
// setting, the RecoilFrame audio menu and every g_zSnd_ActiveBackend dispatch.
#define ZSND_AUDIO_API_DIRECTSOUND 0
#define ZSND_AUDIO_API_A3D 1
int __fastcall ReportMciError(unsigned int mciError, const char* sourceFile, int lineNumber);
int __fastcall ReportA3DError(int a3dError, const char* sourceFile, int sourceLine);
int __fastcall ReportDirectSoundError(int directSoundError, const char* sourceFile, int sourceLine);
int __fastcall SetAudioApiOption(int apiType);
void __fastcall SetSpeedOfSoundMps(float speedOfSoundMps);
int GetAudioApiOption();
void __fastcall SetCDAudioOption(int cdAudioOption);
int GetCDAudioOption();
int __fastcall SetActiveBackendPreInit(int backend);
int __cdecl GetActiveBackend();
void __fastcall SetUseArchiveBanksFlag(int useArchiveBanks);
extern "C" zSndSample* __fastcall FindSampleByName(const char* sampleName);
int __stdcall GainScaleToDirectSoundAttenuation(float gainScale);
int __fastcall ApplyMuteStateToActiveVoices(int enableMute);
int __cdecl IsMuted();
float __stdcall MulGlobalVolumeScaleAndGetPrev(float scale);
float __stdcall SetGlobalVolumeScale(float scale);
void __fastcall SetFlag10PlaybackEnabled(int enabled);
int __cdecl HasMmxMixerSupport();
LPDIRECTSOUND __fastcall AcquireCachedDirectSound(const GUID* deviceGuid);
void __cdecl ReleaseCachedDirectSound();
HRESULT __fastcall CachedDirectSoundGetCaps(DSCAPS* caps);
} // namespace zSnd

/**
 * Purpose: CD audio state bits; zSndCd::Init tests ready as a bitfield
 * (retail 'shr eax,1; test al,1') and zSndPreInitializeRuntimeState clears both.
 */
struct zSndCdFlagBits {
    unsigned int stereoAux : 1;
    unsigned int ready : 1;
};

/**
 * Purpose: CD-audio state record (retail 0x56b318..0x56b33f): track-list count,
 * play mode, flags, MCI/AUX device ids and volumes, and the cached disc length.
 * zSndPreInitializeRuntimeState clears its ready and stereo-aux flags with two
 * separate byte ANDs around one dword store, which VC5 emits for a member flag.
 */
struct zSndCdState {
    int trackListCount;
    int lastPlayMode;
    int unknown08;
    zSndCdFlagBits flags;
    unsigned short deviceId;
    int auxDeviceId;
    unsigned short auxVolumePrimary;
    unsigned short auxVolumeSecondary;
    int trackCountCached;
    int discLengthMinute;
    int discLengthSecond;
};

namespace zSndCd {
int __fastcall Init(zReader::Node* cdTracksNode);
int __cdecl Stop();
int __cdecl Shutdown();
int __cdecl GetTrackCount();
int __fastcall PlayTrackWithMode(int trackIndex, int playbackMode);
int __fastcall GetVolume(unsigned short* primaryVolumeOut, unsigned short* secondaryVolumeOut);
int __fastcall SetVolume(unsigned short primaryVolume, unsigned short secondaryVolume);
} // namespace zSndCd

extern "C" zSndSample* __fastcall
zSndSampleCreateQueuedStreamingSample(WAVEFORMATEX* audioFormat, void* audioBuffer, int bufferBytes);

extern "C" {
extern int g_zSnd_IsInitialized;
extern int g_zSnd_PreInitialized;
extern int g_zSnd_ActiveBackend;
extern unsigned int g_zSnd_WindowHandle;
extern int g_zSnd_UseArchiveBanksFlag;
extern int g_zSnd_SoundLodDefault;
extern void* g_zSnd_SoundLodValuePtr;
extern int g_zSnd_MuteOptionDefault;
extern void* g_zSnd_MuteOptionValuePtr;
extern int g_zSnd_MuteDepth;
extern float g_zSnd_VolumeScaleDefault;
extern void* g_zSnd_GlobalVolumeScalePtr;
extern zSndSample* g_zSndLastSample;
extern zSndSample* g_zSndLastVoice;
extern zSndPlayHandle* g_zSndLastVoiceHandle;
extern int g_zSndLastVoiceMarkerIndex;
extern int g_zSndLastVoiceStopMarkerIndex;
extern int g_zSnd_Flag10PlaybackEnabled;
extern DSCAPS g_zSnd_BackendAuxHandleOrConfig;
extern zReader::Node* g_zSnd_ConfigRootNode;
extern zArchiveList* g_zSnd_SearchPathList;
extern int g_zSnd_ListenerStateValid;
extern zSndListenerState g_zSnd_ListenerState;
extern zVec3 g_zSnd_ListenerVelocity;
extern zVec3 g_zSnd_PreviousListenerPos;
extern zArchiveList* g_zSndStream_PendingList;
extern zArchiveList* g_zSndStream_ActiveList;
extern zArchiveList* g_zSndStream_FreeList;
extern zSndStreamRequest* g_zSndStream_MatchedRequest;
extern int g_zSndStream_MatchedRequestCount;
extern CZNodePartial* g_zSndStream_RootNode;
extern zSndCdState g_zSndCdState;
extern float g_zSndSpeedOfSoundMps;
extern float g_zSndInvSpeedOfSoundMps;
}

#define g_zSndCdTrackListCount (g_zSndCdState.trackListCount)
#define g_zSndCdLastPlayMode (g_zSndCdState.lastPlayMode)
#define g_zSndCdFlags (g_zSndCdState.flags)
#define g_zSndCdDeviceId (g_zSndCdState.deviceId)
#define g_zSndCdAuxDeviceId (g_zSndCdState.auxDeviceId)
#define g_zSndCdAuxVolumePrimary (g_zSndCdState.auxVolumePrimary)
#define g_zSndCdAuxVolumeSecondary (g_zSndCdState.auxVolumeSecondary)
#define g_zSndCdTrackCountCached (g_zSndCdState.trackCountCached)
#define g_zSndCdDiscLengthMinute (g_zSndCdState.discLengthMinute)
#define g_zSndCdDiscLengthSecond (g_zSndCdState.discLengthSecond)

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
extern "C" char g_zEffectAnim_TokenRange[0x06];

extern "C" int __fastcall zSndBackendInitA3D();
extern "C" int __cdecl zSndBackendInitDirectSound();
extern "C" int __fastcall zSndPreInitializeRuntimeState(unsigned int hwnd);
extern "C" int __fastcall zSndUpdateListenerState(zSndListenerState* listenerState, zVec3* listenerVelocity);
extern "C" float __cdecl zSndGetSpeedOfSoundMps();

extern "C" int __fastcall zSndSystemInit(unsigned int hwnd, const char* zrdPath);
namespace zSndSystem {
int __cdecl Shutdown();
}
namespace zSndBackend {
int __cdecl Shutdown();
}
namespace zSndStreamMgr {
int __fastcall UpdateActiveRequestPredicate(void* payload, void* userData);
int __cdecl Shutdown();
}
extern "C" void __cdecl zSndSampleSetRegistryDestroyAll();
extern "C" int __cdecl zSndSampleSetRegistryGetCount();
extern "C" zSndSampleSet* __fastcall zSndSampleSetRegistryGetByIndex(int index);
extern "C" zSndSampleSet* __fastcall zSndSampleSetRegistryFindByName(const char* setName);
extern "C" int __fastcall zSndSampleSetDestroyByName(const char* setName);
extern "C" int __fastcall zSndSampleSetInitByName(const char* setName);
namespace zSndFadeLists {
void __cdecl StopAllAndShutdown();
} // namespace zSndFadeLists
namespace zSndFadeDispatchList {
void __fastcall PushBack(zSndFadeEntry* fadeEntry);
}
extern "C" void __stdcall zSndFadeActiveListTickAll(float deltaTime);
extern "C" void __fastcall zSndTick(int skipA3dCommit);
extern "C" int __fastcall zSndSystemInitNamedSetsSyntax(zReader::Node* configRootNode);
extern "C" int __fastcall zSndSystemInitLegacySetsSyntax(zReader::Node* configRootNode);
extern "C" int __fastcall zSndGroupLoadConfigBlock(
    zReader::Node* readerNode,
    zSndGroupRuntimeFields* groupFields,
    zSndGroupConfigBlock* outConfigBlock
);
extern "C" zSndGroup* __fastcall zSndGroupLoadFromConfigNode(zReader::Node* readerNode);
extern "C" int __fastcall zSndGroupQueuePendingLoadsFromConfigNode(zReader::Node* readerNode);
extern "C" int __fastcall zSndStreamRequestStopIfActive(zSndPlayHandle* request);
extern "C" int __fastcall zSndPlayHandleStopIfActive(zSndPlayHandle* playHandle);
extern "C" int __fastcall
zSndPlayHandleUpdate3DDispatch(zSndPlayHandle* playHandle, zVec3* worldPos, zVec3* velocity, int velocityScaleMode);
extern "C" zSndPlayHandle* __fastcall
zSndSamplePlayA3D(zSndSample* sample, float gainScale, zVec3* worldPos, zVec3* velocity);
extern "C" zSndPlayHandle* __fastcall zSndSamplePlayA3DSimple(zSndSample* sample, float gainScale);
extern "C" int __fastcall zSndPlayHandleTryEnableManaged(zSndPlayHandle* handle);
extern "C" int __fastcall zSndPlayHandleTryDisableManaged(zSndPlayHandle* handle);
extern "C" int __fastcall zSndStreamRequestMatchGroupPredicate(void* payload, void* group);
extern "C" float __stdcall zSndGainScaleIdentity(float value);
extern "C" zSndSample* __fastcall zSndPendingListFindByName(const char* sampleName);
extern "C" int __fastcall zSndPendingListMatchNamePredicate(void* payload, void* sampleName);
extern "C" int __cdecl zSndStreamMgrEnsureInit();
extern "C" void __cdecl zSndStreamMgrRecycleFinishedRequest();
#else
/* C view of the zSound entry points the zClass units call. */
#include "recoil/recoil_callconv.h"
#include "zclass.h"

typedef struct zSndListenerState zSndListenerState;

extern zVec3 g_zSnd_PreviousListenerPos;

zSndSample* __fastcall FindSampleByName(const char* sampleName);
int __fastcall zSndPlayHandleStopIfActive(zSndPlayHandle* playHandle);
int __fastcall
zSndPlayHandleUpdate3DDispatch(zSndPlayHandle* playHandle, zVec3* worldPos, zVec3* velocity, int velocityScaleMode);
zSndPlayHandle* __fastcall zSndSamplePlayA3D(zSndSample* sample, float gainScale, zVec3* worldPos, zVec3* velocity);
zSndPlayHandle* __fastcall zSndSamplePlayA3DSimple(zSndSample* sample, float gainScale);
int __fastcall zSndPlayHandleTryEnableManaged(zSndPlayHandle* handle);
int __fastcall zSndPlayHandleTryDisableManaged(zSndPlayHandle* handle);
int __fastcall zSndUpdateListenerState(zSndListenerState* listenerState, zVec3* listenerVelocity);
float __cdecl zSndGetSpeedOfSoundMps();
#endif
