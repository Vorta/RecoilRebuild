// zRender compilation unit between zreader.cpp and zrndr_init.c, inferred from
// the retail object boundary [0x48d340, 0x48f500): its .rdata pooled
// constants [0x4d2d88, 0x4d2db8) and its .bss statics [0x56b190, 0x56b1e8) are
// separate from the neighbouring zRender objects. Original filename
// unresolved; zrndr_fx.c is a provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zRender/zrndr.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zclass.h"

#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/*
 * 0x48da60 reads the two scratch offsets, four clip bounds, active FX-surface
 * descriptor, and scratch pointer as one zVideo pass-3 scratch copy data set.
 * 0x48daf0 writes the clip bounds for every pass and writes the scratch offsets
 * only when it switches from the direct scatter path to the clipped helper path.
 * This documents the local source shape only; the complete zVideo data owner is
 * broader than this slice and remains a direct-review data-gate decision.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-scratchoffsetx
 * @recoil-artifact defines .data recoil:data:0x56b190: g_zVideo_FxPass3_ScratchOffsetX.
 * Data owner evidence: zVideo::FxPass3ApplyToCurrentSurface writes the center
 * X bias before clipped scatter calls; BN assembly for 0x48da60 loads it once
 * and applies it to both the destination delta in ECX and the source X stack
 * delta before clip tests.
 * Purpose: cache the pass-3 clipped copy X offset.
 */
int g_zVideo_FxPass3_ScratchOffsetX;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-scratchoffsety
 * @recoil-artifact defines .data recoil:data:0x56b194: g_zVideo_FxPass3_ScratchOffsetY.
 * Data owner evidence: zVideo::FxPass3ApplyToCurrentSurface writes the center
 * Y bias before clipped scatter calls; BN assembly for 0x48da60 loads it once
 * and applies it to both the destination delta in EDX and the source Y stack
 * delta before clip tests.
 * Purpose: cache the pass-3 clipped copy Y offset.
 */
int g_zVideo_FxPass3_ScratchOffsetY;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipminx
 * @recoil-artifact defines .data recoil:data:0x56b1a0: g_zVideo_FxPass3_ClipMinX.
 * Data owner evidence: zVideo::FxPass3ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper tests source
 * and destination X coordinates against it as an inclusive lower bound.
 * Purpose: cache the inclusive minimum X clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMinX;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipminy
 * @recoil-artifact defines .data recoil:data:0x56b1a4: g_zVideo_FxPass3_ClipMinY.
 * Data owner evidence: zVideo::FxPass3ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper tests source
 * and destination Y coordinates against it as an inclusive lower bound.
 * Purpose: cache the inclusive minimum Y clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMinY;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipmaxx
 * @recoil-artifact defines .data recoil:data:0x56b1a8: g_zVideo_FxPass3_ClipMaxX.
 * Data owner evidence: zVideo::FxPass3ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper treats this as
 * the exclusive maximum X edge.
 * Purpose: cache the exclusive maximum X clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMaxX;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-clipmaxy
 * @recoil-artifact defines .data recoil:data:0x56b1ac: g_zVideo_FxPass3_ClipMaxY.
 * Data owner evidence: zVideo::FxPass3ApplyToCurrentSurface writes the
 * current pass-3 clip rectangle and the clipped scatter helper treats this as
 * the exclusive maximum Y edge.
 * Purpose: cache the exclusive maximum Y clip edge for pass-3 scatter copies.
 */
int g_zVideo_FxPass3_ClipMaxY;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvid-noisebytetablesize
 * @recoil-artifact defines .data recoil:data:0x56b1b8: g_zVid_NoiseByteTableSize.
 * Data owner evidence: zVid::NoiseInitBuffers writes the primary-surface
 * width multiplied by 25 before filling the byte table; DrawNoiseRect uses it
 * as the random row-window limit.
 * Purpose: cache the allocated noise-byte table length.
 */
int g_zVid_NoiseByteTableSize;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvid-noisebytetable
 * @recoil-artifact defines .data recoil:data:0x56b1bc: g_zVid_NoiseByteTable.
 * Data owner evidence: zVid::NoiseInitBuffers allocates and fills this byte
 * table, DrawNoiseRect samples it, and zVid::NoiseShutdownBuffers frees and
 * clears it when non-null.
 * Purpose: hold the software noise bytes used by the FX surface overlay path.
 */
unsigned char* g_zVid_NoiseByteTable;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3-scratchpixels16
 * @recoil-artifact defines .data recoil:data:0x56b1c0: g_zVideo_FxPass3_ScratchPixels16.
 * Data owner evidence: zVid::NoiseInitBuffers allocates a width*height
 * 16-bpp scratch buffer and stores it after clearing the active FX-surface
 * descriptor; zVid::NoiseShutdownBuffers frees and clears it when non-null.
 * zVideo::FxPass3CopySurfacePixelToScratchClipped at 0x48da60 writes through
 * this pointer with tight g_zVideo_FxSurfaceWidth row stride, while
 * zVideo::FxPass3ApplyToCurrentSurface at 0x48daf0 stages the radial ring
 * warp here before copying back to the active FX surface.
 * Purpose: stage pass-3 warp, blur, and related 16-bpp FX surface pixels.
 */
unsigned short* g_zVideo_FxPass3_ScratchPixels16;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacepixels16
 * @recoil-artifact defines .data recoil:data:0x56b1c4: g_zVideo_FxSurfacePixels16.
 * Data owner evidence: zVideo::FxSetSurfaceState writes this active surface
 * pointer, zVid::NoiseInitBuffers clears it during scratch initialization,
 * and noise/blur/pass-3/FX-surface routines use it as the 16-bpp destination.
 * zVideo::FxPass3CopySurfacePixelToScratchClipped at 0x48da60 reads source
 * pixels through this pointer using g_zVideo_FxSurfacePitchPixels16.
 * Purpose: point at the currently active 16-bpp FX surface pixel buffer.
 */
unsigned short* g_zVideo_FxSurfacePixels16;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacewidth
 * @recoil-artifact defines .data recoil:data:0x56b1c8: g_zVideo_FxSurfaceWidth.
 * Data owner evidence: zVideo::FxSetSurfaceState writes the active width,
 * zVid::NoiseInitBuffers clears it, and FX/noise/blur paths use it for bounds
 * and tight scratch-buffer row stride. FxPass3 clipped copies use this for
 * scratch row indexing, distinct from the provider pitch used for source rows.
 * Purpose: cache the active FX surface width in pixels.
 */
int g_zVideo_FxSurfaceWidth;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfaceheight
 * @recoil-artifact defines .data recoil:data:0x56b1cc: g_zVideo_FxSurfaceHeight.
 * Data owner evidence: zVideo::FxSetSurfaceState writes the active height,
 * zVid::NoiseInitBuffers clears it, and FX/noise/blur paths use it for full
 * surface clipping.
 * Purpose: cache the active FX surface height in pixels.
 */
int g_zVideo_FxSurfaceHeight;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacepitchbytes
 * @recoil-artifact defines .data recoil:data:0x56b1d0: g_zVideo_FxSurfacePitchBytes.
 * Data owner evidence: zVideo::FxSetSurfaceState writes the provider pitch in
 * bytes and zVid::NoiseInitBuffers clears it with the active surface record.
 * Purpose: retain the active FX surface row pitch in bytes.
 */
int g_zVideo_FxSurfacePitchBytes;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxsurfacepitchpixels16
 * @recoil-artifact defines .data recoil:data:0x56b1d4: g_zVideo_FxSurfacePitchPixels16.
 * Data owner evidence: zVideo::FxSetSurfaceState derives this from pitch
 * bytes divided by two; FX/noise/blur paths use it for source/destination row
 * stepping while scratch rows use g_zVideo_FxSurfaceWidth. BN assembly for
 * 0x48da60 reads g_zVideo_FxSurfacePixels16 after multiplying the biased
 * source Y by this value.
 * Purpose: retain the active FX surface row pitch in 16-bpp pixels.
 */
int g_zVideo_FxSurfacePitchPixels16;
}

namespace
{
    unsigned short zVideoBlendPixel565Alpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha);
    unsigned short zVideoBlendPixel555Alpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha);
    unsigned short zVideoBlendFramebufferPixelAlpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha);
    int zVideoGetAlphaSkipThreshold();
}

namespace zVideo
{
    static int __fastcall zVideoFxPass3ClampCurrentRadius(int currentRadius, int maxRadius);
    static int __fastcall zVideoFxPass3ApproxRadiusIndex(int distanceSquared, int maxRadius);
    static void __fastcall
    zVideoFxPass3CopyDirect(int centerX, int centerY, int dstDx, int dstDy, int srcDx, int srcDy);
    static void __fastcall
    zVideoFxPass3ScatterDirectSymmetric(int centerX, int centerY, int x, int y, int srcX, int srcY);
    static void __fastcall zVideoFxPass3ScatterClippedSymmetric(int x, int y, int srcX, int srcY);
    static void __fastcall zVideoFxPass3CopyScratchToSurface(int minX, int minY, int maxX, int maxY, int currentRadius);
    void __fastcall SetFogColorFromRgb01(zVideo_ColorRgbFloat * color);
    void __fastcall SetFogTargetColorFromRgb01(zVideo_ColorRgbFloat * color);
    void __fastcall PixelPackGetRgbBits(int* outRBits, int* outGBits, int* outBBits);
    void __fastcall PixelPackGetRgbMasks(unsigned int* outRMask, unsigned int* outGMask, unsigned int* outBMask);
    void __fastcall PixelPackGetPackingParams(int* outPackedBase, int* outSumMinus8, int* outBShiftTo8);
}

