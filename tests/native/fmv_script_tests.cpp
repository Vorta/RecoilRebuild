#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vfw.h>

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" std::int32_t g_zFMV_ActionImage_BlitRectW;
extern "C" std::int32_t g_zFMV_ActionImage_BlitRectH;
extern "C" int g_zFMV_ActionImage_BlitRectX;
extern "C" int g_zFMV_ActionImage_BlitRectY;

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

int g_deletedCount;
std::uint32_t g_lastDeleteFlags;
int g_beginCallCount;
int g_updateCallCount;
int g_endCallCount;
double g_lastBeginTimeSec;
double g_lastUpdateTimeSec;
int g_nextUpdateResult;
int g_fakeFmvMciSendCommandCount;
MCIDEVICEID g_fakeFmvMciDevices[8];
UINT g_fakeFmvMciMessages[8];
DWORD_PTR g_fakeFmvMciFlags[8];
DWORD_PTR g_fakeFmvMciParams[8];
MCIERROR g_fakeFmvMciReturns[8];
zFMV_Playback *g_fakeFmvExpectedClosePlayback;
int g_fakeFmvCloseParamOk;
const char *g_fakeFmvOpenDeviceType;
const char *g_fakeFmvOpenElementName;
HWND g_fakeFmvWindowHwnd;
int g_fakeFmvDestRect[4];
int g_fakeFmvSourceRect[4];
DWORD g_fakeFmvSetTimeFormat;
DWORD g_fakeFmvSetAudio;
DWORD g_fakeFmvPlayCallback;
DWORD g_fakeFmvPlayFrom;
DWORD g_fakeFmvPlayTo;
int g_fakeAviStreamReadCount;
PAVISTREAM g_fakeAviStreams[4];
LONG g_fakeAviStarts[4];
LONG g_fakeAviSamples[4];
void *g_fakeAviBuffers[4];
LONG g_fakeAviBufferBytes[4];
HRESULT g_fakeAviReturn;
int g_fakeIcDecompressCount;
HIC g_fakeIcLastCodec;
DWORD g_fakeIcLastFlags;
BITMAPINFOHEADER *g_fakeIcLastSrcFormat;
void *g_fakeIcLastCompressedFrame;
BITMAPINFOHEADER *g_fakeIcLastDstFormat;
void *g_fakeIcLastPixels;
LRESULT g_fakeIcReturn;
int g_fakeFmvOperatorDeleteCount;
void *g_fakeFmvOperatorDeletePtr;
int g_fakeFmvDisplayLockCount;
int g_fakeFmvDisplayUnlockCount;
int g_fakeFmvPlaybackOpenAndPlayCount;
zFMV_Playback *g_fakeFmvPlaybackOpenAndPlaySelf;
unsigned int g_fakeFmvPlaybackOpenAndPlayStartMs;
int g_fakeFmvPlaybackOpenAndPlayEndMs;
int g_fakeFmvPlaybackOpenAndPlayNotifyFlag;
int g_fakeFmvPlaybackStopAndCloseCount;
zFMV_Playback *g_fakeFmvPlaybackStopAndCloseSelf;
int g_fakeFmvAdjustSurfacesCount;
zVidRect32 *g_fakeFmvAdjustSurfacesSrcRect;
zVidRect32 *g_fakeFmvAdjustSurfacesDstRect;
int g_fakeFmvAdjustSurfacesWaitForPresent;
int g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst;
int g_fakeFmvTexDirFindCount;
const char *g_fakeFmvTexDirFindPath;
zVidImagePartial *g_fakeFmvTexDirFindResult;
int g_fakeFmvPostprocessCount;
int g_fakeFmvSwPostprocessCount;
int g_fakeFmvBlitToActiveTargetCount;
zVidImagePartial *g_fakeFmvBlitToActiveTargetImage;
int g_fakeFmvBlitToActiveTargetDstX;
int g_fakeFmvBlitToActiveTargetDstY;
int g_fakeFmvBlitToActiveTargetClipFlags;
zVidRect32 *g_fakeFmvBlitToActiveTargetSrcRect;
int g_fakeFmvUnlockPrimaryCount;
int g_fakeFmvUnlockSwCount;
int g_fakeFmvOverlaySubmitCount;
unsigned int g_fakeFmvOverlaySubmitColor;
zVidRect32 *g_fakeFmvOverlaySubmitRect;
double g_fakeFmvOverlaySubmitAlpha;
int g_fakeFmvOverlayFlushSwCount;
int g_fakeFmvSceneEnterCount;
int g_fakeFmvSceneLeaveCount;
int g_fakeFmvFlushQuadBatchCount;
int g_fakeFmvSwBltCount;
zVidImagePartial *g_fakeFmvSwBltImage;
int g_fakeFmvSwBltColorKeyEnable;
zVidRect32 *g_fakeFmvSwBltSrcRect;
zVidRect32 *g_fakeFmvSwBltDstRect;
int g_fakeFmvSwToPrimaryDirectCount;
zVidRect32 *g_fakeFmvSwToPrimaryDirectSrcRect;
zVidRect32 *g_fakeFmvSwToPrimaryDirectDstRect;
int g_fakeFmvPrimaryToSwDirectCount;
zVidRect32 *g_fakeFmvPrimaryToSwDirectSrcRect;
zVidRect32 *g_fakeFmvPrimaryToSwDirectDstRect;
int g_fakeFmvBlurByModeCount;
zVidRect32 *g_fakeFmvBlurByModeRect;
int g_fakeFmvBlurByModeModes[8];
int g_fakeFmvCaptureSurfaceCount;
int g_fakeFmvCaptureSurfaceSelector;
zVidImagePartial *g_fakeFmvCaptureSurfaceResult;
int g_fakeFmvReleaseImageCount;
zVidImagePartial *g_fakeFmvReleaseImage;
int g_fakeFmvFindFileCount;
int g_fakeFmvFindFileDriveType;
char g_fakeFmvFindFileRelativePath[128];
int g_fakeFmvFindFileUnused;
char *g_fakeFmvFindFileResult;
int g_fakeFmvStreamOpenCount;
PAVISTREAM g_fakeFmvOpenedStream;
const char *g_fakeFmvOpenedPath;
DWORD g_fakeFmvOpenedType;
LONG g_fakeFmvOpenedParam;
UINT g_fakeFmvOpenedMode;
int g_fakeFmvReadFormatCount;
PAVISTREAM g_fakeFmvReadFormatStream;
LONG g_fakeFmvReadFormatPosition;
int g_fakeFmvStreamLengthCount;
PAVISTREAM g_fakeFmvStreamLengthStream;
int g_fakeFmvStreamInfoCount;
PAVISTREAM g_fakeFmvStreamInfoStream;
LONG g_fakeFmvStreamInfoSize;
int g_fakeFmvIcLocateCount;
DWORD g_fakeFmvIcLocateType;
DWORD g_fakeFmvIcLocateHandler;
void *g_fakeFmvIcLocateSrcFormat;
void *g_fakeFmvIcLocateDstFormat;
WORD g_fakeFmvIcLocateMode;
HIC g_fakeFmvLocatedCodec;
int g_fakeFmvIcSendMessageCount;
HIC g_fakeFmvIcSendCodec;
UINT g_fakeFmvIcSendMessage;
DWORD_PTR g_fakeFmvIcSendSrcParam;
DWORD_PTR g_fakeFmvIcSendDstParam;
int g_fakeFmvLockCount;
int g_fakeFmvUnlockCount;
int g_fakeFmvGetCurrentPositionCount;
unsigned int g_fakeFmvLastLockOffset;
unsigned int g_fakeFmvLastLockBytes;
unsigned int g_fakeFmvLastLockFlags;
int g_fakeFmvLockResult;
int g_fakeFmvUnlockResult;
void *g_fakeFmvLockPtr1;
void *g_fakeFmvLockPtr2;
int g_fakeFmvLockBytes1;
int g_fakeFmvLockBytes2;
std::uint32_t g_fakeFmvPlayCursor;
void *g_fakeFmvUnlockPtr1;
void *g_fakeFmvUnlockPtr2;
int g_fakeFmvUnlockBytes1;
int g_fakeFmvUnlockBytes2;

struct TestFmvMciOpenParams {
    DWORD_PTR callback;
    unsigned int deviceId;
    const char *deviceType;
    const char *elementName;
    const char *alias;
};

struct TestFmvMciWindowParams {
    DWORD_PTR callback;
    HWND hwnd;
};

struct TestFmvMciRectParams {
    DWORD_PTR callback;
    int left;
    int top;
    int width;
    int height;
};

struct TestFmvMciSetParams {
    DWORD_PTR callback;
    DWORD timeFormat;
    DWORD audio;
};

struct TestFmvMciPlayParams {
    DWORD_PTR callback;
    DWORD from;
    DWORD to;
};

MCIERROR WINAPI FakeFmvMciSendCommandA(MCIDEVICEID deviceId, UINT message, DWORD_PTR flags,
                                       DWORD_PTR params) {
    const int index = g_fakeFmvMciSendCommandCount;
    if (index < 8) {
        g_fakeFmvMciDevices[index] = deviceId;
        g_fakeFmvMciMessages[index] = message;
        g_fakeFmvMciFlags[index] = flags;
        g_fakeFmvMciParams[index] = params;
    }
    ++g_fakeFmvMciSendCommandCount;

    if (message == 0x803 && params != 0) {
        TestFmvMciOpenParams *const openParams = reinterpret_cast<TestFmvMciOpenParams *>(params);
        g_fakeFmvOpenDeviceType = openParams->deviceType;
        g_fakeFmvOpenElementName = openParams->elementName;
        openParams->deviceId = 0x3456;
    }

    if (message == 0x841 && params != 0) {
        TestFmvMciWindowParams *const windowParams =
            reinterpret_cast<TestFmvMciWindowParams *>(params);
        g_fakeFmvWindowHwnd = windowParams->hwnd;
    }

    if (message == 0x842 && params != 0) {
        TestFmvMciRectParams *const rectParams = reinterpret_cast<TestFmvMciRectParams *>(params);
        int *const capturedRect = flags == 0x50002 ? g_fakeFmvDestRect : g_fakeFmvSourceRect;
        capturedRect[0] = rectParams->left;
        capturedRect[1] = rectParams->top;
        capturedRect[2] = rectParams->width;
        capturedRect[3] = rectParams->height;
    }

    if (message == 0x811 && params != 0) {
        TestFmvMciSetParams *const setParams = reinterpret_cast<TestFmvMciSetParams *>(params);
        g_fakeFmvSetTimeFormat = setParams->timeFormat;
        g_fakeFmvSetAudio = setParams->audio;
    }

    if (message == 0x806 && params != 0) {
        TestFmvMciPlayParams *const playParams = reinterpret_cast<TestFmvMciPlayParams *>(params);
        g_fakeFmvPlayCallback = playParams->callback;
        g_fakeFmvPlayFrom = playParams->from;
        g_fakeFmvPlayTo = playParams->to;
    }

    if (message == 0x804 && params != 0) {
        zFMV_Playback *const closePlayback =
            *reinterpret_cast<zFMV_Playback *const *>(params);
        g_fakeFmvCloseParamOk =
            closePlayback == g_fakeFmvExpectedClosePlayback ? 1 : 0;
    }

    return index < 8 ? g_fakeFmvMciReturns[index] : 0;
}

HRESULT WINAPI FakeFmvAVIStreamRead(PAVISTREAM stream, LONG start, LONG samples, void *buffer,
                                    LONG bufferBytes, LONG *, LONG *) {
    const int index = g_fakeAviStreamReadCount;
    if (index < 4) {
        g_fakeAviStreams[index] = stream;
        g_fakeAviStarts[index] = start;
        g_fakeAviSamples[index] = samples;
        g_fakeAviBuffers[index] = buffer;
        g_fakeAviBufferBytes[index] = bufferBytes;
    }
    ++g_fakeAviStreamReadCount;
    return g_fakeAviReturn;
}

LRESULT __cdecl FakeFmvICDecompress(HIC codec, DWORD flags, BITMAPINFOHEADER *srcFormat,
                                     void *compressedFrame, BITMAPINFOHEADER *dstFormat,
                                     void *pixels) {
    ++g_fakeIcDecompressCount;
    g_fakeIcLastCodec = codec;
    g_fakeIcLastFlags = flags;
    g_fakeIcLastSrcFormat = srcFormat;
    g_fakeIcLastCompressedFrame = compressedFrame;
    g_fakeIcLastDstFormat = dstFormat;
    g_fakeIcLastPixels = pixels;
    return g_fakeIcReturn;
}

void __cdecl FakeFmvOperatorDelete(void *ptr) {
    ++g_fakeFmvOperatorDeleteCount;
    g_fakeFmvOperatorDeletePtr = ptr;
}

int FakeFmvDispatchLockDisplayModeSurfaceState() {
    ++g_fakeFmvDisplayLockCount;
    return 1;
}

int FakeFmvDispatchUnlockDisplayModeSurfaceState() {
    ++g_fakeFmvDisplayUnlockCount;
    return 1;
}

int __fastcall FakeFmvAdjustSurfacesIfEnabled(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
) {
    ++g_fakeFmvAdjustSurfacesCount;
    g_fakeFmvAdjustSurfacesSrcRect = srcRect;
    g_fakeFmvAdjustSurfacesDstRect = dstRect;
    g_fakeFmvAdjustSurfacesWaitForPresent = waitForPresent;
    g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst = blitPrimaryToSwFirst;
    return 0x1234;
}

zVidImagePartial *__fastcall FakeFmvTexDirFindOrCreateByPath(const char *path) {
    ++g_fakeFmvTexDirFindCount;
    g_fakeFmvTexDirFindPath = path;
    return g_fakeFmvTexDirFindResult;
}

int FakeFmvRunPostprocessOnPrimaryBuffer() {
    ++g_fakeFmvPostprocessCount;
    return 1;
}

void FakeFmvRunPostprocessOnSwBuffer() {
    ++g_fakeFmvSwPostprocessCount;
}

void __fastcall FakeFmvBlitToActiveTarget(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    ++g_fakeFmvBlitToActiveTargetCount;
    g_fakeFmvBlitToActiveTargetImage = image;
    g_fakeFmvBlitToActiveTargetDstX = dstX;
    g_fakeFmvBlitToActiveTargetDstY = dstY;
    g_fakeFmvBlitToActiveTargetClipFlags = clipFlags;
    g_fakeFmvBlitToActiveTargetSrcRect = srcRect;
}

int FakeFmvDispatchUnlockPrimarySurfaceState() {
    ++g_fakeFmvUnlockPrimaryCount;
    return 1;
}

int FakeFmvDispatchUnlockSwSurfaceState() {
    ++g_fakeFmvUnlockSwCount;
    return 1;
}

void __fastcall FakeFmvOverlayRectSubmit(
    unsigned int packedColor16,
    zVidRect32 *rectOrNull,
    double alpha
) {
    ++g_fakeFmvOverlaySubmitCount;
    g_fakeFmvOverlaySubmitColor = packedColor16;
    g_fakeFmvOverlaySubmitRect = rectOrNull;
    g_fakeFmvOverlaySubmitAlpha = alpha;
}

void FakeFmvOverlayRectFlushSw() {
    ++g_fakeFmvOverlayFlushSwCount;
}

int FakeFmvSceneEnter() {
    ++g_fakeFmvSceneEnterCount;
    return 1;
}

int FakeFmvSceneLeave() {
    ++g_fakeFmvSceneLeaveCount;
    return 1;
}

void FakeFmvFlushQuadBatch() {
    ++g_fakeFmvFlushQuadBatchCount;
}

void __fastcall FakeFmvBltSwToPrimaryRect(
    zVidImagePartial *srcImage,
    int srcColorKeyEnable,
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    ++g_fakeFmvSwBltCount;
    g_fakeFmvSwBltImage = srcImage;
    g_fakeFmvSwBltColorKeyEnable = srcColorKeyEnable;
    g_fakeFmvSwBltSrcRect = srcRect;
    g_fakeFmvSwBltDstRect = dstRect;
}

void __fastcall FakeFmvBltSwToPrimaryRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    ++g_fakeFmvSwToPrimaryDirectCount;
    g_fakeFmvSwToPrimaryDirectSrcRect = srcRect;
    g_fakeFmvSwToPrimaryDirectDstRect = dstRect;
}

void __fastcall FakeFmvBltPrimaryToSwRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    ++g_fakeFmvPrimaryToSwDirectCount;
    g_fakeFmvPrimaryToSwDirectSrcRect = srcRect;
    g_fakeFmvPrimaryToSwDirectDstRect = dstRect;
}

void __fastcall FakeFmvBlurRegionByMode(
    zVidRect32 *rect,
    int mode
) {
    const int index = g_fakeFmvBlurByModeCount;
    if (index < 8) {
        g_fakeFmvBlurByModeModes[index] = mode;
    }
    ++g_fakeFmvBlurByModeCount;
    g_fakeFmvBlurByModeRect = rect;
}

zVidImagePartial *__fastcall FakeFmvCaptureSurfaceToImage(int selector) {
    ++g_fakeFmvCaptureSurfaceCount;
    g_fakeFmvCaptureSurfaceSelector = selector;
    return g_fakeFmvCaptureSurfaceResult;
}

unsigned int FmvFloatBits(float value) {
    unsigned int bits;
    std::memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    return bits;
}

int __fastcall FakeFmvReleaseImageIfNotDefault(zVidImagePartial *image) {
    ++g_fakeFmvReleaseImageCount;
    g_fakeFmvReleaseImage = image;
    return 1;
}

