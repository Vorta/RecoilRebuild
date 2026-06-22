#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <windows.h>
#include <mmsystem.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

struct TestAction : zFMV_Action {
};

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
int g_fakeFmvAdjustSurfacesCount;
zVidRect32 *g_fakeFmvAdjustSurfacesSrcRect;
zVidRect32 *g_fakeFmvAdjustSurfacesDstRect;
int g_fakeFmvAdjustSurfacesWaitForPresent;
int g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst;
int g_fakeFmvPostprocessCount;
int g_fakeFmvSwPostprocessCount;
int g_fakeFmvUnlockPrimaryCount;
int g_fakeFmvUnlockSwCount;
int g_fakeFmvSwToPrimaryDirectCount;
zVidRect32 *g_fakeFmvSwToPrimaryDirectSrcRect;
zVidRect32 *g_fakeFmvSwToPrimaryDirectDstRect;
int g_fakeFmvBlurByModeCount;
zVidRect32 *g_fakeFmvBlurByModeRect;
int g_fakeFmvBlurByModeModes[8];

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

MCIERROR WINAPI FakeFmvMciSendCommandA(
    MCIDEVICEID deviceId,
    UINT message,
    DWORD_PTR flags,
    DWORD_PTR params
) {
    const int index = g_fakeFmvMciSendCommandCount;
    if (index < 8) {
        g_fakeFmvMciDevices[index] = deviceId;
        g_fakeFmvMciMessages[index] = message;
        g_fakeFmvMciFlags[index] = flags;
        g_fakeFmvMciParams[index] = params;
    }
    ++g_fakeFmvMciSendCommandCount;

    if (message == 0x803 && params != 0) {
        TestFmvMciOpenParams *const openParams =
            reinterpret_cast<TestFmvMciOpenParams *>(params);
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
        TestFmvMciRectParams *const rectParams =
            reinterpret_cast<TestFmvMciRectParams *>(params);
        int *const capturedRect =
            flags == 0x50002 ? g_fakeFmvDestRect : g_fakeFmvSourceRect;
        capturedRect[0] = rectParams->left;
        capturedRect[1] = rectParams->top;
        capturedRect[2] = rectParams->width;
        capturedRect[3] = rectParams->height;
    }

    if (message == 0x811 && params != 0) {
        TestFmvMciSetParams *const setParams =
            reinterpret_cast<TestFmvMciSetParams *>(params);
        g_fakeFmvSetTimeFormat = setParams->timeFormat;
        g_fakeFmvSetAudio = setParams->audio;
    }

    if (message == 0x806 && params != 0) {
        TestFmvMciPlayParams *const playParams =
            reinterpret_cast<TestFmvMciPlayParams *>(params);
        g_fakeFmvPlayCallback = playParams->callback;
        g_fakeFmvPlayFrom = playParams->from;
        g_fakeFmvPlayTo = playParams->to;
    }

    if (message == 0x804 && params != 0) {
        g_fakeFmvCloseParamOk = 1;
    }

    return index < 8 ? g_fakeFmvMciReturns[index] : 0;
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

int FakeFmvRunPostprocessOnPrimaryBuffer() {
    ++g_fakeFmvPostprocessCount;
    return 1;
}

void FakeFmvRunPostprocessOnSwBuffer() {
    ++g_fakeFmvSwPostprocessCount;
}

int FakeFmvDispatchUnlockPrimarySurfaceState() {
    ++g_fakeFmvUnlockPrimaryCount;
    return 1;
}

int FakeFmvDispatchUnlockSwSurfaceState() {
    ++g_fakeFmvUnlockSwCount;
    return 1;
}

void __fastcall FakeFmvBltSwToPrimaryRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    ++g_fakeFmvSwToPrimaryDirectCount;
    g_fakeFmvSwToPrimaryDirectSrcRect = srcRect;
    g_fakeFmvSwToPrimaryDirectDstRect = dstRect;
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

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(
        patch.original,
        patch.address,
        sizeof(patch.original)
    );

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    return true;
}

void RestoreFunctionPatch(
    CodeFunctionPatch &patch
) {
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(
            patch.address,
            patch.original,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            oldProtect,
            &ignored
        );
        FlushInstructionCache(
            GetCurrentProcess(),
            patch.address,
            sizeof(patch.original)
        );
    }

    patch.address = 0;
}

