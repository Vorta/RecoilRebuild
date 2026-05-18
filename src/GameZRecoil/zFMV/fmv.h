#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include <windows.h>

#include "recoil/recoil_callconv.h"

#if defined(_MSC_VER) && _MSC_VER >= 1300
#define RECOIL_FMV_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define RECOIL_FMV_NOINLINE __attribute__((noinline))
#else
#define RECOIL_FMV_NOINLINE
#endif

struct zFMV_Action;
struct zSndPlayHandle;
struct zSndSample;

struct zFMV_Rect {
    int left;
    int top;
    int right;
    int bottom;
};

struct zFMV_Playback {
    int mciPutFlags;
    unsigned short mciDeviceId;
    unsigned short reserved06;
    HWND notifyHwnd;
    zFMV_Rect sourceRect;
    zFMV_Rect destinationRect;
    char *mediaPathDup;

    RECOIL_FMV_NOINLINE zFMV_Playback *RECOIL_THISCALL Init(const char *mediaPath, HWND notifyHwnd);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL OpenAndPlay(unsigned int startMs, int endMs,
                                                         int notifyFlag);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL StopAndClose();
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL ReportMciError(unsigned int mciError);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL SetDestRect(const zFMV_Rect *rect);
};

struct zFMV_Action_Vtbl {
    zFMV_Action *(RECOIL_THISCALL *ScalarDeletingDestructor)(zFMV_Action *self,
                                                             unsigned int flags);
    int(RECOIL_THISCALL *Update)(zFMV_Action *self, double timeSec);
    void(RECOIL_THISCALL *Begin)(zFMV_Action *self, double timeSec);
    void(RECOIL_THISCALL *End)(zFMV_Action *self);
    void(RECOIL_THISCALL *RunBlocking)(zFMV_Action *self);
    void *reserved14;
};

struct zFMV_Action {
    zFMV_Action_Vtbl *vftable;
    zFMV_Action *next;

    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_FMV_NOINLINE zFMV_Action *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    RECOIL_FMV_NOINLINE zFMV_Action *RECOIL_THISCALL
    DerivedScalarDeletingDestructor(unsigned int flags);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL NoOpBegin(double timeSec);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL NoOpEnd();
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL NoOpUpdate(double timeSec);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL FlipSurfaces();
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL RunBlockingImmediate();
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL RunBlockingTimed();
};

struct zFMV_ActionImage : zFMV_Action {
    char *imagePath;
    void *image;
    int doAdjustSurfaces;
    int forcePrimaryPostprocess;
    int blitRect[4];

    RECOIL_FMV_NOINLINE zFMV_ActionImage *RECOIL_THISCALL
    ConstructorWithScreenRect(const char *imagePath, int doAdjustSurfaces,
                              int blitX, int blitY);
    RECOIL_FMV_NOINLINE zFMV_ActionImage *RECOIL_THISCALL
    ConstructorScaled(const char *imagePath, int doAdjustSurfaces);
};

struct zFMV_ActionFade : zFMV_Action {
    int fadeDirectionSign;
    unsigned short fadeColorPacked16;
    unsigned short reserved0e;
    unsigned int durationSecRaw;
    int reserved14;
    double startSec;
    void *capturedFrame;
    int maxAlpha;

    RECOIL_FMV_NOINLINE zFMV_ActionFade *RECOIL_THISCALL Constructor(
        int red, int green, int blue, unsigned int durationSecRaw,
        int fadeDirectionSign, int maxAlpha);
};

struct zFMV_ActionPlayAvi : zFMV_Action {
    char *mediaPath;
    int modeFlags;
    int reserved10[10];

    RECOIL_FMV_NOINLINE zFMV_ActionPlayAvi *RECOIL_THISCALL Constructor(const char *mediaRootPath,
                                                                        const char *mediaFileName,
                                                                        int modeFlags);
};

struct zFMV_ActionPlayMci : zFMV_Action {
    char *mediaPath;
    zFMV_Playback *playback;

    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Begin(double timeSec);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL End();
    RECOIL_FMV_NOINLINE zFMV_ActionPlayMci *RECOIL_THISCALL Constructor(const char *mediaRootPath,
                                                                        const char *playbackTitle,
                                                                        HWND notifyHwnd);
};

