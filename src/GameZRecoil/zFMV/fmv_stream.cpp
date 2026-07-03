/* This source-layout fragment is included by the current compatibility container.
 * Parent build/manifests must compile this path directly after retiring the container include.
 */

/**
 * Reimplements 0x463d50: zFMV_Stream::Init.
 * Purpose: initialize an FMV stream object, audio/video state, and critical section.
 */
zFMV_Stream * zFMV_Stream::Init(
    const char *mediaPath,
    int modeFlags
) {
    this->mediaPath = DuplicateCString(mediaPath);
    srcFormat = 0;
    dstFormat = 0;
    compressedFrameBuffer = 0;
    surface = 0;
    pixels = 0;
    alphaMap = 0;
    palette = 0;
    audioSample = 0;
    audioFormat = 0;
    hasAudioStream = 0;
    hasVideoStream = 0;
    readStreamingAudio = 1;
    this->modeFlags = modeFlags;

    InitializeCriticalSection(&criticalSection);
    AVIFileInit();
    OpenAudio();
    Constructor();
    return this;
}

/**
 * Reimplements 0x463dd0: zFMV_Stream::Destructor.
 * Purpose: release audio/video streams, decompressor state, image buffers, and critical section.
 */
void zFMV_Stream::Destructor() {
    if (hasAudioStream != 0) {
        if (audioBuffer != 0) {
            free(audioBuffer);
            audioBuffer = 0;
        }

        if (audioSample != 0) {
            audioSample->Destroy();
        }

        if (audioFormat != 0) {
            free(audioFormat);
            audioFormat = 0;
        }

        AVIStreamRelease(audioStream);
    }

    if (hasVideoStream != 0) {
        if (videoDecompressor != 0) {
            ICSendMessage(
                videoDecompressor,
                ICM_DECOMPRESS_END,
                0,
                0
            );
            ICClose(videoDecompressor);
        }

        free(srcFormat);
        free(dstFormat);
        free(compressedFrameBuffer);

        if (surface != 0) {
            g_zVideo_pfnImageEnsureSurfaceForCurrentDevice((zVidImagePartial *)(this));
        }

        free(pixels);
        free(alphaMap);
        free(palette);

        AVIStreamRelease(videoStream);
        AVIFileExit();
    }

    DeleteCriticalSection(&criticalSection);
    free(mediaPath);
}

/**
 * Reimplements 0x463ef0: zFMV_Stream::Constructor.
 * Purpose: open the AVI video stream, configure decompression, and initialize the image surface state.
 */
