#include "Battlesport/Mfc42Abi.h"

#include "GameZRecoil/zVideo/zVideo.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideoFxPass3.h"
#include "zClass.h"

#include <math.h>
#include <malloc.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
const int kZVidPaletteColorCount = 256;
const int kZVidPaletteRemapVariantCount = 32;
const int kZVidPaletteRemapColorsPerRecipe =
    kZVidPaletteColorCount * kZVidPaletteRemapVariantCount;

/**
 * Recovered helper: zVidPaletteRemapTableBytesForRecipeCount.
 * Original-source helper evidence: no standalone retail function is present; callers inline
 * the recipe-count scaling as recipeCount * 0x4000 + 0x200 bytes of 16-bit palette data.
 * Purpose: compute the palette-remap table byte count for the current recipe count.
 */
size_t zVidPaletteRemapTableBytesForRecipeCount(
    int recipeCount
) {
    return (size_t)(
        (recipeCount * kZVidPaletteRemapColorsPerRecipe) +
        kZVidPaletteColorCount
    ) * sizeof(unsigned short);
}

/**
 * Recovered helper: zVideo_BlendPixel565Alpha8.
 * Original-source helper evidence: no standalone retail function is present; recovered from
 * address-backed framebuffer blit callers in this source file.
 * Purpose: Blend one 565 destination/source pixel pair using an 8-bit alpha value.
 */
unsigned short zVideo_BlendPixel565Alpha8(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
) {
    const int dstColor = (short)(dstPixel);
    const int srcColor = srcPixel;
    const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
    const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
    int blended = dstColor + (redDelta & 0xfffff800);
    const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return (unsigned short)(blended);
}

/**
 * Recovered helper: zVideo_BlendPixel555Alpha8.
 * Original-source helper evidence: no standalone retail function is present; recovered from
 * address-backed framebuffer blit callers in this source file.
 * Purpose: Blend one 555 destination/source pixel pair using an 8-bit alpha value.
 */
unsigned short zVideo_BlendPixel555Alpha8(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
) {
    const int dstColor = (short)(dstPixel);
    const int srcColor = srcPixel;
    const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
    int blended = dstColor + (redDelta & 0xfffffc00);
    const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
    const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return (unsigned short)(blended);
}

/**
 * Recovered helper: zVideo_BlendFramebufferPixelAlpha8.
 * Original-source helper evidence: no standalone retail function is present; recovered from
 * address-backed framebuffer blit callers in this source file.
 * Purpose: Select the current framebuffer pixel format and blend one alpha-scaled pixel.
 */
unsigned short zVideo_BlendFramebufferPixelAlpha8(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
) {
    if (zRndr::g_pixelPackGreenBits == 6) {
        return zVideo_BlendPixel565Alpha8(
            dstPixel,
            srcPixel,
            alpha
        );
    }

    return zVideo_BlendPixel555Alpha8(
        dstPixel,
        srcPixel,
        alpha
    );
}

/**
 * Recovered helper: zVideo_GetAlphaSkipThreshold.
 * Original-source helper evidence: no standalone retail function is present; recovered from
 * address-backed framebuffer blit callers in this source file.
 * Purpose: Return the alpha-map threshold below which framebuffer pixels are skipped.
 */
int zVideo_GetAlphaSkipThreshold() {
    return zRndr::g_pixelPackGreenBits == 6 ? 3 : 7;
}
} // namespace

extern "C" {
zVideo_PixelPackParams g_zVideo_PixelPack = {0};
/*
 * BN models the texture pixel-pack BSS block at 0x632188..0x6321c4 as the
 * scalar field order below; TexturePixelPack_SetupFromMasks is the writer.
 */
int g_zVideo_TexturePixelPack_RBits = 0;
int g_zVideo_TexturePixelPack_GBits = 0;
int g_zVideo_TexturePixelPack_BBits = 0;
int g_zVideo_TexturePixelPack_ABits = 0;
unsigned int g_zVideo_TexturePixelPack_RMask = 0;
unsigned int g_zVideo_TexturePixelPack_GMask = 0;
unsigned int g_zVideo_TexturePixelPack_BMask = 0;
unsigned int g_zVideo_TexturePixelPack_AMask = 0;
int g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 = 0;
int g_zVideo_TexturePixelPack_GBBitsTotalMinus8 = 0;
int g_zVideo_TexturePixelPack_BShiftTo8 = 0;
int g_zVideo_TexturePixelPack_RGBBitsTotal = 0;
int g_zVideo_TexturePixelPack_RMaskShifted = 0;
int g_zVideo_TexturePixelPack_GMaskShifted = 0;
int g_zVideo_TexturePixelPack_BMaskShifted = 0;
int g_zVideo_TexturePixelPack_NonRgbMaskShifted = 0;
/**
 * Reimplements data 0x53d780: g_zVid_PaletteRemapRecipeCount.
 * Reimplements data 0x53d784: g_zVid_PaletteRemapRecipes.
 * Palette-remap recipe owner: BuildPaletteVariant grows this bank before
 * rebuilding per-image and standalone palette variant tables.
 * Purpose: track the active palette-remap recipe bank.
 */
int g_zVid_PaletteRemapRecipeCount = 0;
zVidPaletteRemapRecipe *g_zVid_PaletteRemapRecipes = 0;
/**
 * Reimplements data 0x632120: g_zVideo_RendererType.
 * Reimplements data 0x56bbe8: g_zVideo_ActiveRendererPath.
 * Reimplements data 0x56bbd8: g_zVideo_FrameTick.
 * Purpose: cache the selected renderer path and current video frame tick.
 */
int g_zVideo_RendererType = 0;
int g_zVideo_ActiveRendererPath = 0;
int g_zVideo_FrameTick = 0;
/**
 * Reimplements data 0x5398fc: g_zVideo_pActiveViewContext.
 * Render-frame active view owner data. BN types this 4-byte .data slot as a
 * zVideo_ViewContext pointer, zero-initialized, and separates its xrefs from
 * the projection/frustum context cache at 0x576214.
 * Purpose: store the camera data record used by the software render frame.
 */
zClass_CameraDataPartial *g_zVideo_pActiveViewContext = 0;
/**
 * Reimplements data 0x576214: g_zVideo_pActiveProjectionViewContext.
 * Projection/frustum active view owner data. BN types this separate 4-byte
 * .data slot as a zVideo_ViewContext pointer, zero-initialized; SetActiveViewContext
 * writes it before projection, model, and frustum users read it.
 * Purpose: cache the camera data record used by projection, clip, and frustum state.
 */
zClass_CameraDataPartial *g_zVideo_pActiveProjectionViewContext = 0;
/**
 * Reimplements data 0x5398f8: g_zVideo_ActiveViewVariantTag.
 * Render-frame variant owner data. BN xrefs show zVideo_sw::RenderFrame and
 * zClass_Camera::RenderScene copying the active view variant tag into this
 * 4-byte .data zTag4 record after view selection; retail initializes it to zero.
 * Purpose: cache the currently selected variant tag for render traversal.
 */
zTag4Partial g_zVideo_ActiveViewVariantTag = {0};
/**
 * Reimplements data 0x57623c: g_zVideo_ProjectClipLeft.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's left project-clip edge for projection users.
 */
float g_zVideo_ProjectClipLeft = 0.0f;
/**
 * Reimplements data 0x576240: g_zVideo_ProjectClipTop.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's top project-clip edge for projection users.
 */
float g_zVideo_ProjectClipTop = 0.0f;
/**
 * Reimplements data 0x576244: g_zVideo_ProjectClipRight.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's right project-clip edge for projection users.
 */
float g_zVideo_ProjectClipRight = 0.0f;
/**
 * Reimplements data 0x576248: g_zVideo_ProjectClipBottom.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's bottom project-clip edge for projection users.
 */
float g_zVideo_ProjectClipBottom = 0.0f;
/**
 * Reimplements data 0x63214c: gVideo_resolutionMenuValid.
 * Reimplements data 0x6321cc: g_zVideo_ClearColorPacked16.
 * Reimplements data 0x632130: g_zVideo_ClearScreenBufferEnabled.
 * Purpose: cache global video initialization and clear-screen options.
 */
int gVideo_resolutionMenuValid = 0;
unsigned int g_zVideo_ClearColorPacked16 = 0;
int g_zVideo_ClearScreenBufferEnabled = 0;
/**
 * Reimplements data 0x56b564: g_zVid_CachedClientRectUpdateMask.
 * BN xrefs identify this zero-initialized int32 as the cached-client-rect
 * update mask used by the setter and renderer-path query helpers.
 * Purpose: gate refreshes of the cached client rectangle.
 */
int g_zVid_CachedClientRectUpdateMask = 0;
/**
 * Reimplements data 0x632154: g_zVideo_IsInitialized.
 * Reimplements data 0x632144: g_zVideo_AdjustSurfacesDisableGate.
 * Reimplements data 0x632124: g_zVideo_FullscreenOption.
 * Purpose: cache video initialization, adjust-surface, and fullscreen state.
 */
int g_zVideo_IsInitialized = 0;
int g_zVideo_AdjustSurfacesDisableGate = 0;
int g_zVideo_FullscreenOption = 0;
/**
 * Reimplements data 0x632134: g_zVideo_PrimaryHasAttachedBackbuffer.
 * Purpose: remember whether the primary DirectDraw surface has an attached
 * backbuffer that can be flipped back to GDI during shutdown or mode changes.
 *
 * DirectDraw primary-surface state owner data. BN xrefs show the flag is set
 * by the surface creation paths when the primary surface owns an attached
 * backbuffer, cleared by the fullscreen surface path without one, and checked
 * by FlipToGDIIfAttached before calling IDirectDraw2::FlipToGDISurface.
 */
int g_zVideo_PrimaryHasAttachedBackbuffer = 0;
/**
 * Reimplements data 0x632128: g_zVideo_UseHalfResBackbuffer.
 * Reimplements data 0x63212c: g_zVideo_HalfResAdjustMode.
 * Purpose: cache half-resolution backbuffer presentation state.
 */
int g_zVideo_UseHalfResBackbuffer = 0;
int g_zVideo_HalfResAdjustMode = 0;
/**
 * Reimplements data 0x4dd1c0: g_zVideo_SoftwareModeHotkeyEnabled.
 * Retail initializes the authored zVideo debug/software-mode hotkey gate enabled.
 * Purpose: gate the software-mode hotkey command.
 */
int g_zVideo_SoftwareModeHotkeyEnabled = 1;
/**
 * Reimplements data 0x633434: g_zVideo_CachedFogModeLightState.
 * Reimplements data 0x633430: g_zVideo_CachedFogEnableRenderState.
 * Reimplements data 0x633438: g_zVideo_CachedFogStartLightStateValue.
 * Reimplements data 0x63343c: g_zVideo_CachedFogEndLightStateValue.
 * Purpose: cache the Direct3D fog render-state values already applied.
 */
int g_zVideo_CachedFogModeLightState = 0;
int g_zVideo_CachedFogEnableRenderState = 0;
float g_zVideo_CachedFogStartLightStateValue = 0.0f;
float g_zVideo_CachedFogEndLightStateValue = 0.0f;
/**
 * Reimplements data 0x632140: g_zVideo_D3DColorNormalizeChannelIndex.
 * Reimplements data 0x6321d0: g_zVideo_FogColorPendingR255.
 * Reimplements data 0x6321d4: g_zVideo_FogColorPendingG255.
 * Reimplements data 0x6321d8: g_zVideo_FogColorPendingB255.
 * Reimplements data 0x6321dc: g_zVideo_D3DColorAttrBiasR.
 * Reimplements data 0x6321e0: g_zVideo_D3DColorAttrBiasG.
 * Reimplements data 0x6321e4: g_zVideo_D3DColorAttrBiasB.
 * D3D fog/color-attribute bias owner data. Current BN models the channel index
 * at 0x632140 separately from the adjacent RGB bias floats at 0x6321dc-0x6321e4;
 * 0x4a7250 writes them and the submitters at 0x4ab320, 0x4abb20, and 0x4ac370
 * consume them while packing D3D TLVERTEX colors.
 * Purpose: stage pending fog color and color-attribute bias for D3D submitters.
 */
int g_zVideo_D3DColorNormalizeChannelIndex = 0;
float g_zVideo_FogColorPendingR255 = 0.0f;
float g_zVideo_FogColorPendingG255 = 0.0f;
float g_zVideo_FogColorPendingB255 = 0.0f;
float g_zVideo_D3DColorAttrBiasR = 0.0f;
float g_zVideo_D3DColorAttrBiasG = 0.0f;
float g_zVideo_D3DColorAttrBiasB = 0.0f;
/**
 * Reimplements data 0x6321e8: g_zVideo_FogTargetColorR255.
 * Reimplements data 0x6321ec: g_zVideo_FogTargetColorG255.
 * Reimplements data 0x6321f0: g_zVideo_FogTargetColorB255.
 * Reimplements data 0x6321f4: g_zVideo_FogColorAppliedR255.
 * Reimplements data 0x6321f8: g_zVideo_FogColorAppliedG255.
 * Reimplements data 0x6321fc: g_zVideo_FogColorAppliedB255.
 * Purpose: cache target and applied D3D fog color channels.
 */
float g_zVideo_FogTargetColorR255 = 0.0f;
float g_zVideo_FogTargetColorG255 = 0.0f;
float g_zVideo_FogTargetColorB255 = 0.0f;
float g_zVideo_FogColorAppliedR255 = 0.0f;
float g_zVideo_FogColorAppliedG255 = 0.0f;
float g_zVideo_FogColorAppliedB255 = 0.0f;
/**
 * Reimplements data 0x63213c: g_zVideo_PendingDitherEnable.
 * Purpose: stage the Direct3D dither render-state value before scene entry.
 */
int g_zVideo_PendingDitherEnable = 0;
/**
 * Reimplements data 0x56bbf4: g_zVideo_InverseZTolerancePending.
 * BN xrefs: zModel_Display_Init, zRndr::SetInverseZTolerance, and
 * zVideo::ModuleInit write this staged hardware-renderer inverse-Z tolerance.
 * Purpose: cache the inverse-Z tolerance pending for non-software renderer paths.
 */
float g_zVideo_InverseZTolerancePending = 0.0f;
/**
 * Reimplements data 0x56bbec: g_zVideo_D3DAppendFanCloseVertexPending.
 * Reimplements data 0x632138: g_zVideo_PendingWireframeState.
 * Purpose: stage Direct3D fan-closing and wireframe render state.
 */
int g_zVideo_D3DAppendFanCloseVertexPending = 0;
int g_zVideo_PendingWireframeState = 0;
/**
 * Reimplements data 0x632148: g_zVideo_D3DSceneDepth.
 * BN types this 4-byte .data slot as an int32, zero-initialized, and confines
 * its xrefs to zVideoD3D::SceneEnter/SceneLeave nesting.
 * Purpose: track nested Direct3D scene entry/leave ownership.
 */
int g_zVideo_D3DSceneDepth = 0;
/**
 * Reimplements data 0x632f9c: g_zVid_AcceptedHardwareRendererCount.
 * Reimplements data 0x632f98: g_zVideo_NumAcceptedDirectDrawDevices.
 * Reimplements data 0x56bc98: g_zVideo_DirectDrawEnumOrdinal.
 * DirectDraw/D3D enumeration counters. BN ties the accepted DirectDraw count
 * at 0x632f98 to the public zVid thunk at 0x4a7480, the accepted renderer
 * count at 0x632f9c to the D3D callback/accessor pair, and the enumeration
 * ordinal at 0x56bc98 only to DirectDrawEnumCallback logging.
 * Purpose: cache startup DirectDraw and Direct3D enumeration totals.
 */
int g_zVid_AcceptedHardwareRendererCount = 0;
int g_zVideo_NumAcceptedDirectDrawDevices = 0;
int g_zVideo_DirectDrawEnumOrdinal = 0;
/**
 * Reimplements data 0x4e073c: g_zVid_TexturePackLoadState.
 * Reimplements data 0x53d768: g_zVid_BuiltinTexturePackCount.
 * Reimplements data 0x53d76c: g_zVid_BuiltinTexturePacks.
 * Reimplements data 0x53d770: g_zVid_TexturePackCount.
 * Reimplements data 0x53d774: g_zVid_TexturePacks.
 * Retail starts with texture-pack loading enabled; zVid accessors toggle this
 * between 0 and 1.
 * Purpose: cache built-in and loaded texture-pack runtime arrays.
 */
int g_zVid_TexturePackLoadState = 1;
int g_zVid_BuiltinTexturePackCount = 0;
zVidTexturePackEntry *g_zVid_BuiltinTexturePacks = 0;
int g_zVid_TexturePackCount = 0;
zVidTexturePackEntry *g_zVid_TexturePacks = 0;
/**
 * Reimplements data 0x4e07e8: g_zVid_DefaultImageTexturePackReadonlyNameFmt.
 * BN identifies this row as g_str_fmt_r_s, a writable char[0x04] format
 * literal used by zVid_TexturePack_EnsureDefaultImagePackLoaded.
 * Purpose: provide the renderer-prefixed default image-pack filename format.
 *
 * Retail stores this row immediately before the "image.zbd" row; both remain
 * mutable .data char arrays rather than const string literals.
 */
char g_zVid_DefaultImageTexturePackReadonlyNameFmt[0x04] = "r%s";
/**
 * Reimplements data 0x4e07ec: g_zVid_DefaultImageTexturePackName.
 * BN identifies this row as g_str_image_zbd, a writable char[0x0a] literal
 * used by zVid_TexturePack_EnsureDefaultImagePackLoaded.
 * Purpose: provide the default image texture-pack archive filename.
 */
char g_zVid_DefaultImageTexturePackName[0x0a] = "image.zbd";
/**
 * Reimplements data 0x4e07f8..0x4e0844:
 * render_video.zvid_texture_pack_archive_literals_data.
 * Purpose: writable archive filename pieces used by
 * zVid_TexturePack_EnsureBuiltinTexturePacksLoaded when probing built-in
 * texture packs.
 *
 * Retail keeps these strings in this row order, with padding bytes preserved
 * by the declared array sizes. The archive extension is the suffix inside the
 * "texturemax.zbd" row, so no separate "zbd" row is introduced here.
 */
char g_zVid_TextureArchiveNameFmt[0x8] = "%s.%s";
char g_zVid_TextureArchiveSizedNameFmt[0x8] = "%s%d.%s";
char g_zVid_TextureArchiveMaxName[0x10] = "texturemax.zbd";
char g_zVid_TextureArchiveSize2Fmt[0x8] = "%s2.%s";
char g_zVid_TextureArchiveSize4Fmt[0x8] = "%s4.%s";
char g_zVid_TextureArchiveSize6Fmt[0x8] = "%s6.%s";
char g_zVid_TextureArchiveSize8Fmt[0x8] = "%s8.%s";
char g_zVid_TextureArchiveRendererSizedNameFmt[0x0c] = "r%s%d.%s";
char g_zVid_TextureArchiveStem[0x8] = "texture";
/**
 * Reimplements data 0x4e3054: g_zVideo_SourceFile_ZvidBuffC.
 * BN types this writable char[0x27] as the zvid_buff.c source-path literal
 * passed to zVideo_dd::ReportError by BltSourceToPrimaryClipped.
 * Purpose: Supplies the original zVideo buffer source-file path for diagnostics.
 */
char g_zVideo_SourceFile_ZvidBuffC[0x27] =
    "D:\\Proj\\GameZRecoil\\zVideo\\zvid_buff.c";
/**
 * Reimplements data 0x4e3084: g_zVideo_InitFailSetModeMsg.
 * Reimplements data 0x4e30a0: g_zVideo_SourceFile_ZvidInitC.
 * Reimplements data 0x4e30c8: g_zVideo_InitFailOpenVideoModeMsg.
 * Purpose: writable zvid_init.c diagnostics passed by InitVideoSystem when
 * opening or setting the selected video mode fails.
 *
 * Retail stores these zvid_init.c rows as writable char arrays, not const
 * string literals, and InitVideoSystem passes their storage directly to the
 * old zError reporting path.
 */
char g_zVideo_InitFailSetModeMsg[0x19] = "Failed to set video mode";
char g_zVideo_SourceFile_ZvidInitC[0x27] =
    "D:\\Proj\\GameZRecoil\\zVideo\\zvid_init.c";
char g_zVideo_InitFailOpenVideoModeMsg[0x1a] =
    "Failed to open video mode";
/**
 * Reimplements data 0x4e30e8: g_zVideo_SourceFile_ZvidDdC.
 * BN types this writable char[0x25] as the zvid_dd.c source-path literal
 * passed by DirectDraw diagnostics. This row is separate from the zvid_init.c,
 * zvid_buff.c, and pending adjacent zVideo source-file strings.
 * Purpose: Supplies the original DirectDraw source-file path for diagnostics.
 */
char g_zVideo_SourceFile_ZvidDdC[0x25] =
    "D:\\Proj\\GameZRecoil\\zVideo\\zvid_dd.c";
/**
 * Reimplements data 0x4e3110: g_zVideo_UnrecognizedPixelFormatMsg.
 * BN types this writable char[0x1a] as the zvid_dd.c diagnostic passed only by
 * zVideo_dd::InitFullscreenSoftwarePixelPack for an unsupported pixel format.
 * Purpose: Supplies the DirectDraw software pixel-pack failure message.
 */
char g_zVideo_UnrecognizedPixelFormatMsg[0x1a] =
    "Unrecognized pixel format";
/**
 * Reimplements data 0x4e312c..0x4e329c:
 * render_video.zvideo_dd_enumeration_diagnostic_literals.
 * Purpose: writable DirectDraw/Direct3D enumeration diagnostic strings used by
 * zvid_dd.c startup enumeration and callback logging.
 *
 * BN types these adjacent rows as writable char arrays in the order below.
 * The neighboring zvid_dd.c source path at 0x4e30e8 and pixel-format message
 * at 0x4e3110 are separate accepted owners, and the later GameZ fallback
 * literal at 0x4e32ac is intentionally not part of this owner.
 */
char g_zVideo_DDrawEnumBeginMsg[0x20] =
    "\nENUMERATE GRAPHICS DEVICES...\n";
char g_zVideo_DDrawEnumAgpSuffix[0x6] =
    "[AGP]";
char g_zVideo_DDrawEnumTooManyDevicesMsg[0x34] =
    "\nCan't handle this many devices - IGNORING THE REST";
char g_zVideo_DDrawEnumDevicePrintfFmt[0x17] =
    "\n%d: Device [%s] - %s\n";
char g_zVideo_D3DEnumNoUsableDriversMsg[0x14] =
    "No useable drivers\n";
char g_zVideo_D3DEnumBeginMsgFmt[0x1c] =
    "\nENUMERATE DRIVERS (%s)...\n";
char g_zVideo_D3DEnumAcceptedMsg[0x9] =
    "+++++OK\n";
char g_zVideo_D3DEnumTooManyDriversMsg[0x2c] =
    "Maximum number of Direct3D drivers exceeded";
char g_zVideo_D3DEnumSkipNo16BitZBufferMsg[0x31] =
    "-----SKIPPED - Does not support 16-bit Z buffer\n";
char g_zVideo_D3DEnumSkipNoRgbColorMsg[0x2b] =
    "-----SKIPPED - Does not support RGB color\n";
char g_zVideo_D3DEnumSkipNoHardwareMsg[0x31] =
    "-----SKIPPED - Does not interface with hardware\n";
char g_zVideo_D3DEnumDriverPrintfFmt[0x10] =
    "DRIVER:%s - %s\n";
/**
 * Reimplements data 0x4e32ac: g_zVideo_DefaultD3DDeviceName.
 * BN types this writable char[0x6] as the zvid_ddd3d.c fallback string
 * returned when no Direct3D device is selected.
 * Purpose: supply the default Direct3D device name for zVid callers.
 */
char g_zVideo_DefaultD3DDeviceName[0x6] = "GameZ";
RECOIL_STATIC_ASSERT(sizeof(g_zVid_DefaultImageTexturePackReadonlyNameFmt) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_DefaultImageTexturePackName) == 0x0a);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveNameFmt) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveSizedNameFmt) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveMaxName) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveSize2Fmt) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveSize4Fmt) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveSize6Fmt) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveSize8Fmt) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveRendererSizedNameFmt) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(g_zVid_TextureArchiveStem) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_SourceFile_ZvidBuffC) == 0x27);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_InitFailSetModeMsg) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_SourceFile_ZvidInitC) == 0x27);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_InitFailOpenVideoModeMsg) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_SourceFile_ZvidDdC) == 0x25);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_UnrecognizedPixelFormatMsg) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDrawEnumBeginMsg) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDrawEnumAgpSuffix) == 0x6);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDrawEnumTooManyDevicesMsg) == 0x34);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDrawEnumDevicePrintfFmt) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumNoUsableDriversMsg) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumBeginMsgFmt) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumAcceptedMsg) == 0x9);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumTooManyDriversMsg) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumSkipNo16BitZBufferMsg) == 0x31);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumSkipNoRgbColorMsg) == 0x2b);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumSkipNoHardwareMsg) == 0x31);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DEnumDriverPrintfFmt) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DefaultD3DDeviceName) == 0x6);
/**
 * Reimplements data 0x53d778: g_zVid_PaletteRemapVariantTableCount.
 * Reimplements data 0x53d77c: g_zVid_PaletteRemapVariantTables.
 * Standalone palette-remap variant table owner: BN identifies the table count
 * and pointer array at 0x53d778/0x53d77c.
 * Purpose: cache standalone palette-remap variant tables.
 */
int g_zVid_PaletteRemapVariantTableCount = 0;
unsigned short **g_zVid_PaletteRemapVariantTables = 0;
/**
 * Reimplements data 0x4e5d30: ZOPT_VIDEO_MODE.
 * Reimplements data 0x4e5d58: ZOPT_VIDEO_ACCELERATION.
 * Reimplements data 0x4e5d5c: ZOPT_HW_API.
 * Purpose: point at the video option storage used by zVid option accessors.
 */
int *ZOPT_VIDEO_MODE = 0;
int *ZOPT_VIDEO_ACCELERATION = 0;
int *ZOPT_HW_API = 0;
/**
 * Reimplements data 0x6359f8: g_zVideo_DDrawCapsHal.
 * Reimplements data 0x635b74: g_zVideo_DDrawCapsHel.
 * DirectDraw enumeration capability scratch buffers. EnumDirectDrawDeviceCallback
 * clears these zero-initialized 0x17c-byte provider records, sets dwSize, and
 * passes them to IDirectDraw2::GetCaps before accepting a hardware API record.
 * Purpose: hold HAL and HEL DirectDraw capability snapshots during enumeration.
 */
DDCAPS g_zVideo_DDrawCapsHal = {0};
DDCAPS g_zVideo_DDrawCapsHel = {0};
/*
 * zVideo hardware default texture owner: BN 0x4a75f0 passes this separate
 * 8x8 four-color checker image directly to the active texture-record callback
 * with a null texture name and zero flags.
 */
unsigned short g_zVideo_DefaultTexturePixels[64] = {
    0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3,
    0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800,
    0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0,
    0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f,
    0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3,
    0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800,
    0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0,
    0x38e3, 0xf800, 0x03e0, 0x001f, 0x38e3, 0xf800, 0x03e0, 0x001f
};
zVidImagePartial g_zVideo_DefaultTextureImage = {
    64,
    8,
    8,
    0,
    0,
    0,
    0,
    0,
    0,
    g_zVideo_DefaultTexturePixels,
    0,
    0,
    0.0f,
    0,
    0,
    0,
    0,
    0,
    0
};
/**
 * Reimplements data 0x4e3370: g_zVideo_OpaqueWhiteArgb.
 * Purpose: provide the Direct3D TL-vertex color used by the opaque textured
 * submit path.
 */
unsigned int g_zVideo_OpaqueWhiteArgb = 0xffffffffu;
/**
 * Reimplements data 0x4e3374..0x4e351c:
 * render_video.zvideo_dd3d_diagnostic_strings_data.
 * Purpose: writable zvid_ddd3d.c diagnostic source path and format strings
 * passed directly to zError and zVideo_dd error reporting.
 *
 * BN types these adjacent rows as writable char arrays in the order below.
 * They sit after g_zVideo_OpaqueWhiteArgb and before later Direct3D texture
 * runtime data; they are authored data, not provider literals.
 */
char g_zVideo_SourceFile_ZvidDdd3dC[0x28] =
    "D:\\Proj\\GameZRecoil\\zVideo\\zvid_ddd3d.c";
char g_zVideo_TextureTooLargeUsingDefaultFmt[0x49] =
    "Texture [%s] dimensions [%d x %d] are too large.  Using default texture.";
char g_zVideo_TextureBadAspectUsingDefaultFmt[0x4f] =
    "Texture [%s] dimensions [%d x %d] have bad aspect ratio.Using default texture.";
char g_zVideo_TexturePaletteUnsupportedUsingDefaultFmt[0x3c] =
    "Texture [%s] Palettes not supported  Using default texture.";
char g_zVideo_TextureNotPowerOf2UsingDefaultFmt[0x4c] =
    "Texture [%s] dimensions [%d x %d] are not power of 2.Using default texture.";
char g_zVideo_NotEnoughMaxTransparentPolysFmt[0x2a] =
    "Not enough MAX_TRANSPARENT_POLYS: need %d";
char g_zVideo_NotEnoughMaxOverwritePolysNeedFmt[0x2d] =
    "Not enough ZVID_MAX_OVERWRITE_POLYS: need %d";
char g_zVideo_NotEnoughMaxOverwritePolysNeedsFmt[0x2e] =
    "Not enough ZVID_MAX_OVERWRITE_POLYS: needs %d";
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_SourceFile_ZvidDdd3dC) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_TextureTooLargeUsingDefaultFmt) == 0x49);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_TextureBadAspectUsingDefaultFmt) == 0x4f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_TexturePaletteUnsupportedUsingDefaultFmt) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_TextureNotPowerOf2UsingDefaultFmt) == 0x4c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_NotEnoughMaxTransparentPolysFmt) == 0x2a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_NotEnoughMaxOverwritePolysNeedFmt) == 0x2d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_NotEnoughMaxOverwritePolysNeedsFmt) == 0x2e);
/**
 * Reimplements data 0x4e354c..0x4e42d0:
 * render_video.zvideo_dd_report_error_diagnostic_strings_data.
 * Purpose: writable DirectDraw/Direct3D HRESULT names and report format used
 * by zVideo_dd::ReportError.
 *
 * BN types the ReportError diagnostic pool as adjacent writable char arrays.
 * The shared "Unknown Error" fallback at 0x4dcaac is a separate owner and is
 * not part of this range.
 */
char g_zVideo_DirectDrawErrorFmt[0x1d] =
    "DirectDraw Error [%s] %s:%d\n";
char g_zVideo_D3DErrorName_ViewportDataNotSet[0x1a] =
    "D3DERR_VIEWPORTDATANOTSET";
char g_zVideo_D3DErrorName_SceneNotInScene[0x1a] =
    "D3DERR_SCENE_NOT_IN_SCENE";
char g_zVideo_D3DErrorName_SceneInScene[0x16] =
    "D3DERR_SCENE_IN_SCENE";
char g_zVideo_D3DErrorName_SceneEndFailed[0x18] =
    "D3DERR_SCENE_END_FAILED";
char g_zVideo_D3DErrorName_SceneBeginFailed[0x1a] =
    "D3DERR_SCENE_BEGIN_FAILED";
char g_zVideo_D3DErrorName_NoViewports[0x13] =
    "D3DERR_NOVIEWPORTS";
char g_zVideo_D3DErrorName_NotInBegin[0x12] =
    "D3DERR_NOTINBEGIN";
char g_zVideo_D3DErrorName_InBegin[0x0f] =
    "D3DERR_INBEGIN";
char g_zVideo_D3DErrorName_LightSetFailed[0x18] =
    "D3DERR_LIGHT_SET_FAILED";
char g_zVideo_D3DErrorName_ZBuffNeedsVideoMemory[0x1f] =
    "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY";
char g_zVideo_D3DErrorName_ZBuffNeedsSystemMemory[0x20] =
    "D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY";
char g_zVideo_D3DErrorName_TextureUnlockFailed[0x1d] =
    "D3DERR_TEXTURE_UNLOCK_FAILED";
char g_zVideo_D3DErrorName_TextureSwapFailed[0x1b] =
    "D3DERR_TEXTURE_SWAP_FAILED";
char g_zVideo_D3DErrorName_TextureNotLocked[0x1a] =
    "D3DERR_TEXTURE_NOT_LOCKED";
char g_zVideo_D3DErrorName_TextureNoSupport[0x1a] =
    "D3DERR_TEXTURE_NO_SUPPORT";
char g_zVideo_D3DErrorName_TextureLocked[0x16] =
    "D3DERR_TEXTURE_LOCKED";
char g_zVideo_D3DErrorName_TextureLockFailed[0x1b] =
    "D3DERR_TEXTURE_LOCK_FAILED";
char g_zVideo_D3DErrorName_TextureLoadFailed[0x1b] =
    "D3DERR_TEXTURE_LOAD_FAILED";
char g_zVideo_D3DErrorName_TextureGetSurfFailed[0x1e] =
    "D3DERR_TEXTURE_GETSURF_FAILED";
char g_zVideo_D3DErrorName_TextureDestroyFailed[0x1e] =
    "D3DERR_TEXTURE_DESTROY_FAILED";
char g_zVideo_D3DErrorName_TextureCreateFailed[0x1d] =
    "D3DERR_TEXTURE_CREATE_FAILED";
char g_zVideo_D3DErrorName_TextureBadSize[0x17] =
    "D3DERR_TEXTURE_BADSIZE";
char g_zVideo_D3DErrorName_SetViewportDataFailed[0x1e] =
    "D3DERR_SETVIEWPORTDATA_FAILED";
char g_zVideo_D3DErrorName_MatrixSetDataFailed[0x1d] =
    "D3DERR_MATRIX_SETDATA_FAILED";
char g_zVideo_D3DErrorName_MatrixGetDataFailed[0x1d] =
    "D3DERR_MATRIX_GETDATA_FAILED";
char g_zVideo_D3DErrorName_MatrixDestroyFailed[0x1d] =
    "D3DERR_MATRIX_DESTROY_FAILED";
char g_zVideo_D3DErrorName_MatrixCreateFailed[0x1c] =
    "D3DERR_MATRIX_CREATE_FAILED";
char g_zVideo_D3DErrorName_MaterialSetDataFailed[0x1f] =
    "D3DERR_MATERIAL_SETDATA_FAILED";
char g_zVideo_D3DErrorName_MaterialGetDataFailed[0x1f] =
    "D3DERR_MATERIAL_GETDATA_FAILED";
char g_zVideo_D3DErrorName_MaterialDestroyFailed[0x1f] =
    "D3DERR_MATERIAL_DESTROY_FAILED";
char g_zVideo_D3DErrorName_MaterialCreateFailed[0x1e] =
    "D3DERR_MATERIAL_CREATE_FAILED";
char g_zVideo_D3DErrorName_InvalidVertexType[0x19] =
    "D3DERR_INVALIDVERTEXTYPE";
char g_zVideo_D3DErrorName_InvalidPrimitiveType[0x1c] =
    "D3DERR_INVALIDPRIMITIVETYPE";
char g_zVideo_D3DErrorName_InvalidCurrentViewport[0x1e] =
    "D3DERR_INVALIDCURRENTVIEWPORT";
char g_zVideo_D3DErrorName_ExecuteUnlockFailed[0x1d] =
    "D3DERR_EXECUTE_UNLOCK_FAILED";
char g_zVideo_D3DErrorName_ExecuteNotLocked[0x1a] =
    "D3DERR_EXECUTE_NOT_LOCKED";
char g_zVideo_D3DErrorName_ExecuteLocked[0x16] =
    "D3DERR_EXECUTE_LOCKED";
char g_zVideo_D3DErrorName_ExecuteLockFailed[0x1b] =
    "D3DERR_EXECUTE_LOCK_FAILED";
char g_zVideo_D3DErrorName_ExecuteFailed[0x16] =
    "D3DERR_EXECUTE_FAILED";
char g_zVideo_D3DErrorName_ExecuteDestroyFailed[0x1e] =
    "D3DERR_EXECUTE_DESTROY_FAILED";
char g_zVideo_D3DErrorName_ExecuteCreateFailed[0x1d] =
    "D3DERR_EXECUTE_CREATE_FAILED";
char g_zVideo_D3DErrorName_ExecuteClippedFailed[0x1e] =
    "D3DERR_EXECUTE_CLIPPED_FAILED";
char g_zVideo_D3DErrorName_InvalidDevice[0x16] =
    "D3DERR_INVALID_DEVICE";
char g_zVideo_D3DErrorName_BadMajorVersion[0x17] =
    "D3DERR_BADMAJORVERSION";
char g_zVideo_D3DErrorName_BadMinorVersion[0x17] =
    "D3DERR_BADMINORVERSION";
char g_zVideo_DDErrorName_NotPageLocked[0x14] =
    "DDERR_NOTPAGELOCKED";
char g_zVideo_DDErrorName_CantPageUnlock[0x15] =
    "DDERR_CANTPAGEUNLOCK";
char g_zVideo_DDErrorName_CantPageLock[0x13] =
    "DDERR_CANTPAGELOCK";
char g_zVideo_DDErrorName_XAlign[0x0d] =
    "DDERR_XALIGN";
char g_zVideo_DDErrorName_WrongMode[0x10] =
    "DDERR_WRONGMODE";
char g_zVideo_DDErrorName_UnsupportedMode[0x16] =
    "DDERR_UNSUPPORTEDMODE";
char g_zVideo_DDErrorName_RegionTooSmall[0x15] =
    "DDERR_REGIONTOOSMALL";
char g_zVideo_DDErrorName_PrimarySurfaceAlreadyExists[0x22] =
    "DDERR_PRIMARYSURFACEALREADYEXISTS";
char g_zVideo_DDErrorName_OverlayNotVisible[0x18] =
    "DDERR_OVERLAYNOTVISIBLE";
char g_zVideo_DDErrorName_NotPalettized[0x14] =
    "DDERR_NOTPALETTIZED";
char g_zVideo_DDErrorName_NotLocked[0x10] =
    "DDERR_NOTLOCKED";
char g_zVideo_DDErrorName_NotFlippable[0x13] =
    "DDERR_NOTFLIPPABLE";
char g_zVideo_DDErrorName_NoAOverlaySurface[0x18] =
    "DDERR_NOAOVERLAYSURFACE";
char g_zVideo_DDErrorName_NoPaletteHw[0x12] =
    "DDERR_NOPALETTEHW";
char g_zVideo_DDErrorName_NoPaletteAttached[0x18] =
    "DDERR_NOPALETTEATTACHED";
char g_zVideo_DDErrorName_NoMipMapHw[0x11] =
    "DDERR_NOMIPMAPHW";
char g_zVideo_DDErrorName_NoHwnd[0x0d] =
    "DDERR_NOHWND";
char g_zVideo_DDErrorName_NoEmulation[0x12] =
    "DDERR_NOEMULATION";
char g_zVideo_DDErrorName_NoDirectDrawHw[0x15] =
    "DDERR_NODIRECTDRAWHW";
char g_zVideo_DDErrorName_NoDdRopsHw[0x11] =
    "DDERR_NODDROPSHW";
char g_zVideo_DDErrorName_NoDirectDc[0x11] =
    "DDERR_NODIRECTDC";
char g_zVideo_DDErrorName_NoClipperAttached[0x18] =
    "DDERR_NOCLIPPERATTACHED";
char g_zVideo_DDErrorName_NoBltHw[0x0e] =
    "DDERR_NOBLTHW";
char g_zVideo_DDErrorName_InvalidSurfaceType[0x19] =
    "DDERR_INVALIDSURFACETYPE";
char g_zVideo_DDErrorName_InvalidPosition[0x16] =
    "DDERR_INVALIDPOSITION";
char g_zVideo_DDErrorName_InvalidDirectDrawGuid[0x1c] =
    "DDERR_INVALIDDIRECTDRAWGUID";
char g_zVideo_DDErrorName_ImplicitlyCreated[0x18] =
    "DDERR_IMPLICITLYCREATED";
char g_zVideo_DDErrorName_HwndSubclassed[0x15] =
    "DDERR_HWNDSUBCLASSED";
char g_zVideo_DDErrorName_HwndAlreadySet[0x15] =
    "DDERR_HWNDALREADYSET";
char g_zVideo_DDErrorName_ExclusiveModeAlreadySet[0x1e] =
    "DDERR_EXCLUSIVEMODEALREADYSET";
char g_zVideo_DDErrorName_DirectDrawAlreadyCreated[0x1f] =
    "DDERR_DIRECTDRAWALREADYCREATED";
char g_zVideo_DDErrorName_DcAlreadyCreated[0x17] =
    "DDERR_DCALREADYCREATED";
char g_zVideo_DDErrorName_ClipperIsUsingHwnd[0x19] =
    "DDERR_CLIPPERISUSINGHWND";
char g_zVideo_DDErrorName_CantDuplicate[0x14] =
    "DDERR_CANTDUPLICATE";
char g_zVideo_DDErrorName_CantCreateDc[0x13] =
    "DDERR_CANTCREATEDC";
char g_zVideo_DDErrorName_BltFastCantClip[0x16] =
    "DDERR_BLTFASTCANTCLIP";
char g_zVideo_DDErrorName_WasStillDrawing[0x16] =
    "DDERR_WASSTILLDRAWING";
char g_zVideo_DDErrorName_VerticalBlankInProgress[0x1e] =
    "DDERR_VERTICALBLANKINPROGRESS";
char g_zVideo_DDErrorName_UnsupportedMask[0x16] =
    "DDERR_UNSUPPORTEDMASK";
char g_zVideo_DDErrorName_UnsupportedFormat[0x18] =
    "DDERR_UNSUPPORTEDFORMAT";
char g_zVideo_DDErrorName_TooBigWidth[0x12] =
    "DDERR_TOOBIGWIDTH";
char g_zVideo_DDErrorName_TooBigSize[0x11] =
    "DDERR_TOOBIGSIZE";
char g_zVideo_DDErrorName_TooBigHeight[0x13] =
    "DDERR_TOOBIGHEIGHT";
char g_zVideo_DDErrorName_SurfaceNotAttached[0x19] =
    "DDERR_SURFACENOTATTACHED";
char g_zVideo_DDErrorName_SurfaceLost[0x12] =
    "DDERR_SURFACELOST";
char g_zVideo_DDErrorName_SurfaceIsObscured[0x18] =
    "DDERR_SURFACEISOBSCURED";
char g_zVideo_DDErrorName_CantLockSurface[0x16] =
    "DDERR_CANTLOCKSURFACE";
char g_zVideo_DDErrorName_SurfaceBusy[0x12] =
    "DDERR_SURFACEBUSY";
char g_zVideo_DDErrorName_SurfaceAlreadyDependent[0x1e] =
    "DDERR_SURFACEALREADYDEPENDENT";
char g_zVideo_DDErrorName_SurfaceAlreadyAttached[0x1d] =
    "DDERR_SURFACEALREADYATTACHED";
char g_zVideo_DDErrorName_ColorKeyNotSet[0x15] =
    "DDERR_COLORKEYNOTSET";
char g_zVideo_DDErrorName_OverlayCantClip[0x16] =
    "DDERR_OVERLAYCANTCLIP";
char g_zVideo_DDErrorName_OverlayColorKeyOnlyOneActive[0x23] =
    "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
char g_zVideo_DDErrorName_PaletteBusy[0x12] =
    "DDERR_PALETTEBUSY";
char g_zVideo_DDErrorName_OutOfVideoMemory[0x17] =
    "DDERR_OUTOFVIDEOMEMORY";
char g_zVideo_DDErrorName_OutOfCaps[0x10] =
    "DDERR_OUTOFCAPS";
char g_zVideo_DDErrorName_NoZOverlayHw[0x13] =
    "DDERR_NOZOVERLAYHW";
char g_zVideo_DDErrorName_NoZBufferHw[0x12] =
    "DDERR_NOZBUFFERHW";
char g_zVideo_DDErrorName_NoVSyncHw[0x10] =
    "DDERR_NOVSYNCHW";
char g_zVideo_DDErrorName_NoTextureHw[0x12] =
    "DDERR_NOTEXTUREHW";
char g_zVideo_DDErrorName_Not8BitColor[0x13] =
    "DDERR_NOT8BITCOLOR";
char g_zVideo_DDErrorName_Not4BitColorIndex[0x18] =
    "DDERR_NOT4BITCOLORINDEX";
char g_zVideo_DDErrorName_Not4BitColor[0x13] =
    "DDERR_NOT4BITCOLOR";
char g_zVideo_DDErrorName_NoStretchHw[0x12] =
    "DDERR_NOSTRETCHHW";
char g_zVideo_DDErrorName_NoRotationHw[0x13] =
    "DDERR_NOROTATIONHW";
char g_zVideo_DDErrorName_NoRasterOpHw[0x13] =
    "DDERR_NORASTEROPHW";
char g_zVideo_DDErrorName_NoOverlayHw[0x12] =
    "DDERR_NOOVERLAYHW";
char g_zVideo_DDErrorName_NotFound[0x0f] =
    "DDERR_NOTFOUND";
char g_zVideo_DDErrorName_NoMirrorHw[0x11] =
    "DDERR_NOMIRRORHW";
char g_zVideo_DDErrorName_NoGdi[0x0c] =
    "DDERR_NOGDI";
char g_zVideo_DDErrorName_NoFlipHw[0x0f] =
    "DDERR_NOFLIPHW";
char g_zVideo_DDErrorName_NoColorKeyHw[0x13] =
    "DDERR_NOCOLORKEYHW";
char g_zVideo_DDErrorName_NoDirectDrawSupport[0x1a] =
    "DDERR_NODIRECTDRAWSUPPORT";
char g_zVideo_DDErrorName_NoExclusiveMode[0x16] =
    "DDERR_NOEXCLUSIVEMODE";
char g_zVideo_DDErrorName_NoColorKey[0x11] =
    "DDERR_NOCOLORKEY";
char g_zVideo_DDErrorName_NoCooperativeLevelSet[0x1c] =
    "DDERR_NOCOOPERATIVELEVELSET";
char g_zVideo_DDErrorName_NoColorConvHw[0x14] =
    "DDERR_NOCOLORCONVHW";
char g_zVideo_DDErrorName_NoClipList[0x11] =
    "DDERR_NOCLIPLIST";
char g_zVideo_DDErrorName_NoAlphaHw[0x10] =
    "DDERR_NOALPHAHW";
char g_zVideo_DDErrorName_No3d[0x0b] =
    "DDERR_NO3D";
char g_zVideo_DDErrorName_LockedSurfaces[0x15] =
    "DDERR_LOCKEDSURFACES";
char g_zVideo_DDErrorName_InvalidRect[0x12] =
    "DDERR_INVALIDRECT";
char g_zVideo_DDErrorName_InvalidPixelFormat[0x19] =
    "DDERR_INVALIDPIXELFORMAT";
char g_zVideo_DDErrorName_InvalidObject[0x14] =
    "DDERR_INVALIDOBJECT";
char g_zVideo_DDErrorName_InvalidMode[0x12] =
    "DDERR_INVALIDMODE";
char g_zVideo_DDErrorName_InvalidClipList[0x16] =
    "DDERR_INVALIDCLIPLIST";
char g_zVideo_DDErrorName_InvalidCaps[0x12] =
    "DDERR_INVALIDCAPS";
char g_zVideo_DDErrorName_HeightAlign[0x12] =
    "DDERR_HEIGHTALIGN";
char g_zVideo_DDErrorName_Exception[0x10] =
    "DDERR_EXCEPTION";
char g_zVideo_DDErrorName_CurrentlyNotAvail[0x18] =
    "DDERR_CURRENTLYNOTAVAIL";
char g_zVideo_DDErrorName_CannotDetachSurface[0x1a] =
    "DDERR_CANNOTDETACHSURFACE";
char g_zVideo_DDErrorName_CannotAttachSurface[0x1a] =
    "DDERR_CANNOTATTACHSURFACE";
char g_zVideo_DDErrorName_AlreadyInitialized[0x19] =
    "DDERR_ALREADYINITIALIZED";
char g_zVideo_DDErrorName_InvalidParams[0x14] =
    "DDERR_INVALIDPARAMS";
char g_zVideo_DDErrorName_OutOfMemory[0x12] =
    "DDERR_OUTOFMEMORY";
char g_zVideo_DDErrorName_NotInitialized[0x15] =
    "DDERR_NOTINITIALIZED";
char g_zVideo_DDErrorName_Generic[0x0e] =
    "DDERR_GENERIC";
char g_zVideo_DDErrorName_Unsupported[0x12] =
    "DDERR_UNSUPPORTED";
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DirectDrawErrorFmt) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ViewportDataNotSet) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_SceneNotInScene) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_SceneInScene) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_SceneEndFailed) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_SceneBeginFailed) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_NoViewports) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_NotInBegin) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_InBegin) == 0x0f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_LightSetFailed) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ZBuffNeedsVideoMemory) == 0x1f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ZBuffNeedsSystemMemory) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureUnlockFailed) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureSwapFailed) == 0x1b);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureNotLocked) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureNoSupport) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureLocked) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureLockFailed) == 0x1b);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureLoadFailed) == 0x1b);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureGetSurfFailed) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureDestroyFailed) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureCreateFailed) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_TextureBadSize) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_SetViewportDataFailed) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MatrixSetDataFailed) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MatrixGetDataFailed) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MatrixDestroyFailed) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MatrixCreateFailed) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MaterialSetDataFailed) == 0x1f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MaterialGetDataFailed) == 0x1f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MaterialDestroyFailed) == 0x1f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_MaterialCreateFailed) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_InvalidVertexType) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_InvalidPrimitiveType) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_InvalidCurrentViewport) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteUnlockFailed) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteNotLocked) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteLocked) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteLockFailed) == 0x1b);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteFailed) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteDestroyFailed) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteCreateFailed) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_ExecuteClippedFailed) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_InvalidDevice) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_BadMajorVersion) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_D3DErrorName_BadMinorVersion) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NotPageLocked) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CantPageUnlock) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CantPageLock) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_XAlign) == 0x0d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_WrongMode) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_UnsupportedMode) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_RegionTooSmall) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_PrimarySurfaceAlreadyExists) == 0x22);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_OverlayNotVisible) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NotPalettized) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NotLocked) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NotFlippable) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoAOverlaySurface) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoPaletteHw) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoPaletteAttached) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoMipMapHw) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoHwnd) == 0x0d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoEmulation) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoDirectDrawHw) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoDdRopsHw) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoDirectDc) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoClipperAttached) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoBltHw) == 0x0e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidSurfaceType) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidPosition) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidDirectDrawGuid) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_ImplicitlyCreated) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_HwndSubclassed) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_HwndAlreadySet) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_ExclusiveModeAlreadySet) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_DirectDrawAlreadyCreated) == 0x1f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_DcAlreadyCreated) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_ClipperIsUsingHwnd) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CantDuplicate) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CantCreateDc) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_BltFastCantClip) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_WasStillDrawing) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_VerticalBlankInProgress) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_UnsupportedMask) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_UnsupportedFormat) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_TooBigWidth) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_TooBigSize) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_TooBigHeight) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_SurfaceNotAttached) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_SurfaceLost) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_SurfaceIsObscured) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CantLockSurface) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_SurfaceBusy) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_SurfaceAlreadyDependent) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_SurfaceAlreadyAttached) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_ColorKeyNotSet) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_OverlayCantClip) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_OverlayColorKeyOnlyOneActive) == 0x23);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_PaletteBusy) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_OutOfVideoMemory) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_OutOfCaps) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoZOverlayHw) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoZBufferHw) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoVSyncHw) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoTextureHw) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_Not8BitColor) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_Not4BitColorIndex) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_Not4BitColor) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoStretchHw) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoRotationHw) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoRasterOpHw) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoOverlayHw) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NotFound) == 0x0f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoMirrorHw) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoGdi) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoFlipHw) == 0x0f);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoColorKeyHw) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoDirectDrawSupport) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoExclusiveMode) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoColorKey) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoCooperativeLevelSet) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoColorConvHw) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoClipList) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NoAlphaHw) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_No3d) == 0x0b);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_LockedSurfaces) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidRect) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidPixelFormat) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidObject) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidMode) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidClipList) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidCaps) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_HeightAlign) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_Exception) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CurrentlyNotAvail) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CannotDetachSurface) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_CannotAttachSurface) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_AlreadyInitialized) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_InvalidParams) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_OutOfMemory) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_NotInitialized) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_Generic) == 0x0e);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_DDErrorName_Unsupported) == 0x12);
/**
 * Reimplements data 0x6333a8: g_zVideo_DefaultTextureRecord.
 * Data owner: render_video.zvideo_default_texture_record_runtime.
 * Purpose: hold the hardware default texture record created by zVideo startup
 * and used by zVideo Direct3D texture fallback/destruction paths.
 *
 * Retail 0x6333a8: zero-initialized zVideo runtime pointer. BN currently
 * names this storage g_zImage_DefaultTextureRecord, but xrefs are only from
 * zVideo::InitVideoSystem, zVideo_dd::ShutdownVideoSystem, and zVideo_dd3d
 * texture-record helpers. The zImage texture-directory pointer at 0x4e071c is
 * separate storage owned by zImage::InitTextureDirectory.
 */
zVideo_TextureRecordPartial *g_zVideo_DefaultTextureRecord = 0;
/**
 * Reimplements data 0x4e5b28: g_zVideo_PaletteOpenFailedFormat.
 * Data owner: render_video.zvideo_palette_brightness_runtime.
 * Purpose: supplies the palette-open diagnostic format used before the
 * palette loader returns its failure code.
 */
char g_zVideo_PaletteOpenFailedFormat[0x21] =
    "ZVID: could not open palette %s\n";
char g_zVideo_PalettePathBuffer[0x100] = {0};
/**
 * Reimplements data 0x632360: g_zVideo_PaletteBrightnessLevel.
 * Purpose: cache the palette brightness adjustment used by palette loading.
 */
int g_zVideo_PaletteBrightnessLevel = 0;
PALETTEENTRY g_zVideo_PaletteFileEntries[0x100] = {0};
PALETTEENTRY g_zVideo_SystemPaletteEntries[0x100] = {0};
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_PaletteOpenFailedFormat) == 0x21);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_PaletteBrightnessLevel) == 4);

/**
 * Reimplements data 0x6333ac: g_zVideo_pfnOpenVideoMode.
 * Reimplements data 0x6333b0: g_zVideo_pfnShutdownVideoSystem.
 * Reimplements data 0x6333b8: g_zVideo_pfnPaletteSetEntries.
 * Reimplements data 0x6333bc: g_zVideo_pfnSetVideoMode.
 * Reimplements data 0x6333b4: g_zVideo_pfnAdjustSurfaces.
 * Reimplements data 0x6333c4: g_zVideo_pfnLockSurfaceState.
 * Reimplements data 0x6333c0: g_zVideo_pfnUnlockSurfaceState.
 * Reimplements data 0x6333d0: g_zVideo_pfnClearZBufferRect.
 * Reimplements data 0x6333cc: g_zVideo_pfnClearSwSurfaceAndZBuffer.
 * Reimplements data 0x6333c8: g_zVideo_pfnClearStateSurfaceAndZBuffer.
 * Reimplements data 0x6333d4: g_zVideo_pfnUpdateFogColor.
 * Reimplements data 0x56bc2c: g_zVideo_pfnQueryDeviceVideoMemoryBytes.
 * Reimplements data 0x56bc30: g_zVideo_pfnQueryTextureMemoryBytes.
 * Reimplements data 0x56bbfc: g_zVideo_pfnBltSwToPrimaryRectDirect.
 * Reimplements data 0x56bc00: g_zVideo_pfnBltPrimaryToSwRectDirect.
 * Reimplements data 0x56bc04: g_zVideo_pfnBltSwToPrimaryRect.
 * Reimplements data 0x56bc34: g_zVideo_pfnGetHwApiDeviceFeatureFlags.
 * Reimplements data 0x56bc38: g_zVideo_pfnImageUploadPixelsToSurface.
 * Reimplements data 0x56bc3c: g_zVideo_pfnImageReleaseSurface.
 * Reimplements data 0x56bc08: g_zVideo_pfnCreateTextureRecord.
 * Reimplements data 0x56bc0c: g_zVideo_pfnTextureRecordLockUploadSurface.
 * Reimplements data 0x56bc10: g_zVideo_pfnTextureRecordUnlockUploadSurface.
 * Reimplements data 0x56bc14: g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef.
 * Renderer dispatch owner: BN 0x4a77a0 initializes this backend function
 * pointer set from zVideo_dd/zVideo_dd3d entry points and leaves the globals
 * zero-initialized before BindRendererDispatch runs.
 * Purpose: hold the active renderer dispatch vector.
 */
zVideo_StatusProc g_zVideo_pfnOpenVideoMode = 0;
zVideo_ShutdownVideoSystemProc g_zVideo_pfnShutdownVideoSystem = 0;
zVideo_PaletteSetEntriesProc g_zVideo_pfnPaletteSetEntries = 0;
zVideo_StatusProc g_zVideo_pfnSetVideoMode = 0;
zVideo_AdjustSurfacesProc g_zVideo_pfnAdjustSurfaces = 0;
zVideo_SurfaceStateProc g_zVideo_pfnLockSurfaceState = 0;
zVideo_SurfaceStateProc g_zVideo_pfnUnlockSurfaceState = 0;
zVideo_ClearZBufferRectProc g_zVideo_pfnClearZBufferRect = 0;
zVideo_ClearSwSurfaceAndZBufferProc g_zVideo_pfnClearSwSurfaceAndZBuffer = 0;
zVideo_ClearStateSurfaceAndZBufferProc g_zVideo_pfnClearStateSurfaceAndZBuffer = 0;
zVideo_UpdateFogColorProc g_zVideo_pfnUpdateFogColor = 0;
zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryDeviceVideoMemoryBytes = 0;
zVideo_QueryMemoryBytesProc g_zVideo_pfnQueryTextureMemoryBytes = 0;
zVideo_BltRectDirectProc g_zVideo_pfnBltSwToPrimaryRectDirect = 0;
zVideo_BltRectDirectProc g_zVideo_pfnBltPrimaryToSwRectDirect = 0;
zVideo_BltImageRectProc g_zVideo_pfnBltSwToPrimaryRect = 0;
zVideo_GetHwApiDeviceFeatureFlagsProc g_zVideo_pfnGetHwApiDeviceFeatureFlags = 0;
zVideo_ImageUploadPixelsProc g_zVideo_pfnImageUploadPixelsToSurface = 0;
zVideo_ImageReleaseSurfaceProc g_zVideo_pfnImageReleaseSurface = 0;
zVideo_CreateTextureRecordProc g_zVideo_pfnCreateTextureRecord = 0;
zVideo_TextureRecordLockUploadSurfaceProc g_zVideo_pfnTextureRecordLockUploadSurface = 0;
zVideo_TextureRecordUnlockUploadSurfaceProc g_zVideo_pfnTextureRecordUnlockUploadSurface = 0;
zVideo_TextureRecordReleaseUploadSurfaceRefProc
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef = 0;
/**
 * Reimplements data 0x56bc18: g_zVideo_pfnTextureRecordFinalizeUpload.
 * Reimplements data 0x56bc1c: g_zVideo_pfnTextureRecordDestroy.
 * Reimplements data 0x56bc20: g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces.
 * Purpose: dispatch texture-record upload finalization, destruction, and
 * upload-surface release hooks.
 */
zVideo_TextureRecordFinalizeUploadProc g_zVideo_pfnTextureRecordFinalizeUpload = 0;
zVideo_DestroyTextureRecordProc g_zVideo_pfnTextureRecordDestroy = 0;
zVideo_ReleaseAllTextureUploadSurfacesProc
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = 0;
/**
 * Reimplements data 0x56bc24: g_zVideo_pfnImageLazyCreateVideoMemorySurface.
 * Reimplements data 0x56bc28: g_zVideo_pfnImageEnsureSurfaceForCurrentDevice.
 * Reimplements data 0x56bc40: g_zVideo_pfnSetFogEnable.
 * Reimplements data 0x56bc44: g_zVideo_pfnSetFogStart.
 * Reimplements data 0x56bc48: g_zVideo_pfnSetFogEnd.
 * Reimplements data 0x56bc4c: g_zVideo_pfnApplyFogStateFromGlobals.
 * Reimplements data 0x56bc50: g_zVideo_pfnSubmitPolyFlatColor16.
 * Reimplements data 0x56bc54: g_zVideo_pfnSubmitPolyGouraudColor16.
 * Reimplements data 0x56bc58: g_zVideo_pfnSubmitPolyColorAttr.
 * Reimplements data 0x56bc5c: g_zVideo_pfnSubmitPolyRenderClass.
 * Reimplements data 0x56bc60: g_zVideo_pfnSubmitPolygon.
 * Reimplements data 0x56bc64: g_zVideo_pfnSubmitPolygonLit.
 * Reimplements data 0x56bc68: g_zVideo_pfnDrawPointColor16.
 * Purpose: dispatch image-surface, fog, and polygon submit hooks.
 */
zVideo_ImageLazyCreateSurfaceProc g_zVideo_pfnImageLazyCreateVideoMemorySurface = 0;
zVideo_ImageProc g_zVideo_pfnImageEnsureSurfaceForCurrentDevice = 0;
zVideo_SetFogEnableProc g_zVideo_pfnSetFogEnable = 0;
zVideo_SetFogFloatProc g_zVideo_pfnSetFogStart = 0;
zVideo_SetFogFloatProc g_zVideo_pfnSetFogEnd = 0;
zVideo_ApplyFogStateProc g_zVideo_pfnApplyFogStateFromGlobals = 0;
zVideo_SubmitPolyFlatColor16Proc g_zVideo_pfnSubmitPolyFlatColor16 = 0;
zVideo_SubmitPolyGouraudColor16Proc g_zVideo_pfnSubmitPolyGouraudColor16 = 0;
zVideo_SubmitPolyColorAttrProc g_zVideo_pfnSubmitPolyColorAttr = 0;
zVideo_SubmitPolyRenderClassProc g_zVideo_pfnSubmitPolyRenderClass = 0;
zVideo_SubmitPolygonProc g_zVideo_pfnSubmitPolygon = 0;
zVideo_SubmitPolygonProc g_zVideo_pfnSubmitPolygonLit = 0;
zVideo_DrawPointColor16Proc g_zVideo_pfnDrawPointColor16 = 0;
/**
 * Reimplements data 0x56bc6c: g_zVideo_pfnFlushSortedPolys.
 * Purpose: dispatch the active renderer's sorted polygon flush routine.
 */
zVideo_FlushProc g_zVideo_pfnFlushSortedPolys = 0;
/**
 * Reimplements data 0x56bc70: g_zVideo_pfnFlushOverwritePolys.
 * Purpose: dispatch the active renderer's overwrite polygon flush routine.
 */
zVideo_FlushProc g_zVideo_pfnFlushOverwritePolys = 0;
/**
 * Reimplements data 0x56bc74: g_zVideo_pfnFlushQuadBatch.
 * Purpose: dispatch the active renderer's queued quad-batch flush routine.
 */
zVideo_FlushProc g_zVideo_pfnFlushQuadBatch = 0;
/**
 * Reimplements data 0x4e307c: g_zVideo_DefaultHwApiDescription.
 * Purpose: provide the writable fallback hardware API description returned
 * when no DirectDraw hardware API record is selected.
 */
char g_zVideo_DefaultHwApiDescription[8] = "Default";

/**
 * Reimplements data 0x633e44: g_zVideo_HwApiDeviceTable.
 * Cached DirectDraw hardware-device owner: BN models four 0x6ec-byte records
 * at 0x633e44. The DirectDraw enumeration callbacks populate the records;
 * memory-query, renderer-selection, and DirectDraw surface paths consume the
 * cached record fields.
 * Purpose: cache accepted DirectDraw hardware API records.
 */
zVidHwApiDeviceRecordPartial g_zVideo_HwApiDeviceTable[4] = {0};
/**
 * Reimplements data 0x633e40: g_zVideo_pSelectedHwApiDeviceRecord.
 * Purpose: point at the selected DirectDraw hardware API record.
 */
zVidHwApiDeviceRecordPartial *g_zVideo_pSelectedHwApiDeviceRecord = 0;
/**
 * Reimplements data 0x6359f4: g_zVideo_pSelectedD3DDeviceInfo.
 * Selected Direct3D device-info pointer. BN types the retail 4-byte
 * zero-initialized slot as a zVidD3DDeviceInfo pointer; zVideo stores either
 * the selected hardware record's first D3D driver record or null.
 * Purpose: cache the active Direct3D device info record for name and device
 * creation queries.
 */
zVidD3DDriverRecordPartial *g_zVideo_pSelectedD3DDeviceInfo = 0;
D3DDEVICEDESC g_zVideo_D3DHalDeviceDesc = {0};
D3DDEVICEDESC g_zVideo_D3DHelDeviceDesc = {0};
/**
 * Reimplements data 0x633404: g_zVideo_D3DMaterialHandle.
 * Reimplements data 0x633638: g_zVideo_QuadBatchCount.
 * Purpose: cache the active Direct3D material handle and queued quad count.
 */
D3DMATERIALHANDLE g_zVideo_D3DMaterialHandle = 0;
int g_zVideo_QuadBatchCount = 0;
zVideo_QuadBatchItemPartial g_zVideo_QuadBatchItemsBase[16] = {0};
D3DTLVERTEX g_zVideo_D3DSubmitTempVertices[64] = {0};
int g_zVideo_SortedPolyDrawOrder[256] = {0};
zVideo_SortedPolyQueueEntry g_zVideo_SortedPolyQueueBase[256] = {0};
zVideo_OverwriteQueueEntry g_zVideo_OverwriteQueueBase[0x180] = {0};
/**
 * Reimplements data 0x632fa0: g_zVideo_SortedPolyQueueCount.
 * Reimplements data 0x6333a4: g_zVideo_OverwriteQueueCount.
 * Purpose: track queued sorted and overwrite polygon counts.
 */
int g_zVideo_SortedPolyQueueCount = 0;
int g_zVideo_OverwriteQueueCount = 0;
zVideo_D3DRenderStateCacheLive g_zVideo_D3DRenderStateCache = {0};
/**
 * Reimplements data 0x633400: g_zVideo_pD3DMaterial2.
 * Reimplements data 0x6333fc: g_zVideo_pD3DViewport2.
 * Reimplements data 0x6333f0: g_zVideo_pD3DDevice.
 * Reimplements data 0x6333ec: g_zVideo_pD3D2.
 * Reimplements data 0x6333dc: g_zVideo_pClipper.
 * Purpose: hold active Direct3D and DirectDraw provider interfaces.
 */
IDirect3DMaterial2 *g_zVideo_pD3DMaterial2 = 0;
IDirect3DViewport2 *g_zVideo_pD3DViewport2 = 0;
IDirect3DDevice2 *g_zVideo_pD3DDevice = 0;
IDirect3D2 *g_zVideo_pD3D2 = 0;
IDirectDrawClipper *g_zVideo_pClipper = 0;
/**
 * Reimplements data 0x6333d8: g_zVideo_pDirectDraw2.
 * Purpose: hold the active IDirectDraw2 provider interface used by DirectDraw
 * surface creation, GDI flipping, and subsystem teardown.
 *
 * DirectDraw/Direct3D interface owner data. BN stores the IDirectDraw2
 * provider pointer after QueryInterface, reads it from surface creation and
 * FlipToGDIIfAttached, and releases/clears it during video teardown.
 */
IDirectDraw2 *g_zVideo_pDirectDraw2 = 0;
/**
 * Reimplements data 0x6333f4: g_zVideo_pZBufferSurface.
 * Reimplements data 0x6333f8: g_zVideo_pZBufferAttachSurface.
 * Reimplements data 0x6333e0: g_zVideo_pPageUnlockSurface.
 * Reimplements data 0x6333e8: g_zVideo_pSurfaceLockVerifier.
 * Reimplements data 0x635d0c: g_zVideo_SurfaceLockVerifyContext.
 * Reimplements data 0x635cf4: g_zVideo_SurfaceLockVerifyFlags.
 * Purpose: hold DirectDraw surface-lock verification state.
 */
IDirectDrawSurface3 *g_zVideo_pZBufferSurface = 0;
IDirectDrawSurface *g_zVideo_pZBufferAttachSurface = 0;
IDirectDrawSurface3 *g_zVideo_pPageUnlockSurface = 0;
zVideo_SurfaceLockVerifier *g_zVideo_pSurfaceLockVerifier = 0;
int g_zVideo_SurfaceLockVerifyContext = 0;
unsigned char g_zVideo_SurfaceLockVerifyFlags = 0;
/*
 * BN models these as zero-initialized 0x20-byte zVideo_SurfaceState records:
 * the software, primary, and display-mode globals are adjacent at 0x632200,
 * 0x632220, and 0x632240; the swap scratch record is the same shape at
 * 0x56bc78 and is used only by the software present adjustment path.
 */
zVideo_SurfaceStatePartial g_zVideo_SwSurfaceState = {0};
zVideo_SurfaceStatePartial g_zVideo_PrimarySurfaceState = {0};
zVideo_SurfaceStatePartial g_zVideo_DisplayModeSurfaceState = {0};
zVideo_SurfaceStatePartial g_zVideo_SurfaceStateSwapScratch = {0};

struct zVideoFxPass3RootElement : zVideoFxPass3Element {
    unsigned short packedColor16;
    unsigned char unknown_3a[0x06];
    double alpha;

    void ApplyPass3();
};

struct zVideoFxPass3Slot : zVideoFxPass3Element {
    int currentRadius;
    int maxRadius;
    int extent;
    float sinFreq;
    float sinPhase;

    zVideoFxPass3Slot * Constructor();
    void SetRectAndPayload(
        int rectLeftPixels,
        int rectTopPixels,
        int currentRadiusPixels,
        int maxRadiusPixels,
        int extentPixels,
        float sinFreqValue,
        float sinPhaseValue
    );
    void ApplyPass3();
};

struct zVideoFxPass3Config : HudUiContainer {
    HudUiRect *inputRectsOrNull[2];
    unsigned short *surfacePixels;
    int surfaceWidth;
    int surfaceHeight;
    int surfacePitchBytes;
    zVideoFxPass3RootElement rootElement;
    zVideoFxPass3Slot slots[5];
    int slotWriteIndex;

    zVideoFxPass3Config * Constructor();
    void Destructor();
    static void CrtInitGlobalSingleton();
    static zVideoFxPass3Config *ConstructGlobalSingleton();
    static void RegisterDestroyAtExit();
    static void DestroyGlobalSingleton();
    void SetInputRectByIndex(
        int index,
        HudUiRect *rectOrNull
    );
    void QueuePrimitiveRaw(
        void *primitive,
        int width,
        int height,
        int pitchBytes
    );
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3RootElement) == 0x48);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3RootElement,
        packedColor16
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3RootElement,
        alpha
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Config,
        rootElement
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Config,
        surfacePixels
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Config,
        surfaceWidth
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Config,
        surfaceHeight
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Config,
        surfacePitchBytes
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Slot,
        currentRadius
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Slot,
        sinPhase
    ) == 0x48
);
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3Slot) == 0x4c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Config,
        slots
    ) == 0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Config,
        slotWriteIndex
    ) == 0x1ec
);
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3Config) == 0x1f0);
#endif

/**
 * Reimplements data 0x56bd58: g_zVideo_FxPass3ConfigLocal.
 * Data owner evidence: retail 0x56bd58 is the authored zero-initialized
 * zVideoFxPass3Config singleton, complete size 0x1f0. Sibling pass-3 scratch,
 * clip, and surface globals are separate zVideo data owners.
 * Purpose: store the local pass-3 UI config used by the zVideo namespace
 * wrapper functions.
 */
zVideoFxPass3Config g_zVideo_FxPass3ConfigLocal;
zVidRect32 g_zVideo_PrimarySurfaceRectScratch = {0};
/**
 * Reimplements data 0x632150: g_zVideo_DisplayModeBpp.
 * BN models this as the zero-initialized int32 written when mode geometry is
 * applied.
 * Purpose: cache the active display mode bit depth.
 */
int g_zVideo_DisplayModeBpp = 0;
/**
 * Reimplements data 0x56b1b8: g_zVid_NoiseByteTableSize.
 * Data owner evidence: zVid::Noise_InitBuffers writes the primary-surface
 * width multiplied by 25 before filling the byte table; DrawNoiseRect uses it
 * as the random row-window limit.
 * Purpose: cache the allocated noise-byte table length.
 */
int g_zVid_NoiseByteTableSize = 0;
/**
 * Reimplements data 0x56b1bc: g_zVid_NoiseByteTable.
 * Data owner evidence: zVid::Noise_InitBuffers allocates and fills this byte
 * table, DrawNoiseRect samples it, and zVid::Noise_ShutdownBuffers frees and
 * clears it when non-null.
 * Purpose: hold the software noise bytes used by the FX surface overlay path.
 */
unsigned char *g_zVid_NoiseByteTable = 0;
/**
 * Reimplements data 0x56b1c0: g_zVideo_FxPass3_ScratchPixels16.
 * Data owner evidence: zVid::Noise_InitBuffers allocates a width*height
 * 16-bpp scratch buffer and stores it after clearing the active FX-surface
 * descriptor; zVid::Noise_ShutdownBuffers frees and clears it when non-null.
 * zVideo::FxPass3_CopySurfacePixelToScratchClipped at 0x48da60 writes through
 * this pointer with tight g_zVideo_FxSurfaceWidth row stride, while
 * zVideo::FxPass3_ApplyToCurrentSurface at 0x48daf0 stages the radial ring
 * warp here before copying back to the active FX surface.
 * Purpose: stage pass-3 warp, blur, and related 16-bpp FX surface pixels.
 */
unsigned short *g_zVideo_FxPass3_ScratchPixels16 = 0;
/**
 * Reimplements data 0x56b1c4: g_zVideo_FxSurfacePixels16.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes this active surface
 * pointer, zVid::Noise_InitBuffers clears it during scratch initialization,
 * and noise/blur/pass-3/FX-surface routines use it as the 16-bpp destination.
 * zVideo::FxPass3_CopySurfacePixelToScratchClipped at 0x48da60 reads source
 * pixels through this pointer using g_zVideo_FxSurfacePitchPixels16.
 * Purpose: point at the currently active 16-bpp FX surface pixel buffer.
 */
unsigned short *g_zVideo_FxSurfacePixels16 = 0;
/**
 * Reimplements data 0x56b1c8: g_zVideo_FxSurfaceWidth.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes the active width,
 * zVid::Noise_InitBuffers clears it, and FX/noise/blur paths use it for bounds
 * and tight scratch-buffer row stride. FxPass3 clipped copies use this for
 * scratch row indexing, distinct from the provider pitch used for source rows.
 * Purpose: cache the active FX surface width in pixels.
 */
int g_zVideo_FxSurfaceWidth = 0;
/**
 * Reimplements data 0x56b1cc: g_zVideo_FxSurfaceHeight.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes the active height,
 * zVid::Noise_InitBuffers clears it, and FX/noise/blur paths use it for full
 * surface clipping.
 * Purpose: cache the active FX surface height in pixels.
 */
int g_zVideo_FxSurfaceHeight = 0;
/**
 * Reimplements data 0x56b1d0: g_zVideo_FxSurfacePitchBytes.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes the provider pitch in
 * bytes and zVid::Noise_InitBuffers clears it with the active surface record.
 * Purpose: retain the active FX surface row pitch in bytes.
 */
int g_zVideo_FxSurfacePitchBytes = 0;
/**
 * Reimplements data 0x56b1d4: g_zVideo_FxSurfacePitchPixels16.
 * Data owner evidence: zVideo::Fx_SetSurfaceState derives this from pitch
 * bytes divided by two; FX/noise/blur paths use it for source/destination row
 * stepping while scratch rows use g_zVideo_FxSurfaceWidth. BN assembly for
 * 0x48da60 multiplies the biased source Y by this value before reading
 * g_zVideo_FxSurfacePixels16.
 * Purpose: retain the active FX surface row pitch in 16-bpp pixels.
 */
int g_zVideo_FxSurfacePitchPixels16 = 0;
/*
 * FxPass3 scratch/clip data-owner note:
 * 0x48da60 reads the two scratch offsets, four clip bounds, active FX-surface
 * descriptor, and scratch pointer as one zVideo pass-3 scratch copy data set.
 * 0x48daf0 writes the clip bounds for every pass and writes the scratch offsets
 * only when it switches from the direct scatter path to the clipped helper path.
 * This documents the local source shape only; the complete zVideo data owner is
 * broader than this slice and remains a parent-owned data-gate decision.
 */
/**
 * Reimplements data 0x56b190: g_zVideo_FxPass3_ScratchOffsetX.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the center
 * X bias before clipped scatter calls; BN assembly for 0x48da60 loads it once
 * and applies it to both the destination delta in ECX and the source X stack
 * delta before clip tests.
 * Purpose: cache the pass-3 clipped copy X offset.
 */
int g_zVideo_FxPass3_ScratchOffsetX = 0;
/**
 * Reimplements data 0x56b194: g_zVideo_FxPass3_ScratchOffsetY.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the center
 * Y bias before clipped scatter calls; BN assembly for 0x48da60 loads it once
 * and applies it to both the destination delta in EDX and the source Y stack
 * delta before clip tests.
 * Purpose: cache the pass-3 clipped copy Y offset.
 */
int g_zVideo_FxPass3_ScratchOffsetY = 0;
/**
 * Reimplements data 0x56b1a0: g_zVideo_FxPass3_ClipMinX.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper tests source
 * and destination X coordinates against it as an inclusive lower bound.
 * Purpose: cache the inclusive minimum X clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMinX = 0;
/**
 * Reimplements data 0x56b1a4: g_zVideo_FxPass3_ClipMinY.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper tests source
 * and destination Y coordinates against it as an inclusive lower bound.
 * Purpose: cache the inclusive minimum Y clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMinY = 0;
/**
 * Reimplements data 0x56b1a8: g_zVideo_FxPass3_ClipMaxX.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper treats this as
 * the exclusive maximum X edge.
 * Purpose: cache the exclusive maximum X clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMaxX = 0;
/**
 * Reimplements data 0x56b1ac: g_zVideo_FxPass3_ClipMaxY.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper treats this as
 * the exclusive maximum Y edge.
 * Purpose: cache the exclusive maximum Y clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMaxY = 0;
/**
 * Reimplements data 0x6333e4: g_zVideo_pDDPalette.
 * Reimplements data 0x6321c8: g_zVideo_hWnd.
 * Purpose: hold the active DirectDraw palette and target window handle.
 */
IDirectDrawPalette *g_zVideo_pDDPalette = 0;
HWND g_zVideo_hWnd = 0;
RECT g_zVideo_CachedClientRectScreen = {0};

/**
 * Reimplements 0x4a6cf0: zVid_PackColorRGB.
 * Purpose: Pack 8-bit RGB components into the active framebuffer pixel format.
 * BN passes red and green as low-byte fastcall registers and consumes the low
 * byte of the stack blue argument.
 */
unsigned int __fastcall zVid_PackColorRGB(
    unsigned char red,
    unsigned char green,
    unsigned int blue
) {
    return ((g_zVideo_PixelPack.gMaskShifted & green) << g_zVideo_PixelPack.sumMinus8) |
        ((g_zVideo_PixelPack.rMaskShifted & red) << g_zVideo_PixelPack.packedBase) |
        ((unsigned char)blue >> g_zVideo_PixelPack.bShiftTo8);
}

/**
 * Reimplements 0x4a6ca0: zVid_PackColor00RRGGBB.
 * Purpose: provide the recovered zVid_PackColor00RRGGBB behavior.
 */
unsigned int __fastcall zVid_PackColor00RRGGBB(
    unsigned int color00RRGGBB
) {
    const unsigned char red = (unsigned char)(color00RRGGBB);
    const unsigned char green = (unsigned char)(color00RRGGBB >> 8);
    const unsigned char blue = (unsigned char)(color00RRGGBB >> 16);

    return ((g_zVideo_PixelPack.gMaskShifted & green) << g_zVideo_PixelPack.sumMinus8) |
           ((g_zVideo_PixelPack.rMaskShifted & red) << g_zVideo_PixelPack.packedBase) |
           (blue >> g_zVideo_PixelPack.bShiftTo8);
}

/**
 * Reimplements 0x4a6d40: zVid_PackColorRgbFloats.
 * Purpose: round RGB float channels and pack them through the active 16-bit pixel format.
 */
unsigned short __fastcall zVid_PackColorRgbFloats(
    zVideo_ColorRgbFloat *color
) {
    unsigned short packed;

    packed = (unsigned short)(int)(color->r + 0.5f);
    packed &= (unsigned short)(g_zVideo_PixelPack.rMaskShifted);
    packed <<= (unsigned char)(g_zVideo_PixelPack.packedBase);
    packed = (unsigned short)(packed |
        (((int)(color->g + 0.5f) & g_zVideo_PixelPack.gMaskShifted) << g_zVideo_PixelPack.sumMinus8));
    packed = (unsigned short)(packed |
        ((unsigned short)(int)(color->b + 0.5f) >> g_zVideo_PixelPack.bShiftTo8));
    return packed;
}

/**
 * Reimplements 0x4a6b80: zVideo::SetClearColorPacked16.
 * Purpose: store the packed 16-bit clear color used by zVideo clear paths.
 *
 * Evidence: BN source file zVideo.cpp is a leaf fastcall store of ECX into
 * zero-initialized g_zVideo_ClearColorPacked16 at 0x6321cc.
 */
void __fastcall zVideo_SetClearColorPacked16(
    unsigned int packedColor16
) {
    g_zVideo_ClearColorPacked16 = packedColor16;
}

/**
 * Reimplements 0x4a7250: zVideo_SetPendingFogTargetColorFromRgb01
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Data evidence: writes the D3D color-attribute bias globals at
 * 0x6321dc-0x6321e4 and, for non-software renderers, the normalize-channel
 * index at 0x632140.
 * Purpose: scale a normalized fog target color into D3D color-bias globals.
 */
void __fastcall zVideo_SetPendingFogTargetColorFromRgb01(
    zVideo_ColorRgbFloat *color
) {
    g_zVideo_D3DColorAttrBiasR = color->r * 255.0f;
    g_zVideo_D3DColorAttrBiasG = color->g * 255.0f;
    g_zVideo_D3DColorAttrBiasB = color->b * 255.0f;
    if (g_zVideo_RendererType == 0) {
        return;
    }

    if (g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasG) {
        g_zVideo_D3DColorNormalizeChannelIndex =
            g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasB ? 0 : 2;
        return;
    }

    if (g_zVideo_D3DColorAttrBiasG >= g_zVideo_D3DColorAttrBiasB) {
        g_zVideo_D3DColorNormalizeChannelIndex = 1;
        return;
    }

    g_zVideo_D3DColorNormalizeChannelIndex =
        g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasB ? 0 : 2;
}

/**
 * Reimplements 0x479ce0: zVideo_SetActiveViewContext.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: BN stores the supplied camera context into the projection
 * context cache at 0x576214, updates gClipRect_Primary at 0x576218, and writes
 * the project clip floats at 0x57623c..0x576248 before zMath projection setup.
 * Purpose: provide the recovered zVideo_SetActiveViewContext behavior.
 */
void __fastcall zVideo_SetActiveViewContext(
    zClass_CameraDataPartial *viewContext
) {
    g_zVideo_pActiveProjectionViewContext = viewContext;

    if (viewContext->nearClip < 1.0f) {
        viewContext->nearClip = 1.0f;
    }

    gClipRect_Primary.zMin = viewContext->nearClip + viewContext->nearClip;
    if (g_zVideo_ActiveRendererPath == 0) {
        zVideo_dd3d::SetQuadBatchDepthAndRhw(1.0f / gClipRect_Primary.zMin);
        viewContext = g_zVideo_pActiveProjectionViewContext;
    }

    gClipRect_Primary.zMax = viewContext->farClip;

    int windowX;
    int windowY;
    if (zClass_Window::gwWindowGetSize(
        viewContext->windowNode,
        &windowX,
        &windowY
    ) != 0) {
        windowX = 0;
        windowY = 0;
    }

    viewContext = g_zVideo_pActiveProjectionViewContext;
    int width;
    int height;
    if (zClass_Window::gwWindowGetResolution(
        viewContext->windowNode,
        &width,
        &height
    ) != 0) {
        width = zVideo::GetPrimarySurfaceWidth();
        height = zVideo::GetPrimarySurfaceHeight();
    }

    const int rightPx = windowX + width;
    const int bottomPx = windowY + height;
    const float left = (float)(windowX);
    const float top = (float)(windowY);
    const float right = (float)(rightPx);
    const float bottom = (float)(bottomPx);
    float viewportOriginX;
    float viewportOriginY;
    float viewportBottom;
    float projectClipLeft;

    if (g_zVideo_ActiveRendererPath == 0) {
        viewportOriginX = left;
        viewportOriginY = top;
        viewportBottom = bottom;
        projectClipLeft = left;
        gClipRect_Primary.xMin = left + 0.5f - 0.999000013f;
        gClipRect_Primary.xMax = right + 1.49900007f;
        gClipRect_Primary.xMaxAlt = right + 0.5f - 0.00100000005f;
        gClipRect_Primary.yMin = top + 0.5f - 0.999000013f;
        gClipRect_Primary.yMax = bottom + 1.49900007f;
        gClipRect_Primary.yMaxAlt = bottom + 0.5f - 0.00100000005f;
    } else {
        const float rightWithSlop = right + 0.00100000005f;
        const float bottomWithSlop = bottom + 0.00100000005f;
        viewportOriginX = left;
        viewportOriginY = top;
        viewportBottom = bottomWithSlop;
        projectClipLeft = left;
        gClipRect_Primary.xMin = left;
        gClipRect_Primary.xMax = rightWithSlop;
        gClipRect_Primary.xMaxAlt = rightWithSlop;
        gClipRect_Primary.yMin = top;
        gClipRect_Primary.yMax = bottomWithSlop;
        gClipRect_Primary.yMaxAlt = bottomWithSlop;
    }

    g_zVideo_ProjectClipLeft = projectClipLeft;
    g_zVideo_ProjectClipTop = viewportOriginY;
    g_zVideo_ProjectClipRight = right - 0.00100000005f;
    g_zVideo_ProjectClipBottom = viewportBottom - 0.00100000005f;

    viewContext = g_zVideo_pActiveProjectionViewContext;
    zMath_Setup_Projection(
        viewportOriginX,
        viewportOriginY,
        (float)(width) * 0.5f,
        (float)(height) * 0.5f,
        viewContext->viewportScaleX,
        viewContext->viewportScaleY,
        viewContext->nearClip,
        viewContext->farClip
    );

    int fovXBits;
    int fovYBits;
    memcpy(
        &fovXBits,
        &viewContext->fovX,
        sizeof(fovXBits)
    );
    memcpy(
        &fovYBits,
        &viewContext->fovY,
        sizeof(fovYBits)
    );
    zMath_SetScreenSize(
        fovXBits,
        fovYBits
    );
}

/**
 * Reimplements 0x44d600: zVideo_sw::RenderFrame.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: BN writes the render-frame active view context at 0x5398fc,
 * updates the active variant tag at 0x5398f8, dispatches the three renderer
 * flush callbacks at 0x56bc6c..0x56bc74, and brackets rendering through the
 * scene-depth owner at 0x632148.
 * Purpose: provide the recovered zVideo_sw_RenderFrame behavior.
 */
int __fastcall zVideo_sw_RenderFrame(
    zClass_NodePartial *camera,
    int updateFxPass3Local
) {
    const int queuedLensFlareSampleCount = zRndr_LensFlare_GetQueuedSampleCount();
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)&slotBuffer);

    g_zVideo_pActiveViewContext = (zClass_CameraDataPartial *)(camera->classData);
    zClass_NodePartial *world = zClass_Camera::gwCameraGetWorld(camera);
    zClass_CameraDataPartial *viewContext = g_zVideo_pActiveViewContext;
    zClass_WindowDataPartial *windowData =
        (zClass_WindowDataPartial *)(viewContext->windowNode->classData);

    if (g_zClass_CameraAutoClipDistanceAdjustEnabled != 0) {
        if (g_FrameDeltaTimeSec <= g_zClass_CameraAutoClipDistanceThreshold) {
            g_zClass_CameraAutoClipDistanceScale += g_zClass_CameraAutoClipDistanceStep;
        } else {
            g_zClass_CameraAutoClipDistanceScale -= g_zClass_CameraAutoClipDistanceStep;
        }

        if (g_zClass_CameraAutoClipDistanceScale > 1.0f) {
            g_zClass_CameraAutoClipDistanceScale = 1.0f;
        } else if (g_zClass_CameraAutoClipDistanceScale < g_zClass_CameraAutoClipDistanceMinScale) {
            g_zClass_CameraAutoClipDistanceScale = g_zClass_CameraAutoClipDistanceMinScale;
        }

        zClass_Camera::gwCameraSetClipDistance(
            camera,
            g_zClass_CameraAutoClipDistanceScale
        );
    }

    zClass_World::InitLightPointInPolygonXZ(world);
    zVideo::ReturnSuccessStub();
    zClass_Camera::gwCameraUpdate(camera);
    zClass_Camera::SyncViewContextPositions();
    zVideo_SetActiveViewContext(g_zVideo_pActiveViewContext);
    zClass_World::UpdateAllLights(world);
    zClass_World::UpdateAllSounds(world);

    const int variantFilterEnabled = g_Variant_FilterEnabled;
    g_zClass_LodDistanceStateStackTop = 0;
    PlayerProbeSampleCandidateBuffer pickCandidates = {0};
    if (variantFilterEnabled != 0) {
        viewContext = g_zVideo_pActiveViewContext;
        if (viewContext->variantOverrideEnabled != 0 && variantFilterEnabled == 1) {
            g_Variant_CurrentTag = viewContext->variantTag;
        } else {
            g_Variant_FilterEnabled = 0;
            zClass_cls_di::FindBestPickCandidateBelowPoint(
                world,
                &viewContext->cameraPos,
                &pickCandidates
            );
            g_Variant_FilterEnabled = variantFilterEnabled;

            if (pickCandidates.candidateCount <= 0) {
                zTag4::Clear(&g_zVideo_pActiveViewContext->variantTag);
                viewContext = g_zVideo_pActiveViewContext;
                g_Variant_CurrentTag = viewContext->variantTag;
            } else if (pickCandidates.entries[0].variantTag.count > 0) {
                g_zVideo_pActiveViewContext->variantTag = pickCandidates.entries[0].variantTag;
                viewContext = g_zVideo_pActiveViewContext;
                g_Variant_CurrentTag = pickCandidates.entries[0].variantTag;
            }
        }

        viewContext = g_zVideo_pActiveViewContext;
        g_zVideo_ActiveViewVariantTag = viewContext->variantTag;
    }

    zVideoD3D::SceneEnter();
    zClass_Camera::RenderWorld(
        world,
        camera,
        g_zVideo_pActiveViewContext
    );
    zMath::MatStackPopPtr();

    g_zVideo_pfnFlushSortedPolys();
    if (updateFxPass3Local != 0) {
        zVideo::FxPass3_UpdateLocal(g_FrameDeltaTimeSec);
    }
    g_zVideo_pfnFlushSortedPolys();
    g_zVideo_pfnFlushOverwritePolys();

    const int visibleLensFlareSampleCount =
        zRndr_LensFlare_BuildVisibleSampleListFromQueue(queuedLensFlareSampleCount);
    for (int sampleIndex = 0; sampleIndex < visibleLensFlareSampleCount; ++sampleIndex) {
        zVec3 visibleSamplePoint = {0};
        zRndr_SpanOcclusion_FilterSampleList(
            sampleIndex,
            &visibleSamplePoint
        );
        zClass_cls_di::SetStopAfterFirstHit(0x40000);
        zClass_cls_di::SetBreakOnFirstCandidate(1);
        viewContext = g_zVideo_pActiveViewContext;
        const int raycastHit = zClass_cls_di::RaycastFindClosest(
            viewContext->worldNode,
            &pickCandidates,
            viewContext->cameraPos.x,
            viewContext->cameraPos.y,
            viewContext->cameraPos.z,
            visibleSamplePoint.x,
            visibleSamplePoint.y,
            visibleSamplePoint.z
        );
        zClass_cls_di::SetBreakOnFirstCandidate(0);
        if (raycastHit != 0 || pickCandidates.candidateCount == 0) {
            zRndr_LensFlare_DrawVisibleSample(sampleIndex);
        }
    }

    g_zVideo_pfnFlushSortedPolys();
    g_zVideo_pfnFlushOverwritePolys();
    g_zVideo_pfnFlushQuadBatch();
    zVideoD3D::SceneLeave();

    if (zClass_TypeList::CountNodes(8) > 1 && (windowData->clearPolyIndexFlags & 0x80000000) != 0) {
        const int clearPolyCount = windowData->clearPolyIndexFlags & 0x7fffffff;
        for (int i = 0; i < clearPolyCount; ++i) {
            zClass_WindowClearPoly *poly = &windowData->clearPolys[i];
            if ((poly->vertCount & 0x80000000) == 0) {
                continue;
            }

            const int vertexCount = poly->vertCount & 0x7fffffff;
            if (vertexCount <= 0) {
                continue;
            }

            zVidRect32 rect;
            rect.left = (int)(poly->vertices[0].x);
            rect.right = rect.left;
            rect.top = (int)(poly->vertices[0].y);
            rect.bottom = rect.top;

            for (int vertexIndex = 1; vertexIndex < vertexCount; ++vertexIndex) {
                const zVec3 *vertex = &poly->vertices[vertexIndex];
                if (rect.left > vertex->x) {
                    rect.left = (int)(vertex->x);
                }
                if (rect.right < vertex->x) {
                    rect.right = (int)(vertex->x);
                }
                if (rect.top > vertex->y) {
                    rect.top = (int)(vertex->y);
                }
                if (rect.bottom < vertex->y) {
                    rect.bottom = (int)(vertex->y);
                }
            }

            zVideo_dd3d::CallClearZBufferRect(&rect);
        }
    }

    return 0;
}

/**
 * Reimplements 0x47a0c0: zVideo_UpdateProjectionStateFromCameraData.
 * Original source path: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: provide the recovered zVideo_UpdateProjectionStateFromCameraData behavior.
 */
void __fastcall zVideo_UpdateProjectionStateFromCameraData(
    zClass_CameraDataPartial *cameraData
) {
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();

    zMat4x3 yawSlotBuffer = {0};
    zMath::MatStackPushAndCloneParent((float *)(&yawSlotBuffer));
    cameraData->localFrustumLeftNormal.x = 1.0f;
    cameraData->localFrustumLeftNormal.y = 0.0f;
    cameraData->localFrustumLeftNormal.z = 0.0f;
    zMath::MatRotateY(cameraData->frustumYaw);
    zMath_Vec3Array_UntransformDirection(
        &cameraData->localFrustumLeftNormal,
        1
    );
    zMath::MatStackPopPtr();

    cameraData->localFrustumRightNormal.x = -cameraData->localFrustumLeftNormal.x;
    cameraData->localFrustumRightNormal.y = cameraData->localFrustumLeftNormal.y;
    cameraData->localFrustumRightNormal.z = cameraData->localFrustumLeftNormal.z;

    cameraData->localFrustumBottomNormal.x = 0.0f;
    cameraData->localFrustumBottomNormal.y = -1.0f;
    cameraData->localFrustumBottomNormal.z = 0.0f;
    zMath::MatRotateX(cameraData->frustumPitch);
    zMath_Vec3Array_UntransformDirection(
        &cameraData->localFrustumBottomNormal,
        1
    );
    zMath::MatStackPopPtr();

    cameraData->localFrustumTopNormal.x = cameraData->localFrustumBottomNormal.x;
    cameraData->localFrustumTopNormal.y = -cameraData->localFrustumBottomNormal.y;
    cameraData->localFrustumTopNormal.z = cameraData->localFrustumBottomNormal.z;

    cameraData->localFrustumNearNormal.x = 0.0f;
    cameraData->localFrustumNearNormal.y = 0.0f;
    cameraData->localFrustumNearNormal.z = -1.0f;
    cameraData->localFrustumFarNormal.x = 0.0f;
    cameraData->localFrustumFarNormal.y = 0.0f;
    cameraData->localFrustumFarNormal.z = 1.0f;
}

/**
 * Recovered helper: zVideo_SubtractVec3.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this zVec3 subtraction pattern for near, camera, and far
 * frustum-center deltas.
 * Purpose: subtract one zVec3 from another and return the delta.
 */
static zVec3 zVideo_SubtractVec3(
    zVec3 *lhs,
    zVec3 *rhs
) {
    zVec3 delta;
    delta.x = lhs->x - rhs->x;
    delta.y = lhs->y - rhs->y;
    delta.z = lhs->z - rhs->z;
    return delta;
}

/**
 * Recovered helper: zVideo_DotVec3.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this x/y/z multiply-add dot-product pattern for every
 * frustum plane comparison.
 * Purpose: compute the dot product of two zVec3 values.
 */
static float zVideo_DotVec3(
    zVec3 *lhs,
    zVec3 *rhs
) {
    return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z;
}

/**
 * Recovered helper: zVideo_TestSpherePlane.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this sphere/plane reject-or-clip test for the side and far
 * frustum planes.
 * Purpose: test one sphere against one frustum plane and update the clip mask.
 */
static int zVideo_TestSpherePlane(
    zVec3 *delta,
    zVec3 *normal,
    float radius,
    int planeBit,
    int *clipMaskInOut
) {
    const float dot = zVideo_DotVec3(
        delta,
        normal
    );
    if (-radius >= dot) {
        return planeBit;
    }

    if (dot < radius) {
        *clipMaskInOut |= planeBit;
    }

    return 0;
}

/**
 * Reimplements 0x478c70: zVideo_FrustumTestSphereClipMask.
 * Original file: GameZRecoil/zModel/zModel_Display.cpp.
 * Purpose: reject or clip a sphere against the active view frustum planes.
 *
 * Evidence: BN reads the projection view-context global at 0x576214, clears
 * the incoming clip mask, tests near and far centers separately, and tests
 * side planes against camera-position deltas while accumulating clip bits.
 */
int __fastcall zVideo_FrustumTestSphereClipMask(
    zVec3 *sphereCenter,
    int *clipMaskInOut,
    float radius
) {
    const int oldMask = *clipMaskInOut;
    *clipMaskInOut = 0;

    zClass_CameraDataPartial *viewContext = g_zVideo_pActiveProjectionViewContext;
    zVec3 delta;
    if ((oldMask & 0x10) != 0) {
        delta = zVideo_SubtractVec3(
            sphereCenter,
            &viewContext->nearClipCenter
        );
        const float dot = zVideo_DotVec3(
            &delta,
            &viewContext->worldFrustumNormals[4]
        );
        if (dot < radius) {
            if (-radius >= dot) {
                return 0x10;
            }
            *clipMaskInOut = 0x10;
        } else {
            *clipMaskInOut = 0;
        }
    }

    viewContext = g_zVideo_pActiveProjectionViewContext;
    delta = zVideo_SubtractVec3(
        sphereCenter,
        &viewContext->cameraPos
    );

    if ((oldMask & 1) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[0],
            radius,
            1,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 2) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[1],
            radius,
            2,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 4) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[2],
            radius,
            4,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 8) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[3],
            radius,
            8,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 0x20) != 0) {
        viewContext = g_zVideo_pActiveProjectionViewContext;
        delta = zVideo_SubtractVec3(
            sphereCenter,
            &viewContext->farClipCenter
        );
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[5],
            radius,
            0x20,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

/**
 * Reimplements 0x4a7770: zVideo_RestoreIconicFullscreenWindowIfNeeded.
 * Purpose: provide the recovered zVideo_RestoreIconicFullscreenWindowIfNeeded behavior.
 */
void zVideo_RestoreIconicFullscreenWindowIfNeeded() {
    if (g_zVideo_IsInitialized != 0 && g_zVideo_FullscreenOption != 0 &&
        IsIconic(g_zVideo_hWnd) != 0) {
        OpenIcon(g_zVideo_hWnd);
    }
}
}

namespace zVid {

/**
 * Reimplements 0x4a59b0: zVid::QueryCachedClientRectUpdateMaskIf3dfx.
 * Original file: Battlesport/zVideo.cpp.
 * Purpose: return the cached client-rect update mask unless the path-2
 * renderer is active.
 *
 * Data evidence: BN reads g_zVideo_ActiveRendererPath at 0x56bbe8 and
 * g_zVid_CachedClientRectUpdateMask at 0x56b564; the branchless predicate
 * subtracts renderer path 2, negates it, and uses sbb as a nonzero mask.
 */
int QueryCachedClientRectUpdateMaskIf3dfx() {
    if (g_zVideo_ActiveRendererPath != 2) {
        return g_zVid_CachedClientRectUpdateMask;
    }
    return 0;
}

/**
 * Reimplements 0x443a40: zVid::UpdateCachedClientRectIfUpdateMaskEnabled.
 * Original file evidence: BN function comment names Battlesport/RecoilApp.cpp;
 * this reconstruction keeps the helper with the zVid video namespace cluster
 * while source-file mapping is stale.
 * Purpose: refresh the cached client rectangle when the renderer-path update
 * mask is set.
 *
 * Evidence: BN calls zVid::QueryCachedClientRectUpdateMaskIf3dfx and tail-jumps
 * to zVideo::UpdateCachedClientRectScreenCoords only when the query is nonzero.
 */
void UpdateCachedClientRectIfUpdateMaskEnabled() {
    if (QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
        zVideo::UpdateCachedClientRectScreenCoords();
    }
}

} // namespace zVid

RECOIL_STATIC_ASSERT(sizeof(zVidHwApiDeviceRecordPartial) == 0x6ec);
RECOIL_STATIC_ASSERT(sizeof(zVidD3DDriverRecordPartial) == 0x190);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidHwApiDeviceRecordPartial,
        m_videoMemTotalBytes
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidHwApiDeviceRecordPartial,
        m_videoMemFreeBytes
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidHwApiDeviceRecordPartial,
        m_textureMemTotalBytes
    ) == 0x9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidHwApiDeviceRecordPartial,
        m_textureMemFreeBytes
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidHwApiDeviceRecordPartial,
        m_deviceFeatureFlags
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidHwApiDeviceRecordPartial,
        m_acceptedD3DDeviceCount
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidHwApiDeviceRecordPartial,
        m_d3dDrivers
    ) == 0xac
);
RECOIL_STATIC_ASSERT(sizeof(DDCAPS) == 0x17c);
RECOIL_STATIC_ASSERT(sizeof(DDSURFACEDESC) == 0x6c);
RECOIL_STATIC_ASSERT(sizeof(D3DDEVICEDESC) == 0xfc);
RECOIL_STATIC_ASSERT(sizeof(D3DVIEWPORT2) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(D3DMATERIAL) == 0x50);
RECOIL_STATIC_ASSERT(sizeof(D3DTLVERTEX) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zVideo_QuadBatchItemPartial) == 0x80);
RECOIL_STATIC_ASSERT(sizeof(zVideo_XyzVertex) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_ColorRgbFloat) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_TexCoord) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_RenderClass,
        textureHandle
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_RenderClass,
        textureMapBlend
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_RenderClass,
        textureAddressU
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_RenderClass,
        textureAddressV
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zVideo_RenderClass) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_SortedPolyQueueEntry) == 0x80c);
RECOIL_STATIC_ASSERT(sizeof(zVideo_OverwriteQueueEntry) == 0x810);
RECOIL_STATIC_ASSERT(sizeof(zVideo_D3DRenderStateCacheLive) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_D3DRenderStateCacheLive,
        alphaBlendEnable
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_D3DRenderStateCacheLive,
        shadeMode
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_D3DRenderStateCacheLive,
        textureMapBlend
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_D3DRenderStateCacheLive,
        textureAddressU
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_D3DRenderStateCacheLive,
        textureAddressV
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_D3DRenderStateCacheLive,
        textureHandle
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_D3DRenderStateCacheLive,
        zWriteEnable
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_SortedPolyQueueEntry,
        vertices
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_OverwriteQueueEntry,
        vertices
    ) == 0x10
);
RECOIL_STATIC_ASSERT(sizeof(zVideo_TextureRecordPartial) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_TextureRecordPartial,
        m_textureHandle
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_TextureRecordPartial,
        m_alphaMode
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_TextureRecordPartial,
        m_uWrapMode
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_TextureRecordPartial,
        m_vWrapMode
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_SurfaceStatePartial,
        locked
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_SurfaceStatePartial,
        pageLockActive
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideo_SurfaceStatePartial,
        surf
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(sizeof(zVideo_SurfaceStatePartial) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zVidImagePartial) == 0x38);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        width
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        height
    ) == 0x06
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        formatFlagsPacked
    ) == 0x09
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        pixels
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        alphaMap
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        pitchWords
    ) == 0x34
);
RECOIL_STATIC_ASSERT(sizeof(zVidTexturePackRecord) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(zVidTexturePackHeader) == 0x18);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidTexturePackEntry,
        fileHandle
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidTexturePackEntry,
        header
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidTexturePackEntry,
        records
    ) == 0x9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidTexturePackEntry,
        paletteTableBaseIndex
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(sizeof(zVidTexturePackEntry) == 0xa4);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidPaletteRemapRecipe,
        color1R
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidPaletteRemapRecipe,
        color0Strength
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zVidPaletteRemapRecipe) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        width
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        formatFlagsPacked
    ) == 0x09
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        uPow2Shift
    ) == 0x0a
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        vPow2Shift
    ) == 0x0b
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        textureAddressFlagsPacked
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        paletteMetaPacked
    ) == 0x0e
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        pixels
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        alphaMap
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        palette
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        widthScale
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        queuedAlphaMap
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        uShiftFrom20
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        uMask
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        vMaskFixed20
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        surface
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVidImagePartial,
        pitchWords
    ) == 0x34
);
RECOIL_STATIC_ASSERT(sizeof(zVidRect32) == sizeof(RECT));

namespace zVid {
/**
 * Reimplements 0x408280: zVid::SetAccelerationOption.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: store the selected video acceleration option and mirror the active
 * hardware-mode option used by zOpt accessors.
 *
 * Evidence: BN writes ecx through ZOPT_VIDEO_ACCELERATION, then stores the same
 * value into g_zOpt_HwMode; VC5SP3 zvid_option_getters byte verification is
 * exact after relocation masking.
 */
void __fastcall SetAccelerationOption(
    int accelerationOption
) {
    *ZOPT_VIDEO_ACCELERATION = accelerationOption;
    g_zOpt_HwMode = accelerationOption;
}

/**
 * Reimplements 0x408290: zVid::SetHwApiOption.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: store the selected hardware API/backend option.
 *
 * Evidence: BN writes ecx through ZOPT_HW_API and returns without touching
 * other state; VC5SP3 zvid_option_getters byte verification is exact after
 * relocation masking.
 */
void __fastcall SetHwApiOption(
    int hwApiOption
) {
    *ZOPT_HW_API = hwApiOption;
}

/**
 * Reimplements 0x408310: zVid::GetAccelerationOption.
 * Purpose: provide the recovered zVid::GetAccelerationOption behavior.
 */
int GetAccelerationOption() {
    return *ZOPT_VIDEO_ACCELERATION;
}

/**
 * Reimplements 0x408320: zVid::GetHwApiOption.
 * Purpose: provide the recovered zVid::GetHwApiOption behavior.
 */
int GetHwApiOption() {
    return *ZOPT_HW_API;
}

/**
 * Reimplements 0x4a7480: zVid::GetAcceptedDirectDrawDeviceCount.
 * Original file evidence: BN comment identifies this as the public zVid thunk
 * in GameZRecoil/zVideo_dd.cpp, tail-jumping to the zvid_dd.c cached accessor.
 * Purpose: return the accepted DirectDraw hardware API device count.
 */
int GetAcceptedDirectDrawDeviceCount() {
    return zVideo_dd::GetAcceptedDirectDrawDeviceCountCached();
}

/**
 * Reimplements 0x4a9910: zVid::GetAcceptedHardwareRendererCount_Cached.
 * Purpose: provide the recovered zVid::GetAcceptedHardwareRendererCount_Cached behavior.
 */
int GetAcceptedHardwareRendererCount_Cached() {
    return g_zVid_AcceptedHardwareRendererCount;
}

/**
 * Reimplements 0x4b3220: zVid::HasAcceptedHardwareRenderer.
 * Purpose: provide the recovered zVid::HasAcceptedHardwareRenderer behavior.
 */
int HasAcceptedHardwareRenderer() {
    return GetAcceptedHardwareRendererCount_Cached() > 0 ? 1 : 0;
}

/**
 * Reimplements 0x46d5c0: zVid::GetTexturePackLoadState
 * Purpose: Return the current texture-pack loading enable state.
 */
int GetTexturePackLoadState() {
    return g_zVid_TexturePackLoadState;
}

/**
 * Reimplements 0x46d5b0: zVid::SetTexturePackLoadState
 * Purpose: Store the texture-pack loading enable state.
 */
void __fastcall SetTexturePackLoadState(
    int texturePackLoadState
) {
    g_zVid_TexturePackLoadState = texturePackLoadState;
}

/**
 * Reimplements 0x4086b0: zVid::GetVideoModeIndexFromOptions.
 * Purpose: provide the recovered zVid::GetVideoModeIndexFromOptions behavior.
 */
int GetVideoModeIndexFromOptions() {
    return *ZOPT_VIDEO_MODE;
}

/**
 * Reimplements 0x408720: zVid::SetVideoModeIndex.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: apply a persisted shell video-mode preset to the render, window,
 * display, and replicate options.
 *
 * Evidence: BN selects modes 2 through 7 through the jump table, writes
 * ZOPT_VIDEO_MODE, updates the zOpt render/window/display sections, sets
 * display bits-per-pixel to 16, and tail-calls zOpt::SetReplicateMode for
 * valid presets; invalid values write ZVID_MODE_INVALID. VC5SP3
 * zvid_set_video_mode_index byte verification is exact after relocation
 * masking.
 */
void __fastcall SetVideoModeIndex(
    int modeIndex
) {
    switch (modeIndex) {
    case 2:
        *ZOPT_VIDEO_MODE = 2;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            320,
            200
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(1);
        return;

    case 3:
        *ZOPT_VIDEO_MODE = 3;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            320,
            240
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(1);
        return;

    case 4:
        *ZOPT_VIDEO_MODE = 4;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            640,
            400
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    case 5:
        *ZOPT_VIDEO_MODE = 5;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            640,
            480
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    case 6:
        *ZOPT_VIDEO_MODE = 6;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            800,
            600
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            800,
            600
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            800,
            600
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    case 7:
        *ZOPT_VIDEO_MODE = 7;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            1024,
            768
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            1024,
            768
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            1024,
            768
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    default:
        *ZOPT_VIDEO_MODE = 0;
        return;
    }
}

/**
 * Reimplements 0x4a9950: zVid::QueryDeviceVideoMemoryBytes.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: queries live or cached DirectDraw video memory totals and free bytes for a device.
 */
int __fastcall QueryDeviceVideoMemoryBytes(
    int deviceIndexOrMinus1,
    int *totalBytes,
    int *freeBytes
) {
    if (g_zVideo_RendererType == 0) {
        *freeBytes = 0;
        *totalBytes = 0;
        return 0;
    }

    if (deviceIndexOrMinus1 == -1) {
        DDSCAPS caps = {0};
        caps.dwCaps = DDSCAPS_VIDEOMEMORY;
        if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
                &caps,
                (DWORD *)totalBytes,
                (DWORD *)freeBytes
            ) == DD_OK) {
            *freeBytes -= g_zVideo_pSelectedHwApiDeviceRecord->m_textureMemTotalBytes;
        } else {
            *freeBytes = 0;
            *totalBytes = 0;
        }
        return 1;
    }

    const zVidHwApiDeviceRecordPartial &device = g_zVideo_HwApiDeviceTable[deviceIndexOrMinus1];
    *totalBytes = device.m_videoMemTotalBytes;
    if (device.m_videoMemTotalBytes == device.m_textureMemTotalBytes) {
        *freeBytes = device.m_videoMemFreeBytes - 0x1f4000;
    } else {
        *freeBytes = device.m_videoMemFreeBytes - device.m_textureMemTotalBytes;
    }

    return 1;
}

/**
 * Reimplements 0x4a9a30: zVid::QueryTextureMemoryBytes.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: queries live or cached DirectDraw texture memory totals and free bytes for a device.
 */
int __fastcall QueryTextureMemoryBytes(
    int deviceIndexOrMinus1,
    int *totalBytes,
    int *freeBytes
) {
    if (g_zVideo_pDirectDraw2 == 0) {
        *freeBytes = 0;
        *totalBytes = 0;
        return 0;
    }

    if (deviceIndexOrMinus1 == -1) {
        DDSCAPS caps = {0};
        caps.dwCaps = DDSCAPS_TEXTURE;
        if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
                &caps,
                (DWORD *)totalBytes,
                (DWORD *)freeBytes
            ) != DD_OK) {
            *freeBytes = 0;
            *totalBytes = 0;
        }
        return 1;
    }

    const zVidHwApiDeviceRecordPartial &device = g_zVideo_HwApiDeviceTable[deviceIndexOrMinus1];
    *totalBytes = device.m_textureMemTotalBytes;
    *freeBytes = device.m_textureMemFreeBytes;
    return 1;
}

/**
 * Reimplements 0x4a59a0: zVid::SetCachedClientRectUpdateMask.
 * Original file: Battlesport/zVideo.cpp.
 * Purpose: store the client-rect update mask used by cached rect refresh helpers.
 *
 * Data evidence: BN stores the fastcall mask argument into the zero-initialized
 * g_zVid_CachedClientRectUpdateMask int32 global at 0x56b564.
 */
void __fastcall SetCachedClientRectUpdateMask(
    int mask
) {
    g_zVid_CachedClientRectUpdateMask = mask;
}

/**
 * Reimplements 0x4a7410: zVid::GetSelectedHwApiDescriptionOrDefault.
 * Purpose: return the selected hardware API description or the default
 * writable fallback string when no hardware API record is selected.
 */
char *GetSelectedHwApiDescriptionOrDefault() {
    return g_zVideo_pSelectedHwApiDeviceRecord != 0
               ? g_zVideo_pSelectedHwApiDeviceRecord->m_driverDescription
               : g_zVideo_DefaultHwApiDescription;
}

/**
 * Reimplements 0x4a9940: zVid::GetSelectedD3DDeviceNameOrDefault.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: return the selected Direct3D device name or the writable default
 * device name when no D3D device record is selected.
 */
char *GetSelectedD3DDeviceNameOrDefault() {
    return g_zVideo_pSelectedD3DDeviceInfo != 0
               ? g_zVideo_pSelectedD3DDeviceInfo->m_deviceName
               : g_zVideo_DefaultD3DDeviceName;
}

/**
 * Reimplements 0x4a7430: zVid::GetHwApiDescription.
 * Purpose: provide the recovered zVid::GetHwApiDescription behavior.
 */
char *__fastcall GetHwApiDescription(
    int index
) {
    return g_zVideo_HwApiDeviceTable[index].m_driverDescription;
}

/**
 * Reimplements 0x4a7450: zVid::GetHwApiDriverName.
 * Purpose: provide the recovered zVid::GetHwApiDriverName behavior.
 */
char *__fastcall GetHwApiDriverName(
    int index
) {
    return g_zVideo_HwApiDeviceTable[index].m_driverName;
}
} // namespace zVid

/**
 * Reimplements 0x4bdb60: zVideoFxPass3Element::Draw.
 * Draws the common HUD base, publishes the parent pass-3 source surface, then dispatches the
 * element-specific pass callback once for each configured input rectangle.
 * Purpose: provide the recovered zVideoFxPass3Element::Draw behavior.
 */
void zVideoFxPass3Element::Draw() {
    zVideoFxPass3Config *const parentConfig = (zVideoFxPass3Config *)(parent);
    DrawBase();

    if (parentConfig == 0) {
        ApplyPass3();
        return;
    }

    if (parentConfig->surfacePixels != 0) {
        zVideo::Fx_SetSurfaceState(
            parentConfig->surfacePixels,
            parentConfig->surfaceWidth,
            parentConfig->surfaceHeight,
            parentConfig->surfacePitchBytes
        );
    }

    int index;
    for (index = 0; index < 2; ++index) {
        HudUiRect *const inputRect = parentConfig->inputRectsOrNull[index];
        if (inputRect != 0) {
            clipRectOrNull = inputRect;
            ApplyPass3();
        }
    }

    clipRectOrNull = parentConfig->inputRectsOrNull[0];
}

/**
 * Original-source helper evidence: no standalone retail address is assigned to
 * the base virtual implementation in this owner. Draw at 0x4bdb60 dispatches
 * the pass callback virtually, and the address-backed overrides are
 * zVideoFxPass3RootElement::ApplyPass3 at 0x4bdbc0 and
 * zVideoFxPass3Slot::ApplyPass3 at 0x4bdc40.
 * Purpose: preserve the empty base pass-3 callback for element types that do
 * not override the pass operation.
 */
void zVideoFxPass3Element::ApplyPass3() {}

/**
 * Reimplements 0x4bdbc0: zVideoFxPass3RootElement::ApplyPass3.
 * Root pass-3 callback submits the currently selected input rectangle as a framebuffer overlay
 * using the root element's recovered color and alpha.
 * Purpose: provide the recovered zVideoFxPass3RootElement::ApplyPass3 behavior.
 */
void zVideoFxPass3RootElement::ApplyPass3() {
    zRndr_OverlayRect_Submit(
        (unsigned int)(packedColor16),
        (zVidRect32 *)(clipRectOrNull),
        alpha
    );
}

/**
 * Reimplements 0x4bdbe0: zVideoFxPass3Slot::Constructor.
 * Installs the pass-3 slot table after the HudUiElement base constructor and clears the input
 * clip consumed by zVideoFxPass3Element::Draw.
 * Purpose: provide the recovered zVideoFxPass3Slot::Constructor behavior.
 */
zVideoFxPass3Slot * zVideoFxPass3Slot::Constructor() {
    HudUiElement::Constructor(
        0,
        0
    );
    clipRectOrNull = 0;
    new (this) zVideoFxPass3Slot;
    return this;
}

/**
 * Reimplements 0x4bdc00: zVideoFxPass3Slot::SetRectAndPayload.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideoFxPass3Slot::SetRectAndPayload behavior.
 */
void zVideoFxPass3Slot::SetRectAndPayload(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreqValue,
    float sinPhaseValue
) {
    SetPos(
        rectLeftPixels,
        rectTopPixels
    );

    currentRadius = currentRadiusPixels;
    maxRadius = maxRadiusPixels;
    extent = extentPixels;
    sinFreq = sinFreqValue;
    sinPhase = sinPhaseValue;
}

/**
 * Reimplements 0x4bdc40: zVideoFxPass3Slot::ApplyPass3.
 * The pass callback forwards the slot position, integer radius payload, sine parameters, and
 * active input clip to the shared pass-3 radial warp routine.
 * Purpose: provide the recovered zVideoFxPass3Slot::ApplyPass3 behavior.
 */
void zVideoFxPass3Slot::ApplyPass3() {
    zVideo::FxPass3_ApplyToCurrentSurface(
        x,
        y,
        currentRadius,
        maxRadius,
        extent,
        sinFreq,
        sinPhase,
        (zVidRect32 *)(clipRectOrNull)
    );
}

/**
 * Reimplements 0x4bef90: zVideoFxPass3Config::Constructor.
 * Constructs the pass-3 singleton as a HudUiContainer, installs the config and element tables,
 * links the root plus five slot children, hides them, and enables the container. The retail
 * constructor leaves surfacePitchBytes untouched.
 * Purpose: provide the recovered zVideoFxPass3Config::Constructor behavior.
 */
zVideoFxPass3Config * zVideoFxPass3Config::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;

    rootElement.HudUiElement::Constructor(
        0,
        0
    );
    rootElement.clipRectOrNull = 0;
    new (&rootElement) zVideoFxPass3RootElement;

    int slotIndex;
    for (slotIndex = 0; slotIndex < 5; ++slotIndex) {
        slots[slotIndex].Constructor();
    }

    inputRectsOrNull[0] = 0;
    inputRectsOrNull[1] = 0;
    surfacePixels = 0;
    surfaceWidth = 0;
    surfaceHeight = 0;

    HudUiContainer::AddChild((HudUiElement *)(&rootElement));
    rootElement.SetVisible(0);

    for (slotIndex = 0; slotIndex < 5; ++slotIndex) {
        HudUiContainer::AddChild((HudUiElement *)(&slots[slotIndex]));
        slots[slotIndex].SetVisible(0);
    }

    slotWriteIndex = 0;
    HudUiContainer::SetEnabled(1);
    return this;
}

/**
 * Reimplements 0x4bee80: zVideoFxPass3Config::Destructor.
 * Destruction mirrors the MSVC array-destructor path, then tears down the container.
 * Purpose: provide the recovered zVideoFxPass3Config::Destructor behavior.
 */
void zVideoFxPass3Config::Destructor() {
    int slotIndex;
    for (slotIndex = 4; slotIndex >= 0; --slotIndex) {
        slots[slotIndex].~zVideoFxPass3Slot();
    }
    rootElement.~zVideoFxPass3RootElement();
    HudUiContainer::DestructorCore();
}

/**
 * Reimplements 0x4bee50: zVideoFxPass3Config::ConstructGlobalSingleton.
 * Purpose: construct and return the local pass-3 config singleton.
 */
zVideoFxPass3Config *zVideoFxPass3Config::ConstructGlobalSingleton() {
    return g_zVideo_FxPass3ConfigLocal.Constructor();
}

/**
 * Reimplements 0x4bee70: zVideoFxPass3Config::DestroyGlobalSingleton.
 * Purpose: provide the recovered zVideoFxPass3Config::DestroyGlobalSingleton behavior.
 */
void zVideoFxPass3Config::DestroyGlobalSingleton() {
    g_zVideo_FxPass3ConfigLocal.Destructor();
}

/**
 * Reimplements 0x4bee60: zVideoFxPass3Config::RegisterDestroyAtExit.
 * Purpose: provide the recovered zVideoFxPass3Config::RegisterDestroyAtExit behavior.
 */
void zVideoFxPass3Config::RegisterDestroyAtExit() {
    atexit(DestroyGlobalSingleton);
}

/**
 * Reimplements 0x4bee40: zVideoFxPass3Config::CrtInitGlobalSingleton.
 * Purpose: provide the recovered zVideoFxPass3Config::CrtInitGlobalSingleton behavior.
 */
void zVideoFxPass3Config::CrtInitGlobalSingleton() {
    ConstructGlobalSingleton();
    RegisterDestroyAtExit();
}

/**
 * Reimplements 0x4bee00: zVideoFxPass3Config::SetInputRectByIndex.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideoFxPass3Config::SetInputRectByIndex behavior.
 */
void zVideoFxPass3Config::SetInputRectByIndex(
    int index,
    HudUiRect *rectOrNull
) {
    if (index < 2) {
        inputRectsOrNull[index] = rectOrNull;
    }
}

namespace zVideo_buff {
/**
 * Reimplements 0x4a69c0: zVideo_buff::ClipCoordToRange.
 * Purpose: provide the recovered zVideo_buff::ClipCoordToRange behavior.
 */
int __fastcall ClipCoordToRange(
    int *coordPtr,
    int minCoord,
    int maxCoord
) {
    const int coord = *coordPtr;
    int clipped = 0;
    if (coord < minCoord) {
        clipped = coord - minCoord;
        *coordPtr = minCoord;
    } else if (coord > maxCoord) {
        clipped = coord - maxCoord;
        *coordPtr = maxCoord;
    }

    return clipped;
}

/**
 * Reimplements 0x4a6fe0: zVideo_buff::CopySurfaceRectToImage.
 * Original source path: GameZRecoil/zImage/zvid_buff.c.
 * Purpose: provide the recovered zVideo_buff::CopySurfaceRectToImage behavior.
 */
zVidImagePartial *__fastcall CopySurfaceRectToImage(
    int sourceSelector,
    zVidRect32 *rect,
    zVidImagePartial *imageOrNull
) {
    zVideo_SurfaceStatePartial *surfaceState = 0;
    if (sourceSelector == 0) {
        surfaceState = &g_zVideo_SwSurfaceState;
    } else if (sourceSelector == 1) {
        surfaceState = &g_zVideo_PrimarySurfaceState;
    } else if (sourceSelector == 2) {
        surfaceState = &g_zVideo_DisplayModeSurfaceState;
    } else {
        return 0;
    }

    const int surfaceWidth = surfaceState->width;
    const int surfaceHeight = surfaceState->height;
    const int pitchWords = surfaceState->pitch >> 1;
    unsigned char *const surfacePixels = (unsigned char *)(surfaceState->pixels);

    int dstOffsetX = 0;
    int dstOffsetY = 0;
    const int originalWidth = rect->right - rect->left;

    int clipped = ClipCoordToRange(
        &rect->left,
        0,
        surfaceWidth
    );
    if (clipped < 0) {
        dstOffsetX = -clipped;
    } else if (clipped > 0) {
        return 0;
    }

    clipped = ClipCoordToRange(
        &rect->right,
        0,
        surfaceWidth
    );
    if (clipped < 0) {
        return 0;
    }

    clipped = ClipCoordToRange(
        &rect->top,
        0,
        surfaceHeight
    );
    if (clipped < 0) {
        dstOffsetY = -clipped;
    } else if (clipped > 0) {
        return 0;
    }

    clipped = ClipCoordToRange(
        &rect->bottom,
        0,
        surfaceHeight
    );
    if (clipped < 0) {
        return 0;
    }

    const int clippedWidth = rect->right - rect->left;
    const int clippedHeight = rect->bottom - rect->top;
    if (clippedWidth <= 0 || clippedHeight <= 0) {
        return 0;
    }

    zVidImagePartial *image = imageOrNull;
    if (image == 0) {
        image = zVid_Image::Create();
        if (image == 0) {
            return 0;
        }

        zVid_Image::SetSize(
            image,
            (short)(clippedHeight),
            (short)(clippedWidth)
        );
        image->pixels = malloc((size_t)(image->pixelCount) * sizeof(unsigned short));
    }

    unsigned char *dstBytes = (unsigned char *)(image->pixels) +
                              (originalWidth * dstOffsetY + dstOffsetX) * sizeof(unsigned short);
    unsigned char *srcBytes =
        surfacePixels + (pitchWords * rect->top + rect->left) * sizeof(unsigned short);
    const int rowBytes = clippedWidth * (int)(sizeof(unsigned short));
    const int dstStrideBytes = originalWidth * (int)(sizeof(unsigned short));
    const int srcStrideBytes = pitchWords * (int)(sizeof(unsigned short));

    {
        for (int row = clippedHeight; row > 0; --row) {
            memcpy(
                dstBytes,
                srcBytes,
                (size_t)(rowBytes)
            );
            dstBytes += dstStrideBytes;
            srcBytes += srcStrideBytes;
        }
    }

    return image;
}

/**
 * Reimplements 0x4a69e0: zVideo_buff::BltSourceToPrimaryClipped.
 * Purpose: provide the recovered zVideo_buff::BltSourceToPrimaryClipped behavior.
 */
void __fastcall BltSourceToPrimaryClipped(
    zVidImagePartial *srcImage,
    int dstX,
    int dstY,
    int srcColorKeyEnable,
    zVidRect32 *srcRect
) {
    zVidRect32 srcRectLocal;
    int srcX;
    int srcY;
    int srcRight;
    int srcBottom;
    if (srcRect != 0) {
        srcX = srcRect->left;
        srcY = srcRect->top;
        srcRight = srcRect->right;
        srcBottom = srcRect->bottom;
        srcRectLocal.left = srcX;
        srcRectLocal.top = srcY;
        srcRectLocal.right = srcRight;
    } else {
        srcRight = srcImage->width;
        srcBottom = srcImage->height;
        srcX = 0;
        srcY = 0;
        srcRectLocal.left = srcX;
        srcRectLocal.top = srcY;
        srcRectLocal.right = srcRight;
    }

    srcRectLocal.bottom = srcBottom;

    zVidRect32 dstRectLocal;
    dstRectLocal.left = dstX;
    dstRectLocal.top = dstY;
    dstRectLocal.right = srcRight - srcX + dstX;
    dstRectLocal.bottom = srcBottom - srcY + dstY;

    int clipped = ClipCoordToRange(
        &dstRectLocal.left,
        0,
        g_zVideo_PrimarySurfaceState.width - 1
    );
    if (clipped < 0) {
        srcRectLocal.left -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped = ClipCoordToRange(
        &dstRectLocal.right,
        0,
        g_zVideo_PrimarySurfaceState.width
    );
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.right -= clipped;
    }

    clipped = ClipCoordToRange(
        &dstRectLocal.top,
        0,
        g_zVideo_PrimarySurfaceState.height - 1
    );
    if (clipped < 0) {
        srcRectLocal.top -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped = ClipCoordToRange(
        &dstRectLocal.bottom,
        0,
        g_zVideo_PrimarySurfaceState.height
    );
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.bottom -= clipped;
    }

    IDirectDrawSurface3 *const primarySurface = g_zVideo_PrimarySurfaceState.surf;
    if (primarySurface == 0) {
        return;
    }

    const int wasLocked = g_zVideo_PrimarySurfaceState.locked;
    if (wasLocked != 0) {
        zVideo_dd::UnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    const DWORD bltFlags = DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE |
                           ((srcImage->formatFlagsPacked & 0x02u) != 0 ? DDBLT_KEYSRC : 0);
    const HRESULT hresult =
        primarySurface
            ->Blt(
                (RECT *)&dstRectLocal,
                srcImage->surface,
                (RECT *)&srcRectLocal,
                bltFlags,
                0
            );

    if (wasLocked != 0) {
        zVideo_dd::LockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    if (hresult != DD_OK) {
        zVideo_dd::ReportError(
            (int)(hresult),
            ::g_zVideo_SourceFile_ZvidBuffC,
            0x150
        );
    }
}
} // namespace zVideo_buff

namespace zVideo {

/**
 * Reimplements 0x4a6bf0: zVideo::PixelPack_SetupFromMasks.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: initialize the global display pixel-pack bit counts, masks, and
 * shifted channel masks from DirectDraw pixel-format masks.
 */
void __fastcall PixelPack_SetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask
) {
    const int redMaskShifted = ((1 << redBits) - 1) << (8 - redBits);
    const int greenBlueBits = greenBits + blueBits;
    g_zVideo_PixelPack.rMask = redMask;
    g_zVideo_PixelPack.gMask = greenMask;
    g_zVideo_PixelPack.bMask = blueMask;
    const int packedBase = redBits + greenBlueBits - 8;
    const int sumMinus8 = greenBlueBits - 8;
    g_zVideo_PixelPack.rBits = redBits;
    g_zVideo_PixelPack.sumMinus8 = sumMinus8;
    g_zVideo_PixelPack.packedBase = packedBase;
    g_zVideo_PixelPack.gBits = greenBits;
    g_zVideo_PixelPack.bShiftTo8 = 8 - blueBits;
    g_zVideo_PixelPack.bBits = blueBits;
    g_zVideo_PixelPack.rMaskShifted = redMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = ((1 << greenBits) - 1) << (8 - greenBits);
    g_zVideo_PixelPack.bMaskShifted = ((1 << blueBits) - 1) << (8 - blueBits);
}

/**
 * Reimplements 0x4a6db0: zVideo::TexturePixelPack_SetupFromMasks.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: initializes the global texture pixel-pack bit counts, masks,
 * shifted channel masks, and inverse non-RGB shifted mask.
 *
 * Evidence: BN assembly writes the contiguous texture pixel-pack globals at
 * 0x632188..0x6321c4 from the RGB/A bit widths and masks; HLIL's low-byte
 * shift rendering is a decompiler artifact, while assembly uses 32-bit shift
 * counts.
 */
void __fastcall TexturePixelPack_SetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    int alphaBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask,
    unsigned int alphaMask
) {
    g_zVideo_TexturePixelPack_ABits = alphaBits;
    g_zVideo_TexturePixelPack_RMask = redMask;
    g_zVideo_TexturePixelPack_AMask = alphaMask;
    g_zVideo_TexturePixelPack_BMask = blueMask;
    const int greenBlueBits = greenBits + blueBits;
    g_zVideo_TexturePixelPack_GMask = greenMask;
    g_zVideo_TexturePixelPack_RBits = redBits;
    const int rgbBitsTotal = redBits + greenBlueBits;
    g_zVideo_TexturePixelPack_RGBBitsTotal = rgbBitsTotal;
    g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 = rgbBitsTotal - 8;
    g_zVideo_TexturePixelPack_GBBitsTotalMinus8 = greenBlueBits - 8;
    g_zVideo_TexturePixelPack_GBits = greenBits;
    g_zVideo_TexturePixelPack_BBits = blueBits;
    g_zVideo_TexturePixelPack_BShiftTo8 = 8 - blueBits;
    const int rMaskShifted = ((1 << redBits) - 1) << (8 - redBits);
    g_zVideo_TexturePixelPack_RMaskShifted = rMaskShifted;
    const int gMaskShifted = ((1 << greenBits) - 1) << (8 - greenBits);
    g_zVideo_TexturePixelPack_GMaskShifted = gMaskShifted;
    const int bMaskShifted = ((1 << blueBits) - 1) << (8 - blueBits);
    g_zVideo_TexturePixelPack_BMaskShifted = bMaskShifted;
    g_zVideo_TexturePixelPack_NonRgbMaskShifted = ~(rMaskShifted | gMaskShifted | bMaskShifted);
}

/**
 * Reimplements 0x4a6b40: zVideo::SetRendererTypeAndActivePath.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: updates the active renderer backend globals and returns the
 * previous renderer type.
 *
 * Evidence: BN loads g_zVideo_RendererType at 0x632120, stores the requested
 * backend to g_zVideo_RendererType and g_zVideo_ActiveRendererPath at
 * 0x56bbe8, and returns the old renderer value.
 */
int __fastcall SetRendererTypeAndActivePath(
    int rendererType
) {
    const int previousRendererType = g_zVideo_RendererType;
    g_zVideo_RendererType = rendererType;
    g_zVideo_ActiveRendererPath = rendererType;
    return previousRendererType;
}

/**
 * Reimplements 0x4a71c0: zVideo::SetHalfResAdjustMode.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: update the half-resolution adjustment mode when the current
 * surface configuration allows it.
 */
int __fastcall SetHalfResAdjustMode(
    int mode
) {
    int previousMode;
    if (mode == g_zVideo_HalfResAdjustMode) {
        return mode;
    }

    if (g_zVideo_UseHalfResBackbuffer != 0) {
        return 0;
    }

    // VC5 keeps the compare-loaded previous mode in ecx until this assignment.
    previousMode = g_zVideo_HalfResAdjustMode;
    g_zVideo_HalfResAdjustMode = mode;
    if (mode == 0 && g_zVideo_RendererType == 0) {
        g_zVideo_pfnBltPrimaryToSwRectDirect(
            0,
            0
        );
    }

    return previousMode;
}

/**
 * Reimplements 0x437ef0: zVideo::HandleSoftwareModeHotkeyCommand.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: cycle the software-mode hotkey presets while preserving HUD state.
 *
 * Evidence: BN dispatches on GetVideoModeIndexFromOptions() - 2 and cycles
 * modes 2->4, 3->5, 4->2, and 5->3; only the downscale paths request
 * half-resolution adjustment disablement.
 */
void __fastcall HandleSoftwareModeHotkeyCommand(
    int
) {
    if (g_zVideo_SoftwareModeHotkeyEnabled == 0) {
        return;
    }

    const int previousHudType = zOpt::SetHudTypeForCurrentHwMode(1);
    const int currentModeIndex = zVid::GetVideoModeIndexFromOptions();
    int nextModeIndex = currentModeIndex;
    int halfResAdjustMode = 1;

    switch (currentModeIndex) {
    case 2:
        nextModeIndex = 4;
        break;
    case 3:
        nextModeIndex = 5;
        break;
    case 4:
        nextModeIndex = 2;
        halfResAdjustMode = 0;
        break;
    case 5:
        nextModeIndex = 3;
        halfResAdjustMode = 0;
        break;
    default:
        zOpt::SetHudTypeForCurrentHwMode(previousHudType);
        return;
    }

    if (Init_ApplyModeIndex(nextModeIndex) == 0) {
        zVid::SetVideoModeIndex(nextModeIndex);
        if (zVid::GetAccelerationOption() == 0) {
            SetHalfResAdjustMode(halfResAdjustMode);
        }
    }

    zOpt::SetHudTypeForCurrentHwMode(previousHudType);
}

/**
 * Reimplements 0x4a7200: zVideo::GetPrimarySurfaceRectScratch.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: updates the reusable primary-surface rectangle dimensions and
 * returns its address.
 *
 * Evidence: BN reads g_zVideo_PrimarySurfaceState width/height at 0x632220 and
 * 0x632224, stores them into g_zVideo_PrimarySurfaceRectScratch.right/bottom at
 * 0x56bbd0 and 0x56bbd4, preserves left/top, and returns 0x56bbc8.
 */
zVidRect32 *GetPrimarySurfaceRectScratch() {
    g_zVideo_PrimarySurfaceRectScratch.right = g_zVideo_PrimarySurfaceState.width;
    g_zVideo_PrimarySurfaceRectScratch.bottom = g_zVideo_PrimarySurfaceState.height;
    return &g_zVideo_PrimarySurfaceRectScratch;
}

/**
 * Reimplements 0x4a6710: zVideo::GetSwSurfacePixels.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the current locked software surface pixel pointer.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.pixels at 0x632210.
 */
void *GetSwSurfacePixels() {
    return g_zVideo_SwSurfaceState.pixels;
}

/**
 * Reimplements 0x4a6720: zVideo::GetSwSurfaceWidth.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached software surface width.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.width at 0x632200.
 */
int GetSwSurfaceWidth() {
    return g_zVideo_SwSurfaceState.width;
}

/**
 * Reimplements 0x4a6730: zVideo::GetSwSurfaceHeight.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached software surface height.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.height at 0x632204.
 */
int GetSwSurfaceHeight() {
    return g_zVideo_SwSurfaceState.height;
}

/**
 * Reimplements 0x4a6740: zVideo::GetSwSurfacePitch.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached software surface pitch.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.pitch at 0x632208.
 */
int GetSwSurfacePitch() {
    return g_zVideo_SwSurfaceState.pitch;
}

/**
 * Reimplements 0x4a67e0: zVideo::GetSwSurfaceLockedFlag.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns whether the software surface state currently holds a lock.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.locked at 0x632214.
 */
int GetSwSurfaceLockedFlag() {
    return g_zVideo_SwSurfaceState.locked;
}

/**
 * Reimplements 0x4a67f0: zVideo::GetPrimarySurfacePixels.
 * Purpose: Returns the current primary surface pixel pointer from the recovered surface-state global.
 */
void *GetPrimarySurfacePixels() {
    return g_zVideo_PrimarySurfaceState.pixels;
}

/**
 * Reimplements 0x4a6800: zVideo::GetPrimarySurfaceWidth.
 * Original source path: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Purpose: return the current primary surface width from the recovered surface-state global.
 */
int GetPrimarySurfaceWidth() {
    return g_zVideo_PrimarySurfaceState.width;
}

/**
 * Reimplements 0x4a6810: zVideo::GetPrimarySurfaceHeight.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached primary surface height.
 *
 * Evidence: BN is a leaf load from g_zVideo_PrimarySurfaceState.height at
 * 0x632224.
 */
int GetPrimarySurfaceHeight() {
    return g_zVideo_PrimarySurfaceState.height;
}

/**
 * Reimplements 0x4a6820: zVideo::GetPrimarySurfacePitch.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached primary surface pitch.
 *
 * Evidence: BN is a leaf load from g_zVideo_PrimarySurfaceState.pitch at
 * 0x632228.
 */
int GetPrimarySurfacePitch() {
    return g_zVideo_PrimarySurfaceState.pitch;
}

/**
 * Reimplements 0x4a66e0: zVideo::GetDisplayModeBpp.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached display-mode bits-per-pixel value.
 *
 * Evidence: BN assembly is a leaf load from g_zVideo_DisplayModeBpp at
 * 0x632150 followed by return.
 */
int GetDisplayModeBpp() {
    return g_zVideo_DisplayModeBpp;
}

/**
 * Reimplements 0x4c7fd0: zVideo::LoadPaletteFileAndApplyBrightness.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo::LoadPaletteFileAndApplyBrightness behavior.
 */
int __fastcall LoadPaletteFileAndApplyBrightness(
    const char *palettePath
) {
    if (palettePath != 0) {
        strcpy(
            g_zVideo_PalettePathBuffer,
            palettePath
        );
    }

    FILE *paletteStream = fopen(
        g_zVideo_PalettePathBuffer,
        "rb"
    );
    if (paletteStream == 0) {
        fprintf(
            stderr,
            g_zVideo_PaletteOpenFailedFormat,
            g_zVideo_PalettePathBuffer
        );
        return 0x800;
    }

    fread(
        g_zVideo_PaletteFileEntries,
        3,
        256,
        paletteStream
    );
    fclose(paletteStream);
    return ApplyBrightnessToPaletteEntries(g_zVideo_PaletteFileEntries);
}

/**
 * Reimplements 0x4c8070: zVideo::ApplyBrightnessToPaletteEntries.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo::ApplyBrightnessToPaletteEntries behavior.
 */
int __fastcall ApplyBrightnessToPaletteEntries(
    PALETTEENTRY *paletteEntries
) {
    if (g_zVideo_IsInitialized == 0) {
        return 0x5a560000;
    }

    if (paletteEntries != 0) {
        memcpy(
            g_zVideo_SystemPaletteEntries,
            paletteEntries,
            sizeof(g_zVideo_SystemPaletteEntries)
        );
    }

    PALETTEENTRY adjustedEntries[256];
    memcpy(
        adjustedEntries,
        g_zVideo_SystemPaletteEntries,
        sizeof(adjustedEntries)
    );

    const int brightnessDelta =
        ((int)((unsigned char)g_zVideo_PaletteBrightnessLevel) << 3) - 32;
    if (brightnessDelta > 0) {
        for (int index = 0; index < 256; ++index) {
            const int red = adjustedEntries[index].peRed + brightnessDelta;
            const int green = adjustedEntries[index].peGreen + brightnessDelta;
            const int blue = adjustedEntries[index].peBlue + brightnessDelta;
            adjustedEntries[index].peRed = (BYTE)(red > 255 ? 255 : red);
            adjustedEntries[index].peGreen = (BYTE)(green > 255 ? 255 : green);
            adjustedEntries[index].peBlue = (BYTE)(blue > 255 ? 255 : blue);
        }
    } else if (brightnessDelta < 0) {
        for (int index = 0; index < 256; ++index) {
            const int red = adjustedEntries[index].peRed + brightnessDelta;
            const int green = adjustedEntries[index].peGreen + brightnessDelta;
            const int blue = adjustedEntries[index].peBlue + brightnessDelta;
            adjustedEntries[index].peRed = (BYTE)(red < 0 ? 0 : red);
            adjustedEntries[index].peGreen = (BYTE)(green < 0 ? 0 : green);
            adjustedEntries[index].peBlue = (BYTE)(blue < 0 ? 0 : blue);
        }
    }

    return g_zVideo_pfnPaletteSetEntries(
        0,
        256,
        adjustedEntries
    );
}

/**
 * Reimplements 0x4a7990: zVideo::Init_SetSurfaceGeometryFromModeIndex.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: initializes cached display, primary, and software surface geometry
 * for the selected video mode index.
 *
 * Evidence: BN switches over mode indices 2..7, writes
 * g_zVideo_UseHalfResBackbuffer plus the width/height fields of the three
 * zVideo_SurfaceState records, clears gVideo_resolutionMenuValid for invalid
 * indices, and stores the legacy computed display bpp value at 0x632150.
 */
void __fastcall Init_SetSurfaceGeometryFromModeIndex(
    int modeIndex
) {
    switch (modeIndex) {
    case 2:
        g_zVideo_UseHalfResBackbuffer = 1;
        g_zVideo_DisplayModeSurfaceState.width = 640;
        g_zVideo_PrimarySurfaceState.width = 640;
        g_zVideo_SwSurfaceState.width = 320;
        g_zVideo_SwSurfaceState.height = 200;
        g_zVideo_DisplayModeSurfaceState.height = 400;
        break;
    case 3:
        g_zVideo_UseHalfResBackbuffer = 1;
        g_zVideo_DisplayModeSurfaceState.width = 640;
        g_zVideo_PrimarySurfaceState.width = 640;
        g_zVideo_SwSurfaceState.width = 320;
        g_zVideo_SwSurfaceState.height = 240;
        g_zVideo_DisplayModeSurfaceState.height = 480;
        break;
    case 5:
        g_zVideo_UseHalfResBackbuffer = 0;
        g_zVideo_DisplayModeSurfaceState.width = 640;
        g_zVideo_SwSurfaceState.width = 640;
        g_zVideo_PrimarySurfaceState.width = 640;
        g_zVideo_DisplayModeSurfaceState.height = 480;
        g_zVideo_SwSurfaceState.height = 480;
        break;
    case 4:
        g_zVideo_UseHalfResBackbuffer = 0;
        g_zVideo_DisplayModeSurfaceState.width = 640;
        g_zVideo_SwSurfaceState.width = 640;
        g_zVideo_PrimarySurfaceState.width = 640;
        g_zVideo_DisplayModeSurfaceState.height = 400;
        g_zVideo_SwSurfaceState.height = 400;
        break;
    case 6:
        g_zVideo_UseHalfResBackbuffer = 0;
        g_zVideo_DisplayModeSurfaceState.width = 800;
        g_zVideo_SwSurfaceState.width = 800;
        g_zVideo_PrimarySurfaceState.width = 800;
        g_zVideo_DisplayModeSurfaceState.height = 600;
        g_zVideo_SwSurfaceState.height = 600;
        break;
    case 7:
        g_zVideo_UseHalfResBackbuffer = 0;
        g_zVideo_DisplayModeSurfaceState.width = 1024;
        g_zVideo_SwSurfaceState.width = 1024;
        g_zVideo_PrimarySurfaceState.width = 1024;
        g_zVideo_DisplayModeSurfaceState.height = 768;
        g_zVideo_SwSurfaceState.height = 768;
        break;
    default:
        gVideo_resolutionMenuValid = 0;
        return;
    }

    g_zVideo_PrimarySurfaceState.height = g_zVideo_DisplayModeSurfaceState.height;
    // Original code keeps this legacy 8/16-bpp expression after the valid mode switch.
    int bitsPerPixel = modeIndex <= 1 ? 1 : 0;
    --bitsPerPixel;
    bitsPerPixel &= 8;
    bitsPerPixel += 8;
    g_zVideo_DisplayModeBpp = bitsPerPixel;
}

/**
 * Reimplements 0x4a66f0: zVideo::Init_ApplyModeIndex.
 * Purpose: provide the recovered zVideo::Init_ApplyModeIndex behavior.
 */
int __fastcall Init_ApplyModeIndex(
    int modeIndex
) {
    Init_SetSurfaceGeometryFromModeIndex(modeIndex);
    return g_zVideo_pfnSetVideoMode(modeIndex);
}

/**
 * Reimplements 0x4a7af0: zVideo::SetVideoMode.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_init.c.
 * Purpose: apply cached geometry for a requested video mode and forward the
 * mode switch through the active renderer backend.
 *
 * Evidence: BN checks g_zVideo_IsInitialized, calls
 * Init_SetSurfaceGeometryFromModeIndex, then dispatches through
 * g_zVideo_pfnSetVideoMode with the original mode index.
 */
int __fastcall SetVideoMode(
    int modeIndex
) {
    if (g_zVideo_IsInitialized == 0) {
        return 0x5a560000;
    }

    Init_SetSurfaceGeometryFromModeIndex(modeIndex);
    return g_zVideo_pfnSetVideoMode(modeIndex);
}

/**
 * Reimplements 0x4a6760: zVideo::CallClearSwSurfaceAndZBuffer.
 * Purpose: Tail-dispatches the installed software clear callback with surface
 * and Z-buffer rectangles.
 */
void __fastcall CallClearSwSurfaceAndZBuffer(
    zVidRect32 *surfaceRect,
    zVidRect32 *zRect
) {
    g_zVideo_pfnClearSwSurfaceAndZBuffer(
        surfaceRect,
        zRect
    );
}

/**
 * Reimplements 0x4a6830: zVideo::CallClearPrimarySurfaceAndZBuffer.
 * Purpose: Tail-dispatches the installed primary clear callback with the
 * primary surface state.
 */
void __fastcall CallClearPrimarySurfaceAndZBuffer(
    zVidRect32 *rect
) {
    g_zVideo_pfnClearStateSurfaceAndZBuffer(
        rect,
        &g_zVideo_PrimarySurfaceState
    );
}

/**
 * Reimplements 0x4a7b20: zVideo::ExchangeClearScreenBufferEnabled.
 * Purpose: Swaps the clear-screen-buffer flag and returns the previous value.
 */
int __fastcall ExchangeClearScreenBufferEnabled(
    int enable
) {
    const int previous = g_zVideo_ClearScreenBufferEnabled;
    g_zVideo_ClearScreenBufferEnabled = enable;
    return previous;
}

/**
 * Reimplements 0x4a7b30: zVideo::GetClearScreenBufferEnabled.
 * Purpose: Returns the current clear-screen-buffer flag.
 */
int GetClearScreenBufferEnabled() {
    return g_zVideo_ClearScreenBufferEnabled;
}

/**
 * Reimplements 0x4a68e0: zVideo::Dispatch_LockDisplayModeSurfaceState.
 * Purpose: Dispatches the configured surface lock provider for the display-mode surface state.
 */
int Dispatch_LockDisplayModeSurfaceState() {
    return g_zVideo_pfnLockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
}

/**
 * Reimplements 0x4a68f0: zVideo::Dispatch_UnlockDisplayModeSurfaceState.
 * Purpose: Dispatches the configured surface unlock provider for the display-mode surface state.
 */
int Dispatch_UnlockDisplayModeSurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
}

/**
 * Reimplements 0x4a67d0: zVideo::Dispatch_UnlockSwSurfaceState.
 * Purpose: Dispatches the configured surface unlock provider for the software surface state.
 */
int Dispatch_UnlockSwSurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_SwSurfaceState);
}

/**
 * Reimplements 0x4a68d0: zVideo::Dispatch_UnlockPrimarySurfaceState.
 * Purpose: Dispatches the configured surface unlock provider for the primary surface state.
 */
int Dispatch_UnlockPrimarySurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
}

/**
 * Reimplements 0x48d420: zVideo::Fx_SetSurfaceState.
 * Purpose: Publishes the active FX surface descriptor and derives the 16-bit pitch.
 */
void __fastcall Fx_SetSurfaceState(
    void *pixels,
    int width,
    int height,
    int pitchBytes
) {
    g_zVideo_FxSurfaceWidth = width;
    g_zVideo_FxSurfaceHeight = height;
    g_zVideo_FxSurfacePitchBytes = pitchBytes;
    g_zVideo_FxSurfacePixels16 = (unsigned short *)(pixels);
    g_zVideo_FxSurfacePitchPixels16 = pitchBytes / 2;
}

/**
 * Reimplements 0x48da60: zVideo::FxPass3_CopySurfacePixelToScratchClipped.
 * Source owner evidence: current BN assembly shows a zVideo namespace helper
 * with no direct callees, fastcall destination deltas in ECX/EDX, source deltas
 * on the stack, scratch-offset biasing for both endpoints, and strict clip
 * checks before a single 16-bpp surface-to-scratch copy.
 * Data owner evidence: reads g_zVideo_FxPass3_ScratchOffsetX/Y,
 * g_zVideo_FxPass3_ClipMin/MaxX/Y, g_zVideo_FxSurfacePixels16,
 * g_zVideo_FxSurfacePitchPixels16, g_zVideo_FxSurfaceWidth, and
 * g_zVideo_FxPass3_ScratchPixels16. This slice documents the touched
 * scratch/clip globals but does not prove the complete zVideo data owner.
 * Pass-3 ring warp uses center-relative deltas; this helper applies the current center bias
 * and rejects copies unless both endpoints are in bounds.
 * Purpose: copy one biased 16-bpp FX-surface pixel into pass-3 scratch only
 * when both the source and destination endpoints are inside the active clip.
 */
void __fastcall FxPass3_CopySurfacePixelToScratchClipped(
    int dstDx,
    int dstDy,
    int srcDx,
    int srcDy
) {
    const int dstX = dstDx + g_zVideo_FxPass3_ScratchOffsetX;
    const int dstY = dstDy + g_zVideo_FxPass3_ScratchOffsetY;
    const int srcX = srcDx + g_zVideo_FxPass3_ScratchOffsetX;
    const int srcY = srcDy + g_zVideo_FxPass3_ScratchOffsetY;

    if (dstX < g_zVideo_FxPass3_ClipMinX || dstX >= g_zVideo_FxPass3_ClipMaxX) {
        return;
    }
    if (dstY < g_zVideo_FxPass3_ClipMinY || dstY >= g_zVideo_FxPass3_ClipMaxY) {
        return;
    }
    if (srcX < g_zVideo_FxPass3_ClipMinX || srcX >= g_zVideo_FxPass3_ClipMaxX) {
        return;
    }
    if (srcY < g_zVideo_FxPass3_ClipMinY || srcY >= g_zVideo_FxPass3_ClipMaxY) {
        return;
    }

    g_zVideo_FxPass3_ScratchPixels16[dstY * g_zVideo_FxSurfaceWidth + dstX] =
        g_zVideo_FxSurfacePixels16[srcY * g_zVideo_FxSurfacePitchPixels16 + srcX];
}

/**
 * Original-source helper evidence: no standalone retail address is assigned to
 * this helper shape in current plan/BN evidence; observed in caller
 * zVideo::FxPass3_ApplyToCurrentSurface at 0x48daf0. The BN body clamps the
 * current radius against a non-negative max radius before the early-exit test.
 * Purpose: clamp the pass-3 current radius to the valid [0, max] range.
 */
static int __fastcall zVideoFxPass3ClampCurrentRadius(
    int currentRadius,
    int maxRadius
) {
    int cappedMaxRadius = 0;
    if (maxRadius > 0) {
        cappedMaxRadius = maxRadius;
    }
    if (currentRadius > cappedMaxRadius) {
        currentRadius = cappedMaxRadius;
    }
    if (currentRadius < 0) {
        currentRadius = 0;
    }
    return currentRadius;
}

/**
 * Original-source helper evidence: no standalone retail address is assigned to
 * this helper shape in current plan/BN evidence; observed twice in caller
 * zVideo::FxPass3_ApplyToCurrentSurface at 0x48daf0. BN uses the repeated
 * integer-bit square-root approximation, then clamps the result to maxRadius.
 * Purpose: approximate the radius-table index used by the pass-3 radial warp.
 */
static int __fastcall zVideoFxPass3ApproxRadiusIndex(
    int distanceSquared,
    int maxRadius
) {
    float distanceSquaredFloat = (float)(distanceSquared);
    int bits = *((int *)(&distanceSquaredFloat));
    bits = (bits >> 1) + 0x1fc00000;
    const int radiusIndex = (int)(*((float *)(&bits)));
    if (radiusIndex >= maxRadius) {
        return maxRadius;
    }
    return radiusIndex;
}

/**
 * Original-source helper evidence: no standalone retail address is assigned to
 * this helper shape in current plan/BN evidence; observed in the non-clipped
 * scatter path of zVideo::FxPass3_ApplyToCurrentSurface at 0x48daf0. BN uses
 * center-relative deltas and direct pointer indexing with surface pitch for the
 * source and tight surface width for scratch.
 * Purpose: copy one pass-3 sample through the direct in-bounds scatter path.
 */
static void __fastcall zVideoFxPass3CopyDirect(
    int centerX,
    int centerY,
    int dstDx,
    int dstDy,
    int srcDx,
    int srcDy
) {
    g_zVideo_FxPass3_ScratchPixels16[
        (centerY + dstDy) * g_zVideo_FxSurfaceWidth + centerX + dstDx
    ] = g_zVideo_FxSurfacePixels16[
        (centerY + srcDy) * g_zVideo_FxSurfacePitchPixels16 + centerX + srcDx
    ];
}

/**
 * Original-source helper evidence: no standalone retail address is assigned to
 * this helper shape in current plan/BN evidence; observed as the repeated
 * eight-way direct scatter pattern in zVideo::FxPass3_ApplyToCurrentSurface at
 * 0x48daf0.
 * Purpose: scatter a direct pass-3 sample to the eight mirrored ring positions.
 */
static void __fastcall zVideoFxPass3ScatterDirectSymmetric(
    int centerX,
    int centerY,
    int x,
    int y,
    int srcX,
    int srcY
) {
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        x,
        y,
        srcX,
        srcY
    );
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        y,
        x,
        srcY,
        srcX
    );
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        -x,
        y,
        -srcX,
        srcY
    );
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        y,
        -x,
        srcY,
        -srcX
    );
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        x,
        -y,
        srcX,
        -srcY
    );
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        -y,
        x,
        -srcY,
        srcX
    );
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        -x,
        -y,
        -srcX,
        -srcY
    );
    zVideoFxPass3CopyDirect(
        centerX,
        centerY,
        -y,
        -x,
        -srcY,
        -srcX
    );
}

/**
 * Original-source helper evidence: no standalone retail address is assigned to
 * this helper shape in current plan/BN evidence; observed as the repeated
 * eight-call clipped scatter pattern in zVideo::FxPass3_ApplyToCurrentSurface
 * at 0x48daf0, with each arm calling the address-backed helper at 0x48da60.
 * Purpose: scatter a pass-3 sample to eight mirrored ring positions through
 * the active clip bounds.
 */
static void __fastcall zVideoFxPass3ScatterClippedSymmetric(
    int x,
    int y,
    int srcX,
    int srcY
) {
    FxPass3_CopySurfacePixelToScratchClipped(
        x,
        y,
        srcX,
        srcY
    );
    FxPass3_CopySurfacePixelToScratchClipped(
        y,
        x,
        srcY,
        srcX
    );
    FxPass3_CopySurfacePixelToScratchClipped(
        -x,
        y,
        -srcX,
        srcY
    );
    FxPass3_CopySurfacePixelToScratchClipped(
        y,
        -x,
        srcY,
        -srcX
    );
    FxPass3_CopySurfacePixelToScratchClipped(
        x,
        -y,
        srcX,
        -srcY
    );
    FxPass3_CopySurfacePixelToScratchClipped(
        -y,
        x,
        -srcY,
        srcX
    );
    FxPass3_CopySurfacePixelToScratchClipped(
        -x,
        -y,
        -srcX,
        -srcY
    );
    FxPass3_CopySurfacePixelToScratchClipped(
        -y,
        -x,
        -srcY,
        -srcX
    );
}

/**
 * Original-source helper evidence: no standalone retail address is assigned to
 * this helper shape in current plan/BN evidence; observed at the tail of
 * zVideo::FxPass3_ApplyToCurrentSurface at 0x48daf0. BN copies a bounded
 * scratch region back to the active FX surface while skipping coordinates that
 * remain inside the current radius.
 * Purpose: copy the staged pass-3 scratch region back to the active FX surface.
 */
static void __fastcall zVideoFxPass3CopyScratchToSurface(
    int minX,
    int minY,
    int maxX,
    int maxY,
    int currentRadius
) {
    int y;
    for (y = minY; y < maxY; ++y) {
        if (y > currentRadius || y < -currentRadius) {
            unsigned short *src =
                g_zVideo_FxPass3_ScratchPixels16 + y * g_zVideo_FxSurfaceWidth + minX;
            unsigned short *dst =
                g_zVideo_FxSurfacePixels16 + y * g_zVideo_FxSurfacePitchPixels16 + minX;
            int x;
            for (x = minX; x < maxX; ++x) {
                if (x > currentRadius || x < -currentRadius) {
                    *dst = *src;
                }
                ++dst;
                ++src;
            }
        }
    }
}

/**
 * Reimplements 0x48daf0: zVideo::FxPass3_ApplyToCurrentSurface.
 * Source owner evidence: current BN assembly identifies the original file as
 * GameZRecoil/zVideo/zVideo.cpp and shows the complete local pass-3 ring-warp
 * source cluster: radius clamp, two alloca float tables, optional clipped
 * helper path through 0x48da60, direct in-bounds scatter path, and final
 * scratch-to-surface copy. There is no C++ object/table ownership in this
 * helper cluster.
 * Data owner evidence: writes the pass-3 clip globals on every active pass and
 * writes g_zVideo_FxPass3_ScratchOffsetX/Y only for the clipped helper path;
 * it also consumes the active FX surface descriptor and scratch pointer. The
 * complete zVideo data owner remains broader than this function pair.
 * Animated radial ring warp for local pass-3 effects. The retail code keeps a fast direct path
 * when the whole ring fits the clip and falls back to the clipped pixel helper when any
 * endpoint can cross the active rectangle.
 * Purpose: apply the local pass-3 animated radial ring warp to the active
 * 16-bpp FX surface.
 */
void __fastcall FxPass3_ApplyToCurrentSurface(
    int centerX,
    int centerY,
    int currentRadius,
    int maxRadius,
    int extent,
    float sinFreq,
    float sinPhase,
    zVidRect32 *clipRectOrNull
) {
    const int cappedMaxRadius = maxRadius > 0 ? maxRadius : 0;
    currentRadius = zVideoFxPass3ClampCurrentRadius(
        currentRadius,
        maxRadius
    );
    if (currentRadius == cappedMaxRadius) {
        return;
    }

    const int currentRadiusSquared = currentRadius * currentRadius;
    const int maxRadiusSquared = cappedMaxRadius * cappedMaxRadius;
    int clipMinX;
    int clipMinY;
    int clipMaxX;
    int clipMaxY;
    if (clipRectOrNull != 0) {
        clipMinX = clipRectOrNull->left;
        clipMinY = clipRectOrNull->top;
        clipMaxX = clipRectOrNull->right;
        clipMaxY = clipRectOrNull->bottom;
    } else {
        clipMinX = 0;
        clipMinY = 0;
        clipMaxX = g_zVideo_FxSurfaceWidth - 1;
        clipMaxY = g_zVideo_FxSurfaceHeight - 1;
    }

    g_zVideo_FxPass3_ClipMinX = clipMinX;
    g_zVideo_FxPass3_ClipMinY = clipMinY;
    g_zVideo_FxPass3_ClipMaxX = clipMaxX;
    g_zVideo_FxPass3_ClipMaxY = clipMaxY;

    const int minOuterX = centerX - cappedMaxRadius - extent;
    const int maxOuterX = centerX + cappedMaxRadius + extent;
    const int minOuterY = centerY - cappedMaxRadius - extent;
    const int maxOuterY = centerY + cappedMaxRadius + extent;
    if (minOuterX > clipMaxX || maxOuterX < clipMinX || minOuterY > clipMaxY ||
        maxOuterY < clipMinY) {
        return;
    }

    float *sinAmpTable = (float *)(_alloca((cappedMaxRadius + 1) * sizeof(float)));
    float *recipTable = (float *)(_alloca((cappedMaxRadius + 1) * sizeof(float)));
    sinAmpTable[0] = (float)(sin(sinPhase) * (double)(extent));
    recipTable[0] = 1.0f;

    int tableIndex = currentRadius - 1;
    if (tableIndex < 1) {
        tableIndex = 1;
    }
    while (tableIndex <= cappedMaxRadius) {
        const float radius = (float)(tableIndex);
        sinAmpTable[tableIndex] = (float)(sin(radius / sinFreq + sinPhase) * (double)(extent));
        recipTable[tableIndex] = 1.0f / radius;
        ++tableIndex;
    }

    const int useClippedPath =
        minOuterX < clipMinX || maxOuterX >= clipMaxX || minOuterY < clipMinY ||
        maxOuterY >= clipMaxY;
    if (useClippedPath) {
        g_zVideo_FxPass3_ScratchOffsetX = centerX;
        g_zVideo_FxPass3_ScratchOffsetY = centerY;
    }

    int y;
    for (y = -cappedMaxRadius; y <= currentRadius; ++y) {
        int x;
        for (x = y; x <= currentRadius; ++x) {
            const int distanceSquared = x * x + y * y;
            int srcX = x;
            int srcY = y;
            if (distanceSquared < maxRadiusSquared &&
                currentRadiusSquared < distanceSquared) {
                const int radiusIndex = zVideoFxPass3ApproxRadiusIndex(
                    distanceSquared,
                    cappedMaxRadius
                );
                const float scale = sinAmpTable[radiusIndex] * recipTable[radiusIndex];
                srcX = x + (int)((float)(x) * scale);
                srcY = y + (int)((float)(y) * scale);
            }

            if (useClippedPath) {
                zVideoFxPass3ScatterClippedSymmetric(
                    x,
                    y,
                    srcX,
                    srcY
                );
            } else {
                zVideoFxPass3ScatterDirectSymmetric(
                    centerX,
                    centerY,
                    x,
                    y,
                    srcX,
                    srcY
                );
            }
        }
    }

    int copyMinX = centerX - cappedMaxRadius;
    int copyMinY = centerY - cappedMaxRadius;
    int copyMaxX = centerX + cappedMaxRadius;
    int copyMaxY = centerY + cappedMaxRadius;
    if (useClippedPath) {
        if (copyMinY < clipMinY) {
            copyMinY = clipMinY;
        }
        if (copyMaxY > clipMaxY) {
            copyMaxY = clipMaxY;
        }
        if (copyMinX < clipMinX) {
            copyMinX = clipMinX;
        }
        if (copyMaxX > clipMaxX) {
            copyMaxX = clipMaxX;
        }
    }

    zVideoFxPass3CopyScratchToSurface(
        copyMinX,
        copyMinY,
        copyMaxX,
        copyMaxY,
        currentRadius
    );
}

/**
 * Reimplements 0x48e380: zVideo::buff_BlurRegionCombined.
 * Purpose: Applies vertical then horizontal 1-2-1 blur over a 16bpp FX-surface region.
 */
void __fastcall buff_BlurRegionCombined(
    zVidRect32 *rectOrNull,
    int
) {
    int top;
    int left;
    int right;
    int bottom;
    if (rectOrNull != 0) {
        left = rectOrNull->left;
        top = rectOrNull->top;
        right = rectOrNull->right;
        bottom = rectOrNull->bottom;
        if (top < 1) {
            top = 1;
        }
        if (left < 0) {
            left = 0;
        }
        if (bottom > g_zVideo_FxSurfaceHeight - 1) {
            bottom = g_zVideo_FxSurfaceHeight - 1;
        }
        if (right > g_zVideo_FxSurfaceWidth - 1) {
            right = g_zVideo_FxSurfaceWidth - 1;
        }
    } else {
        left = 0;
        top = 1;
        right = g_zVideo_FxSurfaceWidth - 1;
        bottom = g_zVideo_FxSurfaceHeight - 1;
    }

    int savedLeft = left;
    int savedTop = top;
    int savedBottom = bottom;
    int columnCount = right - savedLeft + 1;
    unsigned int blueMask;
    unsigned int redMask;
    unsigned int greenMask;
    unsigned int rbMask;
    PixelPack_GetRgbMasks(
        &redMask,
        &greenMask,
        &blueMask
    );
    rbMask = redMask | blueMask;

    int rowDelta = g_zVideo_FxSurfaceWidth - g_zVideo_FxSurfacePitchPixels16;
    unsigned short *src =
        g_zVideo_FxSurfacePixels16 + savedTop * g_zVideo_FxSurfacePitchPixels16 + savedLeft -
        g_zVideo_FxSurfaceWidth;
    unsigned short *scratch =
        g_zVideo_FxPass3_ScratchPixels16 + savedTop * g_zVideo_FxSurfaceWidth + savedLeft -
        g_zVideo_FxSurfaceWidth;

    if (columnCount > 0) {
        int count = columnCount;
        do {
            *scratch = *src;
            ++src;
            ++scratch;
            --count;
        } while (count != 0);
    }

    src = src + rowDelta;
    scratch = scratch + rowDelta;
    if (savedTop < savedBottom) {
        int rowCount = savedBottom - savedTop;
        do {
            if (columnCount > 0) {
                int count = columnCount;
                do {
                    const unsigned int rb =
                        (src[-g_zVideo_FxSurfaceWidth] & rbMask) +
                        ((*src & rbMask) << 1) +
                        (src[g_zVideo_FxSurfaceWidth] & rbMask);
                    const unsigned int green =
                        (src[-g_zVideo_FxSurfaceWidth] & greenMask) +
                        ((*src & greenMask) << 1) +
                        (src[g_zVideo_FxSurfaceWidth] & greenMask);
                    *scratch = (unsigned short)(((rb >> 2) & rbMask) | ((green >> 2) & greenMask));
                    ++src;
                    ++scratch;
                    --count;
                } while (count != 0);
            }

            src = src + rowDelta;
            scratch = scratch + rowDelta;
            --rowCount;
        } while (rowCount != 0);
    }

    if (columnCount > 0) {
        int count = columnCount;
        do {
            *scratch = *src;
            ++src;
            ++scratch;
            --count;
        } while (count != 0);
    }

    top = savedTop - 1;
    bottom = savedBottom + 1;
    left = savedLeft + 1;
    columnCount -= 2;
    src = g_zVideo_FxSurfacePixels16 + top * g_zVideo_FxSurfacePitchPixels16 + left;
    scratch = g_zVideo_FxPass3_ScratchPixels16 + top * g_zVideo_FxSurfaceWidth + left;
    if (top < bottom) {
        int horizontalRowDelta = rowDelta + 2;
        int rowCount = bottom - top;
        do {
            src[-1] = scratch[-1];
            if (columnCount > 0) {
                int count = columnCount;
                do {
                    const unsigned int rb =
                        (scratch[-1] & rbMask) +
                        ((*scratch & rbMask) << 1) +
                        (scratch[1] & rbMask);
                    const unsigned int green =
                        (scratch[-1] & greenMask) +
                        ((*scratch & greenMask) << 1) +
                        (scratch[1] & greenMask);
                    *src = (unsigned short)(((rb >> 2) & rbMask) | ((green >> 2) & greenMask));
                    ++src;
                    ++scratch;
                    --count;
                } while (count != 0);
            }

            *src = *scratch;
            src = src + horizontalRowDelta;
            scratch = scratch + horizontalRowDelta;
            --rowCount;
        } while (rowCount != 0);
    }
}

/**
 * Reimplements 0x48e670: zVideo::buff_BlurRegionVertical.
 * Purpose: Applies the vertical 1-2-1 blur pass over a 16bpp FX-surface region.
 */
void __fastcall buff_BlurRegionVertical(
    zVidRect32 *rectOrNull,
    int
) {
    int top;
    int left;
    int right;
    int bottom;
    if (rectOrNull != 0) {
        left = rectOrNull->left;
        top = rectOrNull->top;
        right = rectOrNull->right;
        bottom = rectOrNull->bottom;
        if (top < 1) {
            top = 1;
        }
        if (left < 0) {
            left = 0;
        }
        if (bottom > g_zVideo_FxSurfaceHeight - 1) {
            bottom = g_zVideo_FxSurfaceHeight - 1;
        }
        if (right > g_zVideo_FxSurfaceWidth - 1) {
            right = g_zVideo_FxSurfaceWidth - 1;
        }
    } else {
        left = 0;
        top = 1;
        right = g_zVideo_FxSurfaceWidth - 1;
        bottom = g_zVideo_FxSurfaceHeight - 1;
    }

    int columnCount = right - left + 1;
    unsigned int redMask;
    unsigned int greenMask;
    unsigned int blueMask;
    unsigned int rbMask;
    PixelPack_GetRgbMasks(
        &redMask,
        &greenMask,
        &blueMask
    );
    rbMask = redMask | blueMask;

    unsigned short *srcRow =
        g_zVideo_FxSurfacePixels16 + top * g_zVideo_FxSurfacePitchPixels16 + left;
    unsigned short *scratchRow =
        g_zVideo_FxPass3_ScratchPixels16 + top * g_zVideo_FxSurfaceWidth + left;
    const int rowDelta = g_zVideo_FxSurfaceWidth - g_zVideo_FxSurfacePitchPixels16;

    if (top < bottom) {
        int rowCount = bottom - top;
        do {
            unsigned short *src = srcRow;
            unsigned short *scratch = scratchRow;
            if (columnCount > 0) {
                int count = columnCount;
                do {
                    const unsigned int rb =
                        (src[-g_zVideo_FxSurfaceWidth] & rbMask) +
                        ((*src & rbMask) << 1) +
                        (src[g_zVideo_FxSurfaceWidth] & rbMask);
                    const unsigned int green =
                        (src[-g_zVideo_FxSurfaceWidth] & greenMask) +
                        ((*src & greenMask) << 1) +
                        (src[g_zVideo_FxSurfaceWidth] & greenMask);
                    *scratch = (unsigned short)(((rb >> 2) & rbMask) | ((green >> 2) & greenMask));
                    ++src;
                    ++scratch;
                    --count;
                } while (count != 0);
            }

            srcRow = src + rowDelta;
            scratchRow = scratch + rowDelta;
            --rowCount;
        } while (rowCount != 0);
    }

    srcRow = g_zVideo_FxSurfacePixels16 + top * g_zVideo_FxSurfacePitchPixels16 + left;
    scratchRow = g_zVideo_FxPass3_ScratchPixels16 + top * g_zVideo_FxSurfaceWidth + left;
    if (top < bottom) {
        int rowCount = bottom - top;
        do {
            unsigned short *src = srcRow;
            unsigned short *scratch = scratchRow;
            if (columnCount > 0) {
                int count = columnCount;
                do {
                    *src = *scratch;
                    ++src;
                    ++scratch;
                    --count;
                } while (count != 0);
            }

            srcRow = src + rowDelta;
            scratchRow = scratch + rowDelta;
            --rowCount;
        } while (rowCount != 0);
    }
}

/**
 * Reimplements 0x48e870: zVideo::buff_BlurRegionHorizontal.
 * Purpose: Applies the horizontal 1-2-1 blur pass over a 16bpp FX-surface region.
 */
void __fastcall buff_BlurRegionHorizontal(
    zVidRect32 *rectOrNull,
    int
) {
    int top;
    int left;
    int right;
    int bottom;
    if (rectOrNull != 0) {
        top = rectOrNull->top;
        left = rectOrNull->left;
        right = rectOrNull->right;
        bottom = rectOrNull->bottom;
        if (top < 0) {
            top = 0;
        }
        if (left < 1) {
            left = 1;
        }
        if (bottom > g_zVideo_FxSurfaceHeight - 1) {
            bottom = g_zVideo_FxSurfaceHeight - 1;
        }
        if (right > g_zVideo_FxSurfaceWidth - 1) {
            right = g_zVideo_FxSurfaceWidth - 1;
        }
    } else {
        top = 0;
        left = 1;
        bottom = g_zVideo_FxSurfaceHeight - 1;
        right = g_zVideo_FxSurfaceWidth - 1;
    }

    unsigned int redMask;
    unsigned int greenMask;
    unsigned int blueMask;
    unsigned int rbMask;
    ++bottom;
    int columnCount = right - left;
    PixelPack_GetRgbMasks(
        &redMask,
        &greenMask,
        &blueMask
    );
    rbMask = redMask | blueMask;

    unsigned short *src =
        g_zVideo_FxSurfacePixels16 + top * g_zVideo_FxSurfacePitchPixels16 + left;
    unsigned short *scratchRow =
        g_zVideo_FxPass3_ScratchPixels16 + top * g_zVideo_FxSurfaceWidth + left;
    const int rowDelta = g_zVideo_FxSurfaceWidth - g_zVideo_FxSurfacePitchPixels16;
    if (top >= bottom) {
        return;
    }

    int y = bottom - top;
    do {
        unsigned short *srcStart = src;
        unsigned short *scratch = scratchRow;
        if (columnCount > 0) {
            int count = columnCount;
            do {
                const unsigned int rb =
                    (src[-1] & rbMask) +
                    ((src[0] & rbMask) << 1) +
                    (src[1] & rbMask);
                const unsigned int green =
                    (src[-1] & greenMask) +
                    ((src[0] & greenMask) << 1) +
                    (src[1] & greenMask);
                ++src;
                ++scratch;
                scratch[-1] = (unsigned short)(
                    ((rb >> 2) & rbMask) | ((green >> 2) & greenMask)
                );
                --count;
            } while (count != 0);
        }

        src = srcStart;
        scratch = scratchRow;
        if (columnCount > 0) {
            int count = columnCount;
            do {
                *src++ = *scratch++;
                --count;
            } while (count != 0);
        }

        src = src + rowDelta;
        scratchRow = scratch + rowDelta;
        --y;
    } while (y != 0);
}

/**
 * Reimplements 0x48ea00: zVideo::buff_BlurRegionByMode.
 * Purpose: Dispatches a blur-region request to horizontal, vertical, or combined mode.
 */
void __fastcall buff_BlurRegionByMode(
    zVidRect32 *rectOrNull,
    int mode
) {
    if (mode == 1) {
        buff_BlurRegionHorizontal(
            rectOrNull,
            mode
        );
    } else if (mode == 2) {
        buff_BlurRegionVertical(
            rectOrNull,
            mode
        );
    } else {
        buff_BlurRegionCombined(
            rectOrNull,
            mode
        );
    }
}

} // namespace zVideo

/**
 * Reimplements 0x4bee20: zVideoFxPass3Config::QueuePrimitiveRaw.
 * Purpose: Stores the pending primitive surface descriptor in the pass-3 config.
 */
void zVideoFxPass3Config::QueuePrimitiveRaw(
    void *primitive,
    int width,
    int height,
    int pitchBytes
) {
    surfacePixels = (unsigned short *)(primitive);
    surfaceWidth = width;
    surfaceHeight = height;
    surfacePitchBytes = pitchBytes;
}

namespace zVideo {

/**
 * Reimplements 0x4bed30: zVideo::zVideoFxPass3Config_UpdateLocal.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source-shape evidence: the VC5 coverage symbol is the zVideo namespace
 * helper `?zVideoFxPass3Config_UpdateLocal@zVideo@@YIXPAUzVideoFxPass3Config@@M@Z`,
 * with the config object passed explicitly rather than as a C++ member method.
 * Purpose: update the local pass-3 config container and reset its slot queue.
 */
void __fastcall zVideoFxPass3Config_UpdateLocal(
    zVideoFxPass3Config *config,
    float deltaTime
) {
    config->HudUiContainer::UpdateAll(deltaTime);
    config->slotWriteIndex = 0;
}

/**
 * Reimplements 0x4bed50: zVideo::zVideoFxPass3Config_SetPrimaryElementParamsLocal.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source-shape evidence: sibling local-config helpers in this cluster use
 * zVideo namespace `__fastcall` functions with an explicit config pointer;
 * the public zVideo wrapper at 0x4beee0 supplies the singleton.
 * Purpose: arm the root pass-3 element with the primary overlay parameters.
 */
void __fastcall zVideoFxPass3Config_SetPrimaryElementParamsLocal(
    zVideoFxPass3Config *config,
    unsigned int packedColor,
    double primaryAlpha
) {
    config->rootElement.packedColor16 = (unsigned short)(packedColor);
    config->rootElement.alpha = primaryAlpha;
    config->rootElement.SetVisible(1);
    config->rootElement.timer = 0.0f;
    config->rootElement.flags |= 0x01u;
}

/**
 * Reimplements 0x4beee0: zVideo::FxPass3_SetPrimaryElementParamsLocal.
 * Purpose: provide the recovered zVideo::FxPass3_SetPrimaryElementParamsLocal behavior.
 */
void __fastcall FxPass3_SetPrimaryElementParamsLocal(
    unsigned int packedColor,
    double primaryAlpha
) {
    zVideoFxPass3Config_SetPrimaryElementParamsLocal(
        &g_zVideo_FxPass3ConfigLocal,
        packedColor,
        primaryAlpha
    );
}

/**
 * Reimplements 0x4bed90: zVideo::zVideoFxPass3Config_QueueElementLocal.
 * Source-shape evidence: this is the same local-config helper family as
 * 0x4bed30 and keeps the config as an explicit namespace-function parameter;
 * the public zVideo wrapper at 0x4bef10 supplies the singleton.
 * Purpose: queue one local pass-3 slot payload for the next config update.
 */
void __fastcall zVideoFxPass3Config_QueueElementLocal(
    zVideoFxPass3Config *config,
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
) {
    const int slotIndex = config->slotWriteIndex;
    zVideoFxPass3Slot *const slot = &config->slots[slotIndex];
    if (slotIndex < 4) {
        config->slotWriteIndex = slotIndex + 1;
    }

    slot->SetRectAndPayload(
        rectLeftPixels,
        rectTopPixels,
        currentRadiusPixels,
        maxRadiusPixels,
        extentPixels,
        sinFreq,
        sinPhase
    );
    slot->SetVisible(1);
    slot->timer = 0.0f;
    slot->flags |= 0x01u;
}

/**
 * Reimplements 0x4bef10: zVideo::FxPass3_QueueElementLocal.
 * Purpose: provide the recovered zVideo::FxPass3_QueueElementLocal behavior.
 */
void __fastcall FxPass3_QueueElementLocal(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
) {
    zVideoFxPass3Config_QueueElementLocal(
        &g_zVideo_FxPass3ConfigLocal,
        rectLeftPixels,
        rectTopPixels,
        currentRadiusPixels,
        maxRadiusPixels,
        extentPixels,
        sinFreq,
        sinPhase
    );
}

/**
 * Reimplements 0x4bef50: zVideo::FxPass3_QueuePrimitive.
 * Purpose: Queues a primitive descriptor on the global pass-3 FX config.
 */
void __fastcall FxPass3_QueuePrimitive(
    void *primitive,
    int width,
    int height,
    int pitchBytes
) {
    g_zVideo_FxPass3ConfigLocal.QueuePrimitiveRaw(
        primitive,
        width,
        height,
        pitchBytes
    );
}

/**
 * Reimplements 0x4bef40: zVideo::FxPass3_SetInputRectByIndex.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo::FxPass3_SetInputRectByIndex behavior.
 */
void __fastcall FxPass3_SetInputRectByIndex(
    int index,
    HudUiRect *rectOrNull
) {
    g_zVideo_FxPass3ConfigLocal.SetInputRectByIndex(
        index,
        rectOrNull
    );
}

/**
 * Reimplements 0x4bef70: zVideo::FxPass3_UpdateLocal.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo::FxPass3_UpdateLocal behavior.
 */
void __fastcall FxPass3_UpdateLocal(
    float deltaTime
) {
    zVideoFxPass3Config_UpdateLocal(
        &g_zVideo_FxPass3ConfigLocal,
        deltaTime
    );
}

/**
 * Reimplements 0x4a6770: zVideo::RunPostprocessOnSwBuffer.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo::RunPostprocessOnSwBuffer behavior.
 */
void RunPostprocessOnSwBuffer() {
    g_zVideo_pfnLockSurfaceState(&g_zVideo_SwSurfaceState);
    zRndr::SetFrameBufferRegion(
        g_zVideo_SwSurfaceState.pixels,
        0,
        0,
        g_zVideo_SwSurfaceState.pitch
    );
    Fx_SetSurfaceState(
        g_zVideo_SwSurfaceState.pixels,
        g_zVideo_SwSurfaceState.width,
        g_zVideo_SwSurfaceState.height,
        g_zVideo_SwSurfaceState.pitch
    );
    FxPass3_QueuePrimitive(
        g_zVideo_SwSurfaceState.pixels,
        g_zVideo_SwSurfaceState.width,
        g_zVideo_SwSurfaceState.height,
        g_zVideo_SwSurfaceState.pitch
    );
}

/**
 * Reimplements 0x4a6840: zVideo::RunPostprocessOnPrimaryBuffer.
 * Purpose: Runs the pass-3 postprocess pipeline against the primary surface.
 */
int RunPostprocessOnPrimaryBuffer() {
    if (g_zVideo_RendererType != 0 || g_zVideo_UseHalfResBackbuffer != 0) {
        g_zVideo_pfnLockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    zRndr::SetFrameBufferRegion(
        g_zVideo_PrimarySurfaceState.pixels,
        0,
        0,
        g_zVideo_PrimarySurfaceState.pitch
    );
    Fx_SetSurfaceState(
        g_zVideo_PrimarySurfaceState.pixels,
        g_zVideo_PrimarySurfaceState.width,
        g_zVideo_PrimarySurfaceState.height,
        g_zVideo_PrimarySurfaceState.pitch
    );
    FxPass3_QueuePrimitive(
        g_zVideo_PrimarySurfaceState.pixels,
        g_zVideo_PrimarySurfaceState.width,
        g_zVideo_PrimarySurfaceState.height,
        g_zVideo_PrimarySurfaceState.pitch
    );

    if (g_zVideo_UseHalfResBackbuffer != 0) {
        g_zVideo_pfnUnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    return 0;
}

/**
 * Reimplements 0x4a6900: zVideo::PresentOrAdjustSurfacesIfEnabled.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: forward enabled surface-present requests through the renderer dispatch and tick the video frame counter.
 */
int __fastcall AdjustSurfacesIfEnabled(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
) {
    int result = g_zVideo_AdjustSurfacesDisableGate;
    if (result <= 0) {
        result = g_zVideo_pfnAdjustSurfaces(
            srcRect,
            dstRect,
            waitForPresent,
            blitPrimaryToSwFirst
        );
        ++g_zVideo_FrameTick;
    }

    return result;
}

/**
 * Reimplements 0x4a77a0: zVideo::BindRendererDispatch.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: bind renderer-specific zVideo dispatch functions and fullscreen state.
 */
void __fastcall BindRendererDispatch(
    int rendererType,
    int fullscreenOption
) {
    SetRendererTypeAndActivePath(rendererType);
    g_zVideo_FullscreenOption = fullscreenOption;
    g_zVideo_pfnOpenVideoMode = zVideo_dd::OpenVideoMode;
    g_zVideo_pfnShutdownVideoSystem =
        (zVideo_ShutdownVideoSystemProc)(zVideo_dd::ShutdownVideoSystem);
    g_zVideo_pfnPaletteSetEntries = zVideo_dd::PaletteSetEntries;
    g_zVideo_pfnSetVideoMode = zVideo_dd::SetVideoMode;
    g_zVideo_pfnAdjustSurfaces = zVideo_dd3d::PresentDisplayModeSurface;
    if (g_zVideo_RendererType != 1) {
        g_zVideo_pfnAdjustSurfaces = zVideo_dd::PresentDisplayModeSurface;
    }
    g_zVideo_pfnLockSurfaceState = zVideo_dd::LockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = zVideo_dd::UnlockSurfaceState;
    g_zVideo_pfnClearZBufferRect = zVideo_dd::ZBuffer_DepthFillRect;
    g_zVideo_pfnClearSwSurfaceAndZBuffer = zVideo_dd::ClearSwBackbufferAndZBufferRects;
    g_zVideo_pfnClearStateSurfaceAndZBuffer = zVideo_dd::ClearScreenAndZBufferRect;
    g_zVideo_pfnUpdateFogColor = zVideo_dd3d::UpdateFogColor;
    g_zVideo_pfnQueryTextureMemoryBytes = zVid::QueryTextureMemoryBytes;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = zVid::QueryDeviceVideoMemoryBytes;
    g_zVideo_pfnBltSwToPrimaryRectDirect = zVideo_dd::BltSwToPrimaryRectDirect;
    g_zVideo_pfnBltPrimaryToSwRectDirect = zVideo_dd::BltPrimaryToSwRectDirect;
    g_zVideo_pfnBltSwToPrimaryRect = zVideo_dd::BltSwToPrimaryRect;
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = zVideo_dd::GetHwApiDeviceFeatureFlags;
    g_zVideo_pfnImageUploadPixelsToSurface =
        zVideo_dd::Image_UploadPixelsToSurface;
    g_zVideo_pfnImageReleaseSurface = zVideo_dd::Image_ReleaseSurface;
    g_zVideo_pfnCreateTextureRecord = zVideo_dd3d::CreateTextureRecord;
    g_zVideo_pfnTextureRecordLockUploadSurface =
        zVideo_dd3d::TextureRecord_LockUploadSurface;
    g_zVideo_pfnTextureRecordUnlockUploadSurface =
        zVideo_dd3d::TextureRecord_UnlockUploadSurface;
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef =
        zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef;
    g_zVideo_pfnTextureRecordFinalizeUpload =
        zVideo_dd3d::TextureRecord_FinalizeUpload;
    g_zVideo_pfnTextureRecordDestroy = zVideo_dd3d::TextureRecord_Destroy;
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = zGame::ReturnOnlyStub;
    g_zVideo_pfnImageLazyCreateVideoMemorySurface =
        zVideo_dd::Image_LazyCreateVideoMemorySurface;
    g_zVideo_pfnImageEnsureSurfaceForCurrentDevice =
        zVideo_dd::Image_EnsureSurfaceForCurrentDevice;
    g_zVideo_pfnSetFogEnable = zVideo_dd3d::SetFogEnable;
    g_zVideo_pfnSetFogStart = zVideo_dd3d::SetFogStart;
    g_zVideo_pfnSetFogEnd = zVideo_dd3d::SetFogEnd;
    g_zVideo_pfnApplyFogStateFromGlobals = zVideo_dd3d::ApplyFogStateFromGlobals;
    g_zVideo_pfnSubmitPolyFlatColor16 = zVideo_dd3d::SubmitPolyFlatColor16;
    g_zVideo_pfnSubmitPolyGouraudColor16 = zVideo_dd3d::SubmitPolyGouraudColor16;
    g_zVideo_pfnSubmitPolyColorAttr = zVideo_dd3d::SubmitPolyColorAttr;
    g_zVideo_pfnSubmitPolyRenderClass = zVideo_dd3d::SubmitPolyRenderClass;
    g_zVideo_pfnSubmitPolygon = zVideo_dd3d::SubmitPolygon;
    g_zVideo_pfnSubmitPolygonLit = zVideo_dd3d::SubmitPolygonLit;
    g_zVideo_pfnDrawPointColor16 = zVideo_dd3d::DrawPointColor16;
    g_zVideo_pfnFlushSortedPolys = zVideo_dd3d::FlushSortedPolys;
    g_zVideo_pfnFlushOverwritePolys = zVideo_dd3d::FlushOverwritePolys;
    g_zVideo_pfnFlushQuadBatch = zVideo_dd3d::FlushQuadBatch;

    if (g_zVideo_pSelectedHwApiDeviceRecord != 0 &&
        g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0) {
        g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags = 0;
    }
}

/**
 * Reimplements 0x4a8870: zVideo::CommitHwApiDeviceSelection.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: commit an accepted hardware API device as the active renderer
 * backend.
 *
 * Evidence: BN calls BindRendererDispatch(1, 1), indexes
 * g_zVideo_HwApiDeviceTable by the supplied index, stores
 * g_zVideo_pSelectedHwApiDeviceRecord to that record, and stores
 * g_zVideo_pSelectedD3DDeviceInfo to the record's m_d3dDrivers field.
 */
void __fastcall CommitHwApiDeviceSelection(
    int hwApiIndex
) {
    BindRendererDispatch(
        1,
        1
    );
    zVidHwApiDeviceRecordPartial &selected = g_zVideo_HwApiDeviceTable[hwApiIndex];
    g_zVideo_pSelectedHwApiDeviceRecord = &selected;
    g_zVideo_pSelectedD3DDeviceInfo = selected.m_d3dDrivers;
}

/**
 * Reimplements 0x4a7490: zVideo::SelectHwApiDeviceOrFallback.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: select the persisted hardware API device or fall back to the
 * software renderer path.
 *
 * Evidence: BN branches on hwApiIndex == -1; nonnegative indexes tail through
 * CommitHwApiDeviceSelection and return one, while fallback binds software
 * fullscreen dispatch, points g_zVideo_pSelectedHwApiDeviceRecord at table
 * entry zero, clears g_zVideo_pSelectedD3DDeviceInfo, and returns zero.
 */
int __fastcall SelectHwApiDeviceOrFallback(
    int hwApiIndex
) {
    if (hwApiIndex != -1) {
        CommitHwApiDeviceSelection(hwApiIndex);
        return 1;
    }

    BindRendererDispatch(
        0,
        1
    );
    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[0];
    g_zVideo_pSelectedD3DDeviceInfo = 0;
    return 0;
}

/**
 * Reimplements 0x4a75e0: zVideo::ReturnSuccessStub.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: return the zVideo success status for dispatch slots that need no
 * backend-specific action.
 *
 * Evidence: BN is a leaf zero-return function with no callees or globals.
 */
int ReturnSuccessStub() {
    return 0;
}

/**
 * Reimplements 0x4a7530: zVideo::ModuleInit.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: initialize zVideo global defaults, software renderer dispatch,
 * DirectDraw device enumeration, and the process-exit teardown hook.
 *
 * Evidence: BN clears the zVideo global state block, seeds pixel-pack defaults,
 * binds the software fullscreen dispatch, runs DirectDraw startup enumeration,
 * registers zVideo::AtExitReleaseAllInterfacesAndSurfaces with atexit, and
 * returns zero.
 */
int ModuleInit() {
    /*
     * BN emits rep stosd for 0x519fe dwords from g_zVideo_RendererType
     * through g_zVideo_OverwriteQueueBase[0x180].
     */
    memset(
        &g_zVideo_RendererType,
        0,
        0x1467f8
    );

    g_zVideo_FrameTick = 0;
    gVideo_resolutionMenuValid = 0;
    g_zVideo_PaletteBrightnessLevel = 4;
    g_zVideo_ClearColorPacked16 = 0;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_CachedFogModeLightState = 0;
    g_zVideo_PendingDitherEnable = -1;
    g_zVideo_InverseZTolerancePending = 0.0199999996f;
    g_zVideo_D3DAppendFanCloseVertexPending = 0;

    PixelPack_SetupFromMasks(
        0,
        0,
        0,
        0,
        0,
        0
    );
    TexturePixelPack_SetupFromMasks(
        4,
        4,
        4,
        4,
        0xf000,
        0x0f00,
        0x00f0,
        0x000f
    );
    BindRendererDispatch(
        0,
        1
    );
    zVideo_dd::StartupEnumerateAndDefaultSelect();
    atexit(AtExitReleaseAllInterfacesAndSurfaces);
    return 0;
}

/**
 * Reimplements 0x4a75f0: zVideo::InitVideoSystem.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_init.c.
 * Purpose: open the requested renderer video mode, initialize backend state,
 * seed hardware texture defaults, and refresh cached client coordinates.
 *
 * Evidence: BN rejects double initialization, stores g_zVideo_hWnd, resets
 * g_zVideo_FrameTick, binds renderer dispatch, opens the mode, hides the cursor,
 * marks initialization, calls zVideo::SetVideoMode, creates the hardware
 * default texture record with a null texture name and zero flags, seeds
 * quad-batch specular values for hardware renderers, and calls
 * zVideo::UpdateCachedClientRectScreenCoords.
 */
int __fastcall InitVideoSystem(
    HWND hWnd,
    int rendererBackend,
    int fullscreen,
    int modeIndex
) {
    if (g_zVideo_IsInitialized != 0) {
        return 0x5a560001;
    }

    g_zVideo_hWnd = hWnd;
    g_zVideo_FrameTick = 0;
    BindRendererDispatch(
        rendererBackend,
        fullscreen
    );

    const int openResult = g_zVideo_pfnOpenVideoMode(modeIndex);
    if (openResult != 0) {
        zError::ReportOld(
            0x800,
            g_zVideo_SourceFile_ZvidInitC,
            0x7a,
            g_zVideo_InitFailOpenVideoModeMsg
        );
        return openResult;
    }

    ShowCursor(FALSE);
    g_zVideo_IsInitialized = 1;
    const int setModeResult = SetVideoMode(modeIndex);
    if (setModeResult != 0) {
        zError::ReportOld(
            0x800,
            g_zVideo_SourceFile_ZvidInitC,
            0x86,
            g_zVideo_InitFailSetModeMsg
        );
        ShutdownVideoSystem();
        return setModeResult;
    }

    if (g_zVideo_RendererType != 0) {
        g_zVideo_DefaultTextureRecord = g_zVideo_pfnCreateTextureRecord(
            0,
            &g_zVideo_DefaultTextureImage,
            0,
            0,
            0
        );
        g_zVideo_QuadBatchCount = 0;
        {
            for (int itemIndex = 0; itemIndex < 16; ++itemIndex) {
                zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[itemIndex];
                item.vertices[3].specular = 0xff000000;
                item.vertices[2].specular = 0xff000000;
                item.vertices[1].specular = 0xff000000;
                item.vertices[0].specular = 0xff000000;
            }
        }
    }

    UpdateCachedClientRectScreenCoords();
    return 0;
}

/**
 * Reimplements 0x4a7740: zVideo::ShutdownVideoSystem.
 * Original file: Battlesport/zVideo.cpp.
 * Purpose: shut down the active zVideo backend and restore cursor visibility.
 *
 * Evidence: BN checks g_zVideo_IsInitialized, clears it on the active path,
 * calls g_zVideo_pfnShutdownVideoSystem, calls ShowCursor(TRUE), and returns a
 * zVideo status code.
 */
int ShutdownVideoSystem() {
    if (g_zVideo_IsInitialized == 0) {
        return 0x5a560000;
    }

    g_zVideo_IsInitialized = 0;
    g_zVideo_pfnShutdownVideoSystem();
    ShowCursor(TRUE);
    return 0;
}

/**
 * Reimplements 0x4a7700: zVideo::UpdateCachedClientRectScreenCoords.
 * Original file: Battlesport/zVideo.cpp.
 * Purpose: cache the client rectangle in screen coordinates for the active
 * zVideo window.
 *
 * Evidence: BN calls GetClientRect with g_zVideo_hWnd and
 * g_zVideo_CachedClientRectScreen, then maps the top-left and bottom-right
 * points with ClientToScreen.
 */
int UpdateCachedClientRectScreenCoords() {
    GetClientRect(
        g_zVideo_hWnd,
        &g_zVideo_CachedClientRectScreen
    );
    ClientToScreen(
        g_zVideo_hWnd,
        (POINT *)(&g_zVideo_CachedClientRectScreen.left)
    );
    ClientToScreen(
        g_zVideo_hWnd,
        (POINT *)(&g_zVideo_CachedClientRectScreen.right)
    );
    return 0;
}

/**
 * Reimplements 0x4a7520: zVideo::AtExitReleaseAllInterfacesAndSurfaces.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: release tracked DirectDraw and Direct3D interfaces from the CRT
 * atexit hook registered by zVideo::ModuleInit.
 *
 * Evidence: BN is a tail jump to zVideo_dd::ReleaseAllInterfacesAndSurfaces
 * and the function is passed directly to atexit by zVideo::ModuleInit.
 */
void AtExitReleaseAllInterfacesAndSurfaces() {
    zVideo_dd::ReleaseAllInterfacesAndSurfaces();
}
} // namespace zVideo

namespace zVideo {
/**
 * Reimplements 0x4a7220: zVideo::SetFogColorFromRgb01
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Data evidence: writes the pending fog-color RGB255 globals at
 * 0x6321d0-0x6321d8.
 * Purpose: scale a normalized fog color into pending 255-space video globals.
 */
void __fastcall SetFogColorFromRgb01(
    zVideo_ColorRgbFloat *color
) {
    g_zVideo_FogColorPendingR255 = color->r * 255.0f;
    g_zVideo_FogColorPendingG255 = color->g * 255.0f;
    g_zVideo_FogColorPendingB255 = color->b * 255.0f;
}

/**
 * Reimplements 0x4a7300: zVideo::SetFogTargetColorFromRgb01
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: writes the target fog-color RGB255 globals at
 * 0x6321e8-0x6321f0.
 * Purpose: scale a normalized fog target color into target 255-space video globals.
 */
void __fastcall SetFogTargetColorFromRgb01(
    zVideo_ColorRgbFloat *color
) {
    g_zVideo_FogTargetColorR255 = color->r * 255.0f;
    g_zVideo_FogTargetColorG255 = color->g * 255.0f;
    g_zVideo_FogTargetColorB255 = color->b * 255.0f;
}

/**
 * Reimplements 0x4a7330: zVideo::CommitFogColorIfChanged
 * Source file evidence: zVideo.cpp.
 * Data evidence: compares pending fog RGB255 globals at 0x6321d0-0x6321d8
 * with applied fog RGB255 globals at 0x6321f4-0x6321fc, copies pending to
 * applied on change, then tail-jumps through g_zVideo_pfnUpdateFogColor.
 * Purpose: apply pending fog color values and notify the renderer only when they change.
 */
void CommitFogColorIfChanged() {
    if (g_zVideo_FogColorAppliedR255 == g_zVideo_FogColorPendingR255 &&
        g_zVideo_FogColorAppliedG255 == g_zVideo_FogColorPendingG255 &&
        g_zVideo_FogColorAppliedB255 == g_zVideo_FogColorPendingB255) {
        return;
    }

    g_zVideo_FogColorAppliedR255 = g_zVideo_FogColorPendingR255;
    g_zVideo_FogColorAppliedG255 = g_zVideo_FogColorPendingG255;
    g_zVideo_FogColorAppliedB255 = g_zVideo_FogColorPendingB255;
    g_zVideo_pfnUpdateFogColor();
}

/**
 * Reimplements 0x4a73a0: zVideo::CommitFogTargetColorIfChanged
 * Source file evidence: zVideo.cpp.
 * Data evidence: compares target fog RGB255 globals at 0x6321e8-0x6321f0
 * with applied fog RGB255 globals at 0x6321f4-0x6321fc, copies target to
 * applied on change, then tail-jumps through g_zVideo_pfnUpdateFogColor.
 * Purpose: apply target fog color values and notify the renderer only when they change.
 */
void CommitFogTargetColorIfChanged() {
    if (g_zVideo_FogColorAppliedR255 == g_zVideo_FogTargetColorR255 &&
        g_zVideo_FogColorAppliedG255 == g_zVideo_FogTargetColorG255 &&
        g_zVideo_FogColorAppliedB255 == g_zVideo_FogTargetColorB255) {
        return;
    }

    g_zVideo_FogColorAppliedR255 = g_zVideo_FogTargetColorR255;
    g_zVideo_FogColorAppliedG255 = g_zVideo_FogTargetColorG255;
    g_zVideo_FogColorAppliedB255 = g_zVideo_FogTargetColorB255;
    g_zVideo_pfnUpdateFogColor();
}

/**
 * Reimplements 0x4a6b90: zVideo::PixelPack_GetRgbBits.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: return the cached display RGB channel bit counts.
 */
void __fastcall PixelPack_GetRgbBits(
    int *outRBits,
    int *outGBits,
    int *outBBits
) {
    *outRBits = g_zVideo_PixelPack.rBits;
    *outGBits = g_zVideo_PixelPack.gBits;
    *outBBits = g_zVideo_PixelPack.bBits;
}

/**
 * Reimplements 0x4a6bb0: zVideo::PixelPack_GetRgbMasks
 * Purpose: Return the cached RGB bit masks from the active pixel-pack record.
 */
void __fastcall PixelPack_GetRgbMasks(
    unsigned int *outRMask,
    unsigned int *outGMask,
    unsigned int *outBMask
) {
    *outRMask = g_zVideo_PixelPack.rMask;
    *outGMask = g_zVideo_PixelPack.gMask;
    *outBMask = g_zVideo_PixelPack.bMask;
}

/**
 * Reimplements 0x4a6bd0: zVideo::PixelPack_GetPackingParams.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: return the cached packed RGB shift parameters.
 */
void __fastcall PixelPack_GetPackingParams(
    int *outPackedBase,
    int *outSumMinus8,
    int *outBShiftTo8
) {
    *outPackedBase = g_zVideo_PixelPack.packedBase;
    *outSumMinus8 = g_zVideo_PixelPack.sumMinus8;
    *outBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
}
} // namespace zVideo

namespace zVid {
/**
 * Reimplements 0x48d340: zVid::Noise_InitBuffers
 * Data-gate evidence: BN writes gRndr_pfnOverlayBlendRow to
 * zRndr::OverlayBlendRow555_Scalar after allocating the noise and FX scratch
 * buffers, so data acceptance waits on the zRndr overlay callback owner.
 * Purpose: Allocate the software-noise byte table and FX pass scratch buffer.
 */
void Noise_InitBuffers() {
    const int width = zVideo::GetPrimarySurfaceWidth();
    const int height = zVideo::GetPrimarySurfaceHeight();

    g_zVid_NoiseByteTableSize = width * 0x19;
    g_zVid_NoiseByteTable = (unsigned char *)(malloc((size_t)(g_zVid_NoiseByteTableSize)));
    for (int i = 0; i < g_zVid_NoiseByteTableSize; ++i) {
        g_zVid_NoiseByteTable[i] = (unsigned char)(rand());
    }

    g_zVideo_FxPass3_ScratchPixels16 =
        (unsigned short *)(malloc((size_t)(height * width) * sizeof(unsigned short)));
    g_zVideo_FxSurfacePixels16 = 0;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555_Scalar;
}

/**
 * Reimplements 0x48d3e0: zVid::Noise_ShutdownBuffers.
 * Original source path: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Data owner evidence: current BN loads the noise table pointer, conditionally
 * frees it, loads the pass-3 scratch pointer, clears g_zVid_NoiseByteTable,
 * then conditionally frees and clears g_zVideo_FxPass3_ScratchPixels16.
 * Purpose: release the software-noise byte table and pass-3 scratch buffer.
 */
void Noise_ShutdownBuffers() {
    if (g_zVid_NoiseByteTable != 0) {
        free(g_zVid_NoiseByteTable);
        g_zVid_NoiseByteTable = 0;
    }

    if (g_zVideo_FxPass3_ScratchPixels16 != 0) {
        free(g_zVideo_FxPass3_ScratchPixels16);
        g_zVideo_FxPass3_ScratchPixels16 = 0;
    }
}

/**
 * Reimplements 0x48d910: zVid::DrawNoiseRect.
 * Original source path: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Purpose: overlay thresholded grayscale noise on the active FX surface rectangle.
 */
void __fastcall DrawNoiseRect(
    zVidRect32 *rectOrNull,
    double intensity
) {
    if (intensity < 0.00390625) {
        return;
    }

    const int threshold = (int)(intensity * 256.0);
    int xMin = 0;
    int yMin = 0;
    int xMax = g_zVideo_FxSurfaceWidth - 1;
    int yMax = g_zVideo_FxSurfaceHeight - 1;
    if (rectOrNull != 0) {
        xMin = rectOrNull->left;
        yMin = rectOrNull->top;
        xMax = rectOrNull->right;
        yMax = rectOrNull->bottom;
    }

    const int rowWidth = xMax - xMin;
    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    zVideo::PixelPack_GetRgbBits(
        &rBits,
        &gBits,
        &bBits
    );

    int gShift = bBits;
    const int rShift = bBits + gBits;
    if (gBits == 6) {
        ++gShift;
    }

    for (int y = yMin; y < yMax; ++y) {
        const int noiseRange = g_zVid_NoiseByteTableSize - rowWidth;
        unsigned char *noiseBytes = g_zVid_NoiseByteTable + (rand() * noiseRange) / 0x7fff;
        unsigned short *dstPixels =
            g_zVideo_FxSurfacePixels16 + y * g_zVideo_FxSurfacePitchPixels16 + xMin;

        for (int x = 0; x < rowWidth; ++x) {
            const unsigned char noiseValue = noiseBytes[x];
            if (noiseValue < threshold) {
                const unsigned short level = (unsigned short)(noiseValue & 0x1f);
                dstPixels[x] = (unsigned short)((level << rShift) | (level << gShift) | level);
            }
        }
    }
}

/**
 * Reimplements 0x48ff70: zVid::InitFrameScratchBuffers.
 * Original source path: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Purpose: initialize noise buffers and select the active renderer span routine table.
 */
int InitFrameScratchBuffers() {
    Noise_InitBuffers();
    zRndr::SelectSpanRoutines();
    return 0;
}

/**
 * Reimplements 0x48ff60: zVid::ShutdownFrameScratchBuffers.
 * Original source path: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Purpose: release the frame scratch and noise buffers used by software video effects.
 */
int ShutdownFrameScratchBuffers() {
    Noise_ShutdownBuffers();
    return 0;
}
} // namespace zVid

namespace zVideo_FxSurface {
/**
 * Original-source helper evidence: no standalone retail function; repeated
 * float-to-int truncation in 0x48ed60 uses the VC5 _ftol lowering pattern.
 * BN shows these as namespace functions in zVideo.cpp with no constructor, vtable, or owned
 * object layout evidence. Model this slice as a source-file namespace cluster over the typed
 * FX-surface globals above.
 * Purpose: truncate FX line-clipping intermediates toward zero.
 */
static int TruncateFloat(
    float value
) {
    return (int)(value);
}

/**
 * Original-source helper evidence: no standalone retail function; 0x48ed60
 * repeats the same Cohen-Sutherland outcode tests for both line endpoints.
 * Purpose: classify an FX line endpoint against the clipped rectangle.
 */
static int FxLineOutCode(
    int x,
    int y,
    int left,
    int top,
    int right,
    int bottom
) {
    int outCode = 0;
    if (x < left) {
        outCode |= 1;
    }
    if (x > right) {
        outCode |= 2;
    }
    if (y < top) {
        outCode |= 4;
    }
    if (y > bottom) {
        outCode |= 8;
    }
    return outCode;
}

/**
 * Original-source helper evidence: no standalone retail function; 0x48ed60
 * inlines this RGB565 alpha blend in both major-axis line loops.
 * Purpose: alpha-blend one RGB565 FX-surface pixel.
 */
static unsigned short BlendFxSurfacePixel565(
    unsigned short dst,
    unsigned short color,
    int alpha
) {
    const int dstValue = (int)(dst);
    const int colorValue = (int)(color);
    const int redDelta = (((colorValue & 0xf800) - (dstValue & 0xf800)) * alpha) >> 8;
    const int greenDelta = (((colorValue & 0x07e0) - (dstValue & 0x07e0)) * alpha) >> 8;
    const int redApplied = dstValue + (redDelta & 0xfffff800);
    const int blueDelta = (((colorValue & 0x001f) - (redApplied & 0x001f)) * alpha) >> 8;
    return (unsigned short)(redApplied + (greenDelta & 0xffe0) + blueDelta);
}

/**
 * Original-source helper evidence: no standalone retail function; 0x48ed60
 * inlines this RGB555 alpha blend in both major-axis line loops.
 * Purpose: alpha-blend one RGB555 FX-surface pixel.
 */
static unsigned short BlendFxSurfacePixel555(
    unsigned short dst,
    unsigned short color,
    int alpha
) {
    const int dstValue = (int)(dst);
    const int colorValue = (int)(color);
    const int redDelta = (((colorValue & 0x7c00) - (dstValue & 0x7c00)) * alpha) >> 8;
    const int greenDelta = (((colorValue & 0x03e0) - (dstValue & 0x03e0)) * alpha) >> 8;
    const int blueDelta = (((colorValue & 0x001f) - (dstValue & 0x001f)) * alpha) >> 8;
    return (unsigned short)(dstValue + (redDelta & 0xfc00) + (greenDelta & 0xffe0) + blueDelta);
}

/**
 * Original-source helper evidence: no standalone retail function; 0x48ed60
 * inlines the same RGB555/RGB565 threshold and solid-write branches in both
 * major-axis line loops.
 * Purpose: draw one alpha-controlled span pixel for an FX line.
 */
static void DrawFxSurfaceSpanPixel(
    unsigned short *pixel,
    unsigned short color,
    int alpha
) {
    if (zRndr::g_pixelPackGreenBits == 5) {
        if (alpha <= 7) {
            return;
        }
        if (alpha >= 252) {
            *pixel = color;
            return;
        }
        *pixel = BlendFxSurfacePixel555(
            *pixel,
            color,
            alpha
        );
        return;
    }

    if (alpha <= 3) {
        return;
    }
    if (alpha >= 252) {
        *pixel = color;
        return;
    }
    *pixel = BlendFxSurfacePixel565(
        *pixel,
        color,
        alpha
    );
}

/**
 * Reimplements 0x48ea20: zVideo_FxSurface::ApplyBlueTintRect.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo_FxSurface::ApplyBlueTintRect behavior.
 */
void __fastcall ApplyBlueTintRect(
    zVidRect32 *rectOrNull
) {
    zVidRect32 clipRect;
    if (rectOrNull != 0) {
        clipRect.left = rectOrNull->left;
        clipRect.top = rectOrNull->top;
        clipRect.right = rectOrNull->right;
        clipRect.bottom = rectOrNull->bottom;
    } else {
        clipRect.left = 0;
        clipRect.top = 0;
        clipRect.right = g_zVideo_FxSurfaceWidth - 1;
        clipRect.bottom = g_zVideo_FxSurfaceHeight - 1;
    }

    unsigned int redMask;
    unsigned int greenMask;
    unsigned int blueMask;
    zVideo::PixelPack_GetRgbMasks(
        &redMask,
        &greenMask,
        &blueMask
    );

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo_dd3d::QueueSolidQuad(
            blueMask,
            &clipRect,
            0.3
        );
        return;
    }

    const unsigned int pairedGreenMask = greenMask | (greenMask << 16);
    const unsigned int pairedRedMask = redMask | (redMask << 16);
    const unsigned int pairedBlueMask = blueMask | (blueMask << 16);
    const unsigned int halvedRedGreenMask =
        ((pairedGreenMask >> 1) & pairedGreenMask) | ((pairedRedMask >> 1) & pairedRedMask);

    unsigned short *row =
        g_zVideo_FxSurfacePixels16 + clipRect.top * g_zVideo_FxSurfacePitchPixels16 + clipRect.left;
    const int rowPairCount = (clipRect.right - clipRect.left - 1) >> 1;
    int y = clipRect.top;
    if (y >= clipRect.bottom) {
        return;
    }

    do {
        unsigned int *pixelPair = (unsigned int *)(row);
        int remainingPairs = rowPairCount;
        do {
            const unsigned int value = *pixelPair;
            *pixelPair = (((value >> 1) & halvedRedGreenMask) | (value & pairedBlueMask));
            ++pixelPair;
        } while (remainingPairs-- != 0);

        row += g_zVideo_FxSurfacePitchPixels16;
        ++y;
    } while (y < clipRect.bottom);
}

/**
 * Reimplements 0x48eb80: zVideo_FxSurface::ApplyGreenMaskRect.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo_FxSurface::ApplyGreenMaskRect behavior.
 */
void __fastcall ApplyGreenMaskRect(
    zVidRect32 *rectOrNull
) {
    zVidRect32 clipRect;
    if (rectOrNull != 0) {
        clipRect.left = rectOrNull->left;
        clipRect.top = rectOrNull->top;
        clipRect.right = rectOrNull->right;
        clipRect.bottom = rectOrNull->bottom;
    } else {
        clipRect.left = 0;
        clipRect.top = 0;
        clipRect.right = g_zVideo_FxSurfaceWidth - 1;
        clipRect.bottom = g_zVideo_FxSurfaceHeight - 1;
    }

    unsigned int redMask;
    unsigned int greenMask;
    unsigned int blueMask;
    zVideo::PixelPack_GetRgbMasks(
        &redMask,
        &greenMask,
        &blueMask
    );

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo_dd3d::QueueSolidQuad(
            greenMask,
            &clipRect,
            0.3
        );
        return;
    }

    const unsigned int pairedGreenMask = greenMask | (greenMask << 16);
    unsigned short *row =
        g_zVideo_FxSurfacePixels16 + clipRect.top * g_zVideo_FxSurfacePitchPixels16 + clipRect.left;
    const int rowPairCount = (clipRect.right - clipRect.left - 1) >> 1;
    int y = clipRect.top;
    if (y >= clipRect.bottom) {
        return;
    }

    do {
        unsigned int *pixelPair = (unsigned int *)(row);
        int remainingPairs = rowPairCount;
        do {
            *pixelPair &= pairedGreenMask;
            ++pixelPair;
        } while (remainingPairs-- != 0);

        row += g_zVideo_FxSurfacePitchPixels16;
        ++y;
    } while (y < clipRect.bottom);
}

/**
 * Reimplements 0x48ed60: zVideo_FxSurface::DrawAlphaBlendedLine.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo_FxSurface::DrawAlphaBlendedLine behavior.
 */
void __fastcall DrawAlphaBlendedLine(
    zVidRect32 *clipRect,
    int x1,
    int y1,
    int x0,
    int y0,
    unsigned int color16,
    float alphaEnd,
    float alphaStart,
    int clipInset
) {
    int dx = x0 - x1;
    int dy = y0 - y1;
    const int left = clipRect->left + clipInset;
    const int top = clipRect->top + clipInset;
    const int right = clipRect->right - clipInset;
    const int bottom = clipRect->bottom - clipInset;
    int startOutCode = FxLineOutCode(
        x1,
        y1,
        left,
        top,
        right,
        bottom
    );
    int endOutCode = FxLineOutCode(
        x0,
        y0,
        left,
        top,
        right,
        bottom
    );
    if ((startOutCode & endOutCode) != 0) {
        return;
    }

    if ((startOutCode | endOutCode) != 0) {
        float slopeYPerX = 0.0f;
        float slopeXPerY = 0.0f;
        if (dx != 0) {
            slopeYPerX = (float)(dy) / (float)(dx);
        }
        if (dy != 0) {
            slopeXPerY = (float)(dx) / (float)(dy);
        }

        if (x1 < left) {
            y1 += TruncateFloat((float)(left - x1) * slopeYPerX);
            x1 = left;
        }
        dx = x0 - x1;
        dy = y0 - y1;
        if (dx != 0) {
            slopeYPerX = (float)(dy) / (float)(dx);
        } else {
            slopeYPerX = 0.0f;
        }
        if (dy != 0) {
            slopeXPerY = (float)(dx) / (float)(dy);
        } else {
            slopeXPerY = 0.0f;
        }

        if (x1 > right) {
            y1 += TruncateFloat((float)(right - x1) * slopeYPerX);
            x1 = right;
        }
        dx = x0 - x1;
        dy = y0 - y1;
        if (dx != 0) {
            slopeYPerX = (float)(dy) / (float)(dx);
        } else {
            slopeYPerX = 0.0f;
        }
        if (dy != 0) {
            slopeXPerY = (float)(dx) / (float)(dy);
        } else {
            slopeXPerY = 0.0f;
        }

        if (x0 < left) {
            y0 += TruncateFloat((float)(left - x0) * slopeYPerX);
            x0 = left;
        }
        dx = x0 - x1;
        dy = y0 - y1;
        if (dy != 0) {
            slopeXPerY = (float)(dx) / (float)(dy);
        } else {
            slopeXPerY = 0.0f;
        }

        if (x0 > right) {
            y0 += TruncateFloat((float)(right - x0) * slopeYPerX);
            x0 = right;
        }
        dx = x0 - x1;
        dy = y0 - y1;
        if (dy != 0) {
            slopeXPerY = (float)(dx) / (float)(dy);
        } else {
            slopeXPerY = 0.0f;
        }

        if (y1 < top) {
            x1 += TruncateFloat((float)(top - y1) * slopeXPerY);
            y1 = top;
        } else if (y1 > bottom) {
            x1 += TruncateFloat((float)(bottom - y1) * slopeXPerY);
            y1 = bottom;
        }
        dx = x0 - x1;
        dy = y0 - y1;
        if (dy != 0) {
            slopeXPerY = (float)(dx) / (float)(dy);
        } else {
            slopeXPerY = 0.0f;
        }

        if (y0 < top) {
            x0 += TruncateFloat((float)(top - y0) * slopeXPerY);
            y0 = top;
        } else if (y0 > bottom) {
            x0 += TruncateFloat((float)(bottom - y0) * slopeXPerY);
            y0 = bottom;
        }
    }

    dx = x0 - x1;
    dy = y0 - y1;
    const int pitchPixels = zRndr::g_pitchBytes >> 1;
    unsigned short *pixel = g_zVideo_FxSurfacePixels16 + pitchPixels * y1 + x1;
    int yStepPitch = pitchPixels;
    int xStep = 1;
    if (dy < 0) {
        dy = -dy;
        yStepPitch = -yStepPitch;
    }
    if (dx < 0) {
        dx = -dx;
        xStep = -1;
    }

    const unsigned short packedColor = (unsigned short)(color16);
    int alphaFixed = TruncateFloat(alphaStart * 255.0f) << 16;
    if (dx > dy) {
        int err = dx >> 1;
        int steps = dx + 1;
        const int alphaStep =
            TruncateFloat(((alphaEnd - alphaStart) / (float)(steps)) * 16777215.0f);
        while (steps != 0) {
            const int alpha = alphaFixed >> 16;
            if (clipInset > 0) {
                unsigned short *spanPixel = pixel;
                int spanCount = clipInset;
                while (spanCount != 0) {
                    DrawFxSurfaceSpanPixel(
                        spanPixel,
                        packedColor,
                        alpha
                    );
                    spanPixel += yStepPitch;
                    --spanCount;
                }
            }

            pixel += xStep;
            err += dy;
            alphaFixed += alphaStep;
            if (err > dx) {
                err -= dx;
                pixel += yStepPitch;
            }
            --steps;
        }
        return;
    }

    {
        int err = dy >> 1;
        int steps = dy + 1;
        const int alphaStep =
            TruncateFloat(((alphaEnd - alphaStart) / (float)(steps)) * 16777215.0f);
        while (steps != 0) {
            const int alpha = alphaFixed >> 16;
            if (clipInset > 0) {
                unsigned short *spanPixel = pixel;
                int spanCount = clipInset;
                while (spanCount != 0) {
                    DrawFxSurfaceSpanPixel(
                        spanPixel,
                        packedColor,
                        alpha
                    );
                    spanPixel += xStep;
                    --spanCount;
                }
            }

            pixel += yStepPitch;
            err += dx;
            alphaFixed += alphaStep;
            if (err > dy) {
                err -= dy;
                pixel += xStep;
            }
            --steps;
        }
    }
}

/**
 * Reimplements 0x48ec90: zVideo_FxSurface::DrawColoredLinesBatch.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVideo_FxSurface::DrawColoredLinesBatch behavior.
 */
void __fastcall DrawColoredLinesBatch(
    zVideoFxColoredLineRecord *lines,
    int count,
    zVidRect32 *clipRectOrNull
) {
    zVidRect32 clipRect;
    if (clipRectOrNull != 0) {
        clipRect.left = clipRectOrNull->left;
        clipRect.top = clipRectOrNull->top;
        clipRect.right = clipRectOrNull->right;
        clipRect.bottom = clipRectOrNull->bottom;
    } else {
        clipRect.left = 0;
        clipRect.top = 0;
        clipRect.right = g_zVideo_FxSurfaceWidth - 1;
        clipRect.bottom = g_zVideo_FxSurfaceHeight - 1;
    }

    if (clipRect.top < 0) {
        clipRect.top = 0;
    }
    if (clipRect.bottom > g_zVideo_FxSurfaceHeight - 1) {
        clipRect.bottom = g_zVideo_FxSurfaceHeight - 1;
    }
    if (clipRect.left < 0) {
        clipRect.left = 0;
    }
    if (clipRect.right > g_zVideo_FxSurfaceWidth - 1) {
        clipRect.right = g_zVideo_FxSurfaceWidth - 1;
    }

    for (int index = 0; index < count; ++index) {
        zVideoFxColoredLineRecord *line = &lines[index];
        DrawAlphaBlendedLine(
            &clipRect,
            line->x + line->width,
            line->y + line->height,
            line->x,
            line->y,
            line->color16,
            line->alphaEnd,
            line->alphaStart,
            line->clipInset
        );
    }
}
} // namespace zVideo_FxSurface

namespace zVid_Image {
unsigned short g_zImage_DefaultImagePixels[64] = {
    0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0,
    0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800,
    0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0,
    0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800,
    0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0,
    0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800,
    0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0,
    0x03e0, 0x03e0, 0xf800, 0xf800, 0x03e0, 0x03e0, 0xf800, 0xf800
};

zVidImagePartial g_zImage_DefaultImage = {
    64,
    8,
    8,
    0,
    5,
    0,
    0,
    0,
    0,
    g_zImage_DefaultImagePixels,
    0,
    0,
    0.0f,
    0,
    0,
    0,
    0,
    0,
    0
};

/**
 * Reimplements 0x46ec00: zVid_Image::Create.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: allocate and zero-initialize a zVid image record.
 */
zVidImagePartial *Create() {
    zVidImagePartial *image = (zVidImagePartial *)(malloc(sizeof(zVidImagePartial)));
    memset(
        image,
        0,
        sizeof(zVidImagePartial)
    );
    return image;
}

/**
 * Reimplements 0x46ecc0: zVid_Image::Destroy.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: release an image object's surface-dependent state, owned buffers,
 * and allocation, returning zero for both null and non-null images.
 *
 * Evidence: BN tests the image pointer, calls the current-device surface
 * provider only when image->surface is non-null, then releases owned buffers
 * and frees the image allocation before returning 0.
 */
int __fastcall Destroy(
    zVidImagePartial *image
) {
    if (image != 0) {
        if (image->surface != 0) {
            g_zVideo_pfnImageEnsureSurfaceForCurrentDevice(image);
        }

        ReleaseOwnedBuffers(image);
        free(image);
    }

    return 0;
}

/**
 * Reimplements 0x46d5a0: zVid_Image::ReleaseIfNotDefault.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: destroy dynamically allocated images while preserving the initialized default image singleton.
 *
 * Evidence: BN compares the incoming image against g_zImage_DefaultImage,
 * calls zVid_Image::Destroy only for non-default images, and returns 0.
 * The VC5-era throw() declaration is retained because callers such as
 * HudUiBackground::~HudUiBackground use it to match retail EH cleanup state numbering.
 */
int __fastcall ReleaseIfNotDefault(
    zVidImagePartial *image
) throw() {
    if (image != &g_zImage_DefaultImage) {
        Destroy(image);
    }

    return 0;
}

/**
 * Reimplements 0x48f500: zVid_Image::BlitToActiveTarget.
 * Source file evidence: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Purpose: Route an image blit to the primary DirectDraw surface when active, otherwise dispatch through the selected source-to-primary blitter.
 */
void __fastcall BlitToActiveTarget(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    if (image->surface != 0 && zRndr::g_frameBuffer == zVideo::GetPrimarySurfacePixels()) {
        zVideo_buff::BltSourceToPrimaryClipped(
            image,
            dstX,
            dstY,
            clipFlags & 0xffff,
            srcRect
        );
        return;
    }

    g_zVideo_pfnBltSourceToPrimary(
        image,
        dstX,
        dstY,
        clipFlags,
        srcRect
    );
}

/**
 * Reimplements 0x48f560: zVid_Image::BlitToFramebufferClipped.
 * Source file evidence: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Purpose: Clip and blit a zVid image into zRndr's active 16-bit framebuffer.
 *
 * The 565/555 alpha-map and color-key branches follow BN's zvid_buff.c
 * assembly-visible contracts; BN loses some row-cursor identities in the long
 * memcpy and paletted paths, so source keeps explicit typed row cursors.
 */
void __fastcall BlitToFramebufferClipped(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    int srcLeft = 0;
    int srcTop = 0;
    int srcRight = image->width;
    int srcBottom = image->height;
    if (srcRect != 0) {
        srcLeft = srcRect->left;
        srcTop = srcRect->top;
        srcRight = srcRect->right;
        srcBottom = srcRect->bottom;
    }

    const int srcWidth = srcRight - srcLeft;
    const int srcHeight = srcBottom - srcTop;
    if (srcWidth < 0 || srcWidth > 2048 || srcHeight < 0 || srcHeight > 2048) {
        return;
    }

    if (srcWidth == 0 || srcHeight == 0 || zRndr::g_frameBuffer == 0 || image->pixels == 0) {
        return;
    }

    const int activeWidth = zRndr::g_activeRegionWidth;
    const int activeHeight = zRndr::g_activeRegionHeight;
    const int dstRight = dstX + srcWidth - 1;
    const int dstBottom = dstY + srcHeight - 1;
    if (dstX >= activeWidth || dstRight < 0 || dstY >= activeHeight || dstBottom < 0) {
        return;
    }

    int clippedDstX = dstX;
    int clippedDstY = dstY;
    int clippedRight = dstRight;
    int clippedBottom = dstBottom;
    if (clippedDstX < 0) {
        clippedDstX = 0;
    }
    if (clippedDstY < 0) {
        clippedDstY = 0;
    }
    if (clippedRight >= activeWidth) {
        clippedRight = activeWidth - 1;
    }
    if (clippedBottom >= activeHeight) {
        clippedBottom = activeHeight - 1;
    }

    const int clippedWidth = clippedRight - clippedDstX + 1;
    const int clippedHeight = clippedBottom - clippedDstY + 1;
    if (clippedWidth <= 0 || clippedHeight <= 0) {
        return;
    }

    const int sourceStartX = srcLeft + clippedDstX - dstX;
    const int sourceStartY = srcTop + clippedDstY - dstY;
    const int sourcePitch = image->pitchWords;
    const int framebufferPitch = (int)((unsigned int)(zRndr::g_pitchBytes) >> 1);
    unsigned short *dstRow =
        (unsigned short *)(zRndr::g_frameBuffer) + framebufferPitch * clippedDstY + clippedDstX;
    const int alphaSkipThreshold = zVideo_GetAlphaSkipThreshold();

    if (image->palette == 0) {
        unsigned short *sourceRow =
            (unsigned short *)(image->pixels) + sourcePitch * sourceStartY + sourceStartX;
        unsigned char *alphaRow = (unsigned char *)(image->alphaMap);
        if (alphaRow != 0) {
            alphaRow += sourcePitch * sourceStartY + sourceStartX;
            for (int row = 0; row < clippedHeight; ++row) {
                for (int x = 0; x < clippedWidth; ++x) {
                    const int alpha = alphaRow[x];
                    if (alpha > alphaSkipThreshold) {
                        const unsigned short sourcePixel = sourceRow[x];
                        dstRow[x] = alpha >= 252
                                        ? sourcePixel
                                        : zVideo_BlendFramebufferPixelAlpha8(
                                              dstRow[x],
                                              sourcePixel,
                                              alpha
                                          );
                    }
                }

                dstRow += framebufferPitch;
                sourceRow += sourcePitch;
                alphaRow += sourcePitch;
            }
            return;
        }

        if ((image->formatFlagsPacked & 0x02) != 0) {
            const unsigned short transparentColor = (unsigned short)(clipFlags);
            for (int row_1 = 0; row_1 < clippedHeight; ++row_1) {
                for (int x_1 = 0; x_1 < clippedWidth; ++x_1) {
                    const unsigned short sourcePixel = sourceRow[x_1];
                    if (sourcePixel != transparentColor) {
                        dstRow[x_1] = sourcePixel;
                    }
                }

                dstRow += framebufferPitch;
                sourceRow += sourcePitch;
            }
            return;
        }

        if (clippedDstX == 0 && clippedRight == activeWidth - 1 &&
            framebufferPitch == sourcePitch) {
            memcpy(
                dstRow,
                sourceRow,
                (size_t)(clippedWidth * clippedHeight) * sizeof(unsigned short)
            );
            return;
        }

        for (int row_2 = 0; row_2 < clippedHeight; ++row_2) {
            memcpy(
                dstRow,
                sourceRow,
                (size_t)(clippedWidth) * sizeof(unsigned short)
            );
            dstRow += framebufferPitch;
            sourceRow += sourcePitch;
        }
        return;
    }

    unsigned char *sourceRow8 =
        (unsigned char *)(image->pixels) + sourcePitch * sourceStartY + sourceStartX;
    unsigned short *palette = (unsigned short *)(image->palette);
    unsigned char *alphaRow8 = (unsigned char *)(image->alphaMap);
    if (alphaRow8 != 0) {
        alphaRow8 += sourcePitch * sourceStartY + sourceStartX;
        for (int row_3 = 0; row_3 < clippedHeight; ++row_3) {
            for (int x_2 = 0; x_2 < clippedWidth; ++x_2) {
                const int alpha = alphaRow8[x_2];
                if (alpha > alphaSkipThreshold) {
                    const unsigned short sourcePixel = palette[sourceRow8[x_2]];
                    dstRow[x_2] = alpha >= 252
                                      ? sourcePixel
                                      : zVideo_BlendFramebufferPixelAlpha8(
                                            dstRow[x_2],
                                            sourcePixel,
                                            alpha
                                        );
                }
            }

            dstRow += framebufferPitch;
            sourceRow8 += sourcePitch;
            alphaRow8 += sourcePitch;
        }
        return;
    }

    if ((image->formatFlagsPacked & 0x02) != 0) {
        const unsigned short transparentIndex = (unsigned short)(clipFlags);
        for (int row_4 = 0; row_4 < clippedHeight; ++row_4) {
            for (int x_3 = 0; x_3 < clippedWidth; ++x_3) {
                const unsigned int sourceIndex = sourceRow8[x_3];
                if ((unsigned short)(sourceIndex) != transparentIndex) {
                    dstRow[x_3] = palette[sourceIndex];
                }
            }

            dstRow += framebufferPitch;
            sourceRow8 += sourcePitch;
        }
        return;
    }

    for (int row_5 = 0; row_5 < clippedHeight; ++row_5) {
        for (int x_4 = 0; x_4 < clippedWidth; ++x_4) {
            dstRow[x_4] = palette[sourceRow8[x_4]];
        }

        dstRow += framebufferPitch;
        sourceRow8 += sourcePitch;
    }
}

/**
 * Reimplements 0x46ecf0: zVid_Image::ReleaseOwnedBuffers.
 * Purpose: provide the recovered zVid_Image::ReleaseOwnedBuffers behavior.
 */
void __fastcall ReleaseOwnedBuffers(
    zVidImagePartial *image
) {
    void(__cdecl * freeProc)(void *) = free;

    if (image->pixels != 0 && (image->formatFlagsPacked & 0x20) != 0) {
        freeProc(image->pixels);
        image->pixels = 0;
        image->formatFlagsPacked &= (unsigned char)(~0x20);
    }

    if (image->alphaMap != 0 && (image->formatFlagsPacked & 0x40) != 0) {
        freeProc(image->alphaMap);
        image->alphaMap = 0;
        image->formatFlagsPacked &= (unsigned char)(~0x40);
    }

    if (image->palette != 0 && (image->formatFlagsPacked & 0x80) != 0 &&
        (image->formatFlagsPacked & 0x10) == 0) {
        freeProc(image->palette);
        image->palette = 0;
        image->formatFlagsPacked &= (unsigned char)(~0x80);
    }
}

/**
 * Reimplements 0x46ec20: zVid_Image::QueryBytesPerPixel.
 * Purpose: provide the recovered zVid_Image::QueryBytesPerPixel behavior.
 */
int __fastcall QueryBytesPerPixel(
    zVidImagePartial *image
) {
    return (image->formatFlagsPacked & 1) != 0 ? 2 : 1;
}

/**
 * Reimplements 0x46ec30: zVid_Image::SetHeaderFlagsByte.
 * Purpose: provide the recovered zVid_Image::SetHeaderFlagsByte behavior.
 */
int __fastcall SetHeaderFlagsByte(
    zVidImagePartial *image,
    unsigned char flags
) {
    image->headerFlagsByte = flags;
    return 0;
}

/**
 * Reimplements 0x46ec60: zVid_Image::SetFormatCode.
 * Purpose: provide the recovered zVid_Image::SetFormatCode behavior.
 */
int __fastcall SetFormatCode(
    zVidImagePartial *image,
    unsigned char formatCode
) {
    image->formatFlagsPacked = formatCode;
    return 0;
}

/**
 * Reimplements 0x46ec90: zVid_Image::SetSize.
 * Purpose: provide the recovered zVid_Image::SetSize behavior.
 */
int __fastcall SetSize(
    zVidImagePartial *image,
    short width,
    short height
) {
    image->width = width;
    image->height = height;
    image->pixelCount = (int)(width) * (int)(height);
    image->pitchWords = width;
    return 0;
}

/**
 * Reimplements 0x4902b0: zVid_Image::CalcPow2ScratchFields.
 * Original source path: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: provide the recovered zVid_Image::CalcPow2ScratchFields behavior.
 */
void __fastcall CalcPow2ScratchFields(
    zVidImagePartial *image
) {
    image->vPow2Shift = 0;
    image->uPow2Shift = 0;

    int width = image->width;
    while (width > 1) {
        width >>= 1;
        ++image->uPow2Shift;
    }

    int height = image->height;
    while (height > 1) {
        height >>= 1;
        ++image->vPow2Shift;
    }

    const unsigned char uShift = image->uPow2Shift;
    image->widthScale = 1.0f;
    image->uShiftFrom20 = 20 - uShift;
    image->uMask = (1 << uShift) - 1;
    image->vMaskFixed20 = (1 << image->vPow2Shift << 20) - 0x100000;
}

/**
 * Reimplements 0x46ec70: zVid_Image_SetPixels.
 * Purpose: provide the recovered zVid_Image_SetPixels behavior.
 */
extern "C" int __fastcall zVid_Image_SetPixels(
    zVidImagePartial *image,
    void *pixels,
    char *alphaMap
) {
    image->pixels = pixels;
    image->alphaMap = alphaMap;
    if (alphaMap != 0) {
        image->formatFlagsPacked |= 0x02u;
    }

    return 0;
}

/**
 * Reimplements 0x4a6e80: zVideo_buff_CaptureSurfaceToImage.
 * Purpose: Captures a selected 16-bit video surface into an owned zVid image.
 */
extern "C" zVidImagePartial *__fastcall zVideo_buff_CaptureSurfaceToImage(
    int sourceSelector
) {
    zVideo::Dispatch_LockDisplayModeSurfaceState();

    zVideo_SurfaceStatePartial *surfaceState = 0;
    if (sourceSelector == 0) {
        surfaceState = &g_zVideo_SwSurfaceState;
    } else if (sourceSelector == 1) {
        surfaceState = &g_zVideo_PrimarySurfaceState;
    } else if (sourceSelector == 2) {
        surfaceState = &g_zVideo_DisplayModeSurfaceState;
    } else {
        return 0;
    }

    const int width = surfaceState->width;
    const int height = surfaceState->height;
    const unsigned int pitchWords = (unsigned int)(surfaceState->pitch) >> 1;
    unsigned char *srcPixels = (unsigned char *)(surfaceState->pixels);

    zVidImagePartial *image = zVid_Image::Create();
    if (image == 0) {
        return 0;
    }

    zVid_Image::SetSize(
        image,
        (short)(width),
        (short)(height)
    );
    void *dstPixels = malloc((size_t)(image->pixelCount) * sizeof(unsigned short));
    image->formatFlagsPacked |= 0x20u;
    zVid_Image_SetPixels(
        image,
        dstPixels,
        0
    );

    unsigned char *dstBytes = (unsigned char *)(dstPixels);
    if (width == (int)(pitchWords)) {
        memcpy(
            dstBytes,
            srcPixels,
            (size_t)(image->pixelCount) * sizeof(unsigned short)
        );
    } else if (height > 0) {
        const int rowBytes = width * sizeof(unsigned short);
        const int pitchBytes = (int)(pitchWords * sizeof(unsigned short));
        {
            for (int row = 0; row < height; ++row) {
                memcpy(
                    dstBytes,
                    srcPixels,
                    (size_t)(rowBytes)
                );
                dstBytes += rowBytes;
                srcPixels += pitchBytes;
            }
        }
    }

    zVideo::Dispatch_UnlockDisplayModeSurfaceState();
    return image;
}

/**
 * Reimplements 0x46ec40: zVid_Image::QueryPixelDataBytes.
 * Purpose: provide the recovered zVid_Image::QueryPixelDataBytes behavior.
 */
int __fastcall QueryPixelDataBytes(
    zVidImagePartial *image
) {
    if (image->paletteMetaPacked != 0) {
        return image->pixelCount;
    }

    return QueryBytesPerPixel(image) * image->pixelCount;
}

/**
 * Reimplements 0x46d870: zVid_Image::ClearZeroAlphaPixelsInPlace.
 * Purpose: provide the recovered zVid_Image::ClearZeroAlphaPixelsInPlace behavior.
 */
void __fastcall ClearZeroAlphaPixelsInPlace(
    zVidImagePartial *image
) {
    if (image->paletteMetaPacked != 0) {
        return;
    }

    const int bytesPerPixel = QueryBytesPerPixel(image);
    if (image->pixelCount <= 0) {
        return;
    }

    unsigned char *alpha = (unsigned char *)(image->alphaMap);
    if (bytesPerPixel == 1) {
        unsigned char *pixels = (unsigned char *)(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
        return;
    }

    if (bytesPerPixel == 2) {
        unsigned short *pixels = (unsigned short *)(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
        return;
    }

    if (bytesPerPixel == 4) {
        unsigned int *pixels = (unsigned int *)(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
    }
}

namespace {
struct zVidImageFileHeader {
    unsigned char formatCode;
    unsigned char unknown_01[3];
    short width;
    short height;
    unsigned char headerFlags;
    unsigned char unknown_09[3];
    short textureAddressFlagsPacked;
    short paletteMeta;
};

RECOIL_STATIC_ASSERT(sizeof(zVidImageFileHeader) == 0x10);
} // namespace

/**
 * Reimplements 0x46ed70: zVid_Image::ReadHeader.
 * Purpose: provide the recovered zVid_Image::ReadHeader behavior.
 */
int __fastcall ReadHeader(
    FILE *file,
    zVidImagePartial *image
) {
    if (file == 0 || image == 0) {
        return -1;
    }

    zVidImageFileHeader header = {0};
    fread(
        &header,
        0x10,
        1,
        file
    );
    SetSize(
        image,
        header.width,
        header.height
    );
    SetFormatCode(
        image,
        header.formatCode
    );
    SetHeaderFlagsByte(
        image,
        header.headerFlags
    );
    image->paletteMetaPacked = header.paletteMeta;
    image->textureAddressFlagsPacked = header.textureAddressFlagsPacked;
    return 0;
}

/**
 * Reimplements 0x46ede0: zVid_Image::ReadData.
 * Purpose: provide the recovered zVid_Image::ReadData behavior.
 */
int __fastcall ReadData(
    FILE *file,
    zVidImagePartial *image,
    int bytesPerPixel
) {
    if (bytesPerPixel == 0) {
        bytesPerPixel = QueryBytesPerPixel(image);
    }

    if (bytesPerPixel != QueryBytesPerPixel(image)) {
        if (bytesPerPixel <= QueryBytesPerPixel(image)) {
            return -1;
        }
        return 0;
    }

    const int pixelBytes = QueryPixelDataBytes(image);
    if (fread(
        image->pixels,
        1,
        pixelBytes,
        file
    ) != (size_t)(pixelBytes)) {
        return -1;
    }

    if ((image->formatFlagsPacked & 0x08) != 0) {
        image->alphaMap = (char *)(malloc((size_t)(image->pixelCount)));
        if (fread(
            image->alphaMap,
            1,
            image->pixelCount,
            file
        ) != (size_t)(image->pixelCount)) {
            image->formatFlagsPacked |= 0x40;
            return -1;
        }
        image->formatFlagsPacked |= 0x40;
    }

    if ((image->formatFlagsPacked & 0x10) == 0 && image->paletteMetaPacked != 0) {
        const int paletteBytes = bytesPerPixel * image->paletteMetaPacked;
        image->palette = malloc((size_t)(paletteBytes));
        if (fread(
            image->palette,
            1,
            paletteBytes,
            file
        ) != (size_t)(paletteBytes)) {
            image->formatFlagsPacked |= 0x80;
            return -1;
        }
        image->formatFlagsPacked |= 0x80;
    }

    if (bytesPerPixel == 2 && (image->formatFlagsPacked & 0x10) == 0) {
        int rBits = 0;
        int gBits = 0;
        int bBits = 0;
        zVideo::PixelPack_GetRgbBits(
            &rBits,
            &gBits,
            &bBits
        );
        if (gBits == 5) {
            unsigned short *colors = image->paletteMetaPacked == 0
                                         ? (unsigned short *)(image->pixels)
                                         : (unsigned short *)(image->palette);
            int count =
                image->paletteMetaPacked == 0 ? image->pixelCount : image->paletteMetaPacked;
            while (count > 0) {
                const unsigned short value = *colors;
                *colors = (unsigned short)(((value >> 1) & 0x7fe0) | (value & 0x1f));
                ++colors;
                --count;
            }
        }
    }

    if (image->paletteMetaPacked != 0) {
        image->palette = zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
            (unsigned short *)(image->palette),
            image->paletteMetaPacked
        );
    }

    return 0;
}

/**
 * Reimplements 0x46ef70: zVid_Image::ReadFromFile.
 * Purpose: provide the recovered zVid_Image::ReadFromFile behavior.
 */
zVidImagePartial *__fastcall ReadFromFile(
    FILE *file
) {
    zVidImagePartial *image = Create();
    if (ReadHeader(
        file,
        image
    ) != 0) {
        return 0;
    }

    image->pixels = malloc(QueryPixelDataBytes(image));
    ReadData(
        file,
        image,
        0
    );
    image->formatFlagsPacked |= 0x20;
    return image;
}

/**
 * Reimplements 0x46e9b0: zVid_Image::ResampleSquare.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: resamples an owned 16-bit zVid image into a square nearest-source
 * pixel buffer and matching alpha map when present.
 *
 * Evidence: BN assembly allocates sideLength * sideLength pixels and only
 * allocates a replacement alpha map when the old image had one, copies pixels
 * and alpha bytes through _ftol-truncated source coordinates, frees the old
 * buffers, and installs the square dimensions and replacement buffers.
 */
void __fastcall ResampleSquare(
    zVidImagePartial *image,
    int sideLength
) {
    const float inverseSideLength = 1.0f / (float)(sideLength);
    unsigned short *oldPixels = (unsigned short *)(image->pixels);
    char *oldAlphaMap = image->alphaMap;
    const int sourceWidth = image->width;
    const int sourceHeight = image->height;
    const float xScale = (float)(sourceWidth)*inverseSideLength;
    const float yScale = (float)(sourceHeight)*inverseSideLength;

    const unsigned int pixelCount = (unsigned int)(sideLength * sideLength);
    unsigned short *newPixels = (unsigned short *)(malloc(pixelCount * sizeof(unsigned short)));
    char *newAlphaMap = 0;
    if (oldAlphaMap != 0) {
        newAlphaMap = (char *)(malloc(pixelCount));
    }

    {
        for (int dstY = 0; dstY < sideLength; ++dstY) {
            const int srcY = (int)((float)(dstY)*yScale);
            unsigned short *newPixelCursor = &newPixels[dstY * sideLength];
            char *newAlphaCursor = newAlphaMap != 0 ? &newAlphaMap[dstY * sideLength] : 0;

            {
                for (int dstX = 0; dstX < sideLength; ++dstX) {
                    const int srcX = (int)((float)(dstX)*xScale);
                    const int sourceIndex = srcY * sourceWidth + srcX;
                    *newPixelCursor++ = oldPixels[sourceIndex];
                    if (oldAlphaMap != 0) {
                        *newAlphaCursor++ = oldAlphaMap[sourceIndex];
                    }
                }
            }
        }
    }

    free(image->pixels);
    image->height = (short)(sideLength);
    image->width = (short)(sideLength);
    image->pixels = newPixels;
    if (oldAlphaMap != 0) {
        free(oldAlphaMap);
        image->alphaMap = newAlphaMap;
    }
}
} // namespace zVid_Image

namespace zVid_PaletteRemap {
/**
 * Reimplements 0x46e680: zVid_PaletteRemap::FindRecipeIndex.
 * Purpose: find an existing palette-remap recipe with the same endpoint colors and strengths.
 *
 * Evidence: BN scans g_zVid_PaletteRemapRecipes and compares the eight
 * zVidPaletteRemapRecipe float fields in retail field order, including the
 * color0Strength check before the color1 RGB fields.
 */
int __fastcall FindRecipeIndex(
    zVidPaletteRemapRecipe *recipe
) {
    for (int i = 0; i < g_zVid_PaletteRemapRecipeCount; ++i) {
        zVidPaletteRemapRecipe *candidate = &g_zVid_PaletteRemapRecipes[i];
        if (recipe->color0R == candidate->color0R && recipe->color0G == candidate->color0G &&
            recipe->color0B == candidate->color0B &&
            recipe->color0Strength == candidate->color0Strength &&
            recipe->color1R == candidate->color1R && recipe->color1G == candidate->color1G &&
            recipe->color1B == candidate->color1B &&
            recipe->color1Strength == candidate->color1Strength) {
            return i;
        }
    }

    return -1;
}

/**
 * Reimplements 0x46e4e0: zVid_PaletteRemap::ApplyRecipeToPaletteVariant.
 * Purpose: blend one source palette toward a recipe endpoint variant and pack 16-bit colors.
 */
void __fastcall ApplyRecipeToPaletteVariant(
    zVidPaletteRemapRecipe *recipe,
    unsigned short *sourceColors,
    int colorCount,
    int variantIndex,
    unsigned short *destColors
) {
    int rBits;
    int gBits;
    int bBits;
    zVideo::PixelPack_GetRgbBits(
        &rBits,
        &gBits,
        &bBits
    );

    const float variantWeight = (float)(variantIndex) * 0.0322580636f;
    const float inverseVariantWeight = 1.0f - variantWeight;
    float r;
    float g;
    float b;
    int red;
    int green;
    int blue;
    zVideo_ColorRgbFloat color;

    while (colorCount > 0) {
        const int packed = *sourceColors;
        r = 0.0f;
        g = 0.0f;
        if (gBits == 5) {
            red = packed & 0x7c00;
            green = packed & 0x03e0;
            r = (float)(red) * 3.15020152e-05f;
            g = (float)(green) * 0.00100806449f;
        } else {
            red = packed & 0xf800;
            green = packed & 0x07e0;
            r = (float)(red) * 1.57510076e-05f;
            g = (float)(green) * 0.000496031775f;
        }
        blue = packed & 0x001f;
        b = (float)(blue) * 0.0322580636f;

        color.r = ((recipe->color0R - r) * inverseVariantWeight * recipe->color0Strength +
                      (recipe->color1R - r) * variantWeight * recipe->color1Strength + r) *
                  255.0f;
        color.g = ((recipe->color1G - g) * variantWeight * recipe->color1Strength +
                      (recipe->color0G - g) * inverseVariantWeight * recipe->color0Strength + g) *
                  255.0f;
        color.b = ((recipe->color1B - b) * variantWeight * recipe->color1Strength +
                      (recipe->color0B - b) * inverseVariantWeight * recipe->color0Strength + b) *
                  255.0f;

        *destColors = zVid_PackColorRgbFloats(&color);
        ++sourceColors;
        ++destColors;
        --colorCount;
    }
}
} // namespace zVid_PaletteRemap

/**
 * Reimplements 0x46e720: zVid_PaletteRemap_BuildPaletteVariant.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Add a palette-remap recipe and rebuild existing palette variant tables.
 */
extern "C" int __fastcall zVid_PaletteRemap_BuildPaletteVariant(
    zVidPaletteRemapRecipe *recipe
) {
    const int existingIndex = zVid_PaletteRemap::FindRecipeIndex(recipe);
    if (existingIndex >= 0) {
        return existingIndex;
    }

    ++g_zVid_PaletteRemapRecipeCount;
    g_zVid_PaletteRemapRecipes = (zVidPaletteRemapRecipe *)(realloc(
        g_zVid_PaletteRemapRecipes,
        (size_t)(g_zVid_PaletteRemapRecipeCount) * sizeof(zVidPaletteRemapRecipe)
    ));
    g_zVid_PaletteRemapRecipes[g_zVid_PaletteRemapRecipeCount - 1] = *recipe;

    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zVidImagePartial *image = g_zImage_TexDirEntries[i].image;
        if (image->paletteMetaPacked == 0 || (image->formatFlagsPacked & 0x10) != 0) {
            continue;
        }

        image->palette = realloc(
            image->palette,
            (size_t)(
                (g_zVid_PaletteRemapRecipeCount * kZVidPaletteRemapColorsPerRecipe) +
                kZVidPaletteColorCount
            ) * sizeof(unsigned short)
        );
        unsigned short *palette = (unsigned short *)(image->palette);
        {
            for (int variant = 0; variant < kZVidPaletteRemapVariantCount; ++variant) {
                zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                    recipe,
                    palette,
                    image->paletteMetaPacked,
                    variant,
                    &palette[(
                        ((g_zVid_PaletteRemapRecipeCount - 1) *
                            kZVidPaletteRemapVariantCount) +
                        variant +
                        1
                    ) * kZVidPaletteColorCount]
                );
            }
        }
    }

    for (int tableIndex = 0; tableIndex < g_zVid_PaletteRemapVariantTableCount; ++tableIndex) {
        unsigned short *oldTable = g_zVid_PaletteRemapVariantTables[tableIndex];
        g_zVid_PaletteRemapVariantTables[tableIndex] =
            (unsigned short *)(realloc(
                oldTable,
                (size_t)(
                    (g_zVid_PaletteRemapRecipeCount * kZVidPaletteRemapColorsPerRecipe) +
                    kZVidPaletteColorCount
                ) * sizeof(unsigned short)
            ));
        unsigned short *table = g_zVid_PaletteRemapVariantTables[tableIndex];

        {
            for (int variant = 0; variant < kZVidPaletteRemapVariantCount; ++variant) {
                zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                    recipe,
                    table,
                    kZVidPaletteColorCount,
                    variant,
                    &table[(
                        ((g_zVid_PaletteRemapRecipeCount - 1) *
                            kZVidPaletteRemapVariantCount) +
                        variant +
                        1
                    ) * kZVidPaletteColorCount]
                );
            }
        }

        for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
            zVidImagePartial *image = g_zImage_TexDirEntries[i].image;
            if (image->palette == oldTable) {
                image->palette = table;
            }
        }
    }

    return g_zVid_PaletteRemapRecipeCount - 1;
}

/**
 * Reimplements 0x46e8d0: zVid_PaletteRemap_BuildAllRecipeVariantsForPalette.
 * Purpose: expand a palette with all variants for every active palette-remap recipe.
 */
extern "C" unsigned short *__fastcall
/**
 * Reimplements 0x46e8d0: zVid_PaletteRemap_BuildAllRecipeVariantsForPalette.
 * Purpose: provide the recovered zVid_PaletteRemap_BuildAllRecipeVariantsForPalette behavior.
 */
zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
    unsigned short *palette,
    int colorCount
) {
    if (g_zVid_PaletteRemapRecipeCount == 0) {
        return palette;
    }

    unsigned short *result = (unsigned short *)(realloc(
        palette,
        zVidPaletteRemapTableBytesForRecipeCount(g_zVid_PaletteRemapRecipeCount)
    ));

    for (int recipeIndex = 0; recipeIndex < g_zVid_PaletteRemapRecipeCount; ++recipeIndex) {
        unsigned short *dest =
            &result[kZVidPaletteColorCount + recipeIndex * kZVidPaletteRemapColorsPerRecipe];
        zVidPaletteRemapRecipe *recipe = &g_zVid_PaletteRemapRecipes[recipeIndex];
        {
            for (int variant = 0; variant < kZVidPaletteRemapVariantCount; ++variant) {
                zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                    recipe,
                    result,
                    colorCount,
                    variant,
                    dest
                );
                dest += kZVidPaletteColorCount;
            }
        }
    }

    return result;
}

/**
 * Reimplements 0x46e960: zVid_PaletteRemap_FindRecipeIndexFromRgb.
 * Purpose: Build the black-to-RGB palette-remap recipe used by renderer shade lookups and find its existing recipe index.
 *
 * Evidence: BN constructs a stack zVidPaletteRemapRecipe with zero color0
 * endpoint fields, RGB color1 fields copied from the input, color0Strength
 * zero, and color1Strength 1.0f before delegating to FindRecipeIndex.
 */
extern "C" int __fastcall zVid_PaletteRemap_FindRecipeIndexFromRgb(
    zColorRgb *rgb
) {
    zVidPaletteRemapRecipe recipe = {0};
    recipe.color1R = rgb->red;
    recipe.color1G = rgb->green;
    recipe.color1B = rgb->blue;
    recipe.color1Strength = 1.0f;
    return zVid_PaletteRemap::FindRecipeIndex(&recipe);
}

/**
 * Reimplements 0x46dae0: zVid_TexturePackEntry_LoadFromFile.
 * Purpose: load one texture-pack ZBD entry table and any palette-remap variant tables.
 */
extern "C" FILE *__fastcall zVid_TexturePackEntry_LoadFromFile(
    zVidTexturePackEntry *entry
) {
    if (g_zVid_TexturePackLoadState == 0) {
        return 0;
    }

    entry->fileHandle = zUtil_ZRDR_OpenFileResolved(
        0,
        entry->filePath,
        "rb"
    );
    if (entry->fileHandle == 0) {
        return 0;
    }

    if (fread(&entry->header, sizeof(entry->header), 1, entry->fileHandle) != 1 ||
        entry->header.fileFormat != 1) {
        fclose(entry->fileHandle);
        entry->fileHandle = 0;
        return 0;
    }

    entry->records = (zVidTexturePackRecord *)(malloc(
        (size_t)(entry->header.recordCount) * sizeof(zVidTexturePackRecord)
    ));
    if (fread(
            entry->records,
            sizeof(zVidTexturePackRecord),
            entry->header.recordCount,
            entry->fileHandle
        ) != (size_t)(entry->header.recordCount)) {
        fclose(entry->fileHandle);
        entry->fileHandle = 0;
        free(entry->records);
        entry->records = 0;
        return 0;
    }

    entry->paletteTableBaseIndex = g_zVid_PaletteRemapVariantTableCount;
    if (entry->header.paletteTableCount <= 0) {
        return entry->fileHandle;
    }

    int tableIndex = g_zVid_PaletteRemapVariantTableCount;
    g_zVid_PaletteRemapVariantTableCount += entry->header.paletteTableCount;
    g_zVid_PaletteRemapVariantTables = (unsigned short **)(realloc(
        g_zVid_PaletteRemapVariantTables,
        (size_t)(g_zVid_PaletteRemapVariantTableCount) * sizeof(unsigned short *)
    ));

    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    zVideo::PixelPack_GetRgbBits(
        &rBits,
        &gBits,
        &bBits
    );

    while (tableIndex < g_zVid_PaletteRemapVariantTableCount) {
        unsigned short *table =
            (unsigned short *)(malloc((size_t)kZVidPaletteColorCount * sizeof(unsigned short)));
        g_zVid_PaletteRemapVariantTables[tableIndex] = table;
        if (fread(
            table,
            sizeof(unsigned short),
            kZVidPaletteColorCount,
            entry->fileHandle
        ) != (size_t)kZVidPaletteColorCount) {
            fclose(entry->fileHandle);
            entry->fileHandle = 0;
            free(entry->records);
            entry->records = 0;
            return 0;
        }

        if (gBits == 5) {
            {
                for (int colorIndex = 0; colorIndex < kZVidPaletteColorCount; ++colorIndex) {
                    unsigned short *color = &table[colorIndex];
                    const unsigned short value = *color;
                    const unsigned short shifted = (unsigned short)(value >> 1);
                    const unsigned short lowXor =
                        (unsigned char)((unsigned char)(value) ^ (unsigned char)(shifted));
                    *color = (unsigned short)((lowXor & 0x1f) ^ shifted);
                }
            }
        }

        g_zVid_PaletteRemapVariantTables[tableIndex] =
            zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
                table,
                kZVidPaletteColorCount
            );
        ++tableIndex;
    }

    return entry->fileHandle;
}

/**
 * Reimplements 0x46da40: zVid_TexturePack_EnsureDefaultImagePackLoaded.
 * Purpose: allocate and load the default image texture pack, with retail fallback path.
 */
extern "C" void zVid_TexturePack_EnsureDefaultImagePackLoaded() {
    if (g_zVid_TexturePackCount > 0) {
        return;
    }

    g_zVid_TexturePacks = (zVidTexturePackEntry *)(realloc(
        g_zVid_TexturePacks,
        (size_t)(g_zVid_TexturePackCount + 1) * sizeof(zVidTexturePackEntry)
    ));
    zVidTexturePackEntry *entry = &g_zVid_TexturePacks[g_zVid_TexturePackCount];
    memset(
        entry,
        0,
        sizeof(*entry)
    );
    strcpy(
        entry->filePath,
        g_zVid_DefaultImageTexturePackName
    );

    if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
        sprintf(
            entry->filePath,
            g_zVid_DefaultImageTexturePackReadonlyNameFmt,
            g_zVid_DefaultImageTexturePackName
        );
        if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
            return;
        }
    }

    ++g_zVid_TexturePackCount;
}

/**
 * Reimplements 0x46df50: zVid_TexturePack_EnsureBuiltinTexturePacksLoaded.
 * Original source path: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVid_TexturePack_EnsureBuiltinTexturePacksLoaded behavior.
 */
extern "C" RECOIL_NO_GS void zVid_TexturePack_EnsureBuiltinTexturePacksLoaded() {
    if (g_zVid_BuiltinTexturePackCount > 0) {
        for (int i = 0; i < g_zVid_BuiltinTexturePackCount; ++i) {
            zVidTexturePackEntry *const entry = &g_zVid_BuiltinTexturePacks[i];
            if (entry->fileHandle == 0) {
                entry->fileHandle = zUtil_ZRDR_OpenFileResolved(
                    g_zImage_MissionSearchPathList,
                    entry->filePath,
                    "rb"
                );
            }
        }
        return;
    }

    char filePath[0x20];
    int probeWasRendererMemory = 0;
    int candidateSize = 8;
    int totalBytes = 0;
    int freeBytes = 0;
    char *const archiveExtension = &g_zVid_TextureArchiveMaxName[11];

    if (g_zVideo_pfnQueryTextureMemoryBytes(-1, &totalBytes, &freeBytes) != 0 &&
        g_zVideo_ActiveRendererPath != 0) {
        candidateSize = (unsigned int)(totalBytes) >> 20;
        sprintf(
            filePath,
            g_zVid_TextureArchiveRendererSizedNameFmt,
            g_zVid_TextureArchiveStem,
            candidateSize,
            archiveExtension
        );
        probeWasRendererMemory = 1;
    } else {
        switch (*g_zImage_TextureMemoryOption) {
        case 1:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize8Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 8;
            break;
        case 2:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize6Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 6;
            break;
        case 3:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize4Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 4;
            break;
        case 4:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize2Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 2;
            break;
        default:
            sprintf(
                filePath,
                "%s",
                g_zVid_TextureArchiveMaxName
            );
            candidateSize = 8;
            break;
        }
    }

    g_zVid_BuiltinTexturePacks = (zVidTexturePackEntry *)(realloc(
        g_zVid_BuiltinTexturePacks,
        (size_t)(g_zVid_BuiltinTexturePackCount + 1) * sizeof(zVidTexturePackEntry)
    ));
    zVidTexturePackEntry *const entry = &g_zVid_BuiltinTexturePacks[g_zVid_BuiltinTexturePackCount];
    memset(
        entry,
        0,
        sizeof(*entry)
    );
    strcpy(
        entry->filePath,
        filePath
    );

    if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
        {
            for (int size = candidateSize; size >= -1; --size) {
                if (size > 0) {
                    if (probeWasRendererMemory != 0) {
                        sprintf(
                            filePath,
                            g_zVid_TextureArchiveRendererSizedNameFmt,
                            g_zVid_TextureArchiveStem,
                            size,
                            archiveExtension
                        );
                    } else {
                        sprintf(
                            filePath,
                            g_zVid_TextureArchiveSizedNameFmt,
                            g_zVid_TextureArchiveStem,
                            size,
                            archiveExtension
                        );
                    }
                } else if (size == 0) {
                    sprintf(
                        filePath,
                        "%s",
                        g_zVid_TextureArchiveMaxName
                    );
                } else {
                    sprintf(
                        filePath,
                        g_zVid_TextureArchiveNameFmt,
                        g_zVid_TextureArchiveStem,
                        archiveExtension
                    );
                }

                strcpy(
                    entry->filePath,
                    filePath
                );
                if (zVid_TexturePackEntry_LoadFromFile(entry) != 0) {
                    break;
                }
            }
        }
    }

    if (entry->fileHandle != 0) {
        ++g_zVid_BuiltinTexturePackCount;
    }
}

namespace {
/**
 * Recovered helper: LoadTexturePackImageByName.
 * Original-source helper evidence: no standalone retail function is present;
 * recovered from address-backed callers 0x46d940 and 0x46dd30 in this source file.
 * Purpose: find and load an image record from a texture-pack entry array.
 */
zVidImagePartial *LoadTexturePackImageByName(
    zVidTexturePackEntry *entries,
    int count,
    const char *imageName,
    bool builtin
) {
    zVidImagePartial *result = 0;
    for (int i = 0; i < count && result == 0; ++i) {
        zVidTexturePackEntry *entry = &entries[i];
        if (entry->fileHandle == 0) {
            continue;
        }

        for (int recordIndex = 0; recordIndex < entry->header.recordCount && result == 0;
            ++recordIndex) {
            zVidTexturePackRecord *record = &entry->records[recordIndex];
            if (_stricmp(
                record->name,
                imageName
            ) != 0) {
                continue;
            }

            fseek(
                entry->fileHandle,
                record->fileOffset,
                SEEK_SET
            );
            result = zVid_Image::ReadFromFile(entry->fileHandle);
            if (record->paletteIndex != -1) {
                const int tableIndex = entry->paletteTableBaseIndex + record->paletteIndex;
                if (builtin) {
                    if (result->palette != 0) {
                        free(result->palette);
                        result->palette = 0;
                        result->formatFlagsPacked &= (unsigned char)(~0x80);
                    }
                    result->paletteMetaPacked = 0x100;
                }
                result->palette = g_zVid_PaletteRemapVariantTables[tableIndex];
            }
        }
    }

    return result;
}
} // namespace

/**
 * Reimplements 0x46d940: zVid_TexturePack_LoadImageByName.
 * Purpose: provide the recovered zVid_TexturePack_LoadImageByName behavior.
 */
extern "C" zVidImagePartial *__fastcall zVid_TexturePack_LoadImageByName(
    const char *imageName
) {
    if (g_zVid_TexturePackCount == 0) {
        zVid_TexturePack_EnsureDefaultImagePackLoaded();
    }

    return LoadTexturePackImageByName(
        g_zVid_TexturePacks,
        g_zVid_TexturePackCount,
        imageName,
        false
    );
}

// Reimplements 0x46dd30: zVid_TexturePack_LoadBuiltinImageByName
extern "C" zVidImagePartial *__fastcall
/**
 * Reimplements 0x46dd30: zVid_TexturePack_LoadBuiltinImageByName.
 * Purpose: provide the recovered zVid_TexturePack_LoadBuiltinImageByName behavior.
 */
zVid_TexturePack_LoadBuiltinImageByName(
    const char *imageName
) {
    return LoadTexturePackImageByName(
        g_zVid_BuiltinTexturePacks,
        g_zVid_BuiltinTexturePackCount,
        imageName,
        true
    );
}

namespace zVid_TexturePack {
/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered zVid_TexturePack::ClosePackEntry helper behavior for zVideo callers.
 */
void ClosePackEntry(
    zVidTexturePackEntry &entry
) {
    if (entry.fileHandle != 0) {
        fclose(entry.fileHandle);
        entry.fileHandle = 0;
    }

    if (entry.records != 0) {
        free(entry.records);
        entry.records = 0;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered zVid_TexturePack::FreePackEntryRecords helper behavior for zVideo callers.
 */
void FreePackEntryRecords(
    zVidTexturePackEntry &entry
) {
    if (entry.records != 0) {
        free(entry.records);
        entry.records = 0;
    }
}
} // namespace zVid_TexturePack

namespace zImage {
/**
 * Reimplements 0x46d730: zImage::ShutdownTextureDirectoryRuntime.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
 * Purpose: close open built-in texture-pack file handles and return the
 * current built-in texture-pack count.
 *
 * Evidence: BN reloads g_zVid_BuiltinTexturePackCount after each iteration,
 * walks g_zVid_BuiltinTexturePacks using the zVidTexturePackEntry stride,
 * calls fclose for non-null fileHandle values, clears each closed handle, and
 * returns the last reloaded count.
 */
int ShutdownTextureDirectoryRuntime() {
    int count = g_zVid_BuiltinTexturePackCount;
    for (int i = 0; i < count; ++i) {
        zVidTexturePackEntry &entry = g_zVid_BuiltinTexturePacks[i];
        if (entry.fileHandle != 0) {
            fclose(entry.fileHandle);
            entry.fileHandle = 0;
        }
        count = g_zVid_BuiltinTexturePackCount;
    }

    return count;
}
} // namespace zImage

namespace zVid_TexturePack {
/**
 * Reimplements 0x46d6b0: zVid_TexturePack::ShutdownBuiltinPacks.
 * Purpose: provide the recovered zVid_TexturePack::ShutdownBuiltinPacks behavior.
 */
void ShutdownBuiltinPacks() {
    zImage::ShutdownTextureDirectoryRuntime();

    for (int i = 0; i < g_zVid_BuiltinTexturePackCount; ++i) {
        FreePackEntryRecords(g_zVid_BuiltinTexturePacks[i]);
    }

    if (g_zVid_BuiltinTexturePacks != 0) {
        free(g_zVid_BuiltinTexturePacks);
        g_zVid_BuiltinTexturePacks = 0;
    }

    g_zVid_BuiltinTexturePackCount = 0;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered zVid_TexturePack::Shutdown helper behavior for zVideo callers.
 */
void Shutdown() {
    for (int i = 0; i < g_zVid_TexturePackCount; ++i) {
        ClosePackEntry(g_zVid_TexturePacks[i]);
    }

    if (g_zVid_TexturePacks != 0) {
        free(g_zVid_TexturePacks);
        g_zVid_TexturePacks = 0;
    }

    g_zVid_TexturePackCount = 0;
}
} // namespace zVid_TexturePack

namespace zVid_TexDir {
/**
 * Reimplements 0x46d5d0: zVid_TexDir::Shutdown.
 * Original file: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: shut down texture-directory entries, palette remap storage, and
 * texture-pack state.
 *
 * Evidence: BN walks g_zImage_TexDirEntries as 0x24-byte records, releases
 * only loadState 1 image/texture pairs, clears loadState 1 and 2 entries,
 * then unconditionally calls the active upload-surface release callback before
 * freeing palette remap tables, recipes, and both texture-pack banks.
 */
int Shutdown() {
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial &entry = g_zImage_TexDirEntries[i];
        if (entry.loadState == 1) {
            if (entry.image != 0) {
                zVid_Image::ReleaseIfNotDefault(entry.image);
                entry.image = 0;
            }

            if (entry.texture != 0) {
                g_zVideo_pfnTextureRecordDestroy(entry.texture);
                entry.texture = 0;
            }
        }

        if (entry.loadState == 1 || entry.loadState == 2) {
            entry.loadState = 0;
        }
    }

    g_zImage_TexDirEntryCount = 0;

    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces();

    for (int tableIndex = 0;
         tableIndex < g_zVid_PaletteRemapVariantTableCount;
         ++tableIndex) {
        free(g_zVid_PaletteRemapVariantTables[tableIndex]);
        g_zVid_PaletteRemapVariantTables[tableIndex] = 0;
    }

    unsigned short **variantTables = g_zVid_PaletteRemapVariantTables;
    g_zVid_PaletteRemapVariantTableCount = 0;
    if (variantTables != 0) {
        free(variantTables);
        g_zVid_PaletteRemapVariantTables = 0;
    }

    zVidPaletteRemapRecipe *recipes = g_zVid_PaletteRemapRecipes;
    if (recipes != 0) {
        free(recipes);
        g_zVid_PaletteRemapRecipes = 0;
    }
    g_zVid_PaletteRemapRecipeCount = 0;

    zVid_TexturePack::ShutdownBuiltinPacks();
    zVid_TexturePack::Shutdown();
    return 0;
}
} // namespace zVid_TexDir

namespace zVideo_dd3d {
/**
 * Reimplements 0x4a6750: zVideo_dd3d::CallClearZBufferRect.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Tail-dispatch the active Z-buffer clear callback.
 */
void __fastcall CallClearZBufferRect(
    zVidRect32 *rect
) {
    g_zVideo_pfnClearZBufferRect(rect);
}

/**
 * Reimplements 0x4a6b60: zVideo_dd3d::SetPendingWireframeState.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: store the deferred Direct3D wireframe fill-mode request.
 *
 * Evidence: BN stores ecx directly into g_zVideo_PendingWireframeState, which
 * BeginSceneAndFlushPendingRenderStates later consumes and resets.
 */
void __fastcall SetPendingWireframeState(
    int pendingWireframeState
) {
    g_zVideo_PendingWireframeState = pendingWireframeState;
}

/**
 * Reimplements 0x4a6b70: zVideo_dd3d::SetPendingDitherEnable.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: store the deferred Direct3D dither-enable render-state request.
 *
 * Evidence: BN stores ecx directly into g_zVideo_PendingDitherEnable, which
 * BeginSceneAndFlushPendingRenderStates applies to D3DRENDERSTATE_DITHERENABLE.
 */
void __fastcall SetPendingDitherEnable(
    int enabled
) {
    g_zVideo_PendingDitherEnable = enabled;
}

/**
 * Reimplements 0x4a9ac0: zVideo_dd3d::BeginSceneAndFlushPendingRenderStates.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: begin the Direct3D scene and flush deferred wireframe and dither states.
 *
 * Evidence: BN calls IDirect3DDevice2::BeginScene, reports zvid_ddd3d.c line 76
 * on failure, maps pending wireframe 0/1 to solid/wireframe fill mode, resets
 * applied pending states to -1, and returns zero on success.
 */
int BeginSceneAndFlushPendingRenderStates() {
    const HRESULT hresult = g_zVideo_pD3DDevice->BeginScene();
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            76
        );
    }

    const int pendingWireframeState = g_zVideo_PendingWireframeState;
    if (pendingWireframeState == 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_FILLMODE,
            D3DFILL_SOLID
        );
        g_zVideo_PendingWireframeState = -1;
    } else if (pendingWireframeState == 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_FILLMODE,
            D3DFILL_WIREFRAME
        );
        g_zVideo_PendingWireframeState = -1;
    }

    // VC5 matches BN when the dither global is reloaded at the call site.
    if (g_zVideo_PendingDitherEnable != -1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_DITHERENABLE,
            (DWORD)(g_zVideo_PendingDitherEnable)
        );
        g_zVideo_PendingDitherEnable = -1;
    }

    return 0;
}

/**
 * Reimplements 0x4a9b40: zVideo_dd3d::EndScene.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: end the active Direct3D scene and report provider failures.
 *
 * Evidence: BN calls IDirect3DDevice2::EndScene, reports zvid_ddd3d.c line 115
 * on nonzero HRESULT, and returns zero on success.
 */
int EndScene() {
    const HRESULT hresult = g_zVideo_pD3DDevice->EndScene();
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            115
        );
    }

    return 0;
}

namespace {
/**
 * Original-source static helper evidence: source-faithful helper for fog color.
 * Purpose: Pack 0..255 RGB floats for address-backed callers 0x4aaa90 and
 * 0x4aab30; BN has no standalone retail function for this body.
 */
DWORD PackFogColorFrom255Floats(
    float red,
    float green,
    float blue
) {
    const DWORD redByte = (DWORD)((int)(red + 0.5f));
    const DWORD greenByte = (DWORD)((int)(green + 0.5f));
    const DWORD blueByte = (DWORD)((int)(blue + 0.5f));
    return ((redByte << 8) | greenByte) << 8 | blueByte;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered PackD3DColorFrom16 helper behavior for zVideo callers.
 */
DWORD PackD3DColorFrom16(
    unsigned int packedColor16,
    int alpha
) {
    const DWORD red = (packedColor16 & g_zVideo_PixelPack.rMask) >> g_zVideo_PixelPack.packedBase;
    const DWORD green = (packedColor16 & g_zVideo_PixelPack.gMask) >> g_zVideo_PixelPack.sumMinus8;
    const DWORD blue = (packedColor16 & g_zVideo_PixelPack.bMask) << g_zVideo_PixelPack.bShiftTo8;
    return ((((red | ((DWORD)(alpha) << 8)) << 8) | green) << 8) | blue;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered WriteFlatTlVertex helper behavior for zVideo callers.
 */
void WriteFlatTlVertex(
    D3DTLVERTEX &dst,
    const zVideo_XyzVertex &src,
    DWORD packedColor
) {
    dst.sx = src.x;
    dst.sy = src.y;
    dst.sz = src.z;
    dst.rhw = src.z;
    dst.color = packedColor;
    dst.specular = 0xff000000;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyFlatVerticesReverse helper behavior for zVideo callers.
 */
void CopyFlatVerticesReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    int vertexCount,
    DWORD packedColor
) {
    for (int i = 0; i < vertexCount; ++i) {
        WriteFlatTlVertex(
            dst[i],
            vertices[vertexCount - 1 - i],
            packedColor
        );
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyGouraudVerticesReverse helper behavior for zVideo callers.
 */
void CopyGouraudVerticesReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const unsigned int *packedColors16,
    int vertexCount,
    int alpha
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        WriteFlatTlVertex(
            dst[i],
            vertices[sourceIndex],
            PackD3DColorFrom16(packedColors16[sourceIndex], alpha)
        );
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for D3D color-attribute submitters.
 * Purpose: Pack a constant RGB color and alpha for address-backed callers 0x4ab320,
 * 0x4abb20, and 0x4ac370; BN has no standalone retail function for this body.
 */
DWORD PackColorAttrConstant(
    const zVideo_ColorRgbFloat &baseColor,
    float attr1Scale,
    DWORD alphaBits
) {
    const DWORD red = (DWORD)((int)(baseColor.r * attr1Scale + 0.5f));
    const DWORD green = (DWORD)((int)(baseColor.g * attr1Scale + 0.5f));
    const DWORD blue = (DWORD)((int)(baseColor.b * attr1Scale + 0.5f));
    return alphaBits | (((red << 8) | green) << 8) | blue;
}

/**
 * Original-source static helper evidence: source-faithful helper for D3D color-attribute submitters.
 * Purpose: Apply the pending fog color bias and normalize channel for address-backed
 * callers 0x4ab320, 0x4abb20, and 0x4ac370; BN has no standalone retail function.
 */
DWORD PackColorAttrBiased(
    const zVideo_ColorRgbFloat &baseColor,
    float attr1Scale,
    float attr0Value,
    DWORD alphaBits
) {
    float red = baseColor.r * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasR;
    float green = baseColor.g * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasG;
    float blue = baseColor.b * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasB;

    const float channels[3] = {red, green, blue};
    const float selected = channels[g_zVideo_D3DColorNormalizeChannelIndex];
    if (selected > 255.0f) {
        const float scale = 255.0f / selected;
        red *= scale;
        green *= scale;
        blue *= scale;
    }

    const DWORD redByte = (DWORD)((int)(red));
    const DWORD greenByte = (DWORD)((int)(green));
    const DWORD blueByte = (DWORD)((int)(blue));
    return alphaBits | (((redByte << 8) | greenByte) << 8) | blueByte;
}

/**
 * Original-source static helper evidence: source-faithful helper for D3D color-attribute submitters.
 * Purpose: Fill reversed specular alpha values for address-backed callers 0x4ab320,
 * 0x4abb20, and 0x4ac370; BN has no standalone retail function for this body.
 */
void FillColorAttrSpecularReverse(
    const float *attr2,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        DWORD specular = 0xff000000;
        if (attr2 != 0) {
            const float source = attr2[vertexCount - 1 - i];
            specular = (DWORD)((int)(0.5f + (1.0f - source) * 255.0f)) << 24;
        }
        g_zVideo_D3DSubmitTempVertices[i].specular = specular;
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for zVideo_dd3d::SubmitPolyColorAttr.
 * Purpose: Fill reversed D3D colors for caller 0x4ab320, including the optional
 * fog-color bias path; BN has no standalone retail function for this body.
 */
void FillColorAttrColorsReverse(
    const zVideo_ColorRgbFloat &baseColor,
    const float *attr0,
    float attr1Scale,
    DWORD alphaBits,
    int vertexCount
) {
    const DWORD constantColor = PackColorAttrConstant(
        baseColor,
        attr1Scale,
        alphaBits
    );
    for (int i = 0; i < vertexCount; ++i) {
        DWORD color = constantColor;
        if (attr0 != 0) {
            const float attr0Value = attr0[vertexCount - 1 - i];
            if (attr0Value > (1.0f / 255.0f)) {
                color = PackColorAttrBiased(
                    baseColor,
                    attr1Scale,
                    attr0Value,
                    alphaBits
                );
            }
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionsReverse helper behavior for zVideo callers.
 */
void CopyPositionsReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const zVideo_XyzVertex &src = vertices[vertexCount - 1 - i];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered PackAlphaWhite helper behavior for zVideo callers.
 */
DWORD PackAlphaWhite(
    float alpha
) {
    return ((DWORD)((int)(alpha * 255.0f)) << 24) | 0x00ffffff;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered WriteTexturedTlVertex helper behavior for zVideo callers.
 */
void WriteTexturedTlVertex(
    D3DTLVERTEX &dst,
    const zVideo_XyzVertex &src,
    const zVideo_TexCoord &texCoord,
    DWORD color
) {
    dst.sx = src.x;
    dst.sy = src.y;
    dst.sz = src.z;
    dst.rhw = src.z;
    dst.color = color;
    dst.specular = 0xff000000;
    dst.tu = texCoord.u;
    dst.tv = texCoord.v;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyTexturedVerticesReverse helper behavior for zVideo callers.
 */
void CopyTexturedVerticesReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const zVideo_TexCoord *texCoords,
    int vertexCount,
    DWORD color
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        WriteTexturedTlVertex(
            dst[i],
            vertices[sourceIndex],
            texCoords[sourceIndex],
            color
        );
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for polygon submitters.
 * Purpose: Pack a gray polygon color with optional high clamp for address-backed
 * callers 0x4abb20 and 0x4ac370; BN has no standalone retail function.
 */
DWORD PackGrayColor(
    float gray,
    DWORD alphaBits,
    bool clampHigh
) {
    DWORD grayByte = (DWORD)((int)(gray));
    if (clampHigh && grayByte > 0xff) {
        grayByte = 0xff;
    }
    return alphaBits | (((grayByte << 8) | grayByte) << 8) | grayByte;
}

/**
 * Original-source static helper evidence: source-faithful helper for polygon submitters.
 * Purpose: Apply the pending fog color bias and normalize channel for address-backed
 * callers 0x4abb20 and 0x4ac370; BN has no standalone retail function.
 */
DWORD PackPolygonBiasedColor(
    float grayBase,
    float attr0Value,
    DWORD alphaBits
) {
    float red = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasR;
    float green = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasG;
    float blue = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasB;

    const float channels[3] = {red, green, blue};
    const float selected = channels[g_zVideo_D3DColorNormalizeChannelIndex];
    if (selected > 255.0f) {
        const float scale = 255.0f / selected;
        red *= scale;
        green *= scale;
        blue *= scale;
    }

    const DWORD redByte = (DWORD)((int)(red));
    const DWORD greenByte = (DWORD)((int)(green));
    const DWORD blueByte = (DWORD)((int)(blue));
    return alphaBits | (((redByte << 8) | greenByte) << 8) | blueByte;
}

/**
 * Original-source static helper evidence: source-faithful helper for zVideo_dd3d::SubmitPolygon.
 * Purpose: Fill reversed polygon colors for caller 0x4abb20, including the
 * optional fog-color bias path; BN has no standalone retail function.
 */
void FillPolygonColorsReverse(
    const float *attr0,
    float grayBase,
    DWORD alphaBits,
    int vertexCount
) {
    if (attr0 == 0) {
        const DWORD color = PackGrayColor(
            grayBase,
            alphaBits,
            false
        );
        for (int i = 0; i < vertexCount; ++i) {
            g_zVideo_D3DSubmitTempVertices[i].color = color;
        }
        return;
    }

    for (int i = 0; i < vertexCount; ++i) {
        const float attr0Value = attr0[vertexCount - 1 - i];
        DWORD color;
        if (attr0Value > (1.0f / 255.0f)) {
            color = PackPolygonBiasedColor(
                grayBase,
                attr0Value,
                alphaBits
            );
        } else {
            color = PackGrayColor(
                grayBase,
                alphaBits,
                true
            );
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for zVideo_dd3d::SubmitPolygonLit.
 * Purpose: Fill reversed lit polygon colors for caller 0x4ac370, including the
 * optional fog-color bias path; BN has no standalone retail function.
 */
void FillPolygonLitColorsReverse(
    const float *attr1,
    const float *attr0,
    DWORD alphaBits,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const float grayBase = (1.0f - attr1[sourceIndex]) * 255.0f;
        DWORD color;
        if (attr0 != 0 && attr0[sourceIndex] > (1.0f / 255.0f)) {
            color = PackPolygonBiasedColor(
                grayBase,
                attr0[sourceIndex],
                alphaBits
            );
        } else {
            color = PackGrayColor(
                grayBase,
                alphaBits,
                attr0 != 0
            );
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionUvReversePreserveColor helper behavior for zVideo callers.
 */
void CopyPositionUvReversePreserveColor(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const zVideo_TexCoord *uvPairs,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const zVideo_XyzVertex &src = vertices[sourceIndex];
        const zVideo_TexCoord &uv = uvPairs[sourceIndex];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
        dst[i].tu = uv.u;
        dst[i].tv = uv.v;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionUvWithPreparedColorReverse helper behavior for zVideo callers.
 */
void CopyPositionUvWithPreparedColorReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const zVideo_TexCoord *uvPairs,
    const D3DTLVERTEX *prepared,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const zVideo_XyzVertex &src = vertices[sourceIndex];
        const zVideo_TexCoord &uv = uvPairs[sourceIndex];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
        dst[i].color = prepared[i].color;
        dst[i].specular = prepared[i].specular;
        dst[i].tu = uv.u;
        dst[i].tv = uv.v;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered AppendFanCloseVertexIfNeeded helper behavior for zVideo callers.
 */
void AppendFanCloseVertexIfNeeded(
    D3DTLVERTEX *vertices,
    int &count
) {
    if (g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        vertices[count] = vertices[1];
        ++count;
        g_zVideo_D3DAppendFanCloseVertexPending = 0;
    }
}
} // namespace

/**
 * Reimplements 0x4a9b70: zVideo_dd3d::PresentDisplayModeSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: flips the Direct3D display-mode surface, optionally blits the
 * primary surface back to software first, and retries lost or busy surfaces.
 *
 * Evidence: BN uses ECX/EDX for source/destination rects, stack arguments for
 * wait/blit flags, checks g_zVideo_DisplayModeSurfaceState.surf for the 0x400
 * no-surface return, issues DirectDraw Blt and Flip provider calls with
 * DDBLT_WAIT/DDFLIP_WAIT flag construction, retries DDERR_WASSTILLDRAWING,
 * restores and retries DDERR_SURFACELOST, and reports zvid_ddd3d.c line 0xae
 * before returning 0x5a56ffff on unrecoverable provider failure.
 */
int __fastcall PresentDisplayModeSurface(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
) {
    // BN keeps the checked display surface live, then reloads it after Blt/retry paths.
    IDirectDrawSurface3 *displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
    if (displaySurface == 0) {
        return 0x400;
    }

    for (;;) {
        if (blitPrimaryToSwFirst != 0) {
            const DWORD bltFlags = waitForPresent != 0 ? DDBLT_WAIT : 0;
            g_zVideo_SwSurfaceState.surf->Blt(
                (RECT *)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT *)(srcRect),
                bltFlags,
                0
            );
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
        }

        HRESULT hresult = displaySurface->Flip(
            0,
            waitForPresent != 0 ? DDFLIP_WAIT : 0
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_WASSTILLDRAWING) {
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
            continue;
        }

        if (hresult == DDERR_SURFACELOST) {
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
            hresult = displaySurface->Restore();
        }

        if (hresult == DD_OK) {
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
            continue;
        }

        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xae
        );
        return 0x5a56ffff;
    }
}

/**
 * Reimplements 0x4aa0f0: zVideo_dd3d::CreateTextureRecord.
 * Original file: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: validates a zVid image for Direct3D texture limits, creates upload
 * and hardware texture surfaces, loads the texture, and returns the default
 * texture record on validation or provider failure.
 *
 * Evidence: BN assembly checks device texture dimensions, power-of-two and
 * aspect-ratio caps, optionally resamples square-only textures, rejects
 * initially paletted images, builds upload/video DDSURFACEDESC records, calls
 * TexturePixelPack_SetupFromMasks and UploadImageToSurface, performs
 * DirectDraw/Direct3D provider QueryInterface/Load/GetHandle calls, fills the
 * zVideo_TextureRecordPartial fields, and releases temporary provider objects
 * on failure.
 */
zVideo_TextureRecordPartial *__fastcall CreateTextureRecord(
    const char *textureName,
    zVidImagePartial *image,
    int useAlpha,
    int clampU,
    int clampV
) {
    IDirectDrawSurface *uploadSurface = 0;
    IDirectDrawSurface *textureSurface = 0;
    IDirect3DTexture2 *uploadTexture = 0;
    IDirect3DTexture2 *texture = 0;
    IDirectDrawPalette *ddPalette = 0;

    const D3DDEVICEDESC *selectedDeviceDesc =
        &g_zVideo_pSelectedD3DDeviceInfo->m_hwDesc;
    int width = image->width;
    int height = image->height;
    if ((DWORD)(width) > selectedDeviceDesc->dwMaxTextureWidth ||
        (DWORD)(height) > selectedDeviceDesc->dwMaxTextureHeight) {
        zError::ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x20e,
            g_zVideo_TextureTooLargeUsingDefaultFmt,
            textureName,
            width,
            height
        );
        return g_zVideo_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) != 0 &&
        (FloorPowerOfTwo(width) != width || FloorPowerOfTwo(height) != height)) {
        zError::ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x224,
            g_zVideo_TextureNotPowerOf2UsingDefaultFmt,
            textureName,
            image->width,
            image->height
        );
        return g_zVideo_DefaultTextureRecord;
    }

    if (width > height * 8 || height > width * 8) {
        zError::ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x233,
            g_zVideo_TextureBadAspectUsingDefaultFmt,
            textureName,
            width,
            height
        );
        return g_zVideo_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0 &&
        image->width != image->height) {
        const int squareSide = FloorPowerOfTwo((int)(sqrt((double)(height * width))));
        zVid_Image::ResampleSquare(
            image,
            squareSide
        );
        width = image->width;
        height = image->height;
    }

    if (image->palette != 0) {
        zError::ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x24a,
            g_zVideo_TexturePaletteUnsupportedUsingDefaultFmt,
            textureName
        );
        return g_zVideo_DefaultTextureRecord;
    }

    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwHeight = (DWORD)(height);
    desc.dwWidth = (DWORD)(width);
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc.ddpfPixelFormat.dwRGBBitCount = 16;

    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
    DWORD alphaMask;
    if (useAlpha == 0) {
        redBits = g_zVideo_PixelPack.rBits;
        greenBits = g_zVideo_PixelPack.gBits;
        blueBits = g_zVideo_PixelPack.bBits;
        alphaBits = 0;
        redMask = g_zVideo_PixelPack.rMask;
        greenMask = g_zVideo_PixelPack.gMask;
        blueMask = g_zVideo_PixelPack.bMask;
        alphaMask = 0;
    } else {
        desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        if (image->alphaMap != 0) {
            redBits = 4;
            greenBits = 4;
            blueBits = 4;
            alphaBits = 4;
            redMask = 0x0f00;
            greenMask = 0x00f0;
            blueMask = 0x000f;
            alphaMask = 0xf000;
        } else {
            redBits = 5;
            greenBits = 5;
            blueBits = 5;
            alphaBits = 1;
            redMask = 0x7c00;
            greenMask = 0x03e0;
            blueMask = 0x001f;
            alphaMask = 0x8000;
        }
    }

    desc.ddpfPixelFormat.dwRBitMask = redMask;
    desc.ddpfPixelFormat.dwGBitMask = greenMask;
    desc.ddpfPixelFormat.dwBBitMask = blueMask;
    desc.ddpfPixelFormat.dwRGBAlphaBitMask = alphaMask;
    zVideo::TexturePixelPack_SetupFromMasks(
        redBits,
        greenBits,
        blueBits,
        alphaBits,
        redMask,
        greenMask,
        blueMask,
        alphaMask
    );

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        &desc,
        &uploadSurface,
        0
    );
    zVideo_TextureRecordPartial *result = 0;
    if (hresult == DD_OK && image->palette != 0) {
        PALETTEENTRY paletteEntries[256];
        memset(
            paletteEntries,
            0,
            sizeof(paletteEntries)
        );
        memcpy(
            paletteEntries,
            image->palette,
            image->paletteMetaPacked
        );
        hresult = g_zVideo_pDirectDraw2->CreatePalette(
            DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            (LPPALETTEENTRY)(image->palette),
            &ddPalette,
            0
        );
        if (hresult == DD_OK) {
            hresult = uploadSurface->SetPalette(ddPalette);
        }
    }
    if (hresult == DD_OK) {
        UploadImageToSurface(
            uploadSurface,
            image,
            useAlpha
        );
        hresult = uploadSurface->QueryInterface(
            IID_IDirect3DTexture2,
            (void **)(&uploadTexture)
        );
    }

    D3DTEXTUREHANDLE textureHandle = 0;
    if (hresult == DD_OK) {
        desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
        if ((g_zVideo_D3DHalDeviceDesc.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) != 0) {
            desc.ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
        }

        hresult = g_zVideo_pDirectDraw2->CreateSurface(
            &desc,
            &textureSurface,
            0
        );
    }
    if (hresult == DD_OK && ddPalette != 0) {
        hresult = textureSurface->SetPalette(ddPalette);
    }
    if (hresult == DD_OK) {
        hresult = textureSurface->QueryInterface(
            IID_IDirect3DTexture2,
            (void **)(&texture)
        );
    }
    if (hresult == DD_OK) {
        hresult = texture->Load(uploadTexture);
    }
    if (hresult == DD_OK) {
        hresult = texture->GetHandle(
            g_zVideo_pD3DDevice,
            &textureHandle
        );
    }
    if (hresult == DD_OK) {
        result = TextureRecord_Create();
        if (result != 0) {
            result->m_uploadSurface = uploadSurface;
            result->m_textureSurface = textureSurface;
            result->m_texture = texture;
            result->m_textureHandle = textureHandle;
            result->m_alphaMode = useAlpha == 0 ? 1 : (image->alphaMap != 0 ? 4 : 5);
            result->m_uWrapMode = clampU != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
            result->m_vWrapMode = clampV != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
        }
        uploadTexture->Release();
    }

    if (hresult != DD_OK) {
        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x30f
        );
        if (texture != 0) {
            texture->Release();
        }
        if (uploadTexture != 0) {
            uploadTexture->Release();
        }
        if (textureSurface != 0) {
            textureSurface->Release();
        }
        if (uploadSurface != 0) {
            uploadSurface->Release();
        }
    }

    return result != 0 ? result : g_zVideo_DefaultTextureRecord;
}

/**
 * Reimplements 0x4a9c20: zVideo_dd3d::CreateDeviceState.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: creates the Direct3D z-buffer/device/viewport/material state and
 * initializes the fixed render-state defaults for the active software surface.
 *
 * Evidence: BN shows z-buffer surface creation, DirectDraw/Direct3D provider
 * setup, material and caps initialization, ten
 * fixed render-state writes, fog enablement, and quad-batch depth seeding.
 */
int CreateDeviceState() {
    DDSURFACEDESC zBufferDesc = {0};
    D3DVIEWPORT2 viewport2 = {0};
    D3DMATERIAL mat = {0};
    // VC5/BN evidence shows the original C source zeroed this provider record again here.
    memset(
        &zBufferDesc,
        0,
        sizeof(zBufferDesc)
    );
    zBufferDesc.dwWidth = (DWORD)(g_zVideo_SwSurfaceState.width);
    zBufferDesc.dwHeight = (DWORD)(g_zVideo_SwSurfaceState.height);
    g_zVideo_ClearScreenBufferEnabled = 1;
    zBufferDesc.dwSize = sizeof(zBufferDesc);
    zBufferDesc.dwFlags = 0x47;
    zBufferDesc.ddsCaps.dwCaps = 0x24000;
    zBufferDesc.dwMipMapCount = 0x10;

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        &zBufferDesc,
        (IDirectDrawSurface **)(&g_zVideo_pZBufferSurface),
        0
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xd3
        );
    }

    hresult = g_zVideo_pZBufferSurface->QueryInterface(
        IID_IDirectDrawSurface,
        (void **)(&g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xd9
        );
    }

    hresult = g_zVideo_SwSurfaceState.surf->AddAttachedSurface(
        (IDirectDrawSurface3 *)(g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xde
        );
    }

    hresult = g_zVideo_pDirectDraw2->QueryInterface(
        IID_IDirect3D2,
        (void **)(&g_zVideo_pD3D2)
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xe5
        );
    }

    hresult = g_zVideo_pD3D2->CreateDevice(
        *g_zVideo_pSelectedD3DDeviceInfo->pD3DDeviceGuid,
        (IDirectDrawSurface *)(g_zVideo_SwSurfaceState.surf),
        &g_zVideo_pD3DDevice
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xed
        );
    }

    hresult = g_zVideo_pD3D2->CreateViewport(
        &g_zVideo_pD3DViewport2,
        0
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xf4
        );
    }

    hresult = g_zVideo_pD3DDevice->AddViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xf9
        );
    }

    const DWORD width = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    const DWORD height = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);
    viewport2.dwSize = sizeof(viewport2);
    viewport2.dwX = 0;
    viewport2.dwY = 0;
    viewport2.dwWidth = width;
    viewport2.dwHeight = height;
    viewport2.dvClipX = 0.0f;
    viewport2.dvClipY = 0.0f;
    viewport2.dvClipWidth = (D3DVALUE)(width);
    viewport2.dvClipHeight = (D3DVALUE)(height);
    viewport2.dvMinZ = 0.0f;
    viewport2.dvMaxZ = 1.0f;

    hresult = g_zVideo_pD3DViewport2->SetViewport2(&viewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x10a
        );
    }

    hresult = g_zVideo_pD3DDevice->SetCurrentViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x10f
        );
    }

    hresult = g_zVideo_pD3D2->CreateMaterial(
        &g_zVideo_pD3DMaterial2,
        0
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x116
        );
    }

    mat.dwSize = sizeof(mat);
    mat.diffuse.b = 0.0f;
    mat.diffuse.g = 0.0f;
    mat.diffuse.r = 0.0f;
    mat.ambient.b = 1.0f;
    mat.ambient.g = 1.0f;
    mat.ambient.r = 1.0f;
    mat.dwRampSize = 0x100;

    hresult = g_zVideo_pD3DMaterial2->SetMaterial(&mat);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x124
        );
    }

    hresult = g_zVideo_pD3DMaterial2->GetHandle(
        g_zVideo_pD3DDevice,
        &g_zVideo_D3DMaterialHandle
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x12a
        );
    }

    hresult = g_zVideo_pD3DViewport2->SetBackground(g_zVideo_D3DMaterialHandle);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x12f
        );
    }

    g_zVideo_D3DHelDeviceDesc.dwSize = sizeof(g_zVideo_D3DHelDeviceDesc);
    g_zVideo_D3DHalDeviceDesc.dwSize = sizeof(g_zVideo_D3DHalDeviceDesc);
    hresult = g_zVideo_pD3DDevice->GetCaps(
        &g_zVideo_D3DHalDeviceDesc,
        &g_zVideo_D3DHelDeviceDesc
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x139
        );
    }

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_CULLMODE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZENABLE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        7
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SPECULARENABLE,
        0
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SHADEMODE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREPERSPECTIVE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREMAG,
        2
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREMIN,
        2
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SRCBLEND,
        5
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_DESTBLEND,
        6
    );

    g_zVideo_PendingWireframeState = -1;
    SetFogEnable(1);
    SetQuadBatchDepthAndRhw(0.99000001f);
    return 0;
}

/**
 * Reimplements 0x4aa9e0: zVideo_dd3d::SetFogEnable.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-enable render state and force the
 * fixed fog light-state mode.
 *
 * Evidence: BN compares g_zVideo_CachedFogEnableRenderState before calling
 * IDirect3DDevice2::SetRenderState(D3DRENDERSTATE_FOGENABLE), stores the new
 * enable value, then ensures D3DLIGHTSTATE_FOGMODE is D3DFOG_LINEAR through
 * IDirect3DDevice2::SetLightState.
 */
void __fastcall SetFogEnable(
    int enable
) {
    if (g_zVideo_CachedFogEnableRenderState != enable) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_FOGENABLE,
            (DWORD)(enable)
        );
        g_zVideo_CachedFogEnableRenderState = enable;
    }

    if (g_zVideo_CachedFogModeLightState != 3) {
        g_zVideo_pD3DDevice->SetLightState(
            D3DLIGHTSTATE_FOGMODE,
            D3DFOG_LINEAR
        );
        g_zVideo_CachedFogModeLightState = 3;
    }
}

/**
 * Reimplements 0x4aaa30: zVideo_dd3d::SetFogStart.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-start light state only when the
 * requested start distance changes.
 *
 * Evidence: BN shows a stdcall float argument, x87 comparison against
 * g_zVideo_CachedFogStartLightStateValue, then a Direct3D provider
 * SetLightState call with selector D3DLIGHTSTATE_FOGSTART and the raw
 * fogStart float bits before updating the cache.
 */
void __stdcall SetFogStart(
    float fogStart
) {
    if (g_zVideo_CachedFogStartLightStateValue != fogStart) {
        g_zVideo_pD3DDevice->SetLightState(
            (D3DLIGHTSTATETYPE)(5),
            *(DWORD *)(&fogStart)
        );
        g_zVideo_CachedFogStartLightStateValue = fogStart;
    }
}

/**
 * Reimplements 0x4aaa60: zVideo_dd3d::SetFogEnd.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-end light-state value only when
 * the requested end distance changes.
 *
 * Evidence: BN shows a stdcall float argument, x87 comparison against
 * g_zVideo_CachedFogEndLightStateValue, then a Direct3D provider
 * SetLightState call using selector 5 with the raw fogEnd float bits before
 * updating the cache. The selector is the retail oddity: SetFogEnd pushes
 * D3DLIGHTSTATE_FOGSTART rather than D3DLIGHTSTATE_FOGEND.
 */
void __stdcall SetFogEnd(
    float fogEnd
) {
    if (g_zVideo_CachedFogEndLightStateValue != fogEnd) {
        g_zVideo_pD3DDevice->SetLightState(
            (D3DLIGHTSTATETYPE)(5),
            *(DWORD *)(&fogEnd)
        );
        g_zVideo_CachedFogEndLightStateValue = fogEnd;
    }
}

/**
 * Reimplements 0x4aaa90: zVideo_dd3d::ApplyFogStateFromGlobals.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: apply pending global fog enable, color, mode, start, and end
 * state to the active Direct3D device.
 *
 * Evidence: BN shows stdcall fogStart, fogEnd, and unused float arguments
 * with ret 0x0c. The function emits FOGENABLE=1, packs pending RGB globals
 * into FOGCOLOR via the recovered zvid_ddd3d.c helper expression, then sends
 * linear fog mode, raw fogStart bits to D3DLIGHTSTATE_FOGSTART, and raw
 * fogEnd bits to D3DLIGHTSTATE_FOGEND through IDirect3DDevice2 providers.
 */
void __stdcall ApplyFogStateFromGlobals(
    float fogStart,
    float fogEnd,
    float unused
) {
    (void)unused;
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_FOGENABLE,
        1
    );

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_FOGCOLOR,
        PackFogColorFrom255Floats(
            g_zVideo_FogColorPendingR255,
            g_zVideo_FogColorPendingG255,
            g_zVideo_FogColorPendingB255
        )
    );

    g_zVideo_pD3DDevice->SetLightState(
        D3DLIGHTSTATE_FOGMODE,
        D3DFOG_LINEAR
    );
    g_zVideo_pD3DDevice->SetLightState(
        (D3DLIGHTSTATETYPE)(5),
        *(DWORD *)(&fogStart)
    );
    g_zVideo_pD3DDevice->SetLightState(
        (D3DLIGHTSTATETYPE)(6),
        *(DWORD *)(&fogEnd)
    );
}

/**
 * Reimplements 0x4aab30: zVideo_dd3d::UpdateFogColor.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: upload the applied global fog RGB floats as a packed Direct3D
 * fog-color render state.
 *
 * Evidence: BN shows a cdecl no-argument function that packs
 * g_zVideo_FogColorAppliedR255/G255/B255 with the same +0.5f, _ftol,
 * 0xRRGGBB sequence used by 0x4aaa90, then calls the Direct3D provider
 * SetRenderState for D3DRENDERSTATE_FOGCOLOR.
 */
void UpdateFogColor() {
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_FOGCOLOR,
        PackFogColorFrom255Floats(
            g_zVideo_FogColorAppliedR255,
            g_zVideo_FogColorAppliedG255,
            g_zVideo_FogColorAppliedB255
        )
    );
}

/**
 * Reimplements 0x4accc0: zVideo_dd3d::SetQuadBatchDepthAndRhw.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: stamp the current Direct3D quad-batch depth and reciprocal
 * homogeneous weight across all cached TL vertices.
 *
 * Evidence: BN uses a bottomRight.z cursor and writes each item's TL vertices
 * in bottom-left, bottom-right, top-right, top-left order for z, then the
 * same order for rhw.
 */
void __stdcall SetQuadBatchDepthAndRhw(
    float depthAndRhw
) {
    for (int itemIndex = 0; itemIndex < 16; ++itemIndex) {
        zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[itemIndex];

        item.vertices[3].sz = depthAndRhw;
        item.vertices[2].sz = depthAndRhw;
        item.vertices[1].sz = depthAndRhw;
        item.vertices[0].sz = depthAndRhw;
        item.vertices[3].rhw = depthAndRhw;
        item.vertices[2].rhw = depthAndRhw;
        item.vertices[1].rhw = depthAndRhw;
        item.vertices[0].rhw = depthAndRhw;
    }
}

/**
 * Reimplements 0x4aab90: zVideo_dd3d::SubmitPolyFlatColor16.
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert flat 16-bit color polygons to Direct3D TL vertices and
 * submit them through the immediate, overwrite, or sorted transparent path.
 */
void __fastcall SubmitPolyFlatColor16(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
) {
    const DWORD packedColor = PackD3DColorFrom16(
        packedColor16,
        alpha
    );

    if (alpha >= 0xff) {
        CopyFlatVerticesReverse(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            vertexCount,
            packedColor
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x503,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedsFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 1;
            entry.vertexCount = vertexCount;
            entry.renderClass = 0;
            entry.renderParam = renderParam;
            if (vertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                0
            );
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }
        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                1
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x520
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x528,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = renderParam;
        if (vertexCount > 0) {
            CopyFlatVerticesReverse(
                entry.vertices,
                vertices,
                vertexCount,
                packedColor
            );
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x547,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = 0;
    entry.renderParam = renderParam;
    if (vertexCount > 0) {
        CopyFlatVerticesReverse(
            entry.vertices,
            vertices,
            vertexCount,
            packedColor
        );
    }
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Reimplements 0x4aaef0: zVideo_dd3d::SubmitPolyGouraudColor16.
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert per-vertex 16-bit color polygons to Direct3D TL vertices
 * and submit them through the immediate, overwrite, or sorted transparent path.
 */
void __fastcall SubmitPolyGouraudColor16(
    zVideo_XyzVertex *vertices,
    unsigned int *packedColors16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
) {
    if (alpha >= 0xff) {
        CopyGouraudVerticesReverse(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            packedColors16,
            vertexCount,
            alpha
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x59d,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 2;
            entry.vertexCount = vertexCount;
            entry.renderClass = 0;
            entry.renderParam = renderParam;
            if (vertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                0
            );
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }
        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                1
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x5bb
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x5c3,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = renderParam;
        if (vertexCount > 0) {
            CopyGouraudVerticesReverse(
                entry.vertices,
                vertices,
                packedColors16,
                vertexCount,
                alpha
            );
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x5e2,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = 0;
    entry.renderParam = renderParam;
    if (vertexCount > 0) {
        CopyGouraudVerticesReverse(
            entry.vertices,
            vertices,
            packedColors16,
            vertexCount,
            alpha
        );
    }
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Reimplements 0x4ab320: zVideo_dd3d::SubmitPolyColorAttr.
 * Purpose: Build color-attribute TL vertices and either draw immediately or queue
 * the overwrite polygon path.
 */
void __fastcall SubmitPolyColorAttr(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat *baseColor,
    float *attr1,
    float *attr0,
    float *attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
) {
    (void)packedColor16;

    const float attr1Scale = 1.0f - *attr1;
    const DWORD alphaBits = alpha < 0xff ? (DWORD)(alpha << 24) : 0xff000000;

    FillColorAttrSpecularReverse(
        attr2,
        vertexCount
    );
    FillColorAttrColorsReverse(
        *baseColor,
        attr0,
        attr1Scale,
        alphaBits,
        vertexCount
    );

    if (alpha < 0xff) {
        return;
    }

    CopyPositionsReverse(
        g_zVideo_D3DSubmitTempVertices,
        vertices,
        vertexCount
    );

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x69c,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 3;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = (int)(renderParam);
        if (vertexCount > 0) {
            memcpy(
                entry.vertices,
                g_zVideo_D3DSubmitTempVertices,
                (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
            );
        }
        return;
    }

    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_TEXTUREHANDLE,
            0
        );
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }
    if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            1
        );
        g_zVideo_D3DRenderStateCache.shadeMode = 1;
    }

    const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
        (D3DPRIMITIVETYPE)(6),
        (D3DVERTEXTYPE)(3),
        g_zVideo_D3DSubmitTempVertices,
        (DWORD)(vertexCount),
        0
    );
    if (hresult != DD_OK) {
        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x6ba
        );
    }
}

/**
 * Reimplements 0x4ab6d0: zVideo_dd3d::SubmitPolyRenderClass.
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Prepare textured TL vertices for a render class and route them to
 * immediate Direct3D drawing or the overwrite/sorted polygon queues.
 */
void __fastcall SubmitPolyRenderClass(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
) {
    if (renderClass == 0) {
        renderClass = (zVideo_RenderClass *)(g_zVideo_DefaultTextureRecord);
        if (renderClass == 0) {
            return;
        }
    }
    const bool opaquePath = renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f;

    if (opaquePath) {
        CopyTexturedVerticesReverse(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            texCoords,
            vertexCount,
            g_zVideo_OpaqueWhiteArgb
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x6fd,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 4;
            entry.vertexCount = vertexCount;
            entry.renderClass = (int)(renderClass);
            entry.renderParam = (int)(renderParam);
            if (vertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                1
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                renderClass->textureHandle
            );
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != renderClass->textureMapBlend) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREMAPBLEND,
                renderClass->textureMapBlend
            );
            g_zVideo_D3DRenderStateCache.textureMapBlend = renderClass->textureMapBlend;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSU,
                renderClass->textureAddressU
            );
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSV,
                renderClass->textureAddressV
            );
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x71d
            );
        }
        return;
    }

    const DWORD alphaWhite = PackAlphaWhite(alpha);
    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x725,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = (int)(renderClass);
        entry.renderParam = (int)(renderParam);
        if (vertexCount > 0) {
            CopyTexturedVerticesReverse(
                entry.vertices,
                vertices,
                texCoords,
                vertexCount,
                alphaWhite
            );
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x74c,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = (int)(renderClass);
    entry.renderParam = (int)(renderParam);
    if (vertexCount > 0) {
        CopyTexturedVerticesReverse(
            entry.vertices,
            vertices,
            texCoords,
            vertexCount,
            alphaWhite
        );
    }
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Reimplements 0x4abb20: zVideo_dd3d::SubmitPolygon.
 * Purpose: Build textured polygon TL vertices with fog color-attribute bias and
 * route them to immediate, overwrite, or sorted transparent submission.
 */
void __fastcall SubmitPolygon(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
) {
    const float attr1Scale = 1.0f - *attr1;
    const DWORD alphaBits = alpha < 1.0f ? ((DWORD)((int)(alpha * 255.0f)) << 24) : 0xff000000;
    const float grayBase = attr1Scale * 255.0f;

    FillColorAttrSpecularReverse(
        attr2,
        vertexCount
    );
    FillPolygonColorsReverse(
        attr0,
        grayBase,
        alphaBits,
        vertexCount
    );

    const bool opaquePath = renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f;

    if (opaquePath) {
        int preparedVertexCount = vertexCount;
        CopyPositionUvReversePreserveColor(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            uvPairs,
            preparedVertexCount
        );
        AppendFanCloseVertexIfNeeded(
            g_zVideo_D3DSubmitTempVertices,
            preparedVertexCount
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x82a,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 5;
            entry.vertexCount = preparedVertexCount;
            entry.renderClass = (int)(renderClass);
            entry.renderParam = (int)(renderParam);
            if (preparedVertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(preparedVertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                2
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 2;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                renderClass->textureHandle
            );
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREMAPBLEND,
                2
            );
            g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSU,
                renderClass->textureAddressU
            );
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSV,
                renderClass->textureAddressV
            );
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(preparedVertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x84a
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x853,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.renderClass = (int)(renderClass);
        entry.renderParam = (int)(renderParam);
        int preparedVertexCount = vertexCount;
        if (vertexCount > 0) {
            CopyPositionUvWithPreparedColorReverse(
                entry.vertices,
                vertices,
                uvPairs,
                g_zVideo_D3DSubmitTempVertices,
                vertexCount
            );
        }
        AppendFanCloseVertexIfNeeded(
            entry.vertices,
            preparedVertexCount
        );
        entry.vertexCount = preparedVertexCount;
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x88a,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.renderClass = (int)(renderClass);
    entry.renderParam = (int)(renderParam);
    int preparedVertexCount = vertexCount;
    if (vertexCount > 0) {
        CopyPositionUvWithPreparedColorReverse(
            entry.vertices,
            vertices,
            uvPairs,
            g_zVideo_D3DSubmitTempVertices,
            vertexCount
        );
    }
    AppendFanCloseVertexIfNeeded(
        entry.vertices,
        preparedVertexCount
    );
    entry.vertexCount = preparedVertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Reimplements 0x4ac370: zVideo_dd3d::SubmitPolygonLit.
 * Purpose: Build lit textured polygon TL vertices with fog color-attribute bias
 * and route them to immediate, overwrite, or sorted transparent submission.
 */
void __fastcall SubmitPolygonLit(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
) {
    const DWORD alphaBits = alpha < 1.0f ? ((DWORD)((int)(alpha * 255.0f)) << 24) : 0xff000000;

    FillColorAttrSpecularReverse(
        attr2,
        vertexCount
    );
    FillPolygonLitColorsReverse(
        attr1,
        attr0,
        alphaBits,
        vertexCount
    );

    const bool opaquePath = renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f;

    if (opaquePath) {
        int preparedVertexCount = vertexCount;
        CopyPositionUvReversePreserveColor(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            uvPairs,
            preparedVertexCount
        );
        AppendFanCloseVertexIfNeeded(
            g_zVideo_D3DSubmitTempVertices,
            preparedVertexCount
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x983,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 6;
            entry.vertexCount = preparedVertexCount;
            entry.renderClass = (int)(renderClass);
            entry.renderParam = (int)(renderParam);
            if (preparedVertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(preparedVertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                2
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 2;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                renderClass->textureHandle
            );
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREMAPBLEND,
                2
            );
            g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSU,
                renderClass->textureAddressU
            );
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSV,
                renderClass->textureAddressV
            );
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(preparedVertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x9a4
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x9ad,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.renderClass = (int)(renderClass);
        entry.renderParam = (int)(renderParam);
        int preparedVertexCount = vertexCount;
        if (vertexCount > 0) {
            CopyPositionUvWithPreparedColorReverse(
                entry.vertices,
                vertices,
                uvPairs,
                g_zVideo_D3DSubmitTempVertices,
                vertexCount
            );
        }
        AppendFanCloseVertexIfNeeded(
            entry.vertices,
            preparedVertexCount
        );
        entry.vertexCount = preparedVertexCount;
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x9e4,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.renderClass = (int)(renderClass);
    entry.renderParam = (int)(renderParam);
    int preparedVertexCount = vertexCount;
    if (vertexCount > 0) {
        CopyPositionUvWithPreparedColorReverse(
            entry.vertices,
            vertices,
            uvPairs,
            g_zVideo_D3DSubmitTempVertices,
            vertexCount
        );
    }
    AppendFanCloseVertexIfNeeded(
        entry.vertices,
        preparedVertexCount
    );
    entry.vertexCount = preparedVertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Reimplements 0x4acbd0: zVideo_dd3d::DrawPointColor16.
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert one 16-bit colored point to a Direct3D TL vertex and draw it
 * through the cached point-list render-state path.
 */
void __fastcall DrawPointColor16(
    zVideo_XyzVertex *pointPos,
    unsigned int packedColor16,
    int pointCount
) {
    (void)pointCount;

    D3DTLVERTEX &vertex = g_zVideo_D3DSubmitTempVertices[0];
    vertex.sx = pointPos->x;
    vertex.sy = pointPos->y;
    vertex.sz = pointPos->z;
    vertex.rhw = pointPos->z;
    vertex.color = PackD3DColorFrom16(
        packedColor16,
        0xff
    );
    vertex.specular = 0xff000000;

    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_TEXTUREHANDLE,
            0
        );
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }
    if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            1
        );
        g_zVideo_D3DRenderStateCache.shadeMode = 1;
    }

    const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
        (D3DPRIMITIVETYPE)(1),
        (D3DVERTEXTYPE)(3),
        g_zVideo_D3DSubmitTempVertices,
        1,
        0
    );
    if (hresult != DD_OK) {
        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xa4c
        );
    }
}

/**
 * Reimplements 0x4acd00: zVideo_dd3d::QueueSolidQuad
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Queue one alpha-blended solid screen-space quad for the Direct3D batch flush.
 */
void __fastcall QueueSolidQuad(
    unsigned int packedColor16,
    zVidRect32 *clipRect,
    double alpha
) {
    const int batchIndex = g_zVideo_QuadBatchCount;
    if ((unsigned int)(batchIndex) >= 0x10) {
        return;
    }

    zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[batchIndex];

    float left;
    float top;
    float right;
    float bottom;
    if (clipRect != 0) {
        left = (float)(clipRect->left);
        top = (float)(clipRect->top);
        right = (float)(clipRect->right);
        bottom = (float)(clipRect->bottom);
    } else {
        left = 0.0f;
        top = 0.0f;
        right = (float)(g_zVideo_PrimarySurfaceState.height);
        bottom = (float)(g_zVideo_PrimarySurfaceState.width);
    }

    item.vertices[0].sx = left;
    item.vertices[0].sy = top;
    item.vertices[1].sx = right;
    item.vertices[1].sy = top;
    item.vertices[2].sx = right;
    item.vertices[2].sy = bottom;
    item.vertices[3].sx = left;
    item.vertices[3].sy = bottom;

    const int alphaByte = (int)(alpha * 255.0);
    const DWORD packedColor = PackD3DColorFrom16(
        packedColor16,
        alphaByte
    );
    for (int i = 0; i < 4; ++i) {
        item.vertices[i].color = packedColor;
    }

    ++g_zVideo_QuadBatchCount;
}

/**
 * Reimplements 0x4ace30: zVideo_dd3d::FlushSortedPolys
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Sort and draw queued Direct3D polys while maintaining the shared render-state cache.
 */
void FlushSortedPolys() {
    int queueCount = g_zVideo_SortedPolyQueueCount;
    if (queueCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            2
        );
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderStateCache.shadeMode = 2;
    }
    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            1
        );
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            0
        );
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
    }

    for (unsigned int i = 0; i < (unsigned int)(queueCount); ++i) {
        g_zVideo_SortedPolyDrawOrder[i] = queueCount - (int)(i)-1;
        queueCount = g_zVideo_SortedPolyQueueCount;
    }

    bool swapped;
    do {
        swapped = false;
        for (unsigned int i = 1; i < (unsigned int)(queueCount); ++i) {
            const int currentIndex = g_zVideo_SortedPolyDrawOrder[i];
            const int previousIndex = g_zVideo_SortedPolyDrawOrder[i - 1];
            if (g_zVideo_SortedPolyQueueBase[currentIndex].vertices[0].sz <
                g_zVideo_SortedPolyQueueBase[previousIndex].vertices[0].sz) {
                g_zVideo_SortedPolyDrawOrder[i - 1] = currentIndex;
                g_zVideo_SortedPolyDrawOrder[i] = previousIndex;
                queueCount = g_zVideo_SortedPolyQueueCount;
                swapped = true;
            }
        }
    } while (swapped);

    for (unsigned int i_4102 = 0; i_4102 < (unsigned int)(g_zVideo_SortedPolyQueueCount);
        ++i_4102) {
        const int drawIndex = g_zVideo_SortedPolyDrawOrder[i_4102];
        zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[drawIndex];
        zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);

        if (renderClass != 0) {
            if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
            }

            const D3DTEXTUREBLEND textureMapBlend = renderClass->textureMapBlend;
            const bool forceTransparentTextureBlend =
                textureMapBlend != (D3DTEXTUREBLEND)(4) &&
                (entry.vertices[0].color & 0xff000000) != 0xff000000;
            if (forceTransparentTextureBlend) {
                if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(4)) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREMAPBLEND,
                        4
                    );
                    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(4);
                }
            } else if (g_zVideo_D3DRenderStateCache.textureMapBlend != textureMapBlend) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREMAPBLEND,
                    textureMapBlend
                );
                g_zVideo_D3DRenderStateCache.textureMapBlend = textureMapBlend;
            }

            if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
            }
        } else if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                0
            );
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            D3DPT_TRIANGLEFAN,
            (D3DVERTEXTYPE)(3),
            entry.vertices,
            (DWORD)(entry.vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0xb09
            );
        }
    }

    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            0
        );
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            1
        );
        g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    }
    g_zVideo_SortedPolyQueueCount = 0;
}

/**
 * Reimplements 0x4ad120: zVideo_dd3d::FlushQuadBatch
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Draw and clear the Direct3D solid-quad batch with cached render-state setup and restoration.
 */
void FlushQuadBatch() {
    if (g_zVideo_QuadBatchCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            2
        );
        g_zVideo_D3DRenderStateCache.shadeMode = 2;
    }
    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            1
        );
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            0
        );
        g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_TEXTUREHANDLE,
            0
        );
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_ALWAYS
    );

    for (unsigned int i = 0; i < (unsigned int)(g_zVideo_QuadBatchCount); ++i) {
        g_zVideo_pD3DDevice->DrawPrimitive(
            D3DPT_TRIANGLEFAN,
            (D3DVERTEXTYPE)(3),
            g_zVideo_QuadBatchItemsBase[i].vertices,
            4,
            0
        );
    }

    g_zVideo_QuadBatchCount = 0;
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_GREATEREQUAL
    );

    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            0
        );
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            1
        );
        g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    }
}

/**
 * Reimplements 0x4ad250: zVideo_dd3d::FlushOverwritePolys
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Draw overwrite-queue primitives with the Direct3D render-state cache and restore depth testing.
 */
void FlushOverwritePolys() {
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_ALWAYS
    );

    for (int i = 0; i < g_zVideo_OverwriteQueueCount; ++i) {
        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[i];
        HRESULT hresult = DD_OK;

        switch (entry.type) {
        case 0: {
            if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    2
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 2;
            }
            if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ALPHABLENDENABLE,
                    1
                );
                g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
            }
            if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ZWRITEENABLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
            }

            zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);
            if (renderClass != 0) {
                if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREHANDLE,
                        renderClass->textureHandle
                    );
                    g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
                }

                const D3DTEXTUREBLEND textureMapBlend = renderClass->textureMapBlend;
                const bool forceTransparentTextureBlend =
                    textureMapBlend != (D3DTEXTUREBLEND)(4) &&
                    (entry.vertices[0].color & 0xff000000) != 0xff000000;
                if (forceTransparentTextureBlend) {
                    if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(4)) {
                        g_zVideo_pD3DDevice->SetRenderState(
                            D3DRENDERSTATE_TEXTUREMAPBLEND,
                            4
                        );
                        g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(4);
                    }
                } else if (g_zVideo_D3DRenderStateCache.textureMapBlend != textureMapBlend) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREMAPBLEND,
                        textureMapBlend
                    );
                    g_zVideo_D3DRenderStateCache.textureMapBlend = textureMapBlend;
                }

                if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREADDRESSU,
                        renderClass->textureAddressU
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
                }
                if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREADDRESSV,
                        renderClass->textureAddressV
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
                }
            } else if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.textureHandle = 0;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );

            if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ALPHABLENDENABLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
            }
            if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ZWRITEENABLE,
                    1
                );
                g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
            }
            break;
        }

        case 1:
        case 2:
        case 3:
            if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.textureHandle = 0;
            }
            if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    1
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 1;
            }
            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );
            break;

        case 4: {
            if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    1
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 1;
            }

            zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);
            if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderStateCache.textureMapBlend != renderClass->textureMapBlend) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREMAPBLEND,
                    renderClass->textureMapBlend
                );
                g_zVideo_D3DRenderStateCache.textureMapBlend = renderClass->textureMapBlend;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );
            break;
        }

        case 5:
        case 6: {
            if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    2
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 2;
            }

            zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);
            if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREMAPBLEND,
                    2
                );
                g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );
            break;
        }

        default:
            break;
        }

        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0xbb7
            );
        }
    }

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_GREATEREQUAL
    );
    g_zVideo_OverwriteQueueCount = 0;
}

/**
 * Reimplements 0x4ad680: zVideo_dd3d::FloorPowerOfTwo.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: return the largest power of two less than or equal to the supplied
 * value.
 *
 * Evidence: BN starts from one, shifts left until the running power reaches or
 * exceeds the input, returns the input on exact match, otherwise shifts once
 * back down before returning.
 */
int __fastcall FloorPowerOfTwo(
    int value
) {
    int powerOfTwo = 1;
    do {
        powerOfTwo <<= 1;
    } while (powerOfTwo < value);

    if (powerOfTwo == value) {
        return value;
    }

    return powerOfTwo >> 1;
}

/**
 * Reimplements 0x4aa9d0: zVideo_dd3d::TextureRecord_Create.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: allocates a zeroed Direct3D texture-record structure.
 *
 * Evidence: BN assembly is a leaf that calls calloc(1, 0x1c) and returns the
 * provider result directly; zVideo_TextureRecordPartial is asserted to 0x1c.
 */
zVideo_TextureRecordPartial *TextureRecord_Create() {
    return (zVideo_TextureRecordPartial *)(calloc(
        1,
        sizeof(zVideo_TextureRecordPartial)
    ));
}

/**
 * Reimplements 0x4aa8b0: zVideo_dd3d::TextureRecord_LockUploadSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: locks a texture record's upload surface and returns the provider
 * pixel pointer and row pitch to the caller.
 *
 * Evidence: BN loads m_uploadSurface at offset zero, calls
 * zVideo_dd::LockSurface_WaitRestore with an uninitialized stack
 * DDSURFACEDESC, copies lpSurface and lPitch to the output pointers only on
 * success, and returns one or zero. The callee owns descriptor clearing and
 * dwSize initialization.
 */
int __fastcall TextureRecord_LockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord,
    void **outPixels,
    int *outPitchBytes
) {
    DDSURFACEDESC lockedDescOut;
    if (zVideo_dd::LockSurface_WaitRestore(
            (IDirectDrawSurface3 *)(textureRecord->m_uploadSurface),
            &lockedDescOut
        ) == 0) {
        *outPitchBytes = lockedDescOut.lPitch;
        *outPixels = lockedDescOut.lpSurface;
        return 1;
    }

    return 0;
}

/**
 * Reimplements 0x4aa6f0: zVideo_dd3d::ConvertImagePixelsForTexture.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: converts zVid 16-bit image pixels into the active Direct3D texture
 * upload pixel format, including alpha-map expansion when present.
 *
 * Evidence: BN assembly has no callees, walks image rows by destination pitch,
 * ignores the useAlpha argument, uses g_zVideo_PixelPack masks for opaque
 * pixels, and selects the 565 versus 555 alpha-map channel shifts from
 * g_zVideo_PixelPack.gBits.
 */
void __fastcall ConvertImagePixelsForTexture(
    unsigned short *dstPixels,
    zVidImagePartial *image,
    int pitchBytes,
    int useAlpha
) {
    (void)useAlpha;

    char *alphaMap = image->alphaMap;
    unsigned short *srcPixels = (unsigned short *)(image->pixels);
    unsigned char *dstRowBytes = (unsigned char *)(dstPixels);

    if (alphaMap == 0) {
        const unsigned int redGreenMask = g_zVideo_PixelPack.rMask | g_zVideo_PixelPack.gMask;
        {
            for (int row = 0; row < image->height; ++row) {
                unsigned short *dstCursor = (unsigned short *)(dstRowBytes);
                {
                    for (int column = 0; column < image->width; ++column) {
                        const unsigned short src = *srcPixels++;
                        const unsigned short alphaBit = src != 0 ? 0x8000 : 0;
                        *dstCursor++ =
                            (unsigned short)((src & g_zVideo_PixelPack.bMask) |
                                             ((src >> 1) & (redGreenMask >> 1)) | alphaBit);
                    }
                }
                dstRowBytes += pitchBytes;
            }
        }
        return;
    }

    unsigned char *alphaCursor = (unsigned char *)(alphaMap);
    unsigned int redAlphaMask;
    int redAlphaShift;
    unsigned int greenAlphaMask;
    int greenAlphaShift;
    if (g_zVideo_PixelPack.gBits == 6) {
        redAlphaMask = 0xf000;
        redAlphaShift = 4;
        greenAlphaMask = 0x780;
        greenAlphaShift = 3;
    } else {
        redAlphaMask = 0x7800;
        redAlphaShift = 3;
        greenAlphaMask = 0x3c0;
        greenAlphaShift = 2;
    }

    {
        for (int row = 0; row < image->height; ++row) {
            unsigned short *dstCursor = (unsigned short *)(dstRowBytes);
            {
                for (int column = 0; column < image->width; ++column) {
                    const unsigned short src = *srcPixels++;
                    const unsigned int alpha = (*alphaCursor++ & 0xf0) << 8;
                    *dstCursor++ =
                        (unsigned short)(((src >> 1) & (g_zVideo_PixelPack.bMask >> 1)) |
                                         ((greenAlphaMask & src) >> greenAlphaShift) |
                                         ((redAlphaMask & src) >> redAlphaShift) | alpha);
                }
            }
            dstRowBytes += pitchBytes;
        }
    }
}

/**
 * Reimplements 0x4aa600: zVideo_dd3d::UploadImageToSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: locks a DirectDraw upload surface, copies or converts image pixels
 * into its pitch layout, and unlocks the surface after upload.
 *
 * Evidence: BN assembly locks through zVideo_dd::LockSurface_WaitRestore,
 * chooses ConvertImagePixelsForTexture only when useAlpha is nonzero, otherwise
 * copies either one contiguous block or one row per pitch, then unlocks through
 * zVideo_dd::UnlockSurface_WaitRestore and returns one unconditionally.
 */
int __fastcall UploadImageToSurface(
    IDirectDrawSurface *uploadSurface,
    zVidImagePartial *image,
    int useAlpha
) {
    DDSURFACEDESC lockedDescOut = {0};
    zVideo_dd::LockSurface_WaitRestore(
        (IDirectDrawSurface3 *)(uploadSurface),
        &lockedDescOut
    );

    unsigned char *dstPixels = (unsigned char *)(lockedDescOut.lpSurface);
    unsigned char *srcPixels = (unsigned char *)(image->pixels);
    if (useAlpha != 0) {
        ConvertImagePixelsForTexture(
            (unsigned short *)(dstPixels),
            image,
            lockedDescOut.lPitch,
            useAlpha
        );
    } else {
        const int width = image->width;
        const int height = image->height;
        if (lockedDescOut.lPitch == width) {
            const int bytesPerPixel = (g_zVideo_DisplayModeBpp + 7) >> 3;
            memcpy(
                dstPixels,
                srcPixels,
                (size_t)(height * bytesPerPixel * width)
            );
        } else {
            const int rowCopyBytes = (g_zVideo_DisplayModeBpp * width + 7) >> 3;
            {
                for (int row = 0; row < height; ++row) {
                    memcpy(
                        dstPixels,
                        srcPixels,
                        (size_t)(rowCopyBytes)
                    );
                    dstPixels += lockedDescOut.lPitch;
                    srcPixels += width << 1;
                }
            }
        }
    }

    zVideo_dd::UnlockSurface_WaitRestore((IDirectDrawSurface3 *)(uploadSurface));
    return 1;
}

/**
 * Reimplements 0x4aa8f0: zVideo_dd3d::TextureRecord_UnlockUploadSurface.
 * Original file: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: unlocks a texture record's upload surface and normalizes provider
 * success to a one-or-zero result.
 *
 * Evidence: BN loads m_uploadSurface at offset zero, calls
 * zVideo_dd::UnlockSurface_WaitRestore, and uses neg/sbb/inc to return one
 * only when the unlock wrapper returns zero.
 */
int __fastcall TextureRecord_UnlockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord
) {
    return zVideo_dd::UnlockSurface_WaitRestore(
               (IDirectDrawSurface3 *)(textureRecord->m_uploadSurface)
           ) == 0
               ? 1
               : 0;
}

/**
 * Reimplements 0x4aa900: zVideo_dd3d::TextureRecord_ReleaseUploadSurfaceRef.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: releases and clears the upload-surface reference when one is held by
 * a texture record.
 *
 * Evidence: BN tests m_uploadSurface at offset zero, calls the provider Release
 * slot at vtable offset 8 when non-null, and stores null back to offset zero.
 */
void __fastcall TextureRecord_ReleaseUploadSurfaceRef(
    zVideo_TextureRecordPartial *textureRecord
) {
    if (textureRecord->m_uploadSurface != 0) {
        textureRecord->m_uploadSurface->Release();
        textureRecord->m_uploadSurface = 0;
    }
}

/**
 * Reimplements 0x4aa920: zVideo_dd3d::TextureRecord_FinalizeUpload.
 * Original file: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: optionally refreshes a texture-record upload surface from an image
 * and loads the temporary upload texture into the target Direct3D texture.
 *
 * Evidence: BN exits when m_uploadSurface is null, optionally calls
 * UploadImageToSurface with image->formatFlagsPacked bit 1, queries the upload
 * surface for IDirect3DTexture2, calls targetTexture->Load(uploadTexture), and
 * releases the temporary upload texture only when Load succeeds.
 */
void __fastcall TextureRecord_FinalizeUpload(
    zVideo_TextureRecordPartial *textureRecord,
    void *,
    zVidImagePartial *image
) {
    IDirectDrawSurface *uploadSurface = textureRecord->m_uploadSurface;
    if (uploadSurface == 0) {
        return;
    }

    IDirect3DTexture2 *targetTexture = textureRecord->m_texture;
    if (image != 0) {
        UploadImageToSurface(
            uploadSurface,
            image,
            image->formatFlagsPacked & 2
        );
    }

    IDirect3DTexture2 *uploadTexture = 0;
    HRESULT hresult =
        uploadSurface->QueryInterface(
            IID_IDirect3DTexture2,
            (void **)(&uploadTexture)
        );
    if (hresult != DD_OK) {
        return;
    }

    hresult = targetTexture->Load(uploadTexture);
    if (hresult == DD_OK) {
        uploadTexture->Release();
    }
}

/**
 * Reimplements 0x4aa980: zVideo_dd3d::TextureRecord_Destroy.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: release non-default Direct3D texture-record provider resources and
 * free the texture record.
 *
 * Evidence: BN hoists the upload surface, texture surface, and texture fields
 * before checking the default texture record, then releases each provider
 * reference in field order before freeing the record.
 */
void __fastcall TextureRecord_Destroy(
    zVideo_TextureRecordPartial *textureRecord
) {
    IDirectDrawSurface *uploadSurface = textureRecord->m_uploadSurface;
    IDirectDrawSurface *textureSurface = textureRecord->m_textureSurface;
    IDirect3DTexture2 *texture = textureRecord->m_texture;

    if (textureRecord == g_zVideo_DefaultTextureRecord) {
        return;
    }

    if (uploadSurface != 0) {
        uploadSurface->Release();
    }
    if (textureSurface != 0) {
        textureSurface->Release();
    }
    if (texture != 0) {
        texture->Release();
    }

    free(textureRecord);
}
} // namespace zVideo_dd3d

namespace zVideoD3D {

/**
 * Reimplements 0x4a74d0: zVideoD3D::SceneEnter.
 * Data evidence: BN reads g_zVideo_D3DSceneDepth at 0x632148, calls
 * zVideo_dd3d::BeginSceneAndFlushPendingRenderStates only when depth is not
 * positive, then increments the same zero-initialized int32.
 * Purpose: provide the recovered zVideoD3D::SceneEnter behavior.
 */
int SceneEnter() {
    if (g_zVideo_D3DSceneDepth <= 0) {
        zVideo_dd3d::BeginSceneAndFlushPendingRenderStates();
        ++g_zVideo_D3DSceneDepth;
    }

    return 0;
}

/**
 * Reimplements 0x4a74f0: zVideoD3D::SceneLeave.
 * Data evidence: BN reads g_zVideo_D3DSceneDepth at 0x632148, calls
 * zVideo_dd3d::EndScene only for the final active scene, decrements the stored
 * depth, and returns zero for all depth states.
 * Purpose: provide the recovered zVideoD3D::SceneLeave behavior.
 */
int SceneLeave() {
    int depth = g_zVideo_D3DSceneDepth;
    if (depth > 0) {
        if (depth <= 1) {
            zVideo_dd3d::EndScene();
            depth = g_zVideo_D3DSceneDepth;
        }
        g_zVideo_D3DSceneDepth = depth - 1;
    }

    return 0;
}

} // namespace zVideoD3D

namespace zVideo_dd {
/**
 * Reimplements 0x4a9900: zVideo_dd::GetAcceptedDirectDrawDeviceCountCached.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: return the cached number of accepted DirectDraw hardware API records.
 *
 * Evidence: BN emits a single load from g_zVideo_NumAcceptedDirectDrawDevices
 * at 0x632f98; EnumDirectDrawDeviceCallback is the only writer.
 */
int GetAcceptedDirectDrawDeviceCountCached() {
    return g_zVideo_NumAcceptedDirectDrawDevices;
}

namespace {
const int kPresentMissingSurfaceResult = 0x400;
const int kPresentFailureResult = 0x5a56ffff;
const int kPresentLinePageLock = 0x6c;
const int kPresentLinePageUnlock = 0x91;
const int kPresentLineBltOrRestore = 0xac;

template <typename InterfaceT>
/**
 * Original inline helper; no standalone retail function exists.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release a DirectDraw/Direct3D COM interface pointer and clear the
 * owning global or field.
 *
 * Original inline helper evidence: BN callers including 0x4a95e0 and 0x4a9300
 * emit the same null-check, provider Release call, and zero-store pattern for
 * temporary and subsystem-owned COM interfaces.
 */
void ReleaseComInterface(
    InterfaceT *&value
) {
    if (value != 0) {
        value->Release();
        value = 0;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered PageUnlockBeforeRelease helper behavior for zVideo callers.
 */
bool PageUnlockBeforeRelease(
    zVideo_SurfaceStatePartial &state,
    int reportLine
) {
    if (state.surf != 0 && state.pageLockActive != 0) {
        const HRESULT hresult = state.surf->PageUnlock(0);
        if (hresult != DD_OK) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                reportLine
            );
            return false;
        }

        state.pageLockActive = 0;
    }

    return true;
}

} // namespace

/**
 * Reimplements 0x4a93d0: zVideo_dd::EnumDirectDrawDeviceCallback.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: collect one DirectDraw device record and enumerate its usable
 * Direct3D drivers during startup.
 *
 * Evidence: BN indexes g_zVideo_HwApiDeviceTable by
 * g_zVideo_NumAcceptedDirectDrawDevices, copies the optional DirectDraw GUID
 * plus driver strings, temporarily selects the record, creates DirectDraw2,
 * gathers caps and video/texture memory, tags AGP-capable devices, calls
 * EnumerateDirect3DDevicesForRecord, increments the accepted DirectDraw count
 * only when D3D enumeration succeeds, tears down temporary interfaces, and
 * returns TRUE to continue enumeration except on capacity or caps failure.
 */
BOOL CALLBACK EnumDirectDrawDeviceCallback(
    GUID *guid,
    LPSTR driverDescription,
    LPSTR driverName,
    LPVOID context
) {
    (void)context;

    zVidHwApiDeviceRecordPartial *entry =
        &g_zVideo_HwApiDeviceTable[g_zVideo_NumAcceptedDirectDrawDevices];
    const int ordinal = g_zVideo_DirectDrawEnumOrdinal++;

    printf(
        g_zVideo_DDrawEnumDevicePrintfFmt,
        ordinal,
        driverName,
        driverDescription
    );
    fflush(stdout);

    if (g_zVideo_NumAcceptedDirectDrawDevices >= 4) {
        printf(g_zVideo_DDrawEnumTooManyDevicesMsg);
        return FALSE;
    }

    memset(
        entry,
        0,
        sizeof(*entry)
    );
    if (guid == 0) {
        entry->pDirectDrawGuid = 0;
    } else {
        entry->pDirectDrawGuid = &entry->m_directDrawGuidStorage;
        entry->m_directDrawGuidStorage = *guid;
    }

    strncpy(
        entry->m_driverName,
        driverName,
        sizeof(entry->m_driverName)
    );
    strncpy(
        entry->m_driverDescription,
        driverDescription,
        sizeof(entry->m_driverDescription)
    );
    g_zVideo_pSelectedHwApiDeviceRecord = entry;

    CreateDirectDraw2ForSelectedDevice();

    memset(
        &g_zVideo_DDrawCapsHal,
        0,
        sizeof(g_zVideo_DDrawCapsHal)
    );
    memset(
        &g_zVideo_DDrawCapsHel,
        0,
        sizeof(g_zVideo_DDrawCapsHel)
    );
    g_zVideo_DDrawCapsHal.dwSize = sizeof(g_zVideo_DDrawCapsHal);
    g_zVideo_DDrawCapsHel.dwSize = sizeof(g_zVideo_DDrawCapsHel);

    const HRESULT capsResult =
        g_zVideo_pDirectDraw2->GetCaps(
            &g_zVideo_DDrawCapsHal,
            &g_zVideo_DDrawCapsHel
        );
    if (capsResult != DD_OK) {
        ReportError(
            (int)(capsResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x739
        );
        return FALSE;
    }

    if ((g_zVideo_DDrawCapsHal.dwCaps & 0x200) != 0 ||
        (g_zVideo_DDrawCapsHel.dwCaps & 0x200) != 0) {
        entry->m_deviceFeatureFlags = 1;
        strcat(
            entry->m_driverName,
            g_zVideo_DDrawEnumAgpSuffix
        );
    }

    DDSCAPS memoryCaps;
    memoryCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
            &memoryCaps,
            (DWORD *)(&entry->m_videoMemTotalBytes),
            (DWORD *)(&entry->m_videoMemFreeBytes)
        ) != DD_OK) {
        entry->m_videoMemFreeBytes = 0;
        entry->m_videoMemTotalBytes = 0;
    }

    memoryCaps.dwCaps = DDSCAPS_TEXTURE;
    if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
            &memoryCaps,
            (DWORD *)(&entry->m_textureMemTotalBytes),
            (DWORD *)(&entry->m_textureMemFreeBytes)
        ) != DD_OK) {
        entry->m_textureMemFreeBytes = 0;
        entry->m_textureMemTotalBytes = 0;
    }

    if (EnumerateDirect3DDevicesForRecord(entry) != 0) {
        g_zVideo_NumAcceptedDirectDrawDevices += 1;
    }

    TeardownVideoSubsystem();
    return TRUE;
}

/**
 * Reimplements 0x4a96b0: zVideo_dd::EnumDirect3DDeviceCallback.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: filter Direct3D enumeration callbacks and append accepted hardware
 * RGB devices with 16-bit Z-buffer support to the current DirectDraw record.
 *
 * Evidence: BN indexes entry->m_d3dDrivers by m_acceptedD3DDeviceCount,
 * rejects non-hardware, non-RGB, and missing 16-bit Z-buffer devices, aborts
 * with teardown plus zError::ReportOld on capacity overflow, copies optional
 * GUID storage and the hardware D3D device descriptor, defaults zero max
 * texture dimensions to 0x100, stores device strings, and increments both
 * accepted-driver counters.
 */
HRESULT CALLBACK EnumDirect3DDeviceCallback(
    GUID *guid,
    LPSTR deviceDescription,
    LPSTR deviceName,
    D3DDEVICEDESC *hwDesc,
    D3DDEVICEDESC *,
    LPVOID context
) {
    zVidHwApiDeviceRecordPartial *entry = (zVidHwApiDeviceRecordPartial *)(context);
    zVidD3DDriverRecordPartial &driver = entry->m_d3dDrivers[entry->m_acceptedD3DDeviceCount];

    printf(
        g_zVideo_D3DEnumDriverPrintfFmt,
        deviceName,
        deviceDescription
    );
    fflush(stdout);

    const unsigned int descFlags = hwDesc->dwFlags;
    if (descFlags == 0) {
        printf(g_zVideo_D3DEnumSkipNoHardwareMsg);
        fflush(stdout);
        return 1;
    }

    if ((descFlags & D3DDD_COLORMODEL) != 0 && hwDesc->dcmColorModel != D3DCOLOR_RGB) {
        printf(g_zVideo_D3DEnumSkipNoRgbColorMsg);
        fflush(stdout);
        return 1;
    }

    if ((hwDesc->dwDeviceZBufferBitDepth & DDBD_16) == 0) {
        printf(g_zVideo_D3DEnumSkipNo16BitZBufferMsg);
        fflush(stdout);
        return 1;
    }

    if (entry->m_acceptedD3DDeviceCount >= 4) {
        TeardownVideoSubsystem();
        zError::ReportOld(
            0x800,
            g_zVideo_SourceFile_ZvidDdC,
            0x7d3,
            g_zVideo_D3DEnumTooManyDriversMsg
        );
        return 0;
    }

    if (guid == 0) {
        driver.pD3DDeviceGuid = 0;
    } else {
        driver.pD3DDeviceGuid = &driver.m_d3dDeviceGuidStorage;
        driver.m_d3dDeviceGuidStorage = *guid;
    }

    memcpy(
        &driver.m_hwDesc,
        hwDesc,
        sizeof(driver.m_hwDesc)
    );
    if (driver.m_hwDesc.dwMaxTextureWidth == 0) {
        driver.m_hwDesc.dwMaxTextureWidth = 0x100;
    }
    if (driver.m_hwDesc.dwMaxTextureHeight == 0) {
        driver.m_hwDesc.dwMaxTextureHeight = 0x100;
    }

    strncpy(
        driver.m_deviceName,
        deviceName,
        sizeof(driver.m_deviceName)
    );
    strncpy(
        driver.m_deviceDescription,
        deviceDescription,
        sizeof(driver.m_deviceDescription)
    );
    printf(g_zVideo_D3DEnumAcceptedMsg);
    fflush(stdout);
    entry->m_acceptedD3DDeviceCount += 1;
    g_zVid_AcceptedHardwareRendererCount += 1;
    return 1;
}

/**
 * Reimplements 0x4a6930: zVideo_dd::PrepareWindowForMode.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: switch the main window to fullscreen DirectDraw style and snapshot
 * the system palette when the desktop is palettized.
 *
 * Evidence: BN calls only Win32/GDI providers, writes no local tables, reads
 * g_zVideo_hWnd, and snapshots 256 PALETTEENTRY records into
 * g_zVideo_SystemPaletteEntries before returning zero.
 */
int PrepareWindowForMode() {
    SetMenu(
        g_zVideo_hWnd,
        0
    );
    SetWindowLongA(
        g_zVideo_hWnd,
        GWL_EXSTYLE,
        WS_EX_APPWINDOW
    );
    SetWindowLongA(
        g_zVideo_hWnd,
        GWL_STYLE,
        (LONG)(0x82000000u)
    );
    UpdateWindow(g_zVideo_hWnd);
    SetFocus(g_zVideo_hWnd);

    if (g_zVideo_hWnd != 0) {
        HDC screenDc = GetDC(0);
        if ((GetDeviceCaps(
            screenDc,
            RASTERCAPS
        ) & RC_PALETTE) != 0) {
            GetSystemPaletteEntries(
                screenDc,
                0,
                0x100,
                g_zVideo_SystemPaletteEntries
            );
        }
        ReleaseDC(
            0,
            screenDc
        );
    }

    return 0;
}

/**
 * Reimplements 0x4a7d20: zVideo_dd::OpenVideoMode.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: run the fullscreen window preparation step and create the selected
 * DirectDraw2 device, returning one on failure and zero on success.
 */
int __fastcall OpenVideoMode(
    int
) {
    if (PrepareWindowForMode() != 0) {
        return 1;
    }

    if (CreateDirectDraw2ForSelectedDevice() != 0) {
        return 1;
    }
    return 0;
}

/**
 * Reimplements 0x4a9390: zVideo_dd::RunDirectDrawDeviceEnumeration.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: run DirectDraw device enumeration during video startup.
 *
 * Evidence: BN prints the enumeration banner, calls the DirectDrawEnumerateA
 * provider import with EnumDirectDrawDeviceCallback and a null context, returns
 * one on DD_OK, and routes nonzero HRESULTs through ReportError at source line
 * 0x6ad before returning zero.
 */
int RunDirectDrawDeviceEnumeration() {
    printf(g_zVideo_DDrawEnumBeginMsg);
    const HRESULT hresult = DirectDrawEnumerateA(
        EnumDirectDrawDeviceCallback,
        0
    );
    if (hresult == DD_OK) {
        return 1;
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x6ad
    );
    return 0;
}

/**
 * Reimplements 0x4a8800: zVideo_dd::CreateDirectDraw2ForSelectedDevice.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create the selected DirectDraw device, query its IDirectDraw2
 * interface, and cache that interface for the active video backend.
 *
 * Evidence: BN reads g_zVideo_pSelectedHwApiDeviceRecord->pDirectDrawGuid,
 * calls the DirectDrawCreate provider import, queries IID_IDirectDraw2 into
 * g_zVideo_pDirectDraw2, releases the temporary IDirectDraw on success, and
 * routes the two HRESULT failures through ReportError.
 */
int CreateDirectDraw2ForSelectedDevice() {
    IDirectDraw *directDraw1;
    const HRESULT createResult =
        DirectDrawCreate(
            g_zVideo_pSelectedHwApiDeviceRecord->pDirectDrawGuid,
            &directDraw1,
            0
        );
    if (createResult != DD_OK) {
        return ReportError(
            (int)(createResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x3c4
        );
    }

    const HRESULT queryResult =
        directDraw1->QueryInterface(
            IID_IDirectDraw2,
            (void **)(&g_zVideo_pDirectDraw2)
        );
    if (queryResult != DD_OK) {
        return ReportError(
            (int)(queryResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x3cb
        );
    }

    directDraw1->Release();
    return 0;
}

/**
 * Reimplements 0x4a95e0: zVideo_dd::EnumerateDirect3DDevicesForRecord.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: query IDirect3D2 from the active DirectDraw2 object and enumerate
 * usable Direct3D drivers for one DirectDraw device record.
 *
 * Evidence: BN preserves the original 0x68-byte stack zeroing, prints the
 * selected DirectDraw record name, queries IID_IDirect3D2 into g_zVideo_pD3D2,
 * resets m_acceptedD3DDeviceCount, runs EnumDevices with
 * EnumDirect3DDeviceCallback, releases g_zVideo_pD3D2, and returns one only
 * when the callback accepted at least one driver.
 */
int __fastcall EnumerateDirect3DDevicesForRecord(
    zVidHwApiDeviceRecordPartial *entry
) {
    unsigned int unusedStackScratch[0x1b];
    memset(
        &unusedStackScratch[1],
        0,
        0x68
    );

    printf(
        g_zVideo_D3DEnumBeginMsgFmt,
        entry->m_driverName
    );
    fflush(stdout);

    const HRESULT queryResult =
        g_zVideo_pDirectDraw2->QueryInterface(
            IID_IDirect3D2,
            (void **)(&g_zVideo_pD3D2)
        );
    if (queryResult != DD_OK) {
        ReportError(
            (int)(queryResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x781
        );
        return 0;
    }

    entry->m_acceptedD3DDeviceCount = 0;
    g_zVideo_pD3D2->EnumDevices(
        EnumDirect3DDeviceCallback,
        entry
    );
    if (g_zVideo_pD3D2 != 0) {
        g_zVideo_pD3D2->Release();
        g_zVideo_pD3D2 = 0;
    }

    if (entry->m_acceptedD3DDeviceCount == 0) {
        printf(g_zVideo_D3DEnumNoUsableDriversMsg);
        return 0;
    }

    return 1;
}

/**
 * Reimplements 0x4a7b40: zVideo_dd::StartupEnumerateAndDefaultSelect.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: enumerate DirectDraw devices and select the first hardware device
 * record as the default startup renderer.
 *
 * Evidence: BN calls RunDirectDrawDeviceEnumeration, stores
 * &g_zVideo_HwApiDeviceTable[0] in g_zVideo_pSelectedHwApiDeviceRecord, clears
 * g_zVideo_pSelectedD3DDeviceInfo, and VC5SP3 byte verification has zero
 * unmasked mismatches for this body.
 */
void StartupEnumerateAndDefaultSelect() {
    RunDirectDrawDeviceEnumeration();
    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[0];
    g_zVideo_pSelectedD3DDeviceInfo = 0;
}

/**
 * Reimplements 0x4a7d40: zVideo_dd::ShutdownVideoSystem.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: clear the default texture record and tear down the DirectDraw
 * backend state.
 *
 * Evidence: BN clears g_zVideo_DefaultTextureRecord after destroying the
 * default record when present, calls TeardownVideoSubsystem unconditionally,
 * and returns zero.
 */
int ShutdownVideoSystem() {
    if (g_zVideo_DefaultTextureRecord != 0) {
        zVideo_dd3d::TextureRecord_Destroy(g_zVideo_DefaultTextureRecord);
        g_zVideo_DefaultTextureRecord = 0;
    }

    TeardownVideoSubsystem();
    return 0;
}

/**
 * Reimplements 0x4a8060: zVideo_dd::LockDirectDrawSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: lock a DirectDraw surface descriptor, restoring and retrying when
 * the provider reports a lost surface.
 *
 * Evidence: BN zeroes and sizes the DDSURFACEDESC to 0x6c bytes, calls
 * IDirectDrawSurface3::Lock with null rect, DDLOCK_WAIT, and null event, loops
 * through Restore on DDERR_SURFACELOST, reports line 0x1b9 on unrecovered
 * provider errors, and returns 0x5a56ffff on failure.
 */
int __fastcall LockDirectDrawSurface(
    IDirectDrawSurface3 *surface,
    DDSURFACEDESC *outLockedSurfaceDesc
) {
    memset(
        outLockedSurfaceDesc,
        0,
        sizeof(*outLockedSurfaceDesc)
    );
    outLockedSurfaceDesc->dwSize = sizeof(*outLockedSurfaceDesc);

    for (;;) {
        HRESULT hresult = surface->Lock(
            0,
            outLockedSurfaceDesc,
            DDLOCK_WAIT,
            0
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x1b9
        );
        return 0x5a56ffff;
    }
}

/**
 * Reimplements 0x4a80c0: zVideo_dd::UnlockDirectDrawSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: unlock a DirectDraw surface, restoring and retrying when the
 * provider reports a lost surface.
 *
 * Evidence: BN calls IDirectDrawSurface3::Unlock with a null surface pointer,
 * loops through Restore on DDERR_SURFACELOST, reports line 0x1d7 on
 * unrecovered provider errors, returns zero on success, and returns
 * 0x5a56ffff on failure.
 */
int __fastcall UnlockDirectDrawSurface(
    IDirectDrawSurface3 *surface
) {
    for (;;) {
        HRESULT hresult = surface->Unlock(0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x1d7
        );
        return 0x5a56ffff;
    }
}

/**
 * Reimplements 0x4a8100: zVideo_dd::LockSurface_WaitRestore.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: locks a DirectDraw surface with DDLOCK_WAIT, retrying once the
 * provider restores a lost surface and reporting permanent failures.
 */
int __fastcall LockSurface_WaitRestore(
    IDirectDrawSurface3 *surface,
    DDSURFACEDESC *lockedDescOut
) {
    memset(
        lockedDescOut,
        0,
        sizeof(*lockedDescOut)
    );
    lockedDescOut->dwSize = sizeof(*lockedDescOut);

    for (;;) {
        HRESULT hresult = surface->Lock(
            0,
            lockedDescOut,
            DDLOCK_WAIT,
            0
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x1fd
        );
        return 0x5a56ffff;
    }
}

/**
 * Reimplements 0x4a8160: zVideo_dd::UnlockSurface_WaitRestore.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: unlocks a DirectDraw surface, retrying once the provider restores a
 * lost surface and reporting permanent failures.
 */
int __fastcall UnlockSurface_WaitRestore(
    IDirectDrawSurface3 *surface
) {
    for (;;) {
        HRESULT hresult = surface->Unlock(0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x21b
        );
        return 0x5a56ffff;
    }
}

/**
 * Reimplements 0x4a7fc0: zVideo_dd::LockSurfaceState.
 * Purpose: lock a tracked DirectDraw surface state and cache the surface
 * descriptor fields used by software rendering paths.
 */
int __fastcall LockSurfaceState(
    zVideo_SurfaceStatePartial *surfaceState
) {
    if (g_zVideo_FullscreenOption == 0 &&
        surfaceState == &g_zVideo_DisplayModeSurfaceState) {
        return 0;
    }

    if (surfaceState->locked != 0) {
        return 0;
    }

    DDSURFACEDESC lockedSurfaceDesc;
    const int result = LockDirectDrawSurface(
        surfaceState->surf,
        &lockedSurfaceDesc
    );
    if (result == 0) {
        surfaceState->locked = 1;
        surfaceState->width = (int)(lockedSurfaceDesc.dwWidth);
        surfaceState->height = (int)(lockedSurfaceDesc.dwHeight);
        surfaceState->lockInfoValid = 1;
        surfaceState->pixels = lockedSurfaceDesc.lpSurface;
        surfaceState->pitch = (int)(lockedSurfaceDesc.lPitch);
    }

    return result;
}

/**
 * Reimplements 0x4a8030: zVideo_dd::UnlockSurfaceState.
 * Purpose: unlock a tracked DirectDraw surface state when the runtime policy
 * and lock flag require it.
 */
int __fastcall UnlockSurfaceState(
    zVideo_SurfaceStatePartial *surfaceState
) {
    if (g_zVideo_FullscreenOption == 0 &&
        surfaceState == &g_zVideo_DisplayModeSurfaceState) {
        return 0;
    }

    if (surfaceState->locked == 0) {
        return 0;
    }

    surfaceState->locked = 0;
    return UnlockDirectDrawSurface(surfaceState->surf);
}

/**
 * Reimplements 0x4a83d0: zVideo_dd::Image_LazyCreateBackingSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: lazily create a DirectDrawSurface3 backing store for a heap-backed
 * image and populate it from the image pixels.
 *
 * Evidence: BN guards alpha maps, null pixels, and zero width/height, creates
 * an offscreen DirectDraw surface from the requested caps, queries
 * IID_IDirectDrawSurface3, stores image->surface only after QueryInterface
 * succeeds, calls Image_PopulateSurfaceFromHeapPixels, and reports line 0x2ed
 * on provider failure.
 */
IDirectDrawSurface3 *__fastcall Image_LazyCreateBackingSurface(
    zVidImagePartial *image,
    unsigned int ddsCapsFlags
) {
    if (image->alphaMap != 0 || image->pixels == 0 || image->height == 0 || image->width == 0) {
        return 0;
    }

    IDirectDrawSurface *baseSurface = 0;
    IDirectDrawSurface3 *surface3 = 0;
    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x10007;
    desc.dwHeight = (DWORD)(image->height);
    desc.dwWidth = (DWORD)(image->width);
    desc.ddsCaps.dwCaps = ddsCapsFlags | DDSCAPS_OFFSCREENPLAIN;
    image->surface = 0;

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        &desc,
        &baseSurface,
        0
    );
    if (hresult == DD_OK) {
        hresult = baseSurface->QueryInterface(
            IID_IDirectDrawSurface3,
            (void **)(&surface3)
        );
        if (hresult == DD_OK) {
            image->surface = surface3;
            Image_PopulateSurfaceFromHeapPixels(image);
            return image->surface;
        }
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x2ed
    );
    return image->surface;
}

/**
 * Reimplements 0x4a8500: zVideo_dd::Image_PopulateSurfaceFromHeapPixels.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: copy an image heap pixel buffer into its locked DirectDraw surface
 * and rebind the image pixels to the surface memory.
 *
 * Evidence: BN locks image->surface with DDLOCK_WAIT, retries after
 * DDERR_SURFACELOST restores, copies width * 2 bytes per row, frees the heap
 * buffer, stores the locked surface pointer and half-pitch, unlocks with the
 * same lost-surface retry pattern, and reports lines 0x31b, 0x31f, 0x33b, and
 * 0x33f on provider failures.
 */
int __fastcall Image_PopulateSurfaceFromHeapPixels(
    zVidImagePartial *image
) {
    DDSURFACEDESC lockedSurfaceDesc = {0};
    lockedSurfaceDesc.dwSize = sizeof(lockedSurfaceDesc);
    HRESULT hresult;

retryLock:
    hresult = image->surface->Lock(
        0,
        &lockedSurfaceDesc,
        DDLOCK_WAIT,
        0
    );
    if (hresult != DD_OK) {
        if (hresult != DDERR_SURFACELOST) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x31f
            );
            return 0;
        }

        hresult = image->surface->Restore();
        if (hresult != DD_OK) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x31b
            );
        }
        goto retryLock;
    }

    const int rowBytes = (int)(image->width) << 1;
    unsigned char *srcPixels = (unsigned char *)(image->pixels);
    unsigned char *dstPixels = (unsigned char *)(lockedSurfaceDesc.lpSurface);
    {
        for (int row = 0; row < image->height; ++row) {
            memcpy(
                dstPixels,
                srcPixels,
                rowBytes
            );
            dstPixels += lockedSurfaceDesc.lPitch;
            srcPixels += rowBytes;
        }
    }

    free(image->pixels);
    image->pixels = lockedSurfaceDesc.lpSurface;
    image->pitchWords = (int)((unsigned int)(lockedSurfaceDesc.lPitch) >> 1);

retryUnlock:
    hresult = image->surface->Unlock(&lockedSurfaceDesc);
    if (hresult == DD_OK) {
        return 1;
    }

    if (hresult == DDERR_SURFACELOST) {
        hresult = image->surface->Restore();
        if (hresult != DD_OK) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x33b
            );
        }
        goto retryUnlock;
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x33f
    );
    return 0;
}

/**
 * Reimplements 0x4a84c0: zVideo_dd::Image_LazyCreateVideoMemorySurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create an image video-memory backing surface when the current
 * renderer/device state requires one.
 *
 * Evidence: BN reads g_zVideo_UseHalfResBackbuffer and
 * g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags, returns null
 * when neither requests video memory, then tail-calls
 * Image_LazyCreateBackingSurface with DDSCAPS_VIDEOMEMORY plus optional
 * DDSCAPS_NONLOCALVIDMEM.
 */
IDirectDrawSurface3 *__fastcall Image_LazyCreateVideoMemorySurface(
    zVidImagePartial *image
) {
    if (g_zVideo_UseHalfResBackbuffer == 0 &&
        g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags == 0) {
        return 0;
    }

    const unsigned int caps =
        (g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0
            ? DDSCAPS_NONLOCALVIDMEM
            : 0) + DDSCAPS_VIDEOMEMORY;
    return Image_LazyCreateBackingSurface(
        image,
        caps
    );
}

/**
 * Reimplements 0x4a8650: zVideo_dd::Image_EnsureSurfaceForCurrentDevice.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release and clear an image-owned DirectDraw surface so it can be
 * recreated for the current video device.
 *
 * Evidence: BN releases image->surface only when g_zVideo_IsInitialized is
 * nonzero and the surface is present, then clears image->surface and
 * image->pixels whenever a stale surface pointer remains.
 */
void __fastcall Image_EnsureSurfaceForCurrentDevice(
    zVidImagePartial *image
) {
    if (g_zVideo_IsInitialized != 0 && image->surface != 0) {
        image->surface->Release();
    }

    if (image->surface != 0) {
        image->surface = 0;
        image->pixels = 0;
    }
}

/**
 * Reimplements 0x4a8680: zVideo_dd::Image_UploadPixelsToSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: ensure an image has a DirectDraw surface and acquire a GDI DC for
 * drawing into it.
 *
 * Evidence: BN returns zero for renderer type 2, lazily creates a backing
 * surface from selected-device feature flags when image->surface is null,
 * calls IDirectDrawSurface3::GetDC, returns one on DD_OK, and reports line
 * 0x36d on provider failure.
 */
int __fastcall Image_UploadPixelsToSurface(
    zVidImagePartial *image,
    HDC *outHdc
) {
    if (g_zVideo_RendererType == 2) {
        return 0;
    }

    if (image->surface == 0) {
        // Original upload path assumes device selection is complete before lazy creation.
        unsigned int caps = DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
        if (g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags == 0) {
            caps = DDSCAPS_SYSTEMMEMORY;
        }

        IDirectDrawSurface3 *surface = Image_LazyCreateBackingSurface(
            image,
            caps
        );
        if (surface == 0) {
            return (int)(surface);
        }
    }

    const HRESULT hresult = image->surface->GetDC(outHdc);
    if (hresult == DD_OK) {
        return 1;
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x36d
    );
    return 0;
}

/**
 * Reimplements 0x4a86f0: zVideo_dd::Image_ReleaseSurface.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release a GDI DC acquired from an image-backed DirectDraw surface.
 *
 * Evidence: BN returns zero when image->surface is null, calls
 * IDirectDrawSurface3::ReleaseDC with the supplied HDC, returns one on DD_OK,
 * and reports line 0x382 on provider failure.
 */
int __fastcall Image_ReleaseSurface(
    zVidImagePartial *image,
    HDC hdc
) {
    IDirectDrawSurface3 *surface = image->surface;
    if (surface == 0) {
        return (int)(surface);
    }

    const HRESULT hresult = surface->ReleaseDC(hdc);
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x382
        );
        return 0;
    }

    return 1;
}

/**
 * Reimplements 0x4a7d90: zVideo_dd::BltSwToPrimaryRectDirect.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: copy a source rectangle from the software surface directly to the
 * primary DirectDraw surface.
 *
 * Evidence: BN uses fastcall ECX=srcRect and EDX=dstRect, calls
 * IDirectDrawSurface3::Blt on g_zVideo_PrimarySurfaceState.surf with dstRect,
 * g_zVideo_SwSurfaceState.surf, srcRect, DDBLT_WAIT, and null DDBLTFX, then
 * reports line 0xe9 on nonzero HRESULT.
 */
void __fastcall BltSwToPrimaryRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->Blt(
        (RECT *)(dstRect),
        g_zVideo_SwSurfaceState.surf,
        (RECT *)(srcRect),
        DDBLT_WAIT,
        0
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0xe9
        );
    }
}

/**
 * Reimplements 0x4a7dd0: zVideo_dd::BltPrimaryToSwRectDirect.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: copy a source rectangle from the primary surface directly back to
 * the software DirectDraw surface.
 *
 * Evidence: BN uses fastcall ECX=srcRect and EDX=dstRect, calls
 * IDirectDrawSurface3::Blt on g_zVideo_SwSurfaceState.surf with dstRect,
 * g_zVideo_PrimarySurfaceState.surf, srcRect, DDBLT_WAIT, and null DDBLTFX,
 * then reports line 0xfc on nonzero HRESULT. BN's raw name shows zVideoDD,
 * while the source/plan owner uses zVideo_dd.
 */
void __fastcall BltPrimaryToSwRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    const HRESULT hresult = g_zVideo_SwSurfaceState.surf->Blt(
        (RECT *)(dstRect),
        g_zVideo_PrimarySurfaceState.surf,
        (RECT *)(srcRect),
        DDBLT_WAIT,
        0
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0xfc
        );
    }
}

/**
 * Reimplements 0x4a7b60: zVideo_dd::PresentDisplayModeSurface.
 * Purpose: Present the software/display-mode surface through DirectDraw.
 *
 * Evidence: BN source file zvid_dd.c page-locks the primary surface for the
 * fullscreen software adjustment path, copies the 0x20-byte primary and
 * software surface records through g_zVideo_SurfaceStateSwapScratch when the
 * swap is enabled, unlocks the active primary record when it remains locked,
 * and retries a failed Blt after DDERR_SURFACELOST Restore succeeds.
 */
int __fastcall PresentDisplayModeSurface(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int skipSurfaceStateSwap
) {
    DWORD presentBltFlags =
        DDBLT_WAIT + (waitForPresent != 0 ? 0 : DDBLT_ASYNC);

    if (g_zVideo_DisplayModeSurfaceState.surf == 0 ||
        g_zVideo_PrimarySurfaceState.surf == 0) {
        return kPresentMissingSurfaceResult;
    }

    HRESULT hresult;

    for (;;) {
        if (g_zVideo_UseHalfResBackbuffer != 0 || g_zVideo_HalfResAdjustMode == 0) {
            hresult = g_zVideo_DisplayModeSurfaceState.surf->Blt(
                (RECT *)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT *)(srcRect),
                presentBltFlags,
                0
            );
        } else {
            hresult = g_zVideo_PrimarySurfaceState.surf->PageLock(0);
            if (hresult != DD_OK) {
                ReportError(
                    (int)(hresult),
                    g_zVideo_SourceFile_ZvidDdC,
                    kPresentLinePageLock
                );
                return 0;
            }

            hresult = g_zVideo_DisplayModeSurfaceState.surf->Blt(
                (RECT *)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT *)(srcRect),
                DDBLT_ASYNC,
                0
            );
            g_zVideo_PrimarySurfaceState.pageLockActive = 1;

            if (skipSurfaceStateSwap == 0) {
                memcpy(
                    &g_zVideo_SurfaceStateSwapScratch,
                    &g_zVideo_PrimarySurfaceState,
                    sizeof(g_zVideo_SurfaceStateSwapScratch)
                );
                memcpy(
                    &g_zVideo_PrimarySurfaceState,
                    &g_zVideo_SwSurfaceState,
                    sizeof(g_zVideo_PrimarySurfaceState)
                );
                memcpy(
                    &g_zVideo_SwSurfaceState,
                    &g_zVideo_SurfaceStateSwapScratch,
                    sizeof(g_zVideo_SwSurfaceState)
                );
            }

            if (g_zVideo_PrimarySurfaceState.pageLockActive != 0) {
                const HRESULT pageUnlockResult =
                    g_zVideo_PrimarySurfaceState.surf->PageUnlock(0);
                if (pageUnlockResult != DD_OK) {
                    ReportError(
                        (int)(pageUnlockResult),
                        g_zVideo_SourceFile_ZvidDdC,
                        kPresentLinePageUnlock
                    );
                    return 0;
                }

                g_zVideo_PrimarySurfaceState.pageLockActive = 0;
            }
        }

        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_DisplayModeSurfaceState.surf->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            kPresentLineBltOrRestore
        );
        return kPresentFailureResult;
    }
}

/**
 * Reimplements 0x4a7e10: zVideo_dd::BltSwToPrimaryRect.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: blit an image-backed software surface to the primary surface with
 * default rectangles, primary clipping, optional source color key, and primary
 * lock preservation.
 *
 * Evidence: BN lazily creates srcImage->surface with caps 0x20004000 when the
 * selected hardware device has feature flags, otherwise DDSCAPS_SYSTEMMEMORY;
 * builds default source and destination rectangles, clips destination against
 * g_zVideo_PrimarySurfaceState width/height while mirroring offsets into the
 * source rect, unlocks/relocks the primary state if it was locked, calls
 * IDirectDrawSurface3::Blt with DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE plus
 * optional DDBLT_KEYSRC, and reports line 0x159 on Blt failure.
 */
void __fastcall BltSwToPrimaryRect(
    zVidImagePartial *srcImage,
    int srcColorKeyEnable,
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    if (srcImage->surface == 0) {
        const unsigned int caps =
            g_zVideo_pSelectedHwApiDeviceRecord != 0 &&
                    g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0
                ? 0x20004000
                : DDSCAPS_SYSTEMMEMORY;
        if (Image_LazyCreateBackingSurface(
            srcImage,
            caps
        ) == 0) {
            return;
        }
    }

    zVidRect32 srcRectLocal;
    if (srcRect != 0) {
        srcRectLocal = *srcRect;
    } else {
        srcRectLocal.left = 0;
        srcRectLocal.top = 0;
        srcRectLocal.right = srcImage->width;
        srcRectLocal.bottom = srcImage->height;
    }

    zVidRect32 dstRectLocal;
    if (dstRect != 0) {
        dstRectLocal = *dstRect;
    } else {
        dstRectLocal.left = 0;
        dstRectLocal.top = 0;
        dstRectLocal.right = srcRectLocal.right - srcRectLocal.left;
        dstRectLocal.bottom = srcRectLocal.bottom - srcRectLocal.top;
    }

    int clipped = zVideo_buff::ClipCoordToRange(
        &dstRectLocal.left,
        0,
        g_zVideo_PrimarySurfaceState.width - 1
    );
    if (clipped < 0) {
        srcRectLocal.left -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped =
        zVideo_buff::ClipCoordToRange(
            &dstRectLocal.right,
            0,
            g_zVideo_PrimarySurfaceState.width
        );
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.right -= clipped;
    }

    clipped = zVideo_buff::ClipCoordToRange(
        &dstRectLocal.top,
        0,
        g_zVideo_PrimarySurfaceState.height - 1
    );
    if (clipped < 0) {
        srcRectLocal.top -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped =
        zVideo_buff::ClipCoordToRange(
            &dstRectLocal.bottom,
            0,
            g_zVideo_PrimarySurfaceState.height
        );
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.bottom -= clipped;
    }

    const int wasLocked = g_zVideo_PrimarySurfaceState.locked;
    if (wasLocked != 0) {
        UnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    const DWORD bltFlags =
        DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE | (srcColorKeyEnable != 0 ? DDBLT_KEYSRC : 0);
    const HRESULT hresult =
        g_zVideo_PrimarySurfaceState.surf
            ->Blt(
                (RECT *)(&dstRectLocal),
                srcImage->surface,
                (RECT *)(&srcRectLocal),
                bltFlags,
                0
            );

    if (wasLocked != 0) {
        LockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x159
        );
    }
}

/**
 * Reimplements 0x4a81a0: zVideo_dd::ZBuffer_DepthFillRect.
 * Purpose: clear the current DirectDraw Z-buffer rectangle to depth zero.
 *
 * Evidence: BN source file zvid_dd.c tests g_zVideo_pZBufferSurface, builds a
 * DDBLTFX with dwSize 0x64 and dwFillDepth zero, calls DirectDrawSurface3::Blt
 * with DDBLT_DEPTHFILL, and reports line 0x242 after a failed Restore retry.
 */
int __fastcall ZBuffer_DepthFillRect(
    zVidRect32 *dstRect
) {
    // BN writes only the DirectDraw fields consumed by the selected fill mode.
    DDBLTFX bltFx;
    HRESULT hresult;
    bltFx.dwSize = sizeof(bltFx);
    if (g_zVideo_pZBufferSurface == 0) {
        goto done;
    }

    bltFx.dwFillDepth = 0;
    hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = g_zVideo_pZBufferSurface->Blt(
            (RECT *)(dstRect),
            0,
            0,
            DDBLT_DEPTHFILL,
            &bltFx
        );
        if (hresult == DD_OK) {
            goto done;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_pZBufferSurface->Restore();
        }
    }

    return ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x242
    );

done:
    return 0;
}

/**
 * Reimplements 0x4a8220: zVideo_dd::ClearScreenAndZBufferRect.
 * Purpose: clear a color surface rectangle and then the matching Z-buffer.
 *
 * Evidence: BN source file zvid_dd.c gates the color fill with
 * g_zVideo_ClearScreenBufferEnabled, uses g_zVideo_ClearColorPacked16 for
 * DDBLT_COLORFILL|DDBLT_WAIT, then optionally clears g_zVideo_pZBufferSurface
 * with DDBLT_DEPTHFILL and report lines 0x267 and 0x27f.
 */
int __fastcall ClearScreenAndZBufferRect(
    zVidRect32 *dstRect,
    zVideo_SurfaceStatePartial *colorSurfaceState
) {
    // BN writes only the DirectDraw fields consumed by the selected fill mode.
    DDBLTFX bltFx;
    bltFx.dwSize = sizeof(bltFx);

    if (g_zVideo_ClearScreenBufferEnabled != 0) {
        bltFx.dwFillColor = g_zVideo_ClearColorPacked16;
        HRESULT hresult = DD_OK;
        while (hresult == DD_OK) {
            hresult = colorSurfaceState->surf->Blt(
                (RECT *)(dstRect),
                0,
                0,
                DDBLT_COLORFILL | DDBLT_WAIT,
                &bltFx
            );
            if (hresult == DD_OK) {
                goto clearZBuffer;
            }

            if (hresult == DDERR_SURFACELOST) {
                hresult = colorSurfaceState->surf->Restore();
            }
        }
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x267
        );
    }

clearZBuffer:
    if (g_zVideo_pZBufferSurface == 0) {
        goto done;
    }

    bltFx.dwFillDepth = 0;
    HRESULT hresult;
    hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = g_zVideo_pZBufferSurface->Blt(
            (RECT *)(dstRect),
            0,
            0,
            DDBLT_DEPTHFILL,
            &bltFx
        );
        if (hresult == DD_OK) {
            goto done;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_pZBufferSurface->Restore();
        }
    }

    return ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x27f
    );

done:
    return 0;
}

/**
 * Reimplements 0x4a82f0: zVideo_dd::ClearSwBackbufferAndZBufferRects.
 * Purpose: clear the software backbuffer rectangle and a separate Z rectangle.
 *
 * Evidence: BN source file zvid_dd.c fills g_zVideo_SwSurfaceState.surf when
 * screen-buffer clearing is enabled, then optionally clears
 * g_zVideo_pZBufferSurface with the supplied Z rectangle and reports lines
 * 0x2a5 and 0x2bd on permanent provider failures.
 */
int __fastcall ClearSwBackbufferAndZBufferRects(
    zVidRect32 *colorRect,
    zVidRect32 *zRect
) {
    // BN writes only the DirectDraw fields consumed by the selected fill mode.
    DDBLTFX bltFx;
    bltFx.dwSize = sizeof(bltFx);

    if (g_zVideo_ClearScreenBufferEnabled != 0) {
        bltFx.dwFillColor = g_zVideo_ClearColorPacked16;
        HRESULT hresult = DD_OK;
        while (hresult == DD_OK) {
            hresult = g_zVideo_SwSurfaceState.surf->Blt(
                (RECT *)(colorRect),
                0,
                0,
                DDBLT_COLORFILL | DDBLT_WAIT,
                &bltFx
            );
            if (hresult == DD_OK) {
                goto clearZBuffer;
            }

            if (hresult == DDERR_SURFACELOST) {
                hresult = g_zVideo_SwSurfaceState.surf->Restore();
            }
        }
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x2a5
        );
    }

clearZBuffer:
    if (g_zVideo_pZBufferSurface == 0) {
        goto done;
    }

    bltFx.dwFillDepth = 0;
    HRESULT hresult;
    hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = g_zVideo_pZBufferSurface->Blt(
            (RECT *)(zRect),
            0,
            0,
            DDBLT_DEPTHFILL,
            &bltFx
        );
        if (hresult == DD_OK) {
            goto done;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_pZBufferSurface->Restore();
        }
    }

    return ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x2bd
    );

done:
    return 0;
}

/**
 * Reimplements 0x4a7d70: zVideo_dd::FlipToGDIIfAttached.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: flip the attached primary DirectDraw surface back to GDI when the
 * fullscreen primary owns an attached backbuffer.
 */
void FlipToGDIIfAttached() {
    if (g_zVideo_pDirectDraw2 != 0 && g_zVideo_PrimaryHasAttachedBackbuffer != 0) {
        g_zVideo_pDirectDraw2->FlipToGDISurface();
    }
}

/**
 * Reimplements 0x4a8720: zVideo_dd::SetDisplayMode.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: enter exclusive fullscreen cooperative mode and apply the current
 * DirectDraw display mode.
 *
 * Evidence: BN calls IDirectDraw2::SetCooperativeLevel with g_zVideo_hWnd and
 * flags 0x13, reports source line 0x393 on failure, then calls SetDisplayMode
 * with the display surface width/height, BPP, zero refresh, and zero flags,
 * reporting line 0x39c on failure.
 */
int SetDisplayMode() {
    HRESULT hresult = g_zVideo_pDirectDraw2->SetCooperativeLevel(
        g_zVideo_hWnd,
        0x13
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x393
        );
        return 0;
    }

    hresult = g_zVideo_pDirectDraw2->SetDisplayMode(
        g_zVideo_DisplayModeSurfaceState.width,
        g_zVideo_DisplayModeSurfaceState.height,
        g_zVideo_DisplayModeBpp,
        0,
        0
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x39c
        );
        return 0;
    }

    return 1;
}

/**
 * Reimplements 0x4a8790: zVideo_dd::SetVideoMode.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: rebuild fullscreen DirectDraw surfaces for the active renderer and
 * verify the restored display surfaces.
 *
 * Evidence: BN uses the mode-independent backend path after zVideo has already
 * applied geometry, gates each DirectDraw setup step on nonzero failure, tests
 * g_zVideo_RendererType for the hardware-only Direct3D device creation, and
 * normalizes the final surface-lock verification result to 0 or 1.
 */
int __fastcall SetVideoMode(
    int
) {
    if (SetDisplayMode() == 0) {
        return 1;
    }

    if (RestoreDisplaySurfaces() != 0) {
        return 1;
    }

    if (ReleaseAllInterfacesAndSurfaces() != 0) {
        return 1;
    }

    if (CreateFullscreenSurfacesForRenderer() != 0) {
        return 1;
    }

    if (g_zVideo_RendererType == 1 && zVideo_dd3d::CreateDeviceState() != 0) {
        return 1;
    }

    if (RestoreDisplaySurfaces() != 0) {
        return 1;
    }

    if (VerifyFullscreenSurfaceLocks() != 0) {
        return 1;
    }
    return 0;
}

/**
 * Reimplements 0x4a9060: zVideo_dd::VerifyFullscreenSurfaceLocks.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: verify that the software, primary, and display-mode DirectDraw
 * surface states can each lock and unlock.
 *
 * Evidence: BN calls LockSurfaceState and UnlockSurfaceState for
 * g_zVideo_SwSurfaceState, g_zVideo_PrimarySurfaceState, and
 * g_zVideo_DisplayModeSurfaceState in that order, returning 1 after any
 * failed probe.
 */
int VerifyFullscreenSurfaceLocks() {
    if (LockSurfaceState(&g_zVideo_SwSurfaceState) != 0) {
        return 1;
    }
    if (UnlockSurfaceState(&g_zVideo_SwSurfaceState) != 0) {
        return 1;
    }
    if (LockSurfaceState(&g_zVideo_PrimarySurfaceState) != 0) {
        return 1;
    }
    if (UnlockSurfaceState(&g_zVideo_PrimarySurfaceState) != 0) {
        return 1;
    }
    if (LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0) {
        return 1;
    }

    return UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ? 1 : 0;
}

/**
 * Reimplements 0x4a90e0: zVideo_dd::RestoreDisplaySurfaces.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: restore the display-mode, primary, and software DirectDraw
 * surfaces when they are present.
 *
 * Evidence: BN restores g_zVideo_DisplayModeSurfaceState.surf, then
 * g_zVideo_PrimarySurfaceState.surf, then g_zVideo_SwSurfaceState.surf, and
 * reports DirectDraw failures at source lines 0x5e1, 0x5e8, and 0x5ef.
 */
int RestoreDisplaySurfaces() {
    if (g_zVideo_DisplayModeSurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_DisplayModeSurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x5e1
            );
        }
    }

    if (g_zVideo_PrimarySurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x5e8
            );
        }
    }

    if (g_zVideo_SwSurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_SwSurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x5ef
            );
        }
    }

    return 0;
}

/**
 * Reimplements 0x4a8f80: zVideo_dd::InitFullscreenSoftwarePixelPack.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: read the fullscreen display pixel format and initialize the
 * software pixel-pack masks for supported formats.
 *
 * Evidence: BN initializes DDPIXELFORMAT.dwSize to 0x20, calls the
 * IDirectDrawSurface3::GetPixelFormat provider slot, reports provider failures
 * at source line 0x597, accepts green masks 0x07e0, 0x03e0, and 0xff00, and
 * tears down plus reports line 0x5bd for unrecognized formats.
 */
int __fastcall InitFullscreenSoftwarePixelPack(
    IDirectDrawSurface3 *displaySurface
) {
    DDPIXELFORMAT pixelFormat = {0};
    pixelFormat.dwSize = sizeof(pixelFormat);

    const HRESULT hresult = displaySurface->GetPixelFormat(&pixelFormat);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x597
        );
    }

    if (pixelFormat.dwGBitMask == 0x07e0) {
        zVideo::PixelPack_SetupFromMasks(
            5,
            6,
            5,
            pixelFormat.dwRBitMask,
            pixelFormat.dwGBitMask,
            pixelFormat.dwBBitMask
        );
        return 0;
    }

    if (pixelFormat.dwGBitMask == 0x03e0) {
        zVideo::PixelPack_SetupFromMasks(
            5,
            5,
            5,
            pixelFormat.dwRBitMask,
            0x03e0,
            pixelFormat.dwBBitMask
        );
        return 0;
    }

    if (pixelFormat.dwGBitMask == 0xff00) {
        zVideo::PixelPack_SetupFromMasks(
            5,
            6,
            5,
            pixelFormat.dwRBitMask,
            pixelFormat.dwGBitMask,
            pixelFormat.dwBBitMask
        );
        return 0;
    }

    TeardownVideoSubsystem();
    zError::ReportOld(
        0x800,
        g_zVideo_SourceFile_ZvidDdC,
        0x5bd,
        g_zVideo_UnrecognizedPixelFormatMsg
    );
    return 0x5a56ffff;
}

/**
 * Reimplements 0x4a88b0: zVideo_dd::CreateSurface3FromDesc.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create a DirectDraw surface and return its DirectDrawSurface3
 * interface.
 *
 * Evidence: BN shows a DirectDraw2::CreateSurface call, a successful-surface
 * QueryInterface for IID_IDirectDrawSurface3, Release of the temporary base
 * surface only after successful QueryInterface, and direct propagation of the
 * current provider HRESULT. BN callers pass one unused zero stack argument and
 * the callee returns with ret 8.
 */
HRESULT __fastcall CreateSurface3FromDesc(
    IDirectDraw2 *directDraw,
    DDSURFACEDESC *desc,
    IDirectDrawSurface3 **outSurface,
    int reserved
) {
    IDirectDrawSurface *createdSurface;
    reserved;
    HRESULT result = directDraw->CreateSurface(
        desc,
        &createdSurface,
        0
    );
    if (result == DD_OK) {
        result = createdSurface->QueryInterface(
            IID_IDirectDrawSurface3,
            (void **)(outSurface)
        );
        if (result == DD_OK) {
            return createdSurface->Release();
        }
    }

    return result;
}

/**
 * Reimplements 0x4a88f0: zVideo_dd::CreateFullscreenSurfacesForRenderer.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: select the active fullscreen surface-creation path for the current
 * renderer and half-resolution setting.
 *
 * Evidence: BN tests g_zVideo_UseHalfResBackbuffer first, then dispatches by
 * g_zVideo_RendererType to the Direct3D hardware or software surface builders.
 */
int CreateFullscreenSurfacesForRenderer() {
    if (g_zVideo_UseHalfResBackbuffer != 0) {
        return CreateHalfResBackbufferSurfaces();
    }

    if (g_zVideo_RendererType == 1) {
        return CreateFullscreenHardwareSurfaces();
    }

    return CreateFullscreenSoftwareSurfaces();
}

/**
 * Reimplements 0x4a8920: zVideo_dd::CreateHalfResBackbufferSurfaces.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create the half-resolution display, primary, software, and clipper
 * surfaces used by fullscreen rendering.
 *
 * Evidence: BN builds the primary/backbuffer surface desc, retrieves the
 * attached backbuffer, reads the selected hardware-device feature flags
 * directly when choosing software-surface caps, creates the half-resolution
 * software surface from the current video dimensions, initializes pixel
 * packing, then attaches a window clipper to the display-mode surface.
 */
int CreateHalfResBackbufferSurfaces() {
    DDSURFACEDESC desc = {0};
    DDSCAPS attachedCaps = {0};
    int defaultGfxFlagsPayload = 0;
    zOptionEntryPartial *gfxFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    if (gfxFlagsOption == 0) {
        gfxFlagsOption = (zOptionEntryPartial *)(&defaultGfxFlagsPayload);
    }

    desc.dwSize = sizeof(desc);
    desc.dwBackBufferCount = 1;
    desc.dwFlags = 0x21;
    desc.ddsCaps.dwCaps = 0x218;

    HRESULT hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_DisplayModeSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x41f
        );
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    attachedCaps.dwCaps = DDSCAPS_BACKBUFFER;
    hresult = g_zVideo_DisplayModeSurfaceState.surf->GetAttachedSurface(
        &attachedCaps,
        &g_zVideo_PrimarySurfaceState.surf
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x429
        );
    }

    desc.dwFlags = 0x07;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = (DWORD)(g_zVideo_SwSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_SwSurfaceState.height);

    hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_SwSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x43f
        );
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(
        0,
        &g_zVideo_pClipper,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x447
        );
    }

    hresult = g_zVideo_pClipper->SetHWnd(
        0,
        g_zVideo_hWnd
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x44b
        );
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x450
        );
    }

    return 0;
}

/**
 * Reimplements 0x4a8b20: zVideo_dd::CreateFullscreenSoftwareSurfaces.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create fullscreen DirectDraw display, primary, software, and
 * clipper surfaces for the software renderer path.
 *
 * Evidence: BN creates the display-mode surface, probes lockability with a
 * fallback to a plain primary surface, creates primary and software surfaces,
 * initializes pixel packing from the display surface, and installs the window
 * clipper.
 */
int CreateFullscreenSoftwareSurfaces() {
    DDSURFACEDESC desc = {0};
    int defaultGfxFlagsPayload = 0;
    zOptionEntryPartial *gfxFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    if (gfxFlagsOption == 0) {
        gfxFlagsOption = (zOptionEntryPartial *)(&defaultGfxFlagsPayload);
    }

    desc.dwSize = sizeof(desc);
    desc.dwFlags = 1;
    desc.ddsCaps.dwCaps = 0xa00;

    HRESULT hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_DisplayModeSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x4cc
        );
    }

    if (LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0) {
        g_zVideo_DisplayModeSurfaceState.surf->Release();
        desc.ddsCaps.dwCaps = 0x200;
        hresult = CreateSurface3FromDesc(
            g_zVideo_pDirectDraw2,
            &desc,
            &g_zVideo_DisplayModeSurfaceState.surf,
            0
        );
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x4da
            );
        }
    } else {
        UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    desc.dwFlags = 7;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);

    hresult =
        CreateSurface3FromDesc(
            g_zVideo_pDirectDraw2,
            &desc,
            &g_zVideo_PrimarySurfaceState.surf,
            0
        );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x4f7
        );
    }

    desc.dwFlags = 7;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);

    hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_SwSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x50d
        );
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(
        0,
        &g_zVideo_pClipper,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x515
        );
    }

    hresult = g_zVideo_pClipper->SetHWnd(
        0,
        g_zVideo_hWnd
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x519
        );
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x51d
        );
    }

    return 0;
}

/**
 * Reimplements 0x4a8dc0: zVideo_dd::CreateFullscreenHardwareSurfaces.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create fullscreen DirectDraw display, attached software, primary,
 * and clipper surfaces for the hardware renderer path.
 *
 * Evidence: BN creates a flipping display-mode surface, obtains the attached
 * backbuffer as the software surface, creates the primary render surface using
 * selected-device feature flags, initializes pixel packing, and installs the
 * window clipper.
 */
int CreateFullscreenHardwareSurfaces() {
    DDSURFACEDESC desc = {0};
    DDSCAPS attachedCaps = {0};
    desc.dwSize = sizeof(desc);
    desc.dwBackBufferCount = 1;
    desc.dwFlags = 0x21;
    desc.ddsCaps.dwCaps = 0x2218;

    HRESULT hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_DisplayModeSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x53b
        );
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    attachedCaps.dwCaps = DDSCAPS_BACKBUFFER;
    hresult = g_zVideo_DisplayModeSurfaceState.surf->GetAttachedSurface(
        &attachedCaps,
        &g_zVideo_SwSurfaceState.surf
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x546
        );
    }

    desc.dwFlags = 7;
    desc.dwWidth = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);
    const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
    desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;

    hresult =
        CreateSurface3FromDesc(
            g_zVideo_pDirectDraw2,
            &desc,
            &g_zVideo_PrimarySurfaceState.surf,
            0
        );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x557
        );
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(
        0,
        &g_zVideo_pClipper,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x55f
        );
    }

    hresult = g_zVideo_pClipper->SetHWnd(
        0,
        g_zVideo_hWnd
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x563
        );
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x567
        );
    }

    return 0;
}

/**
 * Reimplements 0x4a91b0: zVideo_dd::ReleaseAllInterfacesAndSurfaces.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release the Direct3D and DirectDraw interface globals plus tracked
 * surfaces, page-unlocking locked surfaces before their COM release.
 *
 * Evidence: BN releases the D3D material, viewport, device, Direct3D2,
 * clipper, Z-buffer, software, primary, display-mode, and palette globals in
 * this order; PageUnlock failures at source lines 0x652 and 0x662 route
 * through ReportError and stop the remaining release pass.
 */
int ReleaseAllInterfacesAndSurfaces() {
    if (g_zVideo_pD3DMaterial2 != 0) {
        g_zVideo_pD3DMaterial2->Release();
        g_zVideo_pD3DMaterial2 = 0;
    }
    if (g_zVideo_pD3DViewport2 != 0) {
        g_zVideo_pD3DViewport2->Release();
        g_zVideo_pD3DViewport2 = 0;
    }
    if (g_zVideo_pD3DDevice != 0) {
        g_zVideo_pD3DDevice->Release();
        g_zVideo_pD3DDevice = 0;
    }
    if (g_zVideo_pD3D2 != 0) {
        g_zVideo_pD3D2->Release();
        g_zVideo_pD3D2 = 0;
    }
    if (g_zVideo_pClipper != 0) {
        g_zVideo_pClipper->Release();
        g_zVideo_pClipper = 0;
    }
    if (g_zVideo_pZBufferSurface != 0) {
        g_zVideo_pZBufferSurface->Release();
        g_zVideo_pZBufferSurface = 0;
    }

    if (g_zVideo_SwSurfaceState.surf != 0) {
        if (g_zVideo_SwSurfaceState.pageLockActive != 0) {
            const HRESULT hresult = g_zVideo_SwSurfaceState.surf->PageUnlock(0);
            if (hresult != DD_OK) {
                ReportError(
                    (int)(hresult),
                    g_zVideo_SourceFile_ZvidDdC,
                    0x652
                );
                return 0;
            }
            g_zVideo_SwSurfaceState.pageLockActive = 0;
        }
        g_zVideo_SwSurfaceState.surf->Release();
        g_zVideo_SwSurfaceState.surf = 0;
    }

    if (g_zVideo_PrimarySurfaceState.surf != 0) {
        if (g_zVideo_PrimarySurfaceState.pageLockActive != 0) {
            const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->PageUnlock(0);
            if (hresult != DD_OK) {
                ReportError(
                    (int)(hresult),
                    g_zVideo_SourceFile_ZvidDdC,
                    0x662
                );
                return 0;
            }
            g_zVideo_PrimarySurfaceState.pageLockActive = 0;
        }
        g_zVideo_PrimarySurfaceState.surf->Release();
        g_zVideo_PrimarySurfaceState.surf = 0;
    }

    if (g_zVideo_DisplayModeSurfaceState.surf != 0) {
        g_zVideo_DisplayModeSurfaceState.surf->Release();
        g_zVideo_DisplayModeSurfaceState.surf = 0;
    }
    if (g_zVideo_pDDPalette != 0) {
        g_zVideo_pDDPalette->Release();
        g_zVideo_pDDPalette = 0;
    }

    return 0;
}

/**
 * Reimplements 0x4a9160: zVideo_dd::VerifySurfaceStateLocking.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: optionally ask the surface-lock verifier to validate the current
 * surface state for a teardown caller context.
 *
 * Evidence: BN gates on g_zVideo_SurfaceLockVerifyFlags bit 0x20, builds a
 * 0x28-byte zVideo_SurfaceLockVerifyArgs record with callerContext at offset
 * 0x1c, calls g_zVideo_pSurfaceLockVerifier->VerifySurfaceState, and reports
 * nonzero HRESULTs at source line 0x61a.
 */
void __fastcall VerifySurfaceStateLocking(
    int callerContext
) {
    if ((g_zVideo_SurfaceLockVerifyFlags & 0x20) == 0) {
        return;
    }

    zVideo_SurfaceLockVerifyArgs args = {0};
    args.size = sizeof(args);
    args.callerContext = callerContext;
    const int hresult = g_zVideo_pSurfaceLockVerifier->VerifySurfaceState(&args);
    if (hresult != DD_OK) {
        ReportError(
            hresult,
            g_zVideo_SourceFile_ZvidDdC,
            0x61a
        );
    }
}

/**
 * Reimplements 0x4a9300: zVideo_dd::TeardownVideoSubsystem.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: tear down the remaining DirectDraw fullscreen state after the
 * surface/interface release pass.
 *
 * Evidence: BN first calls ReleaseAllInterfacesAndSurfaces, then page-unlocks
 * and releases g_zVideo_pPageUnlockSurface, verifies and releases
 * g_zVideo_pSurfaceLockVerifier, restores IDirectDraw2 cooperative level to
 * normal, releases g_zVideo_pDirectDraw2, and clears each released global.
 */
void TeardownVideoSubsystem() {
    ReleaseAllInterfacesAndSurfaces();

    if (g_zVideo_pPageUnlockSurface != 0) {
        g_zVideo_pPageUnlockSurface->PageUnlock(0);
        g_zVideo_pPageUnlockSurface->Release();
        g_zVideo_pPageUnlockSurface = 0;
    }

    if (g_zVideo_pSurfaceLockVerifier != 0) {
        VerifySurfaceStateLocking(g_zVideo_SurfaceLockVerifyContext);
        g_zVideo_pSurfaceLockVerifier->Release();
        g_zVideo_pSurfaceLockVerifier = 0;
    }

    if (g_zVideo_pDirectDraw2 != 0) {
        g_zVideo_pDirectDraw2->SetCooperativeLevel(
            g_zVideo_hWnd,
            8
        );
        g_zVideo_pDirectDraw2->Release();
        g_zVideo_pDirectDraw2 = 0;
    }
}

/**
 * Reimplements 0x4a9890: zVideo_dd::PaletteSetEntries.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: forward palette updates to the active DirectDraw palette only in
 * 8-bpp display modes.
 *
 * Evidence: BN gates on g_zVideo_DisplayModeBpp == 8, calls
 * IDirectDrawPalette::SetEntries with flags zero, and reports failures at
 * source line 0x823 before returning 0x5a56ffff.
 */
int __fastcall PaletteSetEntries(
    unsigned short firstEntry,
    unsigned short entryCount,
    PALETTEENTRY *entries
) {
    if (g_zVideo_DisplayModeBpp != 8) {
        return 0;
    }

    const HRESULT hresult = g_zVideo_pDDPalette->SetEntries(
        0,
        firstEntry,
        entryCount,
        entries
    );
    if (hresult == DD_OK) {
        return 0;
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x823
    );
    return 0x5a56ffff;
}

/**
 * Reimplements 0x4a9920: zVideo_dd::GetHwApiDeviceFeatureFlags.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: return the DirectDraw hardware API feature flags for the indexed
 * device record.
 *
 * Evidence: BN indexes g_zVideo_HwApiDeviceTable with the 0x6ec-byte
 * zVidHwApiDeviceRecord stride and returns the m_deviceFeatureFlags field
 * without side effects. The table data owner remains blocked separately.
 */
int __fastcall GetHwApiDeviceFeatureFlags(
    int deviceIndex
) {
    return g_zVideo_HwApiDeviceTable[deviceIndex].m_deviceFeatureFlags;
}

/**
 * Reimplements 0x4ad6a0: zVideo_dd::ReportError.
 * Original file: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: maps DirectDraw/Direct3D HRESULTs to report text and emits the legacy DirectDraw error report.
 */
RECOIL_NO_GS int __fastcall ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
) {
    char errorNameBuffer[0x100];

#define ZVIDEO_DD_REPORT_ERROR_NAME(nameText) \
    sprintf( \
        errorNameBuffer, \
        nameText \
    )

    switch (hresult) {
    case DDERR_GENERIC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Generic);
        break;
    case DDERR_UNSUPPORTED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Unsupported);
        break;
    case DDERR_OUTOFMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OutOfMemory);
        break;
    case DDERR_NOTINITIALIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotInitialized);
        break;
    case DDERR_INVALIDPARAMS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidParams);
        break;
    case DDERR_ALREADYINITIALIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_AlreadyInitialized);
        break;
    case DDERR_CANNOTATTACHSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CannotAttachSurface);
        break;
    case DDERR_CANNOTDETACHSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CannotDetachSurface);
        break;
    case DDERR_CURRENTLYNOTAVAIL:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CurrentlyNotAvail);
        break;
    case DDERR_EXCEPTION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Exception);
        break;
    case DDERR_HEIGHTALIGN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HeightAlign);
        break;
    case DDERR_INVALIDCAPS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidCaps);
        break;
    case DDERR_INVALIDCLIPLIST:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidClipList);
        break;
    case DDERR_INVALIDMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidMode);
        break;
    case DDERR_INVALIDOBJECT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidObject);
        break;
    case DDERR_INVALIDPIXELFORMAT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidPixelFormat);
        break;
    case DDERR_INVALIDRECT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidRect);
        break;
    case DDERR_LOCKEDSURFACES:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_LockedSurfaces);
        break;
    case DDERR_NO3D:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_No3d);
        break;
    case DDERR_NOALPHAHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoAlphaHw);
        break;
    case DDERR_NOCLIPLIST:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoClipList);
        break;
    case DDERR_NOCOLORCONVHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoColorConvHw);
        break;
    case DDERR_NOCOOPERATIVELEVELSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoCooperativeLevelSet);
        break;
    case DDERR_NOCOLORKEY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoColorKey);
        break;
    case DDERR_NOCOLORKEYHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoColorKeyHw);
        break;
    case DDERR_NODIRECTDRAWSUPPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDrawSupport);
        break;
    case DDERR_NOEXCLUSIVEMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoExclusiveMode);
        break;
    case DDERR_NOFLIPHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoFlipHw);
        break;
    case DDERR_NOGDI:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoGdi);
        break;
    case DDERR_NOMIRRORHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoMirrorHw);
        break;
    case DDERR_NOTFOUND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotFound);
        break;
    case DDERR_NOOVERLAYHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoOverlayHw);
        break;
    case DDERR_NORASTEROPHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoRasterOpHw);
        break;
    case DDERR_NOROTATIONHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoRotationHw);
        break;
    case DDERR_NOSTRETCHHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoStretchHw);
        break;
    case DDERR_NOT4BITCOLOR:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Not4BitColor);
        break;
    case DDERR_NOT4BITCOLORINDEX:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Not4BitColorIndex);
        break;
    case DDERR_NOT8BITCOLOR:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Not8BitColor);
        break;
    case DDERR_NOTEXTUREHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoTextureHw);
        break;
    case DDERR_NOVSYNCHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoVSyncHw);
        break;
    case DDERR_NOZBUFFERHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoZBufferHw);
        break;
    case DDERR_NOZOVERLAYHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoZOverlayHw);
        break;
    case DDERR_OUTOFCAPS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OutOfCaps);
        break;
    case DDERR_OUTOFVIDEOMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OutOfVideoMemory);
        break;
    case DDERR_OVERLAYCANTCLIP:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayCantClip);
        break;
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayColorKeyOnlyOneActive);
        break;
    case DDERR_PALETTEBUSY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_PaletteBusy);
        break;
    case DDERR_COLORKEYNOTSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ColorKeyNotSet);
        break;
    case DDERR_SURFACEALREADYATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceAlreadyAttached);
        break;
    case DDERR_SURFACEALREADYDEPENDENT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceAlreadyDependent);
        break;
    case DDERR_SURFACEBUSY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceBusy);
        break;
    case DDERR_CANTLOCKSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantLockSurface);
        break;
    case DDERR_SURFACEISOBSCURED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceIsObscured);
        break;
    case DDERR_SURFACELOST:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceLost);
        break;
    case DDERR_SURFACENOTATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceNotAttached);
        break;
    case DDERR_TOOBIGHEIGHT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_TooBigHeight);
        break;
    case DDERR_TOOBIGSIZE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_TooBigSize);
        break;
    case DDERR_TOOBIGWIDTH:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_TooBigWidth);
        break;
    case DDERR_UNSUPPORTEDFORMAT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_UnsupportedFormat);
        break;
    case DDERR_UNSUPPORTEDMASK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_UnsupportedMask);
        break;
    case DDERR_VERTICALBLANKINPROGRESS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_VerticalBlankInProgress);
        break;
    case DDERR_WASSTILLDRAWING:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_WasStillDrawing);
        break;
    case DDERR_CANTPAGELOCK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantPageLock);
        break;
    case DDERR_CANTPAGEUNLOCK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantPageUnlock);
        break;
    case DDERR_NOTPAGELOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotPageLocked);
        break;
    case DDERR_XALIGN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_XAlign);
        break;
    case DDERR_INVALIDDIRECTDRAWGUID:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidDirectDrawGuid);
        break;
    case DDERR_DIRECTDRAWALREADYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_DirectDrawAlreadyCreated);
        break;
    case DDERR_NODIRECTDRAWHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDrawHw);
        break;
    case DDERR_PRIMARYSURFACEALREADYEXISTS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_PrimarySurfaceAlreadyExists);
        break;
    case DDERR_NOEMULATION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoEmulation);
        break;
    case DDERR_REGIONTOOSMALL:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_RegionTooSmall);
        break;
    case DDERR_CLIPPERISUSINGHWND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ClipperIsUsingHwnd);
        break;
    case DDERR_NOCLIPPERATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoClipperAttached);
        break;
    case DDERR_NOHWND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoHwnd);
        break;
    case DDERR_HWNDSUBCLASSED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HwndSubclassed);
        break;
    case DDERR_HWNDALREADYSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HwndAlreadySet);
        break;
    case DDERR_NOPALETTEATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoPaletteAttached);
        break;
    case DDERR_NOPALETTEHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoPaletteHw);
        break;
    case DDERR_BLTFASTCANTCLIP:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_BltFastCantClip);
        break;
    case DDERR_NOBLTHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoBltHw);
        break;
    case DDERR_NODDROPSHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDdRopsHw);
        break;
    case DDERR_OVERLAYNOTVISIBLE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayNotVisible);
        break;
    case DDERR_INVALIDPOSITION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidPosition);
        break;
    case DDERR_NOTAOVERLAYSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoAOverlaySurface);
        break;
    case DDERR_EXCLUSIVEMODEALREADYSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ExclusiveModeAlreadySet);
        break;
    case DDERR_NOTFLIPPABLE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotFlippable);
        break;
    case DDERR_CANTDUPLICATE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantDuplicate);
        break;
    case DDERR_NOTLOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotLocked);
        break;
    case DDERR_CANTCREATEDC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantCreateDc);
        break;
    case DDERR_NODC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDc);
        break;
    case DDERR_WRONGMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_WrongMode);
        break;
    case DDERR_IMPLICITLYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ImplicitlyCreated);
        break;
    case DDERR_NOTPALETTIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotPalettized);
        break;
    case DDERR_UNSUPPORTEDMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_UnsupportedMode);
        break;
    case DDERR_NOMIPMAPHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoMipMapHw);
        break;
    case DDERR_INVALIDSURFACETYPE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidSurfaceType);
        break;
    case DDERR_DCALREADYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_DcAlreadyCreated);
        break;
    case D3DERR_BADMAJORVERSION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_BadMajorVersion);
        break;
    case D3DERR_BADMINORVERSION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_BadMinorVersion);
        break;
    case D3DERR_INVALID_DEVICE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidDevice);
        break;
    case D3DERR_EXECUTE_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteCreateFailed);
        break;
    case D3DERR_EXECUTE_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteDestroyFailed);
        break;
    case D3DERR_EXECUTE_LOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteLockFailed);
        break;
    case D3DERR_EXECUTE_UNLOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteUnlockFailed);
        break;
    case D3DERR_EXECUTE_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteLocked);
        break;
    case D3DERR_EXECUTE_NOT_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteNotLocked);
        break;
    case D3DERR_EXECUTE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteFailed);
        break;
    case D3DERR_EXECUTE_CLIPPED_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteClippedFailed);
        break;
    case D3DERR_TEXTURE_NO_SUPPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureNoSupport);
        break;
    case D3DERR_TEXTURE_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureCreateFailed);
        break;
    case D3DERR_TEXTURE_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureDestroyFailed);
        break;
    case D3DERR_TEXTURE_LOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLockFailed);
        break;
    case D3DERR_TEXTURE_UNLOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureUnlockFailed);
        break;
    case D3DERR_TEXTURE_LOAD_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLoadFailed);
        break;
    case D3DERR_TEXTURE_SWAP_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureSwapFailed);
        break;
    case D3DERR_TEXTURE_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLocked);
        break;
    case D3DERR_TEXTURE_NOT_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureNotLocked);
        break;
    case D3DERR_TEXTURE_GETSURF_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureGetSurfFailed);
        break;
    case D3DERR_MATRIX_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixCreateFailed);
        break;
    case D3DERR_MATRIX_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixDestroyFailed);
        break;
    case D3DERR_MATRIX_SETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixSetDataFailed);
        break;
    case D3DERR_MATRIX_GETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixGetDataFailed);
        break;
    case D3DERR_SETVIEWPORTDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SetViewportDataFailed);
        break;
    case D3DERR_INVALIDCURRENTVIEWPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidCurrentViewport);
        break;
    case D3DERR_INVALIDPRIMITIVETYPE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidPrimitiveType);
        break;
    case D3DERR_INVALIDVERTEXTYPE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidVertexType);
        break;
    case D3DERR_TEXTURE_BADSIZE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureBadSize);
        break;
    case D3DERR_MATERIAL_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialCreateFailed);
        break;
    case D3DERR_MATERIAL_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialDestroyFailed);
        break;
    case D3DERR_MATERIAL_SETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialSetDataFailed);
        break;
    case D3DERR_MATERIAL_GETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialGetDataFailed);
        break;
    case D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ZBuffNeedsSystemMemory);
        break;
    case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ZBuffNeedsVideoMemory);
        break;
    case D3DERR_LIGHT_SET_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_LightSetFailed);
        break;
    case D3DERR_SCENE_IN_SCENE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneInScene);
        break;
    case D3DERR_SCENE_NOT_IN_SCENE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneNotInScene);
        break;
    case D3DERR_SCENE_BEGIN_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneBeginFailed);
        break;
    case D3DERR_SCENE_END_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneEndFailed);
        break;
    case D3DERR_INBEGIN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InBegin);
        break;
    case D3DERR_NOTINBEGIN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_NotInBegin);
        break;
    case D3DERR_NOVIEWPORTS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_NoViewports);
        break;
    case D3DERR_VIEWPORTDATANOTSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ViewportDataNotSet);
        break;
    case DD_OK:
        return 0;
    default:
        ZVIDEO_DD_REPORT_ERROR_NAME("Unknown Error");
        break;
    }

#undef ZVIDEO_DD_REPORT_ERROR_NAME

    if (hresult == DDERR_OUTOFVIDEOMEMORY) {
        int textureMemTotalBytes;
        int textureMemFreeBytes;
        int videoMemTotalBytes;
        int videoMemFreeBytes;

        g_zVideo_pfnQueryTextureMemoryBytes(
            -1,
            &textureMemTotalBytes,
            &textureMemFreeBytes
        );
        g_zVideo_pfnQueryDeviceVideoMemoryBytes(
            -1,
            &videoMemTotalBytes,
            &videoMemFreeBytes
        );
    }

    char reportMessageBuffer[0x100];
    sprintf(
        reportMessageBuffer,
        g_zVideo_DirectDrawErrorFmt,
        errorNameBuffer,
        sourceFile,
        sourceLine
    );
    zError::ReportOld(
        0x400,
        sourceFile,
        sourceLine,
        reportMessageBuffer
    );
    return -1;
}
} // namespace zVideo_dd