struct zFMV_ActionWait : zFMV_Action {
    float durationSec;
    float startSec;

    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Begin(double timeSec);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL Update(double timeSec);
};

struct zFMV_ActionBlur : zFMV_Action {
    int framesRemaining;
    int blurPassCount;
    zFMV_Rect swSurfaceRect;
    zFMV_Rect primarySurfaceRect;

    RECOIL_FMV_NOINLINE zFMV_ActionBlur *RECOIL_THISCALL Constructor(int framesRemaining,
                                                                     int blurPassCount);
};

struct zFMV_ActionPlaySound : zFMV_Action {
    zSndSample *sample;
    zSndPlayHandle *voice;
    char sampleName[0x32];
    unsigned char reserved42[2];

    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Begin(double timeSec);
};

struct zFMV_Script {
    char *m_fmvPath;
    HWND m_hWnd;
    double m_startTimeSec;
    int m_abortOnKey;
    zFMV_Action *m_head;
    zFMV_Action *m_tail;
    zFMV_Action *m_cur;

    RECOIL_FMV_NOINLINE zFMV_Script *RECOIL_THISCALL Init(const char *zrdPath,
                                                          const char *tagPrefix, HWND hWnd);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL LoadActionsFromZrd(const char *zrdPath,
                                                                        const char *tagPrefix);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL AppendAction(zFMV_Action *action);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL BeginCurrentAction(double startTimeSec);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL BeginAtTime();
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL Update(double timeSec);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL UpdateAtTime();
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL RunBlocking(int abortOnKey);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL BeginNow(int destroyActions);
    void RECOIL_THISCALL Cleanup();
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Reset(int destroyActions);
};

struct zFMV_Stream {
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Constructor();
    RECOIL_FMV_NOINLINE zFMV_Stream *RECOIL_THISCALL Init(const char *mediaPath,
                                                          int modeFlags);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL OpenAudio();
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL ReadAndDecodeFrame(unsigned int frameIndex);
    RECOIL_FMV_NOINLINE int RECOIL_THISCALL FillAudioBuffer(unsigned int offset,
                                                                      unsigned int bytes);
    RECOIL_FMV_NOINLINE void RECOIL_THISCALL Destructor();
};

extern zFMV_Action_Vtbl g_zFMV_ActionBase_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionWait_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionPlaySound_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionBlur_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionBlurH_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionBlurV_Vtable;
extern "C" zFMV_Rect g_zFMV_ActionPlayMci_DestRect;

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zFMV_Action_Vtbl) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zFMV_Rect) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zFMV_Playback) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Playback, mciPutFlags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Playback, mciDeviceId) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Playback, notifyHwnd) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Playback, sourceRect) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Playback, destinationRect) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Playback, mediaPathDup) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zFMV_Action) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionWait) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionWait, durationSec) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionWait, startSec) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionImage) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionImage, imagePath) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionImage, image) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionImage, doAdjustSurfaces) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionImage, forcePrimaryPostprocess) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionImage, blitRect) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionFade) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionFade, fadeDirectionSign) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionFade, fadeColorPacked16) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionFade, durationSecRaw) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionFade, startSec) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionFade, capturedFrame) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionFade, maxAlpha) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionPlayAvi) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionPlayAvi, mediaPath) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionPlayAvi, modeFlags) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionPlayMci) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionPlayMci, mediaPath) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionPlayMci, playback) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionBlur) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionBlur, framesRemaining) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionBlur, blurPassCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionBlur, swSurfaceRect) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionBlur, primarySurfaceRect) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionPlaySound) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionPlaySound, voice) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zFMV_ActionPlaySound, sampleName) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zFMV_Script) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Script, m_fmvPath) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Script, m_hWnd) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Script, m_abortOnKey) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Script, m_head) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Script, m_tail) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zFMV_Script, m_cur) == 0x1c);
#endif
