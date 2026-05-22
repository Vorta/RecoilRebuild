#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ddraw.h>

extern "C" int zvid_pack_color_rgb_smoke(void) {
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;

    const std::uint32_t packed565 = zVid_PackColorRGB(0xff, 0x80, 0x20);
    zVideo_ColorRgbFloat color{255.0f, 128.0f, 32.0f};
    const std::uint16_t packedFloats = zVid_PackColorRgbFloats(&color);
    zVideo_SetClearColorPacked16(0xabcd);
    if (packed565 != 0xfc04 || packedFloats != 0xfc04 || g_zVideo_ClearColorPacked16 != 0xabcd) {
        return 1;
    }

    zVideo_ColorRgbFloat fogTarget{0.25f, 1.0f, 0.5f};
    g_zVideo_RendererType = 1;
    g_zVideo_D3DColorNormalizeChannelIndex = -1;
    zVideo_SetPendingFogTargetColorFromRgb01(&fogTarget);
    if (g_zVideo_D3DColorAttrBiasR != 63.75f || g_zVideo_D3DColorAttrBiasG != 255.0f ||
        g_zVideo_D3DColorAttrBiasB != 127.5f || g_zVideo_D3DColorNormalizeChannelIndex != 1) {
        return 2;
    }

    fogTarget = {0.5f, 0.25f, 1.0f};
    g_zVideo_RendererType = 0;
    g_zVideo_D3DColorNormalizeChannelIndex = 7;
    zVideo_SetPendingFogTargetColorFromRgb01(&fogTarget);
    return g_zVideo_D3DColorAttrBiasR == 127.5f && g_zVideo_D3DColorAttrBiasG == 63.75f &&
                   g_zVideo_D3DColorAttrBiasB == 255.0f &&
                   g_zVideo_D3DColorNormalizeChannelIndex == 7
               ? 0
               : 3;
}

extern "C" int zvideo_pixel_pack_setup_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    if (g_zVideo_PixelPack_RBits != 5 || g_zVideo_PixelPack_GBits != 6 ||
        g_zVideo_PixelPack_BBits != 5 || g_zVideo_PixelPack_RShift != 8 ||
        g_zVideo_PixelPack_GShift != 3 || g_zVideo_PixelPack_BShiftTo8 != 3 ||
        g_zVideo_PixelPack_RMaskShifted != 0xf8 || g_zVideo_PixelPack_GMaskShifted != 0xfc ||
        g_zVideo_PixelPack_BMaskShifted != 0xf8) {
        return 1;
    }

    zVideo::TexturePixelPack_SetupFromMasks(4, 4, 4, 4, 0xf000, 0x0f00, 0x00f0, 0x000f);
    return g_zVideo_TexturePixelPack_RBits == 4 && g_zVideo_TexturePixelPack_GBits == 4 &&
                   g_zVideo_TexturePixelPack_BBits == 4 && g_zVideo_TexturePixelPack_ABits == 4 &&
                   g_zVideo_TexturePixelPack_RGBBitsTotal == 12 &&
                   g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 == 4 &&
                   g_zVideo_TexturePixelPack_GBBitsTotalMinus8 == 0 &&
                   g_zVideo_TexturePixelPack_BShiftTo8 == 4 &&
                   g_zVideo_TexturePixelPack_RMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_GMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_BMaskShifted == 0xf0 &&
                   g_zVideo_TexturePixelPack_NonRgbMaskShifted == ~0xf0
               ? 0
               : 2;
}

extern "C" int zvideo_pixel_pack_getters_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 5, 5, 0x7c00, 0x03e0, 0x001f);

    std::int32_t rBits = 0;
    std::int32_t gBits = 0;
    std::int32_t bBits = 0;
    std::uint32_t rMask = 0;
    std::uint32_t gMask = 0;
    std::uint32_t bMask = 0;
    std::int32_t packedBase = 0;
    std::int32_t sumMinus8 = 0;
    std::int32_t bShiftTo8 = 0;

    zVideo::PixelPack_GetRgbBits(&rBits, &gBits, &bBits);
    zVideo::PixelPack_GetRgbMasks(&rMask, &gMask, &bMask);
    zVideo::PixelPack_GetPackingParams(&packedBase, &sumMinus8, &bShiftTo8);

    return rBits == 5 && gBits == 5 && bBits == 5 && rMask == 0x7c00 && gMask == 0x03e0 &&
                   bMask == 0x001f && packedBase == 7 && sumMinus8 == 2 && bShiftTo8 == 3
               ? 0
               : 1;
}

extern "C" int zvideo_set_renderer_type_smoke(void) {
    g_zVideo_RendererType = 7;
    g_zVideo_ActiveRendererPath = 3;

    const std::int32_t previous = zVideo::SetRendererTypeAndActivePath(2);

    return previous == 7 && g_zVideo_RendererType == 2 && g_zVideo_ActiveRendererPath == 2 ? 0 : 1;
}

extern "C" int zvideo_pending_dither_enable_smoke(void) {
    g_zVideo_PendingDitherEnable = -1;
    zVideo_dd3d::SetPendingDitherEnable(0);
    if (g_zVideo_PendingDitherEnable != 0) {
        return 1;
    }

    zVideo_dd3d::SetPendingDitherEnable(1);
    return g_zVideo_PendingDitherEnable == 1 ? 0 : 2;
}

extern "C" int zvideo_surface_accessors_smoke(void) {
    int swPixels = 0x1234;
    int primaryPixels = 0x5678;
    g_zVideo_SwSurfaceState.width = 320;
    g_zVideo_SwSurfaceState.height = 200;
    g_zVideo_SwSurfaceState.pitch = 640;
    g_zVideo_SwSurfaceState.locked = 1;
    g_zVideo_SwSurfaceState.pixels = &swPixels;
    g_zVideo_PrimarySurfaceState.width = 800;
    g_zVideo_PrimarySurfaceState.height = 600;
    g_zVideo_PrimarySurfaceState.pitch = 1600;
    g_zVideo_PrimarySurfaceState.pixels = &primaryPixels;

    return zVideo::GetSwSurfacePixels() == &swPixels && zVideo::GetSwSurfaceWidth() == 320 &&
                   zVideo::GetSwSurfaceHeight() == 200 && zVideo::GetSwSurfacePitch() == 640 &&
                   zVideo::GetSwSurfaceLockedFlag() == 1 &&
                   zVideo::GetPrimarySurfacePixels() == &primaryPixels &&
                   zVideo::GetPrimarySurfaceWidth() == 800 &&
                   zVideo::GetPrimarySurfaceHeight() == 600 &&
                   zVideo::GetPrimarySurfacePitch() == 1600
               ? 0
               : 1;
}

extern "C" int zvideo_primary_surface_rect_scratch_smoke(void) {
    g_zVideo_PrimarySurfaceRectScratch = {11, 22, 33, 44};
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    zVidRect32 *const scratch = zVideo::GetPrimarySurfaceRectScratch();
    return scratch == &g_zVideo_PrimarySurfaceRectScratch && scratch->left == 11 &&
                   scratch->top == 22 && scratch->right == 640 && scratch->bottom == 480
               ? 0
               : 1;
}

extern "C" int zvideo_frame_scratch_buffers_smoke(void) {
    std::free(g_zVid_NoiseByteTable);
    std::free(g_zVideo_FxPass3_ScratchPixels16);
    g_zVid_NoiseByteTable = nullptr;
    g_zVideo_FxPass3_ScratchPixels16 = nullptr;

    g_zVideo_PrimarySurfaceState.width = 4;
    g_zVideo_PrimarySurfaceState.height = 3;
    std::uint16_t staleFxPixel = 0;
    g_zVideo_FxSurfacePixels16 = &staleFxPixel;
    g_zVideo_FxSurfaceWidth = 1;
    g_zVideo_FxSurfaceHeight = 1;
    g_zVideo_FxSurfacePitchBytes = 2;
    g_zVideo_FxSurfacePitchPixels16 = 1;

    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_bytesPerPixel = 1;
    zRndr::g_defaultGraphicsFlags = 8;
    zRndr::g_graphicsFlags = &zRndr::g_defaultGraphicsFlags;
    zRndr::g_pfnOverlayBlendRow = nullptr;

    zVid::InitFrameScratchBuffers();

    const bool result =
        g_zVid_NoiseByteTableSize == 100 && g_zVid_NoiseByteTable != nullptr &&
        g_zVideo_FxPass3_ScratchPixels16 != nullptr && g_zVideo_FxSurfacePixels16 == nullptr &&
        g_zVideo_FxSurfaceWidth == 0 && g_zVideo_FxSurfaceHeight == 0 &&
        g_zVideo_FxSurfacePitchBytes == 0 && g_zVideo_FxSurfacePitchPixels16 == 0 &&
        zRndr::g_pfnOverlayBlendRow == zRndr::OverlayBlendRow555_Scalar &&
        zRndr::g_pixelPackGreenBits == 6 && zRndr::g_perspectiveAdaptiveMinSpan == 0x10 &&
        zRndr::g_perspectiveAdaptiveMaxSpan == 0x40 && (zRndr::g_defaultGraphicsFlags & 4) == 0;

    std::free(g_zVid_NoiseByteTable);
    std::free(g_zVideo_FxPass3_ScratchPixels16);
    g_zVid_NoiseByteTable = nullptr;
    g_zVideo_FxPass3_ScratchPixels16 = nullptr;
    return result ? 0 : 1;
}