namespace zVideo_FxSurface
{
    static int TruncateFloat(float value);
    static int FxLineOutCode(int x, int y, int left, int top, int right, int bottom);
    static void DrawFxSurfaceSpanPixel(unsigned short* pixel, unsigned short color, int alpha);
}

namespace zRndr
{
    // zRndr_Overlay.cpp software overlay callback/global owner. FlushSw selects
    // one of the four 555/565 scalar/MMX row leaves, computes the premultiplied
    // source color and destination scale, and the row leaves consume this state
    // without owning independent data.
    OverlayBlendRowProc g_pfnOverlayBlendRow = 0;
    unsigned int g_swOverlayPremulPacked = 0;
    unsigned int g_swOverlayPremulPackedRot16 = 0;
    int g_swOverlayDstScale5 = 0;
    unsigned int g_swOverlayPremulRPair = 0;
    unsigned int g_swOverlayPremulBPair = 0;
    unsigned int g_swOverlayPremulGPair = 0;
    /*
     * zRndr_Overlay.cpp software overlay rectangle staging bank:
     * BN models 0x62e9dc..0x62e9ff as zero-initialized authored state. Submit
     * writes the rectangle/color/alpha fields, FlushSw consumes the same bank, and
     * lens-flare clipped-framebuffer paths read the enable/color/alpha subset.
     * The four bytes at 0x62e9f4 have no current xrefs and are retained as the
     * compiler-emitted alignment gap before the double at 0x62e9f8.
     */
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overlayblendenabled
     * @recoil-artifact defines .data recoil:data:0x62e9dc: g_overlayBlendEnabled
     * (BN: gRndr_OverlayBlendEnabled).
     * Data owner: render_video.zrndr_overlay_rect_staging_globals.
     * Purpose: record whether a software overlay rectangle is staged for blending.
     */
    int g_overlayBlendEnabled = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overlayblendrectleft
     * @recoil-artifact defines .data recoil:data:0x62e9e0: g_overlayBlendRectLeft
     * (BN: gRndr_OverlayBlendRectLeft).
     * Data owner: render_video.zrndr_overlay_rect_staging_globals.
     * Purpose: store the staged overlay rectangle left edge in pixels.
     */
    int g_overlayBlendRectLeft = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overlayblendrecttop
     * @recoil-artifact defines .data recoil:data:0x62e9e4: g_overlayBlendRectTop
     * (BN: gRndr_OverlayBlendRectTop).
     * Data owner: render_video.zrndr_overlay_rect_staging_globals.
     * Purpose: store the staged overlay rectangle top edge in pixels.
     */
    int g_overlayBlendRectTop = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overlayblendrectright
     * @recoil-artifact defines .data recoil:data:0x62e9e8: g_overlayBlendRectRight
     * (BN: gRndr_OverlayBlendRectRight).
     * Data owner: render_video.zrndr_overlay_rect_staging_globals.
     * Purpose: store the staged overlay rectangle right edge used by the software row flush.
     */
    int g_overlayBlendRectRight = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overlayblendrectbottom
     * @recoil-artifact defines .data recoil:data:0x62e9ec: g_overlayBlendRectBottom
     * (BN: gRndr_OverlayBlendRectBottom).
     * Data owner: render_video.zrndr_overlay_rect_staging_globals.
     * Purpose: store the staged overlay rectangle bottom edge in pixels.
     */
    int g_overlayBlendRectBottom = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overlayblendpackedcolor16
     * @recoil-artifact defines .data recoil:data:0x62e9f0: g_overlayBlendPackedColor16
     * (BN: gRndr_OverlayBlendPackedColor16).
     * Data owner: render_video.zrndr_overlay_rect_staging_globals.
     * Purpose: cache the staged 16-bpp overlay color used by software overlay and lens-flare blending.
     */
    unsigned int g_overlayBlendPackedColor16 = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overlayblendalpha
     * @recoil-artifact defines .data recoil:data:0x62e9f8: g_overlayBlendAlpha
     * (BN: gRndr_OverlayBlendAlpha).
     * Data owner: render_video.zrndr_overlay_rect_staging_globals.
     * Purpose: cache the staged overlay alpha as the x87 double consumed by software overlay paths.
     */
    double g_overlayBlendAlpha = 0.0;
} // namespace zRndr

namespace zVid
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-noise-initbuffers
     * @recoil-artifact defines .text recoil:function:0x48d340: zVid::NoiseInitBuffers
     * @recoil-match byte
     *
     * Data-gate evidence: BN writes gRndr_pfnOverlayBlendRow to
     * zRndr::OverlayBlendRow555Scalar after allocating the noise and FX scratch
     * buffers, so data acceptance waits on the zRndr overlay callback owner.
     * Purpose: Allocate the software-noise byte table and FX pass scratch buffer.
     */
    void __cdecl NoiseInitBuffers()
    {
        const int width = zVideo::GetPrimarySurfaceWidth();
        const int height = zVideo::GetPrimarySurfaceHeight();
        int i;

        g_zVid_NoiseByteTableSize = width * 0x19;
        g_zVid_NoiseByteTable = (unsigned char*)(malloc((size_t)(g_zVid_NoiseByteTableSize)));
        for (i = 0; i < g_zVid_NoiseByteTableSize; ++i) {
            g_zVid_NoiseByteTable[i] = (unsigned char)(rand());
        }

        g_zVideo_FxPass3_ScratchPixels16 = (unsigned short*)(malloc((size_t)(height * width) * sizeof(unsigned short)));
        g_zVideo_FxSurfacePixels16 = 0;
        g_zVideo_FxSurfaceWidth = 0;
        g_zVideo_FxSurfaceHeight = 0;
        g_zVideo_FxSurfacePitchBytes = 0;
        g_zVideo_FxSurfacePitchPixels16 = 0;
        zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555Scalar;
    }
} // namespace zVid

namespace zVid
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-noise-shutdownbuffers
     * @recoil-artifact defines .text recoil:function:0x48d3e0: zVid::NoiseShutdownBuffers.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
     * Data owner evidence: current BN loads the noise table pointer, conditionally
     * frees it, loads the pass-3 scratch pointer, clears g_zVid_NoiseByteTable,
     * then conditionally frees and clears g_zVideo_FxPass3_ScratchPixels16.
     * Purpose: release the software-noise byte table and pass-3 scratch buffer.
     */
    void __cdecl NoiseShutdownBuffers()
    {
        if (g_zVid_NoiseByteTable != 0) {
            free(g_zVid_NoiseByteTable);
        }

        unsigned short* scratchPixels = g_zVideo_FxPass3_ScratchPixels16;
        g_zVid_NoiseByteTable = 0;

        if (scratchPixels != 0) {
            free(scratchPixels);
        }
        g_zVideo_FxPass3_ScratchPixels16 = 0;
    }
} // namespace zVid

