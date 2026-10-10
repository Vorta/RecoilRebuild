#include "GameZRecoil/zVideo/zvid.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zVideo/zvid_state.h"

/* zGame entry point; zgame.h declares it for the C++ units only. */
void __cdecl ReturnOnlyStub(void);

/*
 * Recovered literal-backed zvid_init.c physical contribution
 * [0x4a6b40, 0x4a7b40). Definitions remain in natural retail source order;
 * compiler-emitted switch lowering belongs to InitSetSurfaceGeometryFromModeIndex.
 */

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-set-renderer-type-and-active-path
 * @recoil-artifact defines .text recoil:function:0x4a6b40: zVideo::SetRendererTypeAndActivePath.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: updates the active renderer backend globals and returns the
 * previous renderer type.
 *
 * Evidence: BN loads g_zVideo_RendererType at 0x632120, stores the requested
 * backend to g_zVideo_RendererType and g_zVideo_ActiveRendererPath at
 * 0x56bbe8, and returns the old renderer value.
 */
int __fastcall SetRendererTypeAndActivePath(int rendererType)
{
    const int previousRendererType = g_zVideo_RendererType;
    g_zVideo_RendererType = rendererType;
    g_zVideo_ActiveRendererPath = rendererType;
    return previousRendererType;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-dd3d-set-pending-wireframe-state
 * @recoil-artifact defines .text recoil:function:0x4a6b60: zVideo_dd3d::SetPendingWireframeState.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: store the deferred Direct3D wireframe fill-mode request.
 *
 * Evidence: BN stores ecx directly into g_zVideo_PendingWireframeState, which
 * BeginSceneAndFlushPendingRenderStates later consumes and resets.
 */
void __fastcall SetPendingWireframeState(int pendingWireframeState)
{
    g_zVideo_PendingWireframeState = pendingWireframeState;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-dd3d-set-pending-dither-enable
 * @recoil-artifact defines .text recoil:function:0x4a6b70: zVideo_dd3d::SetPendingDitherEnable.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: store the deferred Direct3D dither-enable render-state request.
 *
 * Evidence: BN stores ecx directly into g_zVideo_PendingDitherEnable, which
 * BeginSceneAndFlushPendingRenderStates applies to D3DRENDERSTATE_DITHERENABLE.
 */
void __fastcall SetPendingDitherEnable(int enabled)
{
    g_zVideo_PendingDitherEnable = enabled;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-set-clear-color-packed16
 * @recoil-artifact defines .text recoil:function:0x4a6b80: zVideoSetClearColorPacked16.
 * @recoil-match byte
 *
 * Purpose: store the packed 16-bit clear color used by zVideo clear paths.
 *
 * Evidence: BN source file zVideo.cpp is a leaf fastcall store of ECX into
 * zero-initialized g_zVideo_ClearColorPacked16 at 0x6321cc.
 */
void __fastcall zVideoSetClearColorPacked16(unsigned int packedColor16)
{
    g_zVideo_ClearColorPacked16 = packedColor16;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-pixel-pack-get-rgb-bits
 * @recoil-artifact defines .text recoil:function:0x4a6b90: zVideo::PixelPackGetRgbBits.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: return the cached display RGB channel bit counts.
 */
void __fastcall PixelPackGetRgbBits(int* outRBits, int* outGBits, int* outBBits)
{
    *outRBits = g_zVideo_PixelPack.rBits;
    *outGBits = g_zVideo_PixelPack.gBits;
    *outBBits = g_zVideo_PixelPack.bBits;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-pixel-pack-get-rgb-masks
 * @recoil-artifact defines .text recoil:function:0x4a6bb0: zVideo::PixelPackGetRgbMasks.
 * @recoil-match byte
 *
 * Purpose: Return the cached RGB bit masks from the active pixel-pack record.
 */
void __fastcall PixelPackGetRgbMasks(unsigned int* outRMask, unsigned int* outGMask, unsigned int* outBMask)
{
    *outRMask = g_zVideo_PixelPack.rMask;
    *outGMask = g_zVideo_PixelPack.gMask;
    *outBMask = g_zVideo_PixelPack.bMask;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-pixel-pack-get-packing-params
 * @recoil-artifact defines .text recoil:function:0x4a6bd0: zVideo::PixelPackGetPackingParams.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: return the cached packed RGB shift parameters.
 */
void __fastcall PixelPackGetPackingParams(int* outPackedBase, int* outSumMinus8, int* outBShiftTo8)
{
    *outPackedBase = g_zVideo_PixelPack.packedBase;
    *outSumMinus8 = g_zVideo_PixelPack.sumMinus8;
    *outBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-pixel-pack-setup-from-masks
 * @recoil-artifact defines .text recoil:function:0x4a6bf0: zVideo::PixelPackSetupFromMasks.
 * @recoil-match source
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: initialize the global display pixel-pack bit counts, masks, and
 * shifted channel masks from DirectDraw pixel-format masks.
 */
void __fastcall PixelPackSetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask
)
{
    g_zVideo_PixelPack.rBits = redBits;
    g_zVideo_PixelPack.gBits = greenBits;
    g_zVideo_PixelPack.bBits = blueBits;
    g_zVideo_PixelPack.rMask = redMask;
    g_zVideo_PixelPack.gMask = greenMask;
    g_zVideo_PixelPack.bMask = blueMask;
    g_zVideo_PixelPack.packedBase = redBits + greenBits + blueBits - 8;
    g_zVideo_PixelPack.sumMinus8 = greenBits + blueBits - 8;
    g_zVideo_PixelPack.bShiftTo8 = 8 - blueBits;
    g_zVideo_PixelPack.rMaskShifted = ((1 << redBits) - 1) << (8 - redBits);
    g_zVideo_PixelPack.gMaskShifted = ((1 << greenBits) - 1) << (8 - greenBits);
    g_zVideo_PixelPack.bMaskShifted = ((1 << blueBits) - 1) << (8 - blueBits);
}

/**
 * @recoil-anchor recoil:anchor:zvid.pack-color-00rrggbb
 * @recoil-artifact defines .text recoil:function:0x4a6ca0: zVidPackColor00RRGGBB.
 * @recoil-match source
 *
 * Purpose: provide the recovered zVidPackColor00RRGGBB behavior.
 */
unsigned short __fastcall zVidPackColor00RRGGBB(unsigned int color00RRGGBB)
{
    unsigned short packed = (unsigned short)((g_zVideo_PixelPack.gMaskShifted & GetGValue(color00RRGGBB))
        << g_zVideo_PixelPack.sumMinus8);
    packed |= (g_zVideo_PixelPack.rMaskShifted & GetRValue(color00RRGGBB)) << g_zVideo_PixelPack.packedBase;
    packed |= GetBValue(color00RRGGBB) >> g_zVideo_PixelPack.bShiftTo8;
    return packed;
}

/**
 * @recoil-anchor recoil:anchor:zvid.pack-color-rgb
 * @recoil-artifact defines .text recoil:function:0x4a6cf0: zVidPackColorRGB.
 * @recoil-match source
 *
 * Purpose: Pack 8-bit RGB components into the active framebuffer pixel format.
 * BN passes red and green as low-byte fastcall registers and consumes the low
 * byte of the stack blue argument.
 */
unsigned short __fastcall zVidPackColorRGB(unsigned char red, unsigned char green, unsigned char blue)
{
    unsigned short packed = (unsigned short)((g_zVideo_PixelPack.gMaskShifted & green) << g_zVideo_PixelPack.sumMinus8);
    packed |= (g_zVideo_PixelPack.rMaskShifted & red) << g_zVideo_PixelPack.packedBase;
    packed |= (unsigned char)((unsigned char)blue >> g_zVideo_PixelPack.bShiftTo8);
    return packed;
}

/**
 * @recoil-anchor recoil:anchor:zvid.pack-color-rgb-floats
 * @recoil-artifact defines .text recoil:function:0x4a6d40: zVidPackColorRgbFloats.
 * @recoil-match source
 *
 * Purpose: round RGB float channels and pack them through the active 16-bit pixel format.
 */
zVideo_PackedColor16 __fastcall zVidPackColorRgbFloats(zVideo_ColorRgbFloat* color)
{
    unsigned short packed;
    zVideo_PackedColor16 result;

    packed = (unsigned short)(int)(color->r + 0.5f);
    packed &= (unsigned short)(g_zVideo_PixelPack.rMaskShifted);
    packed = (unsigned short)((int)(packed) << g_zVideo_PixelPack.packedBase);
    packed = (unsigned short)(packed
        | (((int)(color->g + 0.5f) & g_zVideo_PixelPack.gMaskShifted) << g_zVideo_PixelPack.sumMinus8));
    packed = (unsigned short)(packed | ((unsigned short)(int)(color->b + 0.5f) >> g_zVideo_PixelPack.bShiftTo8));
    result.value = packed;
    return result;
}

/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: initializes the global texture pixel-pack bit counts, masks,
 * shifted channel masks, and inverse non-RGB shifted mask.
 *
 * Evidence: BN assembly writes the contiguous texture pixel-pack globals at
 * 0x632188..0x6321c4 from the RGB/A bit widths and masks; HLIL's low-byte
 * shift rendering is a decompiler artifact, while assembly uses 32-bit shift
 * counts.
 */
void __fastcall TexturePixelPackSetupFromMasks(
    int redBits,
    int greenBits,
    int blueBits,
    int alphaBits,
    unsigned int redMask,
    unsigned int greenMask,
    unsigned int blueMask,
    unsigned int alphaMask
)
{
    // Plain member stores; this order reproduces retail's parameter-load registers.
    g_zVideo_TexturePixelPack_ABits = alphaBits;
    g_zVideo_TexturePixelPack_AMask = alphaMask;
    g_zVideo_TexturePixelPack_RMask = redMask;
    g_zVideo_TexturePixelPack_GMask = greenMask;
    g_zVideo_TexturePixelPack_BMask = blueMask;
    g_zVideo_TexturePixelPack_RBits = redBits;
    g_zVideo_TexturePixelPack_RGBBitsTotal = redBits + greenBits + blueBits;
    g_zVideo_TexturePixelPack_RGBBitsTotalMinus8 = redBits + greenBits + blueBits - 8;
    g_zVideo_TexturePixelPack_GBBitsTotalMinus8 = greenBits + blueBits - 8;
    g_zVideo_TexturePixelPack_GBits = greenBits;
    g_zVideo_TexturePixelPack_BBits = blueBits;
    g_zVideo_TexturePixelPack_BShiftTo8 = 8 - blueBits;
    g_zVideo_TexturePixelPack_RMaskShifted = ((1 << redBits) - 1) << (8 - redBits);
    g_zVideo_TexturePixelPack_GMaskShifted = ((1 << greenBits) - 1) << (8 - greenBits);
    g_zVideo_TexturePixelPack_BMaskShifted = ((1 << blueBits) - 1) << (8 - blueBits);
    g_zVideo_TexturePixelPack_NonRgbMaskShifted
        = ~(g_zVideo_TexturePixelPack_RMaskShifted | g_zVideo_TexturePixelPack_GMaskShifted
            | g_zVideo_TexturePixelPack_BMaskShifted);
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-videobuff-capture-surface-to-image
 * @recoil-artifact defines .text recoil:function:0x4a6e80: zVideobuffCaptureSurfaceToImage.
 * @recoil-match byte
 *
 * Purpose: Captures a selected 16-bit video surface into an owned zVid image.
 */
zVidImagePartial* __fastcall zVideobuffCaptureSurfaceToImage(int sourceSelector)
{
    int width;
    int height;
    unsigned int pitchWords;
    unsigned char* srcPixels;
    zVidImagePartial* image;
    unsigned char* dstBytes;

    DispatchLockDisplayModeSurfaceState();
    switch (sourceSelector) {
    case 0:
        width = g_zVideo_SwSurfaceState.width;
        height = g_zVideo_SwSurfaceState.height;
        pitchWords = (unsigned int)(g_zVideo_SwSurfaceState.pitch) >> 1;
        srcPixels = (unsigned char*)(g_zVideo_SwSurfaceState.pixels);
        break;
    case 1:
        width = g_zVideo_PrimarySurfaceState.width;
        height = g_zVideo_PrimarySurfaceState.height;
        pitchWords = (unsigned int)(g_zVideo_PrimarySurfaceState.pitch) >> 1;
        srcPixels = (unsigned char*)(g_zVideo_PrimarySurfaceState.pixels);
        break;
    case 2:
        width = g_zVideo_DisplayModeSurfaceState.width;
        height = g_zVideo_DisplayModeSurfaceState.height;
        pitchWords = (unsigned int)(g_zVideo_DisplayModeSurfaceState.pitch) >> 1;
        srcPixels = (unsigned char*)(g_zVideo_DisplayModeSurfaceState.pixels);
        break;
    default:
        return 0;
    }

    image = Create();
    if (image == 0) {
        return 0;
    }

    SetSize(image, (short)(width), (short)(height));
    dstBytes = (unsigned char*)(malloc((size_t)(image->pixelCount) * sizeof(unsigned short)));
    image->formatFlagsPacked |= 0x20u;
    zVidImageSetPixels(image, dstBytes, 0);

    if (width != (int)(pitchWords)) {
        if (height > 0) {
            const int rowBytes = width * sizeof(unsigned short);
            const int pitchBytes = (int)(pitchWords * sizeof(unsigned short));
            int row = height;
            do {
                memcpy(dstBytes, srcPixels, (size_t)(rowBytes));
                dstBytes += rowBytes;
                srcPixels += pitchBytes;
            } while (--row != 0);
        }
    } else {
        memcpy(dstBytes, srcPixels, (size_t)(image->pixelCount) * sizeof(unsigned short));
    }

    DispatchUnlockDisplayModeSurfaceState();
    return image;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-buff-copy-surface-rect-to-image
 * @recoil-artifact defines .text recoil:function:0x4a6fe0: zVideo_buff::CopySurfaceRectToImage.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zImage/zvid_buff.c.
 * Purpose: provide the recovered zVideo_buff::CopySurfaceRectToImage behavior.
 */
zVidImagePartial* __fastcall CopySurfaceRectToImage(int sourceSelector, zVidRect32* rect, zVidImagePartial* image)
{
    int surfaceWidth;
    int surfaceHeight;
    unsigned int pitchWords;
    unsigned char* surfacePixels;
    // Retail reserves a whole rect for the destination offsets; only left/top are used.
    zVidRect32 dstRect;
    int originalWidth;
    int clipped;
    int clippedWidth;
    int clippedHeight;
    unsigned char* dstBytes;
    unsigned char* srcBytes;
    int row;
    switch (sourceSelector) {
    case 0:
        surfaceWidth = g_zVideo_SwSurfaceState.width;
        surfaceHeight = g_zVideo_SwSurfaceState.height;
        pitchWords = (unsigned int)(g_zVideo_SwSurfaceState.pitch) >> 1;
        surfacePixels = (unsigned char*)(g_zVideo_SwSurfaceState.pixels);
        break;
    case 1:
        surfaceWidth = g_zVideo_PrimarySurfaceState.width;
        surfaceHeight = g_zVideo_PrimarySurfaceState.height;
        pitchWords = (unsigned int)(g_zVideo_PrimarySurfaceState.pitch) >> 1;
        surfacePixels = (unsigned char*)(g_zVideo_PrimarySurfaceState.pixels);
        break;
    case 2:
        surfaceWidth = g_zVideo_DisplayModeSurfaceState.width;
        surfaceHeight = g_zVideo_DisplayModeSurfaceState.height;
        pitchWords = (unsigned int)(g_zVideo_DisplayModeSurfaceState.pitch) >> 1;
        surfacePixels = (unsigned char*)(g_zVideo_DisplayModeSurfaceState.pixels);
        break;
    default:
        return 0;
    }

    dstRect.top = 0;
    dstRect.left = 0;
    originalWidth = rect->right - rect->left;

    clipped = ClipCoordToRange(&rect->left, 0, surfaceWidth);
    if (clipped < 0) {
        dstRect.left = -clipped;
    } else if (clipped > 0) {
        return 0;
    }

    clipped = ClipCoordToRange(&rect->right, 0, surfaceWidth);
    if (clipped < 0) {
        return 0;
    }

    clipped = ClipCoordToRange(&rect->top, 0, surfaceHeight);
    if (clipped < 0) {
        dstRect.top = -clipped;
    } else if (clipped > 0) {
        return 0;
    }

    clipped = ClipCoordToRange(&rect->bottom, 0, surfaceHeight);
    if (clipped < 0) {
        return 0;
    }

    clippedWidth = rect->right - rect->left;
    clippedHeight = rect->bottom - rect->top;
    if (clippedWidth <= 0 || clippedHeight <= 0) {
        return 0;
    }

    // Retail reuses the image parameter's stack home for the created image.
    if (image == 0) {
        image = Create();
        if (image == 0) {
            return 0;
        }

        SetSize(image, (short)(clippedHeight), (short)(clippedWidth));
        image->pixels = malloc((size_t)(image->pixelCount) * sizeof(unsigned short));
    }

    dstBytes = (unsigned char*)(image->pixels) + (originalWidth * dstRect.top + dstRect.left) * sizeof(unsigned short);
    srcBytes = surfacePixels + (pitchWords * rect->top + rect->left) * sizeof(unsigned short);
    for (row = 0; row < clippedHeight; ++row) {
        memcpy(dstBytes, srcBytes, clippedWidth * sizeof(unsigned short));
        dstBytes += originalWidth * sizeof(unsigned short);
        srcBytes += pitchWords * sizeof(unsigned short);
    }

    return image;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-set-half-res-adjust-mode
 * @recoil-artifact defines .text recoil:function:0x4a71c0: zVideo::SetHalfResAdjustMode.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: update the half-resolution adjustment mode when the current
 * surface configuration allows it.
 */
int __fastcall SetHalfResAdjustMode(int mode)
{
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
        g_zVideo_pfnBltPrimaryToSwRectDirect(0, 0);
    }

    return previousMode;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-get-primary-surface-rect-scratch
 * @recoil-artifact defines .text recoil:function:0x4a7200: zVideo::GetPrimarySurfaceRectScratch.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: updates the reusable primary-surface rectangle dimensions and
 * returns its address.
 *
 * Evidence: BN reads g_zVideo_PrimarySurfaceState width/height at 0x632220 and
 * 0x632224, stores them into g_zVideo_PrimarySurfaceRectScratch.right/bottom at
 * 0x56bbd0 and 0x56bbd4, preserves left/top, and returns 0x56bbc8.
 */
zVidRect32* __cdecl GetPrimarySurfaceRectScratch(void)
{
    g_zVideo_PrimarySurfaceRectScratch.right = g_zVideo_PrimarySurfaceState.width;
    g_zVideo_PrimarySurfaceRectScratch.bottom = g_zVideo_PrimarySurfaceState.height;
    return &g_zVideo_PrimarySurfaceRectScratch;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-set-fog-color-from-rgb01
 * @recoil-artifact defines .text recoil:function:0x4a7220: zVideo::SetFogColorFromRgb01.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Data evidence: writes the pending fog-color RGB255 globals at
 * 0x6321d0-0x6321d8.
 * Purpose: scale a normalized fog color into pending 255-space video globals.
 */
void __fastcall SetFogColorFromRgb01(zVideo_ColorRgbFloat* color)
{
    g_zVideo_FogColorPendingR255 = color->r * 255.0f;
    g_zVideo_FogColorPendingG255 = color->g * 255.0f;
    g_zVideo_FogColorPendingB255 = color->b * 255.0f;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-set-pending-fog-target-color-from-rgb01
 * @recoil-artifact defines .text recoil:function:0x4a7250: zVideoSetPendingFogTargetColorFromRgb01.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Data evidence: writes the D3D color-attribute bias globals at
 * 0x6321dc-0x6321e4 and, for non-software renderers, the normalize-channel
 * index at 0x632140.
 * Purpose: scale a normalized fog target color into D3D color-bias globals.
 */
void __fastcall zVideoSetPendingFogTargetColorFromRgb01(zVideo_ColorRgbFloat* color)
{
    g_zVideo_D3DColorAttrBiasR = color->r * 255.0f;
    g_zVideo_D3DColorAttrBiasG = color->g * 255.0f;
    g_zVideo_D3DColorAttrBiasB = color->b * 255.0f;
    if (g_zVideo_RendererType == 0) {
        return;
    }

    if (g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasG) {
        g_zVideo_D3DColorNormalizeChannelIndex = g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasB ? 0 : 2;
        return;
    }

    if (g_zVideo_D3DColorAttrBiasG >= g_zVideo_D3DColorAttrBiasB) {
        g_zVideo_D3DColorNormalizeChannelIndex = 1;
        return;
    }

    g_zVideo_D3DColorNormalizeChannelIndex = g_zVideo_D3DColorAttrBiasR >= g_zVideo_D3DColorAttrBiasB ? 0 : 2;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-set-fog-target-color-from-rgb01
 * @recoil-artifact defines .text recoil:function:0x4a7300: zVideo::SetFogTargetColorFromRgb01.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: writes the target fog-color RGB255 globals at
 * 0x6321e8-0x6321f0.
 * Purpose: scale a normalized fog target color into target 255-space video globals.
 */
void __fastcall SetFogTargetColorFromRgb01(zVideo_ColorRgbFloat* color)
{
    g_zVideo_FogTargetColorR255 = color->r * 255.0f;
    g_zVideo_FogTargetColorG255 = color->g * 255.0f;
    g_zVideo_FogTargetColorB255 = color->b * 255.0f;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-commit-fog-color-if-changed
 * @recoil-artifact defines .text recoil:function:0x4a7330: zVideo::CommitFogColorIfChanged.
 * @recoil-match byte
 *
 * Source file evidence: zVideo.cpp.
 * Data evidence: compares pending fog RGB255 globals at 0x6321d0-0x6321d8
 * with applied fog RGB255 globals at 0x6321f4-0x6321fc, copies pending to
 * applied on change, then tail-jumps through g_zVideo_pfnUpdateFogColor.
 * Purpose: apply pending fog color values and notify the renderer only when they change.
 */
void __cdecl CommitFogColorIfChanged(void)
{
    if (g_zVideo_FogColorAppliedR255 == g_zVideo_FogColorPendingR255
        && g_zVideo_FogColorAppliedG255 == g_zVideo_FogColorPendingG255
        && g_zVideo_FogColorAppliedB255 == g_zVideo_FogColorPendingB255) {
        return;
    }

    g_zVideo_FogColorAppliedR255 = g_zVideo_FogColorPendingR255;
    g_zVideo_FogColorAppliedG255 = g_zVideo_FogColorPendingG255;
    g_zVideo_FogColorAppliedB255 = g_zVideo_FogColorPendingB255;
    g_zVideo_pfnUpdateFogColor();
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-commit-fog-target-color-if-changed
 * @recoil-artifact defines .text recoil:function:0x4a73a0: zVideo::CommitFogTargetColorIfChanged.
 * @recoil-match byte
 *
 * Source file evidence: zVideo.cpp.
 * Data evidence: compares target fog RGB255 globals at 0x6321e8-0x6321f0
 * with applied fog RGB255 globals at 0x6321f4-0x6321fc, copies target to
 * applied on change, then tail-jumps through g_zVideo_pfnUpdateFogColor.
 * Purpose: apply target fog color values and notify the renderer only when they change.
 */
void __cdecl CommitFogTargetColorIfChanged(void)
{
    if (g_zVideo_FogColorAppliedR255 == g_zVideo_FogTargetColorR255
        && g_zVideo_FogColorAppliedG255 == g_zVideo_FogTargetColorG255
        && g_zVideo_FogColorAppliedB255 == g_zVideo_FogTargetColorB255) {
        return;
    }

    g_zVideo_FogColorAppliedR255 = g_zVideo_FogTargetColorR255;
    g_zVideo_FogColorAppliedG255 = g_zVideo_FogTargetColorG255;
    g_zVideo_FogColorAppliedB255 = g_zVideo_FogTargetColorB255;
    g_zVideo_pfnUpdateFogColor();
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-vid-get-selected-hw-api-description-or-default
 * @recoil-artifact defines .text recoil:function:0x4a7410: zVid::GetSelectedHwApiDescriptionOrDefault.
 * @recoil-match byte
 *
 * Purpose: return the selected hardware API description or the default
 * writable fallback string when no hardware API record is selected.
 */
char* __cdecl GetSelectedHwApiDescriptionOrDefault(void)
{
    return g_zVideo_pSelectedHwApiDeviceRecord != 0 ? g_zVideo_pSelectedHwApiDeviceRecord->m_driverDescription
                                                    : g_zVideo_DefaultHwApiDescription;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-vid-get-hw-api-description
 * @recoil-artifact defines .text recoil:function:0x4a7430: zVid::GetHwApiDescription.
 * @recoil-match byte
 *
 * Purpose: provide the recovered zVid::GetHwApiDescription behavior.
 */
char* __fastcall GetHwApiDescription(int index)
{
    return g_zVideo_HwApiDeviceTable[index].m_driverDescription;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-vid-get-hw-api-driver-name
 * @recoil-artifact defines .text recoil:function:0x4a7450: zVid::GetHwApiDriverName.
 * @recoil-match byte
 *
 * Purpose: provide the recovered zVid::GetHwApiDriverName behavior.
 */
char* __fastcall GetHwApiDriverName(int index)
{
    return g_zVideo_HwApiDeviceTable[index].m_driverName;
}

/**
 * @recoil-anchor recoil:anchor:zvid.accepted-hardware-renderer-count
 * @recoil-artifact defines .text recoil:function:0x4a7470: Public renderer-count accessor.
 * @recoil-match byte
 *
 * Purpose: expose the backend's accepted renderer count through the video API.
 * Retail callers use this entry, which tail-calls the distinct cached getter.
 * The descriptive spelling follows the existing API family; it is not recovered
 * original spelling. Neighboring API order supports this implementation placement.
 */
int __cdecl GetAcceptedHardwareRendererCount(void)
{
    return GetAcceptedHardwareRendererCountCached();
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-vid-get-accepted-direct-draw-device-count
 * @recoil-artifact defines .text recoil:function:0x4a7480: zVid::GetAcceptedDirectDrawDeviceCount.
 * @recoil-match byte
 *
 * Retail 0x4a7480 tail-calls the distinct cached DirectDraw count at 0x4a9900.
 * Purpose: return the accepted DirectDraw hardware API device count.
 */
int __cdecl GetAcceptedDirectDrawDeviceCount(void)
{
    return GetAcceptedDirectDrawDeviceCountCached();
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-select-hw-api-device-or-fallback
 * @recoil-artifact defines .text recoil:function:0x4a7490: zVideo::SelectHwApiDeviceOrFallback.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: select the persisted hardware API device or fall back to the
 * software renderer path.
 *
 * Evidence: BN branches on hwApiIndex == -1; nonnegative indexes tail through
 * CommitHwApiDeviceSelection and return one, while fallback binds software
 * fullscreen dispatch, points g_zVideo_pSelectedHwApiDeviceRecord at table
 * entry zero, clears g_zVideo_pSelectedD3DDeviceInfo, and returns zero.
 */
int __fastcall SelectHwApiDeviceOrFallback(int hwApiIndex)
{
    if (hwApiIndex != -1) {
        CommitHwApiDeviceSelection(hwApiIndex);
        return 1;
    }

    BindRendererDispatch(0, 1);
    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[0];
    g_zVideo_pSelectedD3DDeviceInfo = 0;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-d3-d-scene-enter
 * @recoil-artifact defines .text recoil:function:0x4a74d0: zVideoD3D::SceneEnter.
 * @recoil-match byte
 *
 * Data evidence: BN reads g_zVideo_D3DSceneDepth at 0x632148, calls
 * zVideo_dd3d::BeginSceneAndFlushPendingRenderStates only when depth is not
 * positive, then increments the same zero-initialized int32.
 * Purpose: provide the recovered zVideoD3D::SceneEnter behavior.
 */
int __cdecl SceneEnter(void)
{
    if (g_zVideo_D3DSceneDepth <= 0) {
        BeginSceneAndFlushPendingRenderStates();
        ++g_zVideo_D3DSceneDepth;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-d3-d-scene-leave
 * @recoil-artifact defines .text recoil:function:0x4a74f0: zVideoD3D::SceneLeave.
 * @recoil-match byte
 *
 * Data evidence: BN reads g_zVideo_D3DSceneDepth at 0x632148, calls
 * zVideo_dd3d::EndScene only for the final active scene, decrements the stored
 * depth, and returns zero for all depth states.
 * Purpose: provide the recovered zVideoD3D::SceneLeave behavior.
 */
int __cdecl SceneLeave(void)
{
    int depth = g_zVideo_D3DSceneDepth;
    if (depth > 0) {
        if (depth <= 1) {
            EndScene();
            depth = g_zVideo_D3DSceneDepth;
        }
        g_zVideo_D3DSceneDepth = depth - 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-at-exit-release-all-interfaces-and-surfaces
 * @recoil-artifact defines .text recoil:function:0x4a7520: zVideo::AtExitReleaseAllInterfacesAndSurfaces.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: release tracked DirectDraw and Direct3D interfaces from the CRT
 * atexit hook registered by zVideo::ModuleInit.
 *
 * Evidence: BN is a tail jump to zVideo_dd::ReleaseAllInterfacesAndSurfaces
 * and the function is passed directly to atexit by zVideo::ModuleInit.
 */
void __cdecl AtExitReleaseAllInterfacesAndSurfaces(void)
{
    ReleaseAllInterfacesAndSurfaces();
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-module-init
 * @recoil-artifact defines .text recoil:function:0x4a7530: zVideo::ModuleInit.
 *
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: initialize zVideo global defaults, software renderer dispatch,
 * DirectDraw device enumeration, and the process-exit teardown hook.
 *
 * Evidence: BN clears the zVideo global state block, seeds pixel-pack defaults,
 * binds the software fullscreen dispatch, runs DirectDraw startup enumeration,
 * registers zVideo::AtExitReleaseAllInterfacesAndSurfaces with atexit, and
 * returns zero. Caller 0x4a7530 uses one rep stosd span for the zVideo runtime
 * block. The
 * rebuilt link layout may place CRT/provider globals inside that absolute span,
 * so the source reset names only authored zVideo runtime storage.
 */
int __cdecl ModuleInit(void)
{
    g_zVideo_FrameTick = 0;
    memset(&g_zVideo_GlobalStateStorage, 0, sizeof(g_zVideo_GlobalStateStorage));

    gVideo_resolutionMenuValid = 0;
    g_zVideo_PaletteBrightnessLevel = 4;
    g_zVideo_ClearColorPacked16 = 0;
    g_zVideo_FullscreenOption = 1;
    g_zVideo_CachedFogModeLightState = 0;
    g_zVideo_PendingDitherEnable = -1;
    g_zVideo_InverseZTolerancePending = 0.0199999996f;
    g_zVideo_D3DAppendFanCloseVertexPending = 0;

    PixelPackSetupFromMasks(0, 0, 0, 0, 0, 0);
    TexturePixelPackSetupFromMasks(4, 4, 4, 4, 0xf000, 0x0f00, 0x00f0, 0x000f);
    BindRendererDispatch(0, 1);
    StartupEnumerateAndDefaultSelect();
    atexit(AtExitReleaseAllInterfacesAndSurfaces);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-init-video-system
 * @recoil-artifact defines .text recoil:function:0x4a75f0: zVideo::InitVideoSystem.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_init.c.
 * Purpose: open the requested renderer video mode, initialize backend state,
 * seed hardware texture defaults, and refresh cached client coordinates.
 *
 * Evidence: BN rejects double initialization, stores g_zVideo_hWnd, resets
 * g_zVideo_FrameTick, binds renderer dispatch, opens the mode, hides the cursor,
 * marks initialization, calls zVideo::SetVideoMode, creates the hardware
 * default texture record with a null texture name and zero flags, seeds
 * quad-batch specular values for hardware renderers, and calls
 */
int __fastcall InitVideoSystem(HWND hWnd, int rendererBackend, int fullscreen, int modeIndex)
{
    int openResult;
    int setModeResult;
    if (g_zVideo_IsInitialized != 0) {
        return 0x5a560001;
    }

    g_zVideo_hWnd = hWnd;
    g_zVideo_FrameTick = 0;
    BindRendererDispatch(rendererBackend, fullscreen);

    openResult = g_zVideo_pfnOpenVideoMode(modeIndex);
    if (openResult != 0) {
        ReportOld(0x800, g_zVideo_SourceFile_ZvidInitC, 0x7a, g_zVideo_InitFailOpenVideoModeMsg);
        return openResult;
    }

    ShowCursor(FALSE);
    g_zVideo_IsInitialized = 1;
    setModeResult = SetVideoMode(modeIndex);
    if (setModeResult != 0) {
        ReportOld(0x800, g_zVideo_SourceFile_ZvidInitC, 0x86, g_zVideo_InitFailSetModeMsg);
        ShutdownVideoSystem();
        return setModeResult;
    }

    if (g_zVideo_RendererType != 0) {
        g_zVideo_DefaultTextureRecord = g_zVideo_pfnCreateTextureRecord(0, &g_zVideo_DefaultTextureImage, 0, 0, 0);
        g_zVideo_QuadBatchCount = 0;
        {
            int itemIndex;
            for (itemIndex = 0; itemIndex < 16; ++itemIndex) {
                zVideo_QuadBatchItemPartial* item = &g_zVideo_QuadBatchItemsBase[itemIndex];
                item->vertices[3].specular = 0xff000000;
                item->vertices[2].specular = 0xff000000;
                item->vertices[1].specular = 0xff000000;
                item->vertices[0].specular = 0xff000000;
            }
        }
    }

    UpdateCachedClientRectScreenCoords();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-update-cached-client-rect-screen-coords
 * @recoil-artifact defines .text recoil:function:0x4a7700: zVideo::UpdateCachedClientRectScreenCoords.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: Battlesport/zVideo.cpp.
 * Purpose: cache the client rectangle in screen coordinates for the active
 * zVideo window.
 *
 * Evidence: BN calls GetClientRect with g_zVideo_hWnd and
 * g_zVideo_CachedClientRectScreen, then maps the top-left and bottom-right
 * points with ClientToScreen.
 */
int __fastcall UpdateCachedClientRectScreenCoords(void)
{
    GetClientRect(g_zVideo_hWnd, &g_zVideo_CachedClientRectScreen);
    ClientToScreen(g_zVideo_hWnd, (POINT*)(&g_zVideo_CachedClientRectScreen.left));
    ClientToScreen(g_zVideo_hWnd, (POINT*)(&g_zVideo_CachedClientRectScreen.right));
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-shutdown-video-system
 * @recoil-artifact defines .text recoil:function:0x4a7740: zVideo::ShutdownVideoSystem.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: Battlesport/zVideo.cpp.
 * Purpose: shut down the active zVideo backend and restore cursor visibility.
 *
 * Evidence: BN checks g_zVideo_IsInitialized, clears it on the active path,
 * calls g_zVideo_pfnShutdownVideoSystem, calls ShowCursor(TRUE), and returns a
 * zVideo status code.
 */
int __cdecl ShutdownVideoSystem(void)
{
    if (g_zVideo_IsInitialized == 0) {
        return 0x5a560000;
    }

    g_zVideo_IsInitialized = 0;
    g_zVideo_pfnShutdownVideoSystem();
    ShowCursor(TRUE);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-restore-iconic-fullscreen-window-if-needed
 * @recoil-artifact defines .text recoil:function:0x4a7770: zVideoRestoreIconicFullscreenWindowIfNeeded.
 * @recoil-match byte
 *
 * Purpose: provide the recovered zVideoRestoreIconicFullscreenWindowIfNeeded behavior.
 */
void __cdecl zVideoRestoreIconicFullscreenWindowIfNeeded(void)
{
    if (g_zVideo_IsInitialized != 0 && g_zVideo_FullscreenOption != 0 && IsIconic(g_zVideo_hWnd) != 0) {
        OpenIcon(g_zVideo_hWnd);
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-bind-renderer-dispatch
 * @recoil-artifact defines .text recoil:function:0x4a77a0: zVideo::BindRendererDispatch.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: bind renderer-specific zVideo dispatch functions and fullscreen state.
 */
void __fastcall BindRendererDispatch(int rendererType, int fullscreenOption)
{
    SetRendererTypeAndActivePath(rendererType);
    g_zVideo_FullscreenOption = fullscreenOption;
    g_zVideo_pfnOpenVideoMode = OpenVideoMode;
    g_zVideo_pfnShutdownVideoSystem = (zVideo_ShutdownVideoSystemProc)(zVideoddShutdownVideoSystem);
    g_zVideo_pfnPaletteSetEntries = PaletteSetEntries;
    g_zVideo_pfnSetVideoMode = zVideoddSetVideoMode;
    g_zVideo_pfnAdjustSurfaces = zVideodd3dPresentDisplayModeSurface;
    if (g_zVideo_RendererType != 1) {
        g_zVideo_pfnAdjustSurfaces = zVideoddPresentDisplayModeSurface;
    }
    g_zVideo_pfnLockSurfaceState = LockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = UnlockSurfaceState;
    g_zVideo_pfnClearZBufferRect = ZBufferDepthFillRect;
    g_zVideo_pfnClearSwSurfaceAndZBuffer = ClearSwBackbufferAndZBufferRects;
    g_zVideo_pfnClearStateSurfaceAndZBuffer = ClearScreenAndZBufferRect;
    g_zVideo_pfnUpdateFogColor = UpdateFogColor;
    g_zVideo_pfnQueryTextureMemoryBytes = QueryTextureMemoryBytes;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = QueryDeviceVideoMemoryBytes;
    g_zVideo_pfnBltSwToPrimaryRectDirect = BltSwToPrimaryRectDirect;
    g_zVideo_pfnBltPrimaryToSwRectDirect = BltPrimaryToSwRectDirect;
    g_zVideo_pfnBltSwToPrimaryRect = BltSwToPrimaryRect;
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = GetHwApiDeviceFeatureFlags;
    g_zVideo_pfnImageUploadPixelsToSurface = ImageUploadPixelsToSurface;
    g_zVideo_pfnImageReleaseSurface = ImageReleaseSurface;
    g_zVideo_pfnCreateTextureRecord = CreateTextureRecord;
    g_zVideo_pfnTextureRecordLockUploadSurface = TextureRecordLockUploadSurface;
    g_zVideo_pfnTextureRecordUnlockUploadSurface = TextureRecordUnlockUploadSurface;
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef = TextureRecordReleaseUploadSurfaceRef;
    g_zVideo_pfnTextureRecordFinalizeUpload = TextureRecordFinalizeUpload;
    g_zVideo_pfnTextureRecordDestroy = TextureRecordDestroy;
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = ReturnOnlyStub;
    g_zVideo_pfnImageLazyCreateVideoMemorySurface = ImageLazyCreateVideoMemorySurface;
    g_zVideo_pfnImageEnsureSurfaceForCurrentDevice = ImageEnsureSurfaceForCurrentDevice;
    g_zVideo_pfnSetFogEnable = SetFogEnable;
    g_zVideo_pfnSetFogStart = SetFogStart;
    g_zVideo_pfnSetFogEnd = SetFogEnd;
    g_zVideo_pfnApplyFogStateFromGlobals = ApplyFogStateFromGlobals;
    g_zVideo_pfnSubmitPolyFlatColor16 = SubmitPolyFlatColor16;
    g_zVideo_pfnSubmitPolyGouraudColor16 = SubmitPolyGouraudColor16;
    g_zVideo_pfnSubmitPolyColorAttr = SubmitPolyColorAttr;
    g_zVideo_pfnSubmitPolyRenderClass = SubmitPolyRenderClass;
    g_zVideo_pfnSubmitPolygon = SubmitPolygon;
    g_zVideo_pfnSubmitPolygonLit = SubmitPolygonLit;
    g_zVideo_pfnDrawPointColor16 = DrawPointColor16;
    g_zVideo_pfnFlushSortedPolys = FlushSortedPolys;
    g_zVideo_pfnFlushOverwritePolys = FlushOverwritePolys;
    g_zVideo_pfnFlushQuadBatch = FlushQuadBatch;

    if (g_zVideo_pSelectedHwApiDeviceRecord != 0 && g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0) {
        g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-init-set-surface-geometry-from-mode-index
 * @recoil-artifact defines .text recoil:function:0x4a7990: zVideo::InitSetSurfaceGeometryFromModeIndex.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: initializes cached display, primary, and software surface geometry
 * for the selected video mode index.
 *
 * Evidence: BN switches over mode indices 2..7, writes
 * g_zVideo_UseHalfResBackbuffer plus the width/height fields of the three
 * zVideo_SurfaceState records, clears gVideo_resolutionMenuValid for invalid
 * indices, and stores the legacy computed display bpp value at 0x632150.
 */
void __fastcall InitSetSurfaceGeometryFromModeIndex(int modeIndex)
{
    int bitsPerPixel;
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
    bitsPerPixel = modeIndex <= 1 ? 1 : 0;
    --bitsPerPixel;
    bitsPerPixel &= 8;
    bitsPerPixel += 8;
    g_zVideo_DisplayModeBpp = bitsPerPixel;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-set-video-mode
 * @recoil-artifact defines .text recoil:function:0x4a7af0: zVideo::SetVideoMode.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_init.c.
 * Purpose: apply cached geometry for a requested video mode and forward the
 * mode switch through the active renderer backend.
 *
 * Evidence: BN checks g_zVideo_IsInitialized, calls
 * InitSetSurfaceGeometryFromModeIndex, then dispatches through
 * g_zVideo_pfnSetVideoMode with the original mode index.
 */
int __fastcall SetVideoMode(int modeIndex)
{
    if (g_zVideo_IsInitialized == 0) {
        return 0x5a560000;
    }

    InitSetSurfaceGeometryFromModeIndex(modeIndex);
    return g_zVideo_pfnSetVideoMode(modeIndex);
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-exchange-clear-screen-buffer-enabled
 * @recoil-artifact defines .text recoil:function:0x4a7b20: zVideo::ExchangeClearScreenBufferEnabled.
 * @recoil-match byte
 *
 * Purpose: Swaps the clear-screen-buffer flag and returns the previous value.
 */
int __fastcall ExchangeClearScreenBufferEnabled(int enable)
{
    const int previous = g_zVideo_ClearScreenBufferEnabled;
    g_zVideo_ClearScreenBufferEnabled = enable;
    return previous;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-init.z-video-get-clear-screen-buffer-enabled
 * @recoil-artifact defines .text recoil:function:0x4a7b30: zVideo::GetClearScreenBufferEnabled.
 * @recoil-match byte
 *
 * Purpose: Returns the current clear-screen-buffer flag.
 */
int __cdecl GetClearScreenBufferEnabled(void)
{
    return g_zVideo_ClearScreenBufferEnabled;
}
