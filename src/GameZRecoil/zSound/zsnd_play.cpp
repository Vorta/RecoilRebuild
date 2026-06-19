#include "zSound.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zSound/zA3dProvider.h"

#include "recoil/recoil_types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern "C" void *g_zSnd_BackendDevice;
extern "C" void *g_zSnd_BackendListenerHandle;

namespace {
const char kZSndPlaySourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_play.cpp";
const char kZSnd3dSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_3d.cpp";

/**
 * Original static helper recovered from the zSound playback source cluster.
 * Evidence: no standalone retail helper function; availability tests appear
 * inside address-backed DirectSound play-handle acquisition callers.
 * Purpose: report whether a DirectSound play handle can be reused.
 */
bool DirectSoundHandleIsAvailable(
    zSndPlayHandle *handle
) {
    if (handle->isActive != 0) {
        return false;
    }

    LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(handle->backendBuffer);
    DWORD status = 0;
    buffer->GetStatus(&status);
    return (status & 1) == 0;
}

/**
 * Original static helper recovered from the zSound playback source cluster.
 * Evidence: no standalone retail helper function; availability tests appear
 * inside address-backed A3D play-handle acquisition callers.
 * Purpose: report whether an A3D play handle can be reused.
 */
bool A3dHandleIsAvailable(
    zSndPlayHandle *handle
) {
    if (handle->isActive != 0) {
        return false;
    }

    zA3dProviderSource *const source = (zA3dProviderSource *)(handle->backendBuffer);
    int status = 0;
    source->GetStatus((LPDWORD)&status);
    return (status & 1) == 0;
}

/**
 * Original inline helper recovered from zSound playback snapshot callers.
 * Evidence: no standalone retail helper function; 0x49fff0 queries the
 * DirectSound provider status and preserves the caller's status word.
 * Purpose: test whether a DirectSound backend buffer is currently playing.
 */
inline bool DirectSoundBufferIsPlaying(
    zSndBuffer *backendBuffer,
    int *status
) {
    LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(backendBuffer);
    buffer->GetStatus((LPDWORD)status);
    return (*status & 1) != 0;
}

/**
 * Original inline helper recovered from zSound playback callers.
 * Evidence: no standalone retail helper function; callers only need the
 * DirectSound playing predicate and discard the provider status word.
 * Purpose: test whether a DirectSound backend buffer is currently playing.
 */
inline bool DirectSoundBufferIsPlaying(
    zSndBuffer *backendBuffer
) {
    int status;
    return DirectSoundBufferIsPlaying(
        backendBuffer,
        &status
    );
}

/**
 * Original inline helper recovered from zSound playback snapshot callers.
 * Evidence: no standalone retail helper function; 0x49fff0 queries the A3D
 * provider status and preserves the caller's status word.
 * Purpose: test whether an A3D backend source is currently playing.
 */
inline bool A3dSourceIsPlaying(
    zSndBuffer *backendBuffer,
    int *status
) {
    zA3dProviderSource *const source = (zA3dProviderSource *)(backendBuffer);
    source->GetStatus((LPDWORD)status);
    return (*status & 1) != 0;
}

/**
 * Original inline helper recovered from zSound playback callers.
 * Evidence: no standalone retail helper function; callers only need the A3D
 * playing predicate and discard the provider status word.
 * Purpose: test whether an A3D backend source is currently playing.
 */
inline bool A3dSourceIsPlaying(
    zSndBuffer *backendBuffer
) {
    int status;
    return A3dSourceIsPlaying(
        backendBuffer,
        &status
    );
}

/**
 * Original static helper recovered from address-backed zSound playback callers.
 * Evidence: no standalone retail helper function; callers dispatch the playing
 * query through the active backend tag.
 * Purpose: report whether a backend play handle is currently playing.
 */
bool BackendHandleIsPlaying(
    zSndPlayHandle *handle
) {
    if (g_zSnd_ActiveBackend == 0) {
        return DirectSoundBufferIsPlaying(handle->backendBuffer);
    }

    if (g_zSnd_ActiveBackend == 1) {
        return A3dSourceIsPlaying(handle->backendBuffer);
    }

    return false;
}

/**
 * Original static helper recovered from address-backed zSound teardown callers.
 * Evidence: no standalone retail helper function; 0x4a3690 releases backend
 * COM/provider buffers through the shared IUnknown contract.
 * Purpose: release a non-null backend buffer reference.
 */
void ReleaseBackendBuffer(
    zSndBuffer *buffer
) {
    if (buffer != 0) {
        ((IUnknown *)buffer)->Release();
    }
}

/**
 * Original static helper recovered from address-backed zSound teardown callers.
 * Evidence: no standalone retail helper function; 0x4a3690 frees A3D wave data
 * only when a backend source is present.
 * Purpose: free provider-owned A3D wave data for a backend source.
 */
int FreeA3dWaveData(
    zSndBuffer *buffer
) {
    if (buffer == 0) {
        return 0;
    }

    zA3dProviderSource *object = (zA3dProviderSource *)(buffer);
    return object->FreeWaveData();
}

/**
 * Original static helper recovered from address-backed zSound 3D callers.
 * Evidence: no standalone retail helper function; 0x4a2b40 uses the
 * integer-adjusted square-root approximation before pan/gain math.
 * Purpose: approximate DirectSound listener distance from squared distance.
 */
float ApproximateDirectSoundDistance(
    float distanceSquared
) {
    int bits = 0;
    memcpy(
        &bits,
        &distanceSquared,
        sizeof(bits)
    );
    bits = (bits >> 1) + 0x1fc00000;

    float distance = 0.0f;
    memcpy(
        &distance,
        &bits,
        sizeof(distance)
    );
    return distance;
}

/**
 * Original static helper recovered from address-backed zSound playback callers.
 * Evidence: retail callers use integer-preserving float bit reinterpretation
 * for provider gain replay, with no standalone retail helper function.
 * Purpose: reinterpret stored IEEE-754 bits as a float gain value.
 */
float FloatFromBits(
    int bits
) {
    float value = 0.0f;
    memcpy(
        &value,
        &bits,
        sizeof(value)
    );
    return value;
}

/**
 * Original static helper recovered from address-backed zSound playback callers.
 * Evidence: retail callers store provider gain values as integer bits for
 * later replay, with no standalone retail helper function.
 * Purpose: reinterpret a float gain value as its raw IEEE-754 bits.
 */
int FloatToBits(
    float value
) {
    int bits = 0;
    memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    return bits;
}

/**
 * Original static helper observed in zSnd 3D playback callers
 * (D:\Proj\GameZRecoil\zSound\zsnd_play.cpp).
 * Purpose: compute the zVec3 dot product used by distance, panning, and
 * Doppler calculations.
 */
float Dot(
    const zVec3 &lhs,
    const zVec3 &rhs
) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Original static helper recovered from address-backed callers in zsnd_play.cpp.
 * Evidence: retail callers inline the marker-time refresh and last-voice global
 * stores in zSndSample::PlayOnA3D and zSndSample::PlayOnDirectSound.
 * Purpose: refresh playback marker deadlines and remember the active voice.
 */
void RefreshPlaybackMarkers(
    zSndSample *sample,
    zSndPlayHandle *handle
) {
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

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in 0x49fff0 zSndPlayHandleSnapshot::CreateFromActiveSamples as
 * the repeated snapshot list append pattern that allocates or links a node
 * and copies a zSndPlayHandleSnapshotPayload.
 * Purpose: append one captured payload to the snapshot's intrusive list.
 */
inline void zSndPlayHandleSnapshot::AppendPayload(
    const zSndPlayHandleSnapshotPayload &payload
) {
    zSndPlayHandleSnapshotItem *const listHead = this->listHead;
    zSndPlayHandleSnapshotItem *const node = NewNode(
        listHead,
        listHead->prev
    );
    listHead->prev = node;
    zSndPlayHandleSnapshotPayload *const nodePayload = &node->payload;
    node->prev->next = node;
    if (nodePayload != 0) {
        memcpy(
            nodePayload,
            &payload,
            sizeof(*nodePayload)
        );
    }
    ++itemCount;
}

/**
 * Reimplements 0x4a3690: zSndSample::DestroyOwnedData.
 * Purpose: release runtime-owned sample buffers, voices, and loaded-state flags.
 */
int zSndSample::DestroyOwnedData() {
    if (this == 0 || createGuard != 0) {
        return 0;
    }

    free(markerTimes);
    markerTimes = 0;
    free(markerValues);
    markerValues = 0;
    free(markerAux);
    markerAux = 0;
    free((char *)(highVariant.sampleName));
    highVariant.sampleName = 0;
    free((char *)(medVariant.sampleName));
    medVariant.sampleName = 0;
    free((char *)(lowVariant.sampleName));
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

/**
 * Reimplements 0x4a3910: zSndSample::Destroy.
 * Purpose: release owned sample data and free the sample record itself.
 */
void zSndSample::Destroy() {
    DestroyOwnedData();
    free(this);
}

extern "C" int g_zSnd_ListenerStateValid = 0;
extern "C" zSndListenerState g_zSnd_ListenerState = {0};
extern "C" zVec3 g_zSnd_ListenerVelocity = {0};
extern "C" zVec3 g_zSnd_PreviousListenerPos = {0};

/**
 * Reimplements 0x49f6d0: zSndSample::AcquirePlayHandleDispatch.
 * Purpose: select the active backend-specific play-handle acquisition path.
 */
zSndPlayHandle * zSndSample::AcquirePlayHandleDispatch() {
    if (g_zSnd_ActiveBackend == 0) {
        return AcquireVoice();
    }

    if (g_zSnd_ActiveBackend == 1) {
        return AcquireA3dVoice();
    }

    return 0;
}

/**
 * Reimplements 0x49f830: zSndSample::AcquireVoice.
 * Purpose: select or duplicate a DirectSound play handle for playback.
 */
zSndPlayHandle * zSndSample::AcquireVoice() {
    zSndPlayHandle *voice = 0;
    if (this == 0) {
        return voice;
    }

    if (createGuard != 0) {
        return voice;
    }

    if (primaryVoice.backendBuffer == 0) {
        return voice;
    }

    int status;
    unsigned char playingMask = 1;
    if (primaryVoice.isActive == 0) {
        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer);
        buffer->GetStatus((LPDWORD)&status);
        if (((unsigned char)status & playingMask) == 0) {
            return &primaryVoice;
        }
    }

    int index = 0;
    for (; index < duplicateVoiceCount; ++index) {
        voice = duplicateVoices[index];
        if (voice != 0 && voice->isActive == 0) {
            LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(voice->backendBuffer);
            buffer->GetStatus((LPDWORD)&status);
            if (((unsigned char)status & playingMask) == 0) {
                break;
            }
        }
        voice = 0;
    }

    if (voice == 0 && index < 5) {
        voice = (zSndPlayHandle *)(malloc(sizeof(zSndPlayHandle)));
        memset(
            voice,
            0,
            sizeof(zSndPlayHandle)
        );

        LPDIRECTSOUND const device = (LPDIRECTSOUND)(g_zSnd_BackendDevice);
        const int error = device->DuplicateSoundBuffer(
            (LPDIRECTSOUNDBUFFER)primaryVoice.backendBuffer,
            (LPDIRECTSOUNDBUFFER *)&voice->backendBuffer
        );
        if (error != 0) {
            free(voice);
            return 0;
        }

        zSndPlayHandle **const voices = (zSndPlayHandle **)(realloc(
            duplicateVoices,
            (size_t)(duplicateVoiceCount + 1) * sizeof(zSndPlayHandle *)
        ));
        duplicateVoices = voices;
        voices[duplicateVoiceCount] = voice;
        ++duplicateVoiceCount;
    }

    return voice;
}

/**
 * Reimplements 0x49f6f0: zSndSample::AcquireA3dVoice.
 * Purpose: select or duplicate an A3D provider play handle for playback.
 */
zSndPlayHandle * zSndSample::AcquireA3dVoice() {
    zSndPlayHandle *voice = 0;
    if (this == 0) {
        return voice;
    }

    if (createGuard != 0) {
        return voice;
    }

    if (primaryVoice.backendBuffer == 0) {
        return voice;
    }

    int status;
    unsigned char playingMask = 1;
    if (primaryVoice.isActive == 0) {
        zA3dProviderSource *const source = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        source->GetStatus((LPDWORD)&status);
        if (((unsigned char)status & playingMask) == 0) {
            return &primaryVoice;
        }
    }

    /*
     * The retail function inlines the availability test here instead of
     * calling A3dHandleIsAvailable, preserving the provider call shape visible
     * at BN 0x49f6f0.
     */
    int index = 0;
    for (; index < duplicateVoiceCount; ++index) {
        voice = duplicateVoices[index];
        if (voice != 0 && voice->isActive == 0) {
            zA3dProviderSource *const source = (zA3dProviderSource *)(voice->backendBuffer);
            source->GetStatus((LPDWORD)&status);
            if (((unsigned char)status & playingMask) == 0) {
                break;
            }
        }
        voice = 0;
    }

    if (voice == 0 && index < 5) {
        voice = (zSndPlayHandle *)(malloc(sizeof(zSndPlayHandle)));
        memset(
            voice,
            0,
            sizeof(zSndPlayHandle)
        );

        zA3dProviderDevice *const device = (zA3dProviderDevice *)(g_zSnd_BackendDevice);
        zA3dProviderSource *duplicateSource = 0;
        const int error = device->DuplicateSource(
            (zA3dProviderSource *)primaryVoice.backendBuffer,
            &duplicateSource
        );
        voice->backendBuffer = (zSndBuffer *)duplicateSource;
        if (error < 0) {
            zSnd::ReportA3DError(
                error,
                kZSndPlaySourceFile,
                0xb2
            );
            free(voice);
            return 0;
        }

        zSndPlayHandle **const voices = (zSndPlayHandle **)(realloc(
            duplicateVoices,
            (size_t)(duplicateVoiceCount + 1) * sizeof(zSndPlayHandle *)
        ));
        duplicateVoices = voices;
        voices[duplicateVoiceCount] = voice;
        ++duplicateVoiceCount;
    }

    return voice;
}

/**
 * Reimplements 0x49f9a0: zSnd::GainScaleToDirectSoundAttenuation.
 * Purpose: convert linear gain into DirectSound attenuation units.
 */
int __stdcall zSnd::GainScaleToDirectSoundAttenuation(
    float gainScale
) {
    if (gainScale >= 1.0f) {
        return 0;
    }

    if (!(gainScale > 0.0009765625f)) {
        return -10000;
    }

    const float kDirectSoundAttenuationScale = 602.059991f;
    const float attenuation = (log(gainScale) / log(2.0)) * kDirectSoundAttenuationScale;
    return (int)(attenuation - 0.5f);
}

/**
 * Reimplements 0x4a07a0: zSnd::IsMuted.
 * Purpose: report active mute state after sound preinitialization.
 */
int zSnd::IsMuted() {
    if (g_zSnd_PreInitialized == 0) {
        return 0;
    }

    return g_zSnd_MuteDepth > 0 ? 1 : 0;
}

/**
 * Reimplements 0x4a1090: zSnd::SetGlobalVolumeScale.
 * Purpose: store and return the global sound-volume scale.
 */
float __stdcall zSnd::SetGlobalVolumeScale(
    float scale
) {
    if (g_zSnd_GlobalVolumeScalePtr != 0) {
        *(float *)(g_zSnd_GlobalVolumeScalePtr) = scale;
    }

    return scale;
}

/**
 * Reimplements 0x4a10b0: zSnd::MulGlobalVolumeScaleAndGetPrev.
 * Purpose: multiply the global sound-volume scale and return its previous value.
 */
float __stdcall zSnd::MulGlobalVolumeScaleAndGetPrev(
    float scale
) {
    float *const globalVolumeScale = (float *)(g_zSnd_GlobalVolumeScalePtr);
    const float previousScale = *globalVolumeScale;
    *globalVolumeScale = previousScale * scale;
    return previousScale;
}

/**
 * Reimplements 0x4a10d0: zSnd::SetFlag10PlaybackEnabled.
 * Purpose: set the zSound flag-gated playback enable value.
 */
void __fastcall zSnd::SetFlag10PlaybackEnabled(
    int enabled
) {
    g_zSnd_Flag10PlaybackEnabled = enabled;
}

/**
 * Reimplements 0x4a0670: zSnd::ApplyMuteStateToActiveVoices.
 * Purpose: update nested mute state and rewrite active voice backend gains.
 */
int __fastcall zSnd::ApplyMuteStateToActiveVoices(
    int enableMute
) {
    if (g_zSnd_PreInitialized == 0) {
        return 0;
    }

    const int previousMuted = zSnd::IsMuted();
    if (enableMute != 0) {
        ++g_zSnd_MuteDepth;
    } else {
        --g_zSnd_MuteDepth;
    }

    *(int *)(g_zSnd_MuteOptionValuePtr) = g_zSnd_MuteDepth > 0 ? 1 : 0;

    zSndPlayHandleSnapshot *const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    zSndPlayHandleSnapshotItem *const listHead = snapshot->listHead;
    zSndPlayHandleSnapshotItem *item = listHead->next->next;

    if (g_zSnd_ActiveBackend == 0) {
        while (item != listHead) {
            zSndPlayHandle *const playHandle = item->payload.playHandle;
            LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(playHandle->backendBuffer);
            const int volume = zSnd::IsMuted() != 0 ? -10000 : playHandle->gainScaled;
            buffer->SetVolume(volume);
            item = item->next;
        }
    } else if (g_zSnd_ActiveBackend == 1) {
        while (item != listHead) {
            zSndPlayHandle *const playHandle = item->payload.playHandle;
            zA3dProviderSource *const source = (zA3dProviderSource *)(playHandle->backendBuffer);
            if (zSnd::IsMuted() != 0) {
                source->SetGain(0.0f);
            } else {
                source->SetGain(
                    zSndSample_PlaySimple(FloatFromBits(playHandle->gainScaled))
                );
            }

            item = item->next;
        }
    }

    return previousMuted;
}

/**
 * Reimplements 0x49fa00: zSndSample_PlaySimple.
 * Purpose: return the supplied gain scale through the x87 floating-point
 * return path unchanged.
 */
extern "C" float __stdcall zSndSample_PlaySimple(
    float value
) {
    return value;
}

/**
 * Reimplements 0x4a0500: zSndPlayHandleSnapshot::StopAllIfPlaying.
 * Purpose: Stops every captured backend play handle whose provider reports it
 * is still playing.
 */
int zSndPlayHandleSnapshot::StopAllIfPlaying() {
    int result = 1;
    int status;
    zSndPlayHandleSnapshot *const snapshot = this;
    zSndPlayHandleSnapshotItem *const listHead = snapshot->listHead;
    zSndPlayHandleSnapshotItem *snapshotItem = listHead->next->next;
    int hasItem = (unsigned char)(snapshotItem == listHead) == 0;
    // The byte mask keeps VC5's list-end compare as test dl,dl/test al,al.
    if ((hasItem & 0xff) != 0) {
        do {
            switch (g_zSnd_ActiveBackend) {
            case 0: {
                LPDIRECTSOUNDBUFFER const buffer =
                    (LPDIRECTSOUNDBUFFER)(snapshotItem->payload.playHandle->backendBuffer);
                buffer->GetStatus((LPDWORD)&status);
                if ((status & result) != 0) {
                    snapshotItem->payload.playHandle->StopIfActive();
                }
                break;
            }
            case 1: {
                zA3dProviderSource *const source =
                    (zA3dProviderSource *)(snapshotItem->payload.playHandle->backendBuffer);
                source->GetStatus((LPDWORD)&status);
                if ((status & result) != 0) {
                    snapshotItem->payload.playHandle->StopIfActive();
                }
                break;
            }
            }

            snapshotItem = snapshotItem->next;
            hasItem = (unsigned char)(snapshotItem == snapshot->listHead) == 0;
        } while ((hasItem & 0xff) != 0);
    }

    return result;
}

/**
 * Reimplements 0x4a07c0: zSndPlayHandleSnapshot::NewNode.
 * Purpose: Allocates a snapshot list node and initializes its next/previous links.
 *
 * Callers seed ECX with the owning snapshot, while the helper only uses its
 * two stack arguments and returns with ret 8.
 */
zSndPlayHandleSnapshotItem * zSndPlayHandleSnapshot::NewNode(
    zSndPlayHandleSnapshotItem *listHead,
    zSndPlayHandleSnapshotItem *prev
) {
    zSndPlayHandleSnapshotItem *const result =
        (zSndPlayHandleSnapshotItem *)(::operator new(sizeof(zSndPlayHandleSnapshotItem)));
    result->next = listHead != 0 ? listHead : result;
    result->prev = prev != 0 ? prev : result;
    return result;
}

/**
 * Reimplements 0x4a0300: zSndPlayHandleSnapshotPayload::CaptureFromPlayHandle.
 * Purpose: Captures backend play-handle state into a snapshot payload.
 */
void __fastcall zSndPlayHandleSnapshotPayload::CaptureFromPlayHandle(
    zSndPlayHandle *playHandle
) {
    if (playHandle->handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return;
    }

    memset(
        this,
        0,
        sizeof(*this)
    );
    this->playHandle = playHandle;
    sourceSample = playHandle->ownerSample;

    switch (g_zSnd_ActiveBackend) {
    case 0:
        volumeScaleRaw = (unsigned int)(sourceSample->primaryVoice.gainScaled);
        break;
    case 1:
        volumeScaleRaw = (unsigned int)(sourceSample->primaryVoice.gainScaled);
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

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in 0x49fff0 zSndPlayHandleSnapshot::CreateFromActiveSamples as
 * the inlined allocation/initialization pattern for the sentinel list node
 * immediately after constructing the snapshot object.
 * Purpose: initialize the snapshot backend tag, empty list head, and item count.
 */
inline zSndPlayHandleSnapshot::zSndPlayHandleSnapshot(
    unsigned char tag
) {
    backendTag = tag;
    zSndPlayHandleSnapshotItem *const head =
        (zSndPlayHandleSnapshotItem *)(::operator new(sizeof(zSndPlayHandleSnapshotItem)));
    head->next = head;
    head->prev = head;
    listHead = head;
    itemCount = 0;
}

// Modern MSVC /RTC traps the recovered uninitialized backendTag stack byte;
// keep the retail source shape and disable that debug-only check here.
#if defined(_MSC_VER)
#pragma runtime_checks("", off)
#endif
/**
 * Reimplements 0x49fff0: zSndPlayHandleSnapshot::CreateFromActiveSamples.
 * Purpose: Builds a snapshot of the global volume anchor and active sample voices.
 */
zSndPlayHandleSnapshot * zSndPlayHandleSnapshot::CreateFromActiveSamples() {
    zSndPlayHandleSnapshotPayload payload = {0};

    // BN 0x4a0037 reads this byte from the constructor stack slot before storing
    // zSndPlayHandleSnapshot::backendTag.
    unsigned char backendTag;
    int status;
    zSndPlayHandleSnapshot *const snapshot = new zSndPlayHandleSnapshot(backendTag);

    const int sampleSetCount = zSndSampleSetRegistry_GetCount();
    memcpy(
        &payload.volumeScaleRaw,
        g_zSnd_GlobalVolumeScalePtr,
        sizeof(payload.volumeScaleRaw)
    );
    snapshot->AppendPayload(payload);

    for (int sampleSetIndex = 0; (unsigned int)(sampleSetIndex) < (unsigned int)(sampleSetCount);
        ++sampleSetIndex) {
        zSndSampleSet *const sampleSet = zSndSampleSetRegistry_GetByIndex(sampleSetIndex);
        for (int sampleIndex = 0;
            (unsigned int)(sampleIndex) < (unsigned int)(sampleSet->sampleCount);
            ++sampleIndex) {
            zSndSample *const sample = sampleSet->GetSampleAt(sampleIndex);
            switch (g_zSnd_ActiveBackend) {
            case 1: {
                if (sample->primaryVoice.backendBuffer != 0 &&
                    A3dSourceIsPlaying(
                        sample->primaryVoice.backendBuffer,
                        &status
                    )) {
                    payload.CaptureFromPlayHandle(&sample->primaryVoice);
                    snapshot->AppendPayload(payload);
                }

                for (int voiceIndex = 0; voiceIndex < sample->duplicateVoiceCount; ++voiceIndex) {
                    zSndPlayHandle *const voice = sample->duplicateVoices[voiceIndex];
                    if (voice != 0 && A3dSourceIsPlaying(
                        voice->backendBuffer,
                        &status
                    )) {
                        payload.CaptureFromPlayHandle(voice);
                        zSndPlayHandleSnapshotItem *const listHead = snapshot->listHead;
                        zSndPlayHandleSnapshotItem *const prev = listHead->prev;
                        zSndPlayHandleSnapshotItem *const node = new zSndPlayHandleSnapshotItem;
                        node->next = listHead != 0 ? listHead : node;
                        node->prev = prev != 0 ? prev : node;
                        listHead->prev = node;
                        node->prev->next = node;
                        memcpy(
                            &node->payload,
                            &payload,
                            sizeof(node->payload)
                        );
                        ++snapshot->itemCount;
                    }
                }
                break;
            }

            case 0: {
                if (sample->primaryVoice.backendBuffer != 0 &&
                    DirectSoundBufferIsPlaying(
                        sample->primaryVoice.backendBuffer,
                        &status
                    )) {
                    payload.CaptureFromPlayHandle(&sample->primaryVoice);
                    snapshot->AppendPayload(payload);
                }

                for (int voiceIndex = 0; voiceIndex < sample->duplicateVoiceCount; ++voiceIndex) {
                    zSndPlayHandle *const voice = sample->duplicateVoices[voiceIndex];
                    if (voice != 0 && DirectSoundBufferIsPlaying(
                        voice->backendBuffer,
                        &status
                    )) {
                        payload.CaptureFromPlayHandle(voice);
                        snapshot->AppendPayload(payload);
                    }
                }
                break;
            }
            }
        }
    }

    return snapshot;
}
#if defined(_MSC_VER)
#pragma runtime_checks("", restore)
#endif

/**
 * Reimplements 0x4a0380: zSndPlayHandle::PlayWithDelta_A3D.
 * Purpose: replay an A3D-backed handle with the requested restart and gain delta.
 */
void __fastcall zSndPlayHandle::PlayWithDelta_A3D(
    zSndSampleReplayFields *replayFields,
    zSndPlayHandle *playHandle,
    int restartBeforePlay,
    float gainDelta
) {
    if (gainDelta > 0.0) {
        gainDelta += *(float *)&playHandle->gainScaled;
        *(float *)&playHandle->gainScaled = gainDelta;

        zA3dProviderSource *const gainSource = (zA3dProviderSource *)playHandle->backendBuffer;
        gainSource->SetGain(
            zSndSample_PlaySimple(gainDelta)
        );
    }

    if (restartBeforePlay != 0) {
        zA3dProviderSource *const source = (zA3dProviderSource *)playHandle->backendBuffer;
        source->Rewind();
    }

    zA3dProviderSource *const source = (zA3dProviderSource *)playHandle->backendBuffer;
    const int error = source->Play(
        (unsigned char)(replayFields->flags) & 1
    );
    if (error != 0) {
        zSnd::ReportA3DError(
            error,
            kZSndPlaySourceFile,
            0x58a
        );
    }
}

/**
 * Reimplements 0x4a0400: zSndPlayHandle::PlayWithDelta_DirectSound.
 * Purpose: replay a DirectSound-backed handle with the requested restart and gain delta.
 */
void __fastcall zSndPlayHandle::PlayWithDelta_DirectSound(
    zSndSampleReplayFields *replayFields,
    zSndPlayHandle *playHandle,
    int restartBeforePlay,
    int gainDelta
) {
    if (gainDelta != 0) {
        playHandle->gainScaled += gainDelta;

        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)playHandle->backendBuffer;
        const int error = buffer->SetVolume(playHandle->gainScaled);
        if (error != 0) {
            zSnd::ReportDirectSoundError(
                error,
                kZSndPlaySourceFile,
                0x5a5
            );
        }
    }

    if (restartBeforePlay != 0) {
        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)playHandle->backendBuffer;
        const int error = buffer->SetCurrentPosition(0);
        if (error != 0) {
            zSnd::ReportDirectSoundError(
                error,
                kZSndPlaySourceFile,
                0x5ad
            );
        }
    }

    LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)playHandle->backendBuffer;
    const int error = buffer->Play(
        0,
        0,
        (unsigned char)(replayFields->flags) & 1
    );
    if (error != 0) {
        zSnd::ReportDirectSoundError(
            error,
            kZSndPlaySourceFile,
            0x5b4
        );
    }
}

/**
 * Reimplements 0x4a0490: zSndPlayHandle::PlayWithDelta_BackendDispatch.
 * Purpose: route play-handle replay through the active sound backend.
 */
void __fastcall zSndPlayHandle::PlayWithDelta_BackendDispatch(
    zSndSample *sourceSample,
    zSndPlayHandle *playHandle,
    int restartBeforePlay,
    float gainDelta
) {
    zSndSampleReplayFields *const replayFields = &sourceSample->replayFields;
    switch (g_zSnd_ActiveBackend) {
    case 1:
        if (playHandle->backendBuffer != 0 && *(float *)&playHandle->gainScaled != 0.0f) {
            PlayWithDelta_A3D(
                replayFields,
                playHandle,
                restartBeforePlay,
                gainDelta
            );
        }
        break;

    case 0: {
        const int directSoundGainDelta = (int)(gainDelta * 10000.0f);
        if (playHandle->backendBuffer != 0) {
            PlayWithDelta_DirectSound(
                replayFields,
                playHandle,
                restartBeforePlay,
                directSoundGainDelta
            );
        }
        break;
    }
    }
}

/**
 * Reimplements 0x4a0590: zSndPlayHandleSnapshot::RestoreAllWithGlobalVolumeDelta.
 * Purpose: replay captured handles while applying the current global volume delta.
 */
int zSndPlayHandleSnapshot::RestoreAllWithGlobalVolumeDelta() {
    zSndPlayHandleSnapshot *const snapshot = this;
    zSndPlayHandleSnapshotItem *const volumeAnchor = snapshot->listHead->next;

    const float gainDelta =
        *(float *)(g_zSnd_GlobalVolumeScalePtr) - *(float *)&volumeAnchor->payload.volumeScaleRaw;

    zSndPlayHandleSnapshotItem *item = volumeAnchor->next;
    int hasItem = (unsigned char)(-(item == snapshot->listHead)) == 0;
    if (hasItem != 0) {
        do {
            zSndPlayHandle::PlayWithDelta_BackendDispatch(
                item->payload.sourceSample,
                item->payload.playHandle,
                0,
                gainDelta
            );
            item = item->next;
            hasItem = (unsigned char)(-(item == snapshot->listHead)) == 0;
        } while (hasItem != 0);
    }

    return 1;
}

/**
 * Reimplements 0x4a05f0: zSndPlayHandleSnapshot::Destroy.
 * Purpose: unlink and free every snapshot node, then delete the snapshot object.
 */
int zSndPlayHandleSnapshot::Destroy() {
    if (this != 0) {
        zSndPlayHandleSnapshotItem *const head = listHead;
        zSndPlayHandleSnapshotItem *item = head->next;
        int hasItem = (unsigned char)(-(item == head)) == 0;
        while (hasItem != 0) {
            zSndPlayHandleSnapshotItem *const node = item;
            item = item->next;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            ::operator delete(node);
            --itemCount;
            hasItem = (unsigned char)(-(item == head)) == 0;
        }

        ::operator delete(listHead);
        listHead = 0;
        itemCount = 0;
        ::operator delete(this);
    }

    return 1;
}

// Modern MSVC /RTC traps the recovered unsupported-backend return of the
// uninitialized status stack slot; keep the retail shape for debug smokes.
#if defined(_MSC_VER)
#pragma runtime_checks("", off)
#endif
/**
 * Reimplements 0x49fda0: zSndPlayHandle::StopIfActive.
 * Purpose: stop the active provider buffer/source for this play handle and
 * clear any matching last-voice marker state.
 */
int zSndPlayHandle::StopIfActive() {
    zSndPlayHandle *playHandle = this;
    int status;
    int error;
    zA3dProviderSource *source;
    LPDIRECTSOUNDBUFFER buffer;

    if (g_zSnd_IsInitialized == 0 || g_zSnd_PreInitialized == 0 || playHandle == 0) {
        return -1;
    }

    if (playHandle->handleKind == ZSND_PLAYHANDLE_STREAM_REQUEST) {
        return zSndStreamRequest_StopIfActive(playHandle);
    }

    if (playHandle->ownerSample == g_zSndLastVoice) {
        g_zSndLastVoiceMarkerIndex = 0;
        g_zSndLastVoice = 0;
    }

    int activeBackend = g_zSnd_ActiveBackend;
    if (activeBackend == 0) {
        buffer = (LPDIRECTSOUNDBUFFER)(playHandle->backendBuffer);
        if (buffer == 0) {
            return -1;
        }

        error = buffer->GetStatus((LPDWORD)&status);
        if (error != 0) {
            return zSnd::ReportDirectSoundError(
                error,
                kZSndPlaySourceFile,
                0x392
            );
        }

        if (status == 2) {
            buffer = (LPDIRECTSOUNDBUFFER)(playHandle->backendBuffer);
            error = buffer->Restore();
            if (error != 0) {
                return zSnd::ReportDirectSoundError(
                    error,
                    kZSndPlaySourceFile,
                    0x396
                );
            }
        }

        buffer = (LPDIRECTSOUNDBUFFER)(playHandle->backendBuffer);
        error = buffer->Stop();
        if (error != 0) {
            return zSnd::ReportDirectSoundError(
                error,
                kZSndPlaySourceFile,
                0x39a
            );
        }

        return error;
    }

    --activeBackend;
    if (activeBackend != 0) {
        return status;
    }

    {
        source = (zA3dProviderSource *)(playHandle->backendBuffer);
        if (source == 0) {
            return -1;
        }

        error = source->Stop();
        if (error != 0) {
            return zSnd::ReportA3DError(
                error,
                kZSndPlaySourceFile,
                0x38c
            );
        }

        return error;
    }
}
#if defined(_MSC_VER)
#pragma runtime_checks("", restore)
#endif

/**
 * Reimplements 0x49fec0: zSndSample::StopActiveVoicesIfPlaying.
 * Purpose: stop the sample's primary and duplicate backend voices if present.
 */
int zSndSample::StopActiveVoicesIfPlaying() {
    if (this == 0 || createGuard != 0) {
        return 0;
    }

    if (g_zSnd_ActiveBackend == 0) {
        LPDIRECTSOUNDBUFFER const primaryBuffer = (LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer);
        if (primaryBuffer == 0) {
            return 0;
        }

        int error = primaryBuffer->Stop();
        if (error != 0) {
            return zSnd::ReportDirectSoundError(
                error,
                kZSndPlaySourceFile,
                0x3d6
            );
        }

        {
            for (int index = 0; index < duplicateVoiceCount; ++index) {
                zSndPlayHandle *const voice = duplicateVoices[index];
                if (voice != 0) {
                    LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(voice->backendBuffer);
                    error = buffer->Stop();
                    if (error != 0) {
                        return zSnd::ReportDirectSoundError(
                            error,
                            kZSndPlaySourceFile,
                            0x3de
                        );
                    }
                }
            }
        }

        return 1;
    }

    if (g_zSnd_ActiveBackend == 1) {
        zA3dProviderSource *const primarySource =
            (zA3dProviderSource *)(primaryVoice.backendBuffer);
        if (primarySource == 0) {
            return 0;
        }

        int error = primarySource->Stop();
        if (error != 0) {
            return zSnd::ReportA3DError(
                error,
                kZSndPlaySourceFile,
                0x3c0
            );
        }

        {
            for (int index = 0; index < duplicateVoiceCount; ++index) {
                zSndPlayHandle *const voice = duplicateVoices[index];
                if (voice != 0) {
                    zA3dProviderSource *const source = (zA3dProviderSource *)(voice->backendBuffer);
                    error = source->Stop();
                    if (error != 0) {
                        return zSnd::ReportDirectSoundError(
                            error,
                            kZSndPlaySourceFile,
                            0x3cb
                        );
                    }
                }
            }
        }

        return 1;
    }

    return 1;
}

/**
 * Reimplements 0x4a3620: zSndSample::GetPlayCursorBytes.
 * Source: D:\Proj\GameZRecoil\zSound\zSound.cpp.
 * Purpose: return the active backend play cursor in bytes, or zero on failure.
 */
unsigned int zSndSample::GetPlayCursorBytes() {
    int result = 0;
    if (createGuard != 0) {
        return 0;
    }

    unsigned int playCursorBytes;
    switch (g_zSnd_ActiveBackend) {
    case 0: {
        unsigned int writeCursorBytes;
        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer);
        result = buffer->GetCurrentPosition(
            (LPDWORD)&playCursorBytes,
            (LPDWORD)&writeCursorBytes
        );
        break;
    }
    case 1: {
        zA3dProviderSource *const source = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        source->GetWavePosition((LPDWORD)&playCursorBytes);
        break;
    }
    }

