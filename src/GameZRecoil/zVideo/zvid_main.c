#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zVideo/zvid.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"
#include "zclass.h"

#include <math.h>
#include <malloc.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The retail gmod_init.c contributions compile from gmod_init.c rather than
 * this translation unit.
 */

/*
 * The retail zrndr_draw.c contributions compile from
 * src/GameZRecoil/zRender/zrndr_draw.c rather than this translation unit.
 */

/*
 * The retail zimg_texture.cpp contributions compile from
 * src/GameZRecoil/zImage/zimg_texture.cpp rather than this translation unit.
 */

namespace {
const int kZVidPaletteColorCount = 256;
const int kZVidPaletteRemapVariantCount = 32;
const int kZVidPaletteRemapColorsPerRecipe =
    kZVidPaletteColorCount * kZVidPaletteRemapVariantCount;

/**
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




} // namespace

namespace zVid_Image {
/**
 * Retail places this initialized zImage default-image owner before the
 * texture-directory state rows at 0x4e0718 and the zVideo texture-pack state
 * rows at 0x4e073c. Keep the writable pixel array and typed image record in
 * source order so final linked .data can follow the retail initialized-data
 * boundary instead of the later zVid_Image function cluster.
 * Purpose: provide the fallback 8x8 default image and backing pixels.
 */
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
} // namespace zVid_Image

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
 * Palette-remap recipe owner: BuildPaletteVariant grows this bank before
 * rebuilding per-image and standalone palette variant tables.
 * Purpose: track the active palette-remap recipe bank.
 */
int g_zVid_PaletteRemapRecipeCount = 0;
zVidPaletteRemapRecipe *g_zVid_PaletteRemapRecipes = 0;
/**
 * Purpose: cache the selected renderer path and current video frame tick.
 */
int g_zVideo_RendererType = 0;
int g_zVideo_ActiveRendererPath = 0;
int g_zVideo_FrameTick = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pactiveviewcontext
 * @recoil-artifact defines .data recoil:data:0x5398fc: g_zVideo_pActiveViewContext.
 * Render-frame active view owner data. BN types this 4-byte .data slot as a
 * zVideo_ViewContext pointer, zero-initialized, and separates its xrefs from
 * the projection/frustum context cache at 0x576214.
 * Purpose: store the camera data record used by the software render frame.
 */
zClass_CameraDataPartial *g_zVideo_pActiveViewContext = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pactiveprojectionviewcontext
 * @recoil-artifact defines .data recoil:data:0x576214: g_zVideo_pActiveProjectionViewContext.
 * Projection/frustum active view owner data. BN types this separate 4-byte
 * .data slot as a zVideo_ViewContext pointer, zero-initialized; SetActiveViewContext
 * writes it before projection, model, and frustum users read it.
 * Purpose: cache the camera data record used by projection, clip, and frustum state.
 */
zClass_CameraDataPartial *g_zVideo_pActiveProjectionViewContext = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-activeviewvarianttag
 * @recoil-artifact defines .data recoil:data:0x5398f8: g_zVideo_ActiveViewVariantTag.
 * Render-frame variant owner data. BN xrefs show zVideo_sw::RenderFrame and
 * zClass_Camera::RenderScene copying the active view variant tag into this
 * 4-byte .data zTag4 record after view selection; retail initializes it to zero.
 * Purpose: cache the currently selected variant tag for render traversal.
 */
zTag4Partial g_zVideo_ActiveViewVariantTag = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-f-0x57623c
 * @recoil-artifact defines .data recoil:data:0x57623c: g_zVideo_ProjectClipLeft.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's left project-clip edge for projection users.
 */
float g_zVideo_ProjectClipLeft = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-f-0x576240
 * @recoil-artifact defines .data recoil:data:0x576240: g_zVideo_ProjectClipTop.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's top project-clip edge for projection users.
 */
float g_zVideo_ProjectClipTop = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-f-0x576244
 * @recoil-artifact defines .data recoil:data:0x576244: g_zVideo_ProjectClipRight.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's right project-clip edge for projection users.
 */
float g_zVideo_ProjectClipRight = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-f-0x576248
 * @recoil-artifact defines .data recoil:data:0x576248: g_zVideo_ProjectClipBottom.
 * BN places this zero-initialized float in the project-clip quartet at
 * 0x57623c..0x576248; SetActiveViewContext writes it for projection users.
 * Purpose: cache the active view's bottom project-clip edge for projection users.
 */
float g_zVideo_ProjectClipBottom = 0.0f;
/**
 * Purpose: cache global video initialization and clear-screen options.
 */
int gVideo_resolutionMenuValid = 0;
unsigned int g_zVideo_ClearColorPacked16 = 0;
int g_zVideo_ClearScreenBufferEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvid-cachedclientrectupdatemask
 * @recoil-artifact defines .data recoil:data:0x56b564: g_zVid_CachedClientRectUpdateMask.
 * BN xrefs identify this zero-initialized int32 as the cached-client-rect
 * update mask used by the setter and renderer-path query helpers.
 * Purpose: gate refreshes of the cached client rectangle.
 */
