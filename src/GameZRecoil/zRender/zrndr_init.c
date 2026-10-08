// zRender compilation unit between zrndr_fx.c and zrndr_poly.c, inferred from
// the retail object boundary [0x48f500, 0x492000): its .rdata pooled
// constants [0x4d2db8, 0x4d2de0) duplicate values that the neighbouring
// zRender objects pool separately. Its globals live in the linker common
// area. Original filename unresolved; zrndr_init.c is a provisional name
// (2026-10-02).

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
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-zvideo-pfnbltsourcetoprimary
 * @recoil-artifact defines .data recoil:data:0x6320ac: g_zVideo_pfnBltSourceToPrimary.
 * BN xrefs: zVideo/zRndr setup stores the active source-to-primary blit
 * callback before software HUD/renderer paths dispatch through it.
 * Purpose: renderer-selected 16-bit source blit callback for primary output.
 */
zVideo_BltSourceToPrimaryProc g_zVideo_pfnBltSourceToPrimary = 0;
}

// Option names hud.cpp defines; retail reads these globals (0x4da834,
// 0x4da888), not literals.
extern "C" char g_zOpt_OptionName_GfxFlagsHw[];
extern "C" char g_zOpt_OptionName_GfxFlagsSw[];

namespace zSys
{
    int __cdecl CheckCpuSignatureMask();
}

namespace zVid_Image
{
    void __fastcall BlitToFramebufferClipped(
        zVidImagePartial * image,
        int dstX,
        int dstY,
        unsigned short clipFlags,
        zVidRect32* srcRect
    );
}

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-framebuffer
     * @recoil-artifact defines .data recoil:data:0x632050: gRndr_pFrameBuffer.
     * BN xrefs: zRndr active-region setup stores this pointer; queued raster,
     * immediate line, circle, lens-flare, and span-occlusion sample paths load it
     * as the active software framebuffer before dispatching row/pixel callbacks.
     * Default software render target bank from zRndr_Draw.cpp. BN names the clipped-framebuffer
     * globals at 0x632050, 0x632054, 0x632058, and 0x63205c; lens-flare and span leaves consume
     * them as the active 16-bit framebuffer.
     * Purpose: active 16-bit software renderer framebuffer base.
     */
    void* g_frameBuffer = 0;
    int g_activeRegionWidth = 0;
    int g_activeRegionHeight = 0;
    int g_pitchBytes = 0;
    int g_bytesPerPixel = 0;
    int g_videoStrideMirror0 = 0;
    int g_videoStrideMirror1 = 0;
    ActiveRegionRectPartial g_activeRegionRect = { 0 };
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-scanconvertmode
     * @recoil-artifact defines .data recoil:data:0x57dac8: g_scanConvertMode.
     * Purpose: Store the active zRndr scan-conversion mode consumed by queued raster paths.
     */
    int g_scanConvertMode = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-perspectivetextureenabled
     * @recoil-artifact defines .data recoil:data:0x57dacc: gRndr_PerspectiveTextureEnabled.
     * BN xrefs: zRndr::InitGlobals enables this flag at startup; zModel render
     * paths toggle it while selecting camera/projection setup for textured model
     * submission.
     * Purpose: runtime perspective-texture enable flag for zRndr draw paths.
     */
    int g_perspectiveTextureEnabled = 0;
    int g_perspectiveTextureDeltaXInput = 0;
    int g_perspectiveTextureDeltaXShift = 0;
    int g_perspectiveTextureDeltaXPow2 = 0;
    int g_perspectiveTextureDeltaXBytes = 0;
    float g_perspectiveTextureDeltaXPow2F = 0.0f;
    float g_perspectiveTextureFarZInv = 0.0f;
    int g_perspectiveAdaptiveMinSpan = 0;
    int g_perspectiveAdaptiveMaxSpan = 0;
    float g_perspectiveAdaptiveSlope = 0.0f;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x57dac0
     * @recoil-artifact defines .data recoil:data:0x57dac0: g_inverseDepthBias.
     * Purpose: Cache the inverse-depth bias applied when queued spans and lens-flare samples write depth.
     */
    float g_inverseDepthBias = 0.0f;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x57dac4
     * @recoil-artifact defines .data recoil:data:0x57dac4: g_inverseDepthScale.
     * Purpose: Cache the inverse-depth scale applied with g_inverseDepthBias for software raster depth.
     */
    float g_inverseDepthScale = 0.0f;
    float g_spanDepthBias = 0.0f;
    float g_spanDepthBiasPlusOne = 0.0f;
    float g_spanDepthBiasPlusOneInv = 0.0f;
    // BN BSS order: Color (0x631dd0), Staged (0x631e70), Direct (0x631f10),
    // Active (0x631fb0).
    FogParamsPartial g_fogColorParams = { 0 };
    FogParamsPartial g_fogTargetParamsStaged = { 0 };
    FogParamsPartial g_fogTargetParamsDirect = { 0 };
    FogParamsPartial g_fogParamsActive = { 0 };
    // zRndr span-occlusion subsystem state from zRndr_Draw.cpp. BN names these as
    // gRndr_Span* globals; g_spanIterPrevLink stores the previous node observed in
    // insertion walkers even though one BN data declaration renders it as a link
    // pointer.
    SpanOccluderPolyPartial g_spanOccluderPolys[8] = { 0 };
    int g_spanOccluderPolyCount = 0;
    SpanNodePartial* g_spanAllocCursor = 0;
    SpanNodePartial** g_spanColumnHeadTable = 0;
    SpanNodePartial* g_spanPoolBase = 0;
    SpanNodePartial* g_spanLastNode = 0;
    SpanNodePartial* g_spanIterNode = 0;
    SpanNodePartial* g_spanIterPrevLink = 0;
    int g_spanReservedWriteOnly = 0;
    int g_spanColumnCount = 0;
    int g_spanColumnCountPadded = 0;
    SpanBuildProc g_pfnBuildSpanList = 0;
    SpanBuildProc g_pfnBuildSpanListSecondary = 0;
    // zRndr cached pixel-pack bank. SelectSpanRoutines refreshes this authored
    // cache through zVideo PixelPack getters; fog and span color math consume the
    // cached zRndr scalars rather than reading the upstream provider global.
    int g_pixelPackRedBits = 0;
    int g_pixelPackGreenBits = 0;
    int g_pixelPackBlueBits = 0;
    unsigned int g_pixelPackRedMask = 0;
    unsigned int g_pixelPackGreenMask = 0;
    unsigned int g_pixelPackBlueMask = 0;
    int g_pixelPackRedShift = 0;
    int g_pixelPackGreenShift = 0;
    int g_pixelPackBlueShift = 0;
    // Span callback dispatch bank. BN orders these as the gRndr_pfn* BSS block
    // installed by SelectSpanRoutines and caller-specific draw paths.
    SpanRoutineProc g_pfnSelectedSpanOp = 0;
    FlatImmediateSpanProc g_pfnFlatImmediateSpanOp = 0;
    TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode0 = 0;
    TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode1 = 0;
    TexturedQueuedSpanProc g_pfnSelectedSpanOp_Mode0 = 0;
    TexturedQueuedSpanProc g_pfnSelectedSpanOp_Mode1 = 0;
    TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode0 = 0;
    TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode1 = 0;
    TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode0 = 0;
    TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode1 = 0;
    TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode0 = 0;
    TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode1 = 0;
    TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode0 = 0;
    TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode0 = 0;
    TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode1 = 0;
    TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode1 = 0;
    ImmediateRaster4Proc g_pfnImmediateRaster4 = 0;
    ImmediateRasterSegmentedProc g_pfnImmediateRasterReserved = 0;
    ImmediateRaster5Proc g_pfnImmediateRaster5 = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-pfnpointopcandidate
     * @recoil-artifact defines .data recoil:data:0x6320fc: gRndr_pfnPointOpCandidate.
     * BN xrefs: zRndr::SelectSpanRoutines writes the candidate point operation
     * next to the active point callback selected for immediate/circle sample
     * drawing.
     * Purpose: staged software point operation selected by zRndr span routines.
     */
    PointOpProc g_pfnPointOpCandidate = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-pfnpointopactive
     * @recoil-artifact defines .data recoil:data:0x632100: gRndr_pfnPointOpActive.
     * BN xrefs: zRndr::SelectSpanRoutines installs zRndrPlotPixel16; span
     * occlusion sample and circle octant emitters load this fastcall callback with
     * gRndr_pFrameBuffer plus y/x/color stack arguments.
     * Purpose: active software point operation used by sample and circle drawing.
     */
    PointOpProc g_pfnPointOpActive = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-pfntexturedqueuedfinalize
     * @recoil-artifact defines .data recoil:data:0x632104: gRndr_pfnTexturedQueuedFinalize.
     * Purpose: Holds the selected scalar/MMX textured queued span finalizer.
     */
    SpanRoutineProc g_pfnTexturedQueuedFinalize = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-pfntexturedqueuedfinalizealt
     * @recoil-artifact defines .data recoil:data:0x632108: gRndr_pfnTexturedQueuedFinalizeAlt.
     * Purpose: Holds the optional MMX texture mask setup callback for queued spans.
     */
    SpanRoutineProc g_pfnTexturedQueuedFinalizeAlt = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-texturemipselectionenabled
     * @recoil-artifact defines .data recoil:data:0x63209c: gRndr_TextureMipSelectionEnabled.
     * zRndr texture-mip runtime selector globals. BN places these adjacent int32 data entries at
     * 0x63209c..0x6320a0, after the render-state init bank ending at 0x632098 and before the span
     * callback/function-pointer bank at 0x6320a4.
     * Purpose: Enable texture mip variant selection at runtime.
     */
    int g_textureMipSelectionEnabled = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-texturemipreservedwriteonly
     * @recoil-artifact defines .data recoil:data:0x6320a0: gRndr_TextureMipReservedWriteOnly.
     * Purpose: Preserve the adjacent InitGlobals-cleared texture mip companion slot.
     */
    int g_textureMipReservedWriteOnly = 0;
    // zRndr::InitGlobals-only render-state latch bank. BN orders these eight
    // zero-initialized int32 globals as 0x63207c..0x632098; InitGlobals writes the
    // 0x632088..0x632098 tail first, then the 0x63207c..0x632084 head. Current
    // BN xrefs show no other readers or writers.
    int g_renderStateReservedWriteOnly = 0;
    int g_initField00 = 0;
    int g_initField04 = 0;
    int g_initField08 = 0;
    int g_initField0C = 0;
    int g_initField10 = 0;
    int g_initField14 = 0;
    int g_renderStateReadyWriteOnlyFlag = 0;
    int g_defaultGraphicsFlags = 0;
    int* g_graphicsFlags = 0;
} // namespace zRndr

namespace zVid_Image
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-blittoactivetarget
     * @recoil-artifact defines .text recoil:function:0x48f500: zVid_Image::BlitToActiveTarget.
     * @recoil-match byte
     *
     * Source file evidence: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
     * Purpose: Route an image blit to the primary DirectDraw surface when active, otherwise dispatch through the
     * selected source-to-primary blitter.
     */
    void __fastcall
    BlitToActiveTarget(zVidImagePartial * image, int dstX, int dstY, unsigned short colorKey, zVidRect32* srcRect)
    {
        if (image->surface != 0 && zRndr::g_frameBuffer == zVideo::GetPrimarySurfacePixels()) {
            zVideo_buff::BltSourceToPrimaryClipped(image, dstX, dstY, colorKey, srcRect);
            return;
        }

        g_zVideo_pfnBltSourceToPrimary(image, dstX, dstY, colorKey, srcRect);
    }
} // namespace zVid_Image