namespace {
int g_clearSwCalls;
int g_clearPrimaryCalls;
int g_setVideoModeCalls;
zVidRect32 *g_lastClearSwSurfaceRect;
zVidRect32 *g_lastClearSwZRect;
zVidRect32 *g_lastClearPrimaryRect;
zVideo_SurfaceStatePartial *g_lastClearPrimaryState;

void RECOIL_FASTCALL ClearSwFake(zVidRect32 *surfaceRect, zVidRect32 *zRect) {
    ++g_clearSwCalls;
    g_lastClearSwSurfaceRect = surfaceRect;
    g_lastClearSwZRect = zRect;
}

void RECOIL_FASTCALL ClearStateFake(zVidRect32 *rect, zVideo_SurfaceStatePartial *surfaceState) {
    ++g_clearPrimaryCalls;
    g_lastClearPrimaryRect = rect;
    g_lastClearPrimaryState = surfaceState;
}

std::int32_t RECOIL_FASTCALL SetVideoModeFake(std::int32_t) {
    ++g_setVideoModeCalls;
    return 0x1234;
}
} // namespace

extern "C" int zvideo_clear_dispatch_and_exchange_smoke(void) {
    zVidRect32 surfaceRect{1, 2, 3, 4};
    zVidRect32 zRect{5, 6, 7, 8};
    g_clearSwCalls = 0;
    g_clearPrimaryCalls = 0;
    g_zVideo_pfnClearSwSurfaceAndZBuffer = ClearSwFake;
    g_zVideo_pfnClearStateSurfaceAndZBuffer = ClearStateFake;
    g_zVideo_ClearScreenBufferEnabled = 3;

    zVideo::CallClearSwSurfaceAndZBuffer(&surfaceRect, &zRect);
    zVideo::CallClearPrimarySurfaceAndZBuffer(&surfaceRect);
    if (g_clearSwCalls != 1 || g_lastClearSwSurfaceRect != &surfaceRect ||
        g_lastClearSwZRect != &zRect || g_clearPrimaryCalls != 1 ||
        g_lastClearPrimaryRect != &surfaceRect ||
        g_lastClearPrimaryState != &g_zVideo_PrimarySurfaceState) {
        return 1;
    }

    const std::int32_t previous = zVideo::ExchangeClearScreenBufferEnabled(1);
    return previous == 3 && zVideo::GetClearScreenBufferEnabled() == 1 ? 0 : 2;
}

extern "C" int zvideo_mode_geometry_and_set_video_mode_smoke(void) {
    gVideo_resolutionMenuValid = 1;
    g_zVideo_DisplayModeBpp = 0;
    zVideo::Init_SetSurfaceGeometryFromModeIndex(3);
    if (g_zVideo_UseHalfResBackbuffer != 1 || g_zVideo_DisplayModeSurfaceState.width != 0x280 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x1e0 ||
        g_zVideo_PrimarySurfaceState.width != 0x280 ||
        g_zVideo_PrimarySurfaceState.height != 0x1e0 || g_zVideo_SwSurfaceState.width != 0x140 ||
        g_zVideo_SwSurfaceState.height != 0x0f0 || zVideo::GetDisplayModeBpp() != 0x10) {
        return 1;
    }

    zVideo::Init_SetSurfaceGeometryFromModeIndex(7);
    if (g_zVideo_UseHalfResBackbuffer != 0 || g_zVideo_DisplayModeSurfaceState.width != 0x400 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x300 ||
        g_zVideo_SwSurfaceState.width != 0x400 || g_zVideo_SwSurfaceState.height != 0x300) {
        return 2;
    }

    zVideo::Init_SetSurfaceGeometryFromModeIndex(8);
    if (gVideo_resolutionMenuValid != 0) {
        return 3;
    }

    g_setVideoModeCalls = 0;
    g_zVideo_pfnSetVideoMode = SetVideoModeFake;
    g_zVideo_IsInitialized = 0;
    if (zVideo::SetVideoMode(5) != 0x5a560000 || g_setVideoModeCalls != 0) {
        return 4;
    }

    g_zVideo_IsInitialized = 1;
    if (zVideo::SetVideoMode(5) != 0x1234 || g_setVideoModeCalls != 1 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x1e0) {
        return 5;
    }

    if (zVideo::Init_ApplyModeIndex(4) != 0x1234 || g_setVideoModeCalls != 2 ||
        g_zVideo_DisplayModeSurfaceState.height != 0x190) {
        return 6;
    }

    g_zVideo_pfnSetVideoMode = nullptr;
    return 0;
}

extern "C" int zvideo_init_video_system_reentry_guard_smoke(void) {
    HWND oldHwnd = g_zVideo_hWnd;
    g_zVideo_IsInitialized = 1;
    g_zVideo_FrameTick = 77;

    const std::int32_t result = zVideo::InitVideoSystem(reinterpret_cast<HWND>(0x1234), 1, 1, 5);
    const bool ok = result == 0x5a560001 && g_zVideo_FrameTick == 77 && g_zVideo_hWnd == oldHwnd;

    g_zVideo_IsInitialized = 0;
    return ok ? 0 : 1;
}

extern "C" int zvideo_dispatch_wrappers_smoke(void) {
    zVideo::BindRendererDispatch(0, 1);
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.locked = 1;

    zVidRect32 rect{0, 0, 1, 1};
    zVideo_dd3d::CallClearZBufferRect(&rect);
    if (zVideo::Dispatch_LockDisplayModeSurfaceState() != 0) {
        return 1;
    }

    g_zVideo_DisplayModeSurfaceState.locked = 0;
    return zVideo::Dispatch_UnlockDisplayModeSurfaceState() == 0 ? 0 : 2;
}

extern "C" int zvideo_bind_renderer_dispatch_smoke(void) {
    zVidHwApiDeviceRecordPartial selectedDevice{};
    selectedDevice.m_deviceFeatureFlags = 0x1234;
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;

    zVideo::BindRendererDispatch(1, 2);
    if (g_zVideo_RendererType != 1 || g_zVideo_ActiveRendererPath != 1 ||
        g_zVideo_FullscreenOption != 2 || g_zVideo_pfnOpenVideoMode != zVideo_dd::OpenVideoMode ||
        g_zVideo_pfnSetVideoMode != zVideo_dd::SetVideoMode ||
        g_zVideo_pfnCreateTextureRecord != zVideo_dd3d::CreateTextureRecord ||
        g_zVideo_pfnAdjustSurfaces == nullptr ||
        reinterpret_cast<std::uintptr_t>(g_zVideo_pfnBltSwToPrimaryRectDirect) != 0x004a7d90 ||
        selectedDevice.m_deviceFeatureFlags != 0) {
        return 1;
    }

    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    zVideo::BindRendererDispatch(0, 1);
    return g_zVideo_RendererType == 0 && g_zVideo_ActiveRendererPath == 0 &&
                   g_zVideo_FullscreenOption == 1 && g_zVideo_pfnAdjustSurfaces != nullptr
               ? 0
               : 2;
}