char *__fastcall FakeFmvFindFileOnDriveType(
    int driveType,
    const char *relativePath,
    int unused
) {
    ++g_fakeFmvFindFileCount;
    g_fakeFmvFindFileDriveType = driveType;
    g_fakeFmvFindFileUnused = unused;
    std::strncpy(
        g_fakeFmvFindFileRelativePath,
        relativePath,
        sizeof(g_fakeFmvFindFileRelativePath) - 1
    );
    g_fakeFmvFindFileRelativePath[sizeof(g_fakeFmvFindFileRelativePath) - 1] = '\0';
    return g_fakeFmvFindFileResult;
}

HRESULT WINAPI FakeFmvAVIStreamOpenFromFileA(PAVISTREAM *stream, LPCSTR path, DWORD type,
                                             LONG param, UINT mode, CLSID *) {
    ++g_fakeFmvStreamOpenCount;
    g_fakeFmvOpenedPath = path;
    g_fakeFmvOpenedType = type;
    g_fakeFmvOpenedParam = param;
    g_fakeFmvOpenedMode = mode;
    *stream = g_fakeFmvOpenedStream;
    return 0;
}

HRESULT WINAPI FakeFmvAVIStreamReadFormat(PAVISTREAM stream, LONG position, void *format,
                                          LONG *formatBytes) {
    ++g_fakeFmvReadFormatCount;
    g_fakeFmvReadFormatStream = stream;
    g_fakeFmvReadFormatPosition = position;
    if (formatBytes != nullptr) {
        *formatBytes = sizeof(BITMAPINFOHEADER);
    }
    if (format != nullptr) {
        BITMAPINFOHEADER *const header = reinterpret_cast<BITMAPINFOHEADER *>(format);
        std::memset(header, 0, sizeof(*header));
        header->biSize = sizeof(BITMAPINFOHEADER);
        header->biWidth = 5;
        header->biHeight = 4;
        header->biPlanes = 1;
        header->biBitCount = 16;
    }
    return 0;
}

LONG WINAPI FakeFmvAVIStreamLength(PAVISTREAM stream) {
    ++g_fakeFmvStreamLengthCount;
    g_fakeFmvStreamLengthStream = stream;
    return 9;
}

HRESULT WINAPI FakeFmvAVIStreamInfoA(PAVISTREAM stream, AVISTREAMINFOA *info, LONG size) {
    ++g_fakeFmvStreamInfoCount;
    g_fakeFmvStreamInfoStream = stream;
    g_fakeFmvStreamInfoSize = size;
    std::memset(info, 0, sizeof(*info));
    info->fccHandler = mmioFOURCC('T', 'E', 'S', 'T');
    info->dwScale = 1;
    info->dwRate = 30;
    info->dwSuggestedBufferSize = 48;
    return 0;
}

HIC WINAPI FakeFmvICLocate(DWORD type, DWORD handler, LPBITMAPINFOHEADER srcFormat,
                           LPBITMAPINFOHEADER dstFormat, WORD mode) {
    ++g_fakeFmvIcLocateCount;
    g_fakeFmvIcLocateType = type;
    g_fakeFmvIcLocateHandler = handler;
    g_fakeFmvIcLocateSrcFormat = srcFormat;
    g_fakeFmvIcLocateDstFormat = dstFormat;
    g_fakeFmvIcLocateMode = mode;
    return g_fakeFmvLocatedCodec;
}

LRESULT WINAPI FakeFmvICSendMessage(HIC codec, UINT message, DWORD_PTR srcParam,
                                    DWORD_PTR dstParam) {
    ++g_fakeFmvIcSendMessageCount;
    g_fakeFmvIcSendCodec = codec;
    g_fakeFmvIcSendMessage = message;
    g_fakeFmvIcSendSrcParam = srcParam;
    g_fakeFmvIcSendDstParam = dstParam;
    return 0;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        patch.address = nullptr;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = nullptr;
}

struct TestAction : zFMV_Action {
    zFMV_Action * Delete(std::uint32_t flags) {
        ++g_deletedCount;
        g_lastDeleteFlags = flags;
        return this;
    }

    void Begin(double timeSec) {
        ++g_beginCallCount;
        g_lastBeginTimeSec = timeSec;
    }

    int Update(double timeSec) {
        ++g_updateCallCount;
        g_lastUpdateTimeSec = timeSec;
        return g_nextUpdateResult;
    }

    void End() {
        ++g_endCallCount;
    }
};

struct FakeFmvPlaybackThunk {
    void OpenAndPlay(
        unsigned int startMs,
        int endMs,
        int notifyFlag
    );
    void StopAndClose();
};

void FakeFmvPlaybackThunk::OpenAndPlay(
    unsigned int startMs,
    int endMs,
    int notifyFlag
) {
    ++g_fakeFmvPlaybackOpenAndPlayCount;
    g_fakeFmvPlaybackOpenAndPlaySelf = (zFMV_Playback *)(this);
    g_fakeFmvPlaybackOpenAndPlayStartMs = startMs;
    g_fakeFmvPlaybackOpenAndPlayEndMs = endMs;
    g_fakeFmvPlaybackOpenAndPlayNotifyFlag = notifyFlag;
}

void FakeFmvPlaybackThunk::StopAndClose() {
    ++g_fakeFmvPlaybackStopAndCloseCount;
    g_fakeFmvPlaybackStopAndCloseSelf = (zFMV_Playback *)(this);
}