    return result == 0 ? playCursorBytes : 0;
}

/**
 * Reimplements 0x4a2950: zSnd_UpdateListenerState.
 * Purpose: update cached listener state or forward it to the A3D listener.
 */
extern "C" int __fastcall zSnd_UpdateListenerState(
    zSndListenerState *listenerState,
    zVec3 *listenerVelocity
) {
    if (g_zSnd_ActiveBackend == 0) {
        if (listenerState != 0) {
            memcpy(
                &g_zSnd_ListenerState,
                listenerState,
                sizeof(g_zSnd_ListenerState)
            );
        }

        if (listenerVelocity != 0) {
            g_zSnd_ListenerVelocity = *listenerVelocity;
        }

        g_zSnd_ListenerStateValid = 1;
        return 1;
    }

    if (g_zSnd_ActiveBackend == 1) {
        zA3dProviderListener *const listener =
            (zA3dProviderListener *)(g_zSnd_BackendListenerHandle);
        if (listener == 0) {
            return -1;
        }

        if (listenerState != 0) {
            listener->SetPosition3f(
                listenerState->position.x,
                listenerState->position.y,
                listenerState->position.z
            );
            listener->SetOrientation6f(
                -listenerState->forward.x,
                -listenerState->forward.y,
                -listenerState->forward.z,
                listenerState->up.x,
                listenerState->up.y,
                listenerState->up.z
            );
        }

        if (listenerVelocity != 0) {
            listener->SetVelocity3f(
                listenerVelocity->x,
                listenerVelocity->y,
                listenerVelocity->z
            );
        }
    }

    g_zSnd_ListenerStateValid = 1;
    return 1;
}

