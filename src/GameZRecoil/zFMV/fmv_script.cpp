#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <mmsystem.h>
#include <digitalv.h>
#include <vfw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
extern "C" void(__cdecl *__imp__free)(void *); // VC5 retail import-pointer call shape.
#endif

extern "C" HWND g_RecoilApp_hWndMain = 0;

// BN 0x53a728 and 0x53a708 are four-int rect-shaped state records used by
// zFMV_ActionImage constructors before copying into the action instance.
extern "C" zVidRect32 g_zFMV_ActionImage_BlitRect = {0};
extern "C" zVidRect32 g_zFMV_ActionImage_ActiveRegion = {0};
extern "C" zFMV_Rect g_zFMV_ActionPlayMci_DestRect = {0};
// BN 0x4d2580 is a single float consumed by the multimedia-timer wrappers.
extern "C" const float g_zFMV_ScriptTimeGetTimeToSecondsScale = 0.00100000005f;

namespace {
const int k_zFMV_RendererBackendSoftware = 0;
const int k_zFMV_RendererBackend3dfx = 2;
const int k_zFMV_BlurModeHorizontal = 1;
const int k_zFMV_BlurModeVertical = 2;
const int k_zFMV_BlurModeCombined = 3;
const char kFMVMainSourceFile[] = "D:\\Proj\\GameZRecoil\\zFMV\\fmv_main.cpp";
const char *kFMVStreamSourceFile = "D:\\Proj\\GameZRecoil\\zFMV\\fmv_stream.cpp";
const char kMpegVideoDeviceType[] = "MPEGVideo";
const char kUnknownMciErrorText[] = "Unknown Error ID";
const char *kCannotOpenAviFile = "Cannot Open AVI File";
const char *kCannotReadAviFormatSize = "Cannot Read AVI Format Size";
const char *kCannotReadAviFormat = "Cannot Read AVI Format";
const char *kCannotReadAviStreamInfo = "Cannot Read AVI Stream Info";
const char *kCannotReadAviSoundFormatSize = "Cannot Read AVI Sound Format Size";
const char *kCannotReadAviSoundFormat = "Cannot Read AVI Sound Format";
const char *kCannotReadAviSoundStreamInfo = "Cannot Read AVI Sound Stream Info";
const char *kCannotReadAviSoundStream = "Cannot Read AVI Sound Stream";
const char *kCannotReadAviVideoStream = "Cannot Read AVI Video Stream";
const char *kCannotDecompressAviVideoStream = "Cannot Decompress AVI Video Stream";

struct zFMV_MciWindowParams {
    DWORD_PTR callback;
    HWND hwnd;
    unsigned int commandShow;
    const char *text;
};

struct zFMV_MciRectParams {
    DWORD_PTR callback;
    int left;
    int top;
    int width;
    int height;
};

struct zFMV_MciSetParams {
    DWORD_PTR callback;
    DWORD timeFormat;
    DWORD audio;
};

struct zFMV_MciPlayParams {
    DWORD_PTR callback;
    DWORD from;
    DWORD to;
};

/**
 * Original inline helper evidence: BN 0x4631f0 copies the active-region rect
 * state through a destination rect pointer after zRndr::GetActiveRegionState.
 * Purpose: transfer the recovered active render region into an FMV blit rect.
 */
static inline void CopyActionImageActiveRegionRect(
    zVidRect32 *rect
) {
    *rect = g_zFMV_ActionImage_ActiveRegion;
}

/**
 * Observed in callers 0x462330, 0x4631af, 0x463221, 0x4635af, and 0x463b2f.
 * Purpose: duplicate an input C string through the active C runtime spelling.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
static inline char *DuplicateCString(
    const char *value
) {
#if defined(_MSC_VER)
    return _strdup(value);
#else
    return strdup(value);
#endif
}

/**
 * Observed in caller 0x4626b0.
 * Purpose: return the first node of a zReader array payload.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
zReader::Node *ArrayBase(
    zReader::Node *node
) {
    return node->value.nodes;
}

/**
 * Observed in caller 0x4626b0.
 * Purpose: return one indexed zReader array element.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
zReader::Node *ArrayItem(
    zReader::Node *node,
    int index
) {
    return &ArrayBase(node)[index];
}

/**
 * Observed in caller 0x4626b0.
 * Purpose: fetch a string argument from an FMV action node.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
const char *StringArg(
    zReader::Node *actionNode,
    int index
) {
    zReader::Node *arg = ArrayItem(
        actionNode,
        index
    );
    return arg->type == zReader::ZRDR_NODE_STRING ? arg->value.str : 0;
}

} // namespace

/**
 * Reimplements 0x415aa0: zFMV_Action::~zFMV_Action.
 * Purpose: provide the shared virtual action destructor.
 */
zFMV_Action::~zFMV_Action() {}

/**
 * Reimplements 0x4159d0: zFMV_Action::Update.
 * Purpose: report immediate completion for action types without update behavior.
 */
int zFMV_Action::Update(
    double
) {
    return 0;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_Action virtual slot contract.
 * Purpose: provide the default no-op action start hook.
 */
void zFMV_Action::Begin(double) {}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_Action virtual slot contract.
 * Purpose: provide the default no-op action finish hook.
 */
void zFMV_Action::End() {}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_Action virtual slot contract.
 * Purpose: dispatch the default timed blocking action runner.
 */
void zFMV_Action::RunBlocking() {
    RunBlockingTimed();
}

/**
 * Reimplements 0x462f00: zFMV_Action::FlipSurfaces.
 * Purpose: restore adjusted video surfaces after an FMV action completes.
 */
void zFMV_Action::FlipSurfaces() {
    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
}

/**
 * Reimplements 0x462e30: zFMV_Action::RunBlockingImmediate.
 * Purpose: run an action to completion without advancing elapsed time.
 */
void zFMV_Action::RunBlockingImmediate() {
    Begin(0.0);
    while (Update(0.0) != 0) {
    }
    End();
}

/**
 * Reimplements 0x4159e0: zFMV_Action::RunBlockingTimed.
 * Purpose: run an action to completion using elapsed milliseconds from GetTickCount.
 */
void zFMV_Action::RunBlockingTimed() {
    const double startSec = (double)(GetTickCount()) * 0.00100000005;
    Begin(0.0);
    while (true) {
        const double currentSec = ((double)(GetTickCount()) * 0.00100000005) - startSec;
        if (Update(currentSec) == 0) {
            break;
        }
    }
    End();
}

/**
 * Reimplements 0x462ed0: zFMV_ActionWait::Begin.
 * Purpose: capture the wait action start time.
 */
void zFMV_ActionWait::Begin(
    double timeSec
) {
    startSec = (float)(timeSec);
}

/**
 * Reimplements 0x462ee0: zFMV_ActionWait::Update.
 * Purpose: keep the wait action active until its duration has elapsed.
 */
int zFMV_ActionWait::Update(
    double timeSec
) {
    return timeSec < (double)(startSec + durationSec) ? 1 : 0;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_ActionWait virtual slot contract.
 * Purpose: restore FMV surfaces when a wait action completes.
 */
void zFMV_ActionWait::End() {
    FlipSurfaces();
}

/**
 * Reimplements 0x462e90: zFMV_ActionPlaySound::Begin.
 * Purpose: find and play the named FMV sound sample.
 */
void zFMV_ActionPlaySound::Begin(
    double
) {
    sample = zSnd::FindSampleByName(sampleName);
    if (voice != 0) {
        voice->StopIfActive();
    }
    if (sample != 0) {
        voice = sample->PlayA3DSimple(1.0f);
    }
}

/**
 * Reimplements 0x463c90: zFMV_ActionPlayMci::Update.
 * Purpose: report immediate completion for MCI playback update polling.
 */
int zFMV_ActionPlayMci::Update(
    double
) {
    return 0;
}

/**
 * Reimplements 0x463ca0: zFMV_ActionPlayMci::Begin.
 * Purpose: start the configured MCI playback if a playback object exists.
 */
void zFMV_ActionPlayMci::Begin(
    double
) {
    if (playback != 0) {
        playback->OpenAndPlay(
            0,
            -1,
            0
        );
    }
}

/**
 * Reimplements 0x463cc0: zFMV_ActionPlayMci::End.
 * Purpose: stop MCI playback while preserving and restoring the active video surface.
 */
void zFMV_ActionPlayMci::End() {
    zVideo::Dispatch_LockDisplayModeSurfaceState();
    zVidImagePartial *capturedImage = zVideo_buff_CaptureSurfaceToImage(2);
    zVideo::Dispatch_UnlockDisplayModeSurfaceState();

    if (capturedImage != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVid_Image::BlitToActiveTarget(
            capturedImage,
            0,
            0,
            0,
            0
        );
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            0,
            1
        );
    }

    if (playback != 0) {
        playback->StopAndClose();
    }

    if (capturedImage != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVid_Image::BlitToActiveTarget(
            capturedImage,
            0,
            0,
            0,
            0
        );
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            0,
            1
        );
        zVid_Image::ReleaseIfNotDefault(capturedImage);
    }
}

/**
 * Reimplements 0x462330: zFMV_Playback::Constructor.
 * Purpose: initialize an MCI playback object with a duplicated media path and window handle.
 */
zFMV_Playback::zFMV_Playback(
    const char *mediaPath,
    HWND hwnd
) {
    mediaPathDup = DuplicateCString(mediaPath);
    notifyHwnd = hwnd;
    mciPutFlags = 0;
}

/**
 * Reimplements 0x462360: zFMV_Playback::Destructor.
 * Purpose: release the duplicated MCI media path.
 */
void zFMV_Playback::Destructor() {
    free(mediaPathDup);
}

/**
 * Reimplements 0x462570: zFMV_Playback::ReportMciError.
 * Purpose: translate an MCI error code and report it through the old zError path.
 */
int zFMV_Playback::ReportMciError(
    unsigned int mciError
) {
    char errorText[0x80];
    if (mciGetErrorStringA(
        mciError,
        errorText,
        sizeof(errorText)
    ) == 0) {
        strcpy(
            errorText,
            kUnknownMciErrorText
        );
    }

    zError::ReportOld(
        0x200,
        kFMVMainSourceFile,
        0xc4,
        errorText
    );
    return 0;
}

/**
 * Reimplements 0x462370: zFMV_Playback::OpenAndPlay.
 * Purpose: open an MCI MPEG device, configure its window/rect/time format, and start playback.
 */
void zFMV_Playback::OpenAndPlay(
    unsigned int startMs,
    int endMs,
    int notifyFlag
) {
    zVideo_dd::FlipToGDIIfAttached();

    // Retail writes only the MCI fields consumed by each command.
    zFMV_MciPlayParams playParams;
    zFMV_MciSetParams setParams;
    zFMV_MciWindowParams windowParams;
    zFMV_MciRectParams rectParams;
    MCI_DGV_OPEN_PARMSA openParams;

    openParams.lpstrDeviceType = (LPSTR)(kMpegVideoDeviceType);
    openParams.lpstrElementName = mediaPathDup;
    DWORD mciError = mciSendCommandA(
        0,
        0x803,
        0x2202,
        (DWORD_PTR)(&openParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
    }

    mciDeviceId = (unsigned short)(openParams.wDeviceID);

    windowParams.hwnd = notifyHwnd;
    mciError = mciSendCommandA(
        mciDeviceId,
        0x841,
        0x10002,
        (DWORD_PTR)(&windowParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
        return;
    }

    if ((mciPutFlags & 0x40000) != 0) {
        rectParams.left = destinationRect.left;
        rectParams.width = destinationRect.right - destinationRect.left;
        rectParams.top = destinationRect.top;
        rectParams.height = destinationRect.bottom - destinationRect.top;
        mciError = mciSendCommandA(
            mciDeviceId,
            0x842,
            0x50002,
            (DWORD_PTR)(&rectParams)
        );
        if (mciError != 0) {
            ReportMciError(mciError);
            return;
        }
    }

    if ((mciPutFlags & 0x20000) != 0) {
        rectParams.left = sourceRect.left;
        rectParams.width = sourceRect.right - sourceRect.left;
        rectParams.top = sourceRect.top;
        rectParams.height = sourceRect.bottom - sourceRect.top;
        mciError = mciSendCommandA(
            mciDeviceId,
            0x842,
            0x30002,
            (DWORD_PTR)(&rectParams)
        );
        if (mciError != 0) {
            ReportMciError(mciError);
            return;
        }
    }

    setParams.timeFormat = 0x1b;
    setParams.audio = (DWORD)((unsigned int)(notifyHwnd));
    mciError = mciSendCommandA(
        mciDeviceId,
        0x811,
        0x302,
        (DWORD_PTR)(&setParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
        return;
    }

    DWORD playFlags = 0x6;
    playParams.callback = (DWORD_PTR)(notifyHwnd);
    playParams.from = startMs;
    if (endMs >= 0) {
        playParams.to = (DWORD)(endMs);
        playFlags = 0xe;
    }
    if (notifyFlag == 1) {
        playFlags |= 0x10000;
    }

    mciError = mciSendCommandA(
        mciDeviceId,
        0x806,
        playFlags,
        (DWORD_PTR)(&playParams)
    );
    if (mciError != 0) {
        ReportMciError(mciError);
    }
}

/**
 * Reimplements 0x4624f0: zFMV_Playback::StopAndClose.
 * Purpose: stop and close the active MCI device, reporting any failure.
 */
void zFMV_Playback::StopAndClose() {
    DWORD mciError = mciSendCommandA(
        mciDeviceId,
        0x808,
        0x2,
        0
    );
    if (mciError == 0) {
        MCI_GENERIC_PARMS closeParams;
        mciError = mciSendCommandA(
            mciDeviceId,
            0x804,
            0x2,
            (DWORD_PTR)(&closeParams)
        );
    }

    if (mciError != 0) {
        ReportMciError(mciError);
    }
}

/**
 * Reimplements 0x462540: zFMV_Playback::SetDestRect.
 * Purpose: copy the destination rectangle and mark it for the next MCI put command.
 */
int zFMV_Playback::SetDestRect(
    const zFMV_Rect *rect
) {
    destinationRect = *rect;
    const int result = mciPutFlags | 0x40000;
    mciPutFlags = result;
    return result;
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
            kFMVStreamSourceFile,
            0x60,
            kCannotOpenAviFile
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
            kFMVStreamSourceFile,
            0x67,
            kCannotReadAviFormatSize
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
            kFMVStreamSourceFile,
            0x71,
            kCannotReadAviFormat
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
            kFMVStreamSourceFile,
            0x79,
            kCannotReadAviStreamInfo
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
            kFMVStreamSourceFile,
            0xcb,
            kCannotReadAviSoundFormatSize
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
            kFMVStreamSourceFile,
            0xd2,
            kCannotReadAviSoundFormat
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
            kFMVStreamSourceFile,
            0xd8,
            kCannotReadAviSoundStreamInfo
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
                kFMVStreamSourceFile,
                0xe2,
                kCannotReadAviSoundStream
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
            kFMVStreamSourceFile,
            0xf0,
            kCannotReadAviSoundStream
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
                kFMVStreamSourceFile,
                0x105,
                kCannotReadAviVideoStream
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
                kFMVStreamSourceFile,
                0x10c,
                kCannotDecompressAviVideoStream
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
                kFMVStreamSourceFile,
                0x13d,
                kCannotReadAviSoundStream
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
                kFMVStreamSourceFile,
                0x144,
                kCannotReadAviSoundStream
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
 * Reimplements 0x4625e0: zFMV_Script::Init.
 * Purpose: initialize an FMV script object and optionally load its action sequence.
 */
zFMV_Script * zFMV_Script::Init(
    const char *zrdPath,
    const char *tagPrefix,
    HWND hWnd
) {
    m_hWnd = hWnd != 0 ? hWnd : g_RecoilApp_hWndMain;
    m_fmvPath = 0;
    m_head = 0;
    m_tail = 0;
    m_cur = 0;
    m_abortOnKey = 1;

    if (zrdPath != 0 && tagPrefix != 0) {
        LoadActionsFromZrd(
            zrdPath,
            tagPrefix
        );
    }

    return this;
}

/**
 * Reimplements 0x462630: zFMV_Script::Cleanup.
 * Purpose: free the FMV path and destroy all loaded script actions.
 */
void zFMV_Script::Cleanup() {
    if (m_fmvPath != 0) {
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
        __imp__free(m_fmvPath);
#else
        free(m_fmvPath);
#endif
        m_fmvPath = 0;
    }

    Reset(1);
}

/**
 * Reimplements 0x462660: zFMV_Script::Reset.
 * Purpose: reset the current action pointer and optionally destroy the loaded action list.
 */
void zFMV_Script::Reset(
    int destroyActions
) {
    zFMV_Action *action = m_head;
    if (destroyActions != 0) {
        while (action != 0) {
            zFMV_Action *const next = action->next;
            if (action != 0) {
                delete action;
            }
            action = next;
        }

        m_tail = 0;
        m_cur = 0;
        m_head = 0;
        return;
    }

    {
        m_cur = action;
        return;
    }
}

/**
 * Reimplements 0x4626b0: zFMV_Script::LoadActionsFromZrd.
 * Purpose: load FMV path metadata and construct actions from a named zReader sequence.
 */
int zFMV_Script::LoadActionsFromZrd(
    const char *zrdPath,
    const char *tagPrefix
) {
    zReader::Node *root = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    if (root == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\GameZRecoil\\zFMV\\fmv_script.cpp",
            0x51,
            "Failed to find FMV definitions (fmv.zrd)"
        );
        return -1;
    }

    m_fmvPath = _strdup(zReader::ReadNamedString(
        root,
        "FMV_PATH"
    ));
    zImage_InitMissionResources(zReader::ReadNamedString(
        root,
        "IMAGE_PATH"
    ));

    zReader::Node *sequenceNode = zReader_GetNamedNode(
        root,
        tagPrefix
    );
    if (sequenceNode == 0) {
        return 0;
    }

    const int sequenceActionCount = sequenceNode->value.nodes[0].value.i32;
    int i = 0;
    int actionIndex = 1;
    int result = sequenceActionCount - 1;
    if (result > 0) {
        do {
            zReader::Node *actionNode = &sequenceNode->value.nodes[actionIndex];
            if (actionNode->type != zReader::ZRDR_NODE_ARRAY) {
                result = 0;
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zFMV\\fmv_script.cpp",
                    0x69,
                    "Error in parsing fmv actions:  file=%s, tag=%s",
                    zrdPath,
                    tagPrefix
                );
                break;
            }

            const char *actionTag = actionNode->value.nodes[1].value.str;

            if (strcmp(
                    actionTag,
                    "SHOWIMAGE"
                ) == 0) {
                AppendAction(new zFMV_ActionImage(
                    actionNode->value.nodes[2].value.str,
                    1
                ));
            } else if (strcmp(
                           actionTag,
                           "BLITIMAGE"
                       ) == 0) {
                AppendAction(new zFMV_ActionImage(
                    actionNode->value.nodes[2].value.str,
                    1,
                    actionNode->value.nodes[3].value.i32,
                    actionNode->value.nodes[4].value.i32
                ));
            } else if (strcmp(
                           actionTag,
                           "LOADIMAGE"
                       ) == 0) {
                AppendAction(new zFMV_ActionImage(
                    actionNode->value.nodes[2].value.str,
                    0
                ));
            } else if (strcmp(
                           actionTag,
                           "WAIT"
                       ) == 0) {
                AppendAction(new zFMV_ActionWait(actionNode->value.nodes[2].value.f32));
            } else if (strcmp(
                           actionTag,
                           "FADEIN"
                       ) == 0) {
                zReader::Node *colorValues = actionNode->value.nodes[2].value.nodes;
                AppendAction(new zFMV_ActionFade(
                    colorValues[1].value.i32,
                    colorValues[2].value.i32,
                    colorValues[3].value.i32,
                    actionNode->value.nodes[3].value.u32,
                    -1,
                    actionNode->value.nodes[4].value.i32
                ));
            } else if (strcmp(
                           actionTag,
                           "FADEOUT"
                       ) == 0) {
                zReader::Node *colorValues = actionNode->value.nodes[2].value.nodes;
                AppendAction(new zFMV_ActionFade(
                    colorValues[1].value.i32,
                    colorValues[2].value.i32,
                    colorValues[3].value.i32,
                    actionNode->value.nodes[3].value.u32,
                    1,
                    actionNode->value.nodes[4].value.i32
                ));
            } else if (strcmp(
                           actionTag,
                           "PLAYAVI"
                       ) == 0) {
                const int actionArgCount = actionNode->value.nodes[0].value.i32;
                if (actionArgCount > 3) {
                    AppendAction(new zFMV_ActionPlayAvi(
                        m_fmvPath,
                        actionNode->value.nodes[2].value.str,
                        actionNode->value.nodes[3].value.i32
                    ));
                } else {
                    AppendAction(new zFMV_ActionPlayAvi(
                        m_fmvPath,
                        actionNode->value.nodes[2].value.str,
                        0
                    ));
                }
            } else if (strcmp(
                           actionTag,
                           "PLAYMCI"
                       ) == 0) {
                AppendAction(new zFMV_ActionPlayMci(
                    m_hWnd,
                    m_fmvPath,
                    actionNode->value.nodes[2].value.str
                ));
            } else if (strcmp(
                           actionTag,
                           "BLUR"
                       ) == 0) {
                AppendAction(new zFMV_ActionBlur(
                    1,
                    actionNode->value.nodes[2].value.i32
                ));
            } else if (strcmp(
                           actionTag,
                           "BLURH"
                       ) == 0) {
                const int blurPassCount = actionNode->value.nodes[2].value.i32;
                AppendAction(new zFMV_ActionBlurH(
                    1,
                    blurPassCount
                ));
            } else if (strcmp(
                           actionTag,
                           "BLURV"
                       ) == 0) {
                const int blurPassCount = actionNode->value.nodes[2].value.i32;
                AppendAction(new zFMV_ActionBlurV(
                    1,
                    blurPassCount
                ));
            } else if (strcmp(
                           actionTag,
                           "PLAYSOUND"
                ) == 0) {
                AppendAction(new zFMV_ActionPlaySound(actionNode->value.nodes[2].value.str));
            }
            ++i;
            ++actionIndex;
        } while (i < result);
    }

    zReader::FreeLoadedTree(root);
    return result;
}

/**
 * Reimplements 0x462f10: zFMV_Script::AppendAction.
 * Purpose: append an action to the script's singly linked action list.
 */
int zFMV_Script::AppendAction(
    zFMV_Action *action
) {
    if (action == 0) {
        return 0;
    }

    action->next = 0;
    if (m_tail == 0) {
        *(zFMV_Action *volatile *)(&m_tail) = action;
        *(zFMV_Action *volatile *)(&m_head) = action;
        *(zFMV_Action *volatile *)(&m_cur) = action;
        return 1;
    }

    m_tail->next = action;
    m_tail = action;
    return 1;
}

/**
 * Reimplements 0x462f90: zFMV_Script::BeginCurrentAction.
 * Purpose: prepare render/input/sound state and begin the current action.
 */
int zFMV_Script::BeginCurrentAction(
    double startTimeSec
) {
    if (m_cur == 0) {
        return 0;
    }

    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    zSndSampleSet_InitByName("FMV");
    zInput::Keyboard_ResetTransitionState();
    m_startTimeSec = startTimeSec;
    m_cur->Begin(0.0);
    return 1;
}

/**
 * Reimplements 0x4630a0: zFMV_Script::BeginAtTime.
 * Purpose: begin the current action using the current multimedia timer time.
 */
int zFMV_Script::BeginAtTime() {
    return BeginCurrentAction((double)(timeGetTime()) * g_zFMV_ScriptTimeGetTimeToSecondsScale);
}

/**
 * Reimplements 0x463000: zFMV_Script::Update.
 * Purpose: advance the current action, handle abort input, and start the next action.
 */
int zFMV_Script::Update(
    double timeSec
) {
    if (m_cur == 0) {
        return 0;
    }

    if (m_abortOnKey != 0) {
        if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
            m_cur->End();
            zSndPlayHandleSnapshot::CreateFromActiveSamples()->StopAllIfPlaying();
            m_cur = 0;
            return 0;
        }

        zInput::PollActiveDevices(1);
    }

    const double relativeTimeSec = timeSec - m_startTimeSec;
    if (m_cur->Update(relativeTimeSec) == 0) {
        m_cur->End();
        zFMV_Action *const next = m_cur->next;
        m_cur = next;
        if (next != 0) {
            next->Begin(relativeTimeSec);
        }
    }

    return 1;
}

/**
 * Reimplements 0x4630e0: zFMV_Script::UpdateAtTime.
 * Purpose: update the script using the current multimedia timer time.
 */
int zFMV_Script::UpdateAtTime() {
    return Update((double)(timeGetTime()) * g_zFMV_ScriptTimeGetTimeToSecondsScale);
}

/**
 * Reimplements 0x462f50: zFMV_Script::RunBlocking.
 * Purpose: run the loaded action sequence synchronously until completion.
 */
int zFMV_Script::RunBlocking(
    int abortOnKey
) {
    m_abortOnKey = abortOnKey;
    BeginAtTime();
    if (UpdateAtTime() != 0) {
        do {
        } while (UpdateAtTime() != 0);
    }

    BeginNow(0);
    return 1;
}

/**
 * Reimplements 0x463120: zFMV_Script::BeginNow.
 * Purpose: reset the script action cursor, optionally destroying loaded actions.
 */
void zFMV_Script::BeginNow(
    int destroyActions
) {
    Reset(destroyActions);
}

/**
 * Reimplements 0x463130: zFMV_ActionImage::ConstructorWithScreenRect.
 * Purpose: initialize an image action with an explicit screen blit origin.
 */
zFMV_ActionImage::zFMV_ActionImage(
    const char *path,
    int adjustSurfaces,
    int blitX,
    int blitY
) {
    image = 0;
#if defined(_MSC_VER)
    imagePath = _strdup(path);
#else
    imagePath = strdup(path);
#endif
    doAdjustSurfaces = adjustSurfaces;
    g_zFMV_ActionImage_BlitRect.top = blitY;
    g_zFMV_ActionImage_BlitRect.left = blitX;
    forcePrimaryPostprocess = 1;
    blitRect = g_zFMV_ActionImage_BlitRect;
}

/**
 * Reimplements 0x4631f0: zFMV_ActionImage::ConstructorScaled.
 * Purpose: initialize an image action sized to the active render region.
 */
zFMV_ActionImage::zFMV_ActionImage(
    const char *path,
    int adjustSurfaces
) {
    image = 0;
#if defined(_MSC_VER)
    imagePath = _strdup(path);
#else
    imagePath = strdup(path);
#endif
    doAdjustSurfaces = adjustSurfaces;
    g_zFMV_ActionImage_ActiveRegion.top = 0;
    g_zFMV_ActionImage_ActiveRegion.left = 0;
    forcePrimaryPostprocess = 0;

    int discard;
    zRndr::GetActiveRegionState(
        &g_zFMV_ActionImage_ActiveRegion.right,
        &g_zFMV_ActionImage_ActiveRegion.bottom,
        &discard,
        &discard
    );

    CopyActionImageActiveRegionRect(&blitRect);
}

/**
 * Reimplements 0x463300: zFMV_ActionImage::Begin.
 * Purpose: resolve the image resource used by this FMV image action.
 */
void zFMV_ActionImage::Begin(double) {
    image = zImage::TexDir_FindOrCreateByPath(imagePath);
}

/**
 * Reimplements 0x463320: zFMV_ActionImage::Update.
 * Purpose: blit the resolved image through the active renderer path and finish immediately.
 */
int zFMV_ActionImage::Update(double) {
    int iterations =
        g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware ? 2 : 1;

    if (image != 0) {
        do {
            if (forcePrimaryPostprocess != 0 ||
                g_zVideo_ActiveRendererPath == k_zFMV_RendererBackend3dfx) {
                zVideo::RunPostprocessOnPrimaryBuffer();
                zVid_Image::BlitToActiveTarget(
                    (zVidImagePartial *)(image),
                    blitRect.left,
                    blitRect.top,
                    0,
                    0
                );
                zVideo::Dispatch_UnlockPrimarySurfaceState();
            } else {
                g_zVideo_pfnBltSwToPrimaryRect(
                    (zVidImagePartial *)(image),
                    0,
                    0,
                    &blitRect
                );
            }

            if (doAdjustSurfaces != 0) {
                zVideo::AdjustSurfacesIfEnabled(
                    0,
                    0,
                    1,
                    1
                );
            }
            --iterations;
        } while (iterations != 0);
    }

    return 0;
}

/**
 * Reimplements 0x4633a0: zFMV_ActionImage::End.
 * Purpose: release the resolved image resource.
 */
void zFMV_ActionImage::End() {
    if (image != 0) {
        zVid_Image::ReleaseIfNotDefault((zVidImagePartial *)(image));
        image = 0;
    }
}

/**
 * Reimplements 0x4632a0: zFMV_ActionImage::~zFMV_ActionImage.
 * Purpose: end image playback and free the image path.
 */
zFMV_ActionImage::~zFMV_ActionImage() {
    End();
    if (imagePath != 0) {
        free(imagePath);
        imagePath = 0;
    }
}

/**
 * Reimplements 0x4633c0: zFMV_ActionFade::Constructor.
 * Purpose: initialize fade color, duration, direction, and alpha settings.
 */
zFMV_ActionFade::zFMV_ActionFade(
    int red,
    int green,
    int blue,
    unsigned int duration,
    int direction,
    int alpha
) {
    fadeColorPacked16 = (unsigned short)(zVid_PackColorRGB(
        red,
        green,
        blue
    ));
    durationSecRaw = duration;
    fadeDirectionSign = direction;
    maxAlpha = alpha;
}

/**
 * Reimplements 0x463410: zFMV_ActionFade::Begin.
 * Purpose: capture the current surface and record the fade start time.
 */
void zFMV_ActionFade::Begin(double timeSec) {
    capturedFrame = zVideo_buff_CaptureSurfaceToImage(1);
    startSec = timeSec;
}

/**
 * Reimplements 0x463440: zFMV_ActionFade::Update.
 * Purpose: composite the captured frame with a timed fade overlay.
 */
int zFMV_ActionFade::Update(double timeSec) {
    if (capturedFrame == 0) {
        return 0;
    }

    double fadeProgress = (timeSec - startSec) / *(float *)&durationSecRaw;
    int result = 1;
    if (fadeDirectionSign < 0) {
        fadeProgress = 1.0 - fadeProgress;
        if (fadeProgress <= 0.0) {
            fadeProgress = 0.0;
            result = 0;
        }
    } else {
        if (fadeProgress > 1.0) {
            fadeProgress = 1.0;
            result = 0;
        }
    }

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnSwBuffer();
    } else {
        zVideo::RunPostprocessOnPrimaryBuffer();
    }

    zVid_Image::BlitToActiveTarget(
        (zVidImagePartial *)(capturedFrame),
        0,
        0,
        0,
        0
    );

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::Dispatch_UnlockSwSurfaceState();
    }