void zFMV_Stream::Constructor() {
    currentFrameIndex = 0;

    const HRESULT openResult = AVIStreamOpenFromFileA(
        &videoStream,
        mediaPath,
        streamtypeVIDEO,
        0,
        0x10,
        0
    );
    if (openResult != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0x60,
            g_zFMV_CannotOpenAviFileMsg
        );
        AVIFileExit();
        return;
    }

    LONG formatBytes = 0;
    if (AVIStreamReadFormat(
        videoStream,
        0,
        0,
        &formatBytes
    ) != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0x67,
            g_zFMV_CannotReadAviFormatSizeMsg
        );
        AVIFileExit();
        return;
    }

    srcFormat = calloc(
        formatBytes,
        1
    );
    const LONG dstFormatBytes =
        formatBytes > (LONG)(sizeof(BITMAPV4HEADER)) ? formatBytes : (LONG)(sizeof(BITMAPV4HEADER));
    dstFormat = calloc(
        dstFormatBytes,
        1
    );

    if (AVIStreamReadFormat(
        videoStream,
        0,
        srcFormat,
        &formatBytes
    ) != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0x71,
            g_zFMV_CannotReadAviFormatMsg
        );
        AVIFileExit();
        return;
    }

    videoFrameCount = AVIStreamLength(videoStream);
    if (AVIStreamInfoA(
            videoStream,
            &videoStreamInfo,
            sizeof(videoStreamInfo)
        ) != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0x79,
            g_zFMV_CannotReadAviStreamInfoMsg
        );
        AVIFileExit();
        return;
    }

    memcpy(
        dstFormat,
        srcFormat,
        formatBytes
    );
    BITMAPINFOHEADER *const srcHeader = (BITMAPINFOHEADER *)(srcFormat);
    BITMAPV4HEADER *const dstHeader = (BITMAPV4HEADER *)(dstFormat);
    dstHeader->bV4Size = (DWORD)(dstFormatBytes);
    dstHeader->bV4BitCount = (WORD)(zVideo::GetDisplayModeBpp());
    dstHeader->bV4V4Compression = BI_BITFIELDS;
    if (dstHeader->bV4BitCount == 24) {
        dstHeader->bV4V4Compression = BI_RGB;
    }
    dstHeader->bV4ClrUsed = 0;
    zVideo::PixelPack_GetRgbMasks(
        (unsigned int *)(&dstHeader->bV4RedMask),
        (unsigned int *)(&dstHeader->bV4GreenMask),
        (unsigned int *)(&dstHeader->bV4BlueMask)
    );
    dstHeader->bV4AlphaMask = 0;

    const int alignedWidth = (dstHeader->bV4Width + 3) & ~3;
    dstHeader->bV4SizeImage = dstHeader->bV4Height * alignedWidth * (dstHeader->bV4BitCount >> 3);

    int compressedFrameBytes =
        (srcHeader->biBitCount >> 3) * srcHeader->biWidth * srcHeader->biHeight;
    const int suggestedBufferSize = (int)(videoStreamInfo.dwSuggestedBufferSize);
    if (suggestedBufferSize != 0) {
        compressedFrameBytes = suggestedBufferSize;
    }
    compressedFrameBufferBytes = compressedFrameBytes;

    videoDecompressor = ICLocate(
        ICTYPE_VIDEO,
        videoStreamInfo.fccHandler,
        (LPBITMAPINFOHEADER)(srcFormat),
        (LPBITMAPINFOHEADER)(dstFormat),
        ICMODE_DECOMPRESS
    );
    compressedFrameBuffer = calloc(
        compressedFrameBytes,
        1
    );

    decodedFrameStrideBytes = (dstHeader->bV4BitCount >> 3) * dstHeader->bV4Width;
    ICSendMessage(
        videoDecompressor,
        ICM_DECOMPRESS_BEGIN,
        (DWORD_PTR)(srcFormat),
        (DWORD_PTR)(dstFormat)
    );

    const unsigned int rate = videoStreamInfo.dwRate;
    const unsigned int scale = videoStreamInfo.dwScale;
    videoFramesPerSecond = rate / scale;
    reservedF4 = 0;
    reservedF8 = 0;
    msPerFrame = ((rate >> 1) + (scale * 1000)) / rate;

    frameWidth = dstHeader->bV4Width;
    frameHeight = dstHeader->bV4Height;

    pixels = calloc(
        dstHeader->bV4SizeImage,
        1
    );
    pixelCount = (int)(dstHeader->bV4SizeImage);
    width = (short)(dstHeader->bV4Width);
    height = (short)(dstHeader->bV4Height);
    headerFlagsByte = 0;
    formatFlagsPacked = 0;
    uPow2Shift = 0;
    vPow2Shift = 0;
    alphaMap = 0;
    widthScale = 0.0f;
    queuedAlphaMap = 0;
    uShiftFrom20 = 0;
    uMask = 0;
    vMaskFixed20 = 0;
    surface = 0;
    palette = 0;
    pitchWords = (short)(dstHeader->bV4Width);

    dstHeader->bV4Height = -dstHeader->bV4Height;
    hasVideoStream = 1;
}

/**
 * Reimplements 0x4641a0: zFMV_Stream::OpenAudio.
 * Purpose: open AVI audio, load or queue sample data, and create the FMV sound sample.
 */