void WriteU32(
    HANDLE file,
    unsigned int value
) {
    DWORD written = 0;
    WriteFile(
        file,
        &value,
        sizeof(value),
        &written,
        0
    );
}

void WriteBytes(
    HANDLE file,
    const char *value,
    unsigned int length
) {
    DWORD written = 0;
    WriteFile(
        file,
        value,
        length,
        &written,
        0
    );
}

void WriteStringNode(
    HANDLE file,
    const char *value
) {
    const unsigned int length = (unsigned int)(strlen(value));
    WriteU32(
        file,
        zReader::ZRDR_NODE_STRING
    );
    WriteU32(
        file,
        length
    );
    WriteBytes(
        file,
        value,
        length
    );
}

void WriteIntNode(
    HANDLE file,
    int value
) {
    WriteU32(
        file,
        zReader::ZRDR_NODE_INT
    );
    WriteU32(
        file,
        (unsigned int)(value)
    );
}

void WriteFloatNode(
    HANDLE file,
    float value
) {
    union FloatBits {
        float f32;
        unsigned int u32;
    };

    FloatBits bits;
    bits.f32 = value;
    WriteU32(
        file,
        zReader::ZRDR_NODE_FLOAT
    );
    WriteU32(
        file,
        bits.u32
    );
}

void WriteArrayHeader(
    HANDLE file,
    int count
) {
    WriteU32(
        file,
        zReader::ZRDR_NODE_ARRAY
    );
    WriteU32(
        file,
        (unsigned int)(count)
    );
}

int StringEquals(
    const char *left,
    const char *right
) {
    return left != 0 && strcmp(left, right) == 0;
}

void SetActiveRegion(
    int width,
    int height
) {
    zRndr::g_activeRegionWidth = width;
    zRndr::g_activeRegionHeight = height;
    zRndr::g_pitchBytes = width * 2;
    zRndr::g_bytesPerPixel = 2;
}
} // namespace

extern "C" int zfmv_script_append_action_smoke(void) {
    zFMV_Script script = {};
    TestAction action1 = {};
    TestAction action2 = {};
    action1.next = (zFMV_Action *)(0x11111111);
    action2.next = (zFMV_Action *)(0x22222222);

    if (script.AppendAction(0) != 0 ||
        script.m_head != 0 ||
        script.m_tail != 0 ||
        script.m_cur != 0) {
        return 1;
    }

    if (script.AppendAction(&action1) != 1 ||
        action1.next != 0 ||
        script.m_head != &action1 ||
        script.m_tail != &action1 ||
        script.m_cur != &action1) {
        return 2;
    }

    if (script.AppendAction(&action2) != 1 ||
        action1.next != &action2 ||
        action2.next != 0 ||
        script.m_head != &action1 ||
        script.m_tail != &action2 ||
        script.m_cur != &action1) {
        return 3;
    }

    return 0;
}