    zRndr_OverlayRect_Submit(
        fadeColorPacked16,
        0,
        (double)(maxAlpha) * fadeProgress
    );

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideoD3D::SceneEnter();
        g_zVideo_pfnFlushQuadBatch();
        zVideoD3D::SceneLeave();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            0,
            0
        );
    } else {
        zRndr_OverlayRect_FlushSw();
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            1,
            1
        );
    }

    return result;
}

/**
 * Reimplements 0x463550: zFMV_ActionFade::End.
 * Purpose: release the captured fade frame.
 */
void zFMV_ActionFade::End() {
    if (capturedFrame != 0) {
        zVid_Image::ReleaseIfNotDefault((zVidImagePartial *)(capturedFrame));
        capturedFrame = 0;
    }
}

/**
 * Reimplements 0x463570: zFMV_ActionPlayAvi::Constructor.
 * Purpose: build the AVI media path, resolve CD-ROM fallback, and store mode flags.
 */
zFMV_ActionPlayAvi::zFMV_ActionPlayAvi(
    const char *mediaRootPath,
    const char *mediaFileName,
    int flags
) {
    mediaPath = (char *)(calloc(
        strlen(mediaRootPath) + strlen(mediaFileName) + 0x1b,
        1
    ));
    sprintf(
        mediaPath,
        "%s\\%s",
        mediaRootPath,
        mediaFileName
    );
    modeFlags = flags;

    struct stat statBuffer;
    if (stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        char *resolvedPath = zSys::FindFileOnDriveType(
            DRIVE_CDROM,
            mediaPath,
            0
        );
        if (resolvedPath != 0) {
            strcpy(
                mediaPath,
                resolvedPath
            );
        }
    }
}

