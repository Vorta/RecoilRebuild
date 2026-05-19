#include "GameZRecoil/zSound/zSound.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zReader/zReader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void *g_zSnd_BackendDevice;

namespace {
const char *kZSndCreateSourceFile = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_create.cpp";
const char *kCreateSoundBufferError = "Error creating sound buffer ( %s )";

const unsigned int kRiffMagic = 0x46464952;
const unsigned int kWaveMagic = 0x45564157;
const unsigned int kFmtChunkMagic = 0x20746d66;
const unsigned int kDataChunkMagic = 0x61746164;
const unsigned int kCueChunkMagic = 0x20657563;

typedef int(__stdcall *BackendGetStatusFn)(void *self, int *outStatus);
typedef int(__stdcall *BackendSimpleFn)(void *self);
typedef int(__stdcall *BackendSetIntFn)(void *self, int value);
typedef int(__stdcall *BackendLockFn)(void *self, unsigned int offset,
                                      unsigned int bytes, void **outPtr1, int *outBytes1,
                                      void **outPtr2, int *outBytes2, unsigned int flags);
typedef int(__stdcall *BackendUnlockFn)(void *self, void *ptr1, int bytes1,
                                        void *ptr2, int bytes2);
typedef int(__stdcall *BackendCreateBufferFn)(void *self, void *desc,
                                              zSndBuffer **outBuffer, void *outerUnknown);
typedef int(__stdcall *A3dCreateBufferFn)(void *self, int bufferKind,
                                          zSndBuffer **outBuffer);
typedef int(__stdcall *A3dSetWaveFormatFn)(void *self, WAVEFORMATEX *format);
typedef int(__stdcall *A3dSetRangeFn)(void *self, float rangeMin, float rangeMax, int enabled);
typedef int(__stdcall *BackendSetFloatFn)(void *self, float value);

struct zSndDirectSoundBufferDesc {
    unsigned int size;
    unsigned int flags;
    unsigned int bufferBytes;
    unsigned int reserved;
    WAVEFORMATEX *format;
};

struct DirectSoundDeviceVTable {
    void *slots00_08[3];
    BackendCreateBufferFn CreateSoundBuffer;
};

struct DirectSoundDevice {
    DirectSoundDeviceVTable *vtable;
};

struct DirectSoundBufferVTable {
    void *slots00_20[9];
    BackendGetStatusFn GetStatus;
    void *Initialize;
    BackendLockFn Lock;
    void *Play;
    BackendSetIntFn SetCurrentPosition;
    void *slots38_48[5];
    BackendUnlockFn Unlock;
    BackendSimpleFn Restore;
};

struct DirectSoundBuffer {
    DirectSoundBufferVTable *vtable;
};

struct A3dDeviceVTable {
    void *slots00_40[17];
    A3dCreateBufferFn CreateBufferByKind;
};

struct A3dDevice {
    A3dDeviceVTable *vtable;
};

struct A3dBufferVTable {
    void *slots00_10[5];
    BackendSetIntFn SetSampleDataSize;
    void *slot18;
    A3dSetWaveFormatFn SetWaveFormat;
    void *slots20_28[3];
    BackendLockFn Lock;
    BackendUnlockFn CommitWrite;
    void *Play;
    void *Stop;
    BackendSimpleFn Reset;
    void *slots40_94[22];
    A3dSetRangeFn SetRange;
    void *slots9c_b4[7];
    BackendSetFloatFn SetA3DDistanceScale;
    void *slotsbc_cc[5];
    BackendSetIntFn SetSpatializationEnabled;
};

struct A3dBuffer {
    A3dBufferVTable *vtable;
};

char *DuplicateCString(const char *value) {
#if defined(_MSC_VER)
    return _strdup(value);
#else
    return strdup(value);
#endif
}

unsigned int ReadU32(const void *address) {
    unsigned int value = 0;
    memcpy(&value, address, sizeof(value));
    return value;
}

void InitWaveMarkers(zSndSample *sample, zSndWaveData *waveData) {
    sample->markerCount = waveData->cuePointCount;
    if (sample->markerCount == 0) {
        return;
    }

    if (sample->markerCount > 0) {
        sample->markerTimes = static_cast<float *>(malloc(
            static_cast<size_t>(sample->markerCount) * sizeof(float) + sizeof(float)));
        sample->markerValues = static_cast<float *>(malloc(
            static_cast<size_t>(sample->markerCount) * sizeof(float) + sizeof(float)));
        sample->markerAux = static_cast<int *>(
            malloc(static_cast<size_t>(sample->markerCount) * 2 * sizeof(int) +
                        2 * sizeof(int)));
    } else {
        sample->markerTimes = 0;
        sample->markerValues = 0;
        sample->markerAux = 0;
    }

    int index = 0;
    while (index < sample->markerCount) {
        const zSndCuePoint &cue = waveData->cuePoints[index];
        sample->markerAux[index * 2] = (waveData->fmt->wBitsPerSample >> 3) *
                                       static_cast<int>(cue.position) *
                                       waveData->fmt->nChannels;
        sample->markerTimes[index] =
            static_cast<float>(cue.position) / static_cast<float>(waveData->fmt->nSamplesPerSec);
        ++index;
    }

    sample->markerTimes[index] = static_cast<float>(waveData->pcmByteCount) /
                                 static_cast<float>(waveData->fmt->nAvgBytesPerSec);
    ++sample->markerCount;
}
} // namespace