extern "C" int zfmv_action_flip_surfaces_smoke(void) {
    CodeFunctionPatch adjustPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
            reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
            adjustPatch
        )) {
        return 1;
    }

    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvAdjustSurfacesSrcRect =
        reinterpret_cast<zVidRect32 *>(0x11111111);
    g_fakeFmvAdjustSurfacesDstRect =
        reinterpret_cast<zVidRect32 *>(0x22222222);
    g_fakeFmvAdjustSurfacesWaitForPresent = 0;
    g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst = 0;

    zFMV_Action action = {};
    action.FlipSurfaces();

    const bool ok =
        g_fakeFmvAdjustSurfacesCount == 1 &&
        g_fakeFmvAdjustSurfacesSrcRect == 0 &&
        g_fakeFmvAdjustSurfacesDstRect == 0 &&
        g_fakeFmvAdjustSurfacesWaitForPresent == 1 &&
        g_fakeFmvAdjustSurfacesBlitPrimaryToSwFirst == 1;

    RestoreFunctionPatch(adjustPatch);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_image_constructor_with_screen_rect_smoke(void) {
    g_zFMV_ActionImage_BlitRect.left = -1;
    g_zFMV_ActionImage_BlitRect.top = -1;
    g_zFMV_ActionImage_BlitRect.right = 640;
    g_zFMV_ActionImage_BlitRect.bottom = 480;

    zFMV_ActionImage action = {};
    action.next = (zFMV_Action *)(0x11111111);
    action.image = (void *)(0x22222222);

    zFMV_ActionImage *const returned = new (&action) zFMV_ActionImage(
        "screen.raw",
        7,
        32,
        48
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.image == 0 &&
                   StringEquals(
                       action.imagePath,
                       "screen.raw"
                   ) &&
                   action.doAdjustSurfaces == 7 &&
                   action.forcePrimaryPostprocess == 1 &&
                   g_zFMV_ActionImage_BlitRect.left == 32 &&
                   g_zFMV_ActionImage_BlitRect.top == 48 &&
                   action.blitRect.left == 32 &&
                   action.blitRect.top == 48 &&
                   action.blitRect.right == 640 &&
                   action.blitRect.bottom == 480
               ? 0
               : 1;
}

extern "C" int zfmv_action_image_constructor_scaled_smoke(void) {
    SetActiveRegion(
        800,
        600
    );

    zFMV_ActionImage action = {};
    action.next = (zFMV_Action *)(0x11111111);
    action.image = (void *)(0x22222222);

    zFMV_ActionImage *const returned = new (&action) zFMV_ActionImage(
        "scaled.raw",
        3
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.image == 0 &&
                   StringEquals(
                       action.imagePath,
                       "scaled.raw"
                   ) &&
                   action.doAdjustSurfaces == 3 &&
                   action.forcePrimaryPostprocess == 0 &&
                   action.blitRect.left == 0 &&
                   action.blitRect.top == 0 &&
                   action.blitRect.right == 800 &&
                   action.blitRect.bottom == 600
               ? 0
               : 1;
}

extern "C" int zfmv_action_fade_constructor_smoke(void) {
    zFMV_ActionFade action = {};
    action.next = (zFMV_Action *)(0x11111111);
    action.capturedFrame = (void *)(0x22222222);
    action.startSec = 12.5;

    zFMV_ActionFade *const returned = new (&action) zFMV_ActionFade(
        0xff,
        0x80,
        0x20,
        0x3fc00000,
        -1,
        128
    );

    const unsigned short expectedColor = (unsigned short)(zVid_PackColorRGB(
        0xff,
        0x80,
        0x20
    ));
    return returned == &action &&
                   action.next == 0 &&
                   action.fadeColorPacked16 == expectedColor &&
                   action.durationSecRaw == 0x3fc00000 &&
                   action.fadeDirectionSign == -1 &&
                   action.maxAlpha == 128 &&
                   action.capturedFrame == (void *)(0x22222222) &&
                   action.startSec == 12.5
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_avi_constructor_existing_file_smoke(void) {
    const char *const fileName = "__recoil_fmv_existing_constructor.avi";
    HANDLE file = CreateFileA(
        fileName,
        GENERIC_WRITE,
        0,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        0
    );
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }
    CloseHandle(file);

    zFMV_ActionPlayAvi action = {};
    action.next = (zFMV_Action *)(0x11111111);
    zFMV_ActionPlayAvi *const returned = new (&action) zFMV_ActionPlayAvi(
        ".",
        fileName,
        5
    );

    char expectedPath[MAX_PATH] = {};
    sprintf(
        expectedPath,
        ".\\%s",
        fileName
    );

    const int ok = returned == &action &&
                   action.next == 0 &&
                   action.modeFlags == 5 &&
                   StringEquals(
                       action.mediaPath,
                       expectedPath
                   );
    DeleteFileA(fileName);
    return ok ? 0 : 2;
}

extern "C" int zfmv_action_play_avi_constructor_drive_fallback_smoke(void) {
    zFMV_ActionPlayAvi action = {};
    action.next = (zFMV_Action *)(0x11111111);

    zFMV_ActionPlayAvi *const returned = new (&action) zFMV_ActionPlayAvi(
        ".",
        "__recoil_missing_fmv_constructor_file__.avi",
        9
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.modeFlags == 9 &&
                   action.mediaPath != 0 &&
                   strstr(
                       action.mediaPath,
                       "__recoil_missing_fmv_constructor_file__.avi"
                   ) != 0
               ? 0
               : 1;
}

extern "C" int zfmv_playback_constructor_smoke(void) {
    zFMV_Playback playback = {};
    playback.mciPutFlags = 0x77777777;
    playback.notifyHwnd = (HWND)(0x11111111);
    playback.mediaPathDup = (char *)(0x22222222);

    zFMV_Playback *const returned = new (&playback) zFMV_Playback(
        "movie.avi",
        (HWND)(0x12345678)
    );

    const bool ok =
        returned == &playback && playback.mediaPathDup != 0 &&
        strcmp(
            playback.mediaPathDup,
            "movie.avi"
        ) == 0 &&
        playback.notifyHwnd == (HWND)(0x12345678) && playback.mciPutFlags == 0;

    free(playback.mediaPathDup);
    return ok ? 0 : 1;
}

extern "C" int zfmv_playback_destructor_smoke(void) {
    zFMV_Playback playback = {};
    playback.mediaPathDup = (char *)(malloc(4));
    if (playback.mediaPathDup == 0) {
        return 1;
    }

    strcpy(
        playback.mediaPathDup,
        "x"
    );
    playback.Destructor();
    playback.mediaPathDup = 0;
    playback.Destructor();
    return 0;
}

extern "C" int zfmv_playback_report_mci_error_smoke(void) {
    zFMV_Playback playback = {};
    return playback.ReportMciError(0xffffffffu) == 0 ? 0 : 1;
}

extern "C" int zfmv_playback_open_and_play_smoke(void) {
    CodeFunctionPatch mciPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&mciSendCommandA),
            reinterpret_cast<void *>(&FakeFmvMciSendCommandA),
            mciPatch
        )) {
        return 1;
    }

    zFMV_Playback playback = {};
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
    g_fakeFmvOpenDeviceType = 0;
    g_fakeFmvOpenElementName = 0;
    g_fakeFmvWindowHwnd = 0;
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
    for (int rectIndex = 0; rectIndex < 4; ++rectIndex) {
        g_fakeFmvDestRect[rectIndex] = 0;
        g_fakeFmvSourceRect[rectIndex] = 0;
    }

    playback.OpenAndPlay(
        100,
        250,
        1
    );

    const bool successSequence =
        g_fakeFmvMciSendCommandCount == 6 &&
        g_fakeFmvMciDevices[0] == 0 &&
        g_fakeFmvMciMessages[0] == 0x803 &&
        g_fakeFmvMciFlags[0] == 0x2202 &&
        strcmp(
            g_fakeFmvOpenDeviceType,
            "MPEGVideo"
        ) == 0 &&
        g_fakeFmvOpenElementName == playback.mediaPathDup &&
        playback.mciDeviceId == 0x3456 &&
        g_fakeFmvMciDevices[1] == 0x3456 &&
        g_fakeFmvMciMessages[1] == 0x841 &&
        g_fakeFmvMciFlags[1] == 0x10002 &&
        g_fakeFmvWindowHwnd == playback.notifyHwnd &&
        g_fakeFmvMciDevices[2] == 0x3456 &&
        g_fakeFmvMciMessages[2] == 0x842 &&
        g_fakeFmvMciFlags[2] == 0x50002 &&
        g_fakeFmvDestRect[0] == 5 &&
        g_fakeFmvDestRect[1] == 7 &&
        g_fakeFmvDestRect[2] == 20 &&
        g_fakeFmvDestRect[3] == 30 &&
        g_fakeFmvMciDevices[3] == 0x3456 &&
        g_fakeFmvMciMessages[3] == 0x842 &&
        g_fakeFmvMciFlags[3] == 0x30002 &&
        g_fakeFmvSourceRect[0] == 2 &&
        g_fakeFmvSourceRect[1] == 3 &&
        g_fakeFmvSourceRect[2] == 10 &&
        g_fakeFmvSourceRect[3] == 20 &&
        g_fakeFmvMciDevices[4] == 0x3456 &&
        g_fakeFmvMciMessages[4] == 0x811 &&
        g_fakeFmvMciFlags[4] == 0x302 &&
        g_fakeFmvSetTimeFormat == 0x1b &&
        g_fakeFmvSetAudio ==
            static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(playback.notifyHwnd)) &&
        g_fakeFmvMciDevices[5] == 0x3456 &&
        g_fakeFmvMciMessages[5] == 0x806 &&
        g_fakeFmvMciFlags[5] == 0x1000e &&
        g_fakeFmvPlayCallback ==
            static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(playback.notifyHwnd)) &&
        g_fakeFmvPlayFrom == 100 &&
        g_fakeFmvPlayTo == 250;

    g_fakeFmvMciSendCommandCount = 0;
    for (int index = 0; index < 8; ++index) {
        g_fakeFmvMciMessages[index] = 0;
        g_fakeFmvMciReturns[index] = 0;
    }
    playback.mciPutFlags = 0;
    g_fakeFmvMciReturns[1] = 0x4321;
    playback.OpenAndPlay(
        75,
        -1,
        0
    );
    const bool windowFailureStops =
        g_fakeFmvMciSendCommandCount == 2 &&
        g_fakeFmvMciMessages[0] == 0x803 &&
        g_fakeFmvMciMessages[1] == 0x841;

    RestoreFunctionPatch(mciPatch);
    return successSequence && windowFailureStops ? 0 : 2;
}