namespace zVid_Image
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-blittoframebufferclipped
     * @recoil-artifact defines .text recoil:function:0x48f560: zVid_Image::BlitToFramebufferClipped.
     *
     *
     * Source file evidence: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
     * Purpose: Clip and blit a zVid image into zRndr's active 16-bit framebuffer.
     *
     * The 565/555 alpha-map and color-key branches follow BN's zvid_buff.c
     * assembly-visible contracts; BN loses some row-cursor identities in the long
     * memcpy and paletted paths, so source keeps explicit typed row cursors.
     */
    void __fastcall BlitToFramebufferClipped(
        zVidImagePartial * image,
        int dstX,
        int dstY,
        unsigned short clipFlags,
        zVidRect32* srcRect
    )
    {
        int srcWidth;
        int srcHeight;
        if (srcRect == 0) {
            srcWidth = image->width;
            srcHeight = image->height;
        } else {
            srcWidth = srcRect->right - srcRect->left;
            srcHeight = srcRect->bottom - srcRect->top;
        }

        if (srcWidth < 0 || srcWidth > 0x800 || srcHeight < 0 || srcHeight > 0x800) {
            return;
        }

        // Retail clamps the inclusive right/bottom edges with unsigned compares (jb/jbe).
        int left = dstX;
        int top = dstY;
        unsigned int right = dstX + srcWidth - 1;
        unsigned int bottom = dstY + srcHeight - 1;
        if (dstX + srcWidth <= 0 || dstX >= zRndr::g_activeRegionWidth || dstY + srcHeight <= 0
            || dstY >= zRndr::g_activeRegionHeight) {
            return;
        }

        if (left < 0) {
            left = 0;
        }
        if (right >= (unsigned int)(zRndr::g_activeRegionWidth)) {
            right = zRndr::g_activeRegionWidth - 1;
        }
        if (top < 0) {
            top = 0;
        }
        // Retail (0x48f61d: cmp; jbe) clamps the bottom edge only once it passes the active height.
        if (bottom > (unsigned int)(zRndr::g_activeRegionHeight)) {
            bottom = zRndr::g_activeRegionHeight - 1;
        }

        const int width = right - left + 1;
        int height = bottom - top + 1;
        const int framebufferPitch = (int)((unsigned int)(zRndr::g_pitchBytes) >> 1);
        const int sourcePitch = image->pitchWords;
        int sourceX;
        int sourceY;
        if (srcRect == 0) {
            sourceX = left - dstX;
            sourceY = top - dstY;
        } else {
            sourceX = srcRect->left + left - dstX;
            sourceY = srcRect->top + top - dstY;
        }

        unsigned short* dstRow = (unsigned short*)(zRndr::g_frameBuffer) + framebufferPitch * top + left;
        if (image->palette == 0) {
            unsigned short* srcRow = (unsigned short*)(image->pixels) + sourcePitch * sourceY + sourceX;
            if (image->alphaMap != 0) {
                unsigned char* alphaRow = (unsigned char*)(image->alphaMap) + sourcePitch * sourceY + sourceX;
                if (zRndr::g_pixelPackGreenBits == 6) {
                    for (int row = 0; row < height; ++row) {
                        unsigned short* dst = dstRow;
                        unsigned char* alpha = alphaRow;
                        for (int column = 0; column < width; ++column) {
                            if (*alpha > 3) {
                                if (*alpha >= 0xfc) {
                                    *dst = srcRow[column];
                                } else {
                                    int dstColor = (short)(*dst);
                                    const int srcColor = (short)(srcRow[column]);
                                    int greenDelta = ((srcColor & 0x07e0) - (dstColor & 0x07e0)) * *alpha;
                                    int redDelta = ((srcColor & 0xf800) - (dstColor & 0xf800)) * *alpha;
                                    redDelta = (redDelta >> 8) & 0xfffff800;
                                    greenDelta = (greenDelta >> 8) & 0xffffffe0;
                                    dstColor += redDelta;
                                    int blueDelta = ((srcColor & 0x001f) - (dstColor & 0x001f)) * *alpha;
                                    blueDelta >>= 8;
                                    blueDelta += greenDelta;
                                    dstColor += blueDelta;
                                    *dst = (unsigned short)(dstColor);
                                }
                            }

                            ++dst;
                            ++alpha;
                        }

                        dstRow += framebufferPitch;
                        srcRow += sourcePitch;
                        alphaRow += sourcePitch;
                    }
                    return;
                }

                for (int row_1 = 0; row_1 < height; ++row_1) {
                    unsigned short* dst = dstRow;
                    unsigned char* alpha = alphaRow;
                    for (int column_1 = 0; column_1 < width; ++column_1) {
                        if (*alpha > 7) {
                            if (*alpha >= 0xfc) {
                                *dst = srcRow[column_1];
                            } else {
                                const int dstColor = (short)(*dst);
                                const int srcColor = (short)(srcRow[column_1]);
                                int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * *alpha) >> 8;
                                int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * *alpha) >> 8;
                                redDelta &= 0xfffffc00;
                                greenDelta &= 0xffffffe0;
                                // Retail folds the red step into a 16-bit read-modify-write, then re-reads the
                                // alpha byte for the blue term (0x48f85d, 0x48f869).
                                *dst += redDelta;
                                const int blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * *alpha) >> 8;
                                *dst += blueDelta + greenDelta;
                            }
                        }

                        ++dst;
                        ++alpha;
                    }

                    dstRow += framebufferPitch;
                    srcRow += sourcePitch;
                    alphaRow += sourcePitch;
                }
                return;
            }

            if ((image->formatFlagsPacked & 0x02) != 0) {
                for (int row_2 = 0; row_2 < height; ++row_2) {
                    for (int column_2 = 0; column_2 < width; ++column_2) {
                        if (srcRow[column_2] != clipFlags) {
                            dstRow[column_2] = srcRow[column_2];
                        }
                    }

                    dstRow += framebufferPitch;
                    srcRow += sourcePitch;
                }
                return;
            }

            if (left == 0 && right == (unsigned int)(zRndr::g_activeRegionWidth - 1)
                && framebufferPitch == sourcePitch) {
                // Retail [0x48f94f, 0x48f95a): one rep movsd of (width * height) >> 1 dwords.
                memcpy(dstRow, srcRow, (size_t)((width * height) >> 1) * sizeof(unsigned int));
                return;
            }

            if ((width & 1) == 0) {
                for (int row_3 = 0; row_3 < height; ++row_3) {
                    // Retail [0x48f98a, 0x48f995): rep movsd of width / 2 dwords per row.
                    memcpy(dstRow, srcRow, (size_t)(width >> 1) * sizeof(unsigned int));
                    dstRow += framebufferPitch;
                    srcRow += sourcePitch;
                }
                return;
            }

            for (int row_4 = 0; row_4 < height; ++row_4) {
                // Retail [0x48f9c8, 0x48f9d4): rep movsw of width words per row.
                memcpy(dstRow, srcRow, (size_t)(width) * sizeof(unsigned short));
                dstRow += framebufferPitch;
                srcRow += sourcePitch;
            }
            return;
        }

        unsigned char* srcRow8 = (unsigned char*)(image->pixels) + sourceX + sourcePitch * sourceY;
        const unsigned short* palette = (const unsigned short*)(image->palette);
        if (image->alphaMap != 0) {
            unsigned char* alphaRow8 = (unsigned char*)(image->alphaMap) + sourceX + sourcePitch * sourceY;
            if (zRndr::g_pixelPackGreenBits == 6) {
                for (; height > 0; --height) {
                    unsigned short* dst = dstRow;
                    unsigned char* alpha = alphaRow8;
                    for (int column_3 = 0; column_3 < width; ++column_3) {
                        if (*alpha > 3) {
                            if (*alpha >= 0xfc) {
                                *dst = palette[srcRow8[column_3]];
                            } else {
                                int dstColor = (short)(*dst);
                                const int srcColor = palette[srcRow8[column_3]];
                                int greenDelta = ((srcColor & 0x07e0) - (dstColor & 0x07e0)) * *alpha;
                                int redDelta = ((srcColor & 0xf800) - (dstColor & 0xf800)) * *alpha;
                                redDelta = (redDelta >> 8) & 0xfffff800;
                                greenDelta = (greenDelta >> 8) & 0xffffffe0;
                                dstColor += redDelta;
                                int blueDelta = ((srcColor & 0x001f) - (dstColor & 0x001f)) * *alpha;
                                blueDelta >>= 8;
                                blueDelta += greenDelta;
                                dstColor += blueDelta;
                                *dst = (unsigned short)(dstColor);
                            }
                        }

                        ++dst;
                        ++alpha;
                    }

                    dstRow += framebufferPitch;
                    srcRow8 += sourcePitch;
                    alphaRow8 += sourcePitch;
                }
                return;
            }

            for (int row_5 = 0; row_5 < height; ++row_5) {
                unsigned short* dst = dstRow;
                unsigned char* alpha = alphaRow8;
                for (int column_4 = 0; column_4 < width; ++column_4) {
                    if (*alpha > 7) {
                        if (*alpha >= 0xfc) {
                            *dst = palette[srcRow8[column_4]];
                        } else {
                            const int dstColor = (short)(*dst);
                            const int srcColor = palette[srcRow8[column_4]];
                            int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * *alpha) >> 8;
                            redDelta &= 0xfffffc00;
                            *dst += redDelta;
                            int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * *alpha) >> 8;
                            greenDelta &= 0xffffffe0;
                            const int blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * *alpha) >> 8;
                            *dst += greenDelta + blueDelta;
                        }
                    }

                    ++dst;
                    ++alpha;
                }

                dstRow += framebufferPitch;
                srcRow8 += sourcePitch;
                alphaRow8 += sourcePitch;
            }
            return;
        }

        if ((image->formatFlagsPacked & 0x02) != 0) {
            for (int row_6 = 0; row_6 < height; ++row_6) {
                for (int column_5 = 0; column_5 < width; ++column_5) {
                    if ((unsigned short)(srcRow8[column_5]) != clipFlags) {
                        dstRow[column_5] = palette[srcRow8[column_5]];
                    }
                }

                dstRow += framebufferPitch;
                srcRow8 += sourcePitch;
            }
            return;
        }

        if (left == 0 && right == (unsigned int)(zRndr::g_activeRegionWidth - 1) && framebufferPitch == sourcePitch) {
            // Retail [0x48fce7, 0x48fd0b): one palette-expand loop over width * height pixels.
            const int pixelCount = height * width;
            for (int i = 0; i < pixelCount; ++i) {
                dstRow[i] = palette[srcRow8[i]];
            }
            return;
        }

        for (int row_7 = 0; row_7 < height; ++row_7) {
            // Retail [0x48fd27, 0x48fd4b): the same palette-expand loop over one row.
            for (int column_6 = 0; column_6 < width; ++column_6) {
                dstRow[column_6] = palette[srcRow8[column_6]];
            }

            dstRow += framebufferPitch;
            srcRow8 += sourcePitch;
        }
    }
} // namespace zVid_Image

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-initglobals
     * @recoil-artifact defines .text recoil:function:0x48fd80: zRndr::InitGlobals
     * @recoil-match byte
     *
     * Purpose: Initialize renderer span, queue, fog, and dispatch globals to their startup state.
     */
    int __cdecl InitGlobals()
    {
        g_spanAllocCursor = 0;
        g_spanColumnHeadTable = 0;
        g_spanPoolBase = 0;
        g_spanLastNode = 0;
        g_spanIterNode = 0;
        g_spanIterPrevLink = 0;
        g_spanReservedWriteOnly = 0;
        g_spanColumnCount = 0;

        SetPerspectiveAdaptiveCorrection(0.0001f);

        g_perspectiveTextureDeltaXInput = 0x20;
        g_perspectiveTextureDeltaXPow2 = 0x20;
        g_perspectiveTextureDeltaXShift = 5;
        g_perspectiveTextureDeltaXPow2F = 32.0f;
        g_perspectiveTextureFarZInv = 0.00333f;
        g_perspectiveAdaptiveMinSpan = 0;
        g_inverseDepthBias = 0.0f;
        g_inverseDepthScale = 1.0f;
        g_scanConvertMode = 1;
        g_perspectiveTextureEnabled = 1;
        g_transparentQueueCount = 0;
        g_overwriteQueueCount = 0;
        g_overlayBlendEnabled = 0;
        g_lensFlareSampleQueueCount = 0;
        g_lensFlareVisibleSampleCount = 0;

        zColorRgb color;
        color.blue = 0.04f;
        color.green = 0.04f;
        color.red = 0.04f;
        FogColorSetRgb01Clamped(&color);
        FogColorSetRgb01Clamped((zColorRgb*)(g_fogColorParams.colorRgb01));
        g_fogTargetParamsStaged = g_fogColorParams;
        g_fogParamsActive = g_fogColorParams;

        g_textureMipSelectionEnabled = 1;
        g_textureMipReservedWriteOnly = 0;
        g_frameBuffer = 0;
        g_activeRegionWidth = 0;
        g_activeRegionHeight = 0;
        g_pitchBytes = 0;
        g_bytesPerPixel = 1;
        g_videoStrideMirror0 = 1;
        g_videoStrideMirror1 = 1;
        g_activeRegionRect.right = 0;
        g_activeRegionRect.x = 0;
        g_activeRegionRect.bottom = 0;
        g_activeRegionRect.y = 0;
        g_initField08 = 0;
        g_initField0C = 0;
        g_initField10 = 0;
        g_initField14 = 0;
        g_renderStateReadyWriteOnlyFlag = 1;
        g_renderStateReservedWriteOnly = 0;
        g_initField00 = 0;
        g_initField04 = 0;

        g_zVideo_pfnBltSourceToPrimary = zVid_Image::BlitToFramebufferClipped;
        g_defaultGraphicsFlags = -1;
        zOptionEntryPartial* option = zGame::OptionsFindOption(
            g_zVideo_ActiveRendererPath != 0 ? g_zOpt_OptionName_GfxFlagsHw : g_zOpt_OptionName_GfxFlagsSw
        );
        g_graphicsFlags = option != 0 ? &option->payloadOrBuffer : &g_defaultGraphicsFlags;
        g_perspectiveTextureDeltaXBytes = g_perspectiveTextureDeltaXPow2 * g_bytesPerPixel;
        return 0;
    }
} // namespace zRndr