// Reimplements 0x4a3180: zSndSample::InitFromWaveData_DirectSound
RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSample::InitFromWaveData_DirectSound(zSndWaveData *waveData) {
    if (createGuard != 0) {
        return 0;
    }

    zSndDirectSoundBufferDesc desc = {0};
    desc.size = sizeof(desc);
    desc.flags = 0x80;
    desc.bufferBytes = static_cast<unsigned int>(waveData->pcmByteCount);
    desc.format = waveData->fmt;

    const int flags = replayFields.flags;
    if (((flags >> 2) & 1) != 0) {
        desc.flags = 0xc0;
    }
    if (((flags >> 5) & 1) != 0) {
        desc.flags |= 0x20;
    }
    if (((flags >> 1) & 1) != 0) {
        desc.flags |= 0x08;
    } else if (((flags >> 6) & 1) != 0) {
        desc.flags |= 0x02;
    }
    if (((flags >> 8) & 1) != 0) {
        desc.flags |= 0x10000;
    }

    DirectSoundDevice *const device = (DirectSoundDevice *)(g_zSnd_BackendDevice);
    int error =
        device->vtable->CreateSoundBuffer(device, &desc, &primaryVoice.backendBuffer, 0);
    if (error != 0) {
        zError::ReportOld(0x200, kZSndCreateSourceFile, 0xf5, kCreateSoundBufferError,
                          waveData->nameOrPath);
        zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0xf6);
        return 0;
    }

    primaryVoice.handleKind = ZSND_PLAYHANDLE_BACKEND;
    DirectSoundBuffer * buffer = (DirectSoundBuffer *)(primaryVoice.backendBuffer);

    int status = 0;
    error = buffer->vtable->GetStatus(buffer, &status);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0x10e);
    }
    if ((status & 0x02) != 0) {
        error = buffer->vtable->Restore(buffer);
        if (error != 0) {
            return zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0x113);
        }
    }

    void *audioPtr1 = 0;
    void *audioPtr2 = 0;
    int audioBytes1 = 0;
    int audioBytes2 = 0;
    error = buffer->vtable->Lock(buffer, 0, static_cast<unsigned int>(waveData->pcmByteCount),
                                 &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0x11d);
    }

    memcpy(audioPtr1, waveData->pcmData, audioBytes1);
    if (audioBytes2 != 0) {
        memcpy(audioPtr2, static_cast<unsigned char *>(waveData->pcmData) + audioBytes1,
                    audioBytes2);
        audioBytes1 += audioBytes2;
    }

    error = buffer->vtable->Unlock(buffer, audioPtr1, audioBytes1, audioPtr2, audioBytes2);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0x12a);
    }

    error = buffer->vtable->SetCurrentPosition(buffer, 0);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0x130);
    }

    InitWaveMarkers(this, waveData);
    playbackEventHandler = 0;
    replayFields.flags &= ~0x80;
    return 1;
}