void *zFMV_Playback_OpenAndPlayProc() {
    union MemberToFunction {
        void ( zFMV_Playback::*member)(unsigned int, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &zFMV_Playback::OpenAndPlay;
    return thunk.function;
}

void *FakeFmvPlaybackOpenAndPlayProc() {
    union MemberToFunction {
        void ( FakeFmvPlaybackThunk::*member)(unsigned int, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &FakeFmvPlaybackThunk::OpenAndPlay;
    return thunk.function;
}

void *zFMV_Playback_StopAndCloseProc() {
    union MemberToFunction {
        void ( zFMV_Playback::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &zFMV_Playback::StopAndClose;
    return thunk.function;
}

void *FakeFmvPlaybackStopAndCloseProc() {
    union MemberToFunction {
        void ( FakeFmvPlaybackThunk::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &FakeFmvPlaybackThunk::StopAndClose;
    return thunk.function;
}

zFMV_Action_Vtbl MakeTestActionVtable() {
    union DeleteMemberToFunction {
        zFMV_Action *( TestAction::*member)(std::uint32_t);
        zFMV_Action *( *function)(zFMV_Action *, std::uint32_t);
    };

    union BeginMemberToFunction {
        void ( TestAction::*member)(double);
        void ( *function)(zFMV_Action *, double);
    };

    union UpdateMemberToFunction {
        int ( TestAction::*member)(double);
        int ( *function)(zFMV_Action *, double);
    };

    union EndMemberToFunction {
        void ( TestAction::*member)();
        void ( *function)(zFMV_Action *);
    };

    DeleteMemberToFunction deleteThunk{};
    deleteThunk.member = &TestAction::Delete;
    BeginMemberToFunction beginThunk{};
    beginThunk.member = &TestAction::Begin;
    UpdateMemberToFunction updateThunk{};
    updateThunk.member = &TestAction::Update;
    EndMemberToFunction endThunk{};
    endThunk.member = &TestAction::End;

    zFMV_Action_Vtbl vtable{};
    vtable.ScalarDeletingDestructor = deleteThunk.function;
    vtable.Update = updateThunk.function;
    vtable.Begin = beginThunk.function;
    vtable.End = endThunk.function;
    return vtable;
}

zFMV_Action_Vtbl g_testActionVtable = MakeTestActionVtable();

using TestFmvGetCurrentPositionFn = std::int32_t(__stdcall *)(void *self,
                                                              std::uint32_t *playCursor,
                                                              std::uint32_t *writeCursor);
using TestFmvLockFn = std::int32_t(__stdcall *)(void *self, std::uint32_t offset,
                                                std::uint32_t bytes, void **outPtr1,
                                                std::int32_t *outBytes1, void **outPtr2,
                                                std::int32_t *outBytes2, std::uint32_t flags);
using TestFmvUnlockFn = std::int32_t(__stdcall *)(void *self, void *ptr1,
                                                  std::int32_t bytes1, void *ptr2,
                                                  std::int32_t bytes2);

struct TestFmvDirectSoundBufferVTable {
    void *slots00_0c[4];
    TestFmvGetCurrentPositionFn GetCurrentPosition;
    void *slots14_28[6];
    TestFmvLockFn Lock;
    void *slots30_48[7];
    TestFmvUnlockFn Unlock;
};

struct TestFmvDirectSoundBuffer {
    TestFmvDirectSoundBufferVTable *vtable;
};

std::int32_t __stdcall TestFmvGetCurrentPosition(void *, std::uint32_t *playCursor,
                                                 std::uint32_t *writeCursor) {
    ++g_fakeFmvGetCurrentPositionCount;
    *playCursor = g_fakeFmvPlayCursor;
    *writeCursor = 0;
    return 0;
}

std::int32_t __stdcall TestFmvLockSoundBuffer(void *, std::uint32_t offset,
                                              std::uint32_t bytes, void **outPtr1,
                                              std::int32_t *outBytes1, void **outPtr2,
                                              std::int32_t *outBytes2, std::uint32_t flags) {
    ++g_fakeFmvLockCount;
    g_fakeFmvLastLockOffset = offset;
    g_fakeFmvLastLockBytes = bytes;
    g_fakeFmvLastLockFlags = flags;
    *outPtr1 = g_fakeFmvLockPtr1;
    *outBytes1 = g_fakeFmvLockBytes1;
    *outPtr2 = g_fakeFmvLockPtr2;
    *outBytes2 = g_fakeFmvLockBytes2;
    return g_fakeFmvLockResult;
}

std::int32_t __stdcall TestFmvUnlockSoundBuffer(void *, void *ptr1, std::int32_t bytes1,
                                                void *ptr2, std::int32_t bytes2) {
    ++g_fakeFmvUnlockCount;
    g_fakeFmvUnlockPtr1 = ptr1;
    g_fakeFmvUnlockBytes1 = bytes1;
    g_fakeFmvUnlockPtr2 = ptr2;
    g_fakeFmvUnlockBytes2 = bytes2;
    return g_fakeFmvUnlockResult;
}

void ResetFmvAudioFillFakes(void *ptr1, int bytes1, void *ptr2, int bytes2) {
    g_fakeAviStreamReadCount = 0;
    std::memset(g_fakeAviStreams, 0, sizeof(g_fakeAviStreams));
    std::memset(g_fakeAviStarts, 0, sizeof(g_fakeAviStarts));
    std::memset(g_fakeAviSamples, 0, sizeof(g_fakeAviSamples));
    std::memset(g_fakeAviBuffers, 0, sizeof(g_fakeAviBuffers));
    std::memset(g_fakeAviBufferBytes, 0, sizeof(g_fakeAviBufferBytes));
    g_fakeAviReturn = 0;
    g_fakeIcDecompressCount = 0;
    g_fakeIcLastCodec = 0;
    g_fakeIcLastFlags = 0;
    g_fakeIcLastSrcFormat = 0;
    g_fakeIcLastCompressedFrame = 0;
    g_fakeIcLastDstFormat = 0;
    g_fakeIcLastPixels = 0;
    g_fakeIcReturn = 0;
    g_fakeFmvLockCount = 0;
    g_fakeFmvUnlockCount = 0;
    g_fakeFmvGetCurrentPositionCount = 0;
    g_fakeFmvLastLockOffset = 0;
    g_fakeFmvLastLockBytes = 0;
    g_fakeFmvLastLockFlags = 0xffffffffu;
    g_fakeFmvLockResult = 0;
    g_fakeFmvUnlockResult = 0;
    g_fakeFmvLockPtr1 = ptr1;
    g_fakeFmvLockPtr2 = ptr2;
    g_fakeFmvLockBytes1 = bytes1;
    g_fakeFmvLockBytes2 = bytes2;
    g_fakeFmvPlayCursor = 0;
    g_fakeFmvUnlockPtr1 = 0;
    g_fakeFmvUnlockPtr2 = 0;
    g_fakeFmvUnlockBytes1 = 0;
    g_fakeFmvUnlockBytes2 = 0;
}

void SetupFmvBeginDependencies(zSndSampleSetRegistry &oldRegistry,
                               zSndSampleSet *(&oldBegin)[1],
                               zSndSampleSet &fmvSet) {
    oldRegistry = g_zSnd_SampleSetRegistry;
    oldBegin[0] = &fmvSet;
    fmvSet = {};
    fmvSet.setName = const_cast<char *>("FMV");
    g_zSnd_SampleSetRegistry.begin = oldBegin;
    g_zSnd_SampleSetRegistry.end = oldBegin + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = oldBegin + 1;
    g_zSnd_SampleSetRegistry.useArchiveBanksFlag = 0;
    g_zVideo_PrimarySurfaceState.pixels = reinterpret_cast<void *>(0x12340000);
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    g_zInput_KbdSystemReady = 0;
    g_beginCallCount = 0;
    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_lastBeginTimeSec = -1.0;
    g_lastUpdateTimeSec = -1.0;
    g_nextUpdateResult = 1;
}

void RestoreFmvBeginDependencies(const zSndSampleSetRegistry &oldRegistry) {
    g_zSnd_SampleSetRegistry = oldRegistry;
}

void WriteU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteBytes(HANDLE file, const char *value, std::uint32_t length) {
    DWORD written = 0;
    WriteFile(file, value, length, &written, nullptr);
}

void WriteStringNode(HANDLE file, const char *value) {
    const std::uint32_t length = static_cast<std::uint32_t>(std::strlen(value));
    WriteU32(file, zReader::ZRDR_NODE_STRING);
    WriteU32(file, length);
    WriteBytes(file, value, length);
}

void WriteIntNode(HANDLE file, std::int32_t value) {
    WriteU32(file, zReader::ZRDR_NODE_INT);
    WriteU32(file, static_cast<std::uint32_t>(value));
}

void WriteFloatNode(HANDLE file, float value) {
    union FloatBits {
        float f32;
        std::uint32_t u32;
    };

    FloatBits bits{};
    bits.f32 = value;
    WriteU32(file, zReader::ZRDR_NODE_FLOAT);
    WriteU32(file, bits.u32);
}

void WriteArrayHeader(HANDLE file, std::int32_t count) {
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, static_cast<std::uint32_t>(count));
}

template <typename T> T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}
} // namespace

extern "C" int zfmv_script_reset_smoke(void) {
    g_deletedCount = 0;
    g_lastDeleteFlags = 0;

    TestAction action1{{&g_testActionVtable, nullptr}};
    TestAction action2{{&g_testActionVtable, nullptr}};
    action1.next = &action2;

    zFMV_Script script{};
    script.m_head = &action1;
    script.m_tail = &action2;
    script.m_cur = nullptr;

    script.Reset(0);
    if (script.m_head != &action1 || script.m_tail != &action2 || script.m_cur != &action1 ||
        g_deletedCount != 0) {
        return 1;
    }

    script.Reset(1);
    if (script.m_head != nullptr || script.m_tail != nullptr || script.m_cur != nullptr) {
        return 2;
    }

    return g_deletedCount == 2 && g_lastDeleteFlags == 1 ? 0 : 3;
}

extern "C" int zfmv_script_init_null_path_smoke(void) {
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);

    zFMV_Script script{};
    script.m_fmvPath = reinterpret_cast<char *>(0x11111111);
    script.m_hWnd = reinterpret_cast<HWND>(0x22222222);
    script.m_abortOnKey = 0;
    script.m_head = reinterpret_cast<zFMV_Action *>(0x33333333);
    script.m_tail = reinterpret_cast<zFMV_Action *>(0x44444444);
    script.m_cur = reinterpret_cast<zFMV_Action *>(0x55555555);

    zFMV_Script *returned = script.Init(nullptr, nullptr, nullptr);
    if (returned != &script) {
        return 1;
    }

    if (script.m_hWnd != reinterpret_cast<HWND>(0x12345678) || script.m_abortOnKey != 1 ||
        script.m_fmvPath != nullptr || script.m_head != nullptr || script.m_tail != nullptr ||
        script.m_cur != nullptr) {
        return 2;
    }

    returned = script.Init(nullptr, nullptr, reinterpret_cast<HWND>(0x87654321));
    return returned == &script && script.m_hWnd == reinterpret_cast<HWND>(0x87654321) ? 0 : 3;
}

extern "C" int zfmv_script_cleanup_smoke(void) {
    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_fmvPath = static_cast<char *>(std::malloc(4));
    script.m_head = &action;
    script.m_tail = &action;
    script.m_cur = &action;

    if (script.m_fmvPath == nullptr) {
        return 1;
    }

    g_deletedCount = 0;
    script.Cleanup();

    if (script.m_fmvPath != nullptr || script.m_head != nullptr || script.m_tail != nullptr ||
        script.m_cur != nullptr) {
        return 2;
    }

    return g_deletedCount == 1 && g_lastDeleteFlags == 1 ? 0 : 3;
}

extern "C" int zfmv_script_append_action_smoke(void) {
    zFMV_Script script{};
    TestAction action1{{&g_testActionVtable, reinterpret_cast<zFMV_Action *>(0x11111111)}};
    TestAction action2{{&g_testActionVtable, reinterpret_cast<zFMV_Action *>(0x22222222)}};

    if (script.AppendAction(nullptr) != 0 || script.m_head != nullptr || script.m_tail != nullptr ||
        script.m_cur != nullptr) {
        return 1;
    }

    if (script.AppendAction(&action1) != 1 || action1.next != nullptr ||
        script.m_head != &action1 || script.m_tail != &action1 || script.m_cur != &action1) {
        return 2;
    }

    if (script.AppendAction(&action2) != 1 || action1.next != &action2 || action2.next != nullptr ||
        script.m_head != &action1 || script.m_tail != &action2 || script.m_cur != &action1) {
        return 3;
    }

    return 0;
}

extern "C" int zfmv_script_run_blocking_empty_smoke(void) {
    zFMV_Script script{};
    script.m_abortOnKey = 0;

    const std::int32_t result = script.RunBlocking(1);
    const bool emptyOk = result == 1 && script.m_abortOnKey == 1 && script.m_cur == nullptr;

    zSndSampleSetRegistry oldRegistry{};
    zSndSampleSet *sampleSetSlots[1] = {};
    zSndSampleSet fmvSet{};
    SetupFmvBeginDependencies(oldRegistry, sampleSetSlots, fmvSet);

    TestAction action{{&g_testActionVtable, nullptr}};
    script = {};
    script.m_head = &action;
    script.m_tail = &action;
    script.m_cur = &action;
    g_nextUpdateResult = 0;

    const int actionResult = script.RunBlocking(0);
    const bool actionOk =
        actionResult == 1 && script.m_abortOnKey == 0 && script.m_cur == &action &&
        g_beginCallCount == 1 && g_updateCallCount == 1 && g_endCallCount == 1 &&
        fmvSet.resourcesLoaded == 1;

    RestoreFmvBeginDependencies(oldRegistry);
    if (!emptyOk) {
        return 1;
    }
    return actionOk ? 0 : 2;
}

extern "C" int zfmv_action_run_blocking_timed_smoke(void) {
    g_beginCallCount = 0;
    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_lastBeginTimeSec = -1.0;
    g_lastUpdateTimeSec = -1.0;
    g_nextUpdateResult = 0;

    TestAction action{{&g_testActionVtable, nullptr}};
    action.RunBlockingTimed();

    return g_beginCallCount == 1 && g_lastBeginTimeSec == 0.0 && g_updateCallCount == 1 &&
                   g_lastUpdateTimeSec >= 0.0 && g_endCallCount == 1
               ? 0
               : 1;
}

extern "C" int zfmv_action_run_blocking_immediate_smoke(void) {
    g_beginCallCount = 0;
    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_lastBeginTimeSec = -1.0;
    g_lastUpdateTimeSec = -1.0;
    g_nextUpdateResult = 0;

    TestAction action{{&g_testActionVtable, nullptr}};
    action.RunBlockingImmediate();

    return g_beginCallCount == 1 && g_lastBeginTimeSec == 0.0 && g_updateCallCount == 1 &&
                   g_lastUpdateTimeSec == 0.0 && g_endCallCount == 1
               ? 0
               : 1;
}

extern "C" int zfmv_script_begin_current_action_smoke(void) {
    zFMV_Script emptyScript{};
    if (emptyScript.BeginCurrentAction(12.5) != 0) {
        return 1;
    }

    zSndSampleSetRegistry oldRegistry{};
    zSndSampleSet *sampleSetSlots[1] = {};
    zSndSampleSet fmvSet{};
    SetupFmvBeginDependencies(oldRegistry, sampleSetSlots, fmvSet);

    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_cur = &action;
    const int result = script.BeginCurrentAction(42.25);

    const bool ok = result == 1 && script.m_startTimeSec == 42.25 && g_beginCallCount == 1 &&
                    g_lastBeginTimeSec == 0.0 &&
                    g_zVideo_FxSurfacePixels16 ==
                        reinterpret_cast<unsigned short *>(0x12340000) &&
                    g_zVideo_FxSurfaceWidth == 320 && g_zVideo_FxSurfaceHeight == 200 &&
                    g_zVideo_FxSurfacePitchBytes == 640 &&
                    g_zVideo_FxSurfacePitchPixels16 == 320 && fmvSet.resourcesLoaded == 1;

    RestoreFmvBeginDependencies(oldRegistry);
    return ok ? 0 : 2;
}

extern "C" int zfmv_script_begin_at_time_smoke(void) {
    zSndSampleSetRegistry oldRegistry{};
    zSndSampleSet *sampleSetSlots[1] = {};
    zSndSampleSet fmvSet{};
    SetupFmvBeginDependencies(oldRegistry, sampleSetSlots, fmvSet);

    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_cur = &action;
    const int result = script.BeginAtTime();

    const bool ok = result == 1 && script.m_startTimeSec >= 0.0 && g_beginCallCount == 1 &&
                    g_lastBeginTimeSec == 0.0 && fmvSet.resourcesLoaded == 1;

    RestoreFmvBeginDependencies(oldRegistry);
    return ok ? 0 : 1;
}

extern "C" int zfmv_script_update_smoke(void) {
    zFMV_Script emptyScript{};
    if (emptyScript.Update(12.0) != 0) {
        return 1;
    }

    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_beginCallCount = 0;
    g_lastUpdateTimeSec = -1.0;
    g_lastBeginTimeSec = -1.0;
    g_nextUpdateResult = 1;

    TestAction action1{{&g_testActionVtable, nullptr}};
    TestAction action2{{&g_testActionVtable, nullptr}};
    action1.next = &action2;

    zFMV_Script script{};
    script.m_startTimeSec = 10.0;
    script.m_abortOnKey = 0;
    script.m_cur = &action1;

    if (script.Update(12.5) != 1 || script.m_cur != &action1 || g_updateCallCount != 1 ||
        g_lastUpdateTimeSec != 2.5 || g_endCallCount != 0 || g_beginCallCount != 0) {
        return 2;
    }

    g_nextUpdateResult = 0;
    if (script.Update(14.0) != 1 || script.m_cur != &action2 || g_updateCallCount != 2 ||
        g_lastUpdateTimeSec != 4.0 || g_endCallCount != 1 || g_beginCallCount != 1 ||
        g_lastBeginTimeSec != 4.0) {
        return 3;
    }

    return 0;
}

extern "C" int zfmv_script_update_at_time_smoke(void) {
    g_updateCallCount = 0;
    g_endCallCount = 0;
    g_nextUpdateResult = 1;

    TestAction action{{&g_testActionVtable, nullptr}};
    zFMV_Script script{};
    script.m_startTimeSec = 0.0;
    script.m_abortOnKey = 0;
    script.m_cur = &action;

    const int result = script.UpdateAtTime();
    return result == 1 && script.m_cur == &action && g_updateCallCount == 1 &&
                   g_lastUpdateTimeSec >= 0.0 && g_endCallCount == 0
               ? 0
               : 1;
}

extern "C" int zfmv_script_begin_now_smoke(void) {
    g_deletedCount = 0;

    TestAction action1{{&g_testActionVtable, nullptr}};
    TestAction action2{{&g_testActionVtable, nullptr}};
    action1.next = &action2;

    zFMV_Script script{};
    script.m_head = &action1;
    script.m_tail = &action2;
    script.m_cur = nullptr;

    script.BeginNow(0);
    if (script.m_head != &action1 || script.m_tail != &action2 || script.m_cur != &action1 ||
        g_deletedCount != 0) {
        return 1;
    }

    script.BeginNow(1);
    return script.m_head == nullptr && script.m_tail == nullptr && script.m_cur == nullptr &&
                   g_deletedCount == 2
               ? 0
               : 2;
}

extern "C" int zfmv_action_image_constructor_with_screen_rect_smoke(void) {
    g_zFMV_ActionImage_BlitRectX = -1;
    g_zFMV_ActionImage_BlitRectY = -1;
    g_zFMV_ActionImage_BlitRectW = 640;
    g_zFMV_ActionImage_BlitRectH = 480;

    zFMV_ActionImage action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.image = reinterpret_cast<void *>(0x22222222);

    zFMV_ActionImage *returned = new (&action) zFMV_ActionImage("screen.raw", 7, 32, 48);

    const bool ok =
        returned == &action && action.vftable == &g_zFMV_ActionImage_Vtable &&
        action.next == nullptr && action.image == nullptr && action.imagePath != nullptr &&
        std::strcmp(action.imagePath, "screen.raw") == 0 && action.doAdjustSurfaces == 7 &&
        action.forcePrimaryPostprocess == 1 && g_zFMV_ActionImage_BlitRectX == 32 &&
        g_zFMV_ActionImage_BlitRectY == 48 && action.blitRect.left == 32 &&
        action.blitRect.top == 48 && action.blitRect.right == 640 && action.blitRect.bottom == 480;

    std::free(action.imagePath);
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_image_constructor_scaled_smoke(void) {
    zRndr::g_activeRegionWidth = 800;
    zRndr::g_activeRegionHeight = 600;
    zRndr::g_pitchBytes = 1600;
    zRndr::g_bytesPerPixel = 2;

    zFMV_ActionImage action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.image = reinterpret_cast<void *>(0x22222222);

    zFMV_ActionImage *returned = new (&action) zFMV_ActionImage("scaled.raw", 3);
    const bool ok =
        returned == &action && action.vftable == &g_zFMV_ActionImage_Vtable &&
        action.next == nullptr && action.image == nullptr && action.imagePath != nullptr &&
        std::strcmp(action.imagePath, "scaled.raw") == 0 && action.doAdjustSurfaces == 3 &&
        action.forcePrimaryPostprocess == 0 && action.blitRect.left == 0 && action.blitRect.top == 0 &&
        action.blitRect.right == 800 && action.blitRect.bottom == 600;

    std::free(action.imagePath);
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_image_begin_smoke(void) {
    CodeFunctionPatch findPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zImage::TexDir_FindOrCreateByPath),
                           reinterpret_cast<void *>(&FakeFmvTexDirFindOrCreateByPath),
                           findPatch)) {
        return 1;
    }

    zVidImagePartial image{};
    zFMV_ActionImage action{};
    action.imagePath = const_cast<char *>("begin.raw");
    action.image = reinterpret_cast<void *>(0x33333333);
    g_fakeFmvTexDirFindCount = 0;
    g_fakeFmvTexDirFindPath = nullptr;
    g_fakeFmvTexDirFindResult = &image;

    action.Begin(45.0);

    const bool ok =
        action.image == &image && g_fakeFmvTexDirFindCount == 1 &&
        g_fakeFmvTexDirFindPath == action.imagePath;

    RestoreFunctionPatch(findPatch);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_image_update_smoke(void) {
    CodeFunctionPatch postPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                           reinterpret_cast<void *>(&FakeFmvRunPostprocessOnPrimaryBuffer),
                           postPatch)) {
        return 1;
    }

    CodeFunctionPatch blitPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVid_Image::BlitToActiveTarget),
                           reinterpret_cast<void *>(&FakeFmvBlitToActiveTarget),
                           blitPatch)) {
        RestoreFunctionPatch(postPatch);
        return 2;
    }

    CodeFunctionPatch unlockPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                           reinterpret_cast<void *>(&FakeFmvDispatchUnlockPrimarySurfaceState),
                           unlockPatch)) {
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postPatch);
        return 3;
    }

    CodeFunctionPatch adjustPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
                           reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
                           adjustPatch)) {
        RestoreFunctionPatch(unlockPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postPatch);
        return 4;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const unsigned int oldSwBlt = g_zVideo_pfnBltSwToPrimaryRect;
    g_zVideo_pfnBltSwToPrimaryRect =
        reinterpret_cast<unsigned int>(&FakeFmvBltSwToPrimaryRect);

    zVidImagePartial image{};
    zFMV_ActionImage action{};
    action.image = nullptr;
    g_zVideo_ActiveRendererPath = 2;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvSwBltCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    const bool nullImageOk =
        action.Update(1.25) == 0 && g_fakeFmvPostprocessCount == 0 &&
        g_fakeFmvBlitToActiveTargetCount == 0 && g_fakeFmvUnlockPrimaryCount == 0 &&
        g_fakeFmvSwBltCount == 0 && g_fakeFmvAdjustSurfacesCount == 0;

    action.image = &image;
    action.forcePrimaryPostprocess = 0;
    action.doAdjustSurfaces = 1;
    action.blitRect.left = 10;
    action.blitRect.top = 20;
    action.blitRect.right = 300;
    action.blitRect.bottom = 400;
    g_zVideo_ActiveRendererPath = 0;
    g_fakeFmvSwBltCount = 0;
    g_fakeFmvSwBltImage = nullptr;
    g_fakeFmvSwBltColorKeyEnable = -1;
    g_fakeFmvSwBltSrcRect = reinterpret_cast<zVidRect32 *>(0x11111111);
    g_fakeFmvSwBltDstRect = nullptr;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvAdjustSurfacesSrcRect = reinterpret_cast<zVidRect32 *>(0x22222222);
    g_fakeFmvAdjustSurfacesDstRect = reinterpret_cast<zVidRect32 *>(0x33333333);
    g_fakeFmvAdjustSurfacesWaitForPresent = 0;
    g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst = 0;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    const bool softwareOk =
        action.Update(2.5) == 0 && g_fakeFmvSwBltCount == 1 &&
        g_fakeFmvSwBltImage == &image && g_fakeFmvSwBltColorKeyEnable == 0 &&
        g_fakeFmvSwBltSrcRect == nullptr &&
        g_fakeFmvSwBltDstRect == &action.blitRect &&
        g_fakeFmvAdjustSurfacesCount == 1 &&
        g_fakeFmvAdjustSurfacesSrcRect == nullptr &&
        g_fakeFmvAdjustSurfacesDstRect == nullptr &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 1 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 1 &&
        g_fakeFmvPostprocessCount == 0 && g_fakeFmvBlitToActiveTargetCount == 0 &&
        g_fakeFmvUnlockPrimaryCount == 0;

    action.doAdjustSurfaces = 0;
    action.forcePrimaryPostprocess = 0;
    action.blitRect.left = 33;
    action.blitRect.top = 44;
    g_zVideo_ActiveRendererPath = 2;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvBlitToActiveTargetImage = nullptr;
    g_fakeFmvBlitToActiveTargetDstX = 0;
    g_fakeFmvBlitToActiveTargetDstY = 0;
    g_fakeFmvBlitToActiveTargetClipFlags = -1;
    g_fakeFmvBlitToActiveTargetSrcRect = reinterpret_cast<zVidRect32 *>(0x44444444);
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvSwBltCount = 0;
    const bool postprocessOk =
        action.Update(3.75) == 0 && g_fakeFmvPostprocessCount == 2 &&
        g_fakeFmvBlitToActiveTargetCount == 2 &&
        g_fakeFmvBlitToActiveTargetImage == &image &&
        g_fakeFmvBlitToActiveTargetDstX == 33 &&
        g_fakeFmvBlitToActiveTargetDstY == 44 &&
        g_fakeFmvBlitToActiveTargetClipFlags == 0 &&
        g_fakeFmvBlitToActiveTargetSrcRect == nullptr &&
        g_fakeFmvUnlockPrimaryCount == 2 && g_fakeFmvAdjustSurfacesCount == 0 &&
        g_fakeFmvSwBltCount == 0;

    g_zVideo_pfnBltSwToPrimaryRect = oldSwBlt;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(unlockPatch);
    RestoreFunctionPatch(blitPatch);
    RestoreFunctionPatch(postPatch);
    return nullImageOk && softwareOk && postprocessOk ? 0 : 5;
}

extern "C" int zfmv_action_image_lifecycle_smoke(void) {
    CodeFunctionPatch releasePatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVid_Image::ReleaseIfNotDefault),
                           reinterpret_cast<void *>(&FakeFmvReleaseImageIfNotDefault),
                           releasePatch)) {
        return 1;
    }

    zVidImagePartial image{};
    zFMV_ActionImage action{};
    action.image = nullptr;
    g_fakeFmvReleaseImageCount = 0;
    g_fakeFmvReleaseImage = nullptr;
    action.End();
    const bool nullEndOk = action.image == nullptr && g_fakeFmvReleaseImageCount == 0;

    action.image = &image;
    action.End();
    const bool releaseEndOk =
        action.image == nullptr && g_fakeFmvReleaseImageCount == 1 &&
        g_fakeFmvReleaseImage == &image;

    action.vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x11111111);
    action.image = &image;
    action.imagePath = static_cast<char *>(std::malloc(6));
    if (action.imagePath == nullptr) {
        RestoreFunctionPatch(releasePatch);
        return 2;
    }
    std::strcpy(action.imagePath, "path");
    g_fakeFmvReleaseImageCount = 0;
    g_fakeFmvReleaseImage = nullptr;
    action.Destructor();
    const bool destructorOk =
        action.vftable == &g_zFMV_ActionBase_Vtable && action.image == nullptr &&
        action.imagePath == nullptr && g_fakeFmvReleaseImageCount == 1 &&
        g_fakeFmvReleaseImage == &image;

    CodeFunctionPatch deletePatch{};
    void(__cdecl *operatorDeleteFn)(void *) = &operator delete;
    if (!PatchFunctionJump(reinterpret_cast<void *>(operatorDeleteFn),
                           reinterpret_cast<void *>(&FakeFmvOperatorDelete), deletePatch)) {
        RestoreFunctionPatch(releasePatch);
        return 3;
    }

    void *const storage = std::malloc(sizeof(zFMV_ActionImage));
    if (storage == nullptr) {
        RestoreFunctionPatch(deletePatch);
        RestoreFunctionPatch(releasePatch);
        return 4;
    }

    zFMV_ActionImage *const heapAction = reinterpret_cast<zFMV_ActionImage *>(storage);
    std::memset(heapAction, 0, sizeof(*heapAction));
    heapAction->vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x22222222);
    heapAction->image = &image;
    heapAction->imagePath = static_cast<char *>(std::malloc(5));
    if (heapAction->imagePath == nullptr) {
        std::free(storage);
        RestoreFunctionPatch(deletePatch);
        RestoreFunctionPatch(releasePatch);
        return 5;
    }
    std::strcpy(heapAction->imagePath, "heap");
    g_fakeFmvReleaseImageCount = 0;
    g_fakeFmvReleaseImage = nullptr;
    g_fakeFmvOperatorDeleteCount = 0;
    g_fakeFmvOperatorDeletePtr = nullptr;
    zFMV_ActionImage *const returned = heapAction->ScalarDeletingDestructor(1);
    const bool scalarOk =
        returned == heapAction && heapAction->vftable == &g_zFMV_ActionBase_Vtable &&
        heapAction->image == nullptr && heapAction->imagePath == nullptr &&
        g_fakeFmvReleaseImageCount == 1 && g_fakeFmvReleaseImage == &image &&
        g_fakeFmvOperatorDeleteCount == 1 && g_fakeFmvOperatorDeletePtr == heapAction;

    RestoreFunctionPatch(deletePatch);
    RestoreFunctionPatch(releasePatch);
    std::free(storage);
    return nullEndOk && releaseEndOk && destructorOk && scalarOk ? 0 : 6;
}