int g_zVid_CachedClientRectUpdateMask = 0;
/**
 * Purpose: cache video initialization, adjust-surface, and fullscreen state.
 */
int g_zVideo_IsInitialized = 0;
int g_zVideo_AdjustSurfacesDisableGate = 0;
int g_zVideo_FullscreenOption = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-displaymodebpp
 * @recoil-artifact defines .data recoil:data:0x632150: g_zVideo_DisplayModeBpp.
 * BN models this as the zero-initialized int32 written when mode geometry is
 * applied.
 * Purpose: cache the active display mode bit depth.
 */
int g_zVideo_DisplayModeBpp = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-hwnd
 * @recoil-artifact defines .data recoil:data:0x6321c8: g_zVideo_hWnd.
 * Purpose: hold the active video target window handle.
 */
HWND g_zVideo_hWnd = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-primaryhasattachedbackbuffer
 * @recoil-artifact defines .data recoil:data:0x632134: g_zVideo_PrimaryHasAttachedBackbuffer.
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
 * Purpose: cache half-resolution backbuffer presentation state.
 */
int g_zVideo_UseHalfResBackbuffer = 0;
int g_zVideo_HalfResAdjustMode = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-softwaremodehotkeyenabled
 * @recoil-artifact defines .data recoil:data:0x4dd1c0: g_zVideo_SoftwareModeHotkeyEnabled.
 * Retail initializes the authored zVideo debug/software-mode hotkey gate enabled.
 * Purpose: gate the software-mode hotkey command.
 */
int g_zVideo_SoftwareModeHotkeyEnabled = 1;
/**
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
 * Purpose: cache target and applied D3D fog color channels.
 */
float g_zVideo_FogTargetColorR255 = 0.0f;
float g_zVideo_FogTargetColorG255 = 0.0f;
float g_zVideo_FogTargetColorB255 = 0.0f;
float g_zVideo_FogColorAppliedR255 = 0.0f;
float g_zVideo_FogColorAppliedG255 = 0.0f;
float g_zVideo_FogColorAppliedB255 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pendingditherenable
 * @recoil-artifact defines .data recoil:data:0x63213c: g_zVideo_PendingDitherEnable.
 * Purpose: stage the Direct3D dither render-state value before scene entry.
 */
int g_zVideo_PendingDitherEnable = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-f-0x56bbf4
 * @recoil-artifact defines .data recoil:data:0x56bbf4: g_zVideo_InverseZTolerancePending.
 * BN xrefs: zModel_Display_Init, zRndr::SetInverseZTolerance, and
 * zVideo::ModuleInit write this staged hardware-renderer inverse-Z tolerance.
 * Purpose: cache the inverse-Z tolerance pending for non-software renderer paths.
 */
float g_zVideo_InverseZTolerancePending = 0.0f;
/**
 * Purpose: stage Direct3D fan-closing and wireframe render state.
 */
int g_zVideo_D3DAppendFanCloseVertexPending = 0;
int g_zVideo_PendingWireframeState = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-d3dscenedepth
 * @recoil-artifact defines .data recoil:data:0x632148: g_zVideo_D3DSceneDepth.
 * BN types this 4-byte .data slot as an int32, zero-initialized, and confines
 * its xrefs to zVideoD3D::SceneEnter/SceneLeave nesting.
 * Purpose: track nested Direct3D scene entry/leave ownership.
 */
int g_zVideo_D3DSceneDepth = 0;
/**
 * DirectDraw/D3D enumeration counters. BN ties the accepted DirectDraw count
 * at 0x632f98 to the public zVid thunk at 0x4a7480, the accepted renderer
 * count at 0x632f9c to the D3D callback/accessor pair, and the enumeration
 * ordinal at 0x56bc98 only to DirectDrawEnumCallback logging.
 * Purpose: cache startup DirectDraw and Direct3D enumeration totals.
 */
int g_zVideo_NumAcceptedDirectDrawDevices = 0;
int g_zVid_AcceptedHardwareRendererCount = 0;
int g_zVideo_DirectDrawEnumOrdinal = 0;
/**
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-x04
 * @recoil-artifact defines .data recoil:data:0x4e07e8: g_zVid_DefaultImageTexturePackReadonlyNameFmt.
 * BN identifies this row as g_str_fmt_r_s, a writable char[0x04] format
 * literal used by zVid_TexturePack_EnsureDefaultImagePackLoaded.
 * Purpose: provide the renderer-prefixed default image-pack filename format.
 *
 * Retail stores this row immediately before the "image.zbd" row; both remain
 * mutable .data char arrays rather than const string literals.
 */
char g_zVid_DefaultImageTexturePackReadonlyNameFmt[0x04] = "r%s";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-x0a
 * @recoil-artifact defines .data recoil:data:0x4e07ec: g_zVid_DefaultImageTexturePackName.
 * BN identifies this row as g_str_image_zbd, a writable char[0x0a] literal
 * used by zVid_TexturePack_EnsureDefaultImagePackLoaded.
 * Purpose: provide the default image texture-pack archive filename.
 */
char g_zVid_DefaultImageTexturePackName[0x0a] = "image.zbd";
/**
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-defaulthwapidescription
 * @recoil-artifact defines .data recoil:data:0x4e307c: g_zVideo_DefaultHwApiDescription.
 * Purpose: provide the writable fallback hardware API description returned
 * when no DirectDraw hardware API record is selected.
 */
