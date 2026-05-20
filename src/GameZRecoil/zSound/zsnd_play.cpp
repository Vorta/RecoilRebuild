#include "zSound.h"

#include "GameZRecoil/Time/Time.h"

#include <math.h>
#include "recoil/recoil_types.h"
#include <stdlib.h>
#include <string.h>

extern "C" void *g_zSnd_BackendDevice;
extern "C" void *g_zSnd_BackendListenerHandle;

namespace {
const char *kZSndPlaySourceFile = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_play.cpp";
const char *kZSnd3dSourceFile = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_3d.cpp";

typedef int(__stdcall *BackendGetStatusFn)(void *self, int *outStatus);
typedef int(__stdcall *BackendGetUint32Fn)(void *self, unsigned int *outValue);
typedef int(__stdcall *BackendSetIntFn)(void *self, int value);
typedef int(__stdcall *BackendSimpleFn)(void *self);
typedef int(__stdcall *BackendSetVecFn)(void *self, float x, float y, float z);
typedef int(__stdcall *BackendSetFloatFn)(void *self, float value);
typedef int(__stdcall *BackendPlayDirectSoundFn)(void *self, unsigned int reserved1,
                                                 unsigned int reserved2, unsigned int flags);
typedef int(__stdcall *BackendPlayA3dFn)(void *self, unsigned int flags);
typedef int(__stdcall *BackendDuplicateFn)(void *self, zSndBuffer *source,
                                           zSndBuffer **outDuplicate);
typedef int(__stdcall *BackendGetCurrentPositionFn)(void *self, unsigned int *playCursor,
                                                    unsigned int *writeCursor);

struct DirectSoundBufferVTable {
    void *slots00_0c[4];
    BackendGetCurrentPositionFn GetCurrentPosition;
    void *slots14_1c[3];
    BackendGetUint32Fn GetFrequency;
    BackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    BackendPlayDirectSoundFn Play;
    BackendSetIntFn SetCurrentPosition;
    void *slot38;
    BackendSetIntFn SetVolume;
    BackendSetIntFn SetPan;
    BackendSetIntFn SetFrequency;
    BackendSimpleFn Stop;
    void *slot4c;
    BackendSimpleFn Restore;
};

struct DirectSoundBuffer {
    DirectSoundBufferVTable *vtable;
};

struct UnknownBackendComVTable {
    void *QueryInterface;
    void *AddRef;
    BackendSimpleFn Release;
};

struct UnknownBackendComObject {
    UnknownBackendComVTable *vtable;
};

struct A3dReleaseVTable {
    void *QueryInterface;
    void *AddRef;
    BackendSimpleFn Release;
    void *slot0c;
    void *slot10;
    void *slot14;
    BackendSimpleFn FreeWaveData;
};

struct A3dReleaseObject {
    A3dReleaseVTable *vtable;
};

struct A3dSourceVTable {
    void *slots00_30[13];
    BackendPlayA3dFn Play;
    BackendSimpleFn Stop;
    BackendSimpleFn Rewind;
    void *slots40_44[2];
    BackendSetIntFn SetMode;
    BackendGetUint32Fn GetCurrentPosition;
    BackendSetVecFn SetPosition;
    void *slots54_7c[11];
    BackendSetVecFn SetVelocity;
    void *slots84_9c[7];
    BackendSetFloatFn SetGain;
    void *slotsa4_ac[3];
    BackendSetFloatFn SetDopplerScale;
    void *slotsb4_cc[7];
    BackendSetIntFn SetSpatializationEnabled;
    void *slotsd4_dc[3];
    BackendGetStatusFn GetStatus;
};

struct A3dSource {
    A3dSourceVTable *vtable;
};

struct A3dListenerVTable {
    void *slots00_08[3];
    BackendSetVecFn SetPosition;
    void *slots10_28[7];
    int(__stdcall *SetOrientation)(void *self, float forwardX, float forwardY,
                                            float forwardZ, float upX, float upY, float upZ);
    void *slots30_38[3];
    BackendSetVecFn SetVelocity;
};

struct A3dListener {
    A3dListenerVTable *vtable;
};

struct DirectSoundDeviceVTable {
    void *slots00_10[5];
    BackendDuplicateFn DuplicateSoundBuffer;
};

struct A3dDeviceVTable {
    void *slots00_30[13];
    BackendSimpleFn CommitDeferredSettings;
    void *slots38_44[4];
    BackendDuplicateFn DuplicateBufferA3D;
};

struct BackendDevice {
    void *vtable;
};

bool DirectSoundHandleIsAvailable(zSndPlayHandle *handle) {
    if (handle->isActive != 0) {
        return false;
    }

    DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(handle->backendBuffer);
    int status = 0;
    buffer->vtable->GetStatus(buffer, &status);
    return (status & 1) == 0;
}

bool A3dHandleIsAvailable(zSndPlayHandle *handle) {
    if (handle->isActive != 0) {
        return false;
    }

    A3dSource *const source = (A3dSource *)(handle->backendBuffer);
    int status = 0;
    source->vtable->GetStatus(source, &status);
    return (status & 1) == 0;
}

RECOIL_FORCEINLINE bool DirectSoundBufferIsPlaying(zSndBuffer *backendBuffer, int *status) {
    DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(backendBuffer);
    buffer->vtable->GetStatus(buffer, status);
    return (*status & 1) != 0;
}

RECOIL_FORCEINLINE bool DirectSoundBufferIsPlaying(zSndBuffer *backendBuffer) {
    int status;
    return DirectSoundBufferIsPlaying(backendBuffer, &status);
}

RECOIL_FORCEINLINE bool A3dSourceIsPlaying(zSndBuffer *backendBuffer, int *status) {
    A3dSource *const source = (A3dSource *)(backendBuffer);
    source->vtable->GetStatus(source, status);
    return (*status & 1) != 0;
}

RECOIL_FORCEINLINE bool A3dSourceIsPlaying(zSndBuffer *backendBuffer) {
    int status;
    return A3dSourceIsPlaying(backendBuffer, &status);
}

bool BackendHandleIsPlaying(zSndPlayHandle *handle) {
    if (g_zSnd_ActiveBackend == 0) {
        return DirectSoundBufferIsPlaying(handle->backendBuffer);
    }

    if (g_zSnd_ActiveBackend == 1) {
        return A3dSourceIsPlaying(handle->backendBuffer);
    }

    return false;
}

void ReleaseBackendBuffer(zSndBuffer *buffer) {
    if (buffer != 0) {
        UnknownBackendComObject * object = (UnknownBackendComObject *)(buffer);
        object->vtable->Release(object);
    }
}

int FreeA3dWaveData(zSndBuffer *buffer) {
    if (buffer == 0) {
        return 0;
    }

    A3dReleaseObject * object = (A3dReleaseObject *)(buffer);
    return object->vtable->FreeWaveData(object);
}

float ApproximateDirectSoundDistance(float distanceSquared) {
    int bits = 0;
    memcpy(&bits, &distanceSquared, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;

    float distance = 0.0f;
    memcpy(&distance, &bits, sizeof(distance));
    return distance;
}

float FloatFromBits(int bits) {
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

int FloatToBits(float value) {
    int bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float Dot(const zVec3 &lhs, const zVec3 &rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

void RefreshPlaybackMarkers(zSndSample *sample, zSndPlayHandle *handle) {
    if (sample->markerCount != 0 && sample->playbackEventHandler != 0) {
        {
        for (int index = 0; index < sample->markerCount; ++index) {
            sample->markerValues[index] = sample->markerTimes[index] +
                                          g_Time_UnscaledAccumulatedTimeSec -
                                          sample->markerBaseTime;
        }
        }

        g_zSndLastVoice = sample;
        g_zSndLastVoiceHandle = handle;
    }
}
} // namespace

RECOIL_FORCEINLINE void zSndPlayHandleSnapshot::AppendPayload(
    const zSndPlayHandleSnapshotPayload &payload) {
    zSndPlayHandleSnapshotItem *const listHead = this->listHead;
    zSndPlayHandleSnapshotItem *const node =
        NewNode(listHead, listHead->prev);
    listHead->prev = node;
    zSndPlayHandleSnapshotPayload *const nodePayload = &node->payload;
    node->prev->next = node;
    if (nodePayload != 0) {
        memcpy(nodePayload, &payload, sizeof(*nodePayload));
    }
    ++itemCount;
}

// Reimplements 0x4a3690: zSndSample::DestroyOwnedData
RECOIL_NOINLINE int RECOIL_THISCALL zSndSample::DestroyOwnedData() {
    if (this == 0 || createGuard != 0) {
        return 0;
    }

    free(markerTimes);
    markerTimes = 0;
    free(markerValues);
    markerValues = 0;
    free(markerAux);
    markerAux = 0;
    free(const_cast<char *>(highVariant.sampleName));
    highVariant.sampleName = 0;
    free(const_cast<char *>(medVariant.sampleName));
    medVariant.sampleName = 0;
    free(const_cast<char *>(lowVariant.sampleName));
    lowVariant.sampleName = 0;

    for (int i = 0; i < duplicateVoiceCount; ++i) {
        zSndPlayHandle *voice = duplicateVoices[i];
        if (voice != 0) {
            ReleaseBackendBuffer(voice->backendBuffer);
            voice->backendBuffer = 0;
            free(voice);
        }
    }

    free(duplicateVoices);
    duplicateVoices = 0;
    duplicateVoiceCount = 0;
    if (primaryVoice.backendBuffer != 0) {
        if (g_zSnd_ActiveBackend == 1 && FreeA3dWaveData(primaryVoice.backendBuffer) < 0) {
            return 0;
        }

        ReleaseBackendBuffer(primaryVoice.backendBuffer);
    }
    primaryVoice.backendBuffer = 0;
    replayFields.flags &= ~0x08;
    return 1;
}

// Reimplements 0x4a3910: zSndSample::Destroy
RECOIL_NOINLINE void RECOIL_THISCALL zSndSample::Destroy() {
    DestroyOwnedData();
    free(this);
}

extern "C" int g_zSnd_ListenerStateValid = 0;
extern "C" zSndListenerState g_zSnd_ListenerState = {0};
extern "C" zVec3 g_zSnd_ListenerVelocity = {0};
extern "C" zVec3 g_zSnd_PreviousListenerPos = {0};

// Reimplements 0x49f6d0: zSndSample::AcquirePlayHandleDispatch
RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL zSndSample::AcquirePlayHandleDispatch() {
    if (g_zSnd_ActiveBackend == 0) {
        return AcquireVoice();
    }

    if (g_zSnd_ActiveBackend == 1) {
        return AcquireA3dVoice();
    }

    return 0;
}

// Reimplements 0x49f830: zSndSample::AcquireVoice
RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL zSndSample::AcquireVoice() {
    if (this == 0 || createGuard != 0 || primaryVoice.backendBuffer == 0) {
        return 0;
    }

    if (DirectSoundHandleIsAvailable(&primaryVoice)) {
        return &primaryVoice;
    }

    zSndPlayHandle *result = 0;
    int index = 0;
    for (; index < duplicateVoiceCount; ++index) {
        zSndPlayHandle *const voice = duplicateVoices[index];
        if (voice != 0 && DirectSoundHandleIsAvailable(voice)) {
            result = voice;
            break;
        }
    }

    if (result == 0 && index < 5) {
        result = static_cast<zSndPlayHandle *>(malloc(sizeof(zSndPlayHandle)));
        memset(result, 0, sizeof(zSndPlayHandle));

        BackendDevice *const device = (BackendDevice *)(g_zSnd_BackendDevice);
        const int error =
            ((DirectSoundDeviceVTable *)(device->vtable))->DuplicateSoundBuffer(device, primaryVoice.backendBuffer, &result->backendBuffer);
        if (error != 0) {
            free(result);
            return 0;
        }

        zSndPlayHandle **const voices = static_cast<zSndPlayHandle **>(
            realloc(duplicateVoices, static_cast<size_t>(duplicateVoiceCount + 1) *
                                              sizeof(zSndPlayHandle *)));
        duplicateVoices = voices;
        voices[duplicateVoiceCount] = result;
        ++duplicateVoiceCount;
    }

    return result;
}

// Reimplements 0x49f6f0: zSndSample::AcquireA3dVoice
RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL zSndSample::AcquireA3dVoice() {
    if (this == 0 || createGuard != 0 || primaryVoice.backendBuffer == 0) {
        return 0;
    }

    if (A3dHandleIsAvailable(&primaryVoice)) {
        return &primaryVoice;
    }

    zSndPlayHandle *result = 0;
    int index = 0;
    for (; index < duplicateVoiceCount; ++index) {
        zSndPlayHandle *const voice = duplicateVoices[index];
        if (voice != 0 && A3dHandleIsAvailable(voice)) {
            result = voice;
            break;
        }
    }

    if (result == 0 && index < 5) {
        result = static_cast<zSndPlayHandle *>(malloc(sizeof(zSndPlayHandle)));
        memset(result, 0, sizeof(zSndPlayHandle));

        BackendDevice *const device = (BackendDevice *)(g_zSnd_BackendDevice);
        const int error =
            ((A3dDeviceVTable *)(device->vtable))->DuplicateBufferA3D(device, primaryVoice.backendBuffer, &result->backendBuffer);
        if (error < 0) {
            zSnd::ReportA3DError(error, kZSndPlaySourceFile, 0xb2);
            free(result);
            return 0;
        }

        zSndPlayHandle **const voices = static_cast<zSndPlayHandle **>(
            realloc(duplicateVoices, static_cast<size_t>(duplicateVoiceCount + 1) *
                                              sizeof(zSndPlayHandle *)));
        duplicateVoices = voices;
        voices[duplicateVoiceCount] = result;
        ++duplicateVoiceCount;
    }

    return result;
}

// Reimplements 0x49f9a0: zSnd::GainScaleToDirectSoundAttenuation
RECOIL_NOINLINE int RECOIL_STDCALL
zSnd::GainScaleToDirectSoundAttenuation(float gainScale) {
    if (gainScale >= 1.0f) {
        return 0;
    }

    if (!(gainScale > 0.0009765625f)) {
        return -10000;
    }

    const float kDirectSoundAttenuationScale = 602.059991f;
    const float attenuation = (log(gainScale) / log(2.0)) * kDirectSoundAttenuationScale;
    return static_cast<int>(attenuation - 0.5f);
}

// Reimplements 0x4a07a0: zSnd::IsMuted
RECOIL_NOINLINE int RECOIL_CDECL zSnd::IsMuted() {
    if (g_zSnd_IsInitialized == 0) {
        return 0;
    }

    return g_zSnd_MuteDepth > 0 ? 1 : 0;
}

// Reimplements 0x4a1090: zSnd::SetGlobalVolumeScale
RECOIL_NOINLINE float RECOIL_STDCALL zSnd::SetGlobalVolumeScale(float scale) {
    if (g_zSnd_GlobalVolumeScalePtr != 0) {
        *static_cast<float *>(g_zSnd_GlobalVolumeScalePtr) = scale;
    }

    return scale;
}

// Reimplements 0x4a10b0: zSnd::MulGlobalVolumeScaleAndGetPrev
// (D:\Proj\GameZRecoil\zSound\zSound.cpp)
RECOIL_NOINLINE float RECOIL_STDCALL zSnd::MulGlobalVolumeScaleAndGetPrev(float scale) {
    float *const globalVolumeScale = static_cast<float *>(g_zSnd_GlobalVolumeScalePtr);
    const float previousScale = *globalVolumeScale;
    *globalVolumeScale = previousScale * scale;
    return previousScale;
}

// Reimplements 0x4a10d0: zSnd::SetFlag10PlaybackEnabled
// (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL zSnd::SetFlag10PlaybackEnabled(int enabled) {
    g_zSnd_Flag10PlaybackEnabled = enabled;
}

// Reimplements 0x4a0670: zSnd::ApplyMuteStateToActiveVoices
RECOIL_NOINLINE int RECOIL_FASTCALL
zSnd::ApplyMuteStateToActiveVoices(int enableMute) {
    if (g_zSnd_IsInitialized == 0) {
        return 0;
    }

    const int previousMuted = zSnd::IsMuted();
    if (enableMute != 0) {
        ++g_zSnd_MuteDepth;
    } else {
        --g_zSnd_MuteDepth;
    }

    *static_cast<int *>(g_zSnd_MuteOptionValuePtr) = g_zSnd_MuteDepth > 0 ? 1 : 0;

    zSndPlayHandleSnapshot *const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    zSndPlayHandleSnapshotItem *const listHead = snapshot->listHead;
    zSndPlayHandleSnapshotItem *item = listHead->next->next;

    if (g_zSnd_ActiveBackend == 0) {
        while (item != listHead) {
            zSndPlayHandle *const playHandle = item->payload.playHandle;
            DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(playHandle->backendBuffer);
            const int volume = zSnd::IsMuted() != 0 ? -10000 : playHandle->gainScaled;
            buffer->vtable->SetVolume(buffer, volume);
            item = item->next;
        }
    } else if (g_zSnd_ActiveBackend == 1) {
        while (item != listHead) {
            zSndPlayHandle *const playHandle = item->payload.playHandle;
            A3dSource *const source = (A3dSource *)(playHandle->backendBuffer);
            if (zSnd::IsMuted() != 0) {
                source->vtable->SetGain(source, 0.0f);
            } else {
                source->vtable->SetGain(
                    source, zSndSample_PlaySimple(FloatFromBits(playHandle->gainScaled)));
            }

            item = item->next;
        }
    }

    return previousMuted;
}

// Reimplements 0x49fa00: zSndSample_PlaySimple
extern "C" RECOIL_NOINLINE float RECOIL_STDCALL zSndSample_PlaySimple(float value) {
    return value;
}

// Reimplements 0x4a07c0: zSndPlayHandleSnapshot::NewNode
// Callers seed ECX with the owning snapshot, while the helper only uses its
// two stack arguments and returns with ret 8.
RECOIL_NOINLINE zSndPlayHandleSnapshotItem *RECOIL_THISCALL zSndPlayHandleSnapshot::NewNode(
    zSndPlayHandleSnapshotItem *listHead, zSndPlayHandleSnapshotItem *prev) {
    zSndPlayHandleSnapshotItem *const result = static_cast<zSndPlayHandleSnapshotItem *>(
        ::operator new(sizeof(zSndPlayHandleSnapshotItem)));
    result->next = listHead != 0 ? listHead : result;
    result->prev = prev != 0 ? prev : result;
    return result;
}

// Reimplements 0x4a0300: zSndPlayHandleSnapshotPayload::CaptureFromPlayHandle
RECOIL_NOINLINE void RECOIL_FASTCALL
zSndPlayHandleSnapshotPayload::CaptureFromPlayHandle(zSndPlayHandle *playHandle) {
    if (playHandle->handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return;
    }

    memset(this, 0, sizeof(*this));
    this->playHandle = playHandle;
    sourceSample = playHandle->ownerSample;

    switch (g_zSnd_ActiveBackend) {
    case 0:
        volumeScaleRaw = static_cast<unsigned int>(sourceSample->primaryVoice.gainScaled);
        break;
    case 1:
        volumeScaleRaw = static_cast<unsigned int>(sourceSample->primaryVoice.gainScaled);
        break;
    }

    if (playHandle->hasWorldPos == 0) {
        return;
    }

    flags |= 1;
    zVec3 *const worldPosDest = &worldPos;
    const zVec3 *const worldPosSrc = &playHandle->worldPos;
    zVec3 *const velocityDest = &velocityOrDir;
    const zVec3 *const velocitySrc = &playHandle->velocityOrDir;
    *worldPosDest = *worldPosSrc;
    *velocityDest = *velocitySrc;
}

RECOIL_FORCEINLINE zSndPlayHandleSnapshot::zSndPlayHandleSnapshot(unsigned char tag) {
    backendTag = tag;
    zSndPlayHandleSnapshotItem *const head = static_cast<zSndPlayHandleSnapshotItem *>(
        ::operator new(sizeof(zSndPlayHandleSnapshotItem)));
    head->next = head;
    head->prev = head;
    listHead = head;
    itemCount = 0;
}

// Reimplements 0x49fff0: zSndPlayHandleSnapshot::CreateFromActiveSamples
RECOIL_NOINLINE zSndPlayHandleSnapshot *RECOIL_CDECL
zSndPlayHandleSnapshot::CreateFromActiveSamples() {
    zSndPlayHandleSnapshotPayload payload = {0};

    // BN 0x4a0037 reads this byte from the constructor stack slot before storing
    // zSndPlayHandleSnapshot::backendTag.
    unsigned char backendTag;
    int status;
    zSndPlayHandleSnapshot *const snapshot = new zSndPlayHandleSnapshot(backendTag);

    const int sampleSetCount = zSndSampleSetRegistry_GetCount();
    memcpy(&payload.volumeScaleRaw, g_zSnd_GlobalVolumeScalePtr, sizeof(payload.volumeScaleRaw));
    snapshot->AppendPayload(payload);

    for (int sampleSetIndex = 0;
         static_cast<unsigned int>(sampleSetIndex) < static_cast<unsigned int>(sampleSetCount);
         ++sampleSetIndex) {
        zSndSampleSet *const sampleSet = zSndSampleSetRegistry_GetByIndex(sampleSetIndex);
        for (int sampleIndex = 0; static_cast<unsigned int>(sampleIndex) <
                                           static_cast<unsigned int>(sampleSet->sampleCount);
             ++sampleIndex) {
            zSndSample *const sample = sampleSet->GetSampleAt(sampleIndex);
            switch (g_zSnd_ActiveBackend) {
            case 0: {
                if (sample->primaryVoice.backendBuffer != 0 &&
                    DirectSoundBufferIsPlaying(sample->primaryVoice.backendBuffer, &status)) {
                    payload.CaptureFromPlayHandle(&sample->primaryVoice);
                    snapshot->AppendPayload(payload);
                }

                for (int voiceIndex = 0; voiceIndex < sample->duplicateVoiceCount;
                     ++voiceIndex) {
                    zSndPlayHandle *const voice = sample->duplicateVoices[voiceIndex];
                    if (voice != 0 && DirectSoundBufferIsPlaying(voice->backendBuffer, &status)) {
                        payload.CaptureFromPlayHandle(voice);
                        snapshot->AppendPayload(payload);
                    }
                }
                break;
            }

            case 1: {
                if (sample->primaryVoice.backendBuffer != 0 &&
                    A3dSourceIsPlaying(sample->primaryVoice.backendBuffer, &status)) {
                    payload.CaptureFromPlayHandle(&sample->primaryVoice);
                    snapshot->AppendPayload(payload);
                }

                for (int voiceIndex = 0; voiceIndex < sample->duplicateVoiceCount;
                     ++voiceIndex) {
                    zSndPlayHandle *const voice = sample->duplicateVoices[voiceIndex];
                    if (voice != 0 && A3dSourceIsPlaying(voice->backendBuffer, &status)) {
                        payload.CaptureFromPlayHandle(voice);
                        zSndPlayHandleSnapshotItem *const listHead = snapshot->listHead;
                        zSndPlayHandleSnapshotItem *const prev = listHead->prev;
                        zSndPlayHandleSnapshotItem *const node =
                            new zSndPlayHandleSnapshotItem;
                        node->next = listHead != 0 ? listHead : node;
                        node->prev = prev != 0 ? prev : node;
                        listHead->prev = node;
                        node->prev->next = node;
                        memcpy(&node->payload, &payload, sizeof(node->payload));
                        ++snapshot->itemCount;
                    }
                }
                break;
            }
            }
        }
    }

    return snapshot;
}

// Reimplements 0x4a0500: zSndPlayHandleSnapshot::StopAllIfPlaying
RECOIL_NOINLINE int RECOIL_THISCALL zSndPlayHandleSnapshot::StopAllIfPlaying() {
    zSndPlayHandleSnapshotItem *item = listHead->next->next;
    while (item != listHead) {
        int status;
        const int activeBackend = g_zSnd_ActiveBackend;
        if (activeBackend == 0) {
            if (DirectSoundBufferIsPlaying(item->payload.playHandle->backendBuffer, &status)) {
                item->payload.playHandle->StopIfActive();
            }
        } else if (activeBackend == 1) {
            if (A3dSourceIsPlaying(item->payload.playHandle->backendBuffer, &status)) {
                item->payload.playHandle->StopIfActive();
            }
        }

        item = item->next;
    }

    return 1;
}

// Reimplements 0x4a0380: zSndPlayHandle::PlayWithDelta_A3D
// (D:\Proj\GameZRecoil\zSound\zsnd_play.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL zSndPlayHandle::PlayWithDelta_A3D(
    zSndSampleReplayFields *replayFields, zSndPlayHandle *playHandle,
    int restartBeforePlay, float gainDelta)
{
    if (gainDelta > 0.0f) {
        gainDelta += FloatFromBits(playHandle->gainScaled);
        playHandle->gainScaled = FloatToBits(gainDelta);

        A3dSource *const source = (A3dSource *)playHandle->backendBuffer;
        source->vtable->SetGain(source, zSndSample_PlaySimple(gainDelta));
    }

    if (restartBeforePlay != 0) {
        A3dSource *const source = (A3dSource *)playHandle->backendBuffer;
        source->vtable->Rewind(source);
    }

    A3dSource *const source = (A3dSource *)playHandle->backendBuffer;
    const int error = source->vtable->Play(source, replayFields->flags & 1);
    if (error != 0) {
        zSnd::ReportA3DError(error, kZSndPlaySourceFile, 0x58a);
    }
}

// Reimplements 0x4a0400: zSndPlayHandle::PlayWithDelta_DirectSound
// (D:\Proj\GameZRecoil\zSound\zsnd_play.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL zSndPlayHandle::PlayWithDelta_DirectSound(
    zSndSampleReplayFields *replayFields, zSndPlayHandle *playHandle,
    int restartBeforePlay, int gainDelta)
{
    if (gainDelta != 0) {
        const int volumeDb = playHandle->gainScaled + gainDelta;
        playHandle->gainScaled = volumeDb;

        DirectSoundBuffer *const buffer = (DirectSoundBuffer *)playHandle->backendBuffer;
        const int error = buffer->vtable->SetVolume(buffer, volumeDb);
        if (error != 0) {
            zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x5a5);
        }
    }