/**
 * Reimplements 0x4a2e70: zSnd_GetSpeedOfSoundMps.
 * Purpose: return the current 3D-audio speed-of-sound setting.
 */
extern "C" float zSnd_GetSpeedOfSoundMps() {
    return g_zSndSpeedOfSoundMps;
}

/**
 * Reimplements 0x4a2a30: zSndPlayHandle::Update3DDispatch.
 * Purpose: route play-handle 3D updates to the active sound backend.
 */
int __fastcall zSndPlayHandle::Update3DDispatch(
    zVec3 *worldPos,
    zVec3 *velocity,
    int velocityScaleMode
) {
    if (g_zSnd_ActiveBackend == 0) {
        return Update3D(
            worldPos,
            velocity,
            velocityScaleMode
        );
    }

    if (g_zSnd_ActiveBackend == 1) {
        return Update3D_A3D(
            worldPos,
            velocity,
            velocityScaleMode
        );
    }

    return 0;
}

/**
 * Reimplements 0x4a2b40: zSndPlayHandle::Update3D.
 * Purpose: update DirectSound spatial pan, volume, and Doppler state.
 */
int __fastcall zSndPlayHandle::Update3D(
    zVec3 *worldPos,
    zVec3 *velocity,
    int velocityScaleMode
) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(backendBuffer);
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
        const float distanceSquared = Dot(
            relativePos,
            relativePos
        );

        float distance = sample->rangeMin;
        float inverseDistance = 1.0f / sample->rangeMin;
        if (distanceSquared != 0.0f) {
            distance = ApproximateDirectSoundDistance(distanceSquared);
            inverseDistance = 1.0f / distance;
            pan = (int)(Dot(
                relativePos,
                g_zSnd_ListenerState.right
            ) * inverseDistance * 1600.0f);
        }

        if (distance > sample->rangeMax) {
            buffer->SetVolume(-10000);
            return 0;
        }

        if (distance >= sample->rangeMin) {
            gain += (int)(((distance / sample->rangeMin) - 1.0f) * -600.0f);
            if (gain < -10000) {
                gain = -10000;
            }
        }

        if (velocityScaleMode != 0) {
            const zVec3 relativeVelocity = {velocityOrDir.x - g_zSnd_ListenerVelocity.x,
                velocityOrDir.y - g_zSnd_ListenerVelocity.y,
                velocityOrDir.z - g_zSnd_ListenerVelocity.z};
            const float dopplerDot = Dot(
                relativeVelocity,
                relativePos
            );
            const float dopplerPitchScale =
                1.0f - dopplerDot * inverseDistance * g_zSndInvSpeedOfSoundMps;

            unsigned int baseFrequency = 0;
            buffer->GetFrequency((LPDWORD)&baseFrequency);
            const __int64 baseFrequencyWide = baseFrequency;
            buffer->SetFrequency((int)((float)(baseFrequencyWide)*dopplerPitchScale));
        }
    }

    int error = buffer->SetPan(pan);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSnd3dSourceFile,
            0x160
        );
    }

    error = buffer->SetVolume(gain);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSnd3dSourceFile,
            0x164
        );
    }

    return 1;
}