char g_zVideo_DefaultHwApiDescription[8] = "Default";
/**
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-x25
 * @recoil-artifact defines .data recoil:data:0x4e30e8: g_zVideo_SourceFile_ZvidDdC.
 * BN types this writable char[0x25] as the zvid_dd.c source-path literal
 * passed by DirectDraw diagnostics. This row is separate from the zvid_init.c,
 * zvid_buff.c, and pending adjacent zVideo source-file strings.
 * Purpose: Supplies the original DirectDraw source-file path for diagnostics.
 */
char g_zVideo_SourceFile_ZvidDdC[0x25] =
    "D:\\Proj\\GameZRecoil\\zVideo\\zvid_dd.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-x1a
 * @recoil-artifact defines .data recoil:data:0x4e3110: g_zVideo_UnrecognizedPixelFormatMsg.
 * BN types this writable char[0x1a] as the zvid_dd.c diagnostic passed only by
 * zVideo_dd::InitFullscreenSoftwarePixelPack for an unsupported pixel format.
 * Purpose: Supplies the DirectDraw software pixel-pack failure message.
 */
char g_zVideo_UnrecognizedPixelFormatMsg[0x1a] =
    "Unrecognized pixel format";
/**
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-x6
 * @recoil-artifact defines .data recoil:data:0x4e32ac: g_zVideo_DefaultD3DDeviceName.
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
 * Standalone palette-remap variant table owner: BN identifies the table count
 * and pointer array at 0x53d778/0x53d77c.
 * Purpose: cache standalone palette-remap variant tables.
 */
int g_zVid_PaletteRemapVariantTableCount = 0;
unsigned short **g_zVid_PaletteRemapVariantTables = 0;
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-xffffffffu
 * @recoil-artifact defines .data recoil:data:0x4e3370: g_zVideo_OpaqueWhiteArgb.
 * Purpose: provide the Direct3D TL-vertex color used by the opaque textured
 * submit path.
 */
unsigned int g_zVideo_OpaqueWhiteArgb = 0xffffffffu;
/**
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-x21
 * @recoil-artifact defines .data recoil:data:0x4e5b28: g_zVideo_PaletteOpenFailedFormat.
 * Data owner: render_video.zvideo_palette_brightness_runtime.
 * Purpose: supplies the palette-open diagnostic format used before the
 * palette loader returns its failure code.
 */
char g_zVideo_PaletteOpenFailedFormat[0x21] =
    "ZVID: could not open palette %s\n";
/*
 * BN models these as zero-initialized 0x20-byte zVideo_SurfaceState records:
 * the software, primary, and display-mode globals are adjacent at 0x632200,
 * 0x632220, and 0x632240.
 */
zVideo_SurfaceStatePartial g_zVideo_SwSurfaceState = {0};
zVideo_SurfaceStatePartial g_zVideo_PrimarySurfaceState = {0};
zVideo_SurfaceStatePartial g_zVideo_DisplayModeSurfaceState = {0};
char g_zVideo_PalettePathBuffer[0x100] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-palettebrightnesslevel
 * @recoil-artifact defines .data recoil:data:0x632360: g_zVideo_PaletteBrightnessLevel.
 * Purpose: cache the palette brightness adjustment used by palette loading.
 */
int g_zVideo_PaletteBrightnessLevel = 0;
PALETTEENTRY g_zVideo_PaletteFileEntries[0x100] = {0};
PALETTEENTRY g_zVideo_SystemPaletteEntries[0x100] = {0};
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_PaletteOpenFailedFormat) == 0x21);
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_PaletteBrightnessLevel) == 4);
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-cachedclientrectscreen
 * @recoil-artifact defines .data recoil:data:0x632f88: g_zVideo_CachedClientRectScreen.
 * Purpose: cache the client rectangle converted to screen coordinates.
 */
RECT g_zVideo_CachedClientRectScreen = {0};

/**
 * Purpose: track the queued sorted polygon count and draw order.
 */
int g_zVideo_SortedPolyQueueCount = 0;
int g_zVideo_SortedPolyDrawOrder[256] = {0};
/**
 * Purpose: tracks the active overwrite polygon queue count.
 */
int g_zVideo_OverwriteQueueCount = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-defaulttexturerecord
 * @recoil-artifact defines .data recoil:data:0x6333a8: g_zVideo_DefaultTextureRecord.
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
 * Purpose: dispatch texture-record upload finalization, destruction, and
 * upload-surface release hooks.
 */
zVideo_TextureRecordFinalizeUploadProc g_zVideo_pfnTextureRecordFinalizeUpload = 0;
zVideo_DestroyTextureRecordProc g_zVideo_pfnTextureRecordDestroy = 0;
zVideo_ReleaseAllTextureUploadSurfacesProc
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = 0;
/**
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pfnflushsortedpolys
 * @recoil-artifact defines .data recoil:data:0x56bc6c: g_zVideo_pfnFlushSortedPolys.
 * Purpose: dispatch the active renderer's sorted polygon flush routine.
 */