namespace {
int g_adjustSurfaceCalls;
int g_bltPrimaryToSwDirectCalls;
int g_textureMemoryQueryCalls;
int g_deviceMemoryQueryCalls;
std::int32_t g_lastAdjustWaitForPresent;
std::int32_t g_lastAdjustBlitPrimaryToSwFirst;
std::int32_t g_lastTextureMemoryQueryFlags;
std::int32_t g_lastDeviceMemoryQueryFlags;

void RECOIL_FASTCALL BltPrimaryToSwDirectFake(zVidRect32 *srcRect, zVidRect32 *dstRect) {
    if (srcRect == nullptr && dstRect == nullptr) {
        ++g_bltPrimaryToSwDirectCalls;
    }
}

std::int32_t RECOIL_FASTCALL AdjustSurfacesFake(zVidRect32 *, zVidRect32 *,
                                                std::int32_t waitForPresent,
                                                std::int32_t blitPrimaryToSwFirst) {
    ++g_adjustSurfaceCalls;
    g_lastAdjustWaitForPresent = waitForPresent;
    g_lastAdjustBlitPrimaryToSwFirst = blitPrimaryToSwFirst;
    return 0x123;
}

std::int32_t RECOIL_FASTCALL TextureMemoryQueryFake(std::int32_t flags, std::int32_t *totalBytes,
                                                    std::int32_t *freeBytes) {
    ++g_textureMemoryQueryCalls;
    g_lastTextureMemoryQueryFlags = flags;
    *totalBytes = 0x1000;
    *freeBytes = 0x200;
    return 1;
}

std::int32_t RECOIL_FASTCALL DeviceMemoryQueryFake(std::int32_t flags, std::int32_t *totalBytes,
                                                   std::int32_t *freeBytes) {
    ++g_deviceMemoryQueryCalls;
    g_lastDeviceMemoryQueryFlags = flags;
    *totalBytes = 0x2000;
    *freeBytes = 0x300;
    return 1;
}
} // namespace

extern "C" int zvideo_set_half_res_adjust_mode_smoke(void) {
    g_bltPrimaryToSwDirectCalls = 0;
    g_zVideo_HalfResAdjustMode = 3;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_RendererType = 1;
    g_zVideo_pfnBltPrimaryToSwRectDirect = BltPrimaryToSwDirectFake;

    if (zVideo::SetHalfResAdjustMode(3) != 3 || g_zVideo_HalfResAdjustMode != 3 ||
        g_bltPrimaryToSwDirectCalls != 0) {
        return 1;
    }

    g_zVideo_UseHalfResBackbuffer = 1;
    if (zVideo::SetHalfResAdjustMode(1) != 0 || g_zVideo_HalfResAdjustMode != 3) {
        return 2;
    }

    g_zVideo_UseHalfResBackbuffer = 0;
    if (zVideo::SetHalfResAdjustMode(1) != 3 || g_zVideo_HalfResAdjustMode != 1) {
        return 3;
    }

    g_zVideo_RendererType = 0;
    return zVideo::SetHalfResAdjustMode(0) == 1 && g_zVideo_HalfResAdjustMode == 0 &&
                   g_bltPrimaryToSwDirectCalls == 1
               ? 0
               : 4;
}

extern "C" int zvideo_adjust_surfaces_if_enabled_smoke(void) {
    zVidRect32 rect{0, 0, 1, 1};
    g_adjustSurfaceCalls = 0;
    g_lastAdjustWaitForPresent = 0;
    g_lastAdjustBlitPrimaryToSwFirst = 0;
    g_zVideo_FrameTick = 10;
    g_zVideo_AdjustSurfacesDisableGate = 0;
    g_zVideo_pfnAdjustSurfaces = AdjustSurfacesFake;

    if (zVideo::AdjustSurfacesIfEnabled(&rect, &rect, 7, 9) != 0x123 || g_adjustSurfaceCalls != 1 ||
        g_lastAdjustWaitForPresent != 7 || g_lastAdjustBlitPrimaryToSwFirst != 9 ||
        g_zVideo_FrameTick != 11) {
        return 1;
    }

    g_zVideo_AdjustSurfacesDisableGate = 2;
    return zVideo::AdjustSurfacesIfEnabled(&rect, &rect, 1, 1) == 2 && g_adjustSurfaceCalls == 1 &&
                   g_zVideo_FrameTick == 11
               ? 0
               : 2;
}

extern "C" int zvideo_dd_report_error_smoke(void) {
    g_textureMemoryQueryCalls = 0;
    g_deviceMemoryQueryCalls = 0;
    g_lastTextureMemoryQueryFlags = 0;
    g_lastDeviceMemoryQueryFlags = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = TextureMemoryQueryFake;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = DeviceMemoryQueryFake;

    if (zVideo_dd::ReportError(DD_OK, "video.cpp", 10) != 0 || g_textureMemoryQueryCalls != 0 ||
        g_deviceMemoryQueryCalls != 0) {
        return 1;
    }

    if (zVideo_dd::ReportError(DDERR_INVALIDPARAMS, "video.cpp", 20) != -1 ||
        g_textureMemoryQueryCalls != 0 || g_deviceMemoryQueryCalls != 0) {
        return 2;
    }

    if (zVideo_dd::ReportError(DDERR_OUTOFVIDEOMEMORY, "video.cpp", 30) != -1 ||
        g_textureMemoryQueryCalls != 1 || g_deviceMemoryQueryCalls != 1 ||
        g_lastTextureMemoryQueryFlags != -1 || g_lastDeviceMemoryQueryFlags != -1) {
        return 3;
    }

    g_zVideo_pfnQueryTextureMemoryBytes = nullptr;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = nullptr;
    return 0;
}

extern "C" int zvid_hw_api_accessors_smoke(void) {
    std::strncpy(g_zVideo_HwApiDeviceTable[2].m_driverName, "driver-two",
                 sizeof(g_zVideo_HwApiDeviceTable[2].m_driverName));
    std::strncpy(g_zVideo_HwApiDeviceTable[2].m_driverDescription, "description-two",
                 sizeof(g_zVideo_HwApiDeviceTable[2].m_driverDescription));

    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    if (std::strcmp(zVid::GetSelectedHwApiDescriptionOrDefault(), "Default") != 0) {
        return 1;
    }

    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[2];
    return zVid::GetHwApiDriverName(2) == g_zVideo_HwApiDeviceTable[2].m_driverName &&
                   zVid::GetHwApiDescription(2) ==
                       g_zVideo_HwApiDeviceTable[2].m_driverDescription &&
                   zVid::GetSelectedHwApiDescriptionOrDefault() ==
                       g_zVideo_HwApiDeviceTable[2].m_driverDescription
               ? 0
               : 2;
}

extern "C" int zvid_cached_renderer_and_texture_counts_smoke(void) {
    g_zVid_AcceptedHardwareRendererCount = 3;
    g_zVid_TexturePackLoadState = 1;
    if (zVid::GetAcceptedHardwareRendererCount_Cached() != 3 ||
        zVid::HasAcceptedHardwareRenderer() != 1 || zVid::GetTexturePackLoadState() != 1) {
        return 1;
    }

    g_zVid_AcceptedHardwareRendererCount = 0;
    g_zVid_TexturePackLoadState = 0;
    return zVid::GetAcceptedHardwareRendererCount_Cached() == 0 &&
                   zVid::HasAcceptedHardwareRenderer() == 0 && zVid::GetTexturePackLoadState() == 0
               ? 0
               : 2;
}

extern "C" int zvid_option_accessors_smoke(void) {
    std::int32_t mode = 6;
    std::int32_t acceleration = 1;
    std::int32_t hwApi = 2;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;

    if (zVid::GetVideoModeIndexFromOptions() != 6 || zVid::GetAccelerationOption() != 1 ||
        zVid::GetHwApiOption() != 2) {
        return 1;
    }

    mode = 3;
    zVid::SetAccelerationOption(0);
    zVid::SetHwApiOption(1);
    g_zVideo_NumAcceptedDirectDrawDevices = 3;
    return zVid::GetVideoModeIndexFromOptions() == 3 && zVid::GetAccelerationOption() == 0 &&
                   zVid::GetAcceptedDirectDrawDeviceCount() == 3 && g_zOpt_HwMode == 0 && hwApi == 1
               ? 0
               : 2;
}

