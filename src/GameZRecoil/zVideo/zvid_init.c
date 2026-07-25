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
 * Recovered literal-backed zvid_init.c physical contribution
 * [0x4a6b40, 0x4a7b40). Definitions remain in natural retail source order;
 * compiler-emitted switch lowering belongs to Init_SetSurfaceGeometryFromModeIndex.
 */

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

} // namespace zVideo

namespace zVideo_dd3d {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
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

} // namespace zVideo_dd3d

namespace zVideo_dd3d {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
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

} // namespace zVideo_dd3d

/**
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

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

} // namespace zVideo

namespace zVideo {
/**
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

} // namespace zVideo

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

} // namespace zVideo

/**
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
 * Purpose: round RGB float channels and pack them through the active 16-bit pixel format.
 */
unsigned short __fastcall zVid_PackColorRgbFloats(
    zVideo_ColorRgbFloat *color
) {
    unsigned short packed;

    packed = (unsigned short)(int)(color->r + 0.5f);
    packed &= (unsigned short)(g_zVideo_PixelPack.rMaskShifted);
    packed = (unsigned short)((int)(packed) << g_zVideo_PixelPack.packedBase);
    packed = (unsigned short)(packed |
        (((int)(color->g + 0.5f) & g_zVideo_PixelPack.gMaskShifted) << g_zVideo_PixelPack.sumMinus8));
    packed = (unsigned short)(packed |
        ((unsigned short)(int)(color->b + 0.5f) >> g_zVideo_PixelPack.bShiftTo8));
    return packed;
}

namespace zVideo {
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

} // namespace zVideo

/**
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

namespace zVideo_buff {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zImage/zvid_buff.c.
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

} // namespace zVideo_buff

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

} // namespace zVideo

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

} // namespace zVideo

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

} // namespace zVideo

/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
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

namespace zVideo {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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

} // namespace zVideo

namespace zVideo {
/**
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

} // namespace zVideo

namespace zVideo {
/**
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

} // namespace zVideo

namespace zVid {
/**
 * Purpose: return the selected hardware API description or the default
 * writable fallback string when no hardware API record is selected.
 */
char *GetSelectedHwApiDescriptionOrDefault() {
    return g_zVideo_pSelectedHwApiDeviceRecord != 0
               ? g_zVideo_pSelectedHwApiDeviceRecord->m_driverDescription
               : g_zVideo_DefaultHwApiDescription;
}

} // namespace zVid

namespace zVid {
/**
 * Purpose: provide the recovered zVid::GetHwApiDescription behavior.
 */
char *__fastcall GetHwApiDescription(
    int index
) {
    return g_zVideo_HwApiDeviceTable[index].m_driverDescription;
}

} // namespace zVid

namespace zVid {
/**
 * Purpose: provide the recovered zVid::GetHwApiDriverName behavior.
 */
char *__fastcall GetHwApiDriverName(
    int index
) {
    return g_zVideo_HwApiDeviceTable[index].m_driverName;
}

} // namespace zVid

namespace zVid {
/**
 * Original file evidence: BN comment identifies this as the public zVid thunk
 * in GameZRecoil/zVideo_dd.cpp, tail-jumping to the zvid_dd.c cached accessor.
 * Purpose: return the accepted DirectDraw hardware API device count.
 */
int GetAcceptedDirectDrawDeviceCount() {
    return zVideo_dd::GetAcceptedDirectDrawDeviceCountCached();
}

} // namespace zVid

namespace zVideo {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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

} // namespace zVideo

namespace zVideoD3D {
/**
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

} // namespace zVideoD3D

namespace zVideoD3D {
/**
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

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: release tracked DirectDraw and Direct3D interfaces from the CRT
 * atexit hook registered by zVideo::ModuleInit.
 *
 * Evidence: BN is a tail jump to zVideo_dd::ReleaseAllInterfacesAndSurfaces
 * and the function is passed directly to atexit by zVideo::ModuleInit.
 */
void __cdecl AtExitReleaseAllInterfacesAndSurfaces() {
    zVideo_dd::ReleaseAllInterfacesAndSurfaces();
}

} // namespace zVideo

