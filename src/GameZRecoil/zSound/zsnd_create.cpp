#include "GameZRecoil/zSound/zsnd.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd_a3d_provider.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void *g_zSnd_BackendDevice;

namespace {
/*
 * Source owner note: current BN source comments place the DirectSound sample
 * creation, A3D sample creation, backend lock/unlock helpers, and streaming
 * sample factory in the original zsnd_create.cpp cluster. The backend device
 * and active-backend selector are extern zSound runtime data owned outside this
 * file; this slice only consumes them, so the source/data owner gate remains
 * blocked until the larger zSound runtime-global owner is linked.
 */
const char kZSndCreateSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_create.cpp";
const char kCreateSoundBufferError[] = "Error creating sound buffer ( %s )";

const unsigned int kRiffMagic = 0x46464952;
const unsigned int kWaveMagic = 0x45564157;
const unsigned int kFmtChunkMagic = 0x20746d66;
const unsigned int kDataChunkMagic = 0x61746164;
const unsigned int kCueChunkMagic = 0x20657563;

/**
 * DirectSound provider ABI prefix consumed by the retail CreateSoundBuffer call
 * at 0x4a3180. The modern SDK's DSBUFFERDESC is larger, but the original code
 * passed only this DirectSound v1-compatible prefix with dwSize == 20.
 */
struct zSndDirectSoundLegacyBufferDesc {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX *lpwfxFormat;
};
RECOIL_STATIC_ASSERT(sizeof(zSndDirectSoundLegacyBufferDesc) == 20);
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-create.zsndsample-initfromwavedata
 * @recoil-artifact defines .text recoil:function:0x4a2ea0: zSndSample::InitFromWaveData.
 *
 * Evidence: BN assembly switches on the zSound active-backend selector and
 * dispatches backend 0 to the DirectSound initializer and backend 1 to the A3D
 * initializer, both in this source-file cluster.
 *
 * Purpose: dispatch parsed WAV initialization to the currently selected sound
 * backend.
 */
int __fastcall zSndSample::InitFromWaveData(
    zSndWaveData *waveData
) {
    int initResult = 0;
    switch (g_zSnd_ActiveBackend) {
        case 0:
            initResult = InitFromWaveData_DirectSound(waveData);
            break;
        case 1:
            initResult = InitFromWaveData_A3D(waveData);
            break;
    }

    return initResult;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-create.zsndsample-initfromwavedata-a3d
 * @recoil-artifact defines .text recoil:function:0x4a2ec0: zSndSample::InitFromWaveData_A3D.
 *
 * Evidence: BN source comment and functional target evidence place the A3D WAV
 * upload path in zsnd_create.cpp, with NewSource, SetWaveFormat,
 * AllocateWaveData, Lock/Unlock PCM copy, post-Rewind spatial setup from replay
 * flags, cue marker setup, and loading-flag clear.
 *
 * Purpose: create an A3D source from parsed WAV data, upload the PCM bytes,
 * configure spatial playback, initialize cue markers, and clear the loading flag.
 */
int __fastcall zSndSample::InitFromWaveData_A3D(
    zSndWaveData *waveData
) {
    zSndWaveData *const loadedWaveData = waveData;
    if (createGuard != 0) {
        return 0;
    }

    void *audioPtr1;
    void *audioPtr2;
    int audioBytes1;
    int audioBytes2;
    const unsigned int pcmByteCount = (unsigned int)(loadedWaveData->pcmByteCount);
    WAVEFORMATEX *const fmt = loadedWaveData->fmt;
    zA3dProviderDevice *const device = (zA3dProviderDevice *)(g_zSnd_BackendDevice);
    zA3dProviderSource *source = 0;
    int error = device->NewSource(
        0,
        &source
    );
    primaryVoice.backendBuffer = (zSndBuffer *)source;
    if (error != 0) {
        return zSnd::ReportA3DError(
            error,
            kZSndCreateSourceFile,
            0x4e
        );
    }

    error = ((zA3dProviderSource *)(primaryVoice.backendBuffer))->SetWaveFormat(
        fmt
    );
    if (error != 0) {
        return zSnd::ReportA3DError(
            error,
            kZSndCreateSourceFile,
            0x51
        );
    }

    error = ((zA3dProviderSource *)(primaryVoice.backendBuffer))->AllocateWaveData(
        pcmByteCount
    );
    if (error != 0) {
        return zSnd::ReportA3DError(
            error,
            kZSndCreateSourceFile,
            0x54
        );
    }

    zA3dProviderSource *buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
    error = buffer->Lock(
        0,
        loadedWaveData->pcmByteCount,
        &audioPtr1,
        (LPDWORD)&audioBytes1,
        &audioPtr2,
        (LPDWORD)&audioBytes2,
        0
    );
    if (error != 0) {
        return zSnd::ReportA3DError(
            error,
            kZSndCreateSourceFile,
            0x5a
        );
    }

    memcpy(
        audioPtr1,
        loadedWaveData->pcmData,
        audioBytes1
    );
    if (audioBytes2 != 0) {
        memcpy(
            audioPtr2,
            (unsigned char *)(loadedWaveData->pcmData) + audioBytes1,
            audioBytes2
        );
        audioBytes1 += audioBytes2;
    }

    buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
    error = buffer->Unlock(
        audioPtr1,
        audioBytes1,
        audioPtr2,
        audioBytes2
    );
    if (error != 0) {
        return zSnd::ReportA3DError(
            error,
            kZSndCreateSourceFile,
            0x66
        );
    }

    buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
    buffer->Rewind();

    unsigned int spatialFlags = (unsigned int)(replayFields.flags);
    spatialFlags >>= 2;
    unsigned char spatialMode = (unsigned char)(spatialFlags);
    if ((spatialMode & 1) != 0) {
        buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        buffer->SetMinMaxDistance(
            rangeMin,
            rangeMax,
            1
        );
        buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        buffer->SetDistanceModelScale(
            a3dDistanceScale
        );
    } else {
        buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        buffer->SetTransformMode(
            1
        );
    }

    sampleRate = (float)(loadedWaveData->fmt->nSamplesPerSec);
    markerCount = loadedWaveData->cuePointCount;
    if (markerCount != 0) {
        zSndCuePoint *const cuePoints = loadedWaveData->cuePoints;
        if (markerCount > 0) {
            markerTimes = (float *)(malloc((size_t)(markerCount) * sizeof(float) + sizeof(float)));
            markerValues = (float *)(malloc((size_t)(markerCount) * sizeof(float) + sizeof(float)));
            markerAux = (int *)(malloc((size_t)(markerCount) * 2 * sizeof(int) + 2 * sizeof(int)));
        } else {
            markerTimes = 0;
            markerValues = 0;
            markerAux = 0;
        }

        int index = 0;
        while (index < markerCount) {
            const zSndCuePoint &cue = cuePoints[index];
            markerAux[index * 2] =
                (fmt->wBitsPerSample >> 3) * (int)(cue.position) * fmt->nChannels;
            markerTimes[index] = (float)(cue.position) / (float)(fmt->nSamplesPerSec);
            ++index;
        }

        markerTimes[index] = (float)(pcmByteCount) / (float)(fmt->nAvgBytesPerSec);
        ++markerCount;
    }
    playbackEventHandler = 0;
    replayFields.flags &= ~0x80;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-create.zsndsample-initfromwavedata-directsound
 * @recoil-artifact defines .text recoil:function:0x4a3180: zSndSample::InitFromWaveData_DirectSound.
 *
 * Evidence: BN source comment and assembly show the DirectSound path in
 * zsnd_create.cpp with a 20-byte legacy buffer descriptor, provider calls
 * through the backend device, PCM copy into locked spans, and cue marker setup.
 *
 * Purpose: create a DirectSound sample buffer from parsed WAV data, upload the
 * PCM bytes, initialize cue markers, and clear the loading flag.
 */
int __fastcall zSndSample::InitFromWaveData_DirectSound(
    zSndWaveData *waveData
) {
    zSndWaveData *const loadedWaveData = waveData;
    if (createGuard != 0) {
        return 0;
    }

    zSndDirectSoundLegacyBufferDesc desc;
    const unsigned int pcmByteCount = (unsigned int)(loadedWaveData->pcmByteCount);
    WAVEFORMATEX *const fmt = loadedWaveData->fmt;
    memset(
        &desc,
        0,
        sizeof(desc)
    );
    desc.dwBufferBytes = pcmByteCount;

    const unsigned int flags = (unsigned int)(replayFields.flags);
    unsigned int shiftedFlags = flags;
    shiftedFlags >>= 2;
    unsigned char shiftedMode = (unsigned char)(shiftedFlags);
    desc.dwSize = 20;
    desc.dwFlags = 0x80;
    if ((shiftedMode & 1) != 0) {
        desc.dwFlags = 0xc0;
    }
    desc.lpwfxFormat = fmt;
    shiftedFlags = flags;
    shiftedFlags >>= 5;
    shiftedMode = (unsigned char)(shiftedFlags);
    if ((shiftedMode & 1) != 0) {
        desc.dwFlags |= 0x20;
    }
    shiftedFlags = flags;
    shiftedFlags >>= 1;
    shiftedMode = (unsigned char)(shiftedFlags);
    if ((shiftedMode & 1) != 0) {
        desc.dwFlags |= 0x08;
    } else {
        shiftedFlags = flags;
        shiftedFlags >>= 6;
        shiftedMode = (unsigned char)(shiftedFlags);
        if ((shiftedMode & 1) != 0) {
            desc.dwFlags |= 0x02;
        }
    }
    shiftedFlags = flags;
    shiftedFlags >>= 8;
    shiftedMode = (unsigned char)(shiftedFlags);
    if ((shiftedMode & 1) != 0) {
        desc.dwFlags |= 0x10000;
    }

    LPDIRECTSOUND const device = (LPDIRECTSOUND)(g_zSnd_BackendDevice);
    int createError =
        device->CreateSoundBuffer(
            (DSBUFFERDESC *)(&desc),
            (LPDIRECTSOUNDBUFFER *)&primaryVoice.backendBuffer,
            0
        );
    if (createError != 0) {
        zError::ReportOld(
            0x200,
            kZSndCreateSourceFile,
            0xf5,
            kCreateSoundBufferError,
            loadedWaveData->nameOrPath
        );
        zSnd::ReportDirectSoundError(
            createError,
            kZSndCreateSourceFile,
            0xf6
        );
        return 0;
    }

    primaryVoice.handleKind = ZSND_PLAYHANDLE_BACKEND;

    DWORD status;
    int error = ((LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer))->GetStatus(&status);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSndCreateSourceFile,
            0x10e
        );
    }
    if ((status & 0x02) != 0) {
        error = ((LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer))->Restore();
        if (error != 0) {
            return zSnd::ReportDirectSoundError(
                error,
                kZSndCreateSourceFile,
                0x113
            );
        }
    }

    void *audioPtr1;
    void *audioPtr2;
    DWORD audioBytes1;
    DWORD audioBytes2;
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer);
    error = buffer->Lock(
        0,
        pcmByteCount,
        &audioPtr1,
        &audioBytes1,
        &audioPtr2,
        &audioBytes2,
        0
    );
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSndCreateSourceFile,
            0x11d
        );
    }

    memcpy(
        audioPtr1,
        loadedWaveData->pcmData,
        audioBytes1
    );
    if (audioBytes2 != 0) {
        memcpy(
            audioPtr2,
            (unsigned char *)(loadedWaveData->pcmData) + audioBytes1,
            audioBytes2
        );
        audioBytes1 += audioBytes2;
    }

    error = buffer->Unlock(
        audioPtr1,
        audioBytes1,
        audioPtr2,
        audioBytes2
    );
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSndCreateSourceFile,
            0x12a
        );
    }

    error = buffer->SetCurrentPosition(0);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSndCreateSourceFile,
            0x130
        );
    }

    markerCount = loadedWaveData->cuePointCount;
    if (markerCount != 0) {
        zSndCuePoint *const cuePoints = loadedWaveData->cuePoints;
        if (markerCount > 0) {
            markerTimes = (float *)(malloc((size_t)(markerCount) * sizeof(float) + sizeof(float)));
            markerValues = (float *)(malloc((size_t)(markerCount) * sizeof(float) + sizeof(float)));
            markerAux = (int *)(malloc((size_t)(markerCount) * 2 * sizeof(int) + 2 * sizeof(int)));
        } else {
            markerTimes = 0;
            markerValues = 0;
            markerAux = 0;
        }

        int index = 0;
        while (index < markerCount) {
            const zSndCuePoint &cue = cuePoints[index];
            markerAux[index * 2] =
                (fmt->wBitsPerSample >> 3) * (int)(cue.position) * fmt->nChannels;
            markerTimes[index] = (float)(cue.position) / (float)(fmt->nSamplesPerSec);
            ++index;
        }

        markerTimes[index] = (float)(pcmByteCount) / (float)(fmt->nAvgBytesPerSec);
        ++markerCount;
    }
    playbackEventHandler = 0;
    replayFields.flags &= ~0x80;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-create.zsndsample-lockbackendbuffers
 * @recoil-artifact defines .text recoil:function:0x4a34e0: zSndSample::LockBackendBuffers.
 *
 * Evidence: BN assembly dispatches active backend 0 to the DirectSound Lock
 * slot and active backend 1 to the A3D Lock slot, with zsnd_create.cpp error
 * source lines 0x1ec and 0x1e3.
 *
 * Purpose: lock the active DirectSound or A3D sample buffer and return the
 * writable spans for streamed audio updates.
 */