extern "C" int zvid_set_video_mode_index_smoke(void) {
    std::int32_t mode = -1;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    zVid::SetVideoModeIndex(2);
    if (mode != 2 || replicate != 1 || render.width != 0x140 || render.height != 0xc8 ||
        window.width != 0x280 || window.height != 0x190 || display.width != 0x280 ||
        display.height != 0x190 || display.bitsPerPixel != 0x10) {
        return 1;
    }

    zVid::SetVideoModeIndex(7);
    if (mode != 7 || replicate != 0 || render.width != 0x400 || render.height != 0x300 ||
        window.width != 0x400 || window.height != 0x300 || display.maxXInclusive != 0x3ff ||
        display.maxYInclusive != 0x2ff) {
        return 2;
    }

    zVid::SetVideoModeIndex(8);
    return mode == 0 ? 0 : 3;
}

extern "C" int zvideo_buff_clip_coord_to_range_smoke(void) {
    std::int32_t coord = 5;
    if (zVideo_buff::ClipCoordToRange(&coord, 2, 8) != 0 || coord != 5) {
        return 1;
    }

    coord = -3;
    if (zVideo_buff::ClipCoordToRange(&coord, 2, 8) != -5 || coord != 2) {
        return 2;
    }

    coord = 12;
    return zVideo_buff::ClipCoordToRange(&coord, 2, 8) == 4 && coord == 8 ? 0 : 3;
}

extern "C" int zvideo_buff_copy_surface_rect_to_image_smoke(void) {
    std::uint16_t pixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    std::uint16_t captured[12] = {};
    zVidImagePartial image{};
    image.pixels = captured;

    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = pixels;

    zVidRect32 rect{-1, 1, 3, 4};
    zVidImagePartial *result = zVideo_buff::CopySurfaceRectToImage(0, &rect, &image);
    if (result != &image) {
        return 1;
    }

    if (rect.left != 0 || rect.top != 1 || rect.right != 3 || rect.bottom != 3) {
        return 2;
    }

    return captured[0] == 0 && captured[1] == 5 && captured[2] == 6 && captured[3] == 7 &&
                   captured[4] == 0 && captured[5] == 9 && captured[6] == 10 &&
                   captured[7] == 11
               ? 0
               : 3;
}

extern "C" int zvideo_surface_state_lock_skip_smoke(void) {
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.surf = reinterpret_cast<IDirectDrawSurface3 *>(0x1234);
    g_zVideo_DisplayModeSurfaceState.locked = 1;
    g_zVideo_FullscreenOption = 0;

    if (zVideo_dd::LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ||
        g_zVideo_DisplayModeSurfaceState.locked != 1) {
        return 1;
    }

    if (zVideo_dd::UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ||
        g_zVideo_DisplayModeSurfaceState.locked != 1) {
        return 2;
    }

    zVideo_SurfaceStatePartial alreadyLocked{};
    alreadyLocked.locked = 1;
    zVideo_SurfaceStatePartial unlocked{};
    g_zVideo_FullscreenOption = 1;
    return zVideo_dd::LockSurfaceState(&alreadyLocked) == 0 &&
                   zVideo_dd::UnlockSurfaceState(&unlocked) == 0
               ? 0
               : 3;
}

extern "C" int zvideo_capture_surface_to_image_smoke(void) {
    zVideo::BindRendererDispatch(0, 0);
    std::uint16_t pixels[6] = {1, 2, 0xaaaa, 3, 4, 0xbbbb};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.width = 2;
    g_zVideo_DisplayModeSurfaceState.height = 2;
    g_zVideo_DisplayModeSurfaceState.pitch = 6;
    g_zVideo_DisplayModeSurfaceState.pixels = pixels;

    zVidImagePartial *image = zVideo_buff_CaptureSurfaceToImage(2);
    if (image == nullptr) {
        return 1;
    }

    auto *captured = static_cast<std::uint16_t *>(image->pixels);
    const bool ok = image->width == 2 && image->height == 2 && image->pixelCount == 4 &&
                    (image->formatFlagsPacked & 0x20u) != 0 && captured != nullptr &&
                    captured[0] == 1 && captured[1] == 2 && captured[2] == 3 && captured[3] == 4;

    zVid_Image::ReleaseIfNotDefault(image);
    return ok ? 0 : 2;
}

extern "C" int zvideo_image_lazy_create_backing_surface_guards_smoke(void) {
    zVidImagePartial image{};
    image.width = 8;
    image.height = 4;
    image.pixels = reinterpret_cast<void *>(0x12345678);
    image.alphaMap = reinterpret_cast<char *>(0x1234);

    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != nullptr) {
        return 1;
    }

    image.alphaMap = nullptr;
    image.pixels = nullptr;
    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != nullptr) {
        return 2;
    }

    image.pixels = reinterpret_cast<void *>(0x12345678);
    image.width = 0;
    if (zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) != nullptr) {
        return 3;
    }

    image.width = 8;
    image.height = 0;
    return zVideo_dd::Image_LazyCreateBackingSurface(&image, 0) == nullptr ? 0 : 4;
}

extern "C" int zvideo_blt_sw_to_primary_rect_lazy_failure_smoke(void) {
    zVidHwApiDeviceRecordPartial selectedDevice{};
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;

    zVidImagePartial image{};
    image.width = 8;
    image.height = 4;
    image.alphaMap = reinterpret_cast<char *>(0x1234);
    zVideo_dd::BltSwToPrimaryRect(&image, 0, nullptr, nullptr);
    return image.surface == nullptr ? 0 : 1;
}

extern "C" int zvideo_flip_to_gdi_if_attached_null_smoke(void) {
    g_zVideo_pDirectDraw2 = nullptr;
    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    zVideo_dd::FlipToGDIIfAttached();

    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    zVideo_dd::FlipToGDIIfAttached();
    return 0;
}

extern "C" int zvideo_clear_rect_skip_paths_smoke(void) {
    zVidRect32 rect{0, 0, 10, 10};
    zVideo_SurfaceStatePartial state{};

    g_zVideo_ClearScreenBufferEnabled = 0;
    g_zVideo_pZBufferSurface = nullptr;
    zVideo_dd::ZBuffer_DepthFillRect(&rect);
    zVideo_dd::ClearScreenAndZBufferRect(&rect, &state);
    zVideo_dd::ClearSwBackbufferAndZBufferRects(&rect, &rect);
    return 0;
}

extern "C" int zvideo_palette_set_entries_non8bpp_smoke(void) {
    PALETTEENTRY entries[2] = {};
    g_zVideo_DisplayModeBpp = 16;
    g_zVideo_pDDPalette = reinterpret_cast<IDirectDrawPalette *>(0x1234);
    const std::int32_t result = zVideo_dd::PaletteSetEntries(1, 2, entries);
    g_zVideo_pDDPalette = nullptr;
    return result;
}

extern "C" int zvideo_quad_batch_depth_and_rhw_smoke(void) {
    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));

    zVideo_dd3d::SetQuadBatchDepthAndRhw(0.25f);

    for (std::int32_t itemIndex = 0; itemIndex < 16; ++itemIndex) {
        for (std::int32_t vertexIndex = 0; vertexIndex < 4; ++vertexIndex) {
            const D3DTLVERTEX &vertex =
                g_zVideo_QuadBatchItemsBase[itemIndex].vertices[vertexIndex];
            if (vertex.sz != 0.25f || vertex.rhw != 0.25f) {
                return itemIndex + vertexIndex + 1;
            }
        }
    }

    return 0;
}

extern "C" int zvideo_queue_solid_quad_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_PrimarySurfaceState.width = 640;
    g_zVideo_PrimarySurfaceState.height = 480;

    zVideo_dd3d::QueueSolidQuad(0xf800, nullptr, 0.5);
    if (g_zVideo_QuadBatchCount != 1) {
        return 1;
    }

    const zVideo_QuadBatchItemPartial &full = g_zVideo_QuadBatchItemsBase[0];
    if (full.vertices[0].sx != 0.0f || full.vertices[0].sy != 0.0f ||
        full.vertices[1].sx != 480.0f || full.vertices[1].sy != 0.0f ||
        full.vertices[2].sx != 480.0f || full.vertices[2].sy != 640.0f ||
        full.vertices[3].sx != 0.0f || full.vertices[3].sy != 640.0f ||
        full.vertices[0].color != 0x7ff80000 || full.vertices[3].color != 0x7ff80000) {
        return 2;
    }

    zVidRect32 rect{10, 20, 30, 40};
    zVideo_dd3d::QueueSolidQuad(0x07e0, &rect, 0.25);
    if (g_zVideo_QuadBatchCount != 2) {
        return 3;
    }

    const zVideo_QuadBatchItemPartial &clipped = g_zVideo_QuadBatchItemsBase[1];
    if (clipped.vertices[0].sx != 10.0f || clipped.vertices[0].sy != 20.0f ||
        clipped.vertices[1].sx != 30.0f || clipped.vertices[1].sy != 20.0f ||
        clipped.vertices[2].sx != 30.0f || clipped.vertices[2].sy != 40.0f ||
        clipped.vertices[3].sx != 10.0f || clipped.vertices[3].sy != 40.0f ||
        clipped.vertices[0].color != 0x3f00fc00 || clipped.vertices[2].color != 0x3f00fc00) {
        return 4;
    }

    g_zVideo_QuadBatchCount = 0x10;
    zVideo_dd3d::QueueSolidQuad(0xffff, &rect, 1.0);
    return g_zVideo_QuadBatchCount == 0x10 ? 0 : 5;
}