/**
 * Reimplements 0x4a2a70: zSndPlayHandle::Update3D_A3D.
 * Purpose: update A3D provider position, velocity, gain, and Doppler state.
 */
int __fastcall zSndPlayHandle::Update3D_A3D(
    zVec3 *worldPos,
    zVec3 *velocity,
    int velocityScaleMode
) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    zA3dProviderSource *const source = (zA3dProviderSource *)(backendBuffer);
    if (source == 0) {
        return -1;
    }

    if (worldPos != 0) {
        source->SetPosition3f(
            worldPos->x,
            worldPos->y,
            worldPos->z
        );
    }

    if (velocity != 0) {
        source->SetVelocity3f(
            velocity->x,
            velocity->y,
            velocity->z
        );
    }

    if (zSnd::IsMuted() != 0) {
        source->SetGain(0.0f);
    } else {
        source->SetGain(
            zSndSample_PlaySimple(FloatFromBits(gainScaled))
        );
    }

    source->SetDopplerScale(
        velocityScaleMode != 0 ? 1.0f : 0.0f
    );
    return 1;
}

/**
 * Reimplements 0x49fa10: zSndSample::PlayOnActiveBackend.
 * Purpose: dispatch sample playback to the active sound backend.
 */
zSndPlayHandle *__fastcall zSndSample::PlayOnActiveBackend(
    zVec3 *worldPos,
    float gainScale,
    zVec3 *velocity,
    int backendArg
) {
    if (g_zSnd_ActiveBackend == 0) {
        return PlayOnDirectSound(
            zSnd::GainScaleToDirectSoundAttenuation(gainScale),
            worldPos,
            velocity,
            backendArg
        );
    }

    if (g_zSnd_ActiveBackend == 1) {
        return PlayOnA3D(
            worldPos,
            gainScale,
            velocity,
            backendArg
        );
    }

    return 0;
}