void zFMV_Stream::OpenAudio() {
    audioStream = 0;
    if (AVIStreamOpenFromFileA(
            &audioStream,
            mediaPath,
            streamtypeAUDIO,
            0,
            0,
            0
        ) != 0) {
        return;
    }

    LONG audioFormatBytes = 0;
    if (AVIStreamReadFormat(
        audioStream,
        0,
        0,
        &audioFormatBytes
    ) != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0xcb,
            g_zFMV_CannotReadAviSoundFormatSizeMsg
        );
        return;
    }

    audioFormat = calloc(
        audioFormatBytes,
        1
    );
    if (AVIStreamReadFormat(
        audioStream,
        0,
        audioFormat,
        &audioFormatBytes
    ) != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0xd2,
            g_zFMV_CannotReadAviSoundFormatMsg
        );
        return;
    }

    if (AVIStreamInfoA(
            audioStream,
            &audioStreamInfo,
            sizeof(audioStreamInfo)
        ) != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0xd8,
            g_zFMV_CannotReadAviSoundStreamInfoMsg
        );
        return;
    }

    const unsigned int sampleSize = audioStreamInfo.dwSampleSize;
    if (modeFlags != 0) {
        const unsigned int segmentBytes = audioStreamInfo.dwSuggestedBufferSize;
        audioSegmentBytes = segmentBytes;
        audioBuffer = calloc(
            segmentBytes * 2,
            1
        );

        if (AVIStreamRead(
                audioStream,
                0,
                segmentBytes / sampleSize,
                audioBuffer,
                segmentBytes,
                0,
                0
            ) != 0) {
            zError::ReportOld(
                0x400,
                g_zFMV_SourceFile_FmvStreamCpp,
                0xe2,
                g_zFMV_CannotReadAviSoundStreamMsg
            );
            return;
        }

        audioSample = zSndSample_CreateQueuedStreamingSample(
            (WAVEFORMATEX *)(audioFormat),
            audioBuffer,
            segmentBytes * 2
        );
        audioRefillSecondHalfNext = 1;
        hasAudioStream = 1;
        audioReadSampleIndex = segmentBytes / sampleSize;
        return;
    }

    const unsigned int audioBytes = AVIStreamLength(audioStream) * sampleSize;
    audioSegmentBytes = audioBytes;
    audioBuffer = calloc(
        audioBytes,
        1
    );

    if (AVIStreamRead(
            audioStream,
            0,
            audioStreamInfo.dwLength,
            audioBuffer,
            audioBytes,
            0,
            0
        ) != 0) {
        zError::ReportOld(
            0x400,
            g_zFMV_SourceFile_FmvStreamCpp,
            0xf0,
            g_zFMV_CannotReadAviSoundStreamMsg
        );
        return;
    }

    audioSample = zSndSample_CreateQueuedStreamingSample(
        (WAVEFORMATEX *)(audioFormat),
        audioBuffer,
        audioBytes
    );
    hasAudioStream = 1;
}

/**
 * Reimplements 0x4643a0: zFMV_Stream::ReadAndDecodeFrame
 * (D:\Proj\GameZRecoil\zFMV\fmv_stream.cpp).
 * Purpose: read and decompress one video frame and refill streaming audio when needed.
 */