extern "C" int zfmv_action_fade_constructor_smoke(void) {
    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;

    zFMV_ActionFade action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.reserved0e = 0x7777;
    action.startSec = 123.0;
    action.capturedFrame = reinterpret_cast<void *>(0x22222222);

    zFMV_ActionFade *returned = new (&action) zFMV_ActionFade(
        0xff,
        0x80,
        0x20,
        0x3fc00000,
        -1,
        128
    );

    return returned == &action && action.vftable == &g_zFMV_ActionFade_Vtable &&
                   action.next == nullptr && action.fadeDirectionSign == -1 &&
                   action.fadeColorPacked16 == 0xfc04 && action.reserved0e == 0x7777 &&
                   action.durationSecRaw == 0x3fc00000 && action.startSec == 123.0 &&
                   action.capturedFrame == reinterpret_cast<void *>(0x22222222) &&
                   action.maxAlpha == 128
               ? 0
               : 1;
}

extern "C" int zfmv_action_fade_begin_smoke(void) {
    CodeFunctionPatch capturePatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo_buff_CaptureSurfaceToImage),
                           reinterpret_cast<void *>(&FakeFmvCaptureSurfaceToImage),
                           capturePatch)) {
        return 1;
    }

    zVidImagePartial image{};
    zFMV_ActionFade action{};
    action.startSec = -1.0;
    action.capturedFrame = reinterpret_cast<void *>(0x11111111);
    g_fakeFmvCaptureSurfaceCount = 0;
    g_fakeFmvCaptureSurfaceSelector = 0;
    g_fakeFmvCaptureSurfaceResult = &image;

    action.Begin(12.75);

    const bool ok =
        action.capturedFrame == &image && action.startSec == 12.75 &&
        g_fakeFmvCaptureSurfaceCount == 1 && g_fakeFmvCaptureSurfaceSelector == 1;

    RestoreFunctionPatch(capturePatch);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_fade_update_smoke(void) {
    CodeFunctionPatch primaryPostPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                           reinterpret_cast<void *>(&FakeFmvRunPostprocessOnPrimaryBuffer),
                           primaryPostPatch)) {
        return 1;
    }

    CodeFunctionPatch swPostPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnSwBuffer),
                           reinterpret_cast<void *>(&FakeFmvRunPostprocessOnSwBuffer),
                           swPostPatch)) {
        RestoreFunctionPatch(primaryPostPatch);
        return 2;
    }

    CodeFunctionPatch blitPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVid_Image::BlitToActiveTarget),
                           reinterpret_cast<void *>(&FakeFmvBlitToActiveTarget),
                           blitPatch)) {
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 3;
    }

    CodeFunctionPatch unlockPrimaryPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                           reinterpret_cast<void *>(&FakeFmvDispatchUnlockPrimarySurfaceState),
                           unlockPrimaryPatch)) {
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 4;
    }

    CodeFunctionPatch unlockSwPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockSwSurfaceState),
                           reinterpret_cast<void *>(&FakeFmvDispatchUnlockSwSurfaceState),
                           unlockSwPatch)) {
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 5;
    }

    CodeFunctionPatch overlayPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zRndr_OverlayRect_Submit),
                           reinterpret_cast<void *>(&FakeFmvOverlayRectSubmit),
                           overlayPatch)) {
        RestoreFunctionPatch(unlockSwPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 6;
    }

    CodeFunctionPatch overlayFlushPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zRndr_OverlayRect_FlushSw),
                           reinterpret_cast<void *>(&FakeFmvOverlayRectFlushSw),
                           overlayFlushPatch)) {
        RestoreFunctionPatch(overlayPatch);
        RestoreFunctionPatch(unlockSwPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 7;
    }

    CodeFunctionPatch sceneEnterPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideoD3D::SceneEnter),
                           reinterpret_cast<void *>(&FakeFmvSceneEnter),
                           sceneEnterPatch)) {
        RestoreFunctionPatch(overlayFlushPatch);
        RestoreFunctionPatch(overlayPatch);
        RestoreFunctionPatch(unlockSwPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 8;
    }

    CodeFunctionPatch sceneLeavePatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideoD3D::SceneLeave),
                           reinterpret_cast<void *>(&FakeFmvSceneLeave),
                           sceneLeavePatch)) {
        RestoreFunctionPatch(sceneEnterPatch);
        RestoreFunctionPatch(overlayFlushPatch);
        RestoreFunctionPatch(overlayPatch);
        RestoreFunctionPatch(unlockSwPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 9;
    }

    CodeFunctionPatch adjustPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
                           reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
                           adjustPatch)) {
        RestoreFunctionPatch(sceneLeavePatch);
        RestoreFunctionPatch(sceneEnterPatch);
        RestoreFunctionPatch(overlayFlushPatch);
        RestoreFunctionPatch(overlayPatch);
        RestoreFunctionPatch(unlockSwPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(swPostPatch);
        RestoreFunctionPatch(primaryPostPatch);
        return 10;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const unsigned int oldFlushQuadBatch = g_zVideo_pfnFlushQuadBatch;
    g_zVideo_pfnFlushQuadBatch = FakeFmvFlushQuadBatch;

    zVidImagePartial image{};
    zFMV_ActionFade action{};
    action.capturedFrame = nullptr;
    g_zVideo_ActiveRendererPath = 0;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvSwPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvOverlaySubmitCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    const bool nullOk =
        action.Update(11.0) == 0 && g_fakeFmvPostprocessCount == 0 &&
        g_fakeFmvSwPostprocessCount == 0 && g_fakeFmvBlitToActiveTargetCount == 0 &&
        g_fakeFmvOverlaySubmitCount == 0 && g_fakeFmvAdjustSurfacesCount == 0;

    action.capturedFrame = &image;
    action.startSec = 10.0;
    action.durationSecRaw = FmvFloatBits(4.0f);
    action.fadeDirectionSign = 1;
    action.fadeColorPacked16 = 0x7bef;
    action.maxAlpha = 100;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvSwPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvBlitToActiveTargetImage = nullptr;
    g_fakeFmvBlitToActiveTargetDstX = -1;
    g_fakeFmvBlitToActiveTargetDstY = -1;
    g_fakeFmvBlitToActiveTargetClipFlags = -1;
    g_fakeFmvBlitToActiveTargetSrcRect = reinterpret_cast<zVidRect32 *>(0x11111111);
    g_fakeFmvOverlaySubmitCount = 0;
    g_fakeFmvOverlaySubmitColor = 0;
    g_fakeFmvOverlaySubmitRect = reinterpret_cast<zVidRect32 *>(0x22222222);
    g_fakeFmvOverlaySubmitAlpha = -1.0;
    g_fakeFmvOverlayFlushSwCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvUnlockSwCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvAdjustSurfacesWaitForPresent = 0;
    g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst = 0;
    g_fakeFmvSceneEnterCount = 0;
    g_fakeFmvSceneLeaveCount = 0;
    g_fakeFmvFlushQuadBatchCount = 0;
    const bool softwareOk =
        action.Update(12.0) == 1 && g_fakeFmvPostprocessCount == 1 &&
        g_fakeFmvSwPostprocessCount == 0 && g_fakeFmvBlitToActiveTargetCount == 1 &&
        g_fakeFmvBlitToActiveTargetImage == &image &&
        g_fakeFmvBlitToActiveTargetDstX == 0 &&
        g_fakeFmvBlitToActiveTargetDstY == 0 &&
        g_fakeFmvBlitToActiveTargetClipFlags == 0 &&
        g_fakeFmvBlitToActiveTargetSrcRect == nullptr &&
        g_fakeFmvOverlaySubmitCount == 1 && g_fakeFmvOverlaySubmitColor == 0x7bef &&
        g_fakeFmvOverlaySubmitRect == nullptr && g_fakeFmvOverlaySubmitAlpha == 50.0 &&
        g_fakeFmvOverlayFlushSwCount == 1 && g_fakeFmvUnlockPrimaryCount == 1 &&
        g_fakeFmvUnlockSwCount == 0 && g_fakeFmvAdjustSurfacesCount == 1 &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 1 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 1 &&
        g_fakeFmvSceneEnterCount == 0 && g_fakeFmvSceneLeaveCount == 0 &&
        g_fakeFmvFlushQuadBatchCount == 0;

    action.fadeDirectionSign = -1;
    g_zVideo_ActiveRendererPath = 1;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvSwPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvOverlaySubmitCount = 0;
    g_fakeFmvOverlaySubmitAlpha = -1.0;
    g_fakeFmvOverlayFlushSwCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvUnlockSwCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvAdjustSurfacesWaitForPresent = -1;
    g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst = -1;
    g_fakeFmvSceneEnterCount = 0;
    g_fakeFmvSceneLeaveCount = 0;
    g_fakeFmvFlushQuadBatchCount = 0;
    const bool hardwareOk =
        action.Update(15.0) == 0 && g_fakeFmvPostprocessCount == 0 &&
        g_fakeFmvSwPostprocessCount == 1 && g_fakeFmvBlitToActiveTargetCount == 1 &&
        g_fakeFmvOverlaySubmitCount == 1 && g_fakeFmvOverlaySubmitAlpha == 0.0 &&
        g_fakeFmvOverlayFlushSwCount == 0 && g_fakeFmvUnlockPrimaryCount == 0 &&
        g_fakeFmvUnlockSwCount == 1 && g_fakeFmvAdjustSurfacesCount == 1 &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 0 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 0 &&
        g_fakeFmvSceneEnterCount == 1 && g_fakeFmvSceneLeaveCount == 1 &&
        g_fakeFmvFlushQuadBatchCount == 1;

    g_zVideo_pfnFlushQuadBatch = oldFlushQuadBatch;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(sceneLeavePatch);
    RestoreFunctionPatch(sceneEnterPatch);
    RestoreFunctionPatch(overlayFlushPatch);
    RestoreFunctionPatch(overlayPatch);
    RestoreFunctionPatch(unlockSwPatch);
    RestoreFunctionPatch(unlockPrimaryPatch);
    RestoreFunctionPatch(blitPatch);
    RestoreFunctionPatch(swPostPatch);
    RestoreFunctionPatch(primaryPostPatch);
    return nullOk && softwareOk && hardwareOk ? 0 : 11;
}

extern "C" int zfmv_action_fade_end_smoke(void) {
    CodeFunctionPatch releasePatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVid_Image::ReleaseIfNotDefault),
                           reinterpret_cast<void *>(&FakeFmvReleaseImageIfNotDefault),
                           releasePatch)) {
        return 1;
    }

    zVidImagePartial image{};
    zFMV_ActionFade action{};
    action.capturedFrame = nullptr;
    g_fakeFmvReleaseImageCount = 0;
    g_fakeFmvReleaseImage = nullptr;
    action.End();
    const bool nullOk = action.capturedFrame == nullptr && g_fakeFmvReleaseImageCount == 0;

    action.capturedFrame = &image;
    action.End();
    const bool releaseOk =
        action.capturedFrame == nullptr && g_fakeFmvReleaseImageCount == 1 &&
        g_fakeFmvReleaseImage == &image;

    RestoreFunctionPatch(releasePatch);
    return nullOk && releaseOk ? 0 : 2;
}

extern "C" int zfmv_playback_set_dest_rect_smoke(void) {
    zFMV_Playback playback{};
    playback.mciPutFlags = 0x10;
    const zFMV_Rect rect{1, 2, 3, 4};

    const std::int32_t result = playback.SetDestRect(&rect);

    return result == 0x40010 && playback.mciPutFlags == 0x40010 &&
                   playback.destinationRect.left == 1 && playback.destinationRect.top == 2 &&
                   playback.destinationRect.right == 3 && playback.destinationRect.bottom == 4
               ? 0
               : 1;
}

extern "C" int zfmv_playback_destructor_smoke(void) {
    zFMV_Playback playback{};
    playback.mediaPathDup = static_cast<char *>(std::malloc(4));
    if (playback.mediaPathDup == nullptr) {
        return 1;
    }

    std::strcpy(playback.mediaPathDup, "x");
    playback.Destructor();
    playback.mediaPathDup = nullptr;
    playback.Destructor();
    return 0;
}

extern "C" int zfmv_playback_report_mci_error_smoke(void) {
    zFMV_Playback playback{};
    return playback.ReportMciError(0xffffffffu) == 0 ? 0 : 1;
}

extern "C" int zfmv_playback_open_and_play_smoke(void) {
    CodeFunctionPatch mciPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&mciSendCommandA),
                           reinterpret_cast<void *>(&FakeFmvMciSendCommandA), mciPatch)) {
        return 1;
    }

    zFMV_Playback playback{};
    playback.mediaPathDup = const_cast<char *>("intro.mpg");
    playback.notifyHwnd = reinterpret_cast<HWND>(0x12345678);
    playback.mciPutFlags = 0x60000;
    playback.sourceRect.left = 2;
    playback.sourceRect.top = 3;
    playback.sourceRect.right = 12;
    playback.sourceRect.bottom = 23;
    playback.destinationRect.left = 5;
    playback.destinationRect.top = 7;
    playback.destinationRect.right = 25;
    playback.destinationRect.bottom = 37;

    g_fakeFmvMciSendCommandCount = 0;
    g_fakeFmvOpenDeviceType = nullptr;
    g_fakeFmvOpenElementName = nullptr;
    g_fakeFmvWindowHwnd = nullptr;
    g_fakeFmvSetTimeFormat = 0;
    g_fakeFmvSetAudio = 0;
    g_fakeFmvPlayCallback = 0;
    g_fakeFmvPlayFrom = 0;
    g_fakeFmvPlayTo = 0;
    for (int index = 0; index < 8; ++index) {
        g_fakeFmvMciDevices[index] = 0;
        g_fakeFmvMciMessages[index] = 0;
        g_fakeFmvMciFlags[index] = 0;
        g_fakeFmvMciParams[index] = 0;
        g_fakeFmvMciReturns[index] = 0;
    }
    for (int index = 0; index < 4; ++index) {
        g_fakeFmvDestRect[index] = 0;
        g_fakeFmvSourceRect[index] = 0;
    }

    playback.OpenAndPlay(100, 250, 1);

    const bool successSequence =
        g_fakeFmvMciSendCommandCount == 6 && g_fakeFmvMciDevices[0] == 0 &&
        g_fakeFmvMciMessages[0] == 0x803 && g_fakeFmvMciFlags[0] == 0x2202 &&
        std::strcmp(g_fakeFmvOpenDeviceType, "MPEGVideo") == 0 &&
        g_fakeFmvOpenElementName == playback.mediaPathDup && playback.mciDeviceId == 0x3456 &&
        g_fakeFmvMciDevices[1] == 0x3456 && g_fakeFmvMciMessages[1] == 0x841 &&
        g_fakeFmvMciFlags[1] == 0x10002 && g_fakeFmvWindowHwnd == playback.notifyHwnd &&
        g_fakeFmvMciDevices[2] == 0x3456 && g_fakeFmvMciMessages[2] == 0x842 &&
        g_fakeFmvMciFlags[2] == 0x50002 && g_fakeFmvDestRect[0] == 5 &&
        g_fakeFmvDestRect[1] == 7 && g_fakeFmvDestRect[2] == 20 &&
        g_fakeFmvDestRect[3] == 30 && g_fakeFmvMciDevices[3] == 0x3456 &&
        g_fakeFmvMciMessages[3] == 0x842 && g_fakeFmvMciFlags[3] == 0x30002 &&
        g_fakeFmvSourceRect[0] == 2 && g_fakeFmvSourceRect[1] == 3 &&
        g_fakeFmvSourceRect[2] == 10 && g_fakeFmvSourceRect[3] == 20 &&
        g_fakeFmvMciDevices[4] == 0x3456 && g_fakeFmvMciMessages[4] == 0x811 &&
        g_fakeFmvMciFlags[4] == 0x302 && g_fakeFmvSetTimeFormat == 0x1b &&
        g_fakeFmvSetAudio == static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(
                                 playback.notifyHwnd)) &&
        g_fakeFmvMciDevices[5] == 0x3456 && g_fakeFmvMciMessages[5] == 0x806 &&
        g_fakeFmvMciFlags[5] == 0x1000e && g_fakeFmvPlayCallback ==
            static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(playback.notifyHwnd)) &&
        g_fakeFmvPlayFrom == 100 && g_fakeFmvPlayTo == 250;

    g_fakeFmvMciSendCommandCount = 0;
    for (int index = 0; index < 8; ++index) {
        g_fakeFmvMciMessages[index] = 0;
        g_fakeFmvMciReturns[index] = 0;
    }
    playback.mciPutFlags = 0;
    g_fakeFmvMciReturns[1] = 0x4321;
    playback.OpenAndPlay(75, -1, 0);
    const bool windowFailureStops =
        g_fakeFmvMciSendCommandCount == 2 && g_fakeFmvMciMessages[0] == 0x803 &&
        g_fakeFmvMciMessages[1] == 0x841;

    RestoreFunctionPatch(mciPatch);
    return successSequence && windowFailureStops ? 0 : 2;
}