/**
 * Reimplements 0x463670: zFMV_ActionPlayAvi::~zFMV_ActionPlayAvi.
 * Purpose: free the AVI media path.
 */
zFMV_ActionPlayAvi::~zFMV_ActionPlayAvi() {
    if (mediaPath != 0) {
        free(mediaPath);
        mediaPath = 0;
    }
}

/**
 * Reimplements 0x4636d0: zFMV_ActionPlayAvi::Update.
 * Purpose: advance AVI frame playback, blit the decoded frame, and update surfaces.
 */
int zFMV_ActionPlayAvi::Update(
    double timeSec
) {
    int result = 1;
    const int previousFrameIndex = lastDecodedFrameIndex;
    if (previousFrameIndex < 0) {
        startTimeSec = timeSec;
    }

    zFMV_Stream *const playbackStream = stream;
    const int frameIndex =
        (int)((timeSec - startTimeSec) * (double)(playbackStream->videoFramesPerSecond));
    if (frameIndex != previousFrameIndex) {
        int blitPrimaryToSwFirst = 0;
        if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackend3dfx) {
            zVideo::RunPostprocessOnPrimaryBuffer();
            result = playbackStream->ReadAndDecodeFrame(frameIndex);
            zVideo::Dispatch_UnlockPrimarySurfaceState();
            g_zVideo_pfnBltSwToPrimaryRect(
                (zVidImagePartial *)(stream),
                0,
                0,
                (zVidRect32 *)(&destRect)
            );
            blitPrimaryToSwFirst = 1;
        } else {
            result = playbackStream->ReadAndDecodeFrame(frameIndex);
            g_zVideo_pfnBltSwToPrimaryRect(
                (zVidImagePartial *)(stream),
                0,
                0,
                (zVidRect32 *)(&destRect)
            );
        }

        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            1,
            blitPrimaryToSwFirst
        );
        lastDecodedFrameIndex = frameIndex;
    }

    return result;
}