    if (restartBeforePlay != 0) {
        DirectSoundBuffer *const buffer = (DirectSoundBuffer *)playHandle->backendBuffer;
        const int error = buffer->vtable->SetCurrentPosition(buffer, 0);
        if (error != 0) {
            zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x5ad);
        }
    }

    DirectSoundBuffer *const buffer = (DirectSoundBuffer *)playHandle->backendBuffer;
    const int error = buffer->vtable->Play(buffer, 0, 0, replayFields->flags & 1);
    if (error != 0) {
        zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x5b4);
    }
}

// Reimplements 0x4a0490: zSndPlayHandle::PlayWithDelta_BackendDispatch
// (D:\Proj\GameZRecoil\zSound\zsnd_play.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL zSndPlayHandle::PlayWithDelta_BackendDispatch(
    zSndSample *sourceSample, zSndPlayHandle *playHandle, int restartBeforePlay,
    float gainDelta)
{
    zSndSampleReplayFields *const replayFields = &sourceSample->replayFields;
    if (g_zSnd_ActiveBackend == 0) {
        const int directSoundGainDelta =
            static_cast<int>(gainDelta * 10000.0f);
        if (playHandle->backendBuffer != 0) {
            PlayWithDelta_DirectSound(replayFields, playHandle, restartBeforePlay,
                                      directSoundGainDelta);
        }
        return;
    }

    if (g_zSnd_ActiveBackend == 1 && playHandle->backendBuffer != 0 &&
        FloatFromBits(playHandle->gainScaled) != 0.0f) {
        PlayWithDelta_A3D(replayFields, playHandle, restartBeforePlay, gainDelta);
    }
}

