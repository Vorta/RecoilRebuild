// Checked-in focused native smoke translation unit, formerly extracted from fmv_script_tests.cpp.
// Emits FMV smokes that are excluded from the main fmv_script_tests.cpp object.

#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vfw.h>

extern "C" HWND g_RecoilApp_hWndMain;

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
        g_fakeFmvCloseParamOk = 1;
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
} // namespace

extern "C" int zfmv_script_run_blocking_empty_smoke(void) {
    zFMV_Script script{};
    script.m_abortOnKey = 0;

    const std::int32_t result = script.RunBlocking(1);
    const bool emptyOk = result == 1 && script.m_abortOnKey == 1 && script.m_cur == nullptr;

    struct RunBlockingSmokeAction : zFMV_Action {
        int beginCount;
        int updateCount;
        int endCount;
        int nextUpdateResult;

        void Begin(double) {
            ++beginCount;
        }

        int Update(double) {
            ++updateCount;
            return nextUpdateResult;
        }

        void End() {
            ++endCount;
        }
    };

    zSndSampleSetRegistry oldRegistry = g_zSnd_SampleSetRegistry;
    const int oldUseArchiveBanksFlag = g_zSnd_UseArchiveBanksFlag;
    zSndSampleSet fmvSet{};
    fmvSet.setName = const_cast<char *>("FMV");
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&fmvSet);
    g_zSnd_UseArchiveBanksFlag = 0;
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

    RunBlockingSmokeAction action{};
    script = {};
    script.m_head = &action;
    script.m_tail = &action;
    script.m_cur = &action;
    action.nextUpdateResult = 0;

    const int actionResult = script.RunBlocking(0);
    const bool actionOk =
        actionResult == 1 && script.m_abortOnKey == 0 && script.m_cur == &action &&
        action.beginCount == 1 && action.updateCount == 1 && action.endCount == 1 &&
        fmvSet.resourcesLoaded == 1;

    g_zSnd_SampleSetRegistry = oldRegistry;
    g_zSnd_UseArchiveBanksFlag = oldUseArchiveBanksFlag;
    if (!emptyOk) {
        return 1;
    }
    return actionOk ? 0 : 2;
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

extern "C" int zfmv_action_play_mci_update_smoke(void) {
    alignas(zFMV_ActionPlayMci) unsigned char actionStorage[sizeof(zFMV_ActionPlayMci)];
    zFMV_ActionPlayMci *const action = new (actionStorage) zFMV_ActionPlayMci();
    action->next = reinterpret_cast<zFMV_Action *>(0x11111111);
    action->mediaPath = reinterpret_cast<char *>(0x22222222);
    action->playback = reinterpret_cast<zFMV_Playback *>(0x33333333);

    const int result = action->Update(123.5);
    return result == 0 && action->next == reinterpret_cast<zFMV_Action *>(0x11111111) &&
                   action->mediaPath == reinterpret_cast<char *>(0x22222222) &&
                   action->playback == reinterpret_cast<zFMV_Playback *>(0x33333333)
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_mci_begin_smoke(void) {
    CodeFunctionPatch mciPatch{};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&mciSendCommandA),
            reinterpret_cast<void *>(&FakeFmvMciSendCommandA),
            mciPatch
        )) {
        return 1;
    }

    zFMV_ActionPlayMci emptyAction{};
    emptyAction.mediaPath = nullptr;
    emptyAction.playback = nullptr;
    emptyAction.Begin(12.0);
    const bool nullOk = g_fakeFmvMciSendCommandCount == 0;

    zFMV_Playback playback{};
    playback.mediaPathDup = const_cast<char *>("intro.mpg");
    playback.notifyHwnd = reinterpret_cast<HWND>(0x2468);
    playback.mciPutFlags = 0;

    zFMV_ActionPlayMci action{};
    action.mediaPath = nullptr;
    action.playback = &playback;

    g_fakeFmvMciSendCommandCount = 0;
    for (int index = 0; index < 8; ++index) {
        g_fakeFmvMciDevices[index] = 0;
        g_fakeFmvMciMessages[index] = 0;
        g_fakeFmvMciFlags[index] = 0;
        g_fakeFmvMciParams[index] = 0;
        g_fakeFmvMciReturns[index] = 0;
    }
    g_fakeFmvPlayCallback = 0;
    g_fakeFmvPlayFrom = 0xffffffffu;
    g_fakeFmvPlayTo = 0xffffffffu;

    action.Begin(34.0);

    const bool playbackOk =
        g_fakeFmvMciSendCommandCount == 4 && g_fakeFmvMciDevices[0] == 0 &&
        g_fakeFmvMciMessages[0] == 0x803 && g_fakeFmvMciFlags[0] == 0x2202 &&
        g_fakeFmvMciDevices[1] == 0x3456 && g_fakeFmvMciMessages[1] == 0x841 &&
        g_fakeFmvMciFlags[1] == 0x10002 && g_fakeFmvWindowHwnd == playback.notifyHwnd &&
        g_fakeFmvMciDevices[2] == 0x3456 && g_fakeFmvMciMessages[2] == 0x811 &&
        g_fakeFmvMciFlags[2] == 0x302 && g_fakeFmvSetTimeFormat == 0x1b &&
        g_fakeFmvMciDevices[3] == 0x3456 && g_fakeFmvMciMessages[3] == 0x806 &&
        g_fakeFmvMciFlags[3] == 0x6 && g_fakeFmvPlayCallback ==
            static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(playback.notifyHwnd)) &&
        g_fakeFmvPlayFrom == 0;

    action.playback = nullptr;
    RestoreFunctionPatch(mciPatch);
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
    action.mediaPath = nullptr;
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

    alignas(zFMV_ActionPlayMci) unsigned char actionStorage[sizeof(zFMV_ActionPlayMci)];
    zFMV_ActionPlayMci *const action = new (actionStorage) zFMV_ActionPlayMci();
    action->mediaPath = static_cast<char *>(std::malloc(6));
    action->playback = static_cast<zFMV_Playback *>(std::malloc(sizeof(zFMV_Playback)));
    if (action->mediaPath == nullptr || action->playback == nullptr) {
        RestoreFunctionPatch(deletePatch);
        std::free(action->mediaPath);
        std::free(action->playback);
        return 2;
    }
    std::strcpy(action->mediaPath, "movie");
    std::memset(action->playback, 0, sizeof(*action->playback));
    action->playback->mediaPathDup = static_cast<char *>(std::malloc(6));
    if (action->playback->mediaPathDup == nullptr) {
        RestoreFunctionPatch(deletePatch);
        std::free(action->mediaPath);
        std::free(action->playback);
        return 3;
    }
    std::strcpy(action->playback->mediaPathDup, "movie");

    zFMV_Playback *const directPlayback = action->playback;
    g_fakeFmvOperatorDeleteCount = 0;
    g_fakeFmvOperatorDeletePtr = nullptr;
    action->~zFMV_ActionPlayMci();
    const bool directOk =
        action->mediaPath == nullptr && action->playback == nullptr &&
        g_fakeFmvOperatorDeleteCount == 1 && g_fakeFmvOperatorDeletePtr == directPlayback;

    RestoreFunctionPatch(deletePatch);
    std::free(directPlayback);
    return directOk ? 0 : 6;
}