namespace zVideo
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fx-setsurfacestate
     * @recoil-artifact defines .text recoil:function:0x48d420: zVideo::FxSetSurfaceState.
     * @recoil-match byte
     *
     * Purpose: Publishes the active FX surface descriptor and derives the 16-bit pitch.
     */
    void __fastcall FxSetSurfaceState(void* pixels, int width, int height, int pitchBytes)
    {
        g_zVideo_FxSurfaceWidth = width;
        g_zVideo_FxSurfaceHeight = height;
        g_zVideo_FxSurfacePitchBytes = pitchBytes;
        g_zVideo_FxSurfacePixels16 = (unsigned short*)(pixels);
        g_zVideo_FxSurfacePitchPixels16 = pitchBytes / 2;
    }
} // namespace zVideo

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-overlayblendrow555-scalar
     * @recoil-artifact defines .text recoil:function:0x48d450: zRndr::OverlayBlendRow555Scalar
     * @recoil-match byte
     *
     * Source-shape evidence: BN zRndr_Overlay.cpp loads and stores two 555 pixels
     * per uint32_t using the precomputed overlay premul and destination-scale globals;
     * the row extent is the inclusive right-left delta passed by FlushSw.
     * Owner: shared zRndr_Overlay.cpp overlay callback/global owner with 0x48d7a0,
     * 0x48d4b0, 0x48d510, and 0x48d5f0.
     * Purpose: Blend one 555 overlay row using the cached software overlay alpha and premultiplied source color.
     */
    void __fastcall OverlayBlendRow555Scalar(unsigned short* rowPixels16, int rightDelta)
    {
        int pairCount = rightDelta >> 1;
        unsigned int* rowPairs = (unsigned int*)(rowPixels16);
        do {
            const unsigned int packedPair = *rowPairs;
            const unsigned int loLanes = ((((packedPair & 0x03e07c1fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5)
                                             + g_swOverlayPremulPackedRot16)
                & 0x03e07c1fU;
            const unsigned int hiLanes
                = ((((packedPair >> 5) & 0x03e0f81fU) * (unsigned int)(g_swOverlayDstScale5)) + g_swOverlayPremulPacked)
                & 0x7c1f03e0U;
            *rowPairs = hiLanes | loLanes;
            ++rowPairs;
        } while (pairCount-- != 0);
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-overlayblendrow565-scalar
     * @recoil-artifact defines .text recoil:function:0x48d4b0: zRndr::OverlayBlendRow565Scalar
     * @recoil-match byte
     *
     * Source-shape evidence: BN zRndr_Overlay.cpp matches the 555 row shape with
     * two 565 pixels per uint32_t and the inclusive right-left delta row extent.
     * Owner: shared zRndr_Overlay.cpp overlay callback/global owner with 0x48d7a0,
     * 0x48d450, 0x48d510, and 0x48d5f0.
     * Purpose: Blend one 565 overlay row using the active pixel masks and cached overlay alpha.
     */
    void __fastcall OverlayBlendRow565Scalar(unsigned short* rowPixels16, int rightDelta)
    {
        int pairCount = rightDelta >> 1;
        do {
            const unsigned int packedPair = *(unsigned int*)(rowPixels16);
            const unsigned int loLanes
                = ((((packedPair & 0x07e0f81fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5)
                    + g_swOverlayPremulPackedRot16);
            const unsigned int hiLanes
                = (((packedPair >> 5) & 0x07c0f83fU) * (unsigned int)(g_swOverlayDstScale5)) + g_swOverlayPremulPacked;
            *(unsigned int*)(rowPixels16) = ((loLanes ^ hiLanes) & 0x07e0f81fU) ^ hiLanes;
            rowPixels16 += 2;
        } while (pairCount-- != 0);
    }
} // namespace zRndr

namespace zRndr
{
/**
 * Source-shape evidence: BN zRndr_Overlay.cpp builds replicated 555 masks,
 * premul RGB pairs, and destination-scale words on the stack, then processes
 * four 16-bit pixels per MMX qword before emms. The guarded VC5 x86 path keeps
 * C++ responsible for the function shell and stack constants, and uses narrow
 * inline asm only for the MMX qword loop; the portable fallback remains
 * behavior-only.
 * Owner: shared zRndr_Overlay.cpp overlay callback/global owner with 0x48d7a0,
 * 0x48d450, 0x48d4b0, and 0x48d5f0.
 * Purpose: Blend one RGB555 overlay row through the user-approved zRndr MMX inline-assembly exception.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_OVERLAY_MMX_RAW_ASM)
    /**
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.overlay-blend-row-555-mmx
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.overlay-blend-row-555-mmx recoil:function:0x48d510
     * Original function evidence: retail 0x48d510 contains this approved MMX region.
     * Raw-assembly evidence: the retail MMX qword loop uses packed 555 masks and
     * `emms`; VC5SP3 has no usable intrinsic surface for this instruction shape.
     * Purpose: Blend one RGB555 overlay row through the user-approved zRndr MMX inline-assembly exception.
     */
    void __fastcall OverlayBlendRow555Mmx(unsigned short* rowPixels16, int pixelCount)
    {
        unsigned short scaleWords[4];
        unsigned int redMasks[2];
        unsigned int greenMasks[2];
        unsigned int blueMasks[2];
        unsigned int premulR[2];
        unsigned int premulG[2];
        unsigned int premulB[2];
        const unsigned short scale = (unsigned short)(g_swOverlayDstScale5);

        scaleWords[3] = scale;
        scaleWords[2] = scale;
        scaleWords[1] = scale;
        scaleWords[0] = scale;
        redMasks[1] = 0x7c007c00U;
        redMasks[0] = 0x7c007c00U;
        greenMasks[1] = 0x03e003e0U;
        greenMasks[0] = 0x03e003e0U;
        blueMasks[1] = 0x001f001fU;
        blueMasks[0] = 0x001f001fU;
        premulR[1] = g_swOverlayPremulRPair;
        premulR[0] = g_swOverlayPremulRPair;
        premulG[1] = g_swOverlayPremulGPair;
        premulG[0] = g_swOverlayPremulGPair;
        premulB[1] = g_swOverlayPremulBPair;
        premulB[0] = g_swOverlayPremulBPair;

        __asm {
        mov eax, pixelCount
        mov esi, rowPixels16
        shr eax, 2
        lea esi, [esi+eax*8]
        xor eax, 0ffffffffh
        inc eax
        jge recoil_overlay555_done

        movq mm3, qword ptr [scaleWords]
        movq mm4, qword ptr [redMasks]
        movq mm5, qword ptr [greenMasks]
        movq mm6, qword ptr [blueMasks]
        movq mm7, qword ptr [premulR]
        movq mm2, qword ptr [esi+eax*8]

    recoil_overlay555_loop:
        movq mm0, mm2
        movq mm1, mm2
        pand mm0, mm4
        pand mm1, mm5
        pand mm2, mm6
        psrlw mm0, 5
        psrlw mm1, 5
        pmullw mm2, mm3
        pmullw mm0, mm3
        pmullw mm1, mm3
        inc eax
        psrlw mm2, 5
        paddw mm0, mm7
        paddw mm1, qword ptr [premulG]
        pand mm0, mm4
        paddw mm2, qword ptr [premulB]
        pand mm1, mm5
        pand mm2, mm6
        paddw mm0, mm1
        paddw mm0, mm2
        movq mm2, qword ptr [esi+eax*8]
        movq qword ptr [esi+eax*8-8], mm0
        jne recoil_overlay555_loop

    recoil_overlay555_done:
        emms
        }
    }
#else
    /**
     * Original function evidence: retail 0x48d510 has this portable conditional definition.
     * Purpose: Preserve portable RGB555 overlay row behavior when the VC5 inline-MMX exception is disabled.
     */
    void __fastcall OverlayBlendRow555Mmx(unsigned short* rowPixels16, int pixelCount)
    {
        int groupCount = pixelCount >> 2;
        unsigned short* rowEnd = rowPixels16 + (groupCount << 2);
        int groupIndex = -groupCount;
        while (groupIndex < 0) {
            unsigned short* row = rowEnd + (groupIndex << 2);
            for (int lane = 0; lane < 4; ++lane) {
                const unsigned int dst = row[lane];
                const unsigned int red = (((dst & 0x7c00U) >> 5) * (unsigned int)(g_swOverlayDstScale5)
                                             + (g_swOverlayPremulRPair & 0xffffU))
                    & 0x7c00U;
                const unsigned int green = (((dst & 0x03e0U) >> 5) * (unsigned int)(g_swOverlayDstScale5)
                                               + (g_swOverlayPremulGPair & 0xffffU))
                    & 0x03e0U;
                const unsigned int blue = (((dst & 0x001fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5)
                    + (g_swOverlayPremulBPair & 0xffffU);
                row[lane] = (unsigned short)(red | green | (blue & 0x001fU));
            }
            ++groupIndex;
        }
    }
#endif
} // namespace zRndr
namespace
{
    /**
     * Recovered helper: zVideoBlendPixel565Alpha8.
     * Original-source helper evidence: no standalone retail function is present; recovered from
     * address-backed framebuffer blit callers in this source file.
     * Purpose: Blend one 565 destination/source pixel pair using an 8-bit alpha value.
     */
    unsigned short zVideoBlendPixel565Alpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha)
    {
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
     * Recovered helper: zVideoBlendPixel555Alpha8.
     * Original-source helper evidence: no standalone retail function is present; recovered from
     * address-backed framebuffer blit callers in this source file.
     * Purpose: Blend one 555 destination/source pixel pair using an 8-bit alpha value.
     */
    unsigned short zVideoBlendPixel555Alpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha)
    {
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
     * Recovered helper: zVideoBlendFramebufferPixelAlpha8.
     * Original-source helper evidence: no standalone retail function is present; recovered from
     * address-backed framebuffer blit callers in this source file.
     * Purpose: Select the current framebuffer pixel format and blend one alpha-scaled pixel.
     */
    unsigned short zVideoBlendFramebufferPixelAlpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha)
    {
        if (zRndr::g_pixelPackGreenBits == 6) {
            return zVideoBlendPixel565Alpha8(dstPixel, srcPixel, alpha);
        }

        return zVideoBlendPixel555Alpha8(dstPixel, srcPixel, alpha);
    }

    /**
     * Recovered helper: zVideoGetAlphaSkipThreshold.
     * Original-source helper evidence: no standalone retail function is present; recovered from
     * address-backed framebuffer blit callers in this source file.
     * Purpose: Return the alpha-map threshold below which framebuffer pixels are skipped.
     */
    int zVideoGetAlphaSkipThreshold()
    {
        return zRndr::g_pixelPackGreenBits == 6 ? 3 : 7;
    }

} // namespace

namespace zVideo
{
    /**
     * Original-source helper evidence: no standalone retail address is assigned to
     * this helper shape in current plan/BN evidence; observed in caller
     * zVideo::FxPass3ApplyToCurrentSurface at 0x48daf0. The BN body clamps the
     * current radius against a non-negative max radius before the early-exit test.
     * Purpose: clamp the pass-3 current radius to the valid [0, max] range.
     */
    static int __fastcall zVideoFxPass3ClampCurrentRadius(int currentRadius, int maxRadius)
    {
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
     * zVideo::FxPass3ApplyToCurrentSurface at 0x48daf0. BN uses the repeated
     * integer-bit square-root approximation, then clamps the result to maxRadius.
     * Purpose: approximate the radius-table index used by the pass-3 radial warp.
     */
    static int __fastcall zVideoFxPass3ApproxRadiusIndex(int distanceSquared, int maxRadius)
    {
        float distanceSquaredFloat = (float)(distanceSquared);
        int bits = *((int*)(&distanceSquaredFloat));
        bits = (bits >> 1) + 0x1fc00000;
        const int radiusIndex = (int)(*((float*)(&bits)));
        if (radiusIndex >= maxRadius) {
            return maxRadius;
        }
        return radiusIndex;
    }

    /**
     * Original-source helper evidence: no standalone retail address is assigned to
     * this helper shape in current plan/BN evidence; observed in the non-clipped
     * scatter path of zVideo::FxPass3ApplyToCurrentSurface at 0x48daf0. BN uses
     * center-relative deltas and direct pointer indexing with surface pitch for the
     * source and tight surface width for scratch.
     * Purpose: copy one pass-3 sample through the direct in-bounds scatter path.
     */
    static void __fastcall zVideoFxPass3CopyDirect(int centerX, int centerY, int dstDx, int dstDy, int srcDx, int srcDy)
    {
        g_zVideo_FxPass3_ScratchPixels16[(centerY + dstDy) * g_zVideo_FxSurfaceWidth + centerX + dstDx]
            = g_zVideo_FxSurfacePixels16[(centerY + srcDy) * g_zVideo_FxSurfacePitchPixels16 + centerX + srcDx];
    }

    /**
     * Original-source helper evidence: no standalone retail address is assigned to
     * this helper shape in current plan/BN evidence; observed as the repeated
     * eight-way direct scatter pattern in zVideo::FxPass3ApplyToCurrentSurface at
     * 0x48daf0.
     * Purpose: scatter a direct pass-3 sample to the eight mirrored ring positions.
     */
    static void __fastcall
    zVideoFxPass3ScatterDirectSymmetric(int centerX, int centerY, int x, int y, int srcX, int srcY)
    {
        zVideoFxPass3CopyDirect(centerX, centerY, x, y, srcX, srcY);
        zVideoFxPass3CopyDirect(centerX, centerY, y, x, srcY, srcX);
        zVideoFxPass3CopyDirect(centerX, centerY, -x, y, -srcX, srcY);
        zVideoFxPass3CopyDirect(centerX, centerY, y, -x, srcY, -srcX);
        zVideoFxPass3CopyDirect(centerX, centerY, x, -y, srcX, -srcY);
        zVideoFxPass3CopyDirect(centerX, centerY, -y, x, -srcY, srcX);
        zVideoFxPass3CopyDirect(centerX, centerY, -x, -y, -srcX, -srcY);
        zVideoFxPass3CopyDirect(centerX, centerY, -y, -x, -srcY, -srcX);
    }

    /**
     * Original-source helper evidence: no standalone retail address is assigned to
     * this helper shape in current plan/BN evidence; observed as the repeated
     * eight-call clipped scatter pattern in zVideo::FxPass3ApplyToCurrentSurface
     * at 0x48daf0, with each arm calling the address-backed helper at 0x48da60.
     * Purpose: scatter a pass-3 sample to eight mirrored ring positions through
     * the active clip bounds.
     */
    static void __fastcall zVideoFxPass3ScatterClippedSymmetric(int x, int y, int srcX, int srcY)
    {
        FxPass3CopySurfacePixelToScratchClipped(x, y, srcX, srcY);
        FxPass3CopySurfacePixelToScratchClipped(y, x, srcY, srcX);
        FxPass3CopySurfacePixelToScratchClipped(-x, y, -srcX, srcY);
        FxPass3CopySurfacePixelToScratchClipped(y, -x, srcY, -srcX);
        FxPass3CopySurfacePixelToScratchClipped(x, -y, srcX, -srcY);
        FxPass3CopySurfacePixelToScratchClipped(-y, x, -srcY, srcX);
        FxPass3CopySurfacePixelToScratchClipped(-x, -y, -srcX, -srcY);
        FxPass3CopySurfacePixelToScratchClipped(-y, -x, -srcY, -srcX);
    }

    /**
     * Original-source helper evidence: no standalone retail address is assigned to
     * this helper shape in current plan/BN evidence; observed at the tail of
     * zVideo::FxPass3ApplyToCurrentSurface at 0x48daf0. BN copies a bounded
     * scratch region back to the active FX surface while skipping coordinates that
     * remain inside the current radius.
     * Purpose: copy the staged pass-3 scratch region back to the active FX surface.
     */
    static void __fastcall zVideoFxPass3CopyScratchToSurface(int minX, int minY, int maxX, int maxY, int currentRadius)
    {
        int y;
        for (y = minY; y < maxY; ++y) {
            if (y > currentRadius || y < -currentRadius) {
                unsigned short* src = g_zVideo_FxPass3_ScratchPixels16 + y * g_zVideo_FxSurfaceWidth + minX;
                unsigned short* dst = g_zVideo_FxSurfacePixels16 + y * g_zVideo_FxSurfacePitchPixels16 + minX;
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

} // namespace zVideo

namespace zVideo_FxSurface
{
    /**
     * Original-source helper evidence: no standalone retail function; 0x48ed60
     * inlines this RGB565 alpha blend in both major-axis line loops.
     * Purpose: alpha-blend one RGB565 FX-surface pixel.
     */
    static unsigned short BlendFxSurfacePixel565(unsigned short dst, unsigned short color, int alpha)
    {
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
    static unsigned short BlendFxSurfacePixel555(unsigned short dst, unsigned short color, int alpha)
    {
        const int dstValue = (int)(dst);
        const int colorValue = (int)(color);
        const int redDelta = (((colorValue & 0x7c00) - (dstValue & 0x7c00)) * alpha) >> 8;
        const int greenDelta = (((colorValue & 0x03e0) - (dstValue & 0x03e0)) * alpha) >> 8;
        const int blueDelta = (((colorValue & 0x001f) - (dstValue & 0x001f)) * alpha) >> 8;
        return (unsigned short)(dstValue + (redDelta & 0xfc00) + (greenDelta & 0xffe0) + blueDelta);
    }

    /**
     * Original-source helper evidence: no standalone retail function; repeated
     * float-to-int truncation in 0x48ed60 uses the VC5 _ftol lowering pattern.
     * BN shows these as namespace functions in zVideo.cpp with no constructor, vtable, or owned
     * object layout evidence. Model this slice as a source-file namespace cluster over the typed
     * FX-surface globals above.
     * Purpose: truncate FX line-clipping intermediates toward zero.
     */
    static int TruncateFloat(float value)
    {
        return (int)(value);
    }

    /**
     * Original-source helper evidence: no standalone retail function; 0x48ed60
     * repeats the same Cohen-Sutherland outcode tests for both line endpoints.
     * Purpose: classify an FX line endpoint against the clipped rectangle.
     */
    static int FxLineOutCode(int x, int y, int left, int top, int right, int bottom)
    {
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
     * inlines the same RGB555/RGB565 threshold and solid-write branches in both
     * major-axis line loops.
     * Purpose: draw one alpha-controlled span pixel for an FX line.
     */
    static void DrawFxSurfaceSpanPixel(unsigned short* pixel, unsigned short color, int alpha)
    {
        if (zRndr::g_pixelPackGreenBits == 5) {
            if (alpha <= 7) {
                return;
            }
            if (alpha >= 252) {
                *pixel = color;
                return;
            }
            *pixel = BlendFxSurfacePixel555(*pixel, color, alpha);
            return;
        }

        if (alpha <= 3) {
            return;
        }
        if (alpha >= 252) {
            *pixel = color;
            return;
        }
        *pixel = BlendFxSurfacePixel565(*pixel, color, alpha);
    }

} // namespace zVideo_FxSurface
namespace zRndr
{
/**
 * Source-shape evidence: BN zRndr_Overlay.cpp mirrors the 555 MMX row loop
 * with 565 masks, replicated premul RGB pairs, and four 16-bit pixels per MMX
 * qword before emms. The guarded VC5 x86 path keeps C++ responsible for the
 * function shell and stack constants, and uses narrow inline asm only for the
 * MMX qword loop; the portable fallback remains behavior-only.
 * Owner: shared zRndr_Overlay.cpp overlay callback/global owner with 0x48d7a0,
 * 0x48d450, 0x48d4b0, and 0x48d510.
 * Purpose: Blend one RGB565 overlay row through the user-approved zRndr MMX inline-assembly exception.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_OVERLAY_MMX_RAW_ASM)
    /**
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.overlay-blend-row-565-mmx
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.overlay-blend-row-565-mmx recoil:function:0x48d5f0
     * Original function evidence: retail 0x48d5f0 contains this approved MMX region.
     * Raw-assembly evidence: the retail MMX qword loop uses packed 565 masks and
     * `emms`; VC5SP3 has no usable intrinsic surface for this instruction shape.
     * Purpose: Blend one RGB565 overlay row through the user-approved zRndr MMX inline-assembly exception.
     */
    void __fastcall OverlayBlendRow565Mmx(unsigned short* rowPixels16, int pixelCount)
    {
        unsigned short scaleWords[4];
        unsigned int redMasks[2];
        unsigned int greenMasks[2];
        unsigned int blueMasks[2];
        unsigned int premulR[2];
        unsigned int premulG[2];
        unsigned int premulB[2];
        const unsigned short scale = (unsigned short)(g_swOverlayDstScale5);

        scaleWords[3] = scale;
        scaleWords[2] = scale;
        scaleWords[1] = scale;
        scaleWords[0] = scale;
        redMasks[1] = 0xf800f800U;
        redMasks[0] = 0xf800f800U;
        greenMasks[1] = 0x07e007e0U;
        greenMasks[0] = 0x07e007e0U;
        blueMasks[1] = 0x001f001fU;
        blueMasks[0] = 0x001f001fU;
        premulR[1] = g_swOverlayPremulRPair;
        premulR[0] = g_swOverlayPremulRPair;
        premulG[1] = g_swOverlayPremulGPair;
        premulG[0] = g_swOverlayPremulGPair;
        premulB[1] = g_swOverlayPremulBPair;
        premulB[0] = g_swOverlayPremulBPair;

        __asm {
        mov eax, pixelCount
        mov esi, rowPixels16
        shr eax, 2
        lea esi, [esi+eax*8]
        xor eax, 0ffffffffh
        inc eax
        jge recoil_overlay565_done

        movq mm3, qword ptr [scaleWords]
        movq mm4, qword ptr [redMasks]
        movq mm5, qword ptr [greenMasks]
        movq mm6, qword ptr [blueMasks]
        movq mm7, qword ptr [premulR]
        movq mm2, qword ptr [esi+eax*8]

    recoil_overlay565_loop:
        movq mm0, mm2
        movq mm1, mm2
        pand mm0, mm4
        pand mm1, mm5
        pand mm2, mm6
        psrlw mm0, 5
        psrlw mm1, 5
        pmullw mm2, mm3
        pmullw mm0, mm3
        pmullw mm1, mm3
        inc eax
        psrlw mm2, 5
        paddw mm0, mm7
        paddw mm1, qword ptr [premulG]
        pand mm0, mm4
        paddw mm2, qword ptr [premulB]
        pand mm1, mm5
        pand mm2, mm6
        paddw mm0, mm1
        paddw mm0, mm2
        movq mm2, qword ptr [esi+eax*8]
        movq qword ptr [esi+eax*8-8], mm0
        jne recoil_overlay565_loop

    recoil_overlay565_done:
        emms
        }
    }
#else
    /**
     * Original function evidence: retail 0x48d5f0 has this portable conditional definition.
     * Purpose: Preserve portable RGB565 overlay row behavior when the VC5 inline-MMX exception is disabled.
     */
    void __fastcall OverlayBlendRow565Mmx(unsigned short* rowPixels16, int pixelCount)
    {
        int groupCount = pixelCount >> 2;
        unsigned short* rowEnd = rowPixels16 + (groupCount << 2);
        int groupIndex = -groupCount;
        while (groupIndex < 0) {
            unsigned short* row = rowEnd + (groupIndex << 2);
            for (int lane = 0; lane < 4; ++lane) {
                const unsigned int dst = row[lane];
                const unsigned int red = (((dst & 0xf800U) >> 5) * (unsigned int)(g_swOverlayDstScale5)
                                             + (g_swOverlayPremulRPair & 0xffffU))
                    & 0xf800U;
                const unsigned int green = (((dst & 0x07e0U) >> 5) * (unsigned int)(g_swOverlayDstScale5)
                                               + (g_swOverlayPremulGPair & 0xffffU))
                    & 0x07e0U;
                const unsigned int blue = (((dst & 0x001fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5)
                    + (g_swOverlayPremulBPair & 0xffffU);
                row[lane] = (unsigned short)(red | green | (blue & 0x001fU));
            }
            ++groupIndex;
        }
    }
#endif
} // namespace zRndr

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-overlayrect-submit
 * @recoil-artifact defines .text recoil:function:0x48d6d0: zRndrOverlayRectSubmit
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Overlay.cpp.
 * Source file evidence: recovered original path on the prior source label.
 * Purpose: Submit an overlay rectangle to Direct3D or stage it for software overlay blending.
 */
void __fastcall zRndrOverlayRectSubmit(unsigned short packedColor16, zVidRect32* rectOrNull, double alpha)
{
    const unsigned short overlayColor16 = (unsigned short)(packedColor16);
    zVidRect32 rect;
    int xMax;
    if (rectOrNull != 0) {
        rect.left = rectOrNull->left;
        rect.top = rectOrNull->top;
        xMax = rectOrNull->right;
        rect.bottom = rectOrNull->bottom;
    } else {
        rect.top = 0;
        rect.left = 0;
        rect.bottom = g_zVideo_FxSurfaceHeight;
        xMax = g_zVideo_FxSurfaceWidth - 1;
    }

    if (g_zVideo_ActiveRendererPath != 0) {
        rect.right = xMax + 1;
        zVideo_dd3d::QueueSolidQuad(overlayColor16, alpha, &rect);
        return;
    }

    zRndr::g_overlayBlendRectTop = rect.top;
    zRndr::g_overlayBlendRectRight = xMax;
    zRndr::g_overlayBlendRectBottom = rect.bottom;
    zRndr::g_overlayBlendRectLeft = rect.left;
    zRndr::g_overlayBlendEnabled = 1;
    zRndr::g_overlayBlendPackedColor16 = overlayColor16;
    zRndr::g_overlayBlendAlpha = alpha;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-overlayrect-flushsw
 * @recoil-artifact defines .text recoil:function:0x48d7a0: zRndrOverlayRectFlushSw
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Overlay.cpp.
 * Source file evidence: recovered original path on the prior source label.
 * Source-shape evidence: BN selects the 555/565 scalar or MMX row callback,
 * computes packed premul and destination-scale globals through x87/_ftol, then
 * calls the selected row callback for each FX-surface row.
 * Owner: shared zRndr_Overlay.cpp overlay callback/global owner with row leaves
 * 0x48d450, 0x48d4b0, 0x48d510, and 0x48d5f0.
 * Purpose: Blend the staged software overlay rectangle into the active 16-bit video surface.
 */
void __cdecl zRndrOverlayRectFlushSw()
{
    if (zRndr::g_overlayBlendEnabled == 0) {
        return;
    }

    const unsigned char graphicsFlags = *(const unsigned char*)(zRndr::g_graphicsFlags);
    if ((graphicsFlags & 4U) != 0) {
        if (zRndr::g_pixelPackGreenBits == 5) {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555Mmx;
        } else {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow565Mmx;
        }
    } else {
        if (zRndr::g_pixelPackGreenBits == 5) {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555Scalar;
        } else {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow565Scalar;
        }
    }

    unsigned int redMask;
    unsigned int greenMask;
    unsigned int blueMask;
    zVideo::PixelPackGetRgbMasks(&redMask, &greenMask, &blueMask);

    const int srcScale5 = (int)(zRndr::g_overlayBlendAlpha * 32.0);
    const unsigned int overlayColor16 = zRndr::g_overlayBlendPackedColor16;
    const unsigned int premulR = ((redMask & overlayColor16) * srcScale5) >> 5;
    const unsigned int premulG = ((greenMask & overlayColor16) * srcScale5) >> 5;
    const unsigned int premulB = ((blueMask & overlayColor16) * srcScale5) >> 5;
    const unsigned int premulRPair = premulR | (premulR << 16);
    const unsigned int premulGPair = premulG | (premulG << 16);
    const unsigned int premulBPair = premulB | (premulB << 16);
    zRndr::g_swOverlayPremulRPair = premulRPair;
    zRndr::g_swOverlayPremulGPair = premulGPair;
    zRndr::g_swOverlayPremulBPair = premulBPair;
    zRndr::g_swOverlayPremulPacked
        = (((blueMask & premulBPair) | (redMask & premulRPair)) << 16) | (greenMask & premulGPair);
    zRndr::g_swOverlayPremulPackedRot16 = _rotr(zRndr::g_swOverlayPremulPacked, 16);
    zRndr::g_swOverlayDstScale5 = (int)((1.0 - zRndr::g_overlayBlendAlpha) * 32.0);

    const int pitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    int rowY = zRndr::g_overlayBlendRectTop;
    const int rectLeft = zRndr::g_overlayBlendRectLeft;
    const int pixelCount = zRndr::g_overlayBlendRectRight - rectLeft;
    unsigned short* rowPixels16 = g_zVideo_FxSurfacePixels16 + pitchPixels16 * rowY + rectLeft;
    while (rowY < zRndr::g_overlayBlendRectBottom) {
        zRndr::g_pfnOverlayBlendRow(rowPixels16, pixelCount);
        ++rowY;
        rowPixels16 += g_zVideo_FxSurfacePitchPixels16;
    }
}

namespace zVid
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-drawnoiserect
     * @recoil-artifact defines .text recoil:function:0x48d910: zVid::DrawNoiseRect.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
     * Purpose: overlay thresholded grayscale noise on the active FX surface rectangle.
     */
    void __fastcall DrawNoiseRect(zVidRect32 * rectOrNull, double intensity)
    {
        if (intensity < 0.00390625) {
            return;
        }

        const int threshold = (int)(intensity * 256.0);
        zVidRect32 rect;
        if (rectOrNull != 0) {
            rect = *rectOrNull;
        } else {
            rect.left = 0;
            rect.top = 0;
            rect.bottom = g_zVideo_FxSurfaceHeight - 1;
            rect.right = g_zVideo_FxSurfaceWidth - 1;
        }

        const int xMin = rect.left;
        const int yMin = rect.top;
        const int yMax = rect.bottom;
        const int rowWidth = rect.right - xMin;
        int rBits;
        int gBits;
        int bBits;
        zVideo::PixelPackGetRgbBits(&rBits, &gBits, &bBits);

        const int rShift = bBits + gBits;
        int gShift = bBits;
        if (gBits == 6) {
            ++gShift;
        }

        for (int y = yMin; y < yMax; ++y) {
            const int noiseOffset = (rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
            unsigned char* noiseBytes = g_zVid_NoiseByteTable + noiseOffset;
            unsigned short* dstPixels = g_zVideo_FxSurfacePixels16 + y * g_zVideo_FxSurfacePitchPixels16 + xMin;

            for (int x = 0; x < rowWidth; ++x) {
                const int noiseValue = *noiseBytes;
                if (noiseValue < threshold) {
                    const int level = noiseValue & 0x1f;
                    *dstPixels = (unsigned short)((level << rShift) | (level << gShift) | level);
                }

                ++dstPixels;
                ++noiseBytes;
            }
        }
    }
} // namespace zVid

namespace zVideo
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fxpass3-copysurfacepixeltoscratchclipped
     * @recoil-artifact defines .text recoil:function:0x48da60: zVideo::FxPass3CopySurfacePixelToScratchClipped.
     * @recoil-match byte
     *
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
    void __fastcall FxPass3CopySurfacePixelToScratchClipped(int dstDx, int dstDy, int srcDx, int srcDy)
    {
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

        g_zVideo_FxPass3_ScratchPixels16[dstY * g_zVideo_FxSurfaceWidth + dstX]
            = g_zVideo_FxSurfacePixels16[srcY * g_zVideo_FxSurfacePitchPixels16 + srcX];
    }
} // namespace zVideo

namespace zVideo
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fxpass3-applytocurrentsurface
     * @recoil-artifact defines .text recoil:function:0x48daf0: zVideo::FxPass3ApplyToCurrentSurface.
     *
     *
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
    void __fastcall FxPass3ApplyToCurrentSurface(
        int centerX,
        int centerY,
        int currentRadius,
        int maxRadius,
        int extent,
        float sinFreq,
        float sinPhase,
        zVidRect32* clipRectOrNull
    )
    {
        const int cappedMaxRadius = maxRadius > 0 ? maxRadius : 0;
        if (currentRadius > cappedMaxRadius) {
            currentRadius = cappedMaxRadius;
        }
        if (currentRadius < 0) {
            currentRadius = 0;
        }
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
        if (minOuterX > clipMaxX || maxOuterX < clipMinX || minOuterY > clipMaxY || maxOuterY < clipMinY) {
            return;
        }

        float* sinAmpTable = (float*)(_alloca((cappedMaxRadius + 1) * sizeof(float)));
        float* recipTable = (float*)(_alloca((cappedMaxRadius + 1) * sizeof(float)));
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

        const int useClippedPath
            = minOuterX < clipMinX || maxOuterX >= clipMaxX || minOuterY < clipMinY || maxOuterY >= clipMaxY;
        if (!useClippedPath) {
            int y;
            for (y = -cappedMaxRadius; y <= currentRadius; ++y) {
                int x;
                for (x = y; x <= currentRadius; ++x) {
                    const int distanceSquared = x * x + y * y;
                    int srcX = x;
                    int srcY = y;
                    if (distanceSquared < maxRadiusSquared && currentRadiusSquared < distanceSquared) {
                        const float distanceSquaredFloat = (float)(distanceSquared);
                        const int distanceBits = *((int*)(&distanceSquaredFloat));
                        int approximateBits = (distanceBits >> 1) + 0x1fc00000;
                        int radiusIndex;
                        if ((int)(*((float*)(&approximateBits))) >= cappedMaxRadius) {
                            radiusIndex = cappedMaxRadius;
                        } else {
                            approximateBits = (distanceBits >> 1) + 0x1fc00000;
                            radiusIndex = (int)(*((float*)(&approximateBits)));
                        }
                        const float scale = sinAmpTable[radiusIndex] * recipTable[radiusIndex];
                        srcX = x + (int)((float)(x)*scale);
                        srcY = y + (int)((float)(y)*scale);
                    }

                    g_zVideo_FxPass3_ScratchPixels16[(centerY + y) * g_zVideo_FxSurfaceWidth + centerX + x]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY + srcY) * g_zVideo_FxSurfacePitchPixels16 + centerX + srcX];
                    g_zVideo_FxPass3_ScratchPixels16[(centerY + x) * g_zVideo_FxSurfaceWidth + centerX + y]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY + srcX) * g_zVideo_FxSurfacePitchPixels16 + centerX + srcY];
                    g_zVideo_FxPass3_ScratchPixels16[(centerY + y) * g_zVideo_FxSurfaceWidth + centerX - x]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY + srcY) * g_zVideo_FxSurfacePitchPixels16 + centerX - srcX];
                    g_zVideo_FxPass3_ScratchPixels16[(centerY - x) * g_zVideo_FxSurfaceWidth + centerX + y]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY - srcX) * g_zVideo_FxSurfacePitchPixels16 + centerX + srcY];
                    g_zVideo_FxPass3_ScratchPixels16[(centerY - y) * g_zVideo_FxSurfaceWidth + centerX + x]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY - srcY) * g_zVideo_FxSurfacePitchPixels16 + centerX + srcX];
                    g_zVideo_FxPass3_ScratchPixels16[(centerY + x) * g_zVideo_FxSurfaceWidth + centerX - y]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY + srcX) * g_zVideo_FxSurfacePitchPixels16 + centerX - srcY];
                    g_zVideo_FxPass3_ScratchPixels16[(centerY - y) * g_zVideo_FxSurfaceWidth + centerX - x]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY - srcY) * g_zVideo_FxSurfacePitchPixels16 + centerX - srcX];
                    g_zVideo_FxPass3_ScratchPixels16[(centerY - x) * g_zVideo_FxSurfaceWidth + centerX - y]
                        = g_zVideo_FxSurfacePixels16
                            [(centerY - srcX) * g_zVideo_FxSurfacePitchPixels16 + centerX - srcY];
                }
            }
        } else {
            g_zVideo_FxPass3_ScratchOffsetX = centerX;
            g_zVideo_FxPass3_ScratchOffsetY = centerY;
            int y;
            for (y = -cappedMaxRadius; y <= currentRadius; ++y) {
                int x;
                for (x = y; x <= currentRadius; ++x) {
                    const int distanceSquared = x * x + y * y;
                    if (distanceSquared < maxRadiusSquared && currentRadiusSquared < distanceSquared) {
                        const float distanceSquaredFloat = (float)(distanceSquared);
                        const int distanceBits = *((int*)(&distanceSquaredFloat));
                        int approximateBits = (distanceBits >> 1) + 0x1fc00000;
                        int radiusIndex;
                        if ((int)(*((float*)(&approximateBits))) >= cappedMaxRadius) {
                            radiusIndex = cappedMaxRadius;
                        } else {
                            approximateBits = (distanceBits >> 1) + 0x1fc00000;
                            radiusIndex = (int)(*((float*)(&approximateBits)));
                        }
                        const float scale = sinAmpTable[radiusIndex] * recipTable[radiusIndex];
                        const int srcX = x + (int)((float)(x)*scale);
                        const int srcY = y + (int)((float)(y)*scale);

                        FxPass3CopySurfacePixelToScratchClipped(x, y, srcX, srcY);
                        FxPass3CopySurfacePixelToScratchClipped(y, x, srcY, srcX);
                        FxPass3CopySurfacePixelToScratchClipped(-x, y, -srcX, srcY);
                        FxPass3CopySurfacePixelToScratchClipped(y, -x, srcY, -srcX);
                        FxPass3CopySurfacePixelToScratchClipped(x, -y, srcX, -srcY);
                        FxPass3CopySurfacePixelToScratchClipped(-y, x, -srcY, srcX);
                        FxPass3CopySurfacePixelToScratchClipped(-x, -y, -srcX, -srcY);
                        FxPass3CopySurfacePixelToScratchClipped(-y, -x, -srcY, -srcX);
                    } else {
                        FxPass3CopySurfacePixelToScratchClipped(x, y, x, y);
                        FxPass3CopySurfacePixelToScratchClipped(y, x, y, x);
                        FxPass3CopySurfacePixelToScratchClipped(-x, y, -x, y);
                        FxPass3CopySurfacePixelToScratchClipped(y, -x, y, -x);
                        FxPass3CopySurfacePixelToScratchClipped(x, -y, x, -y);
                        FxPass3CopySurfacePixelToScratchClipped(-y, x, -y, x);
                        FxPass3CopySurfacePixelToScratchClipped(-x, -y, -x, -y);
                        FxPass3CopySurfacePixelToScratchClipped(-y, -x, -y, -x);
                    }
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

        int copyY;
        for (copyY = copyMinY; copyY < copyMaxY; ++copyY) {
            if (copyY > currentRadius || copyY < -currentRadius) {
                unsigned short* src = g_zVideo_FxPass3_ScratchPixels16 + copyY * g_zVideo_FxSurfaceWidth + copyMinX;
                unsigned short* dst = g_zVideo_FxSurfacePixels16 + copyY * g_zVideo_FxSurfacePitchPixels16 + copyMinX;
                int copyX;
                for (copyX = copyMinX; copyX < copyMaxX; ++copyX) {
                    if (copyX > currentRadius || copyX < -currentRadius) {
                        *dst = *src;
                    }
                    ++dst;
                    ++src;
                }
            }
        }
    }
} // namespace zVideo

namespace zVideo
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-buff-blurregioncombined
     * @recoil-artifact defines .text recoil:function:0x48e380: zVideo::buffBlurRegionCombined.
     *
     *
     * Purpose: Applies vertical then horizontal 1-2-1 blur over a 16bpp FX-surface region.
     */
    void __fastcall buffBlurRegionCombined(zVidRect32 * rectOrNull, int)
    {
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
        PixelPackGetRgbMasks(&redMask, &greenMask, &blueMask);
        rbMask = redMask | blueMask;

        int rowDelta = g_zVideo_FxSurfaceWidth - g_zVideo_FxSurfacePitchPixels16;
        unsigned short* src = g_zVideo_FxSurfacePixels16 + savedTop * g_zVideo_FxSurfacePitchPixels16 + savedLeft
            - g_zVideo_FxSurfaceWidth;
        unsigned short* scratch = g_zVideo_FxPass3_ScratchPixels16 + savedTop * g_zVideo_FxSurfaceWidth + savedLeft
            - g_zVideo_FxSurfaceWidth;

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
                        const unsigned int rb = (src[-g_zVideo_FxSurfaceWidth] & rbMask) + ((*src & rbMask) << 1)
                            + (src[g_zVideo_FxSurfaceWidth] & rbMask);
                        const unsigned int green = (src[-g_zVideo_FxSurfaceWidth] & greenMask)
                            + ((*src & greenMask) << 1) + (src[g_zVideo_FxSurfaceWidth] & greenMask);
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
                        const unsigned int rb
                            = (scratch[-1] & rbMask) + ((*scratch & rbMask) << 1) + (scratch[1] & rbMask);
                        const unsigned int green
                            = (scratch[-1] & greenMask) + ((*scratch & greenMask) << 1) + (scratch[1] & greenMask);
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
} // namespace zVideo

namespace zVideo
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-buff-blurregionvertical
     * @recoil-artifact defines .text recoil:function:0x48e670: zVideo::buffBlurRegionVertical.
     *
     *
     * Purpose: Applies the vertical 1-2-1 blur pass over a 16bpp FX-surface region.
     */
    void __fastcall buffBlurRegionVertical(zVidRect32 * rectOrNull, int)
    {
        zVidRect32 rect;
        if (rectOrNull != 0) {
            rect = *rectOrNull;
            if (rect.top < 1) {
                rect.top = 1;
            }
            if (rect.left < 0) {
                rect.left = 0;
            }
            if (rect.bottom > g_zVideo_FxSurfaceHeight - 1) {
                rect.bottom = g_zVideo_FxSurfaceHeight - 1;
            }
            if (rect.right > g_zVideo_FxSurfaceWidth - 1) {
                rect.right = g_zVideo_FxSurfaceWidth - 1;
            }
        } else {
            rect.left = 0;
            rect.top = 1;
            rect.bottom = g_zVideo_FxSurfaceHeight - 1;
            rect.right = g_zVideo_FxSurfaceWidth - 1;
        }

        const int left = rect.left;
        const int top = rect.top;
        const int bottom = rect.bottom;
        const int right = rect.right;
        const int surfaceWidth = g_zVideo_FxSurfaceWidth;
        int columnCount = right - left + 1;
        unsigned int redMask;
        unsigned int greenMask;
        unsigned int blueMask;
        unsigned int rbMask;
        PixelPackGetRgbMasks(&redMask, &greenMask, &blueMask);
        rbMask = redMask | blueMask;

        unsigned short* src = g_zVideo_FxSurfacePixels16 + top * g_zVideo_FxSurfacePitchPixels16 + left;
        unsigned short* scratch = g_zVideo_FxPass3_ScratchPixels16 + top * g_zVideo_FxSurfaceWidth + left;
        const int rowDelta = g_zVideo_FxSurfaceWidth - g_zVideo_FxSurfacePitchPixels16;

        if (top < bottom) {
            int rowCount = bottom - top;
            do {
                if (columnCount > 0) {
                    int count = columnCount;
                    do {
                        const unsigned int rb
                            = (src[-surfaceWidth] & rbMask) + ((*src & rbMask) << 1) + (src[surfaceWidth] & rbMask);
                        const unsigned int green = (src[-surfaceWidth] & greenMask) + ((*src & greenMask) << 1)
                            + (src[surfaceWidth] & greenMask);
                        *scratch = (unsigned short)(((rb >> 2) & rbMask) | ((green >> 2) & greenMask));
                        ++src;
                        ++scratch;
                        --count;
                    } while (count != 0);
                }

                src += rowDelta;
                scratch += rowDelta;
                --rowCount;
            } while (rowCount != 0);
        }

        src = g_zVideo_FxSurfacePixels16 + top * g_zVideo_FxSurfacePitchPixels16 + left;
        scratch = g_zVideo_FxPass3_ScratchPixels16 + top * g_zVideo_FxSurfaceWidth + left;
        if (top < bottom) {
            int rowCount = bottom - top;
            do {
                if (columnCount > 0) {
                    int count = columnCount;
                    do {
                        *src = *scratch;
                        ++src;
                        ++scratch;
                        --count;
                    } while (count != 0);
                }

                src += rowDelta;
                scratch += rowDelta;
                --rowCount;
            } while (rowCount != 0);
        }
    }
} // namespace zVideo