// Reimplements 0x4a0590: zSndPlayHandleSnapshot::RestoreAllWithGlobalVolumeDelta
// (D:\Proj\GameZRecoil\zSound\zsnd_play.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zSndPlayHandleSnapshot::RestoreAllWithGlobalVolumeDelta()
{
    zSndPlayHandleSnapshotItem *const head = listHead;
    zSndPlayHandleSnapshotItem *const volumeAnchor = head->next;
    zSndPlayHandleSnapshotItem *item = volumeAnchor->next;

    float anchorVolume = 0.0f;
    memcpy(&anchorVolume, &volumeAnchor->payload.volumeScaleRaw, sizeof(anchorVolume));

    const float currentVolume =
        g_zSnd_GlobalVolumeScalePtr != 0 ? *(float *)g_zSnd_GlobalVolumeScalePtr : 0.0f;
    const float gainDelta = currentVolume - anchorVolume;

    while (item != head) {
        zSndPlayHandle::PlayWithDelta_BackendDispatch(item->payload.sourceSample,
                                                      item->payload.playHandle, 0, gainDelta);
        item = item->next;
    }

    return 1;
}

// Reimplements 0x4a05f0: zSndPlayHandleSnapshot::Destroy
RECOIL_NOINLINE int RECOIL_THISCALL zSndPlayHandleSnapshot::Destroy() {
    if (this != 0) {
        zSndPlayHandleSnapshotItem *const head = listHead;
        zSndPlayHandleSnapshotItem *item = head->next;
        while (item != head) {
            zSndPlayHandleSnapshotItem *const node = item;
            item = item->next;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            ::operator delete(node);
            --itemCount;
        }

        ::operator delete(listHead);
        listHead = 0;
        itemCount = 0;
        ::operator delete(this);
    }

    return 1;
}