namespace zVideo {
/**
 * Recovered local helper: zVideo_ResetModuleRuntimeState.
 * Original-source helper evidence: no standalone retail function is present;
 * caller 0x4a7530 uses one rep stosd span for the zVideo runtime block. The
 * rebuilt link layout may place CRT/provider globals inside that absolute span,
 * so the source reset names only authored zVideo runtime storage.
 * Purpose: restore zVideo module globals to startup-zero state before seeding
 * ModuleInit defaults.
 */
static void zVideo_ResetModuleRuntimeState() {
    g_zVideo_RendererType = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_FrameTick = 0;
    g_zVideo_pActiveViewContext = 0;
    g_zVideo_pActiveProjectionViewContext = 0;
    memset(&g_zVideo_ActiveViewVariantTag, 0, sizeof(g_zVideo_ActiveViewVariantTag));
    g_zVideo_ProjectClipLeft = 0.0f;
    g_zVideo_ProjectClipTop = 0.0f;
    g_zVideo_ProjectClipRight = 0.0f;
    g_zVideo_ProjectClipBottom = 0.0f;
    gVideo_resolutionMenuValid = 0;
    g_zVideo_ClearColorPacked16 = 0;
    g_zVideo_ClearScreenBufferEnabled = 0;
    g_zVid_CachedClientRectUpdateMask = 0;
    g_zVideo_IsInitialized = 0;
    g_zVideo_AdjustSurfacesDisableGate = 0;
    g_zVideo_FullscreenOption = 0;
    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_HalfResAdjustMode = 0;
    g_zVideo_CachedFogModeLightState = 0;
    g_zVideo_CachedFogEnableRenderState = 0;
    g_zVideo_CachedFogStartLightStateValue = 0.0f;
    g_zVideo_CachedFogEndLightStateValue = 0.0f;
    g_zVideo_D3DColorNormalizeChannelIndex = 0;
    g_zVideo_FogColorPendingR255 = 0.0f;
    g_zVideo_FogColorPendingG255 = 0.0f;
    g_zVideo_FogColorPendingB255 = 0.0f;
    g_zVideo_D3DColorAttrBiasR = 0.0f;
    g_zVideo_D3DColorAttrBiasG = 0.0f;
    g_zVideo_D3DColorAttrBiasB = 0.0f;
    g_zVideo_FogTargetColorR255 = 0.0f;
    g_zVideo_FogTargetColorG255 = 0.0f;
    g_zVideo_FogTargetColorB255 = 0.0f;
    g_zVideo_FogColorAppliedR255 = 0.0f;
    g_zVideo_FogColorAppliedG255 = 0.0f;
    g_zVideo_FogColorAppliedB255 = 0.0f;
    g_zVideo_PendingDitherEnable = 0;
    g_zVideo_InverseZTolerancePending = 0.0f;
    g_zVideo_D3DAppendFanCloseVertexPending = 0;
    g_zVideo_PendingWireframeState = 0;
    g_zVideo_D3DSceneDepth = 0;
    g_zVid_AcceptedHardwareRendererCount = 0;
    g_zVideo_NumAcceptedDirectDrawDevices = 0;
    g_zVideo_DirectDrawEnumOrdinal = 0;
    memset(&g_zVideo_DDrawCapsHal, 0, sizeof(g_zVideo_DDrawCapsHal));
    memset(&g_zVideo_DDrawCapsHel, 0, sizeof(g_zVideo_DDrawCapsHel));
    g_zVideo_DefaultTextureRecord = 0;
    memset(g_zVideo_PalettePathBuffer, 0, sizeof(g_zVideo_PalettePathBuffer));
    g_zVideo_PaletteBrightnessLevel = 0;
    memset(g_zVideo_PaletteFileEntries, 0, sizeof(g_zVideo_PaletteFileEntries));
    memset(g_zVideo_SystemPaletteEntries, 0, sizeof(g_zVideo_SystemPaletteEntries));
    g_zVideo_pfnOpenVideoMode = 0;
    g_zVideo_pfnShutdownVideoSystem = 0;
    g_zVideo_pfnPaletteSetEntries = 0;
    g_zVideo_pfnSetVideoMode = 0;
    g_zVideo_pfnAdjustSurfaces = 0;
    g_zVideo_pfnLockSurfaceState = 0;
    g_zVideo_pfnUnlockSurfaceState = 0;
    g_zVideo_pfnClearZBufferRect = 0;
    g_zVideo_pfnClearSwSurfaceAndZBuffer = 0;
    g_zVideo_pfnClearStateSurfaceAndZBuffer = 0;
    g_zVideo_pfnUpdateFogColor = 0;
    g_zVideo_pfnQueryDeviceVideoMemoryBytes = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = 0;
    g_zVideo_pfnBltPrimaryToSwRectDirect = 0;
    g_zVideo_pfnBltSwToPrimaryRect = 0;
    g_zVideo_pfnGetHwApiDeviceFeatureFlags = 0;
    g_zVideo_pfnImageUploadPixelsToSurface = 0;
    g_zVideo_pfnImageReleaseSurface = 0;
    g_zVideo_pfnCreateTextureRecord = 0;
    g_zVideo_pfnTextureRecordLockUploadSurface = 0;
    g_zVideo_pfnTextureRecordUnlockUploadSurface = 0;
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef = 0;
    g_zVideo_pfnTextureRecordFinalizeUpload = 0;
    g_zVideo_pfnTextureRecordDestroy = 0;
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = 0;
    g_zVideo_pfnImageLazyCreateVideoMemorySurface = 0;
    g_zVideo_pfnImageEnsureSurfaceForCurrentDevice = 0;
    g_zVideo_pfnSetFogEnable = 0;
    g_zVideo_pfnSetFogStart = 0;
    g_zVideo_pfnSetFogEnd = 0;
    g_zVideo_pfnApplyFogStateFromGlobals = 0;
    g_zVideo_pfnSubmitPolyFlatColor16 = 0;
    g_zVideo_pfnSubmitPolyGouraudColor16 = 0;
    g_zVideo_pfnSubmitPolyColorAttr = 0;
    g_zVideo_pfnSubmitPolyRenderClass = 0;
    g_zVideo_pfnSubmitPolygon = 0;
    g_zVideo_pfnSubmitPolygonLit = 0;
    g_zVideo_pfnDrawPointColor16 = 0;
    g_zVideo_pfnFlushSortedPolys = 0;
    g_zVideo_pfnFlushOverwritePolys = 0;
    g_zVideo_pfnFlushQuadBatch = 0;
    memset(g_zVideo_HwApiDeviceTable, 0, sizeof(g_zVideo_HwApiDeviceTable));
    g_zVideo_pSelectedHwApiDeviceRecord = 0;
    g_zVideo_pSelectedD3DDeviceInfo = 0;
    memset(&g_zVideo_D3DHalDeviceDesc, 0, sizeof(g_zVideo_D3DHalDeviceDesc));
    memset(&g_zVideo_D3DHelDeviceDesc, 0, sizeof(g_zVideo_D3DHelDeviceDesc));
    g_zVideo_D3DMaterialHandle = 0;
    g_zVideo_QuadBatchCount = 0;
    memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    memset(g_zVideo_D3DSubmitTempVertices, 0, sizeof(g_zVideo_D3DSubmitTempVertices));
    memset(g_zVideo_SortedPolyDrawOrder, 0, sizeof(g_zVideo_SortedPolyDrawOrder));
    memset(g_zVideo_SortedPolyQueueBase, 0, sizeof(g_zVideo_SortedPolyQueueBase));
    memset(g_zVideo_OverwriteQueueBase, 0, sizeof(g_zVideo_OverwriteQueueBase));
    g_zVideo_SortedPolyQueueCount = 0;
    g_zVideo_OverwriteQueueCount = 0;
    memset(&g_zVideo_D3DRenderStateCache, 0, sizeof(g_zVideo_D3DRenderStateCache));
    g_zVideo_pD3DMaterial2 = 0;
    g_zVideo_pD3DViewport2 = 0;
    g_zVideo_pD3DDevice = 0;
    g_zVideo_pD3D2 = 0;
    g_zVideo_pClipper = 0;
    g_zVideo_pDirectDraw2 = 0;
    g_zVideo_pZBufferSurface = 0;
    g_zVideo_pZBufferAttachSurface = 0;
    g_zVideo_pPageUnlockSurface = 0;
    g_zVideo_pSurfaceLockVerifier = 0;
    g_zVideo_SurfaceLockVerifyContext = 0;
    g_zVideo_SurfaceLockVerifyFlags = 0;
    memset(&g_zVideo_SwSurfaceState, 0, sizeof(g_zVideo_SwSurfaceState));
    memset(&g_zVideo_PrimarySurfaceState, 0, sizeof(g_zVideo_PrimarySurfaceState));
    memset(&g_zVideo_DisplayModeSurfaceState, 0, sizeof(g_zVideo_DisplayModeSurfaceState));
    memset(&g_zVideo_SurfaceStateSwapScratch, 0, sizeof(g_zVideo_SurfaceStateSwapScratch));
    memset(&g_zVideo_PrimarySurfaceRectScratch, 0, sizeof(g_zVideo_PrimarySurfaceRectScratch));
    g_zVideo_DisplayModeBpp = 0;
    g_zVid_NoiseByteTableSize = 0;
    g_zVid_NoiseByteTable = 0;
    g_zVideo_FxPass3_ScratchPixels16 = 0;
    g_zVideo_FxSurfacePixels16 = 0;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    g_zVideo_FxPass3_ScratchOffsetX = 0;
    g_zVideo_FxPass3_ScratchOffsetY = 0;
    g_zVideo_FxPass3_ClipMinX = 0;
    g_zVideo_FxPass3_ClipMinY = 0;
    g_zVideo_FxPass3_ClipMaxX = 0;
    g_zVideo_FxPass3_ClipMaxY = 0;
    g_zVideo_pDDPalette = 0;
    g_zVideo_hWnd = 0;
    memset(&g_zVideo_CachedClientRectScreen, 0, sizeof(g_zVideo_CachedClientRectScreen));
}

} // namespace zVideo