extern "C" int zfmv_playback_stop_and_close_smoke(void) {
    CodeFunctionPatch mciPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&mciSendCommandA),
            reinterpret_cast<void *>(&FakeFmvMciSendCommandA),
            mciPatch
        )) {
        return 1;
    }

    zFMV_Playback playback = {};
    playback.mciDeviceId = 0x3456;
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
        g_fakeFmvMciSendCommandCount == 2 &&
        g_fakeFmvMciDevices[0] == 0x3456 &&
        g_fakeFmvMciMessages[0] == 0x808 &&
        g_fakeFmvMciFlags[0] == 2 &&
        g_fakeFmvMciParams[0] == 0 &&
        g_fakeFmvMciDevices[1] == 0x3456 &&
        g_fakeFmvMciMessages[1] == 0x804 &&
        g_fakeFmvMciFlags[1] == 2 &&
        g_fakeFmvCloseParamOk == 1;

    g_fakeFmvMciSendCommandCount = 0;
    g_fakeFmvMciReturns[0] = 0x1234;
    g_fakeFmvMciReturns[1] = 0;
    playback.StopAndClose();
    const bool stopFailureSkipsClose =
        g_fakeFmvMciSendCommandCount == 1 &&
        g_fakeFmvMciMessages[0] == 0x808;

    RestoreFunctionPatch(mciPatch);
    return successSequence && stopFailureSkipsClose ? 0 : 2;
}