// Reimplements 0x49fda0: zSndPlayHandle::StopIfActive
RECOIL_NOINLINE int RECOIL_THISCALL zSndPlayHandle::StopIfActive() {
    if (g_zSnd_IsInitialized == 0 || g_zSnd_PreInitialized == 0 || this == 0) {
        return -1;
    }

    if (handleKind == ZSND_PLAYHANDLE_STREAM_REQUEST) {
        return zSndStreamRequest_StopIfActive(this);
    }

    if (ownerSample == g_zSndLastVoice) {
        g_zSndLastVoiceMarkerIndex = 0;
        g_zSndLastVoice = 0;
    }

    const int activeBackend = g_zSnd_ActiveBackend;
    if (activeBackend != 0) {
        if (activeBackend != 1) {
            return (int)(this);
        }

        A3dSource *const source = (A3dSource *)(backendBuffer);
        if (source == 0) {
            return -1;
        }

        const int error = source->vtable->Stop(source);
        if (error != 0) {
            return zSnd::ReportA3DError(error, kZSndPlaySourceFile, 0x38c);
        }

        return error;
    }

    DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(backendBuffer);
    if (buffer == 0) {
        return -1;
    }

    int status = 0;
    int error = buffer->vtable->GetStatus(buffer, &status);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x392);
    }

    if (status == 2) {
        error = buffer->vtable->Restore(buffer);
        if (error != 0) {
            return zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x396);
        }
    }

    error = buffer->vtable->Stop(buffer);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x39a);
    }

    return error;
}