namespace zVideo {
/**
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: initialize zVideo global defaults, software renderer dispatch,
 * DirectDraw device enumeration, and the process-exit teardown hook.
 *
 * Evidence: BN clears the zVideo global state block, seeds pixel-pack defaults,
 * binds the software fullscreen dispatch, runs DirectDraw startup enumeration,
 * registers zVideo::AtExitReleaseAllInterfacesAndSurfaces with atexit, and
 * returns zero.
 */
int ModuleInit() {
    zVideo_ResetModuleRuntimeState();

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

} // namespace zVideo

namespace zVideo {
/**
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

} // namespace zVideo

namespace zVideo {
/**
 * Provisional source-placement hypothesis: Battlesport/zVideo.cpp.
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

} // namespace zVideo

namespace zVideo {
/**
 * Provisional source-placement hypothesis: Battlesport/zVideo.cpp.
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

} // namespace zVideo

/**
 * Purpose: provide the recovered zVideo_RestoreIconicFullscreenWindowIfNeeded behavior.
 */
void zVideo_RestoreIconicFullscreenWindowIfNeeded() {
    if (g_zVideo_IsInitialized != 0 && g_zVideo_FullscreenOption != 0 &&
        IsIconic(g_zVideo_hWnd) != 0) {
        OpenIcon(g_zVideo_hWnd);
    }
}

namespace zVideo {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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

} // namespace zVideo

namespace zVideo {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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

} // namespace zVideo

namespace zVideo {
/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_init.c.
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

} // namespace zVideo

namespace zVideo {
/**
 * Purpose: Swaps the clear-screen-buffer flag and returns the previous value.
 */
int __fastcall ExchangeClearScreenBufferEnabled(
    int enable
) {
    const int previous = g_zVideo_ClearScreenBufferEnabled;
    g_zVideo_ClearScreenBufferEnabled = enable;
    return previous;
}

} // namespace zVideo

namespace zVideo {
/**
 * Purpose: Returns the current clear-screen-buffer flag.
 */
int GetClearScreenBufferEnabled() {
    return g_zVideo_ClearScreenBufferEnabled;
}

} // namespace zVideo