int zFMV_Stream::ReadAndDecodeFrame(
    unsigned int frameIndex
) {
    if (frameIndex != 0xffffffffu) {
        currentFrameIndex = frameIndex;
    }

    const unsigned int frameCount = videoFrameCount;
    if ((int)(currentFrameIndex) < (int)(frameCount)) {
        if (AVIStreamRead(
                videoStream,
                currentFrameIndex,
                1,
                compressedFrameBuffer,
                compressedFrameBufferBytes,
                0,
                0
            ) != 0) {
            zError::ReportOld(
                0x400,
                g_zFMV_SourceFile_FmvStreamCpp,
                0x105,
                g_zFMV_CannotReadAviVideoStreamMsg
            );
            return 0;
        }

        EnterCriticalSection(&criticalSection);
        if (ICDecompress(
                videoDecompressor,
                0,
                (LPBITMAPINFOHEADER)(srcFormat),
                compressedFrameBuffer,
                (LPBITMAPINFOHEADER)(dstFormat),
                pixels
            ) != 0) {
            zError::ReportOld(
                0x400,
                g_zFMV_SourceFile_FmvStreamCpp,
                0x10c,
                g_zFMV_CannotDecompressAviVideoStreamMsg
            );
            return 0;
        }
        LeaveCriticalSection(&criticalSection);
    }

    ++currentFrameIndex;
    if ((int)(currentFrameIndex) >= (int)(frameCount)) {
        currentFrameIndex = 0;
    }

    if (hasAudioStream != 0) {
        if (readStreamingAudio != 0) {
            readStreamingAudio = 0;
            audioSample->PlayA3DSimple(1.0f);
            return currentFrameIndex;
        }

        if (modeFlags != 0) {
            const unsigned int segmentBytes = audioSegmentBytes;
            const unsigned int playCursor = audioSample->GetPlayCursorBytes();

            if (audioRefillSecondHalfNext != 0) {
                if (playCursor > 0 && playCursor < segmentBytes) {
                    FillAudioBuffer(
                        segmentBytes,
                        segmentBytes
                    );
                    audioRefillSecondHalfNext = 0;
                    return currentFrameIndex;
                }
            } else if (playCursor > segmentBytes) {
                FillAudioBuffer(
                    0,
                    segmentBytes
                );
                audioRefillSecondHalfNext = 1;
            }
        }
    }

    return currentFrameIndex;
}

/**
 * Reimplements 0x464540: zFMV_Stream::FillAudioBuffer
 * (D:\Proj\GameZRecoil\zFMV\fmv_stream.cpp).
 * Purpose: lock the DirectSound backing buffers and refill them from the AVI audio stream.
 */
int zFMV_Stream::FillAudioBuffer(
    unsigned int offset,
    unsigned int bytes
) {
    void *buffer1Data = 0;
    void *buffer2Data = 0;
    int buffer1Bytes = 0;
    int buffer2Bytes = 0;

    const int result = audioSample->LockBackendBuffers(
        offset,
        bytes,
        &buffer1Data,
        &buffer1Bytes,
        &buffer2Data,
        &buffer2Bytes
    );
    if (result == 0) {
        return result;
    }

    const unsigned int sampleSize = audioStreamInfo.dwSampleSize;
    unsigned int &readSampleIndex = audioReadSampleIndex;

    if (buffer1Bytes != 0) {
        if (AVIStreamRead(
                audioStream,
                readSampleIndex,
                (LONG)((unsigned int)(buffer1Bytes) / sampleSize),
                buffer1Data,
                buffer1Bytes,
                0,
                0
            ) != 0) {
            zError::ReportOld(
                0x400,
                g_zFMV_SourceFile_FmvStreamCpp,
                0x13d,
                g_zFMV_CannotReadAviSoundStreamMsg
            );
        }
        readSampleIndex += (unsigned int)(buffer1Bytes) / sampleSize;
    }

    if (buffer2Bytes != 0) {
        if (AVIStreamRead(
                audioStream,
                readSampleIndex,
                (LONG)((unsigned int)(buffer2Bytes) / sampleSize),
                buffer2Data,
                buffer2Bytes,
                0,
                0
            ) != 0) {
            zError::ReportOld(
                0x400,
                g_zFMV_SourceFile_FmvStreamCpp,
                0x144,
                g_zFMV_CannotReadAviSoundStreamMsg
            );
        }

        // The original advances by the first locked span again after the wrapped read.
        readSampleIndex += (unsigned int)(buffer1Bytes) / sampleSize;
    }

    return audioSample->UnlockBackendBuffers(
        buffer1Data,
        buffer1Bytes,
        buffer2Data,
        buffer2Bytes
    );
}