// Reimplements 0x49fec0: zSndSample::StopActiveVoicesIfPlaying
RECOIL_NOINLINE int RECOIL_THISCALL zSndSample::StopActiveVoicesIfPlaying() {
    if (this == 0 || createGuard != 0) {
        return 0;
    }

    if (g_zSnd_ActiveBackend == 0) {
        DirectSoundBuffer *const primaryBuffer =
            (DirectSoundBuffer *)(primaryVoice.backendBuffer);
        if (primaryBuffer == 0) {
            return 0;
        }

        int error = primaryBuffer->vtable->Stop(primaryBuffer);
        if (error != 0) {
            return zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x3d6);
        }

        {
        for (int index = 0; index < duplicateVoiceCount; ++index) {
            zSndPlayHandle *const voice = duplicateVoices[index];
            if (voice != 0) {
                DirectSoundBuffer *const buffer =
                    (DirectSoundBuffer *)(voice->backendBuffer);
                error = buffer->vtable->Stop(buffer);
                if (error != 0) {
                    return zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x3de);
                }
            }
        }
        }

        return 1;
    }

    if (g_zSnd_ActiveBackend == 1) {
        A3dSource *const primarySource = (A3dSource *)(primaryVoice.backendBuffer);
        if (primarySource == 0) {
            return 0;
        }

        int error = primarySource->vtable->Stop(primarySource);
        if (error != 0) {
            return zSnd::ReportA3DError(error, kZSndPlaySourceFile, 0x3c0);
        }

        {
        for (int index = 0; index < duplicateVoiceCount; ++index) {
            zSndPlayHandle *const voice = duplicateVoices[index];
            if (voice != 0) {
                A3dSource *const source = (A3dSource *)(voice->backendBuffer);
                error = source->vtable->Stop(source);
                if (error != 0) {
                    return zSnd::ReportDirectSoundError(error, kZSndPlaySourceFile, 0x3cb);
                }
            }
        }
        }

        return 1;
    }

    return 1;
}