namespace zVideo
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-buff-blurregionhorizontal
     * @recoil-artifact defines .text recoil:function:0x48e870: zVideo::buffBlurRegionHorizontal.
     *
     *
     * Purpose: Applies the horizontal 1-2-1 blur pass over a 16bpp FX-surface region.
     */
    void __fastcall buffBlurRegionHorizontal(zVidRect32 * rectOrNull, int)
    {
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
        PixelPackGetRgbMasks(&redMask, &greenMask, &blueMask);
        rbMask = redMask | blueMask;

        unsigned short* src = g_zVideo_FxSurfacePixels16 + top * g_zVideo_FxSurfacePitchPixels16 + left;
        unsigned short* scratch = g_zVideo_FxPass3_ScratchPixels16 + top * g_zVideo_FxSurfaceWidth + left;
        const int rowDelta = g_zVideo_FxSurfaceWidth - g_zVideo_FxSurfacePitchPixels16;
        if (top >= bottom) {
            return;
        }

        int y = bottom - top;
        do {
            unsigned short* srcStart = src;
            unsigned short* scratchStart = scratch;
            if (columnCount > 0) {
                int count = columnCount;
                do {
                    const unsigned int rb = (src[-1] & rbMask) + ((src[0] & rbMask) << 1) + (src[1] & rbMask);
                    const unsigned int green
                        = (src[-1] & greenMask) + ((src[0] & greenMask) << 1) + (src[1] & greenMask);
                    ++src;
                    ++scratch;
                    scratch[-1] = (unsigned short)(((rb >> 2) & rbMask) | ((green >> 2) & greenMask));
                    --count;
                } while (count != 0);
            }

            if (columnCount > 0) {
                int count = columnCount;
                do {
                    *srcStart++ = *scratchStart++;
                    --count;
                } while (count != 0);
            }

            src += rowDelta;
            scratch += rowDelta;
            --y;
        } while (y != 0);
    }
} // namespace zVideo