/**
 * Reimplements 0x49fa60: zSndSample::PlayOnA3D.
 * Purpose: start sample playback on the A3D backend.
 */
zSndPlayHandle *__fastcall zSndSample::PlayOnA3D(
    zVec3 *worldPos,
    float gainScale,
    zVec3 *velocity,
    int backendArg
) {
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

    zA3dProviderSource *const source = (zA3dProviderSource *)(result->backendBuffer);
    if (worldPos != 0) {
        source->SetTransformMode(
            0
        );
        if (result->Update3DDispatch(worldPos, velocity, 0) == 0 &&
            (replayFields.flags & 0x01) == 0) {
            return 0;
        }
    } else {
        source->SetTransformMode(
            1
        );
        if (zSnd::IsMuted() != 0) {
            source->SetGain(0.0f);
        } else {
            source->SetGain(
                zSndSample_PlaySimple(FloatFromBits(result->gainScaled))
            );
        }
    }

    source->SetWavePosition(
        backendArg
    );
    RefreshPlaybackMarkers(
        this,
        result
    );

    const int playError = source->Play(
        replayFields.flags & 0x01
    );

    zA3dProviderDevice *const device = (zA3dProviderDevice *)(g_zSnd_BackendDevice);
    device->Flush();

    if (playError != 0) {
        zSnd::ReportA3DError(
            playError,
            kZSndPlaySourceFile,
            0x209
        );
    }

    return result;
}