// Reimplements 0x4a3620: zSndSample::GetPlayCursorBytes
// (D:\Proj\GameZRecoil\zSound\zSound.cpp)
RECOIL_NOINLINE unsigned int RECOIL_THISCALL zSndSample::GetPlayCursorBytes() {
    if (createGuard != 0) {
        return 0;
    }

    unsigned int playCursorBytes = 0;
    if (g_zSnd_ActiveBackend == 0) {
        unsigned int writeCursorBytes = 0;
        DirectSoundBuffer *const buffer =
            (DirectSoundBuffer *)(primaryVoice.backendBuffer);
        const int result =
            buffer->vtable->GetCurrentPosition(buffer, &playCursorBytes, &writeCursorBytes);
        return result == 0 ? playCursorBytes : 0;
    }

    if (g_zSnd_ActiveBackend == 1) {
        A3dSource *const source = (A3dSource *)(primaryVoice.backendBuffer);
        source->vtable->GetCurrentPosition(source, &playCursorBytes);
        return playCursorBytes;
    }

    return 0;
}

// Reimplements 0x4a2950: zSnd_UpdateListenerState
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSnd_UpdateListenerState(zSndListenerState *listenerState, zVec3 *listenerVelocity) {
    if (g_zSnd_ActiveBackend == 0) {
        if (listenerState != 0) {
            memcpy(&g_zSnd_ListenerState, listenerState, sizeof(g_zSnd_ListenerState));
        }

        if (listenerVelocity != 0) {
            g_zSnd_ListenerVelocity = *listenerVelocity;
        }

        g_zSnd_ListenerStateValid = 1;
        return 1;
    }

    if (g_zSnd_ActiveBackend == 1) {
        A3dListener *const listener = (A3dListener *)(g_zSnd_BackendListenerHandle);
        if (listener == 0) {
            return -1;
        }

        if (listenerState != 0) {
            listener->vtable->SetPosition(listener, listenerState->position.x,
                                          listenerState->position.y, listenerState->position.z);
            listener->vtable->SetOrientation(listener, -listenerState->forward.x,
                                             -listenerState->forward.y, -listenerState->forward.z,
                                             listenerState->up.x, listenerState->up.y,
                                             listenerState->up.z);
        }

        if (listenerVelocity != 0) {
            listener->vtable->SetVelocity(listener, listenerVelocity->x, listenerVelocity->y,
                                          listenerVelocity->z);
        }
    }

    g_zSnd_ListenerStateValid = 1;
    return 1;
}