/**
 * Reimplements 0x463790: zFMV_ActionPlayAvi::Begin.
 * Purpose: allocate and initialize the AVI stream and active destination rectangle.
 */
void zFMV_ActionPlayAvi::Begin(
    double
) {
    zFMV_Stream *const streamStorage = (zFMV_Stream *)(::operator new(sizeof(zFMV_Stream)));
    zFMV_Stream *initializedStream = 0;
    if (streamStorage != 0) {
        initializedStream = streamStorage->Init(
            mediaPath,
            modeFlags
        );
    }
    stream = initializedStream;

    destRect.top = 0;
    destRect.left = 0;
    int discard = 0;
    zRndr::GetActiveRegionState(
        &destRect.right,
        &destRect.bottom,
        &discard,
        &discard
    );
    lastDecodedFrameIndex = -1;
}

/**
 * Reimplements 0x463820: zFMV_ActionPlayAvi::End.
 * Purpose: destroy the AVI stream object and clear the stream pointer.
 */
void zFMV_ActionPlayAvi::End() {
    zFMV_Stream *const playbackStream = stream;
    if (playbackStream != 0) {
        playbackStream->Destructor();
        ::operator delete(playbackStream);
    }
    stream = 0;
}

/**
 * Reimplements 0x463b00: zFMV_ActionPlayMci::Constructor.
 * Purpose: build the MCI media path, create playback state, and set its destination rect.
 */