/**
 * Reimplements 0x49fbb0: zSndSample::PlayOnDirectSound.
 * Purpose: start sample playback on the DirectSound backend.
 */
zSndPlayHandle *__fastcall zSndSample::PlayOnDirectSound(
    int attenuation,
    zVec3 *worldPos,
    zVec3 *velocity,
    int backendArg
) {
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

    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)(result->backendBuffer);
    result->handleKind = ZSND_PLAYHANDLE_BACKEND;
    result->ownerSample = this;

    DWORD status = 0;
    buffer->GetStatus(&status);
    if ((status & 0x02) != 0) {
        buffer = (LPDIRECTSOUNDBUFFER)(result->backendBuffer);
        buffer->Restore();
    }

    result->gainScaled = attenuation;
    if (worldPos != 0) {
        if (result->Update3DDispatch(worldPos, velocity, 0) == 0 &&
            (replayFields.flags & 0x01) == 0) {
            return 0;
        }
    } else {
        buffer = (LPDIRECTSOUNDBUFFER)(result->backendBuffer);
        buffer->SetVolume(zSnd::IsMuted() != 0 ? -10000 : result->gainScaled);
    }

    buffer = (LPDIRECTSOUNDBUFFER)(result->backendBuffer);
    buffer->SetCurrentPosition(backendArg);
    RefreshPlaybackMarkers(
        this,
        result
    );

    const int playError = buffer->Play(
        0,
        0,
        replayFields.flags & 0x01
    );
    if (playError != 0) {
        zSnd::ReportDirectSoundError(
            playError,
            kZSndPlaySourceFile,
            0x29f
        );
    }

    return result;
}