// Reimplements 0x4a2e70: zSnd_GetSpeedOfSoundMps
// (D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp)
extern "C" RECOIL_NOINLINE float RECOIL_CDECL zSnd_GetSpeedOfSoundMps() {
    return g_zSndSpeedOfSoundMps;
}

// Reimplements 0x4a2a30: zSndPlayHandle::Update3DDispatch
RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPlayHandle::Update3DDispatch(zVec3 *worldPos, zVec3 *velocity, int velocityScaleMode) {
    if (g_zSnd_ActiveBackend == 0) {
        return Update3D(worldPos, velocity, velocityScaleMode);
    }

    if (g_zSnd_ActiveBackend == 1) {
        return Update3D_A3D(worldPos, velocity, velocityScaleMode);
    }

    return 0;
}

// Reimplements 0x4a2b40: zSndPlayHandle::Update3D
RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPlayHandle::Update3D(zVec3 *worldPos, zVec3 *velocity, int velocityScaleMode) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(backendBuffer);
    if (buffer == 0) {
        return -1;
    }

    int pan = 0;
    int gain = -10000;
    if (g_zSnd_ListenerStateValid == 0 || (hasWorldPos == 0 && worldPos == 0)) {
        if (zSnd::IsMuted() == 0) {
            gain = gainScaled;
        }
    } else {
        zSndSample *const sample = ownerSample;
        if (sample->createGuard != 0) {
            return -1;
        }

        if (worldPos != 0) {
            hasWorldPos = 1;
            this->worldPos = *worldPos;
        }

        if (velocity != 0) {
            velocityOrDir = *velocity;
        }

        if (zSnd::IsMuted() == 0) {
            gain = gainScaled;
        }

        const zVec3 relativePos = {this->worldPos.x - g_zSnd_ListenerState.position.x,
                                this->worldPos.y - g_zSnd_ListenerState.position.y,
                                this->worldPos.z - g_zSnd_ListenerState.position.z};
        const float distanceSquared = Dot(relativePos, relativePos);

        float distance = sample->rangeMin;
        float inverseDistance = 1.0f / sample->rangeMin;
        if (distanceSquared != 0.0f) {
            distance = ApproximateDirectSoundDistance(distanceSquared);
            inverseDistance = 1.0f / distance;
            pan = static_cast<int>(Dot(relativePos, g_zSnd_ListenerState.right) *
                                            inverseDistance * 1600.0f);
        }

        if (distance > sample->rangeMax) {
            buffer->vtable->SetVolume(buffer, -10000);
            return 0;
        }

        if (distance >= sample->rangeMin) {
            gain += static_cast<int>(((distance / sample->rangeMin) - 1.0f) * -600.0f);
            if (gain < -10000) {
                gain = -10000;
            }
        }

        if (velocityScaleMode != 0) {
            const zVec3 relativeVelocity = {velocityOrDir.x - g_zSnd_ListenerVelocity.x,
                                         velocityOrDir.y - g_zSnd_ListenerVelocity.y,
                                         velocityOrDir.z - g_zSnd_ListenerVelocity.z};
            const float dopplerDot = Dot(relativeVelocity, relativePos);
            const float dopplerPitchScale =
                1.0f - dopplerDot * inverseDistance * g_zSndInvSpeedOfSoundMps;

            unsigned int baseFrequency = 0;
            buffer->vtable->GetFrequency(buffer, &baseFrequency);
            const __int64 baseFrequencyWide = baseFrequency;
            buffer->vtable->SetFrequency(
                buffer, static_cast<int>(static_cast<float>(baseFrequencyWide) *
                                                  dopplerPitchScale));
        }
    }

    int error = buffer->vtable->SetPan(buffer, pan);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSnd3dSourceFile, 0x160);
    }

    error = buffer->vtable->SetVolume(buffer, gain);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSnd3dSourceFile, 0x164);
    }

    return 1;
}

// Reimplements 0x4a2a70: zSndPlayHandle::Update3D_A3D
RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPlayHandle::Update3D_A3D(zVec3 *worldPos, zVec3 *velocity, int velocityScaleMode) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    A3dSource *const source = (A3dSource *)(backendBuffer);
    if (source == 0) {
        return -1;
    }

    if (worldPos != 0) {
        source->vtable->SetPosition(source, worldPos->x, worldPos->y, worldPos->z);
    }

    if (velocity != 0) {
        source->vtable->SetVelocity(source, velocity->x, velocity->y, velocity->z);
    }

    if (zSnd::IsMuted() != 0) {
        source->vtable->SetGain(source, 0.0f);
    } else {
        source->vtable->SetGain(source, zSndSample_PlaySimple(FloatFromBits(gainScaled)));
    }

    source->vtable->SetDopplerScale(source, velocityScaleMode != 0 ? 1.0f : 0.0f);
    return 1;
}

// Reimplements 0x49fa10: zSndSample::PlayOnActiveBackend
RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL zSndSample::PlayOnActiveBackend(
    zVec3 *worldPos, float gainScale, zVec3 *velocity, int backendArg) {
    if (g_zSnd_ActiveBackend == 0) {
        return PlayOnDirectSound(zSnd::GainScaleToDirectSoundAttenuation(gainScale), worldPos,
                                 velocity, backendArg);
    }

    if (g_zSnd_ActiveBackend == 1) {
        return PlayOnA3D(worldPos, gainScale, velocity, backendArg);
    }

    return 0;
}