namespace zVideo
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-buff-blurregionbymode
     * @recoil-artifact defines .text recoil:function:0x48ea00: zVideo::buffBlurRegionByMode.
     * @recoil-match byte
     *
     * Purpose: Dispatches a blur-region request to horizontal, vertical, or combined mode.
     */
    void __fastcall buffBlurRegionByMode(zVidRect32 * rectOrNull, int mode)
    {
        if (mode == 1) {
            buffBlurRegionHorizontal(rectOrNull, mode);
        } else if (mode == 2) {
            buffBlurRegionVertical(rectOrNull, mode);
        } else {
            buffBlurRegionCombined(rectOrNull, mode);
        }
    }
} // namespace zVideo

namespace zVideo_FxSurface
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-applybluetintrect
     * @recoil-artifact defines .text recoil:function:0x48ea20: zVideo_FxSurface::ApplyBlueTintRect.
     *
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
     * Purpose: provide the recovered zVideo_FxSurface::ApplyBlueTintRect behavior.
     */
    void __fastcall ApplyBlueTintRect(zVidRect32 * rectOrNull)
    {
        zVidRect32 clipRect;
        if (rectOrNull != 0) {
            clipRect.left = rectOrNull->left;
            clipRect.top = rectOrNull->top;
            clipRect.right = rectOrNull->right;
            clipRect.bottom = rectOrNull->bottom;
        } else {
            clipRect.top = 0;
            clipRect.left = 0;
            clipRect.bottom = g_zVideo_FxSurfaceHeight - 1;
            clipRect.right = g_zVideo_FxSurfaceWidth - 1;
        }

        unsigned int redMask;
        unsigned int greenMask;
        unsigned int blueMask;
        zVideo::PixelPackGetRgbMasks(&redMask, &greenMask, &blueMask);

        if (g_zVideo_ActiveRendererPath != 0) {
            zVideo_dd3d::QueueSolidQuad(blueMask, 0.3, &clipRect);
            return;
        }

        greenMask |= greenMask << 16;
        redMask |= redMask << 16;
        blueMask |= blueMask << 16;
        const int rowPairCount = (clipRect.right - clipRect.left - 1) >> 1;
        unsigned short* row
            = g_zVideo_FxSurfacePixels16 + clipRect.top * g_zVideo_FxSurfacePitchPixels16 + clipRect.left;
        for (int y = clipRect.top; y < clipRect.bottom; ++y) {
            unsigned int* pixelPair = (unsigned int*)(row);
            int remainingPairs = rowPairCount;
            do {
                *pixelPair = ((*pixelPair >> 1) & (((greenMask >> 1) & greenMask) | ((redMask >> 1) & redMask)))
                    | (*pixelPair & blueMask);
                ++pixelPair;
            } while (remainingPairs-- != 0);

            row += g_zVideo_FxSurfacePitchPixels16;
        }
    }
} // namespace zVideo_FxSurface

