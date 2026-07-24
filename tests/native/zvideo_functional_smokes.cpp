#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include <windows.h>

namespace {
int g_lockCalls;
int g_unlockCalls;
zVideo_SurfaceStatePartial *g_lockedSurface;
zVideo_SurfaceStatePartial *g_unlockedSurface;

int __fastcall CaptureLockSurface(zVideo_SurfaceStatePartial *surface) {
    ++g_lockCalls;
    g_lockedSurface = surface;
    surface->locked = 1;
    return 1;
}

int __fastcall CaptureUnlockSurface(zVideo_SurfaceStatePartial *surface) {
    ++g_unlockCalls;
    g_unlockedSurface = surface;
    surface->locked = 0;
    return 0x6a5;
}

struct FxSurfaceState {
    unsigned short *pixels;
    int width;
    int height;
    int pitchBytes;
    int pitchPixels;
    unsigned short *configPixels;
    int configWidth;
    int configHeight;
    int configPitchBytes;

    FxSurfaceState()
        : pixels(g_zVideo_FxSurfacePixels16),
          width(g_zVideo_FxSurfaceWidth),
          height(g_zVideo_FxSurfaceHeight),
          pitchBytes(g_zVideo_FxSurfacePitchBytes),
          pitchPixels(g_zVideo_FxSurfacePitchPixels16),
          configPixels(g_zVideo_FxPass3ConfigLocal.surfacePixels),
          configWidth(g_zVideo_FxPass3ConfigLocal.surfaceWidth),
          configHeight(g_zVideo_FxPass3ConfigLocal.surfaceHeight),
          configPitchBytes(g_zVideo_FxPass3ConfigLocal.surfacePitchBytes) {}

    ~FxSurfaceState() {
        g_zVideo_FxSurfacePixels16 = pixels;
        g_zVideo_FxSurfaceWidth = width;
        g_zVideo_FxSurfaceHeight = height;
        g_zVideo_FxSurfacePitchBytes = pitchBytes;
        g_zVideo_FxSurfacePitchPixels16 = pitchPixels;
        g_zVideo_FxPass3ConfigLocal.surfacePixels = configPixels;
        g_zVideo_FxPass3ConfigLocal.surfaceWidth = configWidth;
        g_zVideo_FxPass3ConfigLocal.surfaceHeight = configHeight;
        g_zVideo_FxPass3ConfigLocal.surfacePitchBytes = configPitchBytes;
    }
};

void ResetObservedSurfaceState() {
    g_lockCalls = 0;
    g_unlockCalls = 0;
    g_lockedSurface = 0;
    g_unlockedSurface = 0;
}

bool PipelineStateMatches(unsigned short *pixels, int width, int height, int pitch) {
    return zRndr::g_frameBuffer == pixels &&
           zRndr::g_pitchBytes == pitch &&
           g_zVideo_FxSurfacePixels16 == pixels &&
           g_zVideo_FxSurfaceWidth == width &&
           g_zVideo_FxSurfaceHeight == height &&
           g_zVideo_FxSurfacePitchBytes == pitch &&
           g_zVideo_FxSurfacePitchPixels16 == pitch / 2 &&
           g_zVideo_FxPass3ConfigLocal.surfacePixels == pixels &&
           g_zVideo_FxPass3ConfigLocal.surfaceWidth == width &&
           g_zVideo_FxPass3ConfigLocal.surfaceHeight == height &&
           g_zVideo_FxPass3ConfigLocal.surfacePitchBytes == pitch;
}
} // namespace

extern "C" int directdraw_enumerate_import_provider_smoke(void) {
    HMODULE const module = LoadLibraryA("ddraw.dll");
    if (module == 0) {
        return 1;
    }

    FARPROC const enumerateProc = GetProcAddress(module, "DirectDrawEnumerateA");
    FreeLibrary(module);
    return enumerateProc != 0 ? 0 : 2;
}