extern "C" int zfmv_playback_stop_and_close_smoke(void) {
    CodeFunctionPatch mciPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&mciSendCommandA),
                           reinterpret_cast<void *>(&FakeFmvMciSendCommandA), mciPatch)) {
        return 1;
    }

    zFMV_Playback playback{};
    playback.mciDeviceId = 0x3456;
    g_fakeFmvExpectedClosePlayback = &playback;
    g_fakeFmvCloseParamOk = 0;
    g_fakeFmvMciSendCommandCount = 0;
    for (int index = 0; index < 8; ++index) {
        g_fakeFmvMciDevices[index] = 0;
        g_fakeFmvMciMessages[index] = 0;
        g_fakeFmvMciFlags[index] = 0;
        g_fakeFmvMciParams[index] = 0;
        g_fakeFmvMciReturns[index] = 0;
    }

    playback.StopAndClose();
    const bool successSequence =
        g_fakeFmvMciSendCommandCount == 2 && g_fakeFmvMciDevices[0] == 0x3456 &&
        g_fakeFmvMciMessages[0] == 0x808 && g_fakeFmvMciFlags[0] == 2 &&
        g_fakeFmvMciParams[0] == 0 && g_fakeFmvMciDevices[1] == 0x3456 &&
        g_fakeFmvMciMessages[1] == 0x804 && g_fakeFmvMciFlags[1] == 2 &&
        g_fakeFmvCloseParamOk == 1;

    g_fakeFmvMciSendCommandCount = 0;
    g_fakeFmvMciReturns[0] = 0x1234;
    g_fakeFmvMciReturns[1] = 0;
    playback.StopAndClose();
    const bool stopFailureSkipsClose =
        g_fakeFmvMciSendCommandCount == 1 && g_fakeFmvMciMessages[0] == 0x808;

    RestoreFunctionPatch(mciPatch);
    return successSequence && stopFailureSkipsClose ? 0 : 2;
}

extern "C" int zfmv_stream_destructor_empty_smoke(void) {
    alignas(8) std::uint8_t storage[0x1d4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    TestFieldAt<char *>(stream, 0x38) = static_cast<char *>(std::malloc(4));
    if (TestFieldAt<char *>(stream, 0x38) == nullptr) {
        return 1;
    }

    std::strcpy(TestFieldAt<char *>(stream, 0x38), "x");
    InitializeCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));

    stream->Destructor();
    return 0;
}

extern "C" int zfmv_stream_constructor_missing_file_smoke(void) {
    alignas(8) std::uint8_t storage[0x1d4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    TestFieldAt<char *>(stream, 0x38) = const_cast<char *>("__missing_stream_ctor__.avi");
    TestFieldAt<std::int32_t>(stream, 0x104) = 0x12345678;

    stream->Constructor();

    return TestFieldAt<std::int32_t>(stream, 0x104) == 0 &&
                   TestFieldAt<std::int32_t>(stream, 0x3c) == 0
               ? 0
               : 1;
}

extern "C" int zfmv_stream_constructor_success_smoke(void) {
    CodeFunctionPatch openPatch{};
    CodeFunctionPatch readFormatPatch{};
    CodeFunctionPatch lengthPatch{};
    CodeFunctionPatch infoPatch{};
    CodeFunctionPatch locatePatch{};
    CodeFunctionPatch sendPatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamOpenFromFileA),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamOpenFromFileA),
                           openPatch)) {
        return 1;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamReadFormat),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamReadFormat),
                           readFormatPatch)) {
        RestoreFunctionPatch(openPatch);
        return 2;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamLength),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamLength), lengthPatch)) {
        RestoreFunctionPatch(readFormatPatch);
        RestoreFunctionPatch(openPatch);
        return 3;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamInfoA),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamInfoA), infoPatch)) {
        RestoreFunctionPatch(lengthPatch);
        RestoreFunctionPatch(readFormatPatch);
        RestoreFunctionPatch(openPatch);
        return 4;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&ICLocate),
                           reinterpret_cast<void *>(&FakeFmvICLocate), locatePatch)) {
        RestoreFunctionPatch(infoPatch);
        RestoreFunctionPatch(lengthPatch);
        RestoreFunctionPatch(readFormatPatch);
        RestoreFunctionPatch(openPatch);
        return 5;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&ICSendMessage),
                           reinterpret_cast<void *>(&FakeFmvICSendMessage), sendPatch)) {
        RestoreFunctionPatch(locatePatch);
        RestoreFunctionPatch(infoPatch);
        RestoreFunctionPatch(lengthPatch);
        RestoreFunctionPatch(readFormatPatch);
        RestoreFunctionPatch(openPatch);
        return 6;
    }

    const int oldBpp = g_zVideo_DisplayModeBpp;
    const unsigned int oldRMask = g_zVideo_PixelPack.rMask;
    const unsigned int oldGMask = g_zVideo_PixelPack.gMask;
    const unsigned int oldBMask = g_zVideo_PixelPack.bMask;
    g_zVideo_DisplayModeBpp = 16;
    g_zVideo_PixelPack.rMask = 0xf800;
    g_zVideo_PixelPack.gMask = 0x07e0;
    g_zVideo_PixelPack.bMask = 0x001f;

    g_fakeFmvStreamOpenCount = 0;
    g_fakeFmvOpenedStream = reinterpret_cast<PAVISTREAM>(0x24681357);
    g_fakeFmvOpenedPath = nullptr;
    g_fakeFmvOpenedType = 0;
    g_fakeFmvOpenedParam = 0;
    g_fakeFmvOpenedMode = 0;
    g_fakeFmvReadFormatCount = 0;
    g_fakeFmvReadFormatStream = nullptr;
    g_fakeFmvReadFormatPosition = -1;
    g_fakeFmvStreamLengthCount = 0;
    g_fakeFmvStreamLengthStream = nullptr;
    g_fakeFmvStreamInfoCount = 0;
    g_fakeFmvStreamInfoStream = nullptr;
    g_fakeFmvStreamInfoSize = 0;
    g_fakeFmvIcLocateCount = 0;
    g_fakeFmvIcLocateType = 0;
    g_fakeFmvIcLocateHandler = 0;
    g_fakeFmvIcLocateSrcFormat = nullptr;
    g_fakeFmvIcLocateDstFormat = nullptr;
    g_fakeFmvIcLocateMode = 0;
    g_fakeFmvLocatedCodec = reinterpret_cast<HIC>(0x11223344);
    g_fakeFmvIcSendMessageCount = 0;
    g_fakeFmvIcSendCodec = nullptr;
    g_fakeFmvIcSendMessage = 0;
    g_fakeFmvIcSendSrcParam = 0;
    g_fakeFmvIcSendDstParam = 0;

    alignas(8) std::uint8_t storage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);
    TestFieldAt<char *>(stream, 0x38) = const_cast<char *>("success.avi");
    TestFieldAt<std::int32_t>(stream, 0x104) = 0x12345678;

    stream->Constructor();

    BITMAPINFOHEADER *const src = TestFieldAt<BITMAPINFOHEADER *>(stream, 0x44);
    BITMAPV4HEADER *const dst = TestFieldAt<BITMAPV4HEADER *>(stream, 0x48);
    const DWORD testHandler = mmioFOURCC('T', 'E', 'S', 'T');
    const bool providerOk =
        g_fakeFmvStreamOpenCount == 1 &&
        g_fakeFmvOpenedPath == TestFieldAt<char *>(stream, 0x38) &&
        g_fakeFmvOpenedType == streamtypeVIDEO && g_fakeFmvOpenedParam == 0 &&
        g_fakeFmvOpenedMode == 0x10 && g_fakeFmvReadFormatCount == 2 &&
        g_fakeFmvReadFormatStream == g_fakeFmvOpenedStream &&
        g_fakeFmvReadFormatPosition == 0 && g_fakeFmvStreamLengthCount == 1 &&
        g_fakeFmvStreamLengthStream == g_fakeFmvOpenedStream &&
        g_fakeFmvStreamInfoCount == 1 &&
        g_fakeFmvStreamInfoStream == g_fakeFmvOpenedStream &&
        g_fakeFmvStreamInfoSize == 0x8c && g_fakeFmvIcLocateCount == 1 &&
        g_fakeFmvIcLocateType == ICTYPE_VIDEO &&
        g_fakeFmvIcLocateHandler == testHandler && g_fakeFmvIcLocateSrcFormat == src &&
        g_fakeFmvIcLocateDstFormat == dst && g_fakeFmvIcLocateMode == ICMODE_DECOMPRESS &&
        g_fakeFmvIcSendMessageCount == 1 &&
        g_fakeFmvIcSendCodec == g_fakeFmvLocatedCodec &&
        g_fakeFmvIcSendMessage == ICM_DECOMPRESS_BEGIN &&
        g_fakeFmvIcSendSrcParam == reinterpret_cast<DWORD_PTR>(src) &&
        g_fakeFmvIcSendDstParam == reinterpret_cast<DWORD_PTR>(dst);

    const bool headerZeros =
        TestFieldAt<std::uint32_t>(stream, 0x08) == 0 &&
        TestFieldAt<void *>(stream, 0x14) == nullptr &&
        TestFieldAt<void *>(stream, 0x18) == nullptr &&
        TestFieldAt<void *>(stream, 0x1c) == nullptr &&
        TestFieldAt<void *>(stream, 0x20) == nullptr &&
        TestFieldAt<void *>(stream, 0x24) == nullptr &&
        TestFieldAt<void *>(stream, 0x28) == nullptr &&
        TestFieldAt<void *>(stream, 0x2c) == nullptr &&
        TestFieldAt<void *>(stream, 0x30) == nullptr;

    const bool layoutOk =
        TestFieldAt<PAVISTREAM>(stream, 0x40) == g_fakeFmvOpenedStream && src != nullptr &&
        dst != nullptr && src->biSize == sizeof(BITMAPINFOHEADER) && src->biWidth == 5 &&
        src->biHeight == 4 && src->biBitCount == 16 &&
        TestFieldAt<std::int32_t>(stream, 0x4c) == 9 &&
        TestFieldAt<std::uint32_t>(stream, 0x54) == testHandler &&
        TestFieldAt<std::uint32_t>(stream, 0x64) == 1 &&
        TestFieldAt<std::uint32_t>(stream, 0x68) == 30 &&
        TestFieldAt<std::uint32_t>(stream, 0x78) == 48 &&
        dst->bV4Size == sizeof(BITMAPV4HEADER) && dst->bV4Width == 5 &&
        dst->bV4Height == -4 && dst->bV4Planes == 1 && dst->bV4BitCount == 16 &&
        dst->bV4V4Compression == BI_BITFIELDS && dst->bV4SizeImage == 64 &&
        dst->bV4RedMask == 0xf800 && dst->bV4GreenMask == 0x07e0 &&
        dst->bV4BlueMask == 0x001f && dst->bV4AlphaMask == 0 &&
        TestFieldAt<std::int32_t>(stream, 0xdc) == 48 &&
        TestFieldAt<HIC>(stream, 0xe0) == g_fakeFmvLocatedCodec &&
        TestFieldAt<void *>(stream, 0xe4) != nullptr &&
        TestFieldAt<std::int32_t>(stream, 0xe8) == 10 &&
        TestFieldAt<std::uint32_t>(stream, 0xec) == 30 &&
        TestFieldAt<std::uint32_t>(stream, 0xf0) == 33 &&
        TestFieldAt<std::int32_t>(stream, 0xf4) == 0 &&
        TestFieldAt<std::int32_t>(stream, 0xf8) == 0 &&
        TestFieldAt<std::int32_t>(stream, 0xfc) == 5 &&
        TestFieldAt<std::int32_t>(stream, 0x100) == 4 &&
        TestFieldAt<std::int32_t>(stream, 0x104) == 0 &&
        TestFieldAt<void *>(stream, 0x10) != nullptr &&
        TestFieldAt<std::int32_t>(stream, 0) == 64 &&
        TestFieldAt<std::int16_t>(stream, 4) == 5 &&
        TestFieldAt<std::int16_t>(stream, 6) == 4 &&
        TestFieldAt<std::int32_t>(stream, 0x34) == 5 &&
        TestFieldAt<std::int32_t>(stream, 0x3c) == 1 && headerZeros;

    std::free(TestFieldAt<void *>(stream, 0x44));
    std::free(TestFieldAt<void *>(stream, 0x48));
    std::free(TestFieldAt<void *>(stream, 0xe4));
    std::free(TestFieldAt<void *>(stream, 0x10));

    g_zVideo_DisplayModeBpp = oldBpp;
    g_zVideo_PixelPack.rMask = oldRMask;
    g_zVideo_PixelPack.gMask = oldGMask;
    g_zVideo_PixelPack.bMask = oldBMask;
    RestoreFunctionPatch(sendPatch);
    RestoreFunctionPatch(locatePatch);
    RestoreFunctionPatch(infoPatch);
    RestoreFunctionPatch(lengthPatch);
    RestoreFunctionPatch(readFormatPatch);
    RestoreFunctionPatch(openPatch);
    return providerOk && layoutOk ? 0 : 7;
}

extern "C" int zfmv_stream_open_audio_missing_file_smoke(void) {
    alignas(8) std::uint8_t storage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    TestFieldAt<char *>(stream, 0x38) = const_cast<char *>("__missing_stream_audio__.avi");
    TestFieldAt<void *>(stream, 0x134) = reinterpret_cast<void *>(0x11111111);
    TestFieldAt<std::int32_t>(stream, 0x130) = 0x22222222;
    TestFieldAt<std::int32_t>(stream, 0x1e0) = 5;

    stream->OpenAudio();

    return TestFieldAt<void *>(stream, 0x134) == nullptr &&
                   TestFieldAt<std::int32_t>(stream, 0x130) == 0x22222222 &&
                   TestFieldAt<std::int32_t>(stream, 0x1e0) == 5
               ? 0
               : 1;
}

extern "C" int zfmv_stream_init_missing_file_smoke(void) {
    alignas(8) std::uint8_t storage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(storage);

    zFMV_Stream *const returned = stream->Init("__missing_stream_init__.avi", 7);
    const bool ok =
        returned == stream && TestFieldAt<char *>(stream, 0x38) != nullptr &&
        std::strcmp(TestFieldAt<char *>(stream, 0x38), "__missing_stream_init__.avi") == 0 &&
        TestFieldAt<std::int32_t>(stream, 0x1d4) == 1 &&
        TestFieldAt<std::int32_t>(stream, 0x1e0) == 7 &&
        TestFieldAt<std::int32_t>(stream, 0x130) == 0 &&
        TestFieldAt<std::int32_t>(stream, 0x3c) == 0 &&
        TestFieldAt<std::int32_t>(stream, 0x104) == 0;

    stream->Destructor();
    return ok ? 0 : 1;
}

extern "C" int zfmv_stream_fill_audio_buffer_smoke(void) {
    CodeFunctionPatch aviPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamRead),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamRead), aviPatch)) {
        return 1;
    }

    const int oldBackend = g_zSnd_ActiveBackend;
    g_zSnd_ActiveBackend = 0;

    unsigned char streamStorage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    zSndSample sample = {};
    TestFmvDirectSoundBufferVTable bufferVTable = {};
    bufferVTable.Lock = &TestFmvLockSoundBuffer;
    bufferVTable.Unlock = &TestFmvUnlockSoundBuffer;
    TestFmvDirectSoundBuffer soundBuffer{&bufferVTable};
    unsigned char span1[8] = {};
    unsigned char span2[8] = {};
    PAVISTREAM const aviStream = reinterpret_cast<PAVISTREAM>(0x13572468);

    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&soundBuffer);
    TestFieldAt<PAVISTREAM>(stream, 0x134) = aviStream;
    TestFieldAt<std::uint32_t>(stream, 0x168) = 2;
    TestFieldAt<zSndSample *>(stream, 0x1d0) = &sample;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    g_fakeFmvLockResult = 0x12345678;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 5;
    const bool lockFailure =
        stream->FillAudioBuffer(10, 12) == 0 && g_fakeFmvLockCount == 1 &&
        g_fakeFmvUnlockCount == 0 && g_fakeAviStreamReadCount == 0 &&
        TestFieldAt<std::uint32_t>(stream, 0x1d8) == 5;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 5;
    const bool firstSpan =
        stream->FillAudioBuffer(10, 12) == 1 && g_fakeFmvLockCount == 1 &&
        g_fakeFmvUnlockCount == 1 && g_fakeFmvLastLockOffset == 10 &&
        g_fakeFmvLastLockBytes == 12 && g_fakeFmvLastLockFlags == 0 &&
        g_fakeAviStreamReadCount == 1 && g_fakeAviStreams[0] == aviStream &&
        g_fakeAviStarts[0] == 5 && g_fakeAviSamples[0] == 2 &&
        g_fakeAviBuffers[0] == span1 && g_fakeAviBufferBytes[0] == 4 &&
        g_fakeFmvUnlockPtr1 == span1 && g_fakeFmvUnlockBytes1 == 4 &&
        g_fakeFmvUnlockPtr2 == nullptr && g_fakeFmvUnlockBytes2 == 0 &&
        TestFieldAt<std::uint32_t>(stream, 0x1d8) == 7;

    ResetFmvAudioFillFakes(span1, 4, span2, 6);
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 3;
    const bool wrappedSpans =
        stream->FillAudioBuffer(20, 14) == 1 && g_fakeAviStreamReadCount == 2 &&
        g_fakeAviStarts[0] == 3 && g_fakeAviSamples[0] == 2 &&
        g_fakeAviBuffers[0] == span1 && g_fakeAviBufferBytes[0] == 4 &&
        g_fakeAviStarts[1] == 5 && g_fakeAviSamples[1] == 3 &&
        g_fakeAviBuffers[1] == span2 && g_fakeAviBufferBytes[1] == 6 &&
        g_fakeFmvUnlockPtr1 == span1 && g_fakeFmvUnlockBytes1 == 4 &&
        g_fakeFmvUnlockPtr2 == span2 && g_fakeFmvUnlockBytes2 == 6 &&
        TestFieldAt<std::uint32_t>(stream, 0x1d8) == 7;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    g_fakeAviReturn = 0x80004005;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 9;
    const bool aviFailureStillUnlocks =
        stream->FillAudioBuffer(0, 4) == 1 && g_fakeAviStreamReadCount == 1 &&
        g_fakeFmvUnlockCount == 1 && TestFieldAt<std::uint32_t>(stream, 0x1d8) == 11;

    ResetFmvAudioFillFakes(span1, 4, nullptr, 0);
    g_fakeFmvUnlockResult = 0x12345678;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 1;
    const bool unlockFailure =
        stream->FillAudioBuffer(0, 4) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeFmvUnlockCount == 1 && TestFieldAt<std::uint32_t>(stream, 0x1d8) == 3;

    g_zSnd_ActiveBackend = oldBackend;
    RestoreFunctionPatch(aviPatch);
    return lockFailure && firstSpan && wrappedSpans && aviFailureStillUnlocks && unlockFailure
               ? 0
               : 2;
}