namespace zVid
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-shutdownframescratchbuffers
     * @recoil-artifact defines .text recoil:function:0x48ff60: zVid::ShutdownFrameScratchBuffers.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
     * Purpose: release the frame scratch and noise buffers used by software video effects.
     */
    int __cdecl ShutdownFrameScratchBuffers()
    {
        NoiseShutdownBuffers();
        return 0;
    }
} // namespace zVid

namespace zVid
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-initframescratchbuffers
     * @recoil-artifact defines .text recoil:function:0x48ff70: zVid::InitFrameScratchBuffers.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zImage\zvid_buff.c.
     * Purpose: initialize noise buffers and select the active renderer span routine table.
     */
    int __cdecl InitFrameScratchBuffers()
    {
        NoiseInitBuffers();
        zRndr::SelectSpanRoutines();
        return 0;
    }
} // namespace zVid

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-selectspanroutines
     * @recoil-artifact defines .text recoil:function:0x48ff80: zRndr::SelectSpanRoutines
     *
     *
     * Purpose: Refresh pixel-pack state and install the active 16-bit point, line, and span routines.
     */
    void __cdecl SelectSpanRoutines()
    {
        zVideo::PixelPackGetRgbBits(&g_pixelPackRedBits, &g_pixelPackGreenBits, &g_pixelPackBlueBits);
        zVideo::PixelPackGetRgbMasks(&g_pixelPackRedMask, &g_pixelPackGreenMask, &g_pixelPackBlueMask);
        zVideo::PixelPackGetPackingParams(&g_pixelPackRedShift, &g_pixelPackGreenShift, &g_pixelPackBlueShift);

        *g_graphicsFlags &= ~4;
        if ((*g_graphicsFlags & 8) != 0) {
            SetPerspectiveAdaptiveSpanParams(0x10, 0x40, 0.100000001f);
        } else {
            SetPerspectiveAdaptiveSpanParams(0x20, 0x200, 0.100000001f);
        }

        if (g_bytesPerPixel != 2) {
            return;
        }

        g_pfnPointOpCandidate = (PointOpProc)zRndrPlotPixel16;
        g_pfnPointOpActive = (PointOpProc)zRndrPlotPixel16;
        g_pfnImmediateRaster4 = zRndrDrawLine16;
        g_pfnImmediateRasterReserved = zRndrDrawLine16Segmented;
        g_pfnImmediateRaster5 = zRndrDrawLine16Clipped;
        g_pfnSelectedSpanOp = (SpanRoutineProc)zRndrFillSpan16Opaque;
        g_pfnSelectedSpanOp_Mode0 = SpanMasked16FromTex16SwitchVShift;
        if (g_pixelPackGreenBits == 5) {
            g_pfnFlatImmediateSpanOp = (FlatImmediateSpanProc)zRndrFillSpan555Solid;
            if ((*g_graphicsFlags & 0x4) != 0) {
                g_pfnTexturedQueuedSpanOp_Mode0
                    = zSys::CheckCpuSignatureMask() != 0 ? SpanCopy16FromTex16ExplicitVShift : SpanCopy16FromTex16;
                g_pfnTexturedQueuedSpanOp_Mode1 = SpanCopy16FromPal8SwitchVShift;
                g_pfnTexturedQueuedFinalize = (SpanRoutineProc)FogBlendSpan555Mmx;
                g_pfnTexturedQueuedFinalizeAlt = (SpanRoutineProc)SpanMmxSetTexUvMasksAndVShift;
            } else {
                g_pfnTexturedQueuedSpanOp_Mode0 = SpanCopy16FromTex16SwitchVShift;
                g_pfnTexturedQueuedSpanOp_Mode1 = SpanCopy16FromPal8SwitchVShift;
                g_pfnTexturedQueuedFinalize = (SpanRoutineProc)FogBlendSpan555Scalar;
                g_pfnTexturedQueuedFinalizeAlt = 0;
            }
        } else {
            g_pfnFlatImmediateSpanOp = (FlatImmediateSpanProc)zRndrFillSpan565Solid;
            if ((*g_graphicsFlags & 0x4) != 0) {
                g_pfnTexturedQueuedSpanOp_Mode0
                    = zSys::CheckCpuSignatureMask() != 0 ? SpanCopy16FromTex16ExplicitVShift : SpanCopy16FromTex16;
                g_pfnTexturedQueuedSpanOp_Mode1 = SpanCopy16FromPal8SwitchVShift;
                g_pfnTexturedQueuedFinalize = (SpanRoutineProc)FogBlendSpan565Mmx;
                g_pfnTexturedQueuedFinalizeAlt = (SpanRoutineProc)SpanMmxSetTexUvMasksAndVShift;
            } else {
                g_pfnTexturedQueuedSpanOp_Mode0 = SpanCopy16FromTex16SwitchVShift;
                g_pfnTexturedQueuedSpanOp_Mode1 = SpanCopy16FromPal8SwitchVShift;
                g_pfnTexturedQueuedFinalize = (SpanRoutineProc)FogBlendSpan565Scalar;
                g_pfnTexturedQueuedFinalizeAlt = 0;
            }
        }

        if ((*g_graphicsFlags & 0x4) != 0) {
            SpanMmxSetPixelFormatMasks(g_pixelPackGreenBits);
        }

        if ((*g_graphicsFlags & 2) != 0) {
            if (g_pixelPackGreenBits == 6) {
                if ((*g_graphicsFlags & 4) != 0) {
                    g_pfnFlatQueuedSpanOp_Mode0 = SpanAlphaBlend565MmxFromTex16Alpha8;
                    g_pfnFlatQueuedSpanOpAlt_Mode0 = SpanAlphaBlend565MmxFromPal8Alpha8;
                } else {
                    g_pfnFlatQueuedSpanOp_Mode0 = SpanAlphaBlend565FromTex16Alpha8;
                    g_pfnFlatQueuedSpanOpAlt_Mode0 = SpanAlphaBlend565FromPal8Alpha8;
                }

                g_pfnTexturedFanTriSpanOp_Mode0 = SpanAlphaBlend565ConstAlphaFromTex16;
                g_pfnTexturedFanTriSpanOp_Mode1 = SpanAlphaBlend565ConstAlphaFastFromPal8;
                g_pfnPolyTlvSpanOp_Mode0 = SpanAlphaBlend565ConstAlphaFromTex16Alpha8;
                g_pfnPolyTlvSpanOpAlt_Mode0 = SpanAlphaBlend565ConstAlphaFromPal8Alpha8;
                g_pfnPolyTlvSpanOp_Mode1 = SpanMasked16FromTex16To565;
                g_pfnPolyTlvSpanOpAlt_Mode1 = SpanMasked16FromPal8To565;
            } else {
                if ((*g_graphicsFlags & 4) != 0) {
                    g_pfnFlatQueuedSpanOp_Mode0 = SpanAlphaBlend555MmxFromTex16Alpha8;
                    g_pfnFlatQueuedSpanOpAlt_Mode0 = SpanAlphaBlend555MmxFromPal8Alpha8;
                } else {
                    g_pfnFlatQueuedSpanOp_Mode0 = SpanAlphaBlend555FromTex16Alpha8;
                    g_pfnFlatQueuedSpanOpAlt_Mode0 = SpanAlphaBlend555FromPal8Alpha8;
                }

                g_pfnTexturedFanTriSpanOp_Mode0 = SpanAlphaBlend555ConstAlphaFromTex16;
                g_pfnTexturedFanTriSpanOp_Mode1 = SpanAlphaBlend555ConstAlphaFastFromPal8;
                g_pfnPolyTlvSpanOp_Mode0 = SpanAlphaBlend555ConstAlphaFromTex16Alpha8;
                g_pfnPolyTlvSpanOpAlt_Mode0 = SpanAlphaBlend555ConstAlphaFromPal8Alpha8;
                g_pfnPolyTlvSpanOp_Mode1 = SpanMasked16FromTex16To565;
                g_pfnPolyTlvSpanOpAlt_Mode1 = SpanAlphaBlend565ConstAlphaFromPal8;
            }
        } else {
            g_pfnFlatQueuedSpanOp_Mode0 = SpanMasked16FromTex16SwitchVShift;
            g_pfnFlatQueuedSpanOpAlt_Mode0 = SpanMasked16FromPal8SwitchVShift;
            g_pfnTexturedFanTriSpanOp_Mode0 = SpanCopy16FromTex16SwitchVShift;
            g_pfnTexturedFanTriSpanOp_Mode1 = SpanCopy16FromPal8SwitchVShift;
            g_pfnPolyTlvSpanOp_Mode0 = SpanMasked16FromTex16SwitchVShift;
            g_pfnPolyTlvSpanOpAlt_Mode0 = SpanMasked16FromPal8SwitchVShift;
            g_pfnPolyTlvSpanOp_Mode1 = SpanMasked16FromTex16SwitchVShift;
            g_pfnPolyTlvSpanOpAlt_Mode1 = SpanMasked16FromPal8SwitchVShift;
        }

        g_pfnFlatQueuedSpanOp_Mode1 = SpanMasked16FromTex16SwitchVShift;
        g_pfnFlatQueuedSpanOpAlt_Mode1 = SpanMasked16FromPal8SwitchVShift;
    }
} // namespace zRndr

namespace zVid_Image
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-calcpow2scratchfields
     * @recoil-artifact defines .text recoil:function:0x4902b0: zVid_Image::CalcPow2ScratchFields.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: GameZRecoil/zImage/zimg_texture.cpp.
     * Purpose: provide the recovered zVid_Image::CalcPow2ScratchFields behavior.
     */
    void __fastcall CalcPow2ScratchFields(zVidImagePartial * image)
    {
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

        const int uShift = image->uPow2Shift;
        image->widthScale = 1.0f;
        image->uShiftFrom20 = 20 - uShift;
        image->uMask = (1 << uShift) - 1;
        image->vMaskFixed20 = (1 << image->vPow2Shift << 20) - 0x100000;
    }
} // namespace zVid_Image