extern "C" int zvideo_flush_quad_batch_empty_smoke(void) {
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_pD3DDevice = nullptr;
    zVideo_dd3d::FlushQuadBatch();
    return g_zVideo_QuadBatchCount == 0 ? 0 : 1;
}

extern "C" int zvideo_flush_sorted_polys_empty_smoke(void) {
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_pD3DDevice = nullptr;
    zVideo_dd3d::FlushSortedPolys();
    return g_zVideo_SortedPolyQueueCount == 0 ? 0 : 1;
}

extern "C" int zvideo_submit_poly_flat_color16_queue_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };

    zVideo_dd3d::SubmitPolyFlatColor16(vertices, 0xf800, 0xff, 0x1234, 3, 1);
    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 1 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderClass != 0 ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x1234) {
        return 1;
    }

    const D3DTLVERTEX &firstOpaque = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &lastOpaque = g_zVideo_OverwriteQueueBase[0].vertices[2];
    if (firstOpaque.sx != 7.0f || firstOpaque.sy != 8.0f || firstOpaque.sz != 9.0f ||
        firstOpaque.rhw != 9.0f || firstOpaque.color != 0xfff80000 ||
        firstOpaque.specular != 0xff000000 || lastOpaque.sx != 1.0f) {
        return 2;
    }

    zVideo_dd3d::SubmitPolyFlatColor16(vertices, 0x07e0, 0x80, 0x5678, 2, 0);
    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass != 0 ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x5678) {
        return 3;
    }

    const D3DTLVERTEX &firstSorted = g_zVideo_SortedPolyQueueBase[0].vertices[0];
    return firstSorted.sx == 4.0f && firstSorted.sy == 5.0f && firstSorted.sz == 6.0f &&
                   firstSorted.color == 0x8000fc00 && firstSorted.specular == 0xff000000
               ? 0
               : 4;
}

extern "C" int zvideo_submit_poly_gouraud_color16_queue_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    std::uint32_t colors[3] = {0xf800, 0x07e0, 0x001f};

    zVideo_dd3d::SubmitPolyGouraudColor16(vertices, colors, 0xff, 0x99, 3, 1);
    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 2 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x99) {
        return 1;
    }

    if (g_zVideo_OverwriteQueueBase[0].vertices[0].sx != 7.0f ||
        g_zVideo_OverwriteQueueBase[0].vertices[0].color != 0xff0000f8 ||
        g_zVideo_OverwriteQueueBase[0].vertices[2].sx != 1.0f ||
        g_zVideo_OverwriteQueueBase[0].vertices[2].color != 0xfff80000) {
        return 2;
    }

    zVideo_dd3d::SubmitPolyGouraudColor16(vertices, colors, 0x40, 0x77, 2, 0);
    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x77) {
        return 3;
    }

    return g_zVideo_SortedPolyQueueBase[0].vertices[0].sx == 4.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].color == 0x4000fc00 &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].sx == 1.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].color == 0x40f80000
               ? 0
               : 4;
}

extern "C" int zvideo_submit_poly_color_attr_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_ColorRgbFloat baseColor{16.0f, 32.0f, 48.0f};
    float attr1[1] = {0.0f};
    float attr0[3] = {0.0f, 0.5f, 1.0f};
    float attr2[3] = {0.0f, 0.5f, 1.0f};

    g_zVideo_D3DColorNormalizeChannelIndex = 2;
    g_zVideo_D3DColorAttrBiasR = 100.0f;
    g_zVideo_D3DColorAttrBiasG = 200.0f;
    g_zVideo_D3DColorAttrBiasB = 300.0f;

    zVideo_dd3d::SubmitPolyColorAttr(vertices, 0, &baseColor, attr1, attr0, attr2, 0x80, 3, 0x44,
                                     1);

    if (g_zVideo_OverwriteQueueCount != 0 || g_zVideo_SortedPolyQueueCount != 0) {
        return 1;
    }

    if (g_zVideo_D3DSubmitTempVertices[0].color != 0x8055aaff ||
        g_zVideo_D3DSubmitTempVertices[0].specular != 0x00000000 ||
        g_zVideo_D3DSubmitTempVertices[1].color != 0x804284c6 ||
        g_zVideo_D3DSubmitTempVertices[1].specular != 0x80000000 ||
        g_zVideo_D3DSubmitTempVertices[2].color != 0x80102030 ||
        g_zVideo_D3DSubmitTempVertices[2].specular != 0xff000000) {
        return 2;
    }

    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    attr1[0] = 0.0f;
    zVideo_dd3d::SubmitPolyColorAttr(vertices, 0, &baseColor, attr1, nullptr, nullptr, 0xff, 3,
                                     0x55, 1);

    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 3 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderClass != 0 ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x55) {
        return 3;
    }

    const D3DTLVERTEX &first = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &last = g_zVideo_OverwriteQueueBase[0].vertices[2];
    return first.sx == 7.0f && first.sy == 8.0f && first.sz == 9.0f && first.rhw == 9.0f &&
                   first.color == 0xff102030 && first.specular == 0xff000000 && last.sx == 1.0f &&
                   last.color == 0xff102030
               ? 0
               : 4;
}

extern "C" int zvideo_submit_poly_render_class_queue_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_TexCoord texCoords[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f},
    };
    zVideo_RenderClass renderClass{};
    renderClass.textureHandle = 0x1234;
    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(3);
    renderClass.textureAddressU = static_cast<D3DTEXTUREADDRESS>(1);
    renderClass.textureAddressV = static_cast<D3DTEXTUREADDRESS>(2);

    zVideo_dd3d::SubmitPolyRenderClass(vertices, texCoords, 3, &renderClass, 0x77, 1.0f, 1);

    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 4 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 3 ||
        g_zVideo_OverwriteQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x77) {
        return 1;
    }

    const D3DTLVERTEX &opaqueFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    if (opaqueFirst.sx != 7.0f || opaqueFirst.sy != 8.0f || opaqueFirst.sz != 9.0f ||
        opaqueFirst.rhw != 9.0f || opaqueFirst.color != 0xffffffff ||
        opaqueFirst.specular != 0xff000000 || opaqueFirst.tu != 0.5f || opaqueFirst.tv != 0.6f) {
        return 2;
    }

    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
    zVideo_dd3d::SubmitPolyRenderClass(vertices, texCoords, 2, &renderClass, 0x88, 0.25f, 1);
    if (g_zVideo_OverwriteQueueCount != 2 || g_zVideo_OverwriteQueueBase[1].type != 0 ||
        g_zVideo_OverwriteQueueBase[1].vertexCount != 2 ||
        g_zVideo_OverwriteQueueBase[1].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_OverwriteQueueBase[1].renderParam != 0x88 ||
        g_zVideo_OverwriteQueueBase[1].vertices[0].sx != 4.0f ||
        g_zVideo_OverwriteQueueBase[1].vertices[0].color != 0x3fffffff ||
        g_zVideo_OverwriteQueueBase[1].vertices[0].tu != 0.3f) {
        return 3;
    }

    zVideo_dd3d::SubmitPolyRenderClass(vertices, texCoords, 2, &renderClass, 0x99, 0.5f, 0);
    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x99) {
        return 4;
    }

    return g_zVideo_SortedPolyQueueBase[0].vertices[0].sx == 4.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].color == 0x7fffffff &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].tu == 0.3f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].sx == 1.0f
               ? 0
               : 5;
}