namespace zVideo_FxSurface
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-applygreenmaskrect
     * @recoil-artifact defines .text recoil:function:0x48eb80: zVideo_FxSurface::ApplyGreenMaskRect.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
     * Purpose: provide the recovered zVideo_FxSurface::ApplyGreenMaskRect behavior.
     */
    void __fastcall ApplyGreenMaskRect(zVidRect32 * rectOrNull)
    {
        zVidRect32 clipRect;
        if (rectOrNull != 0) {
            clipRect.left = rectOrNull->left;
            clipRect.top = rectOrNull->top;
            clipRect.right = rectOrNull->right;
            clipRect.bottom = rectOrNull->bottom;
        } else {
            clipRect.top = 0;
            clipRect.left = 0;
            clipRect.bottom = g_zVideo_FxSurfaceHeight - 1;
            clipRect.right = g_zVideo_FxSurfaceWidth - 1;
        }

        unsigned int redMask;
        unsigned int greenMask;
        unsigned int blueMask;
        zVideo::PixelPackGetRgbMasks(&redMask, &greenMask, &blueMask);

        if (g_zVideo_ActiveRendererPath != 0) {
            zVideo_dd3d::QueueSolidQuad(greenMask, 0.3, &clipRect);
            return;
        }

        redMask |= redMask << 16;
        greenMask |= greenMask << 16;
        blueMask |= blueMask << 16;
        const int rowPairCount = (clipRect.right - clipRect.left - 1) >> 1;
        unsigned short* row
            = g_zVideo_FxSurfacePixels16 + clipRect.top * g_zVideo_FxSurfacePitchPixels16 + clipRect.left;
        for (int y = clipRect.top; y < clipRect.bottom; ++y) {
            unsigned int* pixelPair = (unsigned int*)(row);
            int remainingPairs = rowPairCount;
            do {
                *pixelPair &= greenMask;
                ++pixelPair;
            } while (remainingPairs-- != 0);

            row += g_zVideo_FxSurfacePitchPixels16;
        }
    }
} // namespace zVideo_FxSurface