int __fastcall zSndSample::LockBackendBuffers(
    unsigned int offset,
    unsigned int bytes,
    void **buffer1,
    int *buffer1Bytes,
    void **buffer2,
    int *buffer2Bytes
) {
    if (createGuard != 0) {
        return 0;
    }

    int error = 0;
    if (g_zSnd_ActiveBackend == 0) {
        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer);
        DWORD lockedBytes1 = 0;
        DWORD lockedBytes2 = 0;
        error = buffer->Lock(
            offset,
            bytes,
            buffer1,
            &lockedBytes1,
            buffer2,
            &lockedBytes2,
            0
        );
        *buffer1Bytes = (int)lockedBytes1;
        *buffer2Bytes = (int)lockedBytes2;
        if (error != 0) {
            return zSnd::ReportDirectSoundError(
                error,
                kZSndCreateSourceFile,
                0x1ec
            );
        }
    } else if (g_zSnd_ActiveBackend == 1) {
        zA3dProviderSource *const buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        error = buffer->Lock(
            offset,
            bytes,
            buffer1,
            (LPDWORD)buffer1Bytes,
            buffer2,
            (LPDWORD)buffer2Bytes,
            0
        );
        if (error != 0) {
            return zSnd::ReportA3DError(
                error,
                kZSndCreateSourceFile,
                0x1e3
            );
        }
    }

    return error == 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-create.zsndsample-unlockbackendbuffers
 * @recoil-artifact defines .text recoil:function:0x4a3590: zSndSample::UnlockBackendBuffers.
 *
 * Evidence: BN assembly dispatches active backend 0 to the DirectSound Unlock
 * slot and active backend 1 to the A3D commit-write slot, with zsnd_create.cpp
 * error source lines 0x222 and 0x21b.
 *
 * Purpose: unlock or commit the active backend sample-buffer spans after
 * streamed audio updates.
 */