extern "C" int zfmv_stream_read_and_decode_frame_smoke(void) {
    CodeFunctionPatch aviPatch{};
    CodeFunctionPatch icPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamRead),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamRead), aviPatch)) {
        return 1;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&ICDecompress),
                           reinterpret_cast<void *>(&FakeFmvICDecompress), icPatch)) {
        RestoreFunctionPatch(aviPatch);
        return 2;
    }

    const int oldBackend = g_zSnd_ActiveBackend;
    g_zSnd_ActiveBackend = 0;

    unsigned char streamStorage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    unsigned char compressedFrame[16] = {};
    unsigned char pixels[16] = {};
    BITMAPINFOHEADER srcFormat = {};
    BITMAPINFOHEADER dstFormat = {};
    PAVISTREAM const videoStream = reinterpret_cast<PAVISTREAM>(0x24681357);
    PAVISTREAM const audioStream = reinterpret_cast<PAVISTREAM>(0x13572468);
    HIC const codec = reinterpret_cast<HIC>(0x11223344);

    TestFieldAt<void *>(stream, 0x10) = pixels;
    TestFieldAt<PAVISTREAM>(stream, 0x40) = videoStream;
    TestFieldAt<BITMAPINFOHEADER *>(stream, 0x44) = &srcFormat;
    TestFieldAt<BITMAPINFOHEADER *>(stream, 0x48) = &dstFormat;
    TestFieldAt<std::uint32_t>(stream, 0x4c) = 3;
    TestFieldAt<int>(stream, 0xdc) = sizeof(compressedFrame);
    TestFieldAt<HIC>(stream, 0xe0) = codec;
    TestFieldAt<void *>(stream, 0xe4) = compressedFrame;
    InitializeCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    const bool videoDecode =
        stream->ReadAndDecodeFrame(1) == 2 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStreams[0] == videoStream && g_fakeAviStarts[0] == 1 &&
        g_fakeAviSamples[0] == 1 && g_fakeAviBuffers[0] == compressedFrame &&
        g_fakeAviBufferBytes[0] == sizeof(compressedFrame) &&
        g_fakeIcDecompressCount == 1 && g_fakeIcLastCodec == codec &&
        g_fakeIcLastFlags == 0 && g_fakeIcLastSrcFormat == &srcFormat &&
        g_fakeIcLastCompressedFrame == compressedFrame &&
        g_fakeIcLastDstFormat == &dstFormat && g_fakeIcLastPixels == pixels &&
        TestFieldAt<std::uint32_t>(stream, 0x104) == 2;

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    const bool frameWrap =
        stream->ReadAndDecodeFrame(2) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStarts[0] == 2 && g_fakeIcDecompressCount == 1 &&
        TestFieldAt<std::uint32_t>(stream, 0x104) == 0;

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    g_fakeAviReturn = 0x80004005;
    TestFieldAt<std::uint32_t>(stream, 0x104) = 0;
    const bool videoReadFailure =
        stream->ReadAndDecodeFrame(0) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeIcDecompressCount == 0 && TestFieldAt<std::uint32_t>(stream, 0x104) == 0;

    ResetFmvAudioFillFakes(nullptr, 0, nullptr, 0);
    g_fakeIcReturn = 1;
    const bool decompressFailure =
        stream->ReadAndDecodeFrame(0) == 0 && g_fakeAviStreamReadCount == 1 &&
        g_fakeIcDecompressCount == 1 && TestFieldAt<std::uint32_t>(stream, 0x104) == 0;
    LeaveCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));

    TestFmvDirectSoundBufferVTable bufferVTable = {};
    bufferVTable.GetCurrentPosition = &TestFmvGetCurrentPosition;
    bufferVTable.Lock = &TestFmvLockSoundBuffer;
    bufferVTable.Unlock = &TestFmvUnlockSoundBuffer;
    TestFmvDirectSoundBuffer soundBuffer{&bufferVTable};
    zSndSample sample = {};
    unsigned char audioSpan[8] = {};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&soundBuffer);
    TestFieldAt<PAVISTREAM>(stream, 0x134) = audioStream;
    TestFieldAt<int>(stream, 0x130) = 1;
    TestFieldAt<std::uint32_t>(stream, 0x168) = 2;
    TestFieldAt<zSndSample *>(stream, 0x1d0) = &sample;
    TestFieldAt<int>(stream, 0x1d4) = 0;
    TestFieldAt<int>(stream, 0x1e0) = 1;
    TestFieldAt<std::uint32_t>(stream, 0x1c8) = 8;
    TestFieldAt<std::uint32_t>(stream, 0x4c) = 0;

    ResetFmvAudioFillFakes(audioSpan, 4, nullptr, 0);
    g_fakeFmvPlayCursor = 9;
    TestFieldAt<int>(stream, 0x1dc) = 0;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 4;
    const bool firstHalfRefill =
        stream->ReadAndDecodeFrame(0xffffffffu) == 0 && g_fakeFmvGetCurrentPositionCount == 1 &&
        g_fakeFmvLockCount == 1 && g_fakeFmvLastLockOffset == 0 &&
        g_fakeFmvLastLockBytes == 8 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStreams[0] == audioStream && TestFieldAt<int>(stream, 0x1dc) == 1;

    ResetFmvAudioFillFakes(audioSpan, 4, nullptr, 0);
    g_fakeFmvPlayCursor = 4;
    TestFieldAt<int>(stream, 0x1dc) = 1;
    TestFieldAt<std::uint32_t>(stream, 0x1d8) = 6;
    const bool secondHalfRefill =
        stream->ReadAndDecodeFrame(0xffffffffu) == 0 && g_fakeFmvGetCurrentPositionCount == 1 &&
        g_fakeFmvLockCount == 1 && g_fakeFmvLastLockOffset == 8 &&
        g_fakeFmvLastLockBytes == 8 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStreams[0] == audioStream && TestFieldAt<int>(stream, 0x1dc) == 0;

    DeleteCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));
    g_zSnd_ActiveBackend = oldBackend;
    RestoreFunctionPatch(icPatch);
    RestoreFunctionPatch(aviPatch);
    return videoDecode && frameWrap && videoReadFailure && decompressFailure &&
                   firstHalfRefill && secondHalfRefill
               ? 0
               : 3;
}

extern "C" int zfmv_action_play_avi_constructor_existing_file_smoke(void) {
    const char *fileName = "recoil_playavi_ctor_smoke.tmp";
    FILE *file = std::fopen(fileName, "wb");
    if (file == nullptr) {
        return 1;
    }
    std::fclose(file);

    zFMV_ActionPlayAvi action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);

    zFMV_ActionPlayAvi *returned = new (&action) zFMV_ActionPlayAvi(".", fileName, 5);
    const bool ok = returned == &action &&
                    action.vftable == &g_zFMV_ActionPlayAvi_Vtable &&
                    action.next == nullptr && action.mediaPath != nullptr &&
                    std::strcmp(action.mediaPath, ".\\recoil_playavi_ctor_smoke.tmp") == 0 &&
                    action.modeFlags == 5;

    std::free(action.mediaPath);
    std::remove(fileName);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_play_avi_constructor_drive_fallback_smoke(void) {
    CodeFunctionPatch findPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zSys::FindFileOnDriveType),
                           reinterpret_cast<void *>(&FakeFmvFindFileOnDriveType),
                           findPatch)) {
        return 1;
    }

    char resolvedPath[] = "D:\\CD\\intro.avi";
    zFMV_ActionPlayAvi action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    g_fakeFmvFindFileCount = 0;
    g_fakeFmvFindFileDriveType = 0;
    g_fakeFmvFindFileRelativePath[0] = '\0';
    g_fakeFmvFindFileUnused = -1;
    g_fakeFmvFindFileResult = resolvedPath;

    zFMV_ActionPlayAvi *returned =
        new (&action) zFMV_ActionPlayAvi(
            "missingroot",
            "__recoil_missing_playavi_ctor__.avi",
            7
        );

    const bool ok =
        returned == &action && action.vftable == &g_zFMV_ActionPlayAvi_Vtable &&
        action.next == nullptr && action.mediaPath != nullptr &&
        std::strcmp(action.mediaPath, resolvedPath) == 0 && action.modeFlags == 7 &&
        g_fakeFmvFindFileCount == 1 && g_fakeFmvFindFileDriveType == DRIVE_CDROM &&
        std::strcmp(g_fakeFmvFindFileRelativePath,
                    "missingroot\\__recoil_missing_playavi_ctor__.avi") == 0 &&
        g_fakeFmvFindFileUnused == 0;

    std::free(action.mediaPath);
    RestoreFunctionPatch(findPatch);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_play_avi_lifecycle_smoke(void) {
    zFMV_ActionPlayAvi action{};
    action.vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x11111111);
    action.mediaPath = static_cast<char *>(std::malloc(6));
    action.modeFlags = 9;
    if (action.mediaPath == nullptr) {
        return 1;
    }
    std::strcpy(action.mediaPath, "intro");

    action.Destructor();
    if (action.vftable != &g_zFMV_ActionBase_Vtable || action.mediaPath != nullptr ||
        action.modeFlags != 9) {
        std::free(action.mediaPath);
        return 2;
    }

    action.vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x22222222);
    if (action.ScalarDeletingDestructor(0) != &action ||
        action.vftable != &g_zFMV_ActionBase_Vtable || action.mediaPath != nullptr) {
        return 3;
    }

    CodeFunctionPatch deletePatch{};
    void(__cdecl *operatorDeleteFn)(void *) = &operator delete;
    if (!PatchFunctionJump(reinterpret_cast<void *>(operatorDeleteFn),
                           reinterpret_cast<void *>(&FakeFmvOperatorDelete), deletePatch)) {
        return 4;
    }

    void *const storage = std::malloc(sizeof(zFMV_ActionPlayAvi));
    if (storage == nullptr) {
        RestoreFunctionPatch(deletePatch);
        return 5;
    }

    zFMV_ActionPlayAvi *const heapAction = reinterpret_cast<zFMV_ActionPlayAvi *>(storage);
    std::memset(heapAction, 0, sizeof(*heapAction));
    heapAction->vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x33333333);
    heapAction->mediaPath = static_cast<char *>(std::malloc(6));
    if (heapAction->mediaPath == nullptr) {
        RestoreFunctionPatch(deletePatch);
        std::free(storage);
        return 6;
    }
    std::strcpy(heapAction->mediaPath, "intro");
    g_fakeFmvOperatorDeleteCount = 0;
    g_fakeFmvOperatorDeletePtr = nullptr;

    zFMV_ActionPlayAvi *const deletingResult = heapAction->ScalarDeletingDestructor(1);
    const bool deletingOk =
        deletingResult == heapAction && heapAction->vftable == &g_zFMV_ActionBase_Vtable &&
        heapAction->mediaPath == nullptr && g_fakeFmvOperatorDeleteCount == 1 &&
        g_fakeFmvOperatorDeletePtr == heapAction;

    RestoreFunctionPatch(deletePatch);
    std::free(storage);
    return deletingOk ? 0 : 7;
}

extern "C" int zfmv_action_play_avi_update_smoke(void) {
    CodeFunctionPatch readPatch{};
    CodeFunctionPatch decompressPatch{};
    CodeFunctionPatch postPatch{};
    CodeFunctionPatch unlockPatch{};
    CodeFunctionPatch adjustPatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&AVIStreamRead),
                           reinterpret_cast<void *>(&FakeFmvAVIStreamRead), readPatch)) {
        return 1;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&ICDecompress),
                           reinterpret_cast<void *>(&FakeFmvICDecompress), decompressPatch)) {
        RestoreFunctionPatch(readPatch);
        return 2;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                           reinterpret_cast<void *>(&FakeFmvRunPostprocessOnPrimaryBuffer),
                           postPatch)) {
        RestoreFunctionPatch(decompressPatch);
        RestoreFunctionPatch(readPatch);
        return 3;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                           reinterpret_cast<void *>(&FakeFmvDispatchUnlockPrimarySurfaceState),
                           unlockPatch)) {
        RestoreFunctionPatch(postPatch);
        RestoreFunctionPatch(decompressPatch);
        RestoreFunctionPatch(readPatch);
        return 4;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
                           reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
                           adjustPatch)) {
        RestoreFunctionPatch(unlockPatch);
        RestoreFunctionPatch(postPatch);
        RestoreFunctionPatch(decompressPatch);
        RestoreFunctionPatch(readPatch);
        return 5;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const unsigned int oldSwBlt = g_zVideo_pfnBltSwToPrimaryRect;
    g_zVideo_pfnBltSwToPrimaryRect =
        reinterpret_cast<unsigned int>(&FakeFmvBltSwToPrimaryRect);

    alignas(8) std::uint8_t streamStorage[0x1e4] = {};
    zFMV_Stream *const stream = reinterpret_cast<zFMV_Stream *>(streamStorage);
    unsigned char compressedFrame[8] = {};
    unsigned char pixels[16] = {};
    BITMAPINFOHEADER srcFormat = {};
    BITMAPINFOHEADER dstFormat = {};
    TestFieldAt<PAVISTREAM>(stream, 0x40) = reinterpret_cast<PAVISTREAM>(0x12345678);
    TestFieldAt<BITMAPINFOHEADER *>(stream, 0x44) = &srcFormat;
    TestFieldAt<BITMAPINFOHEADER *>(stream, 0x48) = &dstFormat;
    TestFieldAt<int>(stream, 0x4c) = 8;
    TestFieldAt<HIC>(stream, 0xe0) = reinterpret_cast<HIC>(0x87654321);
    TestFieldAt<void *>(stream, 0xe4) = compressedFrame;
    TestFieldAt<int>(stream, 0xdc) = sizeof(compressedFrame);
    TestFieldAt<void *>(stream, 0x10) = pixels;
    TestFieldAt<int>(stream, 0xec) = 4;
    TestFieldAt<int>(stream, 0x130) = 0;
    InitializeCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));

    zFMV_ActionPlayAvi action{};
    action.stream = stream;
    action.lastDecodedFrameIndex = -1;
    action.destRect.left = 11;
    action.destRect.top = 22;
    action.destRect.right = 333;
    action.destRect.bottom = 444;

    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvSwBltCount = 0;
    g_fakeAviStreamReadCount = 0;
    g_fakeAviReturn = 0;
    g_fakeIcDecompressCount = 0;
    g_fakeIcReturn = 0;
    g_zVideo_ActiveRendererPath = 0;

    const bool softwareOk =
        action.Update(1.0) == 1 && action.startTimeSec == 1.0 &&
        action.lastDecodedFrameIndex == 0 && g_fakeFmvPostprocessCount == 1 &&
        g_fakeFmvUnlockPrimaryCount == 1 && g_fakeAviStreamReadCount == 1 &&
        g_fakeAviStarts[0] == 0 && g_fakeIcDecompressCount == 1 &&
        g_fakeFmvSwBltCount == 1 &&
        g_fakeFmvSwBltImage == reinterpret_cast<zVidImagePartial *>(stream) &&
        g_fakeFmvSwBltColorKeyEnable == 0 && g_fakeFmvSwBltSrcRect == nullptr &&
        g_fakeFmvSwBltDstRect == reinterpret_cast<zVidRect32 *>(&action.destRect) &&
        g_fakeFmvAdjustSurfacesCount == 1 &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 1 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 1;

    const bool sameFrameOk =
        action.Update(1.0) == 1 && action.lastDecodedFrameIndex == 0 &&
        g_fakeAviStreamReadCount == 1 && g_fakeFmvSwBltCount == 1 &&
        g_fakeFmvAdjustSurfacesCount == 1;

    g_zVideo_ActiveRendererPath = 2;
    const bool hardwareOk =
        action.Update(1.5) == 3 && action.lastDecodedFrameIndex == 2 &&
        g_fakeFmvPostprocessCount == 1 && g_fakeFmvUnlockPrimaryCount == 1 &&
        g_fakeAviStreamReadCount == 2 && g_fakeAviStarts[1] == 2 &&
        g_fakeFmvSwBltCount == 2 && g_fakeFmvAdjustSurfacesCount == 2 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 0;

    g_fakeAviReturn = 0x80004005;
    const bool failureOk =
        action.Update(2.0) == 0 && action.lastDecodedFrameIndex == 4 &&
        g_fakeAviStreamReadCount == 3 && g_fakeAviStarts[2] == 4 &&
        g_fakeFmvSwBltCount == 3 && g_fakeFmvAdjustSurfacesCount == 3;

    DeleteCriticalSection(&TestFieldAt<CRITICAL_SECTION>(stream, 0x108));
    g_zVideo_pfnBltSwToPrimaryRect = oldSwBlt;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(unlockPatch);
    RestoreFunctionPatch(postPatch);
    RestoreFunctionPatch(decompressPatch);
    RestoreFunctionPatch(readPatch);
    return softwareOk && sameFrameOk && hardwareOk && failureOk ? 0 : 6;
}