zVideo_FlushProc g_zVideo_pfnFlushSortedPolys = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pfnflushoverwritepolys
 * @recoil-artifact defines .data recoil:data:0x56bc70: g_zVideo_pfnFlushOverwritePolys.
 * Purpose: dispatch the active renderer's overwrite polygon flush routine.
 */
zVideo_FlushProc g_zVideo_pfnFlushOverwritePolys = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pfnflushquadbatch
 * @recoil-artifact defines .data recoil:data:0x56bc74: g_zVideo_pfnFlushQuadBatch.
 * Purpose: dispatch the active renderer's queued quad-batch flush routine.
 */
zVideo_FlushProc g_zVideo_pfnFlushQuadBatch = 0;

/**
 * Purpose: hold active DirectDraw and Direct3D provider interfaces.
 */
IDirectDraw2 *g_zVideo_pDirectDraw2 = 0;
IDirectDrawClipper *g_zVideo_pClipper = 0;
IDirectDrawSurface3 *g_zVideo_pPageUnlockSurface = 0;
IDirectDrawPalette *g_zVideo_pDDPalette = 0;
zVideo_SurfaceLockVerifier *g_zVideo_pSurfaceLockVerifier = 0;
IDirect3D2 *g_zVideo_pD3D2 = 0;
IDirect3DDevice2 *g_zVideo_pD3DDevice = 0;
IDirectDrawSurface3 *g_zVideo_pZBufferSurface = 0;
IDirectDrawSurface *g_zVideo_pZBufferAttachSurface = 0;
IDirect3DViewport2 *g_zVideo_pD3DViewport2 = 0;
IDirect3DMaterial2 *g_zVideo_pD3DMaterial2 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-d3dmaterialhandle
 * @recoil-artifact defines .data recoil:data:0x633404: g_zVideo_D3DMaterialHandle.
 * Purpose: cache the active Direct3D material handle.
 */
D3DMATERIALHANDLE g_zVideo_D3DMaterialHandle = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-d3drenderstatecache
 * @recoil-artifact defines .data recoil:data:0x633408: g_zVideo_D3DRenderStateCache.
 * Purpose: cache the Direct3D render states most recently applied to the
 * active device.
 */
zVideo_D3DRenderStateCacheLive g_zVideo_D3DRenderStateCache = {0};
/**
 * Purpose: cache the Direct3D fog render-state values already applied.
 */
int g_zVideo_CachedFogEnableRenderState = 0;
int g_zVideo_CachedFogModeLightState = 0;
float g_zVideo_CachedFogStartLightStateValue = 0.0f;
float g_zVideo_CachedFogEndLightStateValue = 0.0f;
/**
 * Purpose: hold Direct3D HAL and HEL device capability snapshots.
 */
D3DDEVICEDESC g_zVideo_D3DHalDeviceDesc = {0};
D3DDEVICEDESC g_zVideo_D3DHelDeviceDesc = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-quadbatchcount
 * @recoil-artifact defines .data recoil:data:0x633638: g_zVideo_QuadBatchCount.
 * Purpose: cache the queued Direct3D quad count.
 */
int g_zVideo_QuadBatchCount = 0;
zVideo_QuadBatchItemPartial g_zVideo_QuadBatchItemsBase[16] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pselectedhwapidevicerecord
 * @recoil-artifact defines .data recoil:data:0x633e40: g_zVideo_pSelectedHwApiDeviceRecord.
 * Purpose: point at the selected DirectDraw hardware API record.
 */
zVidHwApiDeviceRecordPartial *g_zVideo_pSelectedHwApiDeviceRecord = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-hwapidevicetable
 * @recoil-artifact defines .data recoil:data:0x633e44: g_zVideo_HwApiDeviceTable.
 * Cached DirectDraw hardware-device owner: BN models four 0x6ec-byte records
 * at 0x633e44. The DirectDraw enumeration callbacks populate the records;
 * memory-query, renderer-selection, and DirectDraw surface paths consume the
 * cached record fields.
 * Purpose: cache accepted DirectDraw hardware API records.
 */
zVidHwApiDeviceRecordPartial g_zVideo_HwApiDeviceTable[4] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-pselectedd3ddeviceinfo
 * @recoil-artifact defines .data recoil:data:0x6359f4: g_zVideo_pSelectedD3DDeviceInfo.
 * Selected Direct3D device-info pointer. BN types the retail 4-byte
 * zero-initialized slot as a zVidD3DDeviceInfo pointer; zVideo stores either
 * the selected hardware record's first D3D driver record or null.
 * Purpose: cache the active Direct3D device info record for name and device
 * creation queries.
 */
zVidD3DDriverRecordPartial *g_zVideo_pSelectedD3DDeviceInfo = 0;
/**
 * DirectDraw enumeration capability scratch buffers. EnumDirectDrawDeviceCallback
 * clears these zero-initialized 0x17c-byte provider records, sets dwSize, and
 * passes them to IDirectDraw2::GetCaps before accepting a hardware API record.
 * Purpose: hold HAL and HEL DirectDraw capability snapshots during enumeration.
 */