zFMV_ActionPlayMci::zFMV_ActionPlayMci(
    HWND hwnd,
    const char *mediaRootPath,
    const char *playbackTitle
) {
    mediaPath = (char *)(calloc(
        strlen(mediaRootPath) + strlen(playbackTitle) + 0x1b,
        1
    ));
    sprintf(
        mediaPath,
        "%s\\%s",
        mediaRootPath,
        playbackTitle
    );

    zFMV_Playback *const playbackObject = new zFMV_Playback(
        mediaPath,
        hwnd
    );
    playback = playbackObject;

    g_zFMV_ActionPlayMci_DestRect.top = 0;
    g_zFMV_ActionPlayMci_DestRect.left = 0;
    int discard;
    zRndr::GetActiveRegionState(
        &g_zFMV_ActionPlayMci_DestRect.right,
        &g_zFMV_ActionPlayMci_DestRect.bottom,
        &discard,
        &discard
    );
    playback->SetDestRect(&g_zFMV_ActionPlayMci_DestRect);
}

/**
 * Reimplements 0x463c10: zFMV_ActionPlayMci::~zFMV_ActionPlayMci.
 * Purpose: free MCI media/playback state.
 */
zFMV_ActionPlayMci::~zFMV_ActionPlayMci() {
    if (mediaPath != 0) {
        free(mediaPath);
        mediaPath = 0;
    }

    zFMV_Playback *const playbackObject = playback;
    if (playbackObject != 0) {
        playbackObject->Destructor();
        ::operator delete(playbackObject);
        playback = 0;
    }
}