// Reimplements 0x4a2ec0: zSndSample::InitFromWaveData_A3D
RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSample::InitFromWaveData_A3D(zSndWaveData *waveData) {
    if (createGuard != 0) {
        return 0;
    }

    A3dDevice *const device = (A3dDevice *)(g_zSnd_BackendDevice);
    int error = device->vtable->CreateBufferByKind(device, 0, &primaryVoice.backendBuffer);
    if (error != 0) {
        return zSnd::ReportA3DError(error, kZSndCreateSourceFile, 0x4e);
    }

    A3dBuffer *buffer = (A3dBuffer *)(primaryVoice.backendBuffer);
    error = buffer->vtable->SetWaveFormat(buffer, waveData->fmt);
    if (error != 0) {
        return zSnd::ReportA3DError(error, kZSndCreateSourceFile, 0x51);
    }

    error = buffer->vtable->SetSampleDataSize(buffer, waveData->pcmByteCount);
    if (error != 0) {
        return zSnd::ReportA3DError(error, kZSndCreateSourceFile, 0x54);
    }

    void *audioPtr1 = 0;
    void *audioPtr2 = 0;
    int audioBytes1 = 0;
    int audioBytes2 = 0;
    error = buffer->vtable->Lock(buffer, 0, static_cast<unsigned int>(waveData->pcmByteCount),
                                 &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0);
    if (error != 0) {
        return zSnd::ReportA3DError(error, kZSndCreateSourceFile, 0x5a);
    }

    memcpy(audioPtr1, waveData->pcmData, audioBytes1);
    if (audioBytes2 != 0) {
        memcpy(audioPtr2, static_cast<unsigned char *>(waveData->pcmData) + audioBytes1,
                    audioBytes2);
        audioBytes1 += audioBytes2;
    }

    buffer = (A3dBuffer *)(primaryVoice.backendBuffer);
    error = buffer->vtable->CommitWrite(buffer, audioPtr1, audioBytes1, audioPtr2, audioBytes2);
    if (error != 0) {
        return zSnd::ReportA3DError(error, kZSndCreateSourceFile, 0x66);
    }

    buffer = (A3dBuffer *)(primaryVoice.backendBuffer);
    buffer->vtable->Reset(buffer);

    buffer = (A3dBuffer *)(primaryVoice.backendBuffer);
    if (((replayFields.flags >> 2) & 1) != 0) {
        buffer->vtable->SetRange(buffer, rangeMin, rangeMax, 1);
        buffer = (A3dBuffer *)(primaryVoice.backendBuffer);
        buffer->vtable->SetA3DDistanceScale(buffer, a3dDistanceScale);
    } else {
        buffer->vtable->SetSpatializationEnabled(buffer, 1);
    }

    sampleRate = static_cast<float>(waveData->fmt->nSamplesPerSec);
    InitWaveMarkers(this, waveData);
    playbackEventHandler = 0;
    replayFields.flags &= ~0x80;
    return 1;
}

// Reimplements 0x4a34e0: zSndSample::LockBackendBuffers
RECOIL_NOINLINE int RECOIL_FASTCALL zSndSample::LockBackendBuffers(
    unsigned int offset, unsigned int bytes, void **buffer1, int *buffer1Bytes,
    void **buffer2, int *buffer2Bytes) {
    if (createGuard != 0) {
        return 0;
    }

    int error = 0;
    if (g_zSnd_ActiveBackend == 0) {
        DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(primaryVoice.backendBuffer);
        error = buffer->vtable->Lock(buffer, offset, bytes, buffer1, buffer1Bytes, buffer2,
                                     buffer2Bytes, 0);
        if (error != 0) {
            return zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0x1ec);
        }
    } else if (g_zSnd_ActiveBackend == 1) {
        A3dBuffer *const buffer = (A3dBuffer *)(primaryVoice.backendBuffer);
        error = buffer->vtable->Lock(buffer, offset, bytes, buffer1, buffer1Bytes, buffer2,
                                     buffer2Bytes, 0);
        if (error != 0) {
            return zSnd::ReportA3DError(error, kZSndCreateSourceFile, 0x1e3);
        }
    }

    return error == 0 ? 1 : 0;
}

// Reimplements 0x4a3590: zSndSample::UnlockBackendBuffers
RECOIL_NOINLINE int RECOIL_FASTCALL zSndSample::UnlockBackendBuffers(
    void *buffer1, int buffer1Bytes, void *buffer2, int buffer2Bytes) {
    if (createGuard != 0) {
        return 0;
    }

    int error = 0;
    if (g_zSnd_ActiveBackend == 0) {
        DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(primaryVoice.backendBuffer);
        error = buffer->vtable->Unlock(buffer, buffer1, buffer1Bytes, buffer2, buffer2Bytes);
        if (error != 0) {
            return zSnd::ReportDirectSoundError(error, kZSndCreateSourceFile, 0x222);
        }
    } else if (g_zSnd_ActiveBackend == 1) {
        A3dBuffer *const buffer = (A3dBuffer *)(primaryVoice.backendBuffer);
        error = buffer->vtable->CommitWrite(buffer, buffer1, buffer1Bytes, buffer2, buffer2Bytes);
        if (error != 0) {
            return zSnd::ReportA3DError(error, kZSndCreateSourceFile, 0x21b);
        }
    }

    return error == 0 ? 1 : 0;
}

// Reimplements 0x4a2ea0: zSndSample::InitFromWaveData
RECOIL_NOINLINE int RECOIL_FASTCALL zSndSample::InitFromWaveData(zSndWaveData *waveData) {
    if (g_zSnd_ActiveBackend == 0) {
        return InitFromWaveData_DirectSound(waveData);
    }

    if (g_zSnd_ActiveBackend == 1) {
        return InitFromWaveData_A3D(waveData);
    }

    return 0;
}