/**
 * Reimplements 0x49fcf0: zSndSample::PlayA3D.
 * Purpose: play a 3D-capable sample through a queued group or active backend.
 */
zSndPlayHandle *__fastcall zSndSample::PlayA3D(
    zVec3 *worldPos,
    float gainScale,
    zVec3 *velocity
) {
    if (g_zSnd_IsInitialized == 0 || g_zSnd_PreInitialized == 0 || this == 0) {
        return 0;
    }

    if (createGuard == 1) {
        return ((zSndGroup *)(this))->QueueStreamRequestWithWorldPos(
            worldPos,
            gainScale,
            velocity
        );
    }

    if ((replayFields.flags & 0x08) == 0) {
        return 0;
    }

    markerBaseTime = 0.0f;
    const float globalGain =
        g_zSnd_GlobalVolumeScalePtr != 0 ? *(float *)(g_zSnd_GlobalVolumeScalePtr) : 0.0f;
    return PlayOnActiveBackend(
        worldPos,
        replayFields.gain * globalGain * gainScale,
        velocity,
        0
    );
}

/**
 * Reimplements 0x49fd50: zSndSample::PlayDirectSound.
 * Purpose: play a DirectSound sample variant with gain scaling and marker state.
 */
zSndPlayHandle *__fastcall zSndSample::PlayDirectSound(
    int variantIndex,
    float gainScale,
    int stopMarkerIndex
) {
    if (createGuard != 0) {
        return 0;
    }

    int backendArg = 0;
    if (markerTimes != 0 && (unsigned int)(variantIndex) < (unsigned int)(markerCount)) {
        markerBaseTime = markerTimes[variantIndex];
        g_zSndLastVoiceStopMarkerIndex = stopMarkerIndex;
        backendArg = markerAux[variantIndex * 2];
    }

    const float globalGain =
        g_zSnd_GlobalVolumeScalePtr != 0 ? *(float *)(g_zSnd_GlobalVolumeScalePtr) : 0.0f;
    return PlayOnActiveBackend(
        0,
        replayFields.gain * gainScale * globalGain,
        0,
        backendArg
    );
}

/**
 * Reimplements 0x49f960: zSndSample::PlayA3DSimple.
 * Purpose: play a non-positional A3D-capable sample or queue a stream group.
 */
zSndPlayHandle * zSndSample::PlayA3DSimple(
    float gainScale
) {
    if (g_zSnd_IsInitialized == 0 || g_zSnd_PreInitialized == 0 || this == 0) {
        return 0;
    }

    if (createGuard == 1) {
        return ((zSndGroup *)(this))->QueueStreamRequestSimple(gainScale);
    }

    return PlayA3D(
        0,
        gainScale,
        0
    );
}
