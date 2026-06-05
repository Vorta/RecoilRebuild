#include "GameZRecoil/zSound/zSound.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zA3dProvider.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void *g_zSnd_BackendDevice;

namespace {
const char kZSndCreateSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_create.cpp";
const char kCreateSoundBufferError[] = "Error creating sound buffer ( %s )";

const unsigned int kRiffMagic = 0x46464952;
const unsigned int kWaveMagic = 0x45564157;
const unsigned int kFmtChunkMagic = 0x20746d66;
const unsigned int kDataChunkMagic = 0x61746164;
const unsigned int kCueChunkMagic = 0x20657563;

// DirectSound provider ABI prefix consumed by the retail CreateSoundBuffer call
// at 0x4a3180. The modern SDK's DSBUFFERDESC is larger, but the original code
// passed only this DirectSound v1-compatible prefix with dwSize == 20.
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
 * Reimplements 0x4a3180: zSndSample::InitFromWaveData_DirectSound.
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
 * Reimplements 0x4a2ec0: zSndSample::InitFromWaveData_A3D.
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
    int error = device->vtable->CreateBufferByKind(
        device,
        0,
        &primaryVoice.backendBuffer
    );
    if (error != 0) {
        return zSnd::ReportA3DError(
            error,
            kZSndCreateSourceFile,
            0x4e
        );
    }

    error = ((zA3dProviderSource *)(primaryVoice.backendBuffer))->vtable->SetWaveFormat(
        (zA3dProviderSource *)(primaryVoice.backendBuffer),
        fmt
    );
    if (error != 0) {
        return zSnd::ReportA3DError(
            error,
            kZSndCreateSourceFile,
            0x51
        );
    }