/**
 * Reimplements 0x463850: zFMV_ActionBlur::Constructor.
 * Purpose: initialize a blur action's frame count and pass count.
 */
zFMV_ActionBlur::zFMV_ActionBlur(
    int framesRemainingParam,
    int blurPassCountParam
) {
    framesRemaining = framesRemainingParam;
    blurPassCount = blurPassCountParam;
}

/**
 * Reimplements 0x463870: zFMV_ActionBlur::Begin.
 * Purpose: capture active surface bounds and seed the blur source surface.
 */
void zFMV_ActionBlur::Begin(
    double
) {
    primarySurfaceRect.top = 0;
    swSurfaceRect.top = 0;
    primarySurfaceRect.left = 0;
    swSurfaceRect.left = 0;
    swSurfaceRect.right = zVideo::GetSwSurfaceWidth();
    swSurfaceRect.bottom = zVideo::GetSwSurfaceHeight();
    primarySurfaceRect.right = zVideo::GetPrimarySurfaceWidth();
    primarySurfaceRect.bottom = zVideo::GetPrimarySurfaceHeight();

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::Fx_SetSurfaceState(
            zVideo::GetPrimarySurfacePixels(),
            swSurfaceRect.right,
            swSurfaceRect.bottom,
            zVideo::GetPrimarySurfacePitch()
        );
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&primarySurfaceRect),
            (zVidRect32 *)(&swSurfaceRect)
        );
    } else {
        zVideo::Fx_SetSurfaceState(
            zVideo::GetSwSurfacePixels(),
            swSurfaceRect.right,
            swSurfaceRect.bottom,
            zVideo::GetSwSurfacePitch()
        );
        g_zVideo_pfnBltPrimaryToSwRectDirect(
            (zVidRect32 *)(&primarySurfaceRect),
            (zVidRect32 *)(&swSurfaceRect)
        );
    }
}