// Reimplements 0x49fa60: zSndSample::PlayOnA3D
RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL zSndSample::PlayOnA3D(zVec3 *worldPos,
                                                                      float gainScale,
                                                                      zVec3 *velocity,
                                                                      int backendArg) {
    if (createGuard != 0) {
        return 0;
    }

    if ((replayFields.flags & 0x10) != 0 && g_zSnd_Flag10PlaybackEnabled == 0) {
        return 0;
    }

    zSndPlayHandle *result = AcquirePlayHandleDispatch();
    if (result == 0 || result->backendBuffer == 0) {
        result = &primaryVoice;
    }

    result->handleKind = ZSND_PLAYHANDLE_BACKEND;
    result->ownerSample = this;
    result->gainScaled = FloatToBits(gainScale);

    A3dSource *const source = (A3dSource *)(result->backendBuffer);
    if (worldPos != 0) {
        source->vtable->SetSpatializationEnabled(source, 0);
        if (result->Update3DDispatch(worldPos, velocity, 0) == 0 &&
            (replayFields.flags & 0x01) == 0) {
            return 0;
        }
    } else {
        source->vtable->SetSpatializationEnabled(source, 1);
        if (zSnd::IsMuted() != 0) {
            source->vtable->SetGain(source, 0.0f);
        } else {
            source->vtable->SetGain(source,
                                    zSndSample_PlaySimple(FloatFromBits(result->gainScaled)));
        }
    }

    source->vtable->SetMode(source, backendArg);
    RefreshPlaybackMarkers(this, result);

    const int playError = source->vtable->Play(source, replayFields.flags & 0x01);

    BackendDevice *const device = (BackendDevice *)(g_zSnd_BackendDevice);
    ((A3dDeviceVTable *)(device->vtable))->CommitDeferredSettings(device);

    if (playError != 0) {
        zSnd::ReportA3DError(playError, kZSndPlaySourceFile, 0x209);
    }

    return result;
}

// Reimplements 0x49fbb0: zSndSample::PlayOnDirectSound
RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL zSndSample::PlayOnDirectSound(
    int attenuation, zVec3 *worldPos, zVec3 *velocity, int backendArg) {
    if (createGuard != 0) {
        return 0;
    }

    if ((replayFields.flags & 0x10) != 0 && g_zSnd_Flag10PlaybackEnabled == 0) {
        return 0;
    }

    zSndPlayHandle *result = AcquirePlayHandleDispatch();
    if (result == 0 || result->backendBuffer == 0) {
        result = &primaryVoice;
    }

    DirectSoundBuffer *buffer = (DirectSoundBuffer *)(result->backendBuffer);
    result->handleKind = ZSND_PLAYHANDLE_BACKEND;
    result->ownerSample = this;

    int status = 0;
    buffer->vtable->GetStatus(buffer, &status);
    if ((status & 0x02) != 0) {
        buffer = (DirectSoundBuffer *)(result->backendBuffer);
        buffer->vtable->Restore(buffer);
    }

    result->gainScaled = attenuation;
    if (worldPos != 0) {
        if (result->Update3DDispatch(worldPos, velocity, 0) == 0 &&
            (replayFields.flags & 0x01) == 0) {
            return 0;
        }
    } else {
        buffer = (DirectSoundBuffer *)(result->backendBuffer);
        buffer->vtable->SetVolume(buffer, zSnd::IsMuted() != 0 ? -10000 : result->gainScaled);
    }

    buffer = (DirectSoundBuffer *)(result->backendBuffer);
    buffer->vtable->SetCurrentPosition(buffer, backendArg);
    RefreshPlaybackMarkers(this, result);

    const int playError = buffer->vtable->Play(buffer, 0, 0, replayFields.flags & 0x01);
    if (playError != 0) {
        zSnd::ReportDirectSoundError(playError, kZSndPlaySourceFile, 0x29f);
    }

    return result;
}

// Reimplements 0x49fcf0: zSndSample::PlayA3D
RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL zSndSample::PlayA3D(zVec3 *worldPos,
                                                                    float gainScale,
                                                                    zVec3 *velocity) {
    if (g_zSnd_IsInitialized == 0 || g_zSnd_PreInitialized == 0 || this == 0) {
        return 0;
    }

    if (createGuard == 1) {
        return ((zSndGroup *)(this))->QueueStreamRequestWithWorldPos(
            worldPos, gainScale, velocity);
    }

    if ((replayFields.flags & 0x08) == 0) {
        return 0;
    }

    markerBaseTime = 0.0f;
    const float globalGain = g_zSnd_GlobalVolumeScalePtr != 0
                                 ? *static_cast<float *>(g_zSnd_GlobalVolumeScalePtr)
                                 : 0.0f;
    return PlayOnActiveBackend(worldPos, replayFields.gain * globalGain * gainScale, velocity, 0);
}

// Reimplements 0x49fd50: zSndSample::PlayDirectSound
RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL zSndSample::PlayDirectSound(
    int variantIndex, float gainScale, int stopMarkerIndex) {
    if (createGuard != 0) {
        return 0;
    }

    int backendArg = 0;
    if (markerTimes != 0 &&
        static_cast<unsigned int>(variantIndex) < static_cast<unsigned int>(markerCount)) {
        markerBaseTime = markerTimes[variantIndex];
        g_zSndLastVoiceStopMarkerIndex = stopMarkerIndex;
        backendArg = markerAux[variantIndex * 2];
    }

    const float globalGain = g_zSnd_GlobalVolumeScalePtr != 0
                                 ? *static_cast<float *>(g_zSnd_GlobalVolumeScalePtr)
                                 : 0.0f;
    return PlayOnActiveBackend(0, replayFields.gain * gainScale * globalGain, 0,
                               backendArg);
}

// Reimplements 0x49f960: zSndSample::PlayA3DSimple
RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL zSndSample::PlayA3DSimple(float gainScale) {
    if (g_zSnd_IsInitialized == 0 || g_zSnd_PreInitialized == 0 || this == 0) {
        return 0;
    }

    if (createGuard == 1) {
        return ((zSndGroup *)(this))->QueueStreamRequestSimple(gainScale);
    }

    return PlayA3D(0, gainScale, 0);
}