extern "C" int zfmv_playback_set_dest_rect_smoke(void) {
    zFMV_Playback playback = {};
    playback.mciPutFlags = 0x10;
    const zFMV_Rect rect = {1, 2, 3, 4};

    const int result = playback.SetDestRect(&rect);

    return result == 0x40010 &&
                   playback.mciPutFlags == 0x40010 &&
                   playback.destinationRect.left == 1 &&
                   playback.destinationRect.top == 2 &&
                   playback.destinationRect.right == 3 &&
                   playback.destinationRect.bottom == 4
               ? 0
               : 1;
}

extern "C" int zfmv_action_play_mci_constructor_smoke(void) {
    SetActiveRegion(
        1024,
        768
    );

    zFMV_ActionPlayMci action = {};
    action.next = (zFMV_Action *)(0x11111111);
    zFMV_ActionPlayMci *const returned = new (&action) zFMV_ActionPlayMci(
        (HWND)(0x1234),
        "movies",
        "intro.mpg"
    );

    return returned == &action &&
                   action.next == 0 &&
                   StringEquals(
                       action.mediaPath,
                       "movies\\intro.mpg"
                   ) &&
                   action.playback != 0 &&
                   StringEquals(
                       action.playback->mediaPathDup,
                       "movies\\intro.mpg"
                   ) &&
                   action.playback->notifyHwnd == (HWND)(0x1234) &&
                   action.playback->destinationRect.left == 0 &&
                   action.playback->destinationRect.top == 0 &&
                   action.playback->destinationRect.right == 1024 &&
                   action.playback->destinationRect.bottom == 768
               ? 0
               : 1;
}