DDCAPS g_zVideo_DDrawCapsHal = {0};
DDCAPS g_zVideo_DDrawCapsHel = {0};
/**
 * Purpose: hold DirectDraw surface-lock verification state.
 */
unsigned char g_zVideo_SurfaceLockVerifyFlags = 0;
int g_zVideo_SurfaceLockVerifyContext = 0;
/**
 * Purpose: store Direct3D submit scratch vertices and queued polygons.
 */
D3DTLVERTEX g_zVideo_D3DSubmitTempVertices[64] = {0};
zVideo_SortedPolyQueueEntry g_zVideo_SortedPolyQueueBase[256] = {0};
zVideo_OverwriteQueueEntry g_zVideo_OverwriteQueueBase[0x180] = {0};

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

/*
 * 0x48da60 reads the two scratch offsets, four clip bounds, active FX-surface
 * descriptor, and scratch pointer as one zVideo pass-3 scratch copy data set.
 * 0x48daf0 writes the clip bounds for every pass and writes the scratch offsets
 * only when it switches from the direct scatter path to the clipped helper path.
 * This documents the local source shape only; the complete zVideo data owner is
 * broader than this slice and remains a parent-owned data-gate decision.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-scratchoffsetx
 * @recoil-artifact defines .data recoil:data:0x56b190: g_zVideo_FxPass3_ScratchOffsetX.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the center
 * X bias before clipped scatter calls; BN assembly for 0x48da60 loads it once
 * and applies it to both the destination delta in ECX and the source X stack
 * delta before clip tests.
 * Purpose: cache the pass-3 clipped copy X offset.
 */
int g_zVideo_FxPass3_ScratchOffsetX;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-scratchoffsety
 * @recoil-artifact defines .data recoil:data:0x56b194: g_zVideo_FxPass3_ScratchOffsetY.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the center
 * Y bias before clipped scatter calls; BN assembly for 0x48da60 loads it once
 * and applies it to both the destination delta in EDX and the source Y stack
 * delta before clip tests.
 * Purpose: cache the pass-3 clipped copy Y offset.
 */
int g_zVideo_FxPass3_ScratchOffsetY;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipminx
 * @recoil-artifact defines .data recoil:data:0x56b1a0: g_zVideo_FxPass3_ClipMinX.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper tests source
 * and destination X coordinates against it as an inclusive lower bound.
 * Purpose: cache the inclusive minimum X clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMinX;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipminy
 * @recoil-artifact defines .data recoil:data:0x56b1a4: g_zVideo_FxPass3_ClipMinY.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper tests source
 * and destination Y coordinates against it as an inclusive lower bound.
 * Purpose: cache the inclusive minimum Y clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMinY;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipmaxx
 * @recoil-artifact defines .data recoil:data:0x56b1a8: g_zVideo_FxPass3_ClipMaxX.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper treats this as
 * the exclusive maximum X edge.
 * Purpose: cache the exclusive maximum X clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMaxX;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipmaxy
 * @recoil-artifact defines .data recoil:data:0x56b1ac: g_zVideo_FxPass3_ClipMaxY.
 * Data owner evidence: zVideo::FxPass3_ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper treats this as
 * the exclusive maximum Y edge.
 * Purpose: cache the exclusive maximum Y clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMaxY;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvid-noisebytetablesize
 * @recoil-artifact defines .data recoil:data:0x56b1b8: g_zVid_NoiseByteTableSize.
 * Data owner evidence: zVid::Noise_InitBuffers writes the primary-surface
 * width multiplied by 25 before filling the byte table; DrawNoiseRect uses it
 * as the random row-window limit.
 * Purpose: cache the allocated noise-byte table length.
 */
int g_zVid_NoiseByteTableSize;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvid-noisebytetable
 * @recoil-artifact defines .data recoil:data:0x56b1bc: g_zVid_NoiseByteTable.
 * Data owner evidence: zVid::Noise_InitBuffers allocates and fills this byte
 * table, DrawNoiseRect samples it, and zVid::Noise_ShutdownBuffers frees and
 * clears it when non-null.
 * Purpose: hold the software noise bytes used by the FX surface overlay path.
 */
unsigned char *g_zVid_NoiseByteTable;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-scratchpixels16
 * @recoil-artifact defines .data recoil:data:0x56b1c0: g_zVideo_FxPass3_ScratchPixels16.
 * Data owner evidence: zVid::Noise_InitBuffers allocates a width*height
 * 16-bpp scratch buffer and stores it after clearing the active FX-surface
 * descriptor; zVid::Noise_ShutdownBuffers frees and clears it when non-null.
 * zVideo::FxPass3_CopySurfacePixelToScratchClipped at 0x48da60 writes through
 * this pointer with tight g_zVideo_FxSurfaceWidth row stride, while
 * zVideo::FxPass3_ApplyToCurrentSurface at 0x48daf0 stages the radial ring
 * warp here before copying back to the active FX surface.
 * Purpose: stage pass-3 warp, blur, and related 16-bpp FX surface pixels.
 */