extern "C" int zvideo_submit_polygon_queue_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f},
    };
    zVideo_RenderClass renderClass{};
    renderClass.textureHandle = 0x2345;
    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(3);
    renderClass.textureAddressU = static_cast<D3DTEXTUREADDRESS>(1);
    renderClass.textureAddressV = static_cast<D3DTEXTUREADDRESS>(2);
    float attr1[1] = {0.0f};

    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    zVideo_dd3d::SubmitPolygon(vertices, uvPairs, attr1, nullptr, nullptr, 3, &renderClass, 0x66,
                               1.0f, 1);
    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 5 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 4 ||
        g_zVideo_OverwriteQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_OverwriteQueueBase[0].renderParam != 0x66 ||
        g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        return 1;
    }

    const D3DTLVERTEX &opaqueFirst = g_zVideo_OverwriteQueueBase[0].vertices[0];
    const D3DTLVERTEX &opaqueClose = g_zVideo_OverwriteQueueBase[0].vertices[3];
    if (opaqueFirst.sx != 7.0f || opaqueFirst.color != 0xffffffff ||
        opaqueFirst.specular != 0xff000000 || opaqueFirst.tu != 0.5f || opaqueClose.sx != 4.0f ||
        opaqueClose.tu != 0.3f) {
        return 2;
    }

    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
    attr1[0] = 0.5f;
    float attr2[2] = {0.0f, 1.0f};
    zVideo_dd3d::SubmitPolygon(vertices, uvPairs, attr1, nullptr, attr2, 2, &renderClass, 0x77,
                               0.5f, 0);

    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x77) {
        return 3;
    }

    const D3DTLVERTEX &transparentFirst = g_zVideo_SortedPolyQueueBase[0].vertices[0];
    const D3DTLVERTEX &transparentLast = g_zVideo_SortedPolyQueueBase[0].vertices[1];
    return transparentFirst.sx == 4.0f && transparentFirst.color == 0x7f7f7f7f &&
                   transparentFirst.specular == 0x00000000 && transparentFirst.tu == 0.3f &&
                   transparentLast.sx == 1.0f && transparentLast.color == 0x7f7f7f7f &&
                   transparentLast.specular == 0xff000000
               ? 0
               : 4;
}

extern "C" int zvideo_submit_polygon_lit_queue_smoke(void) {
    std::memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    std::memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase[0]));
    std::memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase[0]));
    g_zVideo_OverwriteQueueCount = 0;
    g_zVideo_SortedPolyQueueCount = 0;

    zVideo_XyzVertex vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVideo_TexCoord uvPairs[3] = {
        {0.1f, 0.2f},
        {0.3f, 0.4f},
        {0.5f, 0.6f},
    };
    zVideo_RenderClass renderClass{};
    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(3);
    float attr1[3] = {0.0f, 0.5f, 1.0f};

    g_zVideo_D3DAppendFanCloseVertexPending = 1;
    zVideo_dd3d::SubmitPolygonLit(vertices, uvPairs, attr1, nullptr, nullptr, 3, &renderClass, 0x44,
                                  1.0f, 1);

    if (g_zVideo_OverwriteQueueCount != 1 || g_zVideo_OverwriteQueueBase[0].type != 6 ||
        g_zVideo_OverwriteQueueBase[0].vertexCount != 4 ||
        g_zVideo_OverwriteQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        return 1;
    }

    if (g_zVideo_OverwriteQueueBase[0].vertices[0].sx != 7.0f ||
        g_zVideo_OverwriteQueueBase[0].vertices[0].color != 0xff000000 ||
        g_zVideo_OverwriteQueueBase[0].vertices[1].color != 0xff7f7f7f ||
        g_zVideo_OverwriteQueueBase[0].vertices[2].color != 0xffffffff ||
        g_zVideo_OverwriteQueueBase[0].vertices[3].sx != 4.0f) {
        return 2;
    }

    renderClass.textureMapBlend = static_cast<D3DTEXTUREBLEND>(4);
    float attr2[2] = {0.0f, 1.0f};
    zVideo_dd3d::SubmitPolygonLit(vertices, uvPairs, attr1, nullptr, attr2, 2, &renderClass, 0x55,
                                  0.5f, 0);

    if (g_zVideo_SortedPolyQueueCount != 1 || g_zVideo_SortedPolyQueueBase[0].vertexCount != 2 ||
        g_zVideo_SortedPolyQueueBase[0].renderClass !=
            reinterpret_cast<std::int32_t>(&renderClass) ||
        g_zVideo_SortedPolyQueueBase[0].renderParam != 0x55) {
        return 3;
    }

    return g_zVideo_SortedPolyQueueBase[0].vertices[0].sx == 4.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].color == 0x7f7f7f7f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[0].specular == 0x00000000 &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].sx == 1.0f &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].color == 0x7fffffff &&
                   g_zVideo_SortedPolyQueueBase[0].vertices[1].specular == 0xff000000
               ? 0
               : 4;
}

extern "C" int zvideo_present_display_mode_surface_null_smoke(void) {
    g_zVideo_DisplayModeSurfaceState.surf = nullptr;
    zVidRect32 rect{};
    return zVideo_dd3d::PresentDisplayModeSurface(&rect, &rect, 0, 0) == 0x400 ? 0 : 1;
}

extern "C" int zvideo_texture_record_release_upload_null_smoke(void) {
    zVideo_TextureRecordPartial textureRecord{};
    zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef(&textureRecord);
    return textureRecord.m_uploadSurface == nullptr ? 0 : 1;
}

extern "C" int zvideo_texture_record_create_and_power_smoke(void) {
    if (zVideo_dd3d::FloorPowerOfTwo(1) != 1 || zVideo_dd3d::FloorPowerOfTwo(3) != 2 ||
        zVideo_dd3d::FloorPowerOfTwo(64) != 64 || zVideo_dd3d::FloorPowerOfTwo(65) != 64) {
        return 1;
    }

    zVideo_TextureRecordPartial *textureRecord = zVideo_dd3d::TextureRecord_Create();
    if (textureRecord == nullptr) {
        return 2;
    }

    const bool zeroed = textureRecord->m_uploadSurface == nullptr &&
                        textureRecord->m_textureSurface == nullptr &&
                        textureRecord->m_texture == nullptr &&
                        textureRecord->m_textureHandle == 0 && textureRecord->m_alphaMode == 0 &&
                        textureRecord->m_uWrapMode == 0 && textureRecord->m_vWrapMode == 0;
    std::free(textureRecord);
    return zeroed ? 0 : 3;
}

extern "C" int zvideo_convert_image_pixels_for_texture_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    std::uint16_t srcPixels[4] = {0xf800, 0x07e0, 0x001f, 0};
    std::uint16_t dstPixels[8] = {};
    zVidImagePartial image{};
    image.width = 2;
    image.height = 2;
    image.pixels = srcPixels;

    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 0);

    if (dstPixels[0] != 0xfc00 || dstPixels[1] != 0x83f0 || dstPixels[4] != 0x801f ||
        dstPixels[5] != 0) {
        return 1;
    }

    std::uint8_t alphaMap[4] = {0xf0, 0x80, 0x10, 0};
    std::memset(dstPixels, 0, sizeof(dstPixels));
    image.alphaMap = reinterpret_cast<char *>(alphaMap);
    zVideo_dd3d::ConvertImagePixelsForTexture(dstPixels, &image, 8, 1);

    return dstPixels[0] == 0xff00 && dstPixels[1] == 0x80f0 && dstPixels[4] == 0x100f &&
                   dstPixels[5] == 0
               ? 0
               : 2;
}

