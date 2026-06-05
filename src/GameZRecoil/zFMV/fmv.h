#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/zVideo/zVideo.h"
#include "recoil/recoil_callconv.h"

#include <vfw.h>

#ifndef _WINDEF_
struct HWND__;
typedef HWND__ *HWND;
#endif

struct zFMV_Action;
struct zFMV_Stream;
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

    zFMV_Playback * Init(
        const char *mediaPath,
        HWND notifyHwnd
    );
    void Destructor();
    void OpenAndPlay(
        unsigned int startMs,
        int endMs,
        int notifyFlag
    );
    void StopAndClose();
    int ReportMciError(unsigned int mciError);
    int SetDestRect(const zFMV_Rect *rect);
};

/*
 * Temporary ABI scaffold for the FMV action virtual table packet. Current BN
 * assembly at 0x4159e0 and 0x462e30 dispatches with ecx=this through VC-style
 * virtual slots; the null words after slot 0x10 in .rdata are alignment
 * padding, not authored reserved fields. Source-faithful recovery should
 * replace this table mirror with the VC5 zFMV_Action virtual class family.
 */
struct zFMV_Action_Vtbl {
    zFMV_Action *( *ScalarDeletingDestructor)(
        zFMV_Action *self,
        unsigned int flags
    );
    int( *Update)(
        zFMV_Action *self,
        double timeSec
    );
    void( *Begin)(
        zFMV_Action *self,
        double timeSec
    );
    void( *End)(zFMV_Action *self);
    void( *RunBlocking)(zFMV_Action *self);
    void *reserved14;
};

struct zFMV_Action {
    zFMV_Action_Vtbl *vftable;
    zFMV_Action *next;

    void Destructor();
    zFMV_Action * ScalarDeletingDestructor(unsigned int flags);
    zFMV_Action * DerivedScalarDeletingDestructor(
        unsigned int flags
    );
    int NoOpUpdate(double timeSec);
    void FlipSurfaces();
    void RunBlockingImmediate();
    void RunBlockingTimed();
};

struct zFMV_ActionImage : zFMV_Action {
    char *imagePath;
    void *image;
    int doAdjustSurfaces;
    int forcePrimaryPostprocess;
    int blitRect[4];

    zFMV_ActionImage * ConstructorWithScreenRect(
        const char *imagePath,
        int doAdjustSurfaces,
        int blitX,
        int blitY
    );
    zFMV_ActionImage * ConstructorScaled(
        const char *imagePath,
        int doAdjustSurfaces
    );
    void Begin(double timeSec);
    int Update(double timeSec);
    void End();
    void Destructor();
    zFMV_ActionImage * ScalarDeletingDestructor(
        unsigned int flags
    );
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

    zFMV_ActionFade * Constructor(
        int red,
        int green,
        int blue,
        unsigned int durationSecRaw,
        int fadeDirectionSign,
        int maxAlpha
    );
    void Begin(double timeSec);
    int Update(double timeSec);
    void End();
};

struct zFMV_ActionPlayAvi : zFMV_Action {
    char *mediaPath;
    int modeFlags;
    zFMV_Stream *stream;
    int reserved14;
    double startTimeSec;
    int lastDecodedFrameIndex;
    zFMV_Rect destRect;
    int reserved34;

    zFMV_ActionPlayAvi * Constructor(
        const char *mediaRootPath,
        const char *mediaFileName,
        int modeFlags
    );
    void Destructor();
    zFMV_ActionPlayAvi * ScalarDeletingDestructor(
        unsigned int flags
    );
    int Update(double timeSec);
    void Begin(double timeSec);
    void End();
};

struct zFMV_ActionPlayMci : zFMV_Action {
    char *mediaPath;
    zFMV_Playback *playback;

    void Begin(double timeSec);
    int Update(double timeSec);
    void End();
    zFMV_ActionPlayMci * Constructor(
        const char *mediaRootPath,
        const char *playbackTitle,
        HWND notifyHwnd
    );
    void Destructor();
    zFMV_ActionPlayMci * ScalarDeletingDestructor(
        unsigned int flags
    );
};

struct zFMV_ActionWait : zFMV_Action {
    float durationSec;
    float startSec;

    void Begin(double timeSec);
    int Update(double timeSec);
};

struct zFMV_ActionBlur : zFMV_Action {
    int framesRemaining;
    int blurPassCount;
    zFMV_Rect swSurfaceRect;
    zFMV_Rect primarySurfaceRect;

    zFMV_ActionBlur * Constructor(
        int framesRemaining,
        int blurPassCount
    );
    void Begin(double timeSec);
    void End();
    int Update(double timeSec);
};

struct zFMV_ActionBlurH : zFMV_ActionBlur {
    int Update(double timeSec);
};

struct zFMV_ActionBlurV : zFMV_ActionBlur {
    int Update(double timeSec);
};

struct zFMV_ActionPlaySound : zFMV_Action {
    zSndSample *sample;
    zSndPlayHandle *voice;
    char sampleName[0x32];
    unsigned char reserved42[2];

    void Begin(double timeSec);
};