extern "C" int zfmv_action_play_avi_begin_end_smoke(void) {
    const int oldWidth = zRndr::g_activeRegionWidth;
    const int oldHeight = zRndr::g_activeRegionHeight;
    const int oldPitch = zRndr::g_pitchBytes;
    const int oldBytesPerPixel = zRndr::g_bytesPerPixel;
    zRndr::g_activeRegionWidth = 640;
    zRndr::g_activeRegionHeight = 360;
    zRndr::g_pitchBytes = 1280;
    zRndr::g_bytesPerPixel = 2;

    zFMV_ActionPlayAvi action{};
    action.mediaPath = const_cast<char *>("__missing_playavi_begin__.avi");
    action.modeFlags = 7;
    action.stream = reinterpret_cast<zFMV_Stream *>(0x11111111);
    action.lastDecodedFrameIndex = 123;

    action.Begin(25.0);
    zFMV_Stream *const initializedStream = action.stream;
    const bool beginOk =
        initializedStream != nullptr && action.destRect.left == 0 && action.destRect.top == 0 &&
        action.destRect.right == 640 && action.destRect.bottom == 360 &&
        action.lastDecodedFrameIndex == -1;

    CodeFunctionPatch deletePatch{};
    void(__cdecl *operatorDeleteFn)(void *) = &operator delete;
    if (!PatchFunctionJump(reinterpret_cast<void *>(operatorDeleteFn),
                           reinterpret_cast<void *>(&FakeFmvOperatorDelete), deletePatch)) {
        action.End();
        zRndr::g_activeRegionWidth = oldWidth;
        zRndr::g_activeRegionHeight = oldHeight;
        zRndr::g_pitchBytes = oldPitch;
        zRndr::g_bytesPerPixel = oldBytesPerPixel;
        return 1;
    }

    g_fakeFmvOperatorDeleteCount = 0;
    g_fakeFmvOperatorDeletePtr = nullptr;
    action.End();
    const bool endOk = action.stream == nullptr && g_fakeFmvOperatorDeleteCount == 1 &&
                       g_fakeFmvOperatorDeletePtr == initializedStream;

    RestoreFunctionPatch(deletePatch);
    if (initializedStream != nullptr) {
        ::operator delete(initializedStream);
    }
    action.End();

    zRndr::g_activeRegionWidth = oldWidth;
    zRndr::g_activeRegionHeight = oldHeight;
    zRndr::g_pitchBytes = oldPitch;
    zRndr::g_bytesPerPixel = oldBytesPerPixel;
    return beginOk && endOk && action.stream == nullptr ? 0 : 2;
}

extern "C" int zfmv_action_play_mci_constructor_smoke(void) {
    zRndr::g_activeRegionWidth = 1024;
    zRndr::g_activeRegionHeight = 768;
    zRndr::g_pitchBytes = 2048;
    zRndr::g_bytesPerPixel = 2;

    zFMV_ActionPlayMci action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);

    zFMV_ActionPlayMci *returned =
        new (&action) zFMV_ActionPlayMci(
            "movies",
            "intro.mci",
            reinterpret_cast<HWND>(0x2468ace0)
        );

    const bool ok =
        returned == &action && action.vftable == &g_zFMV_ActionPlayMci_Vtable &&
        action.next == nullptr && action.mediaPath != nullptr &&
        std::strcmp(action.mediaPath, "movies\\intro.mci") == 0 && action.playback != nullptr &&
        action.playback->mediaPathDup != nullptr &&
        std::strcmp(action.playback->mediaPathDup, "movies\\intro.mci") == 0 &&
        action.playback->notifyHwnd == reinterpret_cast<HWND>(0x2468ace0) &&
        action.playback->mciPutFlags == 0x40000 && action.playback->destinationRect.left == 0 &&
        action.playback->destinationRect.top == 0 &&
        action.playback->destinationRect.right == 1024 &&
        action.playback->destinationRect.bottom == 768 && g_zFMV_ActionPlayMci_DestRect.left == 0 &&
        g_zFMV_ActionPlayMci_DestRect.top == 0 && g_zFMV_ActionPlayMci_DestRect.right == 1024 &&
        g_zFMV_ActionPlayMci_DestRect.bottom == 768;

    std::free(action.playback->mediaPathDup);
    ::operator delete(action.playback);
    std::free(action.mediaPath);
    return ok ? 0 : 1;
}

extern "C" int zfmv_action_no_op_update_smoke(void) {
    zFMV_Action action{};
    action.vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x11111111);
    action.next = reinterpret_cast<zFMV_Action *>(0x22222222);

    const int result = action.NoOpUpdate(456.25);
    return result == 0 && action.vftable == reinterpret_cast<zFMV_Action_Vtbl *>(0x11111111) &&
                   action.next == reinterpret_cast<zFMV_Action *>(0x22222222)
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_mci_update_smoke(void) {
    zFMV_ActionPlayMci action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.vftable = &g_zFMV_ActionPlayMci_Vtable;
    action.mediaPath = reinterpret_cast<char *>(0x22222222);
    action.playback = reinterpret_cast<zFMV_Playback *>(0x33333333);

    const int result = action.Update(123.5);
    return result == 0 && action.next == reinterpret_cast<zFMV_Action *>(0x11111111) &&
                   action.vftable == &g_zFMV_ActionPlayMci_Vtable &&
                   action.mediaPath == reinterpret_cast<char *>(0x22222222) &&
                   action.playback == reinterpret_cast<zFMV_Playback *>(0x33333333)
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_mci_begin_smoke(void) {
    g_fakeFmvPlaybackOpenAndPlayCount = 0;
    g_fakeFmvPlaybackOpenAndPlaySelf = nullptr;
    g_fakeFmvPlaybackOpenAndPlayStartMs = 0;
    g_fakeFmvPlaybackOpenAndPlayEndMs = 0;
    g_fakeFmvPlaybackOpenAndPlayNotifyFlag = 0;

    CodeFunctionPatch openPatch{};
    if (!PatchFunctionJump(
            zFMV_Playback_OpenAndPlayProc(),
            FakeFmvPlaybackOpenAndPlayProc(),
            openPatch
        )) {
        return 1;
    }

    zFMV_ActionPlayMci emptyAction{};
    emptyAction.Begin(12.0);
    const bool nullOk = g_fakeFmvPlaybackOpenAndPlayCount == 0;

    zFMV_Playback playback{};
    zFMV_ActionPlayMci action{};
    action.playback = &playback;
    action.Begin(34.0);

    const bool playbackOk =
        g_fakeFmvPlaybackOpenAndPlayCount == 1 &&
        g_fakeFmvPlaybackOpenAndPlaySelf == &playback &&
        g_fakeFmvPlaybackOpenAndPlayStartMs == 0 &&
        g_fakeFmvPlaybackOpenAndPlayEndMs == -1 &&
        g_fakeFmvPlaybackOpenAndPlayNotifyFlag == 0;

    RestoreFunctionPatch(openPatch);
    return nullOk && playbackOk ? 0 : 2;
}

extern "C" int zfmv_action_play_mci_end_smoke(void) {
    CodeFunctionPatch lockPatch{};
    CodeFunctionPatch capturePatch{};
    CodeFunctionPatch unlockDisplayPatch{};
    CodeFunctionPatch postPatch{};
    CodeFunctionPatch blitPatch{};
    CodeFunctionPatch unlockPrimaryPatch{};
    CodeFunctionPatch adjustPatch{};
    CodeFunctionPatch releasePatch{};
    CodeFunctionPatch stopPatch{};

    const bool patched =
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::Dispatch_LockDisplayModeSurfaceState),
            reinterpret_cast<void *>(&FakeFmvDispatchLockDisplayModeSurfaceState),
            lockPatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_buff_CaptureSurfaceToImage),
            reinterpret_cast<void *>(&FakeFmvCaptureSurfaceToImage),
            capturePatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::Dispatch_UnlockDisplayModeSurfaceState),
            reinterpret_cast<void *>(&FakeFmvDispatchUnlockDisplayModeSurfaceState),
            unlockDisplayPatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&FakeFmvRunPostprocessOnPrimaryBuffer),
            postPatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVid_Image::BlitToActiveTarget),
            reinterpret_cast<void *>(&FakeFmvBlitToActiveTarget),
            blitPatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
            reinterpret_cast<void *>(&FakeFmvDispatchUnlockPrimarySurfaceState),
            unlockPrimaryPatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
            reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
            adjustPatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVid_Image::ReleaseIfNotDefault),
            reinterpret_cast<void *>(&FakeFmvReleaseImageIfNotDefault),
            releasePatch
        ) &&
        PatchFunctionJump(
            zFMV_Playback_StopAndCloseProc(),
            FakeFmvPlaybackStopAndCloseProc(),
            stopPatch
        );

    if (!patched) {
        RestoreFunctionPatch(stopPatch);
        RestoreFunctionPatch(releasePatch);
        RestoreFunctionPatch(adjustPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postPatch);
        RestoreFunctionPatch(unlockDisplayPatch);
        RestoreFunctionPatch(capturePatch);
        RestoreFunctionPatch(lockPatch);
        return 1;
    }

    zVidImagePartial capturedImage{};
    zFMV_Playback playback{};
    zFMV_ActionPlayMci action{};
    action.playback = &playback;

    g_fakeFmvDisplayLockCount = 0;
    g_fakeFmvDisplayUnlockCount = 0;
    g_fakeFmvCaptureSurfaceCount = 0;
    g_fakeFmvCaptureSurfaceSelector = 0;
    g_fakeFmvCaptureSurfaceResult = &capturedImage;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvBlitToActiveTargetImage = nullptr;
    g_fakeFmvBlitToActiveTargetDstX = -1;
    g_fakeFmvBlitToActiveTargetDstY = -1;
    g_fakeFmvBlitToActiveTargetClipFlags = -1;
    g_fakeFmvBlitToActiveTargetSrcRect = reinterpret_cast<zVidRect32 *>(0x11111111);
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvAdjustSurfacesSrcRect = reinterpret_cast<zVidRect32 *>(0x22222222);
    g_fakeFmvAdjustSurfacesDstRect = reinterpret_cast<zVidRect32 *>(0x33333333);
    g_fakeFmvAdjustSurfacesWaitForPresent = -1;
    g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst = -1;
    g_fakeFmvReleaseImageCount = 0;
    g_fakeFmvReleaseImage = nullptr;
    g_fakeFmvPlaybackStopAndCloseCount = 0;
    g_fakeFmvPlaybackStopAndCloseSelf = nullptr;

    action.End();
    const bool capturedOk =
        g_fakeFmvDisplayLockCount == 1 && g_fakeFmvDisplayUnlockCount == 1 &&
        g_fakeFmvCaptureSurfaceCount == 1 && g_fakeFmvCaptureSurfaceSelector == 2 &&
        g_fakeFmvPostprocessCount == 2 && g_fakeFmvBlitToActiveTargetCount == 2 &&
        g_fakeFmvBlitToActiveTargetImage == &capturedImage &&
        g_fakeFmvBlitToActiveTargetDstX == 0 && g_fakeFmvBlitToActiveTargetDstY == 0 &&
        g_fakeFmvBlitToActiveTargetClipFlags == 0 &&
        g_fakeFmvBlitToActiveTargetSrcRect == nullptr && g_fakeFmvUnlockPrimaryCount == 2 &&
        g_fakeFmvAdjustSurfacesCount == 2 && g_fakeFmvAdjustSurfacesSrcRect == nullptr &&
        g_fakeFmvAdjustSurfacesDstRect == nullptr &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 0 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 1 &&
        g_fakeFmvPlaybackStopAndCloseCount == 1 &&
        g_fakeFmvPlaybackStopAndCloseSelf == &playback && g_fakeFmvReleaseImageCount == 1 &&
        g_fakeFmvReleaseImage == &capturedImage;

    action.playback = nullptr;
    g_fakeFmvDisplayLockCount = 0;
    g_fakeFmvDisplayUnlockCount = 0;
    g_fakeFmvCaptureSurfaceCount = 0;
    g_fakeFmvCaptureSurfaceResult = nullptr;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlitToActiveTargetCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvReleaseImageCount = 0;
    g_fakeFmvPlaybackStopAndCloseCount = 0;

    action.End();
    const bool nullOk =
        g_fakeFmvDisplayLockCount == 1 && g_fakeFmvDisplayUnlockCount == 1 &&
        g_fakeFmvCaptureSurfaceCount == 1 && g_fakeFmvPostprocessCount == 0 &&
        g_fakeFmvBlitToActiveTargetCount == 0 && g_fakeFmvUnlockPrimaryCount == 0 &&
        g_fakeFmvAdjustSurfacesCount == 0 && g_fakeFmvReleaseImageCount == 0 &&
        g_fakeFmvPlaybackStopAndCloseCount == 0;

    RestoreFunctionPatch(stopPatch);
    RestoreFunctionPatch(releasePatch);
    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(unlockPrimaryPatch);
    RestoreFunctionPatch(blitPatch);
    RestoreFunctionPatch(postPatch);
    RestoreFunctionPatch(unlockDisplayPatch);
    RestoreFunctionPatch(capturePatch);
    RestoreFunctionPatch(lockPatch);
    return capturedOk && nullOk ? 0 : 2;
}

extern "C" int zfmv_action_play_mci_lifecycle_smoke(void) {
    CodeFunctionPatch deletePatch{};
    void(__cdecl *operatorDeleteFn)(void *) = &operator delete;
    if (!PatchFunctionJump(reinterpret_cast<void *>(operatorDeleteFn),
                           reinterpret_cast<void *>(&FakeFmvOperatorDelete), deletePatch)) {
        return 1;
    }

    zFMV_ActionPlayMci action{};
    action.vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x11111111);
    action.mediaPath = static_cast<char *>(std::malloc(6));
    action.playback = static_cast<zFMV_Playback *>(std::malloc(sizeof(zFMV_Playback)));
    if (action.mediaPath == nullptr || action.playback == nullptr) {
        RestoreFunctionPatch(deletePatch);
        std::free(action.mediaPath);
        std::free(action.playback);
        return 2;
    }
    std::strcpy(action.mediaPath, "movie");
    std::memset(action.playback, 0, sizeof(*action.playback));
    action.playback->mediaPathDup = static_cast<char *>(std::malloc(6));
    if (action.playback->mediaPathDup == nullptr) {
        RestoreFunctionPatch(deletePatch);
        std::free(action.mediaPath);
        std::free(action.playback);
        return 3;
    }
    std::strcpy(action.playback->mediaPathDup, "movie");

    zFMV_Playback *const directPlayback = action.playback;
    g_fakeFmvOperatorDeleteCount = 0;
    g_fakeFmvOperatorDeletePtr = nullptr;
    action.Destructor();
    const bool directOk =
        action.vftable == &g_zFMV_ActionBase_Vtable && action.mediaPath == nullptr &&
        action.playback == nullptr && g_fakeFmvOperatorDeleteCount == 1 &&
        g_fakeFmvOperatorDeletePtr == directPlayback;

    action.vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x22222222);
    const bool stackScalarOk = action.ScalarDeletingDestructor(0) == &action &&
                               action.vftable == &g_zFMV_ActionBase_Vtable &&
                               g_fakeFmvOperatorDeleteCount == 1;

    void *const actionStorage = std::malloc(sizeof(zFMV_ActionPlayMci));
    void *const playbackStorage = std::malloc(sizeof(zFMV_Playback));
    if (actionStorage == nullptr || playbackStorage == nullptr) {
        RestoreFunctionPatch(deletePatch);
        std::free(directPlayback);
        std::free(actionStorage);
        std::free(playbackStorage);
        return 4;
    }

    zFMV_ActionPlayMci *const heapAction =
        reinterpret_cast<zFMV_ActionPlayMci *>(actionStorage);
    std::memset(heapAction, 0, sizeof(*heapAction));
    heapAction->vftable = reinterpret_cast<zFMV_Action_Vtbl *>(0x33333333);
    heapAction->mediaPath = static_cast<char *>(std::malloc(6));
    heapAction->playback = reinterpret_cast<zFMV_Playback *>(playbackStorage);
    std::memset(heapAction->playback, 0, sizeof(*heapAction->playback));
    heapAction->playback->mediaPathDup = static_cast<char *>(std::malloc(6));
    if (heapAction->mediaPath == nullptr || heapAction->playback->mediaPathDup == nullptr) {
        RestoreFunctionPatch(deletePatch);
        std::free(directPlayback);
        std::free(heapAction->mediaPath);
        std::free(heapAction->playback->mediaPathDup);
        std::free(actionStorage);
        std::free(playbackStorage);
        return 5;
    }
    std::strcpy(heapAction->mediaPath, "movie");
    std::strcpy(heapAction->playback->mediaPathDup, "movie");

    g_fakeFmvOperatorDeleteCount = 0;
    g_fakeFmvOperatorDeletePtr = nullptr;
    zFMV_ActionPlayMci *const deletingResult = heapAction->ScalarDeletingDestructor(1);
    const bool heapScalarOk =
        deletingResult == heapAction && heapAction->vftable == &g_zFMV_ActionBase_Vtable &&
        heapAction->mediaPath == nullptr && heapAction->playback == nullptr &&
        g_fakeFmvOperatorDeleteCount == 2 && g_fakeFmvOperatorDeletePtr == heapAction;

    RestoreFunctionPatch(deletePatch);
    std::free(directPlayback);
    std::free(playbackStorage);
    std::free(actionStorage);
    return directOk && stackScalarOk && heapScalarOk ? 0 : 6;
}