extern "C" int zvid_image_resample_square_smoke(void) {
    std::uint16_t *pixels = static_cast<std::uint16_t *>(std::malloc(8 * sizeof(std::uint16_t)));
    char *alphaMap = static_cast<char *>(std::malloc(8));
    if (pixels == nullptr || alphaMap == nullptr) {
        std::free(pixels);
        std::free(alphaMap);
        return 1;
    }

    for (std::int32_t i = 0; i < 8; ++i) {
        pixels[i] = static_cast<std::uint16_t>(0x10 + i);
        alphaMap[i] = static_cast<char>(0x40 + i);
    }

    zVidImagePartial image{};
    image.width = 4;
    image.height = 2;
    image.pixels = pixels;
    image.alphaMap = alphaMap;

    zVid_Image::ResampleSquare(&image, 2);

    std::uint16_t *newPixels = static_cast<std::uint16_t *>(image.pixels);
    char *newAlphaMap = image.alphaMap;
    const bool ok =
        image.width == 2 && image.height == 2 && newPixels != nullptr && newAlphaMap != nullptr &&
        newPixels[0] == 0x10 && newPixels[1] == 0x12 && newPixels[2] == 0x14 &&
        newPixels[3] == 0x16 && newAlphaMap[0] == static_cast<char>(0x40) &&
        newAlphaMap[1] == static_cast<char>(0x42) && newAlphaMap[2] == static_cast<char>(0x44) &&
        newAlphaMap[3] == static_cast<char>(0x46);

    std::free(image.pixels);
    std::free(image.alphaMap);
    return ok ? 0 : 2;
}

extern "C" int zvid_image_calc_pow2_scratch_fields_smoke(void) {
    zVidImagePartial image{};
    image.width = 64;
    image.height = 16;
    image.uPow2Shift = 9;
    image.vPow2Shift = 9;

    zVid_Image::CalcPow2ScratchFields(&image);
    const bool pow2Ok = image.uPow2Shift == 6 && image.vPow2Shift == 4 &&
                        image.widthScale == 1.0f && image.uShiftFrom20 == 14 &&
                        image.uMask == 0x3f && image.vMaskFixed20 == 0x00f00000;

    image = {};
    image.width = 1;
    image.height = -2;
    zVid_Image::CalcPow2ScratchFields(&image);
    const bool smallOk = image.uPow2Shift == 0 && image.vPow2Shift == 0 &&
                         image.uShiftFrom20 == 20 && image.uMask == 0 && image.vMaskFixed20 == 0;

    return pow2Ok && smallOk ? 0 : 1;
}

extern "C" int zvid_image_release_owned_buffers_smoke(void) {
    zVidImagePartial image{};
    image.pixels = std::malloc(4);
    image.alphaMap = static_cast<char *>(std::malloc(4));
    image.palette = std::malloc(4);
    if (image.pixels == nullptr || image.alphaMap == nullptr || image.palette == nullptr) {
        std::free(image.pixels);
        std::free(image.alphaMap);
        std::free(image.palette);
        return 1;
    }

    image.formatFlagsPacked = 0xe0;
    zVid_Image::ReleaseOwnedBuffers(&image);
    if (image.pixels != nullptr || image.alphaMap != nullptr || image.palette != nullptr ||
        (image.formatFlagsPacked & 0xe0) != 0) {
        return 2;
    }

    void *palette = std::malloc(4);
    if (palette == nullptr) {
        return 3;
    }

    image.palette = palette;
    image.formatFlagsPacked = 0x90;
    zVid_Image::ReleaseOwnedBuffers(&image);
    const bool keptSharedPalette = image.palette == palette && image.formatFlagsPacked == 0x90;
    std::free(palette);
    image.palette = nullptr;
    return keptSharedPalette ? 0 : 4;
}

extern "C" int zvid_image_destroy_smoke(void) {
    zVidImagePartial *image =
        static_cast<zVidImagePartial *>(std::malloc(sizeof(zVidImagePartial)));
    if (image == nullptr) {
        return 1;
    }

    *image = {};
    image->pixels = std::malloc(4);
    if (image->pixels == nullptr) {
        std::free(image);
        return 2;
    }
    image->formatFlagsPacked = 0x20;

    return zVid_Image::Destroy(image) == 0 && zVid_Image::Destroy(nullptr) == 0 ? 0 : 3;
}

extern "C" int zvideo_create_texture_record_guards_smoke(void) {
    zVidD3DDriverRecordPartial selectedD3DDevice{};
    D3DDEVICEDESC *selectedDesc = reinterpret_cast<D3DDEVICEDESC *>(selectedD3DDevice.m_hwDesc);
    selectedDesc->dwMaxTextureWidth = 64;
    selectedDesc->dwMaxTextureHeight = 64;
    g_zVideo_pSelectedD3DDeviceInfo = &selectedD3DDevice;

    zVideo_TextureRecordPartial defaultRecord{};
    g_zImage_DefaultTextureRecord = &defaultRecord;

    zVidImagePartial image{};
    image.width = 128;
    image.height = 8;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = 0;
    if (zVideo_dd3d::CreateTextureRecord("too-large", &image, 0, 0, 0) != &defaultRecord) {
        return 1;
    }

    image.width = 10;
    image.height = 8;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = D3DPTEXTURECAPS_POW2;
    if (zVideo_dd3d::CreateTextureRecord("non-pow2", &image, 0, 0, 0) != &defaultRecord) {
        return 2;
    }

    image.width = 64;
    image.height = 4;
    g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps = 0;
    if (zVideo_dd3d::CreateTextureRecord("bad-aspect", &image, 0, 0, 0) != &defaultRecord) {
        return 3;
    }

    image.width = 8;
    image.height = 8;
    image.palette = reinterpret_cast<void *>(0x1234);
    if (zVideo_dd3d::CreateTextureRecord("paletted", &image, 0, 0, 0) != &defaultRecord) {
        return 4;
    }

    image.palette = nullptr;
    g_zImage_DefaultTextureRecord = nullptr;
    g_zVideo_pSelectedD3DDeviceInfo = nullptr;
    return 0;
}

extern "C" int zvideo_image_alpha_clear_smoke(void) {
    std::uint16_t pixels[4] = {0x1111, 0x2222, 0x3333, 0x4444};
    char alpha[4] = {1, 0, 1, 0};

    zVidImagePartial image{};
    image.pixelCount = 4;
    image.formatFlagsPacked = 1;
    image.paletteMetaPacked = 0;
    image.pixels = pixels;
    image.alphaMap = alpha;

    if (zVid_Image::QueryBytesPerPixel(&image) != 2) {
        return 1;
    }

    zVid_Image::ClearZeroAlphaPixelsInPlace(&image);
    return pixels[0] == 0x1111 && pixels[1] == 0 && pixels[2] == 0x3333 && pixels[3] == 0 ? 0 : 2;
}

extern "C" int zvideo_image_set_pixels_smoke(void) {
    zVidImagePartial image{};
    std::uint16_t pixels[2] = {0x1111, 0x2222};
    char alpha[2] = {1, 0};

    image.formatFlagsPacked = 0x20;
    const bool withAlpha = zVid_Image_SetPixels(&image, pixels, alpha) == 0 &&
                           image.pixels == pixels && image.alphaMap == alpha &&
                           (image.formatFlagsPacked & 0x22u) == 0x22u;

    image.formatFlagsPacked = 0x20;
    const bool withoutAlpha = zVid_Image_SetPixels(&image, pixels, nullptr) == 0 &&
                              image.pixels == pixels && image.alphaMap == nullptr &&
                              image.formatFlagsPacked == 0x20u;

    return withAlpha && withoutAlpha ? 0 : 1;
}

extern "C" int zvideo_image_file_read_helpers_smoke(void) {
    unsigned char header[0x10] = {};
    header[0] = 1;
    *reinterpret_cast<std::int16_t *>(&header[4]) = 2;
    *reinterpret_cast<std::int16_t *>(&header[6]) = 1;
    header[8] = 0x12;
    *reinterpret_cast<std::int16_t *>(&header[0x0c]) = 0x3456;
    *reinterpret_cast<std::int16_t *>(&header[0x0e]) = 0;
    std::uint16_t pixels[2] = {0x7fff, 0x001f};

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::fwrite(header, 1, sizeof(header), file);
    std::fwrite(pixels, 1, sizeof(pixels), file);
    std::rewind(file);

    g_zVideo_PixelPack_RBits = 5;
    zVidImagePartial *image = zVid_Image::ReadFromFile(file);
    std::fclose(file);

    if (image == nullptr) {
        return 2;
    }

    std::uint16_t *readPixels = static_cast<std::uint16_t *>(image->pixels);
    const bool ok = image->width == 2 && image->height == 1 && image->pixelCount == 2 &&
                    image->pitchWords == 2 && image->headerFlagsByte == 0x12 &&
                    image->textureAddressFlagsPacked == 0x3456 && image->paletteMetaPacked == 0 &&
                    (image->formatFlagsPacked & 0x21) == 0x21 && readPixels[0] == 0x3fff &&
                    readPixels[1] == 0x001f;

    zVid_Image::Destroy(image);
    return ok ? 0 : 3;
}