struct zFMV_Script {
    char *m_fmvPath;
    HWND m_hWnd;
    double m_startTimeSec;
    int m_abortOnKey;
    zFMV_Action *m_head;
    zFMV_Action *m_tail;
    zFMV_Action *m_cur;

    zFMV_Script * Init(
        const char *zrdPath,
        const char *tagPrefix,
        HWND hWnd
    );
    int LoadActionsFromZrd(
        const char *zrdPath,
        const char *tagPrefix
    );
    int AppendAction(zFMV_Action *action);
    int BeginCurrentAction(double startTimeSec);
    int BeginAtTime();
    int Update(double timeSec);
    int UpdateAtTime();
    int RunBlocking(int abortOnKey);
    void BeginNow(int destroyActions);
    void Cleanup();
    void Reset(int destroyActions);
};

struct zFMV_Stream : zVidImagePartial {
    char *mediaPath;
    int hasVideoStream;
    PAVISTREAM videoStream;
    void *srcFormat;
    void *dstFormat;
    int videoFrameCount;
    AVISTREAMINFOA videoStreamInfo;
    int compressedFrameBufferBytes;
    HIC videoDecompressor;
    void *compressedFrameBuffer;
    int decodedFrameStrideBytes;
    unsigned int videoFramesPerSecond;
    unsigned int msPerFrame;
    int reservedF4;
    int reservedF8;
    int frameWidth;
    int frameHeight;
    unsigned int currentFrameIndex;
    CRITICAL_SECTION criticalSection;
    unsigned char reserved120[0x10];
    int hasAudioStream;
    PAVISTREAM audioStream;
    AVISTREAMINFOA audioStreamInfo;
    void *audioFormat;
    unsigned int audioSegmentBytes;
    void *audioBuffer;
    zSndSample *audioSample;
    int readStreamingAudio;
    unsigned int audioReadSampleIndex;
    int audioRefillSecondHalfNext;
    int modeFlags;

    void Constructor();
    zFMV_Stream * Init(
        const char *mediaPath,
        int modeFlags
    );
    void OpenAudio();
    int ReadAndDecodeFrame(unsigned int frameIndex);
    int FillAudioBuffer(
        unsigned int offset,
        unsigned int bytes
    );
    void Destructor();
};

extern zFMV_Action_Vtbl g_zFMV_ActionBase_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionImage_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionFade_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionPlayAvi_Vtable;
extern zFMV_Action_Vtbl g_zFMV_ActionPlayMci_Vtable;
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
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Playback,
        mciPutFlags
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Playback,
        mciDeviceId
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Playback,
        notifyHwnd
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Playback,
        sourceRect
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Playback,
        destinationRect
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Playback,
        mediaPathDup
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_Action) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionWait) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionWait,
        durationSec
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionWait,
        startSec
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionImage) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionImage,
        imagePath
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionImage,
        image
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionImage,
        doAdjustSurfaces
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionImage,
        forcePrimaryPostprocess
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionImage,
        blitRect
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionFade) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionFade,
        fadeDirectionSign
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionFade,
        fadeColorPacked16
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionFade,
        durationSecRaw
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionFade,
        startSec
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionFade,
        capturedFrame
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionFade,
        maxAlpha
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionPlayAvi) == 0x38);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayAvi,
        mediaPath
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayAvi,
        modeFlags
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayAvi,
        stream
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayAvi,
        startTimeSec
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayAvi,
        lastDecodedFrameIndex
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayAvi,
        destRect
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionPlayMci) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayMci,
        mediaPath
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlayMci,
        playback
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionBlur) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionBlurH) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionBlurV) == 0x30);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionBlur,
        framesRemaining
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionBlur,
        blurPassCount
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionBlur,
        swSurfaceRect
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionBlur,
        primarySurfaceRect
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionPlaySound) == 0x44);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlaySound,
        voice
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_ActionPlaySound,
        sampleName
    ) == 0x10
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_Script) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Script,
        m_fmvPath
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Script,
        m_hWnd
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Script,
        m_abortOnKey
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Script,
        m_head
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Script,
        m_tail
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Script,
        m_cur
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(sizeof(zFMV_Stream) == 0x1e4);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        mediaPath
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        hasVideoStream
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        videoStream
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        srcFormat
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        dstFormat
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        videoFrameCount
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        videoStreamInfo
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        compressedFrameBufferBytes
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        videoDecompressor
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        compressedFrameBuffer
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        decodedFrameStrideBytes
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        videoFramesPerSecond
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        msPerFrame
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        frameWidth
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        frameHeight
    ) == 0x100
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        currentFrameIndex
    ) == 0x104
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        criticalSection
    ) == 0x108
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        hasAudioStream
    ) == 0x130
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioStream
    ) == 0x134
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioStreamInfo
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioFormat
    ) == 0x1c4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioSegmentBytes
    ) == 0x1c8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioBuffer
    ) == 0x1cc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioSample
    ) == 0x1d0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        readStreamingAudio
    ) == 0x1d4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioReadSampleIndex
    ) == 0x1d8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        audioRefillSecondHalfNext
    ) == 0x1dc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zFMV_Stream,
        modeFlags
    ) == 0x1e0
);
#endif
