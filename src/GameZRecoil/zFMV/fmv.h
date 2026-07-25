#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>
#include <string.h>

#include "GameZRecoil/zVideo/zvid.h"
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

/**
 * Original inline helper evidence: no standalone retail function exists;
 * callers at 0x462330, 0x4631af, 0x463221, 0x4635af, 0x463b2f, and 0x463d50
 * contain the complete C-string duplication operation.
 * Purpose: duplicate an input C string for FMV objects that own their paths.
 */
static inline char *DuplicateCString(
    const char *value
) {
#if defined(_MSC_VER)
    return _strdup(value);
#else
    return strdup(value);
#endif
}

struct zFMV_Rect {
    int left;
    int top;
    int right;
    int bottom;
};

extern "C" zVidRect32 g_zFMV_ActionImage_BlitRect;
extern "C" zVidRect32 g_zFMV_ActionImage_ActiveRegion;

struct zFMV_Playback {
    int mciPutFlags;
    unsigned short mciDeviceId;
    unsigned short reserved06;
    HWND notifyHwnd;
    zFMV_Rect sourceRect;
    zFMV_Rect destinationRect;
    char *mediaPathDup;

    /**
     * Original inline helper evidence: default local setup construction has no standalone retail body.
     * Purpose: let tests and stack setup create playback records before explicit field initialization.
     */
    zFMV_Playback() {}
    /**
     * Purpose: initialize playback state with duplicated media path and notify window.
     */
    zFMV_Playback(
        const char *mediaPath,
        HWND notifyHwnd
    );
    ~zFMV_Playback();
    void OpenAndPlay(
        unsigned int startMs,
        int endMs,
        int notifyFlag
    );
    void StopAndClose();
    int ReportMciError(unsigned int mciError);
    int SetDestRect(const zFMV_Rect *rect);
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zfmv.fmv.zfmv-action.type
 * @recoil-artifact emits .text recoil:function:0x415a80: VC5-generated deleting destructor.
 * Purpose: define the polymorphic FMV action base whose virtual destructor causes VC5 to emit the deleting-destructor helper.
 */
struct zFMV_Action {
    zFMV_Action *next;

