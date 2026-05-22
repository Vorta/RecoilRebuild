#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <mmsystem.h>
#include <vfw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern "C" HWND g_RecoilApp_hWndMain = 0;

extern "C" int g_zFMV_ActionImage_BlitRectX = 0;
extern "C" int g_zFMV_ActionImage_BlitRectY = 0;
extern "C" int g_zFMV_ActionImage_BlitRectW = 0;
extern "C" int g_zFMV_ActionImage_BlitRectH = 0;
extern "C" int g_zFMV_ActionImage_ActiveRegionX = 0;
extern "C" int g_zFMV_ActionImage_ActiveRegionY = 0;
extern "C" int g_zFMV_ActionImage_ActiveRegionW = 0;
extern "C" int g_zFMV_ActionImage_ActiveRegionH = 0;
extern "C" zFMV_Rect g_zFMV_ActionPlayMci_DestRect = {0};

namespace {
const unsigned int k_zFMV_ActionImage_VtblAddress = 0x4d2598;
const unsigned int k_zFMV_ActionFade_VtblAddress = 0x4d25b0;
const unsigned int k_zFMV_ActionPlayAvi_VtblAddress = 0x4d25c8;
const unsigned int k_zFMV_ActionPlayMci_VtblAddress = 0x4d25f8;
const char *kFMVMainSourceFile = "D:\\Proj\\GameZRecoil\\zFMV\\fmv_main.cpp";
const char *kFMVStreamSourceFile = "D:\\Proj\\GameZRecoil\\zFMV\\fmv_stream.cpp";
const char *kMpegVideoDeviceType = "MPEGVideo";
const char *kUnknownMciErrorText = "Unknown Error ID";
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

struct zFMV_MciOpenParams {
    DWORD_PTR callback;
    unsigned int deviceId;
    const char *deviceType;
    const char *elementName;
    const char *alias;
};

struct zFMV_MciWindowParams {
    DWORD_PTR callback;
    HWND hwnd;
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

template <typename T> T &FieldAt(void *base, size_t offset) {
    return *(T *)(static_cast<unsigned char *>(base) + offset);
}

typedef void (RECOIL_THISCALL *zFMV_ImageEnsureSurfaceProc)(zVidImagePartial *image);

char *DuplicateCString(const char *value) {
#if defined(_MSC_VER)
    return _strdup(value);
#else
    return strdup(value);
#endif
}

zFMV_Action_Vtbl MakeBaseActionVtable() {
    union DtorThunk {
        zFMV_Action *(RECOIL_THISCALL zFMV_Action::*member)(unsigned int);
        zFMV_Action *(RECOIL_THISCALL *function)(zFMV_Action *, unsigned int);
    };
    union UpdateThunk {
        int (RECOIL_THISCALL zFMV_Action::*member)(double);
        int(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union BeginThunk {
        void (RECOIL_THISCALL zFMV_Action::*member)(double);
        void(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union EndThunk {
        void (RECOIL_THISCALL zFMV_Action::*member)();
        void(RECOIL_THISCALL *function)(zFMV_Action *);
    };

    DtorThunk dtor = {0};
    dtor.member = &zFMV_Action::ScalarDeletingDestructor;
    UpdateThunk update = {0};
    update.member = &zFMV_Action::NoOpUpdate;
    BeginThunk begin = {0};
    begin.member = &zFMV_Action::NoOpBegin;
    EndThunk end = {0};
    end.member = &zFMV_Action::NoOpEnd;
    EndThunk runBlocking = {0};
    runBlocking.member = &zFMV_Action::RunBlockingTimed;
    zFMV_Action_Vtbl vtable = {
        dtor.function, update.function, begin.function, end.function, runBlocking.function, 0,
    };
    return vtable;
}

zFMV_Action_Vtbl MakeSimpleActionVtable() {
    union DtorThunk {
        zFMV_Action *(RECOIL_THISCALL zFMV_Action::*member)(unsigned int);
        zFMV_Action *(RECOIL_THISCALL *function)(zFMV_Action *, unsigned int);
    };
    union UpdateThunk {
        int (RECOIL_THISCALL zFMV_Action::*member)(double);
        int(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union BeginThunk {
        void (RECOIL_THISCALL zFMV_Action::*member)(double);
        void(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union EndThunk {
        void (RECOIL_THISCALL zFMV_Action::*member)();
        void(RECOIL_THISCALL *function)(zFMV_Action *);
    };

    DtorThunk dtor = {0};
    dtor.member = &zFMV_Action::DerivedScalarDeletingDestructor;
    UpdateThunk update = {0};
    update.member = &zFMV_Action::NoOpUpdate;
    BeginThunk begin = {0};
    begin.member = &zFMV_Action::NoOpBegin;
    EndThunk end = {0};
    end.member = &zFMV_Action::NoOpEnd;
    EndThunk runBlocking = {0};
    runBlocking.member = &zFMV_Action::RunBlockingTimed;
    zFMV_Action_Vtbl vtable = {
        dtor.function, update.function, begin.function, end.function, runBlocking.function, 0,
    };
    return vtable;
}

zFMV_Action_Vtbl MakeWaitActionVtable() {
    union DtorThunk {
        zFMV_Action *(RECOIL_THISCALL zFMV_Action::*member)(unsigned int);
        zFMV_Action *(RECOIL_THISCALL *function)(zFMV_Action *, unsigned int);
    };
    union UpdateThunk {
        int (RECOIL_THISCALL zFMV_ActionWait::*member)(double);
        int(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union BeginThunk {
        void (RECOIL_THISCALL zFMV_ActionWait::*member)(double);
        void(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union EndThunk {
        void (RECOIL_THISCALL zFMV_Action::*member)();
        void(RECOIL_THISCALL *function)(zFMV_Action *);
    };

    DtorThunk dtor = {0};
    dtor.member = &zFMV_Action::DerivedScalarDeletingDestructor;
    UpdateThunk update = {0};
    update.member = &zFMV_ActionWait::Update;
    BeginThunk begin = {0};
    begin.member = &zFMV_ActionWait::Begin;
    EndThunk end = {0};
    end.member = &zFMV_Action::FlipSurfaces;
    EndThunk runBlocking = {0};
    runBlocking.member = &zFMV_Action::RunBlockingTimed;
    zFMV_Action_Vtbl vtable = {
        dtor.function, update.function, begin.function, end.function, runBlocking.function, 0,
    };
    return vtable;
}

zFMV_Action_Vtbl MakePlaySoundActionVtable() {
    union DtorThunk {
        zFMV_Action *(RECOIL_THISCALL zFMV_Action::*member)(unsigned int);
        zFMV_Action *(RECOIL_THISCALL *function)(zFMV_Action *, unsigned int);
    };
    union UpdateThunk {
        int (RECOIL_THISCALL zFMV_Action::*member)(double);
        int(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union BeginThunk {
        void (RECOIL_THISCALL zFMV_ActionPlaySound::*member)(double);
        void(RECOIL_THISCALL *function)(zFMV_Action *, double);
    };
    union EndThunk {
        void (RECOIL_THISCALL zFMV_Action::*member)();
        void(RECOIL_THISCALL *function)(zFMV_Action *);
    };

    DtorThunk dtor = {0};
    dtor.member = &zFMV_Action::DerivedScalarDeletingDestructor;
    UpdateThunk update = {0};
    update.member = &zFMV_Action::NoOpUpdate;
    BeginThunk begin = {0};
    begin.member = &zFMV_ActionPlaySound::Begin;
    EndThunk end = {0};
    end.member = &zFMV_Action::NoOpEnd;
    EndThunk runBlocking = {0};
    runBlocking.member = &zFMV_Action::RunBlockingTimed;
    zFMV_Action_Vtbl vtable = {
        dtor.function, update.function, begin.function, end.function, runBlocking.function, 0,
    };
    return vtable;
}

zReader::Node *ArrayBase(zReader::Node *node) {
    return node->value.nodes;
}

int ArrayCount(zReader::Node *node) {
    return ArrayBase(node)[0].value.i32;
}

zReader::Node *ArrayItem(zReader::Node *node, int index) {
    return &ArrayBase(node)[index];
}

const char *StringArg(zReader::Node *actionNode, int index) {
    zReader::Node *arg = ArrayItem(actionNode, index);
    return arg->type == zReader::ZRDR_NODE_STRING ? arg->value.str : 0;
}

int IntArg(zReader::Node *actionNode, int index) {
    zReader::Node *arg = ArrayItem(actionNode, index);
    return arg->value.i32;
}

unsigned int RawArg(zReader::Node *actionNode, int index) {
    zReader::Node *arg = ArrayItem(actionNode, index);
    return arg->value.u32;
}

float FloatArg(zReader::Node *actionNode, int index) {
    zReader::Node *arg = ArrayItem(actionNode, index);
    if (arg->type == zReader::ZRDR_NODE_INT) {
        return static_cast<float>(arg->value.i32);
    }

    return arg->value.f32;
}

zFMV_ActionImage *NewImageAction() {
    return static_cast<zFMV_ActionImage *>(::operator new(sizeof(zFMV_ActionImage)));
}

zFMV_ActionFade *NewFadeAction() {
    return static_cast<zFMV_ActionFade *>(::operator new(sizeof(zFMV_ActionFade)));
}

zFMV_ActionPlayAvi *NewPlayAviAction() {
    return static_cast<zFMV_ActionPlayAvi *>(
        ::operator new(sizeof(zFMV_ActionPlayAvi)));
}

zFMV_ActionPlayMci *NewPlayMciAction() {
    return static_cast<zFMV_ActionPlayMci *>(
        ::operator new(sizeof(zFMV_ActionPlayMci)));
}

zFMV_ActionBlur *NewBlurAction() {
    return static_cast<zFMV_ActionBlur *>(::operator new(sizeof(zFMV_ActionBlur)));
}

zFMV_Action *BuildAction(zFMV_Script *script, zReader::Node *actionNode) {
    zReader::Node *actionArray = ArrayBase(actionNode);
    const char *actionTag = actionArray[1].value.str;

    if (strcmp(actionTag, "SHOWIMAGE") == 0) {
        zFMV_ActionImage *action = NewImageAction();
        return action != 0 ? action->ConstructorScaled(StringArg(actionNode, 2), 1) : 0;
    }

    if (strcmp(actionTag, "BLITIMAGE") == 0) {
        zFMV_ActionImage *action = NewImageAction();
        return action != 0
                   ? action->ConstructorWithScreenRect(StringArg(actionNode, 2), 1,
                                                       IntArg(actionNode, 3), IntArg(actionNode, 4))
                   : 0;
    }

    if (strcmp(actionTag, "LOADIMAGE") == 0) {
        zFMV_ActionImage *action = NewImageAction();
        return action != 0 ? action->ConstructorScaled(StringArg(actionNode, 2), 0) : 0;
    }

    if (strcmp(actionTag, "WAIT") == 0) {
        zFMV_ActionWait * action = static_cast<zFMV_ActionWait *>(::operator new(sizeof(zFMV_ActionWait)));
        if (action == 0) {
            return 0;
        }

        action->next = 0;
        action->vftable = &g_zFMV_ActionWait_Vtable;
        action->durationSec = FloatArg(actionNode, 2);
        return action;
    }

    if (strcmp(actionTag, "FADEIN") == 0 || strcmp(actionTag, "FADEOUT") == 0) {
        zFMV_ActionFade *action = NewFadeAction();
        if (action == 0) {
            return 0;
        }

        zReader::Node *color = ArrayItem(actionNode, 2);
        const int direction = strcmp(actionTag, "FADEIN") == 0 ? -1 : 1;
        return action->Constructor(ArrayBase(color)[1].value.i32, ArrayBase(color)[2].value.i32,
                                   ArrayBase(color)[3].value.i32, RawArg(actionNode, 3), direction,
                                   IntArg(actionNode, 4));
    }

    if (strcmp(actionTag, "PLAYAVI") == 0) {
        zFMV_ActionPlayAvi *action = NewPlayAviAction();
        if (action == 0) {
            return 0;
        }

        const int modeFlags = ArrayCount(actionNode) > 3 ? IntArg(actionNode, 3) : 0;
        return action->Constructor(script->m_fmvPath, StringArg(actionNode, 2), modeFlags);
    }

    if (strcmp(actionTag, "PLAYMCI") == 0) {
        zFMV_ActionPlayMci *action = NewPlayMciAction();
        return action != 0 ? action->Constructor(script->m_fmvPath, StringArg(actionNode, 2),
                                                       script->m_hWnd)
                                 : 0;
    }

    if (strcmp(actionTag, "BLUR") == 0 || strcmp(actionTag, "BLURH") == 0 ||
        strcmp(actionTag, "BLURV") == 0) {
        zFMV_ActionBlur *action = NewBlurAction();
        if (action == 0) {
            return 0;
        }

        action->Constructor(1, IntArg(actionNode, 2));
        if (strcmp(actionTag, "BLURH") == 0) {
            action->vftable = &g_zFMV_ActionBlurH_Vtable;
        } else if (strcmp(actionTag, "BLURV") == 0) {
            action->vftable = &g_zFMV_ActionBlurV_Vtable;
        }
        return action;
    }

    if (strcmp(actionTag, "PLAYSOUND") == 0) {
        zFMV_ActionPlaySound * action = static_cast<zFMV_ActionPlaySound *>(
            ::operator new(sizeof(zFMV_ActionPlaySound)));
        if (action == 0) {
            return 0;
        }

        action->next = 0;
        action->vftable = &g_zFMV_ActionPlaySound_Vtable;
        strncpy(action->sampleName, StringArg(actionNode, 2), 0x32);
        action->voice = 0;
        return action;
    }

    return 0;
}
} // namespace

zFMV_Action_Vtbl g_zFMV_ActionBase_Vtable = MakeBaseActionVtable();
zFMV_Action_Vtbl g_zFMV_ActionWait_Vtable = MakeWaitActionVtable();
zFMV_Action_Vtbl g_zFMV_ActionPlaySound_Vtable = MakePlaySoundActionVtable();
zFMV_Action_Vtbl g_zFMV_ActionBlur_Vtable = MakeSimpleActionVtable();
zFMV_Action_Vtbl g_zFMV_ActionBlurH_Vtable = MakeSimpleActionVtable();
zFMV_Action_Vtbl g_zFMV_ActionBlurV_Vtable = MakeSimpleActionVtable();

// Reimplements 0x415aa0: zFMV_Action::Destructor
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Action::Destructor() {
    vftable = &g_zFMV_ActionBase_Vtable;
}

// Reimplements 0x415a80: zFMV_Action::ScalarDeletingDestructor
RECOIL_FMV_NOINLINE zFMV_Action *RECOIL_THISCALL
zFMV_Action::ScalarDeletingDestructor(unsigned int flags) {
    zFMV_Action *const self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }
    return self;
}

// Reimplements 0x462e70: zFMV_Action::DerivedScalarDeletingDestructor
RECOIL_FMV_NOINLINE zFMV_Action *RECOIL_THISCALL
zFMV_Action::DerivedScalarDeletingDestructor(unsigned int flags) {
    zFMV_Action *const self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }
    return self;
}

RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Action::NoOpBegin(double) {}

RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Action::NoOpEnd() {}

// Reimplements 0x4159d0: zFMV_Action::NoOpUpdate
RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_Action::NoOpUpdate(double) {
    return 0;
}

// Reimplements 0x462f00: zFMV_Action::FlipSurfaces
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Action::FlipSurfaces() {
    zVideo::AdjustSurfacesIfEnabled(0, 0, 1, 1);
}

// Reimplements 0x462e30: zFMV_Action::RunBlockingImmediate
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Action::RunBlockingImmediate() {
    vftable->Begin(this, 0.0);
    while (vftable->Update(this, 0.0) != 0) {
    }
    vftable->End(this);
}

// Reimplements 0x4159e0: zFMV_Action::RunBlockingTimed
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Action::RunBlockingTimed() {
    const double startSec = static_cast<double>(GetTickCount()) * 0.00100000005;
    vftable->Begin(this, 0.0);
    while (true) {
        const double currentSec = (static_cast<double>(GetTickCount()) * 0.00100000005) - startSec;
        if (vftable->Update(this, currentSec) == 0) {
            break;
        }
    }
    vftable->End(this);
}

// Reimplements 0x462ed0: zFMV_ActionWait::Begin
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_ActionWait::Begin(double timeSec) {
    startSec = static_cast<float>(timeSec);
}

RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_ActionWait::Update(double timeSec) {
    return timeSec < static_cast<double>(startSec + durationSec) ? 1 : 0;
}

// Reimplements 0x462e90: zFMV_ActionPlaySound::Begin
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_ActionPlaySound::Begin(double) {
    sample = zSnd::FindSampleByName(sampleName);
    if (voice != 0) {
        voice->StopIfActive();
    }
    if (sample != 0) {
        voice = sample->PlayA3DSimple(1.0f);
    }
}

// Reimplements 0x463ca0: zFMV_ActionPlayMci::Begin
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_ActionPlayMci::Begin(double) {
    if (playback != 0) {
        playback->OpenAndPlay(0, -1, 0);
    }
}

// Reimplements 0x463cc0: zFMV_ActionPlayMci::End
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_ActionPlayMci::End() {
    zVideo::Dispatch_LockDisplayModeSurfaceState();
    zVidImagePartial *capturedImage = zVideo_buff_CaptureSurfaceToImage(2);
    zVideo::Dispatch_UnlockDisplayModeSurfaceState();

    if (capturedImage != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVid_Image::BlitToActiveTarget(capturedImage, 0, 0, 0, 0);
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(0, 0, 0, 1);
    }

    if (playback != 0) {
        playback->StopAndClose();
    }

    if (capturedImage != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVid_Image::BlitToActiveTarget(capturedImage, 0, 0, 0, 0);
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(0, 0, 0, 1);
        zVid_Image::ReleaseIfNotDefault(capturedImage);
    }
}

// Reimplements 0x462330: zFMV_Playback::Init
RECOIL_FMV_NOINLINE zFMV_Playback *RECOIL_THISCALL zFMV_Playback::Init(const char *mediaPath,
                                                                       HWND hwnd) {
    mediaPathDup = DuplicateCString(mediaPath);
    notifyHwnd = hwnd;
    mciPutFlags = 0;
    return this;
}

// Reimplements 0x462360: zFMV_Playback::Destructor
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Playback::Destructor() {
    free(mediaPathDup);
}

// Reimplements 0x462570: zFMV_Playback::ReportMciError
RECOIL_FMV_NOINLINE int RECOIL_THISCALL
zFMV_Playback::ReportMciError(unsigned int mciError) {
    char errorText[0x80];
    if (mciGetErrorStringA(mciError, errorText, sizeof(errorText)) == 0) {
        strcpy(errorText, kUnknownMciErrorText);
    }

    zError::ReportOld(0x200, kFMVMainSourceFile, 0xc4, errorText);
    return 0;
}

// Reimplements 0x462370: zFMV_Playback::OpenAndPlay
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Playback::OpenAndPlay(unsigned int startMs,
                                                                    int endMs,
                                                                    int notifyFlag) {
    zVideo_dd::FlipToGDIIfAttached();

    zFMV_MciOpenParams openParams = {0};
    openParams.deviceType = kMpegVideoDeviceType;
    openParams.elementName = mediaPathDup;
    DWORD mciError = mciSendCommandA(0, 0x803, 0x2202, (DWORD_PTR)(&openParams));
    if (mciError != 0) {
        ReportMciError(mciError);
    }

    mciDeviceId = static_cast<unsigned short>(openParams.deviceId);

    zFMV_MciWindowParams windowParams = {0};
    windowParams.hwnd = notifyHwnd;
    mciError =
        mciSendCommandA(mciDeviceId, 0x841, 0x10002, (DWORD_PTR)(&windowParams));
    if (mciError != 0) {
        ReportMciError(mciError);
        return;
    }

    if ((mciPutFlags & 0x40000) != 0) {
        zFMV_MciRectParams destParams = {0};
        destParams.left = destinationRect.left;
        destParams.top = destinationRect.top;
        destParams.width = destinationRect.right - destinationRect.left;
        destParams.height = destinationRect.bottom - destinationRect.top;
        mciError =
            mciSendCommandA(mciDeviceId, 0x842, 0x50002, (DWORD_PTR)(&destParams));
        if (mciError != 0) {
            ReportMciError(mciError);
            return;
        }
    }

    if ((mciPutFlags & 0x20000) != 0) {
        zFMV_MciRectParams sourceParams = {0};
        sourceParams.left = sourceRect.left;
        sourceParams.top = sourceRect.top;
        sourceParams.width = sourceRect.right - sourceRect.left;
        sourceParams.height = sourceRect.bottom - sourceRect.top;
        mciError = mciSendCommandA(mciDeviceId, 0x842, 0x30002,
                                   (DWORD_PTR)(&sourceParams));
        if (mciError != 0) {
            ReportMciError(mciError);
            return;
        }
    }

    zFMV_MciSetParams setParams = {0};
    setParams.timeFormat = 0x1b;
    setParams.audio = static_cast<DWORD>((unsigned int)(notifyHwnd));
    mciError = mciSendCommandA(mciDeviceId, 0x811, 0x302, (DWORD_PTR)(&setParams));
    if (mciError != 0) {
        ReportMciError(mciError);
        return;
    }

    zFMV_MciPlayParams playParams = {0};
    playParams.callback = (DWORD_PTR)(notifyHwnd);
    playParams.from = startMs;
    DWORD playFlags = 0x6;
    if (endMs >= 0) {
        playParams.to = static_cast<DWORD>(endMs);
        playFlags = 0xe;
    }
    if (notifyFlag == 1) {
        playFlags |= 0x10000;
    }

    mciError =
        mciSendCommandA(mciDeviceId, 0x806, playFlags, (DWORD_PTR)(&playParams));
    if (mciError != 0) {
        ReportMciError(mciError);
    }
}

// Reimplements 0x4624f0: zFMV_Playback::StopAndClose
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Playback::StopAndClose() {
    DWORD mciError = mciSendCommandA(mciDeviceId, 0x808, 0x2, 0);
    if (mciError == 0) {
        zFMV_Playback *self = this;
        mciError = mciSendCommandA(mciDeviceId, 0x804, 0x2, (DWORD_PTR)(&self));
    }

    if (mciError != 0) {
        ReportMciError(mciError);
    }
}

// Reimplements 0x462540: zFMV_Playback::SetDestRect
RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_Playback::SetDestRect(const zFMV_Rect *rect) {
    destinationRect.left = rect->left;
    destinationRect.top = rect->top;
    destinationRect.right = rect->right;
    destinationRect.bottom = rect->bottom;
    const int result = mciPutFlags | 0x40000;
    mciPutFlags = result;
    return result;
}

// Reimplements 0x463ef0: zFMV_Stream::Constructor
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Stream::Constructor() {
    const size_t kMediaPathOffset = 0x38;
    const size_t kHasVideoStreamOffset = 0x3c;
    const size_t kVideoStreamOffset = 0x40;
    const size_t kSrcFormatOffset = 0x44;
    const size_t kDstFormatOffset = 0x48;
    const size_t kVideoFrameCountOffset = 0x4c;
    const size_t kVideoStreamInfoOffset = 0x50;
    const size_t kStreamInfoFccHandlerOffset = 0x54;
    const size_t kStreamInfoDwScaleOffset = 0x64;
    const size_t kStreamInfoDwRateOffset = 0x68;
    const size_t kStreamInfoSuggestedBufferSizeOffset = 0x78;
    const size_t kVideoDecompressorOffset = 0xe0;
    const size_t kCompressedFrameBufferOffset = 0xe4;
    const size_t kDecodedFrameStrideBytesOffset = 0xe8;
    const size_t kVideoFramesPerSecondOffset = 0xec;
    const size_t kMsPerFrameOffset = 0xf0;
    const size_t kReservedF4Offset = 0xf4;
    const size_t kReservedF8Offset = 0xf8;
    const size_t kFrameWidthOffset = 0xfc;
    const size_t kFrameHeightOffset = 0x100;
    const size_t kCurrentFrameIndexOffset = 0x104;
    const size_t kCompressedFrameBufferBytesOffset = 0xdc;

    FieldAt<int>(this, kCurrentFrameIndexOffset) = 0;

    PAVISTREAM &videoStream = FieldAt<PAVISTREAM>(this, kVideoStreamOffset);
    const HRESULT openResult = AVIStreamOpenFromFileA(
        &videoStream, FieldAt<char *>(this, kMediaPathOffset), streamtypeVIDEO, 0, 0x10, 0);
    if (openResult != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0x60, kCannotOpenAviFile);
        AVIFileExit();
        return;
    }

    LONG formatBytes = 0;
    if (AVIStreamReadFormat(videoStream, 0, 0, &formatBytes) != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0x67, kCannotReadAviFormatSize);
        AVIFileExit();
        return;
    }

    void *&srcFormat = FieldAt<void *>(this, kSrcFormatOffset);
    void *&dstFormat = FieldAt<void *>(this, kDstFormatOffset);
    srcFormat = calloc(formatBytes, 1);
    const LONG dstFormatBytes = formatBytes > static_cast<LONG>(sizeof(BITMAPV4HEADER))
                                    ? formatBytes
                                    : static_cast<LONG>(sizeof(BITMAPV4HEADER));
    dstFormat = calloc(dstFormatBytes, 1);

    if (AVIStreamReadFormat(videoStream, 0, srcFormat, &formatBytes) != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0x71, kCannotReadAviFormat);
        AVIFileExit();
        return;
    }

    FieldAt<int>(this, kVideoFrameCountOffset) = AVIStreamLength(videoStream);
    if (AVIStreamInfoA(videoStream,
                       (AVISTREAMINFOA *)(
                           &FieldAt<unsigned char>(this, kVideoStreamInfoOffset)),
                       0x8c) != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0x79, kCannotReadAviStreamInfo);
        AVIFileExit();
        return;
    }

    memcpy(dstFormat, srcFormat, formatBytes);
    BITMAPINFOHEADER *const srcHeader = static_cast<BITMAPINFOHEADER *>(srcFormat);
    BITMAPV4HEADER *const dstHeader = static_cast<BITMAPV4HEADER *>(dstFormat);
    dstHeader->bV4Size = static_cast<DWORD>(dstFormatBytes);
    dstHeader->bV4BitCount = static_cast<WORD>(zVideo::GetDisplayModeBpp());
    dstHeader->bV4V4Compression = BI_BITFIELDS;
    if (dstHeader->bV4BitCount == 24) {
        dstHeader->bV4V4Compression = BI_RGB;
    }
    dstHeader->bV4ClrUsed = 0;
    zVideo::PixelPack_GetRgbMasks((unsigned int *)(&dstHeader->bV4RedMask),
                                  (unsigned int *)(&dstHeader->bV4GreenMask),
                                  (unsigned int *)(&dstHeader->bV4BlueMask));
    dstHeader->bV4AlphaMask = 0;

    const int alignedWidth = (dstHeader->bV4Width + 3) & ~3;
    dstHeader->bV4SizeImage = dstHeader->bV4Height * alignedWidth * (dstHeader->bV4BitCount >> 3);

    int compressedFrameBytes =
        (srcHeader->biBitCount >> 3) * srcHeader->biWidth * srcHeader->biHeight;
    const int suggestedBufferSize =
        FieldAt<int>(this, kStreamInfoSuggestedBufferSizeOffset);
    if (suggestedBufferSize != 0) {
        compressedFrameBytes = suggestedBufferSize;
    }
    FieldAt<int>(this, kCompressedFrameBufferBytesOffset) = compressedFrameBytes;

    FieldAt<HIC>(this, kVideoDecompressorOffset) =
        ICLocate(ICTYPE_VIDEO, FieldAt<DWORD>(this, kStreamInfoFccHandlerOffset),
                 static_cast<LPBITMAPINFOHEADER>(srcFormat),
                 (LPBITMAPINFOHEADER)(dstFormat), ICMODE_DECOMPRESS);
    FieldAt<void *>(this, kCompressedFrameBufferOffset) = calloc(compressedFrameBytes, 1);

    FieldAt<int>(this, kDecodedFrameStrideBytesOffset) =
        (dstHeader->bV4BitCount >> 3) * dstHeader->bV4Width;
    ICSendMessage(FieldAt<HIC>(this, kVideoDecompressorOffset), ICM_DECOMPRESS_BEGIN,
                  (DWORD_PTR)(srcFormat), (DWORD_PTR)(dstFormat));

    const unsigned int rate = FieldAt<unsigned int>(this, kStreamInfoDwRateOffset);
    const unsigned int scale = FieldAt<unsigned int>(this, kStreamInfoDwScaleOffset);
    FieldAt<unsigned int>(this, kVideoFramesPerSecondOffset) = rate / scale;
    FieldAt<int>(this, kReservedF4Offset) = 0;
    FieldAt<int>(this, kReservedF8Offset) = 0;
    FieldAt<unsigned int>(this, kMsPerFrameOffset) = ((rate >> 1) + (scale * 1000)) / rate;

    FieldAt<int>(this, kFrameWidthOffset) = dstHeader->bV4Width;
    FieldAt<int>(this, kFrameHeightOffset) = dstHeader->bV4Height;

    FieldAt<void *>(this, 0x10) = calloc(dstHeader->bV4SizeImage, 1);
    FieldAt<int>(this, 0x00) = static_cast<int>(dstHeader->bV4SizeImage);
    FieldAt<short>(this, 0x04) = static_cast<short>(dstHeader->bV4Width);
    FieldAt<short>(this, 0x06) = static_cast<short>(dstHeader->bV4Height);
    FieldAt<unsigned char>(this, 0x08) = 0;
    FieldAt<unsigned char>(this, 0x09) = 0;
    FieldAt<unsigned char>(this, 0x0a) = 0;
    FieldAt<unsigned char>(this, 0x0b) = 0;
    FieldAt<void *>(this, 0x14) = 0;
    FieldAt<void *>(this, 0x1c) = 0;
    FieldAt<void *>(this, 0x20) = 0;
    FieldAt<void *>(this, 0x24) = 0;
    FieldAt<void *>(this, 0x28) = 0;
    FieldAt<void *>(this, 0x2c) = 0;
    FieldAt<void *>(this, 0x30) = 0;
    FieldAt<void *>(this, 0x18) = 0;
    FieldAt<int>(this, 0x34) = static_cast<short>(dstHeader->bV4Width);

    dstHeader->bV4Height = -dstHeader->bV4Height;
    FieldAt<int>(this, kHasVideoStreamOffset) = 1;
}

// Reimplements 0x4641a0: zFMV_Stream::OpenAudio
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Stream::OpenAudio() {
    const size_t kMediaPathOffset = 0x38;
    const size_t kHasAudioStreamOffset = 0x130;
    const size_t kAudioStreamOffset = 0x134;
    const size_t kAudioStreamInfoOffset = 0x138;
    const size_t kAudioStreamInfoDwLengthOffset = 0x158;
    const size_t kAudioStreamInfoSuggestedBufferSizeOffset = 0x160;
    const size_t kAudioStreamInfoSampleSizeOffset = 0x168;
    const size_t kAudioFormatOffset = 0x1c4;
    const size_t kAudioSegmentBytesOffset = 0x1c8;
    const size_t kAudioBufferOffset = 0x1cc;
    const size_t kAudioSampleOffset = 0x1d0;
    const size_t kReadStreamingAudioOffset = 0x1d4;
    const size_t kAudioReadSampleIndexOffset = 0x1d8;
    const size_t kAudioRefillSecondHalfNextOffset = 0x1dc;
    const size_t kModeFlagsOffset = 0x1e0;

    PAVISTREAM &audioStream = FieldAt<PAVISTREAM>(this, kAudioStreamOffset);
    audioStream = 0;
    if (AVIStreamOpenFromFileA(&audioStream, FieldAt<char *>(this, kMediaPathOffset),
                               streamtypeAUDIO, 0, 0, 0) != 0) {
        return;
    }

    LONG audioFormatBytes = 0;
    if (AVIStreamReadFormat(audioStream, 0, 0, &audioFormatBytes) != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0xcb, kCannotReadAviSoundFormatSize);
        return;
    }

    void *&audioFormat = FieldAt<void *>(this, kAudioFormatOffset);
    audioFormat = calloc(audioFormatBytes, 1);
    if (AVIStreamReadFormat(audioStream, 0, audioFormat, &audioFormatBytes) != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0xd2, kCannotReadAviSoundFormat);
        return;
    }

    if (AVIStreamInfoA(audioStream,
                       (AVISTREAMINFOA *)(
                           &FieldAt<unsigned char>(this, kAudioStreamInfoOffset)),
                       0x8c) != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0xd8, kCannotReadAviSoundStreamInfo);
        return;
    }

    const unsigned int sampleSize = FieldAt<unsigned int>(this, kAudioStreamInfoSampleSizeOffset);
    if (FieldAt<int>(this, kModeFlagsOffset) != 0) {
        const unsigned int segmentBytes =
            FieldAt<unsigned int>(this, kAudioStreamInfoSuggestedBufferSizeOffset);
        FieldAt<unsigned int>(this, kAudioSegmentBytesOffset) = segmentBytes;
        void *const audioBuffer = calloc(segmentBytes * 2, 1);
        FieldAt<void *>(this, kAudioBufferOffset) = audioBuffer;

        if (AVIStreamRead(audioStream, 0, segmentBytes / sampleSize, audioBuffer, segmentBytes,
                          0, 0) != 0) {
            zError::ReportOld(0x400, kFMVStreamSourceFile, 0xe2, kCannotReadAviSoundStream);
            return;
        }

        FieldAt<zSndSample *>(this, kAudioSampleOffset) = zSndSample_CreateQueuedStreamingSample(
            static_cast<WAVEFORMATEX *>(audioFormat), audioBuffer, segmentBytes * 2);
        FieldAt<int>(this, kAudioRefillSecondHalfNextOffset) = 1;
        FieldAt<int>(this, kHasAudioStreamOffset) = 1;
        FieldAt<unsigned int>(this, kAudioReadSampleIndexOffset) = segmentBytes / sampleSize;
        return;
    }

    const unsigned int audioBytes = AVIStreamLength(audioStream) * sampleSize;
    FieldAt<unsigned int>(this, kAudioSegmentBytesOffset) = audioBytes;
    void *const audioBuffer = calloc(audioBytes, 1);
    FieldAt<void *>(this, kAudioBufferOffset) = audioBuffer;

    if (AVIStreamRead(audioStream, 0, FieldAt<unsigned int>(this, kAudioStreamInfoDwLengthOffset),
                      audioBuffer, audioBytes, 0, 0) != 0) {
        zError::ReportOld(0x400, kFMVStreamSourceFile, 0xf0, kCannotReadAviSoundStream);
        return;
    }

    FieldAt<zSndSample *>(this, kAudioSampleOffset) = zSndSample_CreateQueuedStreamingSample(
        static_cast<WAVEFORMATEX *>(audioFormat), audioBuffer, audioBytes);
    FieldAt<int>(this, kHasAudioStreamOffset) = 1;
}

// Reimplements 0x4643a0: zFMV_Stream::ReadAndDecodeFrame
// (D:\Proj\GameZRecoil\zFMV\fmv_stream.cpp)
RECOIL_FMV_NOINLINE int RECOIL_THISCALL
zFMV_Stream::ReadAndDecodeFrame(unsigned int frameIndex) {
    const size_t kPixelsOffset = 0x10;
    const size_t kVideoStreamOffset = 0x40;
    const size_t kSrcFormatOffset = 0x44;
    const size_t kDstFormatOffset = 0x48;
    const size_t kVideoFrameCountOffset = 0x4c;
    const size_t kCompressedFrameBufferBytesOffset = 0xdc;
    const size_t kCodecOffset = 0xe0;
    const size_t kCompressedFrameBufferOffset = 0xe4;
    const size_t kCurrentFrameIndexOffset = 0x104;
    const size_t kCriticalSectionOffset = 0x108;
    const size_t kHasAudioStreamOffset = 0x130;
    const size_t kAudioSegmentBytesOffset = 0x1c8;
    const size_t kAudioSampleOffset = 0x1d0;
    const size_t kReadStreamingAudioOffset = 0x1d4;
    const size_t kAudioRefillSecondHalfNextOffset = 0x1dc;
    const size_t kModeFlagsOffset = 0x1e0;

    unsigned int &currentFrameIndex =
        FieldAt<unsigned int>(this, kCurrentFrameIndexOffset);
    if (frameIndex != 0xffffffffu) {
        currentFrameIndex = frameIndex;
    }

    const unsigned int frameCount = FieldAt<unsigned int>(this, kVideoFrameCountOffset);
    if (static_cast<int>(currentFrameIndex) < static_cast<int>(frameCount)) {
        if (AVIStreamRead(FieldAt<PAVISTREAM>(this, kVideoStreamOffset), currentFrameIndex, 1,
                          FieldAt<void *>(this, kCompressedFrameBufferOffset),
                          FieldAt<int>(this, kCompressedFrameBufferBytesOffset), 0,
                          0) != 0) {
            zError::ReportOld(0x400, kFMVStreamSourceFile, 0x105, kCannotReadAviVideoStream);
            return 0;
        }

        CRITICAL_SECTION &lock = FieldAt<CRITICAL_SECTION>(this, kCriticalSectionOffset);
        EnterCriticalSection(&lock);
        if (ICDecompress((HIC)(FieldAt<void *>(this, kCodecOffset)), 0,
                         static_cast<LPBITMAPINFOHEADER>(FieldAt<void *>(this, kSrcFormatOffset)),
                         FieldAt<void *>(this, kCompressedFrameBufferOffset),
                         static_cast<LPBITMAPINFOHEADER>(FieldAt<void *>(this, kDstFormatOffset)),
                         FieldAt<void *>(this, kPixelsOffset)) != 0) {
            zError::ReportOld(0x400, kFMVStreamSourceFile, 0x10c,
                              kCannotDecompressAviVideoStream);
            return 0;
        }
        LeaveCriticalSection(&lock);
    }

    ++currentFrameIndex;
    if (static_cast<int>(currentFrameIndex) >= static_cast<int>(frameCount)) {
        currentFrameIndex = 0;
    }

    if (FieldAt<int>(this, kHasAudioStreamOffset) != 0) {
        if (FieldAt<int>(this, kReadStreamingAudioOffset) != 0) {
            FieldAt<int>(this, kReadStreamingAudioOffset) = 0;
            FieldAt<zSndSample *>(this, kAudioSampleOffset)->PlayA3DSimple(1.0f);
            return currentFrameIndex;
        }

        if (FieldAt<int>(this, kModeFlagsOffset) != 0) {
            const unsigned int segmentBytes =
                FieldAt<unsigned int>(this, kAudioSegmentBytesOffset);
            const unsigned int playCursor =
                FieldAt<zSndSample *>(this, kAudioSampleOffset)->GetPlayCursorBytes();

            if (FieldAt<int>(this, kAudioRefillSecondHalfNextOffset) != 0) {
                if (playCursor > 0 && playCursor < segmentBytes) {
                    FillAudioBuffer(segmentBytes, segmentBytes);
                    FieldAt<int>(this, kAudioRefillSecondHalfNextOffset) = 0;
                    return currentFrameIndex;
                }
            } else if (playCursor > segmentBytes) {
                FillAudioBuffer(0, segmentBytes);
                FieldAt<int>(this, kAudioRefillSecondHalfNextOffset) = 1;
            }
        }
    }

    return currentFrameIndex;
}

// Reimplements 0x464540: zFMV_Stream::FillAudioBuffer
// (D:\Proj\GameZRecoil\zFMV\fmv_stream.cpp)
RECOIL_FMV_NOINLINE int RECOIL_THISCALL
zFMV_Stream::FillAudioBuffer(unsigned int offset, unsigned int bytes) {
    const size_t kAudioStreamOffset = 0x134;
    const size_t kAudioStreamInfoSampleSizeOffset = 0x168;
    const size_t kAudioSampleOffset = 0x1d0;
    const size_t kAudioReadSampleIndexOffset = 0x1d8;

    void *buffer1Data = 0;
    void *buffer2Data = 0;
    int buffer1Bytes = 0;
    int buffer2Bytes = 0;

    zSndSample *const audioSample = FieldAt<zSndSample *>(this, kAudioSampleOffset);
    const int result =
        audioSample->LockBackendBuffers(offset, bytes, &buffer1Data, &buffer1Bytes, &buffer2Data,
                                        &buffer2Bytes);
    if (result == 0) {
        return result;
    }

    const unsigned int sampleSize =
        FieldAt<unsigned int>(this, kAudioStreamInfoSampleSizeOffset);
    PAVISTREAM const audioStream = FieldAt<PAVISTREAM>(this, kAudioStreamOffset);
    unsigned int &readSampleIndex =
        FieldAt<unsigned int>(this, kAudioReadSampleIndexOffset);

    if (buffer1Bytes != 0) {
        if (AVIStreamRead(audioStream, readSampleIndex,
                          static_cast<LONG>(static_cast<unsigned int>(buffer1Bytes) / sampleSize),
                          buffer1Data, buffer1Bytes, 0, 0) != 0) {
            zError::ReportOld(0x400, kFMVStreamSourceFile, 0x13d, kCannotReadAviSoundStream);
        }
        readSampleIndex += static_cast<unsigned int>(buffer1Bytes) / sampleSize;
    }

    if (buffer2Bytes != 0) {
        if (AVIStreamRead(audioStream, readSampleIndex,
                          static_cast<LONG>(static_cast<unsigned int>(buffer2Bytes) / sampleSize),
                          buffer2Data, buffer2Bytes, 0, 0) != 0) {
            zError::ReportOld(0x400, kFMVStreamSourceFile, 0x144, kCannotReadAviSoundStream);
        }

        // The original advances by the first locked span again after the wrapped read.
        readSampleIndex += static_cast<unsigned int>(buffer1Bytes) / sampleSize;
    }

    return audioSample->UnlockBackendBuffers(buffer1Data, buffer1Bytes, buffer2Data, buffer2Bytes);
}

// Reimplements 0x463d50: zFMV_Stream::Init
RECOIL_FMV_NOINLINE zFMV_Stream *RECOIL_THISCALL zFMV_Stream::Init(const char *mediaPath,
                                                                   int modeFlags) {
    const size_t kMediaPathOffset = 0x38;
    const size_t kSrcFormatOffset = 0x44;
    const size_t kDstFormatOffset = 0x48;
    const size_t kCompressedFrameBufferOffset = 0xe4;
    const size_t kSurfaceOffset = 0x30;
    const size_t kPixelsOffset = 0x10;
    const size_t kAlphaMapOffset = 0x14;
    const size_t kPaletteOffset = 0x18;
    const size_t kAudioSampleOffset = 0x1d0;
    const size_t kAudioFormatOffset = 0x1c4;
    const size_t kHasAudioStreamOffset = 0x130;
    const size_t kHasVideoStreamOffset = 0x3c;
    const size_t kReadStreamingAudioOffset = 0x1d4;
    const size_t kModeFlagsOffset = 0x1e0;
    const size_t kCriticalSectionOffset = 0x108;

    FieldAt<char *>(this, kMediaPathOffset) = DuplicateCString(mediaPath);
    FieldAt<void *>(this, kSrcFormatOffset) = 0;
    FieldAt<void *>(this, kDstFormatOffset) = 0;
    FieldAt<void *>(this, kCompressedFrameBufferOffset) = 0;
    FieldAt<void *>(this, kSurfaceOffset) = 0;
    FieldAt<void *>(this, kPixelsOffset) = 0;
    FieldAt<void *>(this, kAlphaMapOffset) = 0;
    FieldAt<void *>(this, kPaletteOffset) = 0;
    FieldAt<zSndSample *>(this, kAudioSampleOffset) = 0;
    FieldAt<void *>(this, kAudioFormatOffset) = 0;
    FieldAt<int>(this, kHasAudioStreamOffset) = 0;
    FieldAt<int>(this, kHasVideoStreamOffset) = 0;
    FieldAt<int>(this, kReadStreamingAudioOffset) = 1;
    FieldAt<int>(this, kModeFlagsOffset) = modeFlags;

    InitializeCriticalSection(&FieldAt<CRITICAL_SECTION>(this, kCriticalSectionOffset));
    AVIFileInit();
    OpenAudio();
    Constructor();
    return this;
}

// Reimplements 0x463dd0: zFMV_Stream::Destructor
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Stream::Destructor() {
    const size_t kPixelsOffset = 0x10;
    const size_t kAlphaMapOffset = 0x14;
    const size_t kPaletteOffset = 0x18;
    const size_t kSurfaceOffset = 0x30;
    const size_t kMediaPathOffset = 0x38;
    const size_t kHasVideoStreamOffset = 0x3c;
    const size_t kVideoStreamOffset = 0x40;
    const size_t kStreamInfoOffset = 0x44;
    const size_t kBitmapInfoOffset = 0x48;
    const size_t kCodecOffset = 0xe0;
    const size_t kDecompressBufferOffset = 0xe4;
    const size_t kCriticalSectionOffset = 0x108;
    const size_t kHasAudioStreamOffset = 0x130;
    const size_t kAudioStreamOffset = 0x134;
    const size_t kAudioFormatOffset = 0x1c4;
    const size_t kAudioBufferOffset = 0x1cc;
    const size_t kAudioSampleOffset = 0x1d0;

    if (FieldAt<void *>(this, kHasAudioStreamOffset) != 0) {
        void *&audioBuffer = FieldAt<void *>(this, kAudioBufferOffset);
        if (audioBuffer != 0) {
            free(audioBuffer);
            audioBuffer = 0;
        }

        zSndSample *const sample = FieldAt<zSndSample *>(this, kAudioSampleOffset);
        if (sample != 0) {
            sample->Destroy();
        }

        void *&audioFormat = FieldAt<void *>(this, kAudioFormatOffset);
        if (audioFormat != 0) {
            free(audioFormat);
            audioFormat = 0;
        }

        AVIStreamRelease((PAVISTREAM)(FieldAt<void *>(this, kAudioStreamOffset)));
    }

    if (FieldAt<void *>(this, kHasVideoStreamOffset) != 0) {
        HIC const codec = (HIC)(FieldAt<void *>(this, kCodecOffset));
        if (codec != 0) {
            ICSendMessage(codec, ICM_DECOMPRESS_END, 0, 0);
            ICClose(codec);
        }

        free(FieldAt<void *>(this, kStreamInfoOffset));
        free(FieldAt<void *>(this, kBitmapInfoOffset));
        free(FieldAt<void *>(this, kDecompressBufferOffset));

        if (FieldAt<void *>(this, kSurfaceOffset) != 0) {
            ((zFMV_ImageEnsureSurfaceProc)(
                g_zVideo_pfnImageEnsureSurfaceForCurrentDevice))(
                (zVidImagePartial *)(this));
        }

        free(FieldAt<void *>(this, kPixelsOffset));
        free(FieldAt<void *>(this, kAlphaMapOffset));
        free(FieldAt<void *>(this, kPaletteOffset));

        AVIStreamRelease((PAVISTREAM)(FieldAt<void *>(this, kVideoStreamOffset)));
        AVIFileExit();
    }

    DeleteCriticalSection(&FieldAt<CRITICAL_SECTION>(this, kCriticalSectionOffset));
    free(FieldAt<void *>(this, kMediaPathOffset));
}

// Reimplements 0x4625e0: zFMV_Script::Init
RECOIL_FMV_NOINLINE zFMV_Script *RECOIL_THISCALL zFMV_Script::Init(const char *zrdPath,
                                                                   const char *tagPrefix,
                                                                   HWND hWnd) {
    m_hWnd = hWnd != 0 ? hWnd : g_RecoilApp_hWndMain;
    m_fmvPath = 0;
    m_head = 0;
    m_tail = 0;
    m_cur = 0;
    m_abortOnKey = 1;

    if (zrdPath != 0 && tagPrefix != 0) {
        LoadActionsFromZrd(zrdPath, tagPrefix);
    }

    return this;
}

// Reimplements 0x462630: zFMV_Script::Cleanup
void RECOIL_THISCALL zFMV_Script::Cleanup() {
    if (m_fmvPath != 0) {
        free(m_fmvPath);
        m_fmvPath = 0;
    }

    Reset(1);
}

// Reimplements 0x462660: zFMV_Script::Reset
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Script::Reset(int destroyActions) {
    zFMV_Action *action = m_head;
    if (destroyActions != 0) {
        while (action != 0) {
            zFMV_Action *const next = action->next;
            action->vftable->ScalarDeletingDestructor(action, 1);
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

// Reimplements 0x4626b0: zFMV_Script::LoadActionsFromZrd
RECOIL_FMV_NOINLINE int RECOIL_THISCALL
zFMV_Script::LoadActionsFromZrd(const char *zrdPath, const char *tagPrefix) {
    zReader::Node *root = zReader::LoadNodeFromPath(zrdPath, 0, 0);
    if (root == 0) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zFMV\\fmv_script.cpp", 0x51,
                          "Failed to find FMV definitions (fmv.zrd)");
        return -1;
    }

    m_fmvPath = DuplicateCString(zReader::ReadNamedString(root, "FMV_PATH"));
    zImage_InitMissionResources(zReader::ReadNamedString(root, "IMAGE_PATH"));

    zReader::Node *sequenceNode = zReader_GetNamedNode(root, tagPrefix);
    if (sequenceNode == 0) {
        return 0;
    }

    int result = ArrayCount(sequenceNode) - 1;
    if (result > 0) {
        for (int i = 1; i < ArrayCount(sequenceNode); ++i) {
            zReader::Node *actionNode = ArrayItem(sequenceNode, i);
            if (actionNode->type != zReader::ZRDR_NODE_ARRAY) {
                result = 0;
                zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zFMV\\fmv_script.cpp", 0x69,
                                  "Error in parsing fmv actions:  file=%s, tag=%s", zrdPath,
                                  tagPrefix);
                break;
            }

            zFMV_Action *action = BuildAction(this, actionNode);
            if (action != 0) {
                AppendAction(action);
            }
        }
    }

    zReader::FreeLoadedTree(root);
    return result;
}

// Reimplements 0x462f10: zFMV_Script::AppendAction
RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_Script::AppendAction(zFMV_Action *action) {
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

// Reimplements 0x462f90: zFMV_Script::BeginCurrentAction
RECOIL_FMV_NOINLINE int RECOIL_THISCALL
zFMV_Script::BeginCurrentAction(double startTimeSec) {
    if (m_cur == 0) {
        return 0;
    }

    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int height = zVideo::GetPrimarySurfaceHeight();
    const int width = zVideo::GetPrimarySurfaceWidth();
    zVideo::Fx_SetSurfaceState(zVideo::GetPrimarySurfacePixels(), width, height, pitchBytes);
    zSndSampleSet_InitByName("FMV");
    zInput::Keyboard_ResetTransitionState();
    m_startTimeSec = startTimeSec;
    m_cur->vftable->Begin(m_cur, 0.0);
    return 1;
}

// Reimplements 0x4630a0: zFMV_Script::BeginAtTime
RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_Script::BeginAtTime() {
    return BeginCurrentAction(static_cast<double>(timeGetTime()) *
                              static_cast<double>(0.00100000005f));
}

// Reimplements 0x463000: zFMV_Script::Update
RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_Script::Update(double timeSec) {
    if (m_cur == 0) {
        return 0;
    }

    if (m_abortOnKey != 0) {
        if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
            m_cur->vftable->End(m_cur);
            zSndPlayHandleSnapshot::CreateFromActiveSamples()->StopAllIfPlaying();
            m_cur = 0;
            return 0;
        }

        zInput::PollActiveDevices(1);
    }

    const double relativeTimeSec = timeSec - m_startTimeSec;
    if (m_cur->vftable->Update(m_cur, relativeTimeSec) == 0) {
        m_cur->vftable->End(m_cur);
        zFMV_Action *const next = m_cur->next;
        m_cur = next;
        if (next != 0) {
            next->vftable->Begin(next, relativeTimeSec);
        }
    }

    return 1;
}

// Reimplements 0x4630e0: zFMV_Script::UpdateAtTime
RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_Script::UpdateAtTime() {
    return Update(static_cast<double>(timeGetTime()) * static_cast<double>(0.00100000005f));
}

// Reimplements 0x462f50: zFMV_Script::RunBlocking
RECOIL_FMV_NOINLINE int RECOIL_THISCALL zFMV_Script::RunBlocking(int abortOnKey) {
    m_abortOnKey = abortOnKey;
    BeginAtTime();
    if (UpdateAtTime() != 0) {
        do {
        } while (UpdateAtTime() != 0);
    }

    BeginNow(0);
    return 1;
}

// Reimplements 0x463120: zFMV_Script::BeginNow
RECOIL_FMV_NOINLINE void RECOIL_THISCALL zFMV_Script::BeginNow(int destroyActions) {
    Reset(destroyActions);
}

// Reimplements 0x463130: zFMV_ActionImage::ConstructorWithScreenRect
RECOIL_FMV_NOINLINE zFMV_ActionImage *RECOIL_THISCALL zFMV_ActionImage::ConstructorWithScreenRect(
    const char *path, int adjustSurfaces, int blitX, int blitY) {
    next = 0;
    image = 0;
    vftable = (zFMV_Action_Vtbl *)(
        static_cast<unsigned int>(k_zFMV_ActionImage_VtblAddress));
    imagePath = DuplicateCString(path);
    doAdjustSurfaces = adjustSurfaces;
    g_zFMV_ActionImage_BlitRectY = blitY;
    g_zFMV_ActionImage_BlitRectX = blitX;
    forcePrimaryPostprocess = 1;
    blitRect[0] = g_zFMV_ActionImage_BlitRectX;
    blitRect[1] = g_zFMV_ActionImage_BlitRectY;
    blitRect[2] = g_zFMV_ActionImage_BlitRectW;
    blitRect[3] = g_zFMV_ActionImage_BlitRectH;
    return this;
}

// Reimplements 0x4631f0: zFMV_ActionImage::ConstructorScaled
RECOIL_FMV_NOINLINE zFMV_ActionImage *RECOIL_THISCALL
zFMV_ActionImage::ConstructorScaled(const char *path, int adjustSurfaces) {
    next = 0;
    vftable = (zFMV_Action_Vtbl *)(
        static_cast<unsigned int>(k_zFMV_ActionImage_VtblAddress));
    image = 0;
    imagePath = DuplicateCString(path);
    doAdjustSurfaces = adjustSurfaces;
    g_zFMV_ActionImage_ActiveRegionY = 0;
    g_zFMV_ActionImage_ActiveRegionX = 0;
    forcePrimaryPostprocess = 0;

    int discard = 0;
    zRndr::GetActiveRegionState(&g_zFMV_ActionImage_ActiveRegionW,
                                &g_zFMV_ActionImage_ActiveRegionH, &discard, &discard);

    blitRect[0] = g_zFMV_ActionImage_ActiveRegionX;
    blitRect[1] = g_zFMV_ActionImage_ActiveRegionY;
    blitRect[2] = g_zFMV_ActionImage_ActiveRegionW;
    blitRect[3] = g_zFMV_ActionImage_ActiveRegionH;
    return this;
}

// Reimplements 0x4633c0: zFMV_ActionFade::Constructor
RECOIL_FMV_NOINLINE zFMV_ActionFade *RECOIL_THISCALL
zFMV_ActionFade::Constructor(int red, int green, int blue,
                             unsigned int duration, int direction, int alpha) {
    next = 0;
    vftable = (zFMV_Action_Vtbl *)(
        static_cast<unsigned int>(k_zFMV_ActionFade_VtblAddress));
    fadeColorPacked16 = static_cast<unsigned short>(
        zVid_PackColorRGB(static_cast<unsigned char>(red), static_cast<unsigned char>(green),
                          static_cast<unsigned char>(blue)));
    maxAlpha = alpha;
    durationSecRaw = duration;
    fadeDirectionSign = direction;
    return this;
}

// Reimplements 0x463570: zFMV_ActionPlayAvi::Constructor
RECOIL_FMV_NOINLINE zFMV_ActionPlayAvi *RECOIL_THISCALL zFMV_ActionPlayAvi::Constructor(
    const char *mediaRootPath, const char *mediaFileName, int flags) {
    next = 0;
    vftable = (zFMV_Action_Vtbl *)(
        static_cast<unsigned int>(k_zFMV_ActionPlayAvi_VtblAddress));

    const size_t rootLen = strlen(mediaRootPath);
    const size_t fileLen = strlen(mediaFileName);
    mediaPath = static_cast<char *>(calloc(rootLen + fileLen + 0x1b, 1));
    sprintf(mediaPath, "%s\\%s", mediaRootPath, mediaFileName);
    modeFlags = flags;

    struct stat statBuffer;
    if (stat(mediaPath, &statBuffer) == -1) {
        char *resolvedPath = zSys::FindFileOnDriveType(DRIVE_CDROM, mediaPath, 0);
        if (resolvedPath != 0) {
            strcpy(mediaPath, resolvedPath);
        }
    }

    return this;
}

// Reimplements 0x463b00: zFMV_ActionPlayMci::Constructor
RECOIL_FMV_NOINLINE zFMV_ActionPlayMci *RECOIL_THISCALL
zFMV_ActionPlayMci::Constructor(const char *mediaRootPath, const char *playbackTitle, HWND hwnd) {
    next = 0;
    vftable = (zFMV_Action_Vtbl *)(
        static_cast<unsigned int>(k_zFMV_ActionPlayMci_VtblAddress));

    const size_t rootLen = strlen(mediaRootPath);
    const size_t titleLen = strlen(playbackTitle);
    mediaPath = static_cast<char *>(calloc(rootLen + titleLen + 0x1b, 1));
    sprintf(mediaPath, "%s\\%s", mediaRootPath, playbackTitle);

    zFMV_Playback *const playbackStorage = static_cast<zFMV_Playback *>(::operator new(sizeof(zFMV_Playback)));
    zFMV_Playback *initializedPlayback = 0;
    if (playbackStorage != 0) {
        initializedPlayback = playbackStorage->Init(mediaPath, hwnd);
    }
    playback = initializedPlayback;

    g_zFMV_ActionPlayMci_DestRect.top = 0;
    g_zFMV_ActionPlayMci_DestRect.left = 0;
    int discard = 0;
    zRndr::GetActiveRegionState(&g_zFMV_ActionPlayMci_DestRect.right,
                                &g_zFMV_ActionPlayMci_DestRect.bottom, &discard, &discard);
    playback->SetDestRect(&g_zFMV_ActionPlayMci_DestRect);
    return this;
}

// Reimplements 0x463850: zFMV_ActionBlur::Constructor
RECOIL_FMV_NOINLINE zFMV_ActionBlur *RECOIL_THISCALL
zFMV_ActionBlur::Constructor(int framesRemainingParam, int blurPassCountParam) {
    next = 0;
    vftable = &g_zFMV_ActionBlur_Vtable;
    framesRemaining = framesRemainingParam;
    blurPassCount = blurPassCountParam;
    return this;
}