extern "C" int zfmv_action_blur_constructor_smoke(void) {
    zFMV_ActionBlur action = {};
    action.next = (zFMV_Action *)(0x11111111);

    zFMV_ActionBlur *const returned = new (&action) zFMV_ActionBlur(
        12,
        3
    );

    return returned == &action &&
                   action.next == 0 &&
                   action.framesRemaining == 12 &&
                   action.blurPassCount == 3
               ? 0
               : 1;
}

extern "C" int zfmv_action_blur_update_smoke(void) {
    CodeFunctionPatch postPrimaryPatch = {};
    CodeFunctionPatch postSwPatch = {};
    CodeFunctionPatch blurPatch = {};
    CodeFunctionPatch unlockPrimaryPatch = {};
    CodeFunctionPatch unlockSwPatch = {};
    CodeFunctionPatch adjustPatch = {};

    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&FakeFmvRunPostprocessOnPrimaryBuffer),
            postPrimaryPatch
        )) {
        return 1;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnSwBuffer),
            reinterpret_cast<void *>(&FakeFmvRunPostprocessOnSwBuffer),
            postSwPatch
        )) {
        RestoreFunctionPatch(postPrimaryPatch);
        return 2;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::buff_BlurRegionByMode),
            reinterpret_cast<void *>(&FakeFmvBlurRegionByMode),
            blurPatch
        )) {
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 3;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
            reinterpret_cast<void *>(&FakeFmvDispatchUnlockPrimarySurfaceState),
            unlockPrimaryPatch
        )) {
        RestoreFunctionPatch(blurPatch);
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 4;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::Dispatch_UnlockSwSurfaceState),
            reinterpret_cast<void *>(&FakeFmvDispatchUnlockSwSurfaceState),
            unlockSwPatch
        )) {
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blurPatch);
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 5;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
            reinterpret_cast<void *>(&FakeFmvAdjustSurfacesIfEnabled),
            adjustPatch
        )) {
        RestoreFunctionPatch(unlockSwPatch);
        RestoreFunctionPatch(unlockPrimaryPatch);
        RestoreFunctionPatch(blurPatch);
        RestoreFunctionPatch(postSwPatch);
        RestoreFunctionPatch(postPrimaryPatch);
        return 6;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    zVideo_BltRectDirectProc oldSwToPrimary =
        g_zVideo_pfnBltSwToPrimaryRectDirect;
    g_zVideo_pfnBltSwToPrimaryRectDirect = &FakeFmvBltSwToPrimaryRectDirect;

    zFMV_ActionBlur action = {};
    action.framesRemaining = 2;
    action.blurPassCount = 2;
    action.swSurfaceRect.left = 1;
    action.swSurfaceRect.top = 2;
    action.swSurfaceRect.right = 3;
    action.swSurfaceRect.bottom = 4;
    action.primarySurfaceRect.left = 5;
    action.primarySurfaceRect.top = 6;
    action.primarySurfaceRect.right = 7;
    action.primarySurfaceRect.bottom = 8;
    g_zVideo_ActiveRendererPath = 0;
    g_fakeFmvSwPostprocessCount = 0;
    g_fakeFmvPostprocessCount = 0;
    g_fakeFmvBlurByModeCount = 0;
    g_fakeFmvBlurByModeRect = reinterpret_cast<zVidRect32 *>(0x11111111);
    std::memset(
        g_fakeFmvBlurByModeModes,
        0,
        sizeof(g_fakeFmvBlurByModeModes)
    );
    g_fakeFmvUnlockSwCount = 0;
    g_fakeFmvUnlockPrimaryCount = 0;
    g_fakeFmvAdjustSurfacesCount = 0;
    g_fakeFmvSwToPrimaryDirectCount = 0;
    const bool softwareCombinedOk =
        action.Update(1.0) == 1 && action.framesRemaining == 1 &&
        g_fakeFmvSwPostprocessCount == 1 && g_fakeFmvPostprocessCount == 0 &&
        g_fakeFmvBlurByModeCount == 2 && g_fakeFmvBlurByModeRect == 0 &&
        g_fakeFmvBlurByModeModes[0] == 3 &&
        g_fakeFmvBlurByModeModes[1] == 3 &&
        g_fakeFmvUnlockSwCount == 1 && g_fakeFmvUnlockPrimaryCount == 0 &&
        g_fakeFmvSwToPrimaryDirectCount == 1 &&
        g_fakeFmvSwToPrimaryDirectSrcRect ==
            reinterpret_cast<zVidRect32 *>(&action.swSurfaceRect) &&
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

    zFMV_ActionBlurH hAction = {};
    hAction.framesRemaining = 2;
    hAction.blurPassCount = 1;
    g_zVideo_ActiveRendererPath = 0;
    g_fakeFmvBlurByModeCount = 0;
    hAction.Update(3.0);
    const bool horizontalOk =
        hAction.framesRemaining == 1 && g_fakeFmvBlurByModeCount == 1 &&
        g_fakeFmvBlurByModeModes[0] == 1;

    zFMV_ActionBlurV vAction = {};
    vAction.framesRemaining = 2;
    vAction.blurPassCount = 1;
    g_zVideo_ActiveRendererPath = 1;
    g_fakeFmvBlurByModeCount = 0;
    vAction.Update(4.0);
    const bool verticalOk =
        vAction.framesRemaining == 1 && g_fakeFmvBlurByModeCount == 1 &&
        g_fakeFmvBlurByModeModes[0] == 2;

    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(unlockSwPatch);
    RestoreFunctionPatch(unlockPrimaryPatch);
    RestoreFunctionPatch(blurPatch);
    RestoreFunctionPatch(postSwPatch);
    RestoreFunctionPatch(postPrimaryPatch);
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldSwToPrimary;
    g_zVideo_ActiveRendererPath = oldRendererPath;

    return softwareCombinedOk && hardwareCombinedOk && horizontalOk &&
                   verticalOk
               ? 0
               : 7;
}