int __fastcall zSndSample::UnlockBackendBuffers(
    void *buffer1,
    int buffer1Bytes,
    void *buffer2,
    int buffer2Bytes
) {
    if (createGuard != 0) {
        return 0;
    }

    int error = 0;
    if (g_zSnd_ActiveBackend == 0) {
        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(primaryVoice.backendBuffer);
        error = buffer->Unlock(
            buffer1,
            buffer1Bytes,
            buffer2,
            buffer2Bytes
        );
        if (error != 0) {
            return zSnd::ReportDirectSoundError(
                error,
                kZSndCreateSourceFile,
                0x222
            );
        }
    } else if (g_zSnd_ActiveBackend == 1) {
        zA3dProviderSource *const buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        error = buffer->Unlock(
            buffer1,
            buffer1Bytes,
            buffer2,
            buffer2Bytes
        );
        if (error != 0) {
            return zSnd::ReportA3DError(
                error,
                kZSndCreateSourceFile,
                0x21b
            );
        }
    }

    return error == 0 ? 1 : 0;
}

/**
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
            if (voice->backendBuffer != 0) {
                ((IUnknown *)(voice->backendBuffer))->Release();
            }
            voice->backendBuffer = 0;
            free(voice);
        }
    }

    free(duplicateVoices);
    duplicateVoices = 0;
    duplicateVoiceCount = 0;
    if (primaryVoice.backendBuffer != 0) {
        if (g_zSnd_ActiveBackend == 1 &&
            ((zA3dProviderSource *)(primaryVoice.backendBuffer))->FreeWaveData() < 0) {
            return 0;
        }

        ((IUnknown *)(primaryVoice.backendBuffer))->Release();
    }
    primaryVoice.backendBuffer = 0;
    replayFields.flags &= ~0x08;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-create.zsndsample-createqueuedstreamingsample
 * @recoil-artifact defines .text recoil:function:0x4a3850: zSndSample_CreateQueuedStreamingSample.
 *
 * Evidence: BN assembly allocates a zeroed zSndSample, constructs a temporary
 * zSndWaveData around caller-owned PCM storage, dispatches InitFromWaveData,
 * mirrors the init result into replay flag bit 0x08, and frees on failure.
 *
 * Purpose: allocate a streaming zSndSample around caller-owned PCM storage and
 * initialize it through the active backend.
 */
extern "C" zSndSample *__fastcall zSndSample_CreateQueuedStreamingSample(
    WAVEFORMATEX *audioFormat,
    void *audioBuffer,
    int bufferBytes
) {
    zSndSample *sample = (zSndSample *)(calloc(
        1,
        sizeof(zSndSample)
    ));
    if (sample == 0) {
        return 0;
    }

    zSndWaveData waveData(
        "",
        0
    );
    waveData.fmt = audioFormat;
    waveData.pcmData = audioBuffer;
    waveData.pcmByteCount = bufferBytes;

    sample->replayFields.flags |= 0x101;
    const int initResult = sample->InitFromWaveData(&waveData);
    sample->replayFields.gain = 1.0f;
    sample->replayFields.flags = (sample->replayFields.flags & ~0x08) | ((initResult & 1) << 3);
    if ((sample->replayFields.flags & 0x08) == 0) {
        free(sample);
        sample = 0;
    }

    return sample;
}

/**
 * Purpose: release owned sample data and free the sample record itself.
 */
void zSndSample::Destroy() {
    DestroyOwnedData();
    free(this);
}