    error = ((zA3dProviderSource *)(primaryVoice.backendBuffer))->vtable->SetSampleDataSize(
        (zA3dProviderSource *)(primaryVoice.backendBuffer),
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
    error = buffer->vtable->Lock(
        buffer,
        0,
        loadedWaveData->pcmByteCount,
        &audioPtr1,
        &audioBytes1,
        &audioPtr2,
        &audioBytes2,
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
    error = buffer->vtable->CommitWrite(
        buffer,
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
    buffer->vtable->Rewind(buffer);

    unsigned int spatialFlags = (unsigned int)(replayFields.flags);
    spatialFlags >>= 2;
    unsigned char spatialMode = (unsigned char)(spatialFlags);
    if ((spatialMode & 1) != 0) {
        buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        buffer->vtable->SetRange(
            buffer,
            rangeMin,
            rangeMax,
            1
        );
        buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        buffer->vtable->SetA3DDistanceScale(
            buffer,
            a3dDistanceScale
        );
    } else {
        buffer = (zA3dProviderSource *)(primaryVoice.backendBuffer);
        buffer->vtable->SetSpatializationEnabled(
            buffer,
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
 * Reimplements 0x4a34e0: zSndSample::LockBackendBuffers.
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
        error = buffer->vtable
                    ->Lock(
                        buffer,
                        offset,
                        bytes,
                        buffer1,
                        buffer1Bytes,
                        buffer2,
                        buffer2Bytes,
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
 * Reimplements 0x4a3590: zSndSample::UnlockBackendBuffers.
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
        error = buffer->vtable->CommitWrite(
            buffer,
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
 * Reimplements 0x4a2ea0: zSndSample::InitFromWaveData.
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
 * Reimplements 0x4a3850: zSndSample_CreateQueuedStreamingSample.
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

    zSndWaveData waveData = {0};
    waveData.ConstructorFromPath(
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

    waveData.Destructor();
    return sample;
}

/**
 * Reimplements 0x4a53f0: zSndWaveData::ConstructorFromPath.
 *
 * Purpose: initialize a WAV data record from a path and optionally load and
 * parse it immediately.
 */
zSndWaveData * zSndWaveData::ConstructorFromPath(
    const char *path,
    int loadNow
) {
    nameOrPath = _strdup(path);
    fileData = 0;
    pcmData = 0;
    fmt = 0;
    pcmByteCount = 0;
    fileSize = 0;
    parsedOk = 0;
    cuePointCount = 0;
    cuePoints = 0;

    if (loadNow != 0) {
        LoadAndParseIfNeeded();
    }

    return this;
}

/**
 * Reimplements 0x4a5440: zSndWaveData::Destructor.
 *
 * Purpose: reset parsed WAV state and release the duplicated path string.
 */
void zSndWaveData::Destructor() {
    Reset();
    if (nameOrPath != 0) {
        free(nameOrPath);
    }
}

/**
 * Reimplements 0x4a5460: zSndWaveData::ParseLoadedWaveFile.
 *
 * Purpose: scan a loaded RIFF/WAVE buffer and cache its fmt, data, and cue
 * chunk records.
 */
int zSndWaveData::ParseLoadedWaveFile() {
    unsigned char *chunk = (unsigned char *)(fileData);
    if (chunk == 0) {
        return 0;
    }

    fmt = 0;
    pcmData = 0;
    pcmByteCount = 0;

    const unsigned int riffMagic = *((unsigned int *)(chunk));
    chunk += 4;
    const unsigned int riffPayloadBytes = *((unsigned int *)(chunk));
    chunk += 4;
    const unsigned int waveMagic = *((unsigned int *)(chunk));
    chunk += 4;
    if (riffMagic == kRiffMagic && waveMagic == kWaveMagic) {
        unsigned char *const riffEnd = chunk + riffPayloadBytes - 4;
        int invalidFmtChunk = 0;
        while (chunk < riffEnd && invalidFmtChunk == 0) {
            const unsigned int chunkId = *((unsigned int *)(chunk));
            chunk += 4;
            const unsigned int chunkSize = *((unsigned int *)(chunk));
            chunk += 4;

            if (chunkId != kCueChunkMagic) {
                if (chunkId != kFmtChunkMagic) {
                    if (chunkId == kDataChunkMagic) {
                        if (pcmData == 0 || pcmByteCount == 0) {
                            pcmData = chunk;
                            pcmByteCount = (int)(chunkSize);
                        }
                    }
                } else {
                    if (fmt == 0) {
                        if (chunkSize >= 0x0e) {
                            fmt = (WAVEFORMATEX *)(chunk);
                        } else {
                            invalidFmtChunk = 1;
                        }
                    }
                }
            } else {
                if (cuePoints == 0 || cuePointCount == 0) {
                    cuePointCount = (int)(*((unsigned int *)(chunk)));
                    cuePoints = (zSndCuePoint *)(chunk + 4);
                }
            }

            if (invalidFmtChunk == 0) {
                chunk += ((chunkSize + 1) & ~1u);
            }
        }
    }

    return 1;
}

/**
 * Reimplements 0x4a5540: zSndWaveData::LoadAndParseIfNeeded.
 *
 * Purpose: load a named WAV file from disk once, parse it, and cache the parse
 * result.
 */
int zSndWaveData::LoadAndParseIfNeeded() {
    if (parsedOk != 0) {
        return 1;
    }

    if (nameOrPath == 0) {
        return 0;
    }

    FILE *const file = fopen(
        nameOrPath,
        "rb"
    );
    if (file == 0) {
        return 0;
    }

    fileSize = zUtil::ZRDR_GetFileSize(file);
    fileData = calloc(
        fileSize,
        1
    );
    fread(
        fileData,
        fileSize,
        1,
        file
    );
    fclose(file);

    parsedOk = ParseLoadedWaveFile();
    return parsedOk;
}

/**
 * Reimplements 0x4a5600: zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded.
 *
 * Purpose: load a named WAV payload from an index archive once, parse it, and
 * cache the parse result.
 */
int zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded(
    zIndexArchive *archive
) {
    if (parsedOk != 0) {
        return 1;
    }

    unsigned int archiveFileSize = 0;
    archive->ReadFileByName(
        nameOrPath,
        0,
        &archiveFileSize
    );
    unsigned int *const fileSizeOut = (unsigned int *)(&fileSize);
    *fileSizeOut = archiveFileSize;
    if (archiveFileSize > 0) {
        fileData = calloc(
            archiveFileSize,
            1
        );
        archive->ReadFileByName(
            nameOrPath,
            fileData,
            fileSizeOut
        );
        parsedOk = ParseLoadedWaveFile();
    }

    return parsedOk;
}

/**
 * Reimplements 0x4a55c0: zSndWaveData::Reset.
 *
 * Purpose: free loaded WAV file storage and clear cached parse fields.
 */
int zSndWaveData::Reset() {
    if (parsedOk != 0) {
        if (fileData != 0) {
            free(fileData);
            fileData = 0;
        }

        fileSize = 0;
        pcmData = 0;
        pcmByteCount = 0;
        fmt = 0;
        cuePointCount = 0;
        cuePoints = 0;
        parsedOk = 0;
    }

    return 1;
}
