#include "GameZRecoil/zSound/zsnd.h"

#include "GameZRecoil/zReader/zreader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
const unsigned int kRiffMagic = 0x46464952;
const unsigned int kWaveMagic = 0x45564157;
const unsigned int kFmtChunkMagic = 0x20746d66;
const unsigned int kDataChunkMagic = 0x61746164;
const unsigned int kCueChunkMagic = 0x20657563;
} // namespace

/**
 * Reimplements 0x4a53f0: zSndWaveData::zSndWaveData.
 *
 * Purpose: initialize a WAV data record from a path and optionally load and
 * parse it immediately.
 */
zSndWaveData::zSndWaveData(
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
        fmt = 0;
        pcmByteCount = 0;
        cuePoints = 0;
        cuePointCount = 0;
        parsedOk = 0;
    }

    return 1;
}