extern "C" int zvideo_palette_remap_no_recipes_smoke(void) {
    std::uint16_t palette[2] = {1, 2};
    g_zVid_PaletteRemapRecipeCount = 0;
    g_zVid_PaletteRemapRecipes = nullptr;
    return zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(palette, 2) == palette ? 0 : 1;
}

extern "C" int zvideo_texture_pack_load_image_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "ztp", 0, packPath) == 0) {
        return 1;
    }

    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 1;
    zVidTexturePackRecord record{};
    std::strcpy(record.name, "font.tex");
    record.fileOffset = sizeof(packHeader) + sizeof(record);
    record.paletteIndex = -1;

    unsigned char imageHeader[0x10] = {};
    imageHeader[0] = 1;
    *reinterpret_cast<std::int16_t *>(&imageHeader[4]) = 1;
    *reinterpret_cast<std::int16_t *>(&imageHeader[6]) = 1;
    std::uint16_t pixel = 0x1234;

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }
    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(&record, sizeof(record), 1, out);
    std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
    std::fwrite(&pixel, sizeof(pixel), 1, out);
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    g_zVid_TexturePackLoadState = 1;
    g_zVideo_PixelPack_RBits = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        DeleteFileA(packPath);
        return 3;
    }

    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;
    zVidImagePartial *image = zVid_TexturePack_LoadImageByName("font.tex");
    const bool ok = image != nullptr && image->width == 1 && image->height == 1 &&
                    image->pixelCount == 1 &&
                    static_cast<std::uint16_t *>(image->pixels)[0] == pixel;

    zVid_Image::Destroy(image);
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = nullptr;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePackLoadState = 0;
    DeleteFileA(packPath);
    return ok ? 0 : 4;
}

extern "C" int zvideo_image_surface_helpers_guard_smoke(void) {
    zVidHwApiDeviceRecordPartial selectedDevice{};
    g_zVideo_pSelectedHwApiDeviceRecord = &selectedDevice;
    g_zVideo_UseHalfResBackbuffer = 0;

    zVidImagePartial image{};
    if (zVideo_dd::Image_LazyCreateVideoMemorySurface(&image) != nullptr) {
        return 1;
    }

    image.surface = reinterpret_cast<IDirectDrawSurface3 *>(0x1234);
    image.pixels = reinterpret_cast<void *>(0x5678);
    g_zVideo_IsInitialized = 0;
    zVideo_dd::Image_EnsureSurfaceForCurrentDevice(&image);
    if (image.surface != nullptr || image.pixels != nullptr) {
        return 2;
    }

    HDC hdc = nullptr;
    g_zVideo_RendererType = 2;
    if (zVideo_dd::Image_UploadPixelsToSurface(&image, &hdc) != 0) {
        return 3;
    }

    return zVideo_dd::Image_ReleaseSurface(&image, hdc) == 0 ? 0 : 4;
}

extern "C" int zvideo_restore_display_surfaces_null_smoke(void) {
    g_zVideo_DisplayModeSurfaceState.surf = nullptr;
    g_zVideo_PrimarySurfaceState.surf = nullptr;
    g_zVideo_SwSurfaceState.surf = nullptr;
    return zVideo_dd::RestoreDisplaySurfaces();
}

extern "C" int zvideo_select_hw_api_device_smoke(void) {
    g_zVideo_pSelectedHwApiDeviceRecord = nullptr;
    g_zVideo_pSelectedD3DDeviceInfo = nullptr;

    if (zVideo::SelectHwApiDeviceOrFallback(-1) != 0 || g_zVideo_RendererType != 0 ||
        g_zVideo_FullscreenOption != 1 ||
        g_zVideo_pSelectedHwApiDeviceRecord != &g_zVideo_HwApiDeviceTable[0] ||
        g_zVideo_pSelectedD3DDeviceInfo != nullptr) {
        return 1;
    }

    if (zVideo::SelectHwApiDeviceOrFallback(2) != 1 || g_zVideo_RendererType != 1 ||
        g_zVideo_FullscreenOption != 1 ||
        g_zVideo_pSelectedHwApiDeviceRecord != &g_zVideo_HwApiDeviceTable[2] ||
        g_zVideo_pSelectedD3DDeviceInfo != g_zVideo_HwApiDeviceTable[2].m_d3dDrivers) {
        return 2;
    }

    return 0;
}

extern "C" int zvideo_return_success_stub_smoke(void) {
    return zVideo::ReturnSuccessStub();
}

extern "C" int zvid_cached_client_rect_smoke(void) {
    zVid::SetCachedClientRectUpdateMask(0x55);
    g_zVideo_ActiveRendererPath = 1;
    if (zVid_QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
        return 1;
    }

    g_zVideo_ActiveRendererPath = 2;
    if (zVid_QueryCachedClientRectUpdateMaskIf3dfx() != 0x55) {
        return 2;
    }

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-video-test", WS_OVERLAPPEDWINDOW, 20, 30, 160,
                                120, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 3;
    }

    RECT client{};
    GetClientRect(hwnd, &client);
    g_zVideo_hWnd = hwnd;
    g_zVideo_CachedClientRectScreen = {100, 100, 100, 100};

    const std::int32_t result = zVideo::UpdateCachedClientRectScreenCoords();
    const LONG cachedWidth =
        g_zVideo_CachedClientRectScreen.right - g_zVideo_CachedClientRectScreen.left;
    const LONG cachedHeight =
        g_zVideo_CachedClientRectScreen.bottom - g_zVideo_CachedClientRectScreen.top;

    g_zVideo_CachedClientRectScreen = {7, 8, 9, 10};
    zVid::SetCachedClientRectUpdateMask(0);
    zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
    const bool noUpdateOk =
        g_zVideo_CachedClientRectScreen.left == 7 && g_zVideo_CachedClientRectScreen.top == 8 &&
        g_zVideo_CachedClientRectScreen.right == 9 && g_zVideo_CachedClientRectScreen.bottom == 10;

    zVid::SetCachedClientRectUpdateMask(1);
    zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
    const LONG helperWidth =
        g_zVideo_CachedClientRectScreen.right - g_zVideo_CachedClientRectScreen.left;
    const LONG helperHeight =
        g_zVideo_CachedClientRectScreen.bottom - g_zVideo_CachedClientRectScreen.top;

    DestroyWindow(hwnd);
    return result == 0 && cachedWidth == client.right - client.left &&
                   cachedHeight == client.bottom - client.top && noUpdateOk &&
                   helperWidth == client.right - client.left &&
                   helperHeight == client.bottom - client.top
               ? 0
               : 4;
}

extern "C" int zvideo_restore_iconic_fullscreen_window_smoke(void) {
    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-video-iconic-test", WS_OVERLAPPEDWINDOW, 20,
                                30, 160, 120, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    g_zVideo_hWnd = hwnd;
    g_zVideo_IsInitialized = 1;
    g_zVideo_FullscreenOption = 1;
    ShowWindow(hwnd, SW_MINIMIZE);
    zVideo_RestoreIconicFullscreenWindowIfNeeded();
    const bool restored = IsIconic(hwnd) == 0;

    ShowWindow(hwnd, SW_MINIMIZE);
    g_zVideo_FullscreenOption = 0;
    zVideo_RestoreIconicFullscreenWindowIfNeeded();
    const bool skipped = IsIconic(hwnd) != 0;

    DestroyWindow(hwnd);
    return restored && skipped ? 0 : 2;
}

namespace {
int g_videoShutdownCalls;

void RECOIL_CDECL VideoShutdownFake() {
    ++g_videoShutdownCalls;
}
} // namespace

extern "C" int zvideo_shutdown_video_system_smoke(void) {
    g_videoShutdownCalls = 0;
    g_zVideo_IsInitialized = 0;
    g_zVideo_pfnShutdownVideoSystem = VideoShutdownFake;

    if (zVideo::ShutdownVideoSystem() != 0x5a560000 || g_videoShutdownCalls != 0) {
        return 1;
    }

    g_zVideo_IsInitialized = 1;
    const std::int32_t result = zVideo::ShutdownVideoSystem();
    return result == 0 && g_zVideo_IsInitialized == 0 && g_videoShutdownCalls == 1 ? 0 : 2;
}