extern "C" int zfmv_script_load_actions_from_zrd_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(
            sizeof(tempDir),
            tempDir
        ) == 0 ||
        GetTempFileNameA(
            tempDir,
            "fmv",
            0,
            tempPath
        ) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(
        tempPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        0
    );
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteArrayHeader(
        file,
        7
    );
    WriteStringNode(
        file,
        "FMV_PATH"
    );
    WriteStringNode(
        file,
        "movies"
    );
    WriteStringNode(
        file,
        "IMAGE_PATH"
    );
    WriteStringNode(
        file,
        "images"
    );
    WriteStringNode(
        file,
        "INTRO"
    );
    WriteArrayHeader(
        file,
        4
    );
    WriteArrayHeader(
        file,
        3
    );
    WriteStringNode(
        file,
        "WAIT"
    );
    WriteFloatNode(
        file,
        1.25f
    );
    WriteArrayHeader(
        file,
        3
    );
    WriteStringNode(
        file,
        "BLURH"
    );
    WriteIntNode(
        file,
        4
    );
    WriteArrayHeader(
        file,
        3
    );
    WriteStringNode(
        file,
        "PLAYSOUND"
    );
    WriteStringNode(
        file,
        "intro_whoosh"
    );
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(
        file,
        0,
        0,
        FILE_CURRENT
    );
    strcpy(
        record.name,
        "fmv.zrd"
    );

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

    zFMV_Script script = {};
    script.Init(
        0,
        0,
        0
    );
    const int result = script.LoadActionsFromZrd(
        "C:\\dummy\\fmv.zrd",
        "INTRO"
    );

    zFMV_ActionWait *const wait = (zFMV_ActionWait *)(script.m_head);
    zFMV_ActionBlur *const blur =
        wait != 0 ? (zFMV_ActionBlur *)(wait->next) : 0;
    zFMV_ActionPlaySound *const sound =
        blur != 0 ? (zFMV_ActionPlaySound *)(blur->next) : 0;

    const int ok = result == 3 &&
                   StringEquals(
                       script.m_fmvPath,
                       "movies"
                   ) &&
                   wait != 0 &&
                   wait->durationSec == 1.25f &&
                   blur != 0 &&
                   blur->framesRemaining == 1 &&
                   blur->blurPassCount == 4 &&
                   sound != 0 &&
                   strcmp(
                       sound->sampleName,
                       "intro_whoosh"
                   ) == 0 &&
                   sound->voice == 0 &&
                   sound->next == 0;

    script.Cleanup();
    g_zArchive_MountedList = 0;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}