namespace zVideo_FxSurface
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-drawcoloredlinesbatch
     * @recoil-artifact defines .text recoil:function:0x48ec90: zVideo_FxSurface::DrawColoredLinesBatch.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
     * Purpose: provide the recovered zVideo_FxSurface::DrawColoredLinesBatch behavior.
     */
    void __fastcall DrawColoredLinesBatch(zVideoFxColoredLineRecord * lines, int count, zVidRect32* clipRectOrNull)
    {
        zVidRect32 clipRect;
        if (clipRectOrNull != 0) {
            clipRect.left = clipRectOrNull->left;
            clipRect.top = clipRectOrNull->top;
            clipRect.right = clipRectOrNull->right;
            clipRect.bottom = clipRectOrNull->bottom;
        } else {
            clipRect.top = 0;
            clipRect.left = 0;
            clipRect.bottom = g_zVideo_FxSurfaceHeight - 1;
            clipRect.right = g_zVideo_FxSurfaceWidth - 1;
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
            zVideoFxColoredLineRecord* line = &lines[index];
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

namespace zVideo_FxSurface
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-drawalphablendedline
     * @recoil-artifact defines .text recoil:function:0x48ed60: zVideo_FxSurface::DrawAlphaBlendedLine.
     *
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
     * Purpose: provide the recovered zVideo_FxSurface::DrawAlphaBlendedLine behavior.
     */
    void __fastcall DrawAlphaBlendedLine(
        zVidRect32 * clipRect,
        int x1,
        int y1,
        int x0,
        int y0,
        unsigned short color16,
        float alphaEnd,
        float alphaStart,
        int clipInset
    )
    {
        int dx = x0 - x1;
        int dy = y0 - y1;
        const int left = clipRect->left + clipInset;
        const int top = clipRect->top + clipInset;
        const int right = clipRect->right - clipInset;
        const int bottom = clipRect->bottom - clipInset;
        int startOutCode = 0;
        if (x1 < left) {
            startOutCode |= 1;
        }
        if (x1 > right) {
            startOutCode |= 2;
        }
        if (y1 < top) {
            startOutCode |= 4;
        }
        if (y1 > bottom) {
            startOutCode |= 8;
        }
        int endOutCode = 0;
        if (x0 < left) {
            endOutCode |= 1;
        }
        if (x0 > right) {
            endOutCode |= 2;
        }
        if (y0 < top) {
            endOutCode |= 4;
        }
        if (y0 > bottom) {
            endOutCode |= 8;
        }
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
                y1 += (int)((float)(left - x1) * slopeYPerX);
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
                y1 += (int)((float)(right - x1) * slopeYPerX);
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
                y0 += (int)((float)(left - x0) * slopeYPerX);
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
                y0 += (int)((float)(right - x0) * slopeYPerX);
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
                x1 += (int)((float)(top - y1) * slopeXPerY);
                y1 = top;
            } else if (y1 > bottom) {
                x1 += (int)((float)(bottom - y1) * slopeXPerY);
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
                x0 += (int)((float)(top - y0) * slopeXPerY);
                y0 = top;
            } else if (y0 > bottom) {
                x0 += (int)((float)(bottom - y0) * slopeXPerY);
                y0 = bottom;
            }
        }

        dx = x0 - x1;
        dy = y0 - y1;
        const int pitchPixels = zRndr::g_pitchBytes >> 1;
        unsigned short* pixel = g_zVideo_FxSurfacePixels16 + pitchPixels * y1 + x1;
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
        int alphaFixed = (int)(alphaStart * 255.0f) << 16;
        if (dx > dy) {
            int err = dx >> 1;
            int steps = dx + 1;
            const int alphaStep = (int)(((alphaEnd - alphaStart) / (float)(steps)) * 16777215.0f);
            while (steps != 0) {
                const int alpha = alphaFixed >> 16;
                if (clipInset > 0) {
                    unsigned short* spanPixel = pixel;
                    int spanCount = clipInset;
                    while (spanCount != 0) {
                        const int dstValue = (int)(*spanPixel);
                        const int colorValue = (int)(packedColor);
                        if (zRndr::g_pixelPackGreenBits == 5) {
                            if (alpha > 7) {
                                if (alpha >= 252) {
                                    *spanPixel = packedColor;
                                } else {
                                    const int redDelta = (((colorValue & 0x7c00) - (dstValue & 0x7c00)) * alpha) >> 8;
                                    const int greenDelta = (((colorValue & 0x03e0) - (dstValue & 0x03e0)) * alpha) >> 8;
                                    const int blueDelta = (((colorValue & 0x001f) - (dstValue & 0x001f)) * alpha) >> 8;
                                    *spanPixel = (unsigned short)(dstValue + (redDelta & 0xfc00) + (greenDelta & 0xffe0)
                                        + blueDelta);
                                }
                            }
                        } else if (alpha > 3) {
                            if (alpha >= 252) {
                                *spanPixel = packedColor;
                            } else {
                                const int redDelta = (((colorValue & 0xf800) - (dstValue & 0xf800)) * alpha) >> 8;
                                const int greenDelta = (((colorValue & 0x07e0) - (dstValue & 0x07e0)) * alpha) >> 8;
                                const int redApplied = dstValue + (redDelta & 0xfffff800);
                                const int blueDelta = (((colorValue & 0x001f) - (redApplied & 0x001f)) * alpha) >> 8;
                                *spanPixel = (unsigned short)(redApplied + (greenDelta & 0xffe0) + blueDelta);
                            }
                        }
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
            const int alphaStep = (int)(((alphaEnd - alphaStart) / (float)(steps)) * 16777215.0f);
            while (steps != 0) {
                const int alpha = alphaFixed >> 16;
                if (clipInset > 0) {
                    unsigned short* spanPixel = pixel;
                    int spanCount = clipInset;
                    while (spanCount != 0) {
                        const int dstValue = (int)(*spanPixel);
                        const int colorValue = (int)(packedColor);
                        if (zRndr::g_pixelPackGreenBits == 5) {
                            if (alpha > 7) {
                                if (alpha >= 252) {
                                    *spanPixel = packedColor;
                                } else {
                                    const int redDelta = (((colorValue & 0x7c00) - (dstValue & 0x7c00)) * alpha) >> 8;
                                    const int greenDelta = (((colorValue & 0x03e0) - (dstValue & 0x03e0)) * alpha) >> 8;
                                    const int blueDelta = (((colorValue & 0x001f) - (dstValue & 0x001f)) * alpha) >> 8;
                                    *spanPixel = (unsigned short)(dstValue + (redDelta & 0xfc00) + (greenDelta & 0xffe0)
                                        + blueDelta);
                                }
                            }
                        } else if (alpha > 3) {
                            if (alpha >= 252) {
                                *spanPixel = packedColor;
                            } else {
                                const int redDelta = (((colorValue & 0xf800) - (dstValue & 0xf800)) * alpha) >> 8;
                                const int greenDelta = (((colorValue & 0x07e0) - (dstValue & 0x07e0)) * alpha) >> 8;
                                const int redApplied = dstValue + (redDelta & 0xfffff800);
                                const int blueDelta = (((colorValue & 0x001f) - (redApplied & 0x001f)) * alpha) >> 8;
                                *spanPixel = (unsigned short)(redApplied + (greenDelta & 0xffe0) + blueDelta);
                            }
                        }
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
} // namespace zVideo_FxSurface