/**
 * Reimplements 0x463920: zFMV_ActionBlur::End.
 * Purpose: restore the video FX surface state to the primary surface.
 */
void zFMV_ActionBlur::End() {
    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_ActionBlur virtual slot contract.
 * Purpose: run blur actions immediately without timed polling.
 */
void zFMV_ActionBlur::RunBlocking() {
    RunBlockingImmediate();
}

/**
 * Reimplements 0x463950: zFMV_ActionBlur::Update.
 * Purpose: apply combined blur passes for one frame and report whether frames remain.
 */
int zFMV_ActionBlur::Update(
    double
) {
    --framesRemaining;
    int passes = blurPassCount;

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeCombined
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnSwBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeCombined
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&swSurfaceRect),
            (zVidRect32 *)(&primarySurfaceRect)
        );
    }

    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    return framesRemaining != 0;
}

/**
 * Reimplements 0x4639e0: zFMV_ActionBlurH::Update.
 * Purpose: apply horizontal blur passes for one frame and report whether frames remain.
 */
int zFMV_ActionBlurH::Update(
    double
) {
    --framesRemaining;
    int passes = blurPassCount;

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeHorizontal
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnSwBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeHorizontal
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&swSurfaceRect),
            (zVidRect32 *)(&primarySurfaceRect)
        );
    }

    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    return framesRemaining != 0;
}

/**
 * Reimplements 0x463a70: zFMV_ActionBlurV::Update.
 * Purpose: apply vertical blur passes for one frame and report whether frames remain.
 */
int zFMV_ActionBlurV::Update(
    double
) {
    --framesRemaining;
    int passes = blurPassCount;

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeVertical
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnSwBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeVertical
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&swSurfaceRect),
            (zVidRect32 *)(&primarySurfaceRect)
        );
    }

    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    return framesRemaining != 0;
}