extern "C" int zvideo_fxpass3_slot_constructor_and_apply_smoke(void) {
    FxSurfaceState savedFxState;
    unsigned short *const savedScratch = g_zVideo_FxPass3_ScratchPixels16;
    const int savedOffsetX = g_zVideo_FxPass3_ScratchOffsetX;
    const int savedOffsetY = g_zVideo_FxPass3_ScratchOffsetY;
    const int savedClipMinX = g_zVideo_FxPass3_ClipMinX;
    const int savedClipMinY = g_zVideo_FxPass3_ClipMinY;
    const int savedClipMaxX = g_zVideo_FxPass3_ClipMaxX;
    const int savedClipMaxY = g_zVideo_FxPass3_ClipMaxY;

    zVideoFxPass3Slot slot;
    if (slot.clipRectOrNull != 0 || slot.x != 0 || slot.y != 0) {
        return 1;
    }

    unsigned short pixels[49];
    unsigned short original[49];
    unsigned short scratch[49];
    for (int index = 0; index < 49; ++index) {
        pixels[index] = static_cast<unsigned short>(0x2000 + index);
        original[index] = pixels[index];
        scratch[index] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = 7;
    g_zVideo_FxSurfaceHeight = 7;
    g_zVideo_FxSurfacePitchBytes = 14;
    g_zVideo_FxSurfacePitchPixels16 = 7;

    HudUiRect clip = {1, 1, 5, 5};
    slot.SetRectAndPayload(3, 3, 1, 2, 2, 1.0f, 0.0f);
    slot.clipRectOrNull = &clip;
    slot.ApplyPass3();

    const bool ok =
        pixels[2 + 2 * 7] == original[1 + 1 * 7] &&
        pixels[0] == original[0] &&
        g_zVideo_FxPass3_ScratchOffsetX == 3 &&
        g_zVideo_FxPass3_ScratchOffsetY == 3 &&
        g_zVideo_FxPass3_ClipMinX == 1 &&
        g_zVideo_FxPass3_ClipMinY == 1 &&
        g_zVideo_FxPass3_ClipMaxX == 5 &&
        g_zVideo_FxPass3_ClipMaxY == 5;

    g_zVideo_FxPass3_ScratchPixels16 = savedScratch;
    g_zVideo_FxPass3_ScratchOffsetX = savedOffsetX;
    g_zVideo_FxPass3_ScratchOffsetY = savedOffsetY;
    g_zVideo_FxPass3_ClipMinX = savedClipMinX;
    g_zVideo_FxPass3_ClipMinY = savedClipMinY;
    g_zVideo_FxPass3_ClipMaxX = savedClipMaxX;
    g_zVideo_FxPass3_ClipMaxY = savedClipMaxY;
    return ok ? 0 : 2;
}

extern "C" int zvideo_fxpass3_root_overlay_smoke(void) {
    const int savedRendererPath = g_zVideo_ActiveRendererPath;
    const int savedOverlayEnabled = zRndr::g_overlayBlendEnabled;
    const int savedLeft = zRndr::g_overlayBlendRectLeft;
    const int savedTop = zRndr::g_overlayBlendRectTop;
    const int savedRight = zRndr::g_overlayBlendRectRight;
    const int savedBottom = zRndr::g_overlayBlendRectBottom;
    const unsigned int savedColor = zRndr::g_overlayBlendPackedColor16;
    const double savedAlpha = zRndr::g_overlayBlendAlpha;
    FxSurfaceState savedFxState;

    zVideoFxPass3RootElement root;
    HudUiRect rect = {2, 3, 8, 9};
    root.clipRectOrNull = &rect;
    root.packedColor16 = 0xabcd;
    root.alpha = 0.375;

    g_zVideo_ActiveRendererPath = 0;
    zRndr::g_overlayBlendEnabled = 0;
    root.ApplyPass3();
    const bool explicitRectOk =
        zRndr::g_overlayBlendEnabled == 1 &&
        zRndr::g_overlayBlendRectLeft == 2 &&
        zRndr::g_overlayBlendRectTop == 3 &&
        zRndr::g_overlayBlendRectRight == 8 &&
        zRndr::g_overlayBlendRectBottom == 9 &&
        zRndr::g_overlayBlendPackedColor16 == 0xabcd &&
        zRndr::g_overlayBlendAlpha == 0.375;

    zRndr::g_overlayBlendEnabled = 0;
    root.clipRectOrNull = 0;
    root.packedColor16 = 0xf81f;
    root.alpha = 0.25;
    g_zVideo_FxSurfaceWidth = 13;
    g_zVideo_FxSurfaceHeight = 17;
    root.ApplyPass3();
    const bool fallbackRectOk =
        zRndr::g_overlayBlendEnabled == 1 &&
        zRndr::g_overlayBlendRectLeft == 0 &&
        zRndr::g_overlayBlendRectTop == 0 &&
        zRndr::g_overlayBlendRectRight == 12 &&
        zRndr::g_overlayBlendRectBottom == 17 &&
        zRndr::g_overlayBlendPackedColor16 == 0xf81f &&
        zRndr::g_overlayBlendAlpha == 0.25;

    g_zVideo_ActiveRendererPath = savedRendererPath;
    zRndr::g_overlayBlendEnabled = savedOverlayEnabled;
    zRndr::g_overlayBlendRectLeft = savedLeft;
    zRndr::g_overlayBlendRectTop = savedTop;
    zRndr::g_overlayBlendRectRight = savedRight;
    zRndr::g_overlayBlendRectBottom = savedBottom;
    zRndr::g_overlayBlendPackedColor16 = savedColor;
    zRndr::g_overlayBlendAlpha = savedAlpha;
    return explicitRectOk && fallbackRectOk ? 0 : 1;
}

extern "C" int zvideo_run_postprocess_on_sw_buffer_smoke(void) {
    const zVideo_SurfaceStateProc savedLock = g_zVideo_pfnLockSurfaceState;
    const zVideo_SurfaceStatePartial savedSurface = g_zVideo_SwSurfaceState;
    void *const savedFrameBuffer = zRndr::g_frameBuffer;
    const int savedPitch = zRndr::g_pitchBytes;
    const int savedBytesPerPixel = zRndr::g_bytesPerPixel;
    FxSurfaceState savedFxState;

    unsigned short pixels[16] = {};
    g_zVideo_SwSurfaceState = zVideo_SurfaceStatePartial{};
    g_zVideo_SwSurfaceState.pixels = pixels;
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_pfnLockSurfaceState = CaptureLockSurface;
    ResetObservedSurfaceState();
    zRndr::g_frameBuffer = 0;
    zRndr::g_pitchBytes = 0;
    zRndr::g_bytesPerPixel = 7;

    zVideo::RunPostprocessOnSwBuffer();
    const bool ok =
        g_lockCalls == 1 &&
        g_lockedSurface == &g_zVideo_SwSurfaceState &&
        g_zVideo_SwSurfaceState.locked == 1 &&
        zRndr::g_bytesPerPixel == 7 &&
        PipelineStateMatches(pixels, 4, 3, 8);

    g_zVideo_pfnLockSurfaceState = savedLock;
    g_zVideo_SwSurfaceState = savedSurface;
    zRndr::g_frameBuffer = savedFrameBuffer;
    zRndr::g_pitchBytes = savedPitch;
    zRndr::g_bytesPerPixel = savedBytesPerPixel;
    return ok ? 0 : 1;
}

extern "C" int zvideo_run_postprocess_on_primary_buffer_smoke(void) {
    const zVideo_SurfaceStateProc savedLock = g_zVideo_pfnLockSurfaceState;
    const zVideo_SurfaceStateProc savedUnlock = g_zVideo_pfnUnlockSurfaceState;
    const zVideo_SurfaceStatePartial savedSurface = g_zVideo_PrimarySurfaceState;
    const int savedRendererType = g_zVideo_RendererType;
    const int savedHalfRes = g_zVideo_UseHalfResBackbuffer;
    void *const savedFrameBuffer = zRndr::g_frameBuffer;
    const int savedPitch = zRndr::g_pitchBytes;
    const int savedBytesPerPixel = zRndr::g_bytesPerPixel;
    FxSurfaceState savedFxState;

    unsigned short pixels[16] = {};
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial{};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 4;
    g_zVideo_PrimarySurfaceState.height = 3;
    g_zVideo_PrimarySurfaceState.pitch = 8;
    g_zVideo_pfnLockSurfaceState = CaptureLockSurface;
    g_zVideo_pfnUnlockSurfaceState = CaptureUnlockSurface;

    ResetObservedSurfaceState();
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    const int softwareResult = zVideo::RunPostprocessOnPrimaryBuffer();
    const bool softwareOk =
        softwareResult == 0 &&
        g_lockCalls == 0 &&
        g_unlockCalls == 0 &&
        PipelineStateMatches(pixels, 4, 3, 8);

    ResetObservedSurfaceState();
    g_zVideo_RendererType = 1;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_PrimarySurfaceState.locked = 0;
    const int hardwareResult = zVideo::RunPostprocessOnPrimaryBuffer();
    const bool hardwareOk =
        hardwareResult == 0 &&
        g_lockCalls == 1 &&
        g_lockedSurface == &g_zVideo_PrimarySurfaceState &&
        g_unlockCalls == 0 &&
        PipelineStateMatches(pixels, 4, 3, 8);

    ResetObservedSurfaceState();
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 1;
    g_zVideo_PrimarySurfaceState.locked = 0;
    const int halfResResult = zVideo::RunPostprocessOnPrimaryBuffer();
    const bool halfResOk =
        halfResResult == 0 &&
        g_lockCalls == 1 &&
        g_unlockCalls == 1 &&
        g_unlockedSurface == &g_zVideo_PrimarySurfaceState &&
        PipelineStateMatches(pixels, 4, 3, 8);

    g_zVideo_pfnLockSurfaceState = savedLock;
    g_zVideo_pfnUnlockSurfaceState = savedUnlock;
    g_zVideo_PrimarySurfaceState = savedSurface;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_UseHalfResBackbuffer = savedHalfRes;
    zRndr::g_frameBuffer = savedFrameBuffer;
    zRndr::g_pitchBytes = savedPitch;
    zRndr::g_bytesPerPixel = savedBytesPerPixel;
    return softwareOk && hardwareOk && halfResOk ? 0 : 1;
}