unsigned short *g_zVideo_FxPass3_ScratchPixels16;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacepixels16
 * @recoil-artifact defines .data recoil:data:0x56b1c4: g_zVideo_FxSurfacePixels16.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes this active surface
 * pointer, zVid::Noise_InitBuffers clears it during scratch initialization,
 * and noise/blur/pass-3/FX-surface routines use it as the 16-bpp destination.
 * zVideo::FxPass3_CopySurfacePixelToScratchClipped at 0x48da60 reads source
 * pixels through this pointer using g_zVideo_FxSurfacePitchPixels16.
 * Purpose: point at the currently active 16-bpp FX surface pixel buffer.
 */
unsigned short *g_zVideo_FxSurfacePixels16;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacewidth
 * @recoil-artifact defines .data recoil:data:0x56b1c8: g_zVideo_FxSurfaceWidth.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes the active width,
 * zVid::Noise_InitBuffers clears it, and FX/noise/blur paths use it for bounds
 * and tight scratch-buffer row stride. FxPass3 clipped copies use this for
 * scratch row indexing, distinct from the provider pitch used for source rows.
 * Purpose: cache the active FX surface width in pixels.
 */
int g_zVideo_FxSurfaceWidth;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfaceheight
 * @recoil-artifact defines .data recoil:data:0x56b1cc: g_zVideo_FxSurfaceHeight.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes the active height,
 * zVid::Noise_InitBuffers clears it, and FX/noise/blur paths use it for full
 * surface clipping.
 * Purpose: cache the active FX surface height in pixels.
 */
int g_zVideo_FxSurfaceHeight;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacepitchbytes
 * @recoil-artifact defines .data recoil:data:0x56b1d0: g_zVideo_FxSurfacePitchBytes.
 * Data owner evidence: zVideo::Fx_SetSurfaceState writes the provider pitch in
 * bytes and zVid::Noise_InitBuffers clears it with the active surface record.
 * Purpose: retain the active FX surface row pitch in bytes.
 */
int g_zVideo_FxSurfacePitchBytes;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacepitchpixels16
 * @recoil-artifact defines .data recoil:data:0x56b1d4: g_zVideo_FxSurfacePitchPixels16.
 * Data owner evidence: zVideo::Fx_SetSurfaceState derives this from pitch
 * bytes divided by two; FX/noise/blur paths use it for source/destination row
 * stepping while scratch rows use g_zVideo_FxSurfaceWidth. BN assembly for
 * 0x48da60 reads g_zVideo_FxSurfacePixels16 after multiplying the biased
 * source Y by this value.
 * Purpose: retain the active FX surface row pitch in 16-bpp pixels.
 */
int g_zVideo_FxSurfacePitchPixels16;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-primarysurfacerectscratch
 * @recoil-artifact defines .data recoil:data:0x56bbc8: g_zVideo_PrimarySurfaceRectScratch.
 * Purpose: stores the primary surface rectangle scratch used by zVideo
 * software-present adjustment code.
 */
zVidRect32 g_zVideo_PrimarySurfaceRectScratch;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-surfacestateswapscratch
 * @recoil-artifact defines .data recoil:data:0x56bc78: g_zVideo_SurfaceStateSwapScratch.
 * BN models this as a zero-initialized 0x20-byte zVideo_SurfaceState record:
 * the swap scratch record is the same shape as the software, primary, and
 * display-mode globals but retail stores it earlier at 0x56bc78, before the
 * pass-3 config singleton at 0x56bd58. It is used only by the software
 * present adjustment path.
 * Purpose: stores the surface-state swap scratch record used by zVideo
 * software-present adjustment code.
 */
zVideo_SurfaceStatePartial g_zVideo_SurfaceStateSwapScratch;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3configlocal
 * @recoil-artifact defines .data recoil:data:0x56bd58: g_zVideo_FxPass3ConfigLocal.
 * Data owner evidence: retail 0x56bd58 is the authored zero-initialized
 * zVideoFxPass3Config singleton, complete size 0x1f0. Sibling pass-3 scratch,
 * clip, and surface globals are separate zVideo data owners.
 * Purpose: store the local pass-3 UI config used by the zVideo namespace
 * wrapper functions.
 */
zVideoFxPass3Config g_zVideo_FxPass3ConfigLocal;
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_FxPass3ConfigLocal) == 0x1f0);


















}

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

/*
 * The retail zgame_opt.c contribution compiles from the registered
 * options/runtime-probe translation unit.
 */




/* The DirectDraw-backed zVid contributions compile from zvid_dd.c. */


} // namespace zVid

/**
 * Draws the common HUD base, publishes the parent pass-3 source surface, then dispatches the
 * element-specific pass callback once for each configured input rectangle.
 * The address-backed definition compiles through the registered zui.cpp
 * physical-order target.
 */
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
 * Root pass-3 callback submits the currently selected input rectangle as a framebuffer overlay
 * using the root element's recovered color and alpha.
 * The address-backed definition compiles through the registered zui.cpp
 * physical-order target.
 */
/**
 * Constructs the pass-3 slot element and clears the input clip consumed by the
 * pass-3 element draw path.
 * The address-backed definition compiles through the registered zui.cpp
 * physical-order target.
 */

/**
 * The pass callback forwards the slot position, integer radius payload, sine parameters, and
 * active input clip to the shared pass-3 radial warp routine.
 * The address-backed definition compiles through the registered zui.cpp
 * physical-order target.
 */