    /**
     * Original inline helper evidence: derived action constructors store the
     * base next pointer before installing their concrete action vtable.
     * Purpose: initialize the action-list link for all FMV action records.
     */
    zFMV_Action() {
        next = 0;
    }
    virtual ~zFMV_Action();
    virtual int Update(double timeSec);
    virtual void Begin(double timeSec);
    virtual void End();
    virtual void RunBlocking();
    void FlipSurfaces();
    void RunBlockingImmediate();
    void RunBlockingTimed();
};

struct zFMV_ActionImage : zFMV_Action {
    char *imagePath;
    void *image;
    int doAdjustSurfaces;
    int forcePrimaryPostprocess;
    zVidRect32 blitRect;

    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the image-action vtable for local setup.
     */
    zFMV_ActionImage() {}
    /**
     * Purpose: initialize an image action with an explicit screen blit origin.
     */
    zFMV_ActionImage(
        const char *imagePath,
        int doAdjustSurfaces,
        int blitX,
        int blitY
    );
    /**
     * Purpose: initialize an image action sized to the active render region.
     */
    zFMV_ActionImage(
        const char *imagePath,
        int doAdjustSurfaces
    );
    void Begin(double timeSec);
    int Update(double timeSec);
    void End();
    ~zFMV_ActionImage();
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

    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the fade-action vtable for local setup.
     */
    zFMV_ActionFade() {}
    /**
     * Purpose: initialize fade color, duration, direction, and alpha settings.
     */
    zFMV_ActionFade(
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

    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the PlayAvi-action vtable for local setup.
     */
    zFMV_ActionPlayAvi() {}
    /**
     * Purpose: build the AVI media path, resolve CD-ROM fallback, and store mode flags.
     */
    zFMV_ActionPlayAvi(
        const char *mediaRootPath,
        const char *mediaFileName,
        int modeFlags
    );
    ~zFMV_ActionPlayAvi();
    int Update(double timeSec);
    void Begin(double timeSec);
    void End();
};

struct zFMV_ActionPlayMci : zFMV_Action {
    char *mediaPath;
    zFMV_Playback *playback;

    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the PlayMci-action vtable for local setup.
     */
    zFMV_ActionPlayMci() {}
    void Begin(double timeSec);
    int Update(double timeSec);
    void End();
    /**
     * Purpose: build the MCI media path, create playback state, and set its destination rect.
     */
    zFMV_ActionPlayMci(
        HWND notifyHwnd,
        const char *mediaRootPath,
        const char *playbackTitle
    );
    ~zFMV_ActionPlayMci();
};

struct zFMV_ActionWait : zFMV_Action {
    float durationSec;
    float startSec;

    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the wait-action vtable for local setup.
     */
    zFMV_ActionWait() {}
    /**
     * Original inline helper evidence: Retail LoadActionsFromZrd inlines wait-action construction.
     * Purpose: initialize a wait action duration before Begin records the start time.
     */
    zFMV_ActionWait(
        float durationSecParam
    ) {
        durationSec = durationSecParam;
    }
    void Begin(double timeSec);
    int Update(double timeSec);
    void End();
};

struct zFMV_ActionBlur : zFMV_Action {
    int framesRemaining;
    int blurPassCount;
    zFMV_Rect swSurfaceRect;
    zFMV_Rect primarySurfaceRect;

    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the blur-action vtable for local setup.
     */
    zFMV_ActionBlur() {}
    /**
     * Purpose: initialize a blur action's frame count and pass count.
     */
    zFMV_ActionBlur(
        int framesRemaining,
        int blurPassCount
    );
    void Begin(double timeSec);
    void End();
    int Update(double timeSec);
    void RunBlocking();
};

struct zFMV_ActionBlurH : zFMV_ActionBlur {
    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the horizontal-blur vtable for local setup.
     */
    zFMV_ActionBlurH() {}
    /**
     * Original inline helper evidence: Retail constructs horizontal blur actions through the blur constructor body plus the derived vtable.
     * Purpose: initialize horizontal blur action state while preserving the derived update dispatch.
     */
    zFMV_ActionBlurH(
        int framesRemaining,
        int blurPassCount
    ) : zFMV_ActionBlur(
            framesRemaining,
            blurPassCount
        ) {}
    int Update(double timeSec);
};

struct zFMV_ActionBlurV : zFMV_ActionBlur {
    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the vertical-blur vtable for local setup.
     */
    zFMV_ActionBlurV() {}
    /**
     * Original inline helper evidence: Retail constructs vertical blur actions through the blur constructor body plus the derived vtable.
     * Purpose: initialize vertical blur action state while preserving the derived update dispatch.
     */
    zFMV_ActionBlurV(
        int framesRemaining,
        int blurPassCount
    ) : zFMV_ActionBlur(
            framesRemaining,
            blurPassCount
        ) {}
    int Update(double timeSec);
};

struct zFMV_ActionPlaySound : zFMV_Action {
    zSndSample *sample;
    zSndPlayHandle *voice;
    char sampleName[0x32];
    unsigned char reserved42[2];

    /**
     * Original inline helper evidence: No standalone retail function is expected for default test/setup construction.
     * Purpose: let compiler-generated construction install the sound-action vtable for local setup.
     */
    zFMV_ActionPlaySound() {}
    /**
     * Original inline helper evidence: Retail LoadActionsFromZrd inlines sound-action construction.
     * Purpose: copy the sample name and clear the playback voice before action start.
     */
    zFMV_ActionPlaySound(
        const char *sampleNameParam
    ) {
        strncpy(
            sampleName,
            sampleNameParam,
            0x32
        );
        voice = 0;
    }
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

extern "C" zFMV_Rect g_zFMV_ActionPlayMci_DestRect;

#if defined(_M_IX86) || defined(__i386__)
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