namespace zFloat
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-set255f
     * @recoil-artifact defines .text recoil:function:0x490330: zFloat::Set255f (GameZRecoil/zMath/zmth_main.c).
     * @recoil-match byte
     *
     * Purpose: write the constant 255.0f into the caller's float (color-scale helpers).
     */
    void __fastcall Set255f(float* value)
    {
        *value = 255.0f;
    }
} // namespace zFloat

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setframebufferregion
     * @recoil-artifact defines .text recoil:function:0x490340: zRndr::SetFrameBufferRegion
     * @recoil-match byte
     *
     * Purpose: Set the active framebuffer region, pixel depth, pitch, and derived perspective texture stride.
     */
    void __fastcall
    SetFrameBufferRegion(void* pixels, zOpt_ViewRectSection* activeRegionRect, int bitsPerPixel, int pitchBytes)
    {
        g_frameBuffer = pixels;
        if (activeRegionRect != 0) {
            g_activeRegionWidth = activeRegionRect->rightExclusive - activeRegionRect->x;
            g_activeRegionHeight = activeRegionRect->bottomExclusive - activeRegionRect->y;
            g_activeRegionRect.x = activeRegionRect->x;
            g_activeRegionRect.y = activeRegionRect->y;
            g_activeRegionRect.right = activeRegionRect->rightExclusive;
            g_activeRegionRect.bottom = activeRegionRect->bottomExclusive;
        }

        if (bitsPerPixel != 0) {
            g_bytesPerPixel = (int)((unsigned int)(bitsPerPixel) >> 3);
        }

        g_pitchBytes = pitchBytes;
        g_perspectiveTextureDeltaXBytes = g_perspectiveTextureDeltaXPow2 * g_bytesPerPixel;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setactiveregionsizefromrect
     * @recoil-artifact defines .text recoil:function:0x4903c0: zRndr::SetActiveRegionSizeFromRect
     * @recoil-match byte
     *
     * Source file evidence: D:\Proj\GameZRecoil\zModel\zmodel.cpp.
     * Data evidence: writes the active-region width and height globals at
     * 0x632054 and 0x632058 from the HudUiRect extents.
     * Purpose: Refresh cached active region dimensions from a HUD rectangle.
     */
    void __fastcall SetActiveRegionSizeFromRect(HudUiRect * rect)
    {
        if (rect != 0) {
            g_activeRegionWidth = rect->right - rect->left;
            g_activeRegionHeight = rect->bottom - rect->top;
        }
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setvideostridemirrors
     * @recoil-artifact defines .text recoil:function:0x4903e0: zRndr::SetVideoStrideMirrors.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: copy the current video stride into the renderer span mirror globals.
     */
    void __fastcall SetVideoStrideMirrors(int stride)
    {
        g_videoStrideMirror1 = stride;
        g_videoStrideMirror0 = stride;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-getactiveregionstate
     * @recoil-artifact defines .text recoil:function:0x4903f0: zRndr::GetActiveRegionState
     * @recoil-match byte
     *
     * Source file evidence: GameZRecoil/zRndr/zRndr_Draw.cpp.
     * Data evidence: reads the active-region framebuffer, width, height,
     * bytes-per-pixel, and pitch globals at 0x632050-0x632060.
     * Purpose: Return the active framebuffer pointer and report the cached region dimensions, pixel depth, and pitch.
     */
    void* __fastcall GetActiveRegionState(int* outWidth, int* outHeight, int* outBitsPerPixel, int* outPitchBytes)
    {
        *outWidth = g_activeRegionWidth;
        *outHeight = g_activeRegionHeight;
        *outBitsPerPixel = g_bytesPerPixel << 3;
        *outPitchBytes = g_pitchBytes;
        return g_frameBuffer;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setperspectivetexturedeltax
     * @recoil-artifact defines .text recoil:function:0x490430: zRndr::SetPerspectiveTextureDeltaX
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: Cache the perspective texture span chunk size and byte stride derived from delta X.
     */
    void __fastcall SetPerspectiveTextureDeltaX(int deltaX)
    {
        g_perspectiveTextureDeltaXInput = deltaX;

        int clampedDeltaX = deltaX;
        if (clampedDeltaX < 8) {
            clampedDeltaX = 8;
        }

        int shift = -1;
        g_perspectiveTextureDeltaXShift = shift;
        if (clampedDeltaX != 0) {
            do {
                ++shift;
                clampedDeltaX >>= 1;
            } while (clampedDeltaX != 0);

            g_perspectiveTextureDeltaXShift = shift;
        }

        g_perspectiveTextureDeltaXPow2 = 1 << shift;
        const int byteStride = g_perspectiveTextureDeltaXPow2 * g_bytesPerPixel;
        g_perspectiveTextureDeltaXPow2F = (float)(g_perspectiveTextureDeltaXPow2);
        g_perspectiveTextureDeltaXBytes = byteStride;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setperspectiveadaptivespanparams
     * @recoil-artifact defines .text recoil:function:0x490480: zRndr::SetPerspectiveAdaptiveSpanParams
     * @recoil-match byte
     *
     * Purpose: Store the adaptive perspective span-size thresholds selected for the renderer.
     */
    void __fastcall SetPerspectiveAdaptiveSpanParams(int minSpan, int maxSpan, float slope)
    {
        g_perspectiveAdaptiveMinSpan = minSpan;
        g_perspectiveAdaptiveMaxSpan = maxSpan;
        g_perspectiveAdaptiveSlope = slope;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setperspectivetexturefarz
     * @recoil-artifact defines .text recoil:function:0x4904a0: zRndr::SetPerspectiveTextureFarZ
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: Cache the reciprocal far-Z value used by perspective texture correction.
     */
    void __stdcall SetPerspectiveTextureFarZ(float farZ)
    {
        if (farZ != 0.0) {
            g_perspectiveTextureFarZInv = 1.0f / farZ;
        }
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setperspectiveadaptivecorrection
     * @recoil-artifact defines .text recoil:function:0x4904d0: zRndr::SetPerspectiveAdaptiveCorrection
     * @recoil-match byte
     *
     * Purpose: Cache adaptive perspective depth-bias terms used by textured span subdivision.
     */
    void __stdcall SetPerspectiveAdaptiveCorrection(float perspectiveAdaptiveCorrection)
    {
        const float plusOne = perspectiveAdaptiveCorrection + 1.0f;
        g_spanDepthBias = perspectiveAdaptiveCorrection;
        g_spanDepthBiasPlusOne = plusOne;
        if (plusOne == 0.0f) {
            g_spanDepthBiasPlusOneInv = 0.0f;
        } else {
            g_spanDepthBiasPlusOneInv = 1.0f / plusOne;
        }
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanocclusioninit
     * @recoil-artifact defines .text recoil:function:0x490520: zRndr::SpanOcclusionInit.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: initialize software span-occlusion columns for the active display height.
     *
     * Evidence: BN stores visible and padded column counts, allocates the column
     * table and span-node pool with calloc, initializes the table, clears the saved
     * occluder count, and installs the local and secondary span-list callbacks.
     */
    int __fastcall SpanOcclusionInit(int height)
    {
        g_spanColumnCount = height;
        g_spanColumnCountPadded = height + 0x80;
        g_spanColumnHeadTable
            = (SpanNodePartial**)(calloc((size_t)(g_spanColumnCountPadded), sizeof(SpanNodePartial*)));
        g_spanPoolBase = (SpanNodePartial*)(calloc((size_t)(g_spanColumnCountPadded) << 8, sizeof(SpanNodePartial)));

        SpanOcclusionBuildColumnHeadTable();
        g_spanOccluderPolyCount = 0;
        g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
        g_pfnBuildSpanListSecondary = zRndrSpanOcclusionBuildSpanList;
        return 0;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanocclusionbuildcolumnheadtable
     * @recoil-artifact defines .text recoil:function:0x490590: zRndr::SpanOcclusionBuildColumnHeadTable.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: clear per-column span heads and rebuild them from saved occluder
     * polygons.
     * Evidence: BN clears gRndr_SpanColumnHeadTable for gRndr_SpanColumnCount
     * entries, resets allocation and iteration cursors to the span pool, then
     * rasterizes each saved gRndr_SpanOccluderPolys entry.
     */
    void __cdecl SpanOcclusionBuildColumnHeadTable()
    {
        SpanNodePartial** columnHead = g_spanColumnHeadTable;
        int columnIndex = 0;
        while (columnIndex < g_spanColumnCount) {
            *columnHead = 0;
            ++columnHead;
            ++columnIndex;
        }

        g_spanIterNode = 0;
        g_spanAllocCursor = g_spanPoolBase;
        g_spanIterPrevLink = 0;

        int polyIndex = 0;
        if (polyIndex < g_spanOccluderPolyCount) {
            SpanOccluderPolyPartial* poly = g_spanOccluderPolys;
            do {
                SpanOcclusionRasterizeOccluderPoly(poly, poly->vertCount);
                ++polyIndex;
                ++poly;
            } while (polyIndex < g_spanOccluderPolyCount);
        }
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanocclusionresetframe
     * @recoil-artifact defines .text recoil:function:0x490600: zRndr::SpanOcclusionResetFrame.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: clear saved span-occluder polygons for a new rendered frame.
     * Evidence: BN writes zero to gRndr_SpanOccluderPolyCount and returns.
     */
    void __cdecl SpanOcclusionResetFrame()
    {
        g_spanOccluderPolyCount = 0;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanocclusionsubmitoccluderrect
     * @recoil-artifact defines .text recoil:function:0x490610: zRndr::SpanOcclusionSubmitOccluderRect.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\Battlesport\zrndr_span.cpp.
     * Purpose: convert one HUD rectangle into a four-vertex span-occluder polygon.
     *
     * Evidence: BN converts rect bounds to four xyz vertices, optionally halves x/y
     * coordinates for replicated rendering, assigns the uniform z value, and calls
     * zRndr::SpanOcclusionAddPolygon(vertices, 4).
     */
    void __fastcall SpanOcclusionSubmitOccluderRect(const HudUiRect* rect, int halveIfReplicate, float z)
    {
        zVec3 vertices[4];
        vertices[0].x = (float)(rect->left);
        vertices[0].y = (float)(rect->top);
        vertices[1].x = vertices[0].x;
        vertices[1].y = (float)(rect->bottom);
        vertices[2].x = (float)(rect->right);
        vertices[2].y = vertices[1].y;
        vertices[3].x = vertices[2].x;
        vertices[3].y = vertices[0].y;

        if (halveIfReplicate != 0) {
            vertices[0].x *= 0.5f;
            vertices[0].y *= 0.5f;
            vertices[1].x *= 0.5f;
            vertices[1].y *= 0.5f;
            vertices[2].x *= 0.5f;
            vertices[2].y *= 0.5f;
            vertices[3].x *= 0.5f;
            vertices[3].y *= 0.5f;
        }

        vertices[0].z = vertices[1].z = vertices[2].z = vertices[3].z = z;
        SpanOcclusionAddPolygon(vertices, 4);
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanocclusionaddpolygon
     * @recoil-artifact defines .text recoil:function:0x490710: zRndr::SpanOcclusionAddPolygon.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: append one saved span-occluder polygon for the next column-table
     * rebuild.
     *
     * Evidence: BN caps the saved polygon list at seven active entries, copies the
     * submitted xyz vertices into gRndr_SpanOccluderPolys, clamps vertCount to
     * eight, and increments gRndr_SpanOccluderPolyCount.
     */
    void __fastcall SpanOcclusionAddPolygon(const zVec3* vertices, int vertCount)
    {
        if (g_spanOccluderPolyCount >= 7) {
            return;
        }

        for (int i = 0; i < vertCount; ++i) {
            SpanOccluderPolyPartial* const slot = &g_spanOccluderPolys[g_spanOccluderPolyCount];
            *(zVec3*)(slot->vertices[i]) = vertices[i];
        }

        const int clampedVertCount = vertCount > 8 ? 8 : vertCount;
        // Retail stores through a slot pointer and then re-reads the count for the increment.
        SpanOccluderPolyPartial* const poly = &g_spanOccluderPolys[g_spanOccluderPolyCount];
        poly->vertCount = clampedVertCount;
        ++g_spanOccluderPolyCount;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanocclusionshutdown
     * @recoil-artifact defines .text recoil:function:0x490780: zRndr::SpanOcclusionShutdown.
     * @recoil-match byte
     *
     * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: release the software span-occlusion column table and span-node pool.
     * Evidence: BN frees non-null gRndr_SpanColumnHeadTable and gRndr_SpanPoolBase
     * through the CRT free import, clears those two globals, and returns zero in
     * eax before the epilogue.
     */
    int __cdecl SpanOcclusionShutdown()
    {
        if (g_spanColumnHeadTable != 0) {
            free(g_spanColumnHeadTable);
            g_spanColumnHeadTable = 0;
        }

        if (g_spanPoolBase != 0) {
            free(g_spanPoolBase);
            g_spanPoolBase = 0;
        }

        return 0;
    }
} // namespace zRndr

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-testspandepthorderpair
 * @recoil-artifact defines .text recoil:function:0x4907c0: zRndrSpanOcclusionTestSpanDepthOrderPair.
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: decide whether one overlapping span node is in front of another
 * using the recovered inverse-depth bias thresholds.
 *
 * Evidence: BN evaluates interpolated inverse-depth values at endpoint and
 * overlap samples through zRndr_SpanNode fields, compares against
 * gRndr_SpanDepthBiasPlusOne and gRndr_SpanDepthBiasPlusOneInv, and returns the
 * depth-order predicate used by span-occlusion insertion and visibility tests.
 */
int __fastcall zRndrSpanOcclusionTestSpanDepthOrderPair(zRndr::SpanNodePartial* lhs, zRndr::SpanNodePartial* rhs)
{
    float depthA;
    float depthB;
    if (rhs->sampleXMin == rhs->sampleXMax) {
        if (lhs->sampleXMin == lhs->sampleXMax) {
            depthA = lhs->invDepth * zRndr::g_spanDepthBiasPlusOne;
        } else {
            depthA = ((float)(rhs->sampleXMax - lhs->sampleXMin) * lhs->depthSlope + lhs->invDepth)
                * zRndr::g_spanDepthBiasPlusOne;
        }
        return *(int*)&depthA >= *(int*)&rhs->invDepth;
    }

    if (lhs->sampleXMin == lhs->sampleXMax) {
        depthA = ((float)(lhs->sampleXMax - rhs->sampleXMin) * rhs->depthSlope + rhs->invDepth)
            * zRndr::g_spanDepthBiasPlusOneInv;
        return *(int*)&lhs->invDepth >= *(int*)&depthA;
    }

    const float lhsWidth = (float)(lhs->sampleXMax - lhs->sampleXMin);
    const float lhsDepthDelta = lhs->invDepth - lhs->invDepthStep;
    const float rhsStartOffset = (float)(rhs->sampleXMin - lhs->sampleXMin);
    const float rhsStartDepthDelta = rhs->invDepth - lhs->invDepth;
    const float lhsStartSide = lhsDepthDelta * rhsStartOffset + lhsWidth * rhsStartDepthDelta;
    const float negativeBias = -zRndr::g_spanDepthBias;
    if (lhsStartSide >= negativeBias) {
        const float lhsEndSide = (rhs->invDepthStep - lhs->invDepth) * lhsWidth
            + (float)(rhs->sampleXMax - lhs->sampleXMin) * lhsDepthDelta;
        if (lhsEndSide >= negativeBias) {
            return 0;
        }
        if (lhsStartSide <= zRndr::g_spanDepthBias && lhsEndSide <= zRndr::g_spanDepthBias) {
            return 1;
        }
    } else if (lhsStartSide <= zRndr::g_spanDepthBias
        && (rhs->invDepthStep - lhs->invDepth) * lhsWidth + (float)(rhs->sampleXMax - lhs->sampleXMin) * lhsDepthDelta
            <= zRndr::g_spanDepthBias) {
        return 1;
    }

    const float rhsDepthDelta = rhs->invDepth - rhs->invDepthStep;
    const float lhsStartOffset = (float)(lhs->sampleXMin - rhs->sampleXMin);
    const float rhsWidth = (float)(rhs->sampleXMax - rhs->sampleXMin);
    const float lhsStartDepthDelta = lhs->invDepth - rhs->invDepth;
    const float rhsStartSide = rhsDepthDelta * lhsStartOffset + rhsWidth * lhsStartDepthDelta;
    if (rhsStartSide >= negativeBias) {
        const float rhsEndSide = (lhs->invDepthStep - rhs->invDepth) * rhsWidth
            + (float)(lhs->sampleXMax - rhs->sampleXMin) * rhsDepthDelta;
        if (rhsEndSide >= negativeBias) {
            return 1;
        }
        if (rhsStartSide <= zRndr::g_spanDepthBias && rhsEndSide <= zRndr::g_spanDepthBias) {
            return 0;
        }
    } else if (rhsStartSide <= zRndr::g_spanDepthBias
        && (lhs->invDepthStep - rhs->invDepth) * rhsWidth + (float)(lhs->sampleXMax - rhs->sampleXMin) * rhsDepthDelta
            <= zRndr::g_spanDepthBias) {
        return 0;
    }

    if (lhs->sampleXMax < rhs->sampleXMax) {
        depthA = lhs->invDepthStep;
        depthB = (float)(lhs->sampleXMax - rhs->sampleXMin) * rhs->depthSlope + rhs->invDepth;
    } else {
        depthA = (float)(rhs->sampleXMax - lhs->sampleXMin) * lhs->depthSlope + lhs->invDepth;
        depthB = rhs->invDepthStep;
    }
    if (lhs->sampleXMin < rhs->sampleXMin) {
        depthB += rhs->invDepth;
        depthA = lhs->depthSlope * rhsStartOffset + depthA + lhs->invDepth;
    } else {
        depthA += lhs->invDepth;
        depthB = rhs->depthSlope * lhsStartOffset + depthB + rhs->invDepth;
    }
    return *(int*)&depthA >= *(int*)&depthB;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-insertspannode-local
 * @recoil-artifact defines .text recoil:function:0x490ae0: zRndr_SpanOcclusion_InsertSpanNode_Local.
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: insert the pending span into a column using depth-tested occlusion
 * splitting.
 *
 * Evidence: BN identifies this as the callback installed in
 * gRndr_pfnBuildSpanList; the wrapper forwards spanList, columnIndex, and
 * spanCount into the recovered depth-tested insertion helper.
 */
void __fastcall
zRndr_SpanOcclusion_InsertSpanNode_Local(zRndr::SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    using namespace zRndr;

    SpanNodePartial* pending = g_spanAllocCursor;
    SpanNodePartial** columnHeadTable = g_spanColumnHeadTable;
    SpanNodePartial* current = columnHeadTable[columnIndex];
    *spanCount = 0;
    if (current == 0 || pending->sampleXMax < current->sampleXMin) {
        g_spanAllocCursor->next = current;
        g_spanColumnHeadTable[columnIndex] = g_spanAllocCursor;
        spanList[*spanCount] = g_spanAllocCursor;
        ++*spanCount;
        ++g_spanAllocCursor;
        return;
    }

    pending->next = 0;
    g_spanIterNode = current;
    g_spanIterPrevLink = 0;

    SpanNodePartial* previous = 0;
    float maxPendingDepth;
    float maxCurrentDepth;
    // Separate scaled depths: retail gives the dead spanCount slot [esp+0x3c] to maxPendingDepth.
    float scaledOccluderDepth;
    float scaledPendingDepth;
    int pendingInFront;
    while (current != 0) {
        previous = g_spanIterPrevLink;
        current = g_spanIterNode;

        while (current != 0 && pending->sampleXMin > current->sampleXMax) {
            previous = current;
            current = current->next;
        }
        if (current == 0) {
            pending->next = 0;
            g_spanIterNode = g_spanAllocCursor;
            g_spanIterPrevLink = previous;
            if (previous != 0) {
                previous->next = g_spanAllocCursor;
            }
            g_spanLastNode = g_spanAllocCursor;
            ++g_spanAllocCursor;
            pending = g_spanLastNode;

            if (*spanCount > 0) {
                SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                    lastVisible->sampleXMax = pending->sampleXMax;
                    lastVisible->invDepthStep = pending->invDepthStep;
                    lastVisible->next = pending->next;
                    g_spanLastNode = lastVisible;
                    g_spanIterNode = lastVisible;
                    return;
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }
            } else {
                spanList[*spanCount] = pending;
                ++*spanCount;
            }
            return;
        }

        if (pending->sampleXMax < current->sampleXMin) {
            g_spanIterNode = current;
            g_spanIterPrevLink = previous;
            if (previous != 0) {
                previous->next = g_spanAllocCursor;
            }
            g_spanAllocCursor->next = g_spanIterNode;
            g_spanLastNode = g_spanAllocCursor;
            ++g_spanAllocCursor;
            pending = g_spanLastNode;

            if (*spanCount > 0) {
                SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                    lastVisible->sampleXMax = pending->sampleXMax;
                    lastVisible->invDepthStep = pending->invDepthStep;
                    lastVisible->next = pending->next;
                    g_spanLastNode = lastVisible;
                    g_spanIterNode = lastVisible;
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }
            } else {
                spanList[*spanCount] = pending;
                ++*spanCount;
            }

            if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
            }
            return;
        }

        // Retail orders the depth bounds by comparing the float bit patterns as integers.
        if (*(int*)(&pending->invDepth) > *(int*)(&pending->invDepthStep)) {
            maxPendingDepth = pending->invDepth;
        } else {
            maxPendingDepth = pending->invDepthStep;
        }
        scaledOccluderDepth = (*(int*)(&current->invDepth) < *(int*)(&current->invDepthStep) ? current->invDepth
                                                                                             : current->invDepthStep)
            * g_spanDepthBiasPlusOne;
        if (*(int*)(&scaledOccluderDepth) >= *(int*)(&maxPendingDepth)) {
            pendingInFront = 0;
        } else {
            scaledPendingDepth = *(int*)(&pending->invDepth) < *(int*)(&pending->invDepthStep) ? pending->invDepth
                                                                                               : pending->invDepthStep;
            if (*(int*)(&current->invDepth) > *(int*)(&current->invDepthStep)) {
                maxCurrentDepth = current->invDepth;
            } else {
                maxCurrentDepth = current->invDepthStep;
            }
            scaledPendingDepth *= g_spanDepthBiasPlusOne;
            if (*(int*)(&scaledPendingDepth) >= *(int*)(&maxCurrentDepth)) {
                pendingInFront = 1;
            } else {
                pendingInFront = zRndrSpanOcclusionTestSpanDepthOrderPair(pending, current);
            }
        }

        if (pendingInFront != 0) {
            if (current->sampleXMax <= pending->sampleXMax) {
                if (current->sampleXMin < pending->sampleXMin) {
                    if (current->sampleXMax >= pending->sampleXMin) {
                        g_spanIterNode = current;
                        g_spanIterPrevLink = previous;
                        const int oldMin = current->sampleXMin;
                        const int newMax = pending->sampleXMin - 1;
                        current->sampleXMax = newMax;
                        current->invDepthStep = current->invDepth + (float)(newMax - oldMin) * current->depthSlope;
                    }
                    continue;
                }

                if (current->sampleXMax < pending->sampleXMax) {
                    const float slope = pending->depthSlope;
                    const int coveredMax = current->sampleXMax;
                    const int rightMin = coveredMax + 1;
                    SpanNodePartial* right = pending + 1;
                    right->next = pending->next;
                    right->sampleXMax = pending->sampleXMax;
                    right->invDepthStep = pending->invDepthStep;
                    right->depthSlope = pending->depthSlope;
                    pending->sampleXMax = coveredMax;
                    pending->invDepthStep = pending->invDepth + (float)(coveredMax - pending->sampleXMin) * slope;
                    right->sampleXMin = rightMin;
                    right->invDepth = pending->invDepth + (float)(rightMin - pending->sampleXMin) * slope;

                    g_spanAllocCursor->next = current->next;
                    g_spanIterNode = g_spanAllocCursor;
                    g_spanIterPrevLink = previous;
                    if (previous != 0) {
                        previous->next = g_spanAllocCursor;
                    }
                    g_spanLastNode = g_spanAllocCursor;
                    ++g_spanAllocCursor;
                    pending = g_spanLastNode;

                    if (*spanCount > 0) {
                        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                        if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                            lastVisible->sampleXMax = pending->sampleXMax;
                            lastVisible->invDepthStep = pending->invDepthStep;
                            lastVisible->next = pending->next;
                            g_spanLastNode = lastVisible;
                            g_spanIterNode = lastVisible;
                        } else {
                            spanList[*spanCount] = pending;
                            ++*spanCount;
                        }
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }

                    if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                        g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
                    }

                    pending = g_spanAllocCursor;
                    pending->next = 0;
                    continue;
                }

                pending->next = current->next;
                g_spanIterNode = g_spanAllocCursor;
                g_spanIterPrevLink = previous;
                if (previous != 0) {
                    previous->next = g_spanAllocCursor;
                }
                g_spanLastNode = g_spanAllocCursor;
                ++g_spanAllocCursor;
                pending = g_spanLastNode;

                if (*spanCount > 0) {
                    SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                    if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                        lastVisible->sampleXMax = pending->sampleXMax;
                        lastVisible->invDepthStep = pending->invDepthStep;
                        lastVisible->next = pending->next;
                        g_spanLastNode = lastVisible;
                        g_spanIterNode = lastVisible;
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }

                if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                    g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
                }
                return;
            } else if (pending->sampleXMin <= current->sampleXMin) {
                if (current->sampleXMin <= pending->sampleXMax) {
                    g_spanIterNode = current;
                    g_spanIterPrevLink = previous;
                    const int oldMax = current->sampleXMax;
                    const int newMin = pending->sampleXMax + 1;
                    current->sampleXMin = newMin;
                    current->invDepth = current->invDepthStep + (float)(newMin - oldMax) * current->depthSlope;
                    if (g_spanIterPrevLink != 0) {
                        g_spanIterPrevLink->next = g_spanAllocCursor;
                    }
                    g_spanAllocCursor->next = g_spanIterNode;
                    g_spanLastNode = g_spanAllocCursor;
                    ++g_spanAllocCursor;
                    pending = g_spanLastNode;

                    if (*spanCount > 0) {
                        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                        if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                            lastVisible->sampleXMax = pending->sampleXMax;
                            lastVisible->invDepthStep = pending->invDepthStep;
                            lastVisible->next = pending->next;
                            g_spanLastNode = lastVisible;
                            g_spanIterNode = lastVisible;
                        } else {
                            spanList[*spanCount] = pending;
                            ++*spanCount;
                        }
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }

                    if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                        g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
                    }
                    return;
                } else {
                    continue;
                }
            } else {
                if (pending->sampleXMax >= current->sampleXMax) {
                    continue;
                }

                g_spanIterNode = current;
                g_spanIterPrevLink = previous;
                // Copy the node before trimming its right remainder; the old
                // next, minimum and left depth are superseded by the split.
                SpanNodePartial rightFragment;
                rightFragment.next = current->next;
                rightFragment.sampleXMin = current->sampleXMin;
                rightFragment.sampleXMax = current->sampleXMax;
                rightFragment.invDepth = current->invDepth;
                rightFragment.invDepthStep = current->invDepthStep;
                rightFragment.depthSlope = current->depthSlope;
                rightFragment.sampleXMin = pending->sampleXMax + 1;
                rightFragment.invDepth = rightFragment.invDepthStep
                    + (float)(rightFragment.sampleXMin - rightFragment.sampleXMax) * current->depthSlope;
                g_spanIterNode->sampleXMax = pending->sampleXMin - 1;
                g_spanIterNode->invDepthStep = g_spanIterNode->invDepth
                    + (float)(g_spanIterNode->sampleXMax - g_spanIterNode->sampleXMin) * g_spanIterNode->depthSlope;

                g_spanAllocCursor->next = g_spanIterNode->next;
                g_spanIterNode->next = g_spanAllocCursor;
                g_spanLastNode = g_spanAllocCursor;
                ++g_spanAllocCursor;
                pending = g_spanLastNode;

                if (*spanCount > 0) {
                    SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                    if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                        lastVisible->sampleXMax = pending->sampleXMax;
                        lastVisible->invDepthStep = pending->invDepthStep;
                        lastVisible->next = pending->next;
                        g_spanLastNode = lastVisible;
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }

                g_spanIterNode = g_spanLastNode;
                g_spanAllocCursor->sampleXMin = rightFragment.sampleXMin;
                g_spanAllocCursor->sampleXMax = rightFragment.sampleXMax;
                g_spanAllocCursor->invDepth = rightFragment.invDepth;
                g_spanAllocCursor->invDepthStep = rightFragment.invDepthStep;
                g_spanAllocCursor->depthSlope = rightFragment.depthSlope;
                g_spanAllocCursor->next = g_spanIterNode->next;
                g_spanIterNode->next = g_spanAllocCursor;
                g_spanLastNode = g_spanAllocCursor;
                ++g_spanAllocCursor;
                return;
            }
        } else if (current->sampleXMin <= pending->sampleXMin) {
            if (current->sampleXMax >= pending->sampleXMax) {
                g_spanIterNode = current;
                g_spanIterPrevLink = previous;
                return;
            }
            if (current->sampleXMax >= pending->sampleXMin) {
                // Retail re-derives the trimmed start depth from the far end of the span.
                pending->sampleXMin = current->sampleXMax + 1;
                pending->invDepth
                    = pending->invDepthStep + (float)(pending->sampleXMin - pending->sampleXMax) * pending->depthSlope;
                g_spanIterNode = current;
                g_spanIterPrevLink = previous;
            }
        } else {
            if (current->sampleXMax < pending->sampleXMax) {
                g_spanIterNode = current;
                g_spanIterPrevLink = previous;
                const float slope = pending->depthSlope;
                // Retail forms the right fragment start before the left fragment end (0x490d67, 0x490d74).
                // Retail forms the right fragment start before the left fragment end (0x490d67, 0x490d74).
                const int rightMin = current->sampleXMax + 1;
                const int leftMax = current->sampleXMin - 1;
                SpanNodePartial* right = pending + 1;
                right->next = pending->next;
                right->sampleXMax = pending->sampleXMax;
                right->invDepthStep = pending->invDepthStep;
                right->depthSlope = pending->depthSlope;
                pending->sampleXMax = leftMax;
                pending->invDepthStep = pending->invDepth + (float)(leftMax - pending->sampleXMin) * slope;
                right->sampleXMin = rightMin;
                right->invDepth = pending->invDepth + (float)(rightMin - pending->sampleXMin) * slope;

                if (g_spanIterPrevLink != 0) {
                    g_spanIterPrevLink->next = g_spanAllocCursor;
                }
                g_spanAllocCursor->next = g_spanIterNode;
                g_spanLastNode = g_spanAllocCursor;
                ++g_spanAllocCursor;
                pending = g_spanLastNode;

                if (*spanCount > 0) {
                    SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                    if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                        lastVisible->sampleXMax = pending->sampleXMax;
                        lastVisible->invDepthStep = pending->invDepthStep;
                        lastVisible->next = pending->next;
                        g_spanLastNode = lastVisible;
                        g_spanIterNode = lastVisible;
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }

                if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                    g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
                }

                pending = g_spanAllocCursor;
                pending->next = 0;
                continue;
            }
            if (pending->sampleXMin <= current->sampleXMin && current->sampleXMin <= pending->sampleXMax) {
                if (pending->sampleXMin < current->sampleXMin) {
                    g_spanIterNode = current;
                    g_spanIterPrevLink = previous;
                    pending->sampleXMax = current->sampleXMin - 1;
                    pending->invDepthStep
                        = pending->invDepth + (float)(pending->sampleXMax - pending->sampleXMin) * pending->depthSlope;
                    if (g_spanIterPrevLink != 0) {
                        g_spanIterPrevLink->next = g_spanAllocCursor;
                    }
                    g_spanAllocCursor->next = g_spanIterNode;
                    g_spanLastNode = g_spanAllocCursor;
                    ++g_spanAllocCursor;
                    pending = g_spanLastNode;

                    if (*spanCount > 0) {
                        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                        if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                            lastVisible->sampleXMax = pending->sampleXMax;
                            lastVisible->invDepthStep = pending->invDepthStep;
                            lastVisible->next = pending->next;
                            g_spanLastNode = lastVisible;
                            g_spanIterNode = lastVisible;
                        } else {
                            spanList[*spanCount] = pending;
                            ++*spanCount;
                        }
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }

                    if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                        g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
                    }
                }
                return;
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-insertspannode-nodepthtest
 * @recoil-artifact defines .text recoil:function:0x4912a0: zRndrSpanOcclusionInsertSpanNodeNoDepthTest.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: insert the pending span into a column without depth-order testing.
 *
 * Leaf fastcall callback. Every overlap case, including list exhaustion, owns
 * its emission: publish g_spanLastNode, advance the cursor, then read the node
 * back into pending before merging adjacent output fragments.
 */
void __fastcall
zRndrSpanOcclusionInsertSpanNodeNoDepthTest(zRndr::SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    using namespace zRndr;

    SpanNodePartial* pending = g_spanAllocCursor;
    SpanNodePartial** columnHeadTable = g_spanColumnHeadTable;
    SpanNodePartial* current = columnHeadTable[columnIndex];
    *spanCount = 0;
    if (current == 0 || pending->sampleXMax < current->sampleXMin) {
        g_spanAllocCursor->next = current;
        g_spanColumnHeadTable[columnIndex] = g_spanAllocCursor;
        spanList[*spanCount] = g_spanAllocCursor;
        ++*spanCount;
        ++g_spanAllocCursor;
        return;
    }

    pending->next = 0;
    g_spanIterNode = current;
    g_spanIterPrevLink = 0;

    SpanNodePartial* previous = 0;
    while (current != 0) {
        previous = g_spanIterPrevLink;
        current = g_spanIterNode;

        while (current != 0 && pending->sampleXMin > current->sampleXMax) {
            previous = current;
            current = current->next;
        }
        if (current == 0) {
            pending->next = 0;
            g_spanIterNode = g_spanAllocCursor;
            g_spanIterPrevLink = previous;
            if (previous != 0) {
                previous->next = g_spanAllocCursor;
            }
            g_spanLastNode = g_spanAllocCursor;
            ++g_spanAllocCursor;
            pending = g_spanLastNode;

            if (*spanCount > 0) {
                SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                    lastVisible->sampleXMax = pending->sampleXMax;
                    lastVisible->invDepthStep = pending->invDepthStep;
                    lastVisible->next = pending->next;
                    g_spanLastNode = lastVisible;
                    g_spanIterNode = lastVisible;
                    return;
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }
            } else {
                spanList[*spanCount] = pending;
                ++*spanCount;
            }
            return;
        }

        if (pending->sampleXMax < current->sampleXMin) {
            g_spanIterNode = current;
            g_spanIterPrevLink = previous;
            if (previous != 0) {
                previous->next = g_spanAllocCursor;
            }
            g_spanAllocCursor->next = g_spanIterNode;
            g_spanLastNode = g_spanAllocCursor;
            ++g_spanAllocCursor;
            pending = g_spanLastNode;

            if (*spanCount > 0) {
                SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                    lastVisible->sampleXMax = pending->sampleXMax;
                    lastVisible->invDepthStep = pending->invDepthStep;
                    lastVisible->next = pending->next;
                    g_spanLastNode = lastVisible;
                    g_spanIterNode = lastVisible;
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }
            } else {
                spanList[*spanCount] = pending;
                ++*spanCount;
            }

            if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
            }
            return;
        }

        if (current->sampleXMax <= pending->sampleXMax) {
            if (current->sampleXMin < pending->sampleXMin) {
                if (current->sampleXMax >= pending->sampleXMin) {
                    g_spanIterNode = current;
                    g_spanIterPrevLink = previous;
                    const int oldMin = current->sampleXMin;
                    const int newMax = pending->sampleXMin - 1;
                    current->sampleXMax = newMax;
                    current->invDepthStep = current->invDepth + (float)(newMax - oldMin) * current->depthSlope;
                }
                continue;
            }

            if (current->sampleXMax < pending->sampleXMax) {
                const float slope = pending->depthSlope;
                const int coveredMax = current->sampleXMax;
                const int rightMin = coveredMax + 1;
                SpanNodePartial* right = pending + 1;
                right->next = pending->next;
                right->sampleXMax = pending->sampleXMax;
                right->invDepthStep = pending->invDepthStep;
                right->depthSlope = pending->depthSlope;
                pending->sampleXMax = coveredMax;
                pending->invDepthStep = pending->invDepth + (float)(coveredMax - pending->sampleXMin) * slope;
                right->sampleXMin = rightMin;
                right->invDepth = pending->invDepth + (float)(rightMin - pending->sampleXMin) * slope;

                g_spanAllocCursor->next = current->next;
                g_spanIterNode = g_spanAllocCursor;
                g_spanIterPrevLink = previous;
                if (previous != 0) {
                    previous->next = g_spanAllocCursor;
                }
                g_spanLastNode = g_spanAllocCursor;
                ++g_spanAllocCursor;
                pending = g_spanLastNode;

                if (*spanCount > 0) {
                    SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                    if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                        lastVisible->sampleXMax = pending->sampleXMax;
                        lastVisible->invDepthStep = pending->invDepthStep;
                        lastVisible->next = pending->next;
                        g_spanLastNode = lastVisible;
                        g_spanIterNode = lastVisible;
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }

                if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                    g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
                }

                pending = g_spanAllocCursor;
                pending->next = 0;
                continue;
            }

            pending->next = current->next;
            g_spanIterNode = g_spanAllocCursor;
            g_spanIterPrevLink = previous;
            if (previous != 0) {
                previous->next = g_spanAllocCursor;
            }
            g_spanLastNode = g_spanAllocCursor;
            ++g_spanAllocCursor;
            pending = g_spanLastNode;

            if (*spanCount > 0) {
                SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                    lastVisible->sampleXMax = pending->sampleXMax;
                    lastVisible->invDepthStep = pending->invDepthStep;
                    lastVisible->next = pending->next;
                    g_spanLastNode = lastVisible;
                    g_spanIterNode = lastVisible;
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }
            } else {
                spanList[*spanCount] = pending;
                ++*spanCount;
            }

            if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
            }
            return;
        } else if (pending->sampleXMin <= current->sampleXMin) {
            if (current->sampleXMin <= pending->sampleXMax) {
                g_spanIterNode = current;
                g_spanIterPrevLink = previous;
                const int oldMax = current->sampleXMax;
                const int newMin = pending->sampleXMax + 1;
                current->sampleXMin = newMin;
                current->invDepth = current->invDepthStep + (float)(newMin - oldMax) * current->depthSlope;
                if (g_spanIterPrevLink != 0) {
                    g_spanIterPrevLink->next = g_spanAllocCursor;
                }
                g_spanAllocCursor->next = g_spanIterNode;
                g_spanLastNode = g_spanAllocCursor;
                ++g_spanAllocCursor;
                pending = g_spanLastNode;

                if (*spanCount > 0) {
                    SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                    if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                        lastVisible->sampleXMax = pending->sampleXMax;
                        lastVisible->invDepthStep = pending->invDepthStep;
                        lastVisible->next = pending->next;
                        g_spanLastNode = lastVisible;
                        g_spanIterNode = lastVisible;
                    } else {
                        spanList[*spanCount] = pending;
                        ++*spanCount;
                    }
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }

                if (g_spanLastNode->sampleXMin <= g_spanColumnHeadTable[columnIndex]->sampleXMin) {
                    g_spanColumnHeadTable[columnIndex] = g_spanLastNode;
                }
                return;
            } else {
                continue;
            }
        } else {
            if (pending->sampleXMax >= current->sampleXMax) {
                continue;
            }

            g_spanIterNode = current;
            g_spanIterPrevLink = previous;
            // Copy the node before trimming its right remainder; the old
            // next, minimum and left depth are superseded by the split.
            SpanNodePartial rightFragment;
            rightFragment.next = current->next;
            rightFragment.sampleXMin = current->sampleXMin;
            rightFragment.sampleXMax = current->sampleXMax;
            rightFragment.invDepth = current->invDepth;
            rightFragment.invDepthStep = current->invDepthStep;
            rightFragment.depthSlope = current->depthSlope;
            rightFragment.sampleXMin = pending->sampleXMax + 1;
            rightFragment.invDepth = rightFragment.invDepthStep
                + (float)(rightFragment.sampleXMin - rightFragment.sampleXMax) * current->depthSlope;
            g_spanIterNode->sampleXMax = pending->sampleXMin - 1;
            g_spanIterNode->invDepthStep = g_spanIterNode->invDepth
                + (float)(g_spanIterNode->sampleXMax - g_spanIterNode->sampleXMin) * g_spanIterNode->depthSlope;

            g_spanAllocCursor->next = g_spanIterNode->next;
            g_spanIterNode->next = g_spanAllocCursor;
            g_spanLastNode = g_spanAllocCursor;
            ++g_spanAllocCursor;
            pending = g_spanLastNode;

            if (*spanCount > 0) {
                SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                if (pending->sampleXMin == lastVisible->sampleXMax + 1) {
                    lastVisible->sampleXMax = pending->sampleXMax;
                    lastVisible->invDepthStep = pending->invDepthStep;
                    lastVisible->next = pending->next;
                    g_spanLastNode = lastVisible;
                } else {
                    spanList[*spanCount] = pending;
                    ++*spanCount;
                }
            } else {
                spanList[*spanCount] = pending;
                ++*spanCount;
            }

            g_spanIterNode = g_spanLastNode;
            g_spanAllocCursor->sampleXMin = rightFragment.sampleXMin;
            g_spanAllocCursor->sampleXMax = rightFragment.sampleXMax;
            g_spanAllocCursor->invDepth = rightFragment.invDepth;
            g_spanAllocCursor->invDepthStep = rightFragment.invDepthStep;
            g_spanAllocCursor->depthSlope = rightFragment.depthSlope;
            g_spanAllocCursor->next = g_spanIterNode->next;
            g_spanIterNode->next = g_spanAllocCursor;
            g_spanLastNode = g_spanAllocCursor;
            ++g_spanAllocCursor;
            return;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-buildspanlist
 * @recoil-artifact defines .text recoil:function:0x491840: zRndrSpanOcclusionBuildSpanList.
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: build visible fragments for one pending span against the current
 * column's occlusion list.
 *
 * Evidence: BN identifies this as the secondary span-list callback installed in
 * gRndr_pfnBuildSpanListSecondary; it forwards the callback arguments into the
 * recovered depth-tested visible-span builder.
 */
void __fastcall zRndrSpanOcclusionBuildSpanList(zRndr::SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    using namespace zRndr;

    SpanNodePartial* columnHead = g_spanColumnHeadTable[columnIndex];
    SpanNodePartial* pending = g_spanAllocCursor;
    SpanNodePartial* emitted;
    *spanCount = 0;
    if (columnHead == 0) {
        g_spanAllocCursor->next = 0;
        spanList[*spanCount] = g_spanAllocCursor;
        ++*spanCount;
        ++g_spanAllocCursor;
        return;
    }
    if (pending->sampleXMax < columnHead->sampleXMin) {
        spanList[*spanCount] = g_spanAllocCursor;
        ++*spanCount;
        ++g_spanAllocCursor;
        return;
    }

    pending->next = 0;
    SpanNodePartial* maxDepthSpan = 0;
    SpanNodePartial* minDepthSpan = 0;
    float maxDepth = 0.0f;
    float minDepth = 0.0f;
    SpanNodePartial occluder;
    SpanNodePartial* current = columnHead;
    for (;;) {
        while (current != 0 && pending->sampleXMin > current->sampleXMax) {
            current = current->next;
        }
        if (current != 0) {
            occluder.next = current->next;
            occluder.sampleXMin = current->sampleXMin;
            occluder.sampleXMax = current->sampleXMax;
            occluder.invDepth = current->invDepth;
            occluder.invDepthStep = current->invDepthStep;
            occluder.depthSlope = current->depthSlope;
            current = &occluder;
        }
        if (current == 0) {
            break;
        }
        if (pending->sampleXMax < current->sampleXMin) {
            // Retail emits the cursor node here without advancing the allocator.
            emitted = g_spanAllocCursor;
            if (*spanCount > 0) {
                SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                if (emitted->sampleXMin == lastVisible->sampleXMax + 1) {
                    lastVisible->sampleXMax = emitted->sampleXMax;
                    lastVisible->invDepthStep = emitted->invDepthStep;
                    lastVisible->next = emitted->next;
                    g_spanLastNode = lastVisible;
                } else {
                    spanList[*spanCount] = emitted;
                    ++*spanCount;
                }
            } else {
                spanList[*spanCount] = emitted;
                ++*spanCount;
            }
            return;
        }

        if (maxDepthSpan != pending) {
            maxDepth
                = (double)pending->invDepth > pending->invDepthStep ? (double)pending->invDepth : pending->invDepthStep;
            maxDepthSpan = pending;
        }
        const float occluderMinDepth
            = (double)current->invDepth < current->invDepthStep ? (double)current->invDepth : current->invDepthStep;
        int pendingInFront;
        if (occluderMinDepth * g_spanDepthBiasPlusOne >= maxDepth) {
            pendingInFront = 0;
        } else {
            if (minDepthSpan != pending) {
                minDepth = (double)pending->invDepth < pending->invDepthStep ? (double)pending->invDepth
                                                                             : pending->invDepthStep;
                minDepthSpan = pending;
            }
            if (minDepth * g_spanDepthBiasPlusOne
                >= ((double)current->invDepth > current->invDepthStep ? (double)current->invDepth
                                                                      : current->invDepthStep)) {
                pendingInFront = 1;
            } else {
                pendingInFront = zRndrSpanOcclusionTestSpanDepthOrderPair(pending, current);
            }
        }

        if (pendingInFront != 0) {
            if (current->sampleXMax <= pending->sampleXMax) {
                if (current->sampleXMin < pending->sampleXMin && current->sampleXMax >= pending->sampleXMin) {
                    // Trim the occluder copy to the part left of the pending span.
                    current->sampleXMax = pending->sampleXMin - 1;
                    current->invDepthStep
                        = (float)(current->sampleXMax - current->sampleXMin) * current->depthSlope + current->invDepth;
                    continue;
                }
                if (current->sampleXMin >= pending->sampleXMin) {
                    if (current->sampleXMax >= pending->sampleXMax) {
                        break;
                    }
                    // Split the pending span after the occluder and emit its left part.
                    const float slope = pending->depthSlope;
                    const int coveredMax = current->sampleXMax;
                    const int rightMin = coveredMax + 1;
                    SpanNodePartial* right = pending + 1;
                    right->next = pending->next;
                    right->sampleXMax = pending->sampleXMax;
                    right->invDepthStep = pending->invDepthStep;
                    right->depthSlope = pending->depthSlope;
                    pending->sampleXMax = coveredMax;
                    pending->invDepthStep = pending->invDepth + (float)(coveredMax - pending->sampleXMin) * slope;
                    right->sampleXMin = rightMin;
                    right->invDepth = pending->invDepth + (float)(rightMin - pending->sampleXMin) * slope;
                    g_spanLastNode = g_spanAllocCursor;
                    ++g_spanAllocCursor;
                    emitted = g_spanLastNode;
                    if (*spanCount > 0) {
                        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                        if (emitted->sampleXMin == lastVisible->sampleXMax + 1) {
                            lastVisible->sampleXMax = emitted->sampleXMax;
                            lastVisible->invDepthStep = emitted->invDepthStep;
                            lastVisible->next = emitted->next;
                            g_spanLastNode = lastVisible;
                        } else {
                            spanList[*spanCount] = emitted;
                            ++*spanCount;
                        }
                    } else {
                        spanList[*spanCount] = emitted;
                        ++*spanCount;
                    }
                    pending = g_spanAllocCursor;
                    pending->next = 0;
                }
            } else {
                if (pending->sampleXMin <= current->sampleXMin && current->sampleXMin <= pending->sampleXMax) {
                    current->sampleXMin = pending->sampleXMax + 1;
                    current->invDepth = (float)(current->sampleXMin - current->sampleXMax) * current->depthSlope
                        + current->invDepthStep;
                    emitted = g_spanAllocCursor;
                    if (*spanCount > 0) {
                        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                        if (emitted->sampleXMin == lastVisible->sampleXMax + 1) {
                            lastVisible->sampleXMax = emitted->sampleXMax;
                            lastVisible->invDepthStep = emitted->invDepthStep;
                            lastVisible->next = emitted->next;
                            g_spanLastNode = lastVisible;
                        } else {
                            spanList[*spanCount] = emitted;
                            ++*spanCount;
                        }
                    } else {
                        spanList[*spanCount] = emitted;
                        ++*spanCount;
                    }
                    return;
                }
                if (pending->sampleXMin > current->sampleXMin && pending->sampleXMax < current->sampleXMax) {
                    g_spanLastNode = g_spanAllocCursor;
                    ++g_spanAllocCursor;
                    emitted = g_spanLastNode;
                    if (*spanCount > 0) {
                        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                        if (emitted->sampleXMin == lastVisible->sampleXMax + 1) {
                            lastVisible->sampleXMax = emitted->sampleXMax;
                            lastVisible->invDepthStep = emitted->invDepthStep;
                            lastVisible->next = emitted->next;
                            g_spanLastNode = lastVisible;
                        } else {
                            spanList[*spanCount] = emitted;
                            ++*spanCount;
                        }
                    } else {
                        spanList[*spanCount] = emitted;
                        ++*spanCount;
                    }
                    return;
                }
            }
        } else if (current->sampleXMin <= pending->sampleXMin) {
            if (current->sampleXMax >= pending->sampleXMin) {
                if (pending->sampleXMax <= current->sampleXMax) {
                    return;
                }
                // Retail re-derives the trimmed start depth from the span's far end.
                pending->sampleXMin = current->sampleXMax + 1;
                pending->invDepth
                    = (float)(pending->sampleXMin - pending->sampleXMax) * pending->depthSlope + pending->invDepthStep;
            } else if (pending->sampleXMax <= current->sampleXMax) {
                return;
            }
        } else {
            if (current->sampleXMax < pending->sampleXMax) {
                // The occluder sits inside the pending span: emit the left part, keep the right part pending.
                const float slope = pending->depthSlope;
                // Retail forms the right fragment start before the left fragment end (0x491b69, 0x491b7f).
                const int rightMin = current->sampleXMax + 1;
                const int leftMax = current->sampleXMin - 1;
                SpanNodePartial* right = pending + 1;
                right->next = pending->next;
                right->sampleXMax = pending->sampleXMax;
                right->invDepthStep = pending->invDepthStep;
                right->depthSlope = pending->depthSlope;
                pending->sampleXMax = leftMax;
                pending->invDepthStep = pending->invDepth + (float)(leftMax - pending->sampleXMin) * slope;
                right->sampleXMin = rightMin;
                right->invDepth = pending->invDepth + (float)(rightMin - pending->sampleXMin) * slope;
                emitted = g_spanAllocCursor;
                if (*spanCount > 0) {
                    SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                    if (emitted->sampleXMin == lastVisible->sampleXMax + 1) {
                        lastVisible->sampleXMax = emitted->sampleXMax;
                        lastVisible->invDepthStep = emitted->invDepthStep;
                        lastVisible->next = emitted->next;
                        g_spanLastNode = lastVisible;
                    } else {
                        spanList[*spanCount] = emitted;
                        ++*spanCount;
                    }
                } else {
                    spanList[*spanCount] = emitted;
                    ++*spanCount;
                }
                pending = ++g_spanAllocCursor;
                pending->next = 0;
                continue;
            }
            // Retail keeps this end test although the check above implies it (0x491c22).
            if (pending->sampleXMin <= current->sampleXMin && current->sampleXMin <= pending->sampleXMax
                && pending->sampleXMax <= current->sampleXMax) {
                if (pending->sampleXMin < current->sampleXMin) {
                    pending->sampleXMax = current->sampleXMin - 1;
                    pending->invDepthStep
                        = (float)(pending->sampleXMax - pending->sampleXMin) * pending->depthSlope + pending->invDepth;
                    emitted = g_spanAllocCursor;
                    if (*spanCount > 0) {
                        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
                        if (emitted->sampleXMin == lastVisible->sampleXMax + 1) {
                            lastVisible->sampleXMax = emitted->sampleXMax;
                            lastVisible->invDepthStep = emitted->invDepthStep;
                            lastVisible->next = emitted->next;
                            g_spanLastNode = lastVisible;
                        } else {
                            spanList[*spanCount] = emitted;
                            ++*spanCount;
                        }
                    } else {
                        spanList[*spanCount] = emitted;
                        ++*spanCount;
                    }
                }
                return;
            }
        }
    }

    g_spanLastNode = g_spanAllocCursor;
    ++g_spanAllocCursor;
    emitted = g_spanLastNode;
    if (*spanCount > 0) {
        SpanNodePartial* lastVisible = spanList[*spanCount - 1];
        if (emitted->sampleXMin == lastVisible->sampleXMax + 1) {
            lastVisible->sampleXMax = emitted->sampleXMax;
            lastVisible->invDepthStep = emitted->invDepthStep;
            lastVisible->next = emitted->next;
            g_spanLastNode = lastVisible;
        } else {
            spanList[*spanCount] = emitted;
            ++*spanCount;
        }
    } else {
        spanList[*spanCount] = emitted;
        ++*spanCount;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-buildspanlistfast
 * @recoil-artifact defines .text recoil:function:0x491da0: zRndrSpanOcclusionBuildSpanListFast.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: emit the pending span as the only visible span and advance the span
 * allocation cursor.
 * Evidence: BN writes null next, stores gRndr_SpanAllocCursor into spanList[0],
 * writes spanCount = 1, increments the cursor by one zRndr_SpanNode, and
 * returns.
 */
void __fastcall zRndrSpanOcclusionBuildSpanListFast(zRndr::SpanNodePartial** spanList, int, int* spanCount)
{
    zRndr::g_spanAllocCursor->next = 0;
    spanList[0] = zRndr::g_spanAllocCursor;
    *spanCount = 1;
    ++zRndr::g_spanAllocCursor;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-testcolumnvisibility
 * @recoil-artifact defines .text recoil:function:0x491dd0: zRndrSpanOcclusionTestColumnVisibility.
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: test whether the pending span node remains visible in one occlusion
 * column.
 *
 * Evidence: retail copies each column-list occluder and trims the pending
 * g_spanAllocCursor span in place. Endpoint depth bounds avoid an ambiguous
 * overlap test; zRndrSpanOcclusionTestSpanDepthOrderPair resolves the remainder.
 * The invDepthStep field holds the inverse depth at the span's final endpoint.
 */
void __fastcall zRndrSpanOcclusionTestColumnVisibility(int columnIndex, int* isVisible)
{
    zRndr::SpanNodePartial* columnHead = zRndr::g_spanColumnHeadTable[columnIndex];
    zRndr::SpanNodePartial* pending = zRndr::g_spanAllocCursor;
    *isVisible = 0;
    if (columnHead == 0) {
        *isVisible = 1;
        return;
    }
    if (pending->sampleXMax < columnHead->sampleXMin) {
        *isVisible = 1;
        return;
    }

    pending->next = 0;
    zRndr::SpanNodePartial* minDepthSpan = 0;
    zRndr::SpanNodePartial* maxDepthSpan = 0;
    float maxDepth = 0.0f;
    float minDepth = 0.0f;
    zRndr::SpanNodePartial occluder;
    // Retail re-tests the walk pointer at the loop head (0x491e2d), so the walk
    // starts from a copy of the column head rather than the tested variable.
    zRndr::SpanNodePartial* current = columnHead;
    for (;;) {
        while (current != 0 && pending->sampleXMin > current->sampleXMax) {
            current = current->next;
        }
        // Retail null-tests the walk pointer again after redirecting it to the
        // occluder copy (0x491e78) and reads the copy through it.
        if (current != 0) {
            occluder.next = current->next;
            occluder.sampleXMin = current->sampleXMin;
            occluder.sampleXMax = current->sampleXMax;
            occluder.invDepth = current->invDepth;
            occluder.invDepthStep = current->invDepthStep;
            occluder.depthSlope = current->depthSlope;
            current = &occluder;
        }
        if (current == 0) {
            *isVisible = 1;
            return;
        }
        if (pending->sampleXMax < current->sampleXMin) {
            *isVisible = 1;
            return;
        }

        // Retail compares the endpoint depths double-widened (fld/fld/fcompp)
        // and selects them on the FPU before each store or multiply.
        if (maxDepthSpan != pending) {
            maxDepth
                = (double)pending->invDepth > pending->invDepthStep ? (double)pending->invDepth : pending->invDepthStep;
            maxDepthSpan = pending;
        }
        const float occluderMinDepth
            = (double)current->invDepth < current->invDepthStep ? (double)current->invDepth : current->invDepthStep;
        int pendingInFront;
        if (occluderMinDepth * zRndr::g_spanDepthBiasPlusOne >= maxDepth) {
            pendingInFront = 0;
        } else {
            if (minDepthSpan != pending) {
                minDepth = (double)pending->invDepth < pending->invDepthStep ? (double)pending->invDepth
                                                                             : pending->invDepthStep;
                minDepthSpan = pending;
            }
            if (minDepth * zRndr::g_spanDepthBiasPlusOne
                >= ((double)current->invDepth > current->invDepthStep ? (double)current->invDepth
                                                                      : current->invDepthStep)) {
                pendingInFront = 1;
            } else {
                pendingInFront = zRndrSpanOcclusionTestSpanDepthOrderPair(pending, current);
            }
        }

        if (pendingInFront != 0) {
            if (current->sampleXMax <= pending->sampleXMax) {
                *isVisible = 1;
                return;
            }
            if (pending->sampleXMin <= current->sampleXMin && current->sampleXMin <= pending->sampleXMax) {
                *isVisible = 1;
                return;
            }
            if (pending->sampleXMin > current->sampleXMin && pending->sampleXMax < current->sampleXMax) {
                *isVisible = 1;
                return;
            }
        } else if (current->sampleXMin <= pending->sampleXMin) {
            if (current->sampleXMax >= pending->sampleXMin) {
                if (pending->sampleXMax <= current->sampleXMax) {
                    return;
                }
                pending->sampleXMin = current->sampleXMax + 1;
            } else if (pending->sampleXMax <= current->sampleXMax) {
                return;
            }
        } else {
            if (current->sampleXMax < pending->sampleXMax) {
                *isVisible = 1;
                return;
            }
            // Retail keeps this end test although the check above implies it (0x491fb5);
            // VC5 folds it when spelled current-first.
            if (pending->sampleXMin <= current->sampleXMin && current->sampleXMin <= pending->sampleXMax
                && pending->sampleXMax <= current->sampleXMax) {
                if (pending->sampleXMin < current->sampleXMin) {
                    *isVisible = 1;
                }
                return;
            }
        }
    }
}