// Reimplements 0x4a3850: zSndSample_CreateQueuedStreamingSample
extern "C" RECOIL_NOINLINE zSndSample *RECOIL_FASTCALL zSndSample_CreateQueuedStreamingSample(
    WAVEFORMATEX *audioFormat, void *audioBuffer, int bufferBytes) {
    zSndSample *sample = static_cast<zSndSample *>(calloc(1, sizeof(zSndSample)));
    if (sample == 0) {
        return 0;
    }

    zSndWaveData waveData = {0};
    waveData.ConstructorFromPath("", 0);
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

// Reimplements 0x4a53f0: zSndWaveData::ConstructorFromPath
RECOIL_NOINLINE zSndWaveData *RECOIL_THISCALL
zSndWaveData::ConstructorFromPath(const char *path, int loadNow) {
    nameOrPath = DuplicateCString(path);
    parsedOk = 0;
    fileSize = 0;
    fileData = 0;
    pcmByteCount = 0;
    fmt = 0;
    cuePointCount = 0;
    cuePoints = 0;
    pcmData = 0;

    if (loadNow != 0) {
        LoadAndParseIfNeeded();
    }

    return this;
}

// Reimplements 0x4a5440: zSndWaveData::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL zSndWaveData::Destructor() {
    Reset();
    if (nameOrPath != 0) {
        free(nameOrPath);
    }
}

// Reimplements 0x4a5460: zSndWaveData::ParseLoadedWaveFile
RECOIL_NOINLINE int RECOIL_THISCALL zSndWaveData::ParseLoadedWaveFile() {
    unsigned char *const bytes = static_cast<unsigned char *>(fileData);
    if (bytes == 0) {
        return 0;
    }

    fmt = 0;
    pcmData = 0;
    pcmByteCount = 0;

    const unsigned int riffMagic = ReadU32(bytes);
    const unsigned int riffPayloadBytes = ReadU32(bytes + 4);
    const unsigned int waveMagic = ReadU32(bytes + 8);
    unsigned char *chunk = bytes + 12;
    if (riffMagic == kRiffMagic && waveMagic == kWaveMagic) {
        unsigned char *const riffEnd = chunk + riffPayloadBytes - 4;
        int invalidFmtChunk = 0;
        while (chunk < riffEnd && invalidFmtChunk == 0) {
            const unsigned int chunkId = ReadU32(chunk);
            const unsigned int chunkSize = ReadU32(chunk + 4);
            unsigned char *const body = chunk + 8;

            if (chunkId == kCueChunkMagic) {
                if (cuePoints == 0 || cuePointCount == 0) {
                    cuePointCount = static_cast<int>(ReadU32(body));
                    cuePoints = (zSndCuePoint *)(body + 4);
                }
            } else if (chunkId == kFmtChunkMagic) {
                if (fmt == 0) {
                    if (chunkSize < 0x0e) {
                        invalidFmtChunk = 1;
                    } else {
                        fmt = (WAVEFORMATEX *)(body);
                    }
                }
            } else if (chunkId == kDataChunkMagic) {
                if (pcmData == 0 || pcmByteCount == 0) {
                    pcmData = body;
                    pcmByteCount = static_cast<int>(chunkSize);
                }
            }

            if (invalidFmtChunk == 0) {
                chunk = body + ((chunkSize + 1) & ~1u);
            }
        }
    }

    return 1;
}

// Reimplements 0x4a5540: zSndWaveData::LoadAndParseIfNeeded
RECOIL_NOINLINE int RECOIL_THISCALL zSndWaveData::LoadAndParseIfNeeded() {
    if (parsedOk != 0) {
        return 1;
    }

    if (nameOrPath == 0) {
        return 0;
    }

    FILE *const file = fopen(nameOrPath, "rb");
    if (file == 0) {
        return 0;
    }

    fileSize = zUtil::ZRDR_GetFileSize(file);
    fileData = calloc(fileSize, 1);
    fread(fileData, fileSize, 1, file);
    fclose(file);

    parsedOk = ParseLoadedWaveFile();
    return parsedOk;
}

// Reimplements 0x4a5600: zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded
RECOIL_NOINLINE int RECOIL_THISCALL
zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded(zIndexArchive *archive) {
    if (parsedOk != 0) {
        return 1;
    }

    unsigned int archiveFileSize = 0;
    archive->ReadFileByName(nameOrPath, 0, &archiveFileSize);
    fileSize = static_cast<int>(archiveFileSize);
    if (archiveFileSize > 0) {
        fileData = calloc(archiveFileSize, 1);
        archive->ReadFileByName(nameOrPath, fileData, &archiveFileSize);
        fileSize = static_cast<int>(archiveFileSize);
        parsedOk = ParseLoadedWaveFile();
    }

    return parsedOk;
}

// Reimplements 0x4a55c0: zSndWaveData::Reset
RECOIL_NOINLINE int RECOIL_THISCALL zSndWaveData::Reset() {
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
