#include "zsnd.h"

#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zSound/zsnd_a3d_provider.h"
#include "GameZRecoil/zReader/zreader.h"

#include "recoil/recoil_types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern "C" void *g_zSnd_BackendDevice;
extern "C" void *g_zSnd_BackendListenerHandle;

/*
 * zsnd_3d.cpp physical-contribution routing anchors. The data and bodies below
 * compile from their literal-backed translation unit without duplicate definitions.
 */

/*
 * zsnd_create.cpp physical-contribution routing anchors. The definitions now
 * compile only from their literal-backed translation unit.
 */



namespace {
const char kZSndPlaySourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_play.cpp";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsnd-directsoundattenunitygain
 * @recoil-artifact defines .rdata recoil:data:0x4d2eb0: g_zSnd_DirectSoundAttenUnityGain.
 * Purpose: Provides the DirectSound attenuation unity-gain constant.
 */
const float g_zSnd_DirectSoundAttenUnityGain = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsnd-directsoundattenmingain
 * @recoil-artifact defines .rdata recoil:data:0x4d2eb4: g_zSnd_DirectSoundAttenMinGain.
 * Purpose: Provides the DirectSound attenuation minimum-gain constant.
 */
const float g_zSnd_DirectSoundAttenMinGain = 0.0009765625f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsnd-directsoundattenroundbias
 * @recoil-artifact defines .rdata recoil:data:0x4d2eb8: g_zSnd_DirectSoundAttenRoundBias.
 * Purpose: Provides the DirectSound attenuation rounding-bias constant.
 */
const float g_zSnd_DirectSoundAttenRoundBias = 0.5f;
/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zsound-zsnd_play-data-f
 * @recoil-artifact defines .data recoil:data:0x4e2204: g_zSnd_DirectSoundAttenScale.
 * Purpose: DirectSound 3D attenuation distance scale used by
 * zSndPlayHandle::Update3D.
 */
const float g_zSnd_DirectSoundAttenScale = 1000.0f;

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
 * Owner: engine.zclass.camera_listener_bridge_previous_pos; storage remains
 * in zsnd_play.cpp, but current BN xrefs are from Camera.c listener updates.
 * Purpose: Stores the previous camera listener position used to derive
 * velocity before zSnd_UpdateListenerState.
 */
extern "C" zVec3 g_zSnd_PreviousListenerPos = {0};















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
#if defined(_MSC_VER)
#pragma runtime_checks("", restore)
#endif






// Modern MSVC /RTC traps the recovered unsupported-backend return of the
// uninitialized status stack slot; keep the retail shape for debug smokes.
#if defined(_MSC_VER)
#pragma runtime_checks("", off)
#endif
#if defined(_MSC_VER)
#pragma runtime_checks("", restore)
#endif













/**
 * Purpose: advance backend deferred work, active fades, and the last-voice
 * marker callback timeline.
 */
extern "C" void __fastcall zSnd_Tick(
    int skipA3dCommit
) {
    if (g_zSnd_ActiveBackend == 1 && skipA3dCommit == 0) {
        ((zA3dProviderDevice *)(g_zSnd_BackendDevice))->Flush();
        ((zA3dProviderDevice *)(g_zSnd_BackendDevice))->Clear();
    }

    zSndFadeActiveList_TickAll(g_FrameDeltaTimeSec);

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsndlastvoice
     * @recoil-artifact defines .data recoil:data:0x56b3d0: g_zSndLastVoice.
     * Data: g_zSndLastVoice.
     * Purpose: drive marker callback state for the currently tracked voice.
     */
    zSndSample *const sample = g_zSndLastVoice;
    if (sample == 0) {
        return;
    }

    float *const markerValues = sample->markerValues;
    if (markerValues == 0) {
        g_zSndLastVoiceMarkerIndex = 0;
        g_zSndLastVoice = 0;
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    if (g_Time_UnscaledAccumulatedTimeSec < markerValues[g_zSndLastVoiceMarkerIndex]) {
        return;
    }

    sample->playbackEventHandler(g_zSndLastVoiceMarkerIndex);

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsndlastvoicemarkerindex
     * @recoil-artifact defines .data recoil:data:0x56b3d8: g_zSndLastVoiceMarkerIndex.
     * Data: g_zSndLastVoiceMarkerIndex.
     * Purpose: cache the current marker index before stop/reset decisions.
     */
    const int markerIndex = g_zSndLastVoiceMarkerIndex;
    if ((unsigned int)(markerIndex) >= (unsigned int)(g_zSndLastVoice->markerCount)) {
        g_zSndLastVoiceMarkerIndex = 0;
        g_zSndLastVoice = 0;
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    if ((unsigned int)(markerIndex) >= (unsigned int)(g_zSndLastVoiceStopMarkerIndex)) {
        g_zSndLastVoiceHandle->StopIfActive();
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    g_zSndLastVoiceMarkerIndex = markerIndex + 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-acquireplayhandledispatch
 * @recoil-artifact defines .text recoil:function:0x49f6d0: zSndSample::AcquirePlayHandleDispatch.
 * Purpose: select the active backend-specific play-handle acquisition path.
 */
zSndPlayHandle * zSndSample::AcquirePlayHandleDispatch() {
    zSndPlayHandle *voice = 0;

    if (g_zSnd_ActiveBackend == 1) {
        voice = AcquireA3dVoice();
    }
    else if (g_zSnd_ActiveBackend == 0) {
        voice = AcquireVoice();
    }

    return voice;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-acquirea3dvoice
 * @recoil-artifact defines .text recoil:function:0x49f6f0: zSndSample::AcquireA3dVoice.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-acquirevoice
 * @recoil-artifact defines .text recoil:function:0x49f830: zSndSample::AcquireVoice.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-playa3dsimple
 * @recoil-artifact defines .text recoil:function:0x49f960: zSndSample::PlayA3DSimple.
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsnd-gainscaletodirectsoundattenuation
 * @recoil-artifact defines .text recoil:function:0x49f9a0: zSnd::GainScaleToDirectSoundAttenuation.
 * Purpose: convert linear gain into DirectSound attenuation units.
 */
int __stdcall zSnd::GainScaleToDirectSoundAttenuation(
    float gainScale
) {
    if (gainScale >= g_zSnd_DirectSoundAttenUnityGain) {
        return 0;
    }

    if (!(gainScale > g_zSnd_DirectSoundAttenMinGain)) {
        return -10000;
    }

    const float attenuation = (log(gainScale) / log(2.0)) * g_zSnd_DirectSoundAttenScale;
    return (int)(attenuation - g_zSnd_DirectSoundAttenRoundBias);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-playsimple
 * @recoil-artifact defines .text recoil:function:0x49fa00: zSndSample_PlaySimple.
 * Purpose: return the supplied gain scale through the x87 floating-point
 * return path unchanged.
 */
extern "C" float __stdcall zSndSample_PlaySimple(
    float value
) {
    return value;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-playonactivebackend
 * @recoil-artifact defines .text recoil:function:0x49fa10: zSndSample::PlayOnActiveBackend.
 * Purpose: dispatch sample playback to the active sound backend.
 */
zSndPlayHandle *__fastcall zSndSample::PlayOnActiveBackend(
    zVec3 *worldPos,
    float gainScale,
    zVec3 *velocity,
    int backendArg
) {
    zSndPlayHandle *result = 0;

    if (g_zSnd_ActiveBackend == 1) {
        result = PlayOnA3D(
            worldPos,
            gainScale,
            velocity,
            backendArg
        );
    }
    else if (g_zSnd_ActiveBackend == 0) {
        result = PlayOnDirectSound(
            zSnd::GainScaleToDirectSoundAttenuation(gainScale),
            worldPos,
            velocity,
            backendArg
        );
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-playona3d
 * @recoil-artifact defines .text recoil:function:0x49fa60: zSndSample::PlayOnA3D.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-playondirectsound
 * @recoil-artifact defines .text recoil:function:0x49fbb0: zSndSample::PlayOnDirectSound.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-playa3d
 * @recoil-artifact defines .text recoil:function:0x49fcf0: zSndSample::PlayA3D.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-playdirectsound
 * @recoil-artifact defines .text recoil:function:0x49fd50: zSndSample::PlayDirectSound.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandle-stopifactive
 * @recoil-artifact defines .text recoil:function:0x49fda0: zSndPlayHandle::StopIfActive.
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

    switch (g_zSnd_ActiveBackend) {
    case 1:
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

    case 0:
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

    return status;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsample-stopactivevoicesifplaying
 * @recoil-artifact defines .text recoil:function:0x49fec0: zSndSample::StopActiveVoicesIfPlaying.
 * Purpose: stop the sample's primary and duplicate backend voices if present.
 */
int zSndSample::StopActiveVoicesIfPlaying() {
    if (this == 0 || createGuard != 0) {
        return 0;
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

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandlesnapshot-createfromactivesamples
 * @recoil-artifact defines .text recoil:function:0x49fff0: zSndPlayHandleSnapshot::CreateFromActiveSamples.
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandlesnapshotpayload-capturefromplayhandle
 * @recoil-artifact defines .text recoil:function:0x4a0300: zSndPlayHandleSnapshotPayload::CaptureFromPlayHandle.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandle-playwithdelta-a3d
 * @recoil-artifact defines .text recoil:function:0x4a0380: zSndPlayHandle::PlayWithDelta_A3D.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandle-playwithdelta-directsound
 * @recoil-artifact defines .text recoil:function:0x4a0400: zSndPlayHandle::PlayWithDelta_DirectSound.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandle-playwithdelta-backenddispatch
 * @recoil-artifact defines .text recoil:function:0x4a0490: zSndPlayHandle::PlayWithDelta_BackendDispatch.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandlesnapshot-stopallifplaying
 * @recoil-artifact defines .text recoil:function:0x4a0500: zSndPlayHandleSnapshot::StopAllIfPlaying.
 * Purpose: Stops every captured still-playing backend handle in this original
 * translation unit so VC5 retains its register and stack-allocation shape.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandlesnapshot-restoreallwithglobalvolumedelta
 * @recoil-artifact defines .text recoil:function:0x4a0590: zSndPlayHandleSnapshot::RestoreAllWithGlobalVolumeDelta.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandlesnapshot-destroy
 * @recoil-artifact defines .text recoil:function:0x4a05f0: zSndPlayHandleSnapshot::Destroy.
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsnd-applymutestatetoactivevoices
 * @recoil-artifact defines .text recoil:function:0x4a0670: zSnd::ApplyMuteStateToActiveVoices.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsnd-ismuted
 * @recoil-artifact defines .text recoil:function:0x4a07a0: zSnd::IsMuted.
 * Purpose: report active mute state after sound preinitialization.
 */
int __cdecl zSnd::IsMuted() {
    if (g_zSnd_PreInitialized == 0) {
        return 0;
    }

    return g_zSnd_MuteDepth > 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndplayhandlesnapshot-newnode
 * @recoil-artifact defines .text recoil:function:0x4a07c0: zSndPlayHandleSnapshot::NewNode.
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

namespace zSnd {
/**
 * Purpose: Store the archive-bank selector global for sound-bank loading.
 */
void __fastcall SetUseArchiveBanksFlag(
    int useArchiveBanks
) {
    g_zSnd_UseArchiveBanksFlag = useArchiveBanks;
}
} // namespace zSnd

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsnd-samplesetregistry
 * @recoil-artifact defines .data recoil:data:0x56b290: g_zSnd_SampleSetRegistry.
 * Purpose: store the process-wide sample-set registry in its original VC5
 * std::vector<zSndSampleSet *> form.
 * Compiler-emitted 0x4a0800: VC5 static-initializer coordinator for this
 * global vector.
 * Compiler-emitted 0x4a0810: VC5 vector-constructor thunk for this global.
 * Compiler-emitted 0x4a0830: VC5 atexit-registration helper for this global.
 * Compiler-emitted 0x4a0840: VC5 vector-destructor thunk for this global.
 */
zSndSampleSetRegistry g_zSnd_SampleSetRegistry;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsndbankarchivenamelow
 * @recoil-artifact defines .data recoil:data:0x4e2238: g_zSndBankArchiveNameLow.
 * Owner data: audio_fmv archive-bank name buffer; adjacent archive-bank flag
 * at 0x4e2234 is separately owned.
 * Purpose: provide the writable low-quality sound archive bank name.
 */
char g_zSndBankArchiveNameLow[0x0c] = "soundsL.zbd";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsndbankarchivenamemedium
 * @recoil-artifact defines .data recoil:data:0x4e2244: g_zSndBankArchiveNameMedium.
 * Owner data: audio_fmv archive-bank name buffer.
 * Purpose: provide the writable medium-quality sound archive bank name.
 */
char g_zSndBankArchiveNameMedium[0x0c] = "soundsM.zbd";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.g-zsndbankarchivenamehigh
 * @recoil-artifact defines .data recoil:data:0x4e2250: g_zSndBankArchiveNameHigh.
 * Owner data: audio_fmv archive-bank name buffer.
 * Purpose: provide the writable high-quality sound archive bank name.
 */
char g_zSndBankArchiveNameHigh[0x0c] = "soundsH.zbd";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-initbyname
 * @recoil-artifact defines .text recoil:function:0x4a0860: zSndSampleSet_InitByName.
 * Purpose: find a registered sample set by name and dispatch its
 * initialization routine.
 */
extern "C" int __fastcall zSndSampleSet_InitByName(
    const char *setName
) {
    return zSndSampleSetRegistry_FindByName(setName)->Init();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-destroybyname
 * @recoil-artifact defines .text recoil:function:0x4a0870: zSndSampleSet_DestroyByName.
 * Purpose: find a registered sample set by name and dispatch its destroy routine.
 */
extern "C" int __fastcall zSndSampleSet_DestroyByName(
    const char *setName
) {
    return zSndSampleSetRegistry_FindByName(setName)->Destroy();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsamplesetregistry-destroyall
 * @recoil-artifact defines .text recoil:function:0x4a0880: zSndSampleSetRegistry_DestroyAll.
 * Purpose: destroy registered sample sets, clear their slots, and reset the active range.
 */
extern "C" void __cdecl zSndSampleSetRegistry_DestroyAll() {
    for (zSndSampleSetRegistry::iterator it = g_zSnd_SampleSetRegistry.begin();
        it != g_zSnd_SampleSetRegistry.end();
        ++it) {
        zSndSampleSet *set = *it;
        if (set != 0) {
            set->DestroyOwnedData();
            delete set;
            *it = 0;
        }
    }

    g_zSnd_SampleSetRegistry.clear();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsamplesetregistry-getbyindex
 * @recoil-artifact defines .text recoil:function:0x4a08d0: zSndSampleSetRegistry_GetByIndex.
 * Purpose: Returns the registry entry at a non-negative in-range index.
 */
extern "C" zSndSampleSet *__fastcall zSndSampleSetRegistry_GetByIndex(
    int index
) {
    if (index < 0) {
        return 0;
    }

    if ((unsigned int)(index) < (unsigned int)(g_zSnd_SampleSetRegistry.size())) {
        return g_zSnd_SampleSetRegistry[index];
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsamplesetregistry-getcount
 * @recoil-artifact defines .text recoil:function:0x4a0900: zSndSampleSetRegistry_GetCount.
 * Purpose: Returns the number of active sample-set registry entries.
 */
extern "C" int __cdecl zSndSampleSetRegistry_GetCount() {
    return (int)(g_zSnd_SampleSetRegistry.size());
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsamplesetregistry-findbyname
 * @recoil-artifact defines .text recoil:function:0x4a0920: zSndSampleSetRegistry_FindByName.
 * Purpose: return the registered sample set whose stored name exactly matches
 * the requested name.
 */
extern "C" zSndSampleSet *__fastcall zSndSampleSetRegistry_FindByName(
    const char *setName
) {
    for (zSndSampleSetRegistry::iterator it = g_zSnd_SampleSetRegistry.begin();
        it != g_zSnd_SampleSetRegistry.end();
        ++it) {
        if (strcmp(
            (*it)->setName,
            setName
        ) == 0) {
            return *it;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsnd-findsamplebyname
 * @recoil-artifact defines .text recoil:function:0x4a0990: zSnd::FindSampleByName.
 * Provisional source-placement hypothesis: GameZRecoil/zSound/zsnd.cpp.
 * Purpose: find a loaded sample by name across registered sample sets and pending stream groups.
 */
zSndSample *__fastcall zSnd::FindSampleByName(
    const char *sampleName
) {
    if (g_zSnd_IsInitialized == 0 || sampleName == 0) {
        return 0;
    }

    for (zSndSampleSetRegistry::iterator it = g_zSnd_SampleSetRegistry.begin();
        it != g_zSnd_SampleSetRegistry.end();
        ++it) {
        zSndSample *const sample = (*it)->FindSampleByName(sampleName);
        if (sample != 0) {
            return sample;
        }
    }

    return zSndPendingList_FindByName(sampleName);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-registryaddentry
 * @recoil-artifact defines .text recoil:function:0x4a09e0: zSndSampleSet::RegistryAddEntry.
 * Purpose: Allocates sample entries, stores the set name, and appends this set to the registry.
 */
zSndSampleSet * zSndSampleSet::RegistryAddEntry(
    const char *name,
    int count
) {
    samples = (zSndSample *)(calloc(
        (size_t)(count),
        sizeof(zSndSample)
    ));
    sampleCount = count;
    resourcesLoaded = 0;

    if (name != 0) {
        setName = _strdup(name);
    }

    g_zSnd_SampleSetRegistry.push_back(this);
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-destroyowneddata
 * @recoil-artifact defines .text recoil:function:0x4a0c00: zSndSampleSet::DestroyOwnedData.
 * Purpose: release owned sample storage and reset the sample count.
 */
void zSndSampleSet::DestroyOwnedData() {
    Destroy();
    if (samples != 0) {
        free(samples);
    }
    if (setName != 0) {
        free(setName);
    }
    sampleCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-init
 * @recoil-artifact defines .text recoil:function:0x4a0c40: zSndSampleSet::Init.
 * Purpose: initialize an unloaded sample set from archive banks first, then
 * from loose sample paths, and mark the set loaded.
 */
int zSndSampleSet::Init() {
    const char *const archiveNames[3] = {
        g_zSndBankArchiveNameHigh,
        g_zSndBankArchiveNameMedium,
        g_zSndBankArchiveNameLow
    };
    int archiveBankIndex = 0;
    int archiveInitialized = 0;
    zIndexArchive archive;
    archive.Reset();

    if (this == 0 || resourcesLoaded != 0) {
        archive.Destroy();
        return 0;
    }

    if (g_zSnd_UseArchiveBanksFlag != 0) {
        const int soundLod = *(int *)(g_zSnd_SoundLodValuePtr);
        if (soundLod == 1) {
            archiveBankIndex = 1;
        } else if (soundLod == 2) {
            archiveBankIndex = 2;
        }

        {
            for (int attempt = 0; attempt < 3 && archiveInitialized == 0; ++attempt) {
                const char *archivePath = archiveNames[archiveBankIndex];
                if (zReader::FileExists(archivePath) == 0) {
                    const char *resolvedPath =
                        zUtil_ZRDR_ResolvePathInSearchPathList(
                            g_zSnd_SearchPathList,
                            archivePath
                        );
                    if (resolvedPath != 0) {
                        archivePath = resolvedPath;
                    } else {
                        archivePath = 0;
                    }
                }

                if (archivePath != 0) {
                    archiveInitialized = archive.Init(archivePath);
                }

                if (archiveInitialized == 0) {
                    ++archiveBankIndex;
                    if (archiveBankIndex >= 3) {
                        archiveBankIndex = 0;
                    }
                }
            }
        }

        if (archiveInitialized != 0) {
            LoadSamplesFromIndexArchive(&archive);
            archive.CloseAndFreeRecords();
        }
    }

    {
        zSndSample *sample = samples;
        int index = 0;
        if (sampleCount > 0) {
            do {
                zSndSampleReplayFields *replayFields = &sample->replayFields;
                if ((replayFields->flags & 0x08) == 0) {
                    const char *const path = zUtil_ZRDR_ResolvePathInSearchPathList(
                        g_zSnd_SearchPathList,
                        replayFields->resourceName
                    );
                    if (path != 0) {
                        zSndWaveData *waveData = new zSndWaveData(
                            path,
                            1
                        );

                        if (waveData != 0 && waveData->parsedOk != 0) {
                            int initResult = sample->InitFromWaveData(waveData);
                            int flags = replayFields->flags;
                            initResult &= 1;
                            flags &= ~0x08;
                            initResult <<= 3;
                            flags |= initResult;
                            replayFields->flags = flags;
                        } else {
                            replayFields->flags &= ~0x08;
                        }

                        if (waveData != 0) {
                            delete waveData;
                        }
                    }
                }

                ++index;
                ++sample;
            } while (index < sampleCount);
        }
    }

    resourcesLoaded = 1;
    archive.Destroy();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-destroy
 * @recoil-artifact defines .text recoil:function:0x4a0e40: zSndSampleSet::Destroy.
 * Purpose: release loaded sample resources and clear the sample-set loaded flag.
 */
int zSndSampleSet::Destroy() {
    if (this == 0 || resourcesLoaded == 0) {
        return 0;
    }

    for (int i = 0; i < sampleCount; ++i) {
        samples[i].DestroyOwnedData();
    }
    resourcesLoaded = 0;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-getsampleat
 * @recoil-artifact defines .text recoil:function:0x4a0e90: zSndSampleSet::GetSampleAt.
 * Purpose: Returns the indexed sample pointer when the signed upper-bound check passes.
 */
zSndSample * zSndSampleSet::GetSampleAt(
    int index
) {
    if (this != 0 && index < sampleCount) {
        return &samples[index];
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-findsamplebyname
 * @recoil-artifact defines .text recoil:function:0x4a0ec0: zSndSampleSet::FindSampleByName.
 * Provisional source-placement hypothesis: GameZRecoil/zSound/zsnd.cpp.
 * Purpose: find a loaded sample in this sample set by source sample id for the active backend.
 */
zSndSample * zSndSampleSet::FindSampleByName(
    const char *sampleName
) {
    if (this == 0 || (g_zSnd_ActiveBackend != 0 && g_zSnd_ActiveBackend != 1)) {
        return 0;
    }

    {
        for (int index = 0; index < sampleCount; ++index) {
            zSndSample *const sample = &samples[index];
            if (strcmp(sampleName, sample->replayFields.sampleId) == 0 &&
                sample->primaryVoice.backendBuffer != 0) {
                return sample;
            }
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsndsampleset-loadsamplesfromindexarchive
 * @recoil-artifact defines .text recoil:function:0x4a0fb0: zSndSampleSet::LoadSamplesFromIndexArchive.
 * Purpose: load still-unloaded samples from the supplied index archive and
 * mirror each load result into the sample loaded flag.
 */
int zSndSampleSet::LoadSamplesFromIndexArchive(
    zIndexArchive *archive
) {
    zSndSample *sample = samples;
    int index = 0;
    if (sampleCount > 0) {
        do {
            zSndSampleReplayFields *replayFields = &sample->replayFields;
            if ((replayFields->flags & 0x08) == 0) {
                zSndWaveData *waveData = new zSndWaveData(
                    replayFields->resourceName,
                    0
                );

                waveData->LoadAndParseFromIndexArchiveIfNeeded(archive);

                if (waveData->parsedOk != 0) {
                    int initResult = sample->InitFromWaveData(waveData);
                    int flags = replayFields->flags;
                    initResult &= 1;
                    flags &= ~0x08;
                    initResult <<= 3;
                    flags |= initResult;
                    replayFields->flags = flags;
                } else {
                    replayFields->flags &= ~0x08;
                }

                if (waveData != 0) {
                    delete waveData;
                }
            }

            ++index;
            ++sample;
        } while (index < sampleCount);
    }

    return 1;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsnd-setglobalvolumescale
 * @recoil-artifact defines .text recoil:function:0x4a1090: zSnd::SetGlobalVolumeScale.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsnd-mulglobalvolumescaleandgetprev
 * @recoil-artifact defines .text recoil:function:0x4a10b0: zSnd::MulGlobalVolumeScaleAndGetPrev.
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-play.zsnd-setflag10playbackenabled
 * @recoil-artifact defines .text recoil:function:0x4a10d0: zSnd::SetFlag10PlaybackEnabled.
 * Purpose: set the zSound flag-gated playback enable value.
 */
void __fastcall zSnd::SetFlag10PlaybackEnabled(
    int enabled
) {
    g_zSnd_Flag10PlaybackEnabled = enabled;
}