/**
 * Constructs the pass-3 singleton as a HudUiContainer, installs the config and
 * element tables, links the root plus five slot children, hides them, and
 * enables the container. The retail constructor leaves surfacePitchBytes
 * untouched.
 */







/**
 * Destruction is compiler-owned: VC5 emits the reverse member/base destructor
 * path for the five embedded slots, root element, and HudUiContainer base.
 */

/*
 * The retail zvid_buff.c contributions compile from zvid_buff.c rather than
 * this translation unit.
 */

namespace zVideo_buff {


} // namespace zVideo_buff

namespace zVideo {





/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-handlesoftwaremodehotkeycommand
 * @recoil-artifact defines .text recoil:function:0x437ef0: zVideo::HandleSoftwareModeHotkeyCommand.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getdisplaymodebpp
 * @recoil-artifact defines .text recoil:function:0x4a66e0: zVideo::GetDisplayModeBpp.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached display-mode bits-per-pixel value.
 *
 * Evidence: BN assembly is a leaf load from g_zVideo_DisplayModeBpp at
 * 0x632150 followed by return.
 */
int GetDisplayModeBpp() {
    return g_zVideo_DisplayModeBpp;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-init-applymodeindex
 * @recoil-artifact defines .text recoil:function:0x4a66f0: zVideo::Init_ApplyModeIndex.
 * Purpose: provide the recovered zVideo::Init_ApplyModeIndex behavior.
 */
int __fastcall Init_ApplyModeIndex(
    int modeIndex
) {
    Init_SetSurfaceGeometryFromModeIndex(modeIndex);
    return g_zVideo_pfnSetVideoMode(modeIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getswsurfacepixels
 * @recoil-artifact defines .text recoil:function:0x4a6710: zVideo::GetSwSurfacePixels.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the current locked software surface pixel pointer.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.pixels at 0x632210.
 */
void *GetSwSurfacePixels() {
    return g_zVideo_SwSurfaceState.pixels;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getswsurfacewidth
 * @recoil-artifact defines .text recoil:function:0x4a6720: zVideo::GetSwSurfaceWidth.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached software surface width.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.width at 0x632200.
 */
int GetSwSurfaceWidth() {
    return g_zVideo_SwSurfaceState.width;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getswsurfaceheight
 * @recoil-artifact defines .text recoil:function:0x4a6730: zVideo::GetSwSurfaceHeight.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached software surface height.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.height at 0x632204.
 */
int GetSwSurfaceHeight() {
    return g_zVideo_SwSurfaceState.height;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getswsurfacepitch
 * @recoil-artifact defines .text recoil:function:0x4a6740: zVideo::GetSwSurfacePitch.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached software surface pitch.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.pitch at 0x632208.
 */
int GetSwSurfacePitch() {
    return g_zVideo_SwSurfaceState.pitch;
}

} // namespace zVideo

namespace zVideo_dd3d {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-callclearzbufferrect
 * @recoil-artifact defines .text recoil:function:0x4a6750: zVideo_dd3d::CallClearZBufferRect.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Tail-dispatch the active Z-buffer clear callback.
 */
void __fastcall CallClearZBufferRect(
    zVidRect32 *rect
) {
    g_zVideo_pfnClearZBufferRect(rect);
}

} // namespace zVideo_dd3d

namespace zVideo {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-callclearswsurfaceandzbuffer
 * @recoil-artifact defines .text recoil:function:0x4a6760: zVideo::CallClearSwSurfaceAndZBuffer.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-runpostprocessonswbuffer
 * @recoil-artifact defines .text recoil:function:0x4a6770: zVideo::RunPostprocessOnSwBuffer.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-dispatch-unlockswsurfacestate
 * @recoil-artifact defines .text recoil:function:0x4a67d0: zVideo::Dispatch_UnlockSwSurfaceState.
 * Purpose: Dispatches the configured surface unlock provider for the software surface state.
 */
int Dispatch_UnlockSwSurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_SwSurfaceState);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getswsurfacelockedflag
 * @recoil-artifact defines .text recoil:function:0x4a67e0: zVideo::GetSwSurfaceLockedFlag.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns whether the software surface state currently holds a lock.
 *
 * Evidence: BN is a leaf load from g_zVideo_SwSurfaceState.locked at 0x632214.
 */
int GetSwSurfaceLockedFlag() {
    return g_zVideo_SwSurfaceState.locked;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getprimarysurfacepixels
 * @recoil-artifact defines .text recoil:function:0x4a67f0: zVideo::GetPrimarySurfacePixels.
 * Purpose: Returns the current primary surface pixel pointer from the recovered surface-state global.
 */
void *GetPrimarySurfacePixels() {
    return g_zVideo_PrimarySurfaceState.pixels;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getprimarysurfacewidth
 * @recoil-artifact defines .text recoil:function:0x4a6800: zVideo::GetPrimarySurfaceWidth.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
 * Purpose: return the current primary surface width from the recovered surface-state global.
 */
int GetPrimarySurfaceWidth() {
    return g_zVideo_PrimarySurfaceState.width;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getprimarysurfaceheight
 * @recoil-artifact defines .text recoil:function:0x4a6810: zVideo::GetPrimarySurfaceHeight.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached primary surface height.
 *
 * Evidence: BN is a leaf load from g_zVideo_PrimarySurfaceState.height at
 * 0x632224.
 */
int GetPrimarySurfaceHeight() {
    return g_zVideo_PrimarySurfaceState.height;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-getprimarysurfacepitch
 * @recoil-artifact defines .text recoil:function:0x4a6820: zVideo::GetPrimarySurfacePitch.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: returns the cached primary surface pitch.
 *
 * Evidence: BN is a leaf load from g_zVideo_PrimarySurfaceState.pitch at
 * 0x632228.
 */
int GetPrimarySurfacePitch() {
    return g_zVideo_PrimarySurfaceState.pitch;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-callclearprimarysurfaceandzbuffer
 * @recoil-artifact defines .text recoil:function:0x4a6830: zVideo::CallClearPrimarySurfaceAndZBuffer.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-runpostprocessonprimarybuffer
 * @recoil-artifact defines .text recoil:function:0x4a6840: zVideo::RunPostprocessOnPrimaryBuffer.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-dispatch-unlockprimarysurfacestate
 * @recoil-artifact defines .text recoil:function:0x4a68d0: zVideo::Dispatch_UnlockPrimarySurfaceState.
 * Purpose: Dispatches the configured surface unlock provider for the primary surface state.
 */
int Dispatch_UnlockPrimarySurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-dispatch-lockdisplaymodesurfacestate
 * @recoil-artifact defines .text recoil:function:0x4a68e0: zVideo::Dispatch_LockDisplayModeSurfaceState.
 * Purpose: Dispatches the configured surface lock provider for the display-mode surface state.
 */
int Dispatch_LockDisplayModeSurfaceState() {
    return g_zVideo_pfnLockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-dispatch-unlockdisplaymodesurfacestate
 * @recoil-artifact defines .text recoil:function:0x4a68f0: zVideo::Dispatch_UnlockDisplayModeSurfaceState.
 * Purpose: Dispatches the configured surface unlock provider for the display-mode surface state.
 */
int Dispatch_UnlockDisplayModeSurfaceState() {
    return g_zVideo_pfnUnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-adjustsurfacesifenabled
 * @recoil-artifact defines .text recoil:function:0x4a6900: zVideo::PresentOrAdjustSurfacesIfEnabled.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-loadpalettefileandapplybrightness
 * @recoil-artifact defines .text recoil:function:0x4c7fd0: zVideo::LoadPaletteFileAndApplyBrightness.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-applybrightnesstopaletteentries
 * @recoil-artifact defines .text recoil:function:0x4c8070: zVideo::ApplyBrightnessToPaletteEntries.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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


















} // namespace zVideo

namespace zVideo {

/**
 * Source-shape evidence: the VC5 coverage symbol is the zVideo namespace
 * helper `?zVideoFxPass3Config_UpdateLocal@zVideo@@YIXPAUzVideoFxPass3Config@@M@Z`,
 * with the config object passed explicitly rather than as a C++ member method.
 * Purpose: update the local pass-3 config container and reset its slot queue.
 */

/**
 * Source-shape evidence: sibling local-config helpers in this cluster use
 * zVideo namespace `__fastcall` functions with an explicit config pointer;
 * the public zVideo wrapper at 0x4beee0 supplies the singleton.
 * Purpose: arm the root pass-3 element with the primary overlay parameters.
 */

/**
 * Source-shape evidence: this is the same local-config helper family as
 * 0x4bed30 and keeps the config as an explicit namespace-function parameter;
 * the public zVideo wrapper at 0x4bef10 supplies the singleton.
 * Purpose: queue one local pass-3 slot payload for the next config update.
 */

/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: return the zVideo success status for dispatch slots that need no
 * backend-specific action.
 *
 * Evidence: BN is a leaf zero-return function with no callees or globals.
 */
int ReturnSuccessStub() {
    return 0;
}






} // namespace zVideo












namespace zVideo {






} // namespace zVideo

namespace zVid {




} // namespace zVid

namespace zVideo_FxSurface {








} // namespace zVideo_FxSurface

namespace zVid_Image {















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




} // namespace zVid_Image

namespace zVid_PaletteRemap {

} // namespace zVid_PaletteRemap










namespace zVid_TexturePack {

} // namespace zVid_TexturePack


namespace zVid_TexturePack {

} // namespace zVid_TexturePack




namespace zVideoD3D {



} // namespace zVideoD3D

/*
 * The retail zvid_ddd3d.c contributions compile from zvid_ddd3d.c rather than
 * this translation unit.
 */

namespace zVideo_dd {
/* The remaining DirectDraw backend contributions compile from zvid_dd.c. */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-preparewindowformode
 * @recoil-artifact defines .text recoil:function:0x4a6930: zVideo_dd::PrepareWindowForMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
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

} // namespace zVideo_dd
/*
 * The retail zvid_init.c contributions compile from zvid_init.c rather than
 * this translation unit.
 */

 #include "recoil/Mfc42Abi.h"