extern "C" int zfmv_action_blur_constructor_smoke(void) {
    zFMV_ActionBlur action{};
    action.next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action.framesRemaining = 0x22222222;
    action.blurPassCount = 0x33333333;
    action.swSurfaceRect = {1, 2, 3, 4};
    action.primarySurfaceRect = {5, 6, 7, 8};

    zFMV_ActionBlur *returned = new (&action) zFMV_ActionBlur(12, 3);

    return returned == &action && action.vftable == &g_zFMV_ActionBlur_Vtable &&
                   action.next == nullptr && action.framesRemaining == 12 &&
                   action.blurPassCount == 3 && action.swSurfaceRect.left == 1 &&
                   action.swSurfaceRect.top == 2 && action.swSurfaceRect.right == 3 &&
                   action.swSurfaceRect.bottom == 4 && action.primarySurfaceRect.left == 5 &&
                   action.primarySurfaceRect.top == 6 && action.primarySurfaceRect.right == 7 &&
                   action.primarySurfaceRect.bottom == 8
               ? 0
               : 1;
}

extern "C" int zfmv_action_blur_begin_end_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_SurfaceStatePartial oldSw = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimary = g_zVideo_PrimarySurfaceState;
    zVideo_BltRectDirectProc oldSwToPrimary = g_zVideo_pfnBltSwToPrimaryRectDirect;
    zVideo_BltRectDirectProc oldPrimaryToSw = g_zVideo_pfnBltPrimaryToSwRectDirect;
    unsigned short swPixels[4] = {};
    unsigned short primaryPixels[4] = {};

    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 200;
    g_zVideo_SwSurfaceState.pitch = 640;
    g_zVideo_SwSurfaceState.pixels = swPixels;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;
    g_zVideo_PrimarySurfaceState.pitch = 1280;
    g_zVideo_PrimarySurfaceState.pixels = primaryPixels;
    g_zVideo_pfnBltSwToPrimaryRectDirect = &FakeFmvBltSwToPrimaryRectDirect;
    g_zVideo_pfnBltPrimaryToSwRectDirect = &FakeFmvBltPrimaryToSwRectDirect;

    zFMV_ActionBlur action{};
    g_zVideo_ActiveRendererPath = 0;
    g_fakeFmvSwToPrimaryDirectCount = 0;
    g_fakeFmvPrimaryToSwDirectCount = 0;
    action.Begin(1.0);
    const bool softwareOk =
        action.swSurfaceRect.left == 0 && action.swSurfaceRect.top == 0 &&
        action.swSurfaceRect.right == 320 && action.swSurfaceRect.bottom == 200 &&
        action.primarySurfaceRect.left == 0 && action.primarySurfaceRect.top == 0 &&
        action.primarySurfaceRect.right == 640 && action.primarySurfaceRect.bottom == 480 &&
        g_zVideo_FxSurfacePixels16 == swPixels && g_zVideo_FxSurfaceWidth == 320 &&
        g_zVideo_FxSurfaceHeight == 200 && g_zVideo_FxSurfacePitchBytes == 640 &&
        g_fakeFmvPrimaryToSwDirectCount == 1 &&
        g_fakeFmvPrimaryToSwDirectSrcRect ==
            reinterpret_cast<zVidRect32 *>(&action.primarySurfaceRect) &&
        g_fakeFmvPrimaryToSwDirectDstRect ==
            reinterpret_cast<zVidRect32 *>(&action.swSurfaceRect) &&
        g_fakeFmvSwToPrimaryDirectCount == 0;

    g_zVideo_ActiveRendererPath = 1;
    g_fakeFmvSwToPrimaryDirectCount = 0;
    g_fakeFmvPrimaryToSwDirectCount = 0;
    action.Begin(2.0);
    const bool hardwareOk =
        g_zVideo_FxSurfacePixels16 == primaryPixels && g_zVideo_FxSurfaceWidth == 320 &&
        g_zVideo_FxSurfaceHeight == 200 && g_zVideo_FxSurfacePitchBytes == 1280 &&
        g_fakeFmvSwToPrimaryDirectCount == 1 &&
        g_fakeFmvSwToPrimaryDirectSrcRect ==
            reinterpret_cast<zVidRect32 *>(&action.primarySurfaceRect) &&
        g_fakeFmvSwToPrimaryDirectDstRect ==
            reinterpret_cast<zVidRect32 *>(&action.swSurfaceRect) &&
        g_fakeFmvPrimaryToSwDirectCount == 0;

    action.End();
    const bool endOk =
        g_zVideo_FxSurfacePixels16 == primaryPixels && g_zVideo_FxSurfaceWidth == 640 &&
        g_zVideo_FxSurfaceHeight == 480 && g_zVideo_FxSurfacePitchBytes == 1280;

    g_zVideo_pfnBltPrimaryToSwRectDirect = oldPrimaryToSw;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldSwToPrimary;
    g_zVideo_SwSurfaceState = oldSw;
    g_zVideo_PrimarySurfaceState = oldPrimary;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    return softwareOk && hardwareOk && endOk ? 0 : 2;
}

extern "C" int zfmv_action_blur_update_smoke(void) {
    CodeFunctionPatch postPrimaryPatch{};
    CodeFunctionPatch postSwPatch{};
    CodeFunctionPatch blurPatch{};
    CodeFunctionPatch unlockPrimaryPatch{};
    CodeFunctionPatch unlockSwPatch{};
    CodeFunctionPatch adjustPatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                           reinterpret_cast<void *>(&FakeFmvRunPostprocessOnPrimaryBuffer),
                           postPrimaryPatch)) {
        return 1;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnSwBuffer),
                           reinterpret_cast<void *>(&FakeFmvRunPostprocessOnSwBuffer),
                           postSwPatch)) {
        RestoreFunctionPatch(postPrimaryPatch);
        return 2;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::buff_BlurRegionByMode),
                           reinterpret_cast<void *>(&FakeFmvBlurRegionByMode), blurPatch)) {
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 3;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                           reinterpret_cast<void *>(&FakeFmvDispatchUnlockPrimarySurfaceState),
                           unlockPrimaryPatch)) {
        RestoreFunctionPatch(blurPatch);
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 4;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockSwSurfaceState),
                           reinterpret_cast<void *>(&FakeFmvDispatchUnlockSwSurfaceState),
                           unlockSwPatch)) {
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blurPatch);
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 5;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
                           reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
                           adjustPatch)) {
        RestoreFunctionPatch(unlockSwPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blurPatch);
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 6;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    zVideo_BltRectDirectProc oldSwToPrimary = g_zVideo_pfnBltSwToPrimaryRectDirect;
    g_zVideo_pfnBltSwToPrimaryRectDirect = &FakeFmvBltSwToPrimaryRectDirect;

    zFMV_ActionBlur action{};
    action.framesRemaining = 2;
    action.blurPassCount = 2;
    action.swSurfaceRect = {1, 2, 3, 4};
    action.primarySurfaceRect = {5, 6, 7, 8};
    g_zVideo_ActiveRendererPath = 0;
    g_fakeFmvSwPostprocessCount = 0;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlurByModeCount = 0;
    g_fakeFmvBlurByModeRect = reinterpret_cast<zVidRect32 *>(0x11111111);
    std::memset(g_fakeFmvBlurByModeModes, 0, sizeof(g_fakeFmvBlurByModeModes));
    g_fakeFmvUnlockSwCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvSwToPrimaryDirectCount = 0;
    const bool softwareCombinedOk =
        action.Update(1.0) == 1 && action.framesRemaining == 1 &&
        g_fakeFmvSwPostprocessCount == 1 && g_fakeFmvPostprocessCount == 0 &&
        g_fakeFmvBlurByModeCount == 2 && g_fakeFmvBlurByModeRect == nullptr &&
        g_fakeFmvBlurByModeModes[0] == 3 && g_fakeFmvBlurByModeModes[1] == 3 &&
        g_fakeFmvUnlockSwCount == 1 && g_fakeFmvUnlockPrimaryCount == 0 &&
        g_fakeFmvSwToPrimaryDirectCount == 1 &&
        g_fakeFmvSwToPrimaryDirectSrcRect == reinterpret_cast<zVidRect32 *>(&action.swSurfaceRect) &&
        g_fakeFmvSwToPrimaryDirectDstRect ==
            reinterpret_cast<zVidRect32 *>(&action.primarySurfaceRect) &&
        g_fakeFmvAdjustSurfacesCount == 1 &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 1 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 1;

    action.framesRemaining = 1;
    action.blurPassCount = 0;
    g_zVideo_ActiveRendererPath = 1;
    g_fakeFmvSwPostprocessCount = 0;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlurByModeCount = 0;
    g_fakeFmvUnlockSwCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvSwToPrimaryDirectCount = 0;
    const bool hardwareCombinedOk =
        action.Update(2.0) == 0 && action.framesRemaining == 0 &&
        g_fakeFmvPostprocessCount == 1 && g_fakeFmvSwPostprocessCount == 0 &&
        g_fakeFmvBlurByModeCount == 0 && g_fakeFmvUnlockPrimaryCount == 1 &&
        g_fakeFmvUnlockSwCount == 0 && g_fakeFmvSwToPrimaryDirectCount == 0 &&
        g_fakeFmvAdjustSurfacesCount == 1;

    zFMV_ActionBlurH hAction{};
    hAction.framesRemaining = 2;
    hAction.blurPassCount = 1;
    g_zVideo_ActiveRendererPath = 0;
    g_fakeFmvBlurByModeCount = 0;
    hAction.Update(3.0);
    const bool horizontalOk =
        hAction.framesRemaining == 1 && g_fakeFmvBlurByModeCount == 1 &&
        g_fakeFmvBlurByModeModes[0] == 1;

    zFMV_ActionBlurV vAction{};
    vAction.framesRemaining = 2;
    vAction.blurPassCount = 1;
    g_zVideo_ActiveRendererPath = 1;
    g_fakeFmvBlurByModeCount = 0;
    vAction.Update(4.0);
    const bool verticalOk =
        vAction.framesRemaining == 1 && g_fakeFmvBlurByModeCount == 1 &&
        g_fakeFmvBlurByModeModes[0] == 2;

    g_zVideo_pfnBltSwToPrimaryRectDirect = oldSwToPrimary;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(unlockSwPatch);
    RestoreFunctionPatch(unlockPrimaryPatch);
    RestoreFunctionPatch(blurPatch);
    RestoreFunctionPatch(postSwPatch);
    RestoreFunctionPatch(postPrimaryPatch);
    return softwareCombinedOk && hardwareCombinedOk && horizontalOk && verticalOk ? 0 : 7;
}

extern "C" int zfmv_script_load_actions_from_zrd_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "fmv", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteArrayHeader(file, 7);
    WriteStringNode(file, "FMV_PATH");
    WriteStringNode(file, "movies");
    WriteStringNode(file, "IMAGE_PATH");
    WriteStringNode(file, "images");
    WriteStringNode(file, "INTRO");
    WriteArrayHeader(file, 4);
    WriteArrayHeader(file, 3);
    WriteStringNode(file, "WAIT");
    WriteFloatNode(file, 1.25f);
    WriteArrayHeader(file, 3);
    WriteStringNode(file, "BLURH");
    WriteIntNode(file, 4);
    WriteArrayHeader(file, 3);
    WriteStringNode(file, "PLAYSOUND");
    WriteStringNode(file, "intro_whoosh");
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "fmv.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;
    g_zArchive_MountedList = &list;

    zFMV_Script script{};
    script.Init(nullptr, nullptr, nullptr);
    const std::int32_t result = script.LoadActionsFromZrd("C:\\dummy\\fmv.zrd", "INTRO");

    auto *wait = static_cast<zFMV_ActionWait *>(script.m_head);
    auto *blur = static_cast<zFMV_ActionBlur *>(wait != nullptr ? wait->next : nullptr);
    auto *sound = static_cast<zFMV_ActionPlaySound *>(blur != nullptr ? blur->next : nullptr);

    const bool ok = result == 3 && script.m_fmvPath != nullptr &&
                    std::strcmp(script.m_fmvPath, "movies") == 0 && wait != nullptr &&
                    wait->vftable == &g_zFMV_ActionWait_Vtable && wait->durationSec == 1.25f &&
                    blur != nullptr && blur->vftable == &g_zFMV_ActionBlurH_Vtable &&
                    blur->framesRemaining == 1 && blur->blurPassCount == 4 && sound != nullptr &&
                    sound->vftable == &g_zFMV_ActionPlaySound_Vtable &&
                    std::strcmp(sound->sampleName, "intro_whoosh") == 0 &&
                    sound->voice == nullptr && sound->next == nullptr;

    script.Cleanup();
    g_zArchive_MountedList = nullptr;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}

extern "C" int zfmv_action_wait_begin_update_smoke(void) {
    zFMV_ActionWait action{};
    action.durationSec = 2.5f;
    action.startSec = -1.0f;

    action.Begin(10.25);

    return action.startSec == 10.25f && action.Update(12.0) == 1 && action.Update(12.75) == 0 ? 0
                                                                                              : 1;
}

extern "C" int zfmv_action_flip_surfaces_smoke(void) {
    CodeFunctionPatch adjustPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
                           reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
                           adjustPatch)) {
        return 1;
    }

    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvAdjustSurfacesSrcRect = reinterpret_cast<zVidRect32 *>(0x11111111);
    g_fakeFmvAdjustSurfacesDstRect = reinterpret_cast<zVidRect32 *>(0x22222222);
    g_fakeFmvAdjustSurfacesWaitForPresent = 0;
    g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst = 0;

    zFMV_Action action{};
    action.FlipSurfaces();

    const bool ok =
        g_fakeFmvAdjustSurfacesCount == 1 && g_fakeFmvAdjustSurfacesSrcRect == nullptr &&
        g_fakeFmvAdjustSurfacesDstRect == nullptr &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 1 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 1;

    RestoreFunctionPatch(adjustPatch);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_base_destructor_smoke(void) {
    zFMV_Action action{};
    action.vftable = &g_zFMV_ActionWait_Vtable;
    action.next = reinterpret_cast<zFMV_Action *>(0x1234);

    action.Destructor();
    if (action.vftable != &g_zFMV_ActionBase_Vtable ||
        action.next != reinterpret_cast<zFMV_Action *>(0x1234)) {
        return 1;
    }

    action.vftable = &g_zFMV_ActionWait_Vtable;
    if (action.ScalarDeletingDestructor(0) != &action ||
        action.vftable != &g_zFMV_ActionBase_Vtable) {
        return 2;
    }

    CodeFunctionPatch deletePatch{};
    void(__cdecl *operatorDeleteFn)(void *) = &operator delete;
    if (!PatchFunctionJump(reinterpret_cast<void *>(operatorDeleteFn),
                           reinterpret_cast<void *>(&FakeFmvOperatorDelete), deletePatch)) {
        return 3;
    }

    g_fakeFmvOperatorDeleteCount = 0;
    g_fakeFmvOperatorDeletePtr = nullptr;
    void *const storage = std::malloc(sizeof(zFMV_Action));
    if (storage == nullptr) {
        RestoreFunctionPatch(deletePatch);
        return 4;
    }

    zFMV_Action *const heapAction = reinterpret_cast<zFMV_Action *>(storage);
    heapAction->vftable = &g_zFMV_ActionWait_Vtable;
    heapAction->next = reinterpret_cast<zFMV_Action *>(0x5678);

    zFMV_Action *const deletingResult = heapAction->ScalarDeletingDestructor(1);
    const bool deletingOk =
        deletingResult == heapAction && heapAction->vftable == &g_zFMV_ActionBase_Vtable &&
        heapAction->next == reinterpret_cast<zFMV_Action *>(0x5678) &&
        g_fakeFmvOperatorDeleteCount == 1 && g_fakeFmvOperatorDeletePtr == heapAction;

    RestoreFunctionPatch(deletePatch);
    std::free(storage);
    return deletingOk ? 0 : 5;
}

extern "C" int zfmv_action_derived_scalar_deleting_destructor_smoke(void) {
    zFMV_ActionWait action{};
    action.vftable = &g_zFMV_ActionWait_Vtable;

    return action.DerivedScalarDeletingDestructor(0) == &action &&
                   action.vftable == &g_zFMV_ActionBase_Vtable
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_sound_begin_missing_sample_smoke(void) {
    zFMV_ActionPlaySound action{};
    action.vftable = &g_zFMV_ActionPlaySound_Vtable;
    std::strcpy(action.sampleName, "__missing_fmv_sample__");
    action.sample = reinterpret_cast<zSndSample *>(0x1234);
    action.voice = nullptr;

    action.Begin(0.0);

    return action.sample == nullptr && action.voice == nullptr ? 0 : 1;
}
