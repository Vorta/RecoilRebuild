#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

struct zVec3;
struct zVec2;
struct zProjectedPoint;
struct zColorRgb;
struct zVidImagePartial;
struct zVidPaletteRemapRecipe;
struct zVidRect32;
struct zOpt_ViewRectSection;
struct zImage_TexDirEntryPartial;
struct HudUiRect;
struct zRndr_LensFlareSource {
    float depthFadeInvZMin;
    float depthFadeInvZMax;
    float depthFadeScale;
    int lensFlareEnabled;
    unsigned char padding_10[0x04];
    float fadeNear;
    float fadeFar;
};

RECOIL_STATIC_ASSERT(sizeof(zRndr_LensFlareSource) == 0x1c);

struct zRndr_LensFlareVisibleSampleDef {
    float sampleCenterX;
    float sampleCenterY;
    float depthDivisor;
    unsigned int packedColor16;
    zRndr_LensFlareSource *lensFlareSource;
};

RECOIL_STATIC_ASSERT(sizeof(zRndr_LensFlareVisibleSampleDef) == 0x14);

struct zRndr_LinePoint2I {
    int x;
    int y;
};

struct zRndr_LineClipRect2I {
    int left;
    int top;
    int right;
    int bottom;
};

RECOIL_STATIC_ASSERT(sizeof(zRndr_LineClipRect2I) == 0x10);

extern "C" {
typedef void (RECOIL_FASTCALL *zVideo_BltSourceToPrimaryProc)(void *self, int dstX,
                                                              int dstY,
                                                              int clipFlags,
                                                              void *srcRect);

extern zVideo_BltSourceToPrimaryProc g_zVideo_pfnBltSourceToPrimary;
}

extern float g_zRndr_InverseZTolerance;
extern int g_zRndr_ActivePaletteRemapKey;
extern int g_zRndr_ActivePaletteShadeRecipeIndex;
extern int g_zRndr_CircleCenterX;
extern int g_zRndr_CircleCenterY;
extern int g_zRndr_CircleDrawAuxArg;

namespace zRndr {
struct ActiveRegionRectPartial {
    int x;
    int y;
    int right;
    int bottom;
};

struct FogParamsPartial {
    float colorRgb01[3];
    int packedColorRed;
    int packedColorGreen;
    int packedColorBlue;
    int packedColor16;
    int packedColor16Dup;
    int packedColorRamp[32];
};

struct SpanOccluderPolyPartial {
    float vertices[8][3];
    int vertCount;
};

struct SpanNodePartial {
    SpanNodePartial *next;
    int sampleXMin;
    int sampleXMax;
    float invDepth;
    float invDepthStep;
    float depthSlope;
};

struct LensFlareSamplePartial {
    float x;
    float y;
    float reciprocalZ;
    int packedColor16;
    int lensFlareSource;
};

struct QueuedVec3 {
    float x;
    float y;
    float z;
};

struct TransparentQueuedPolyDrawCmd {
    zImage_TexDirEntryPartial *materialRef;
    int vertexCount;
    QueuedVec3 polyVerts[67];
    QueuedVec3 triVerts[3];
    float triUVs[6];
    int scanConvertMode;
    int hasClippedTriVerts;
    float savedInvDepthBias;
    float savedInvDepthScale;
    int alphaOrShadeBits;
    int shadeOrSpanMode;
    int texKey;
};

struct OverwriteQueuedPolyDrawCmd {
    int hasClippedTriVerts;
    QueuedVec3 polyVerts[67];
    QueuedVec3 triVerts[3];
    float alphaOrShadeF;
    int shadeOrSpanMode;
    int vertexCount;
    unsigned char unknown_358[0x18];
    zImage_TexDirEntryPartial *materialRef;
    unsigned char unknown_374[0x104];
    int scanConvertMode;
    unsigned char unknown_47c[0x04];
    float savedInvDepthBias;
    float savedInvDepthScale;
    unsigned char unknown_488[0x04];
};

typedef void (RECOIL_FASTCALL *SpanBuildProc)(SpanNodePartial **spanList, int columnIndex,
                                              int *spanCount);
typedef void (RECOIL_FASTCALL *OverlayBlendRowProc)(unsigned short *rowPixels16,
                                                    int pixelCount);
typedef void (RECOIL_FASTCALL *SpanRoutineProc)(int spanOpContext,
                                                int pixelCount);
typedef void (RECOIL_FASTCALL *PointOpProc)(void *frameBuffer, int y, int x,
                                            int color16);
typedef void (RECOIL_FASTCALL *FlatImmediateSpanProc)(int flatSpanOpEcxArg,
                                                      int flatSpanOpEdxArg,
                                                      int pixelCount);
typedef void (RECOIL_FASTCALL *TexturedQueuedSpanProc)(int texU, int texV,
                                                       int pixelCount,
                                                       int texVShift);

extern void *g_frameBuffer;
extern int g_activeRegionWidth;
extern int g_activeRegionHeight;
extern ActiveRegionRectPartial g_activeRegionRect;
extern int g_pitchBytes;
extern int g_bytesPerPixel;
extern int g_videoStrideMirror0;
extern int g_videoStrideMirror1;
extern int g_scanConvertMode;
extern int g_perspectiveTextureEnabled;
extern int g_perspectiveTextureDeltaXPow2;
extern int g_perspectiveTextureDeltaXBytes;
extern int g_perspectiveTextureDeltaXInput;
extern int g_perspectiveTextureDeltaXShift;
extern float g_perspectiveTextureDeltaXPow2F;
extern float g_perspectiveTextureFarZInv;
extern int g_perspectiveAdaptiveMinSpan;
extern int g_perspectiveAdaptiveMaxSpan;
extern float g_perspectiveAdaptiveSlope;
extern float g_inverseDepthBias;
extern float g_inverseDepthScale;
extern float g_spanDepthBias;
extern float g_spanDepthBiasPlusOne;
extern float g_spanDepthBiasPlusOneInv;
extern FogParamsPartial g_fogColorParams;
extern FogParamsPartial g_fogTargetParamsDirect;
extern FogParamsPartial g_fogTargetParamsStaged;
extern FogParamsPartial g_fogParamsActive;
extern SpanOccluderPolyPartial g_spanOccluderPolys[8];
extern int g_spanOccluderPolyCount;
extern SpanNodePartial *g_spanAllocCursor;
extern SpanNodePartial **g_spanColumnHeadTable;
extern SpanNodePartial *g_spanPoolBase;
extern SpanNodePartial *g_spanLastNode;
extern SpanNodePartial *g_spanIterNode;
extern SpanNodePartial *g_spanIterPrevLink;
extern int g_spanReservedWriteOnly;
extern int g_spanColumnCount;
extern int g_spanColumnCountPadded;
extern SpanBuildProc g_pfnBuildSpanList;
extern SpanBuildProc g_pfnBuildSpanListSecondary;
extern OverlayBlendRowProc g_pfnOverlayBlendRow;
extern unsigned int g_swOverlayPremulPacked;
extern unsigned int g_swOverlayPremulPackedRot16;
extern int g_swOverlayDstScale5;
extern int g_pixelPackRedBits;
extern int g_pixelPackGreenBits;
extern int g_pixelPackBlueBits;
extern unsigned int g_pixelPackRedMask;
extern unsigned int g_pixelPackGreenMask;
extern unsigned int g_pixelPackBlueMask;
extern int g_pixelPackRedShift;
extern int g_pixelPackGreenShift;
extern int g_pixelPackBlueShift;
extern unsigned short g_mmxMaskRedPacked[4];
extern unsigned short g_mmxMaskGreenPacked[4];
extern unsigned short g_mmxMaskGreenBits[4];
extern unsigned short g_mmxMaskBlueBits[4];
extern unsigned short g_mmxBitsBlue255[4];
extern unsigned short g_mmxBitsGreen255[4];
extern unsigned short g_mmxBitsRed255[4];
extern unsigned short g_mmxFogFactors[4];
extern int g_mmxUPair[2];
extern int g_mmxVPair[2];
extern int g_mmxUStepDup2[2];
extern int g_mmxVStepDup2[2];
extern int g_mmxUMask[2];
extern int g_mmxVMask[2];
extern int g_mmxVShiftCounts[2];
extern unsigned char *g_spanActiveTexels;
extern char *g_spanActiveTexAlphaMap;
extern unsigned short *g_spanActiveTexPalette;
extern int g_spanTexUAdvance;
extern int g_spanTexVAdvance;
extern unsigned short *g_spanCurrentDst16;
extern int g_spanActiveConstAlphaBits;
extern int g_spanActiveTexVMask;
extern int g_spanActiveTexUMask;
extern int g_spanActiveShadeFixed16;
extern int g_spanActiveShadeStepFixed16;
extern PointOpProc g_pfnPointOpCandidate;
extern PointOpProc g_pfnPointOpActive;
extern SpanRoutineProc g_pfnImmediateRaster4;
extern SpanRoutineProc g_pfnImmediateRasterReserved;
extern SpanRoutineProc g_pfnImmediateRaster5;
extern SpanRoutineProc g_pfnSelectedSpanOp;
extern TexturedQueuedSpanProc g_pfnSelectedSpanOp_Mode0;
extern FlatImmediateSpanProc g_pfnFlatImmediateSpanOp;
extern TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode1;
extern SpanRoutineProc g_pfnTexturedQueuedFinalize;
extern SpanRoutineProc g_pfnTexturedQueuedFinalizeAlt;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode0;
extern TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode0;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode1;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode1;
extern TransparentQueuedPolyDrawCmd g_transparentQueue[0x15e];
extern OverwriteQueuedPolyDrawCmd g_overwriteQueue[0x15e];
extern int g_transparentQueueSortIndices[0x15e];
extern int g_transparentQueueCount;
extern int g_overwriteQueueCount;
extern int g_overlayBlendEnabled;
extern int g_overlayBlendRectLeft;
extern int g_overlayBlendRectTop;
extern int g_overlayBlendRectRight;
extern int g_overlayBlendRectBottom;
extern unsigned int g_overlayBlendPackedColor16;
extern double g_overlayBlendAlpha;
extern int g_lensFlareSampleQueueCount;
extern int g_lensFlareVisibleSampleCount;
extern int g_lensFlareVisibilityActive;
extern zImage_TexDirEntryPartial *g_lensFlareVisibleSampleStages[4];
extern zRndr_LensFlareVisibleSampleDef *g_lensFlareVisibleSampleDefs[0x40];
extern LensFlareSamplePartial g_lensFlareSampleQueue[0x28a];
extern int g_textureMipSelectionEnabled;
extern int g_textureMipReservedWriteOnly;
extern int g_renderStateReadyWriteOnlyFlag;
extern int g_renderStateReservedWriteOnly;
extern int g_initField00;
extern int g_initField04;
extern int g_initField08;
extern int g_initField0C;
extern int g_initField10;
extern int g_initField14;
extern int g_defaultGraphicsFlags;
extern int *g_graphicsFlags;

RECOIL_NOINLINE int RECOIL_CDECL InitGlobals();
RECOIL_NOINLINE void RECOIL_STDCALL
SetPerspectiveAdaptiveCorrection(float perspectiveAdaptiveCorrection);
RECOIL_NOINLINE void RECOIL_FASTCALL SetPerspectiveAdaptiveSpanParams(int minSpan,
                                                                      int maxSpan,
                                                                      float slope);
RECOIL_NOINLINE void *RECOIL_FASTCALL GetActiveRegionState(int *outWidth,
                                                           int *outHeight,
                                                           int *outBitsPerPixel,
                                                           int *outPitchBytes);
RECOIL_NOINLINE void RECOIL_FASTCALL SetFrameBufferRegion(void *pixels,
                                                          zOpt_ViewRectSection *activeRegionRect,
                                                          int bitsPerPixel,
                                                          int pitchBytes);
RECOIL_NOINLINE void RECOIL_FASTCALL SetActiveRegionSizeFromRect(HudUiRect *rect);
RECOIL_NOINLINE void RECOIL_FASTCALL SetVideoStrideMirrors(int stride);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanOcclusionAddPolygon(const zVec3 *vertices,
                                                             int vertCount);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanOcclusionSubmitOccluderRect(const HudUiRect *rect,
                                                                     int halveIfReplicate,
                                                                     float z);
RECOIL_NOINLINE int RECOIL_FASTCALL SpanOcclusionInit(int height);
RECOIL_NOINLINE void RECOIL_CDECL SpanOcclusionBuildColumnHeadTable();
RECOIL_NOINLINE void RECOIL_FASTCALL
SpanOcclusionRasterizeOccluderPoly(SpanOccluderPolyPartial *poly, int vertCount);
RECOIL_NOINLINE void RECOIL_CDECL SpanOcclusionResetFrame();
RECOIL_NOINLINE void RECOIL_CDECL SpanOcclusionShutdown();
RECOIL_NOINLINE void RECOIL_FASTCALL OverlayBlendRow555_Scalar(unsigned short *rowPixels16,
                                                               int pixelCount);
RECOIL_NOINLINE void RECOIL_FASTCALL OverlayBlendRow565_Scalar(unsigned short *rowPixels16,
                                                               int pixelCount);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMmxSetPixelFormatMasks(int greenBits);
RECOIL_NOINLINE void RECOIL_CDECL SelectSpanRoutines();
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565_Mmx_FromPal8(FogParamsPartial *params,
                                                                    int packedRed,
                                                                    int packedGreen,
                                                                    int packedBlue);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromPal8(int texU,
                                                                         int texV,
                                                                         int pixelCount,
                                                                         int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromPal8To565(int texU, int texV,
                                                               int pixelCount,
                                                               int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromTex16To565(int texU,
                                                                int texV,
                                                                int pixelCount,
                                                                int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565FromTex16Alpha8(int texU,
                                                                      int texV,
                                                                      int pixelCount,
                                                                      int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555FromTex16Alpha8(int texU,
                                                                      int texV,
                                                                      int pixelCount,
                                                                      int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromTex16Alpha8(
    int texU, int texV, int pixelCount, int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFromTex16Alpha8(
    int texU, int texV, int pixelCount, int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565MmxFromTex16Alpha8(int texU,
                                                                         int texV,
                                                                         int pixelCount,
                                                                         int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555MmxFromTex16Alpha8(int texU,
                                                                         int texV,
                                                                         int pixelCount,
                                                                         int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565FromPal8Alpha8(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555FromPal8Alpha8(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromPal8Alpha8(
    int texU, int texV, int pixelCount, int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFromPal8Alpha8(
    int texU, int texV, int pixelCount, int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565MmxFromPal8Alpha8(int texU,
                                                                        int texV,
                                                                        int pixelCount,
                                                                        int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555MmxFromPal8Alpha8(int texU,
                                                                        int texV,
                                                                        int pixelCount,
                                                                        int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromTex16(int texU,
                                                                          int texV,
                                                                          int pixelCount,
                                                                          int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFromTex16(int texU,
                                                                          int texV,
                                                                          int pixelCount,
                                                                          int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFastFromPal8(
    int texU, int texV, int pixelCount, int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFastFromPal8(
    int texU, int texV, int pixelCount, int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan565Scalar(unsigned short *pixels,
                                                           int pixelCount,
                                                           int fogCoordFixed24,
                                                           int fogCoordStepFixed24);
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan555Scalar(unsigned short *pixels,
                                                           int pixelCount,
                                                           int fogCoordFixed24,
                                                           int fogCoordStepFixed24);
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan565Mmx(unsigned short *pixels,
                                                        int pixelCount,
                                                        int fogCoordFixed24,
                                                        int fogCoordStepFixed24);
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan555Mmx(unsigned short *pixels,
                                                        int pixelCount,
                                                        int fogCoordFixed24,
                                                        int fogCoordStepFixed24);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromTex16SwitchVShift(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromTex16SwitchVShift(int texU,
                                                                       int texV,
                                                                       int pixelCount,
                                                                       int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMmxSetTexUvMasksAndVShift(int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromTex16(int texU, int texV,
                                                         int pixelCount,
                                                         int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromTex16ExplicitVShift(int texU,
                                                                       int texV,
                                                                       int pixelCount,
                                                                       int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromPal8SwitchVShift(int texU,
                                                                    int texV,
                                                                    int pixelCount,
                                                                    int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromPal8SwitchVShift(int texU,
                                                                      int texV,
                                                                      int pixelCount,
                                                                      int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL SpanShade16FromPal8SwitchVShift(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift);
RECOIL_NOINLINE void RECOIL_FASTCALL FogColor_SetRgb01Clamped(zColorRgb *color);
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogTargetColorRgb01Clamped(zColorRgb *color);
RECOIL_NOINLINE void RECOIL_CDECL CommitDirectFogParamsIfChanged();
RECOIL_NOINLINE void RECOIL_CDECL CommitFogColorParamsIfChanged();
RECOIL_NOINLINE void RECOIL_CDECL CommitStagedFogParamsIfChanged();
RECOIL_NOINLINE void RECOIL_FASTCALL BlendPackedColor565WithFogInPlace(int *ioPackedColor,
                                                                       int blend255);
RECOIL_NOINLINE void RECOIL_CDECL LensFlare_ResetSampleQueue();
RECOIL_NOINLINE void RECOIL_FASTCALL LensFlare_DrawQueuedSample16_ClippedFramebuffer(
    LensFlareSamplePartial *sample, int yOffsetPixels, float screenScale);
RECOIL_NOINLINE void RECOIL_FASTCALL LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer(
    int yOffsetPixels, float screenScale);
} // namespace zRndr

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_InsertSpanNode_Local(
    zRndr::SpanNodePartial **spanList, int columnIndex, int *spanCount);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest(
    zRndr::SpanNodePartial **spanList, int columnIndex, int *spanCount);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_BuildSpanList(
    zRndr::SpanNodePartial **spanList, int columnIndex, int *spanCount);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_BuildSpanListFast(
    zRndr::SpanNodePartial **spanList, int columnIndex, int *spanCount);

RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_SpanOcclusion_TestColumnVisibility(int columnIndex, int *isVisible);

RECOIL_NOINLINE int RECOIL_FASTCALL
zRndr_SpanOcclusion_TestPointVisibility(zVec3 *samplePoint);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_TestSample(int x, int y,
                                                                    int color16);

RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_DrawCircleOctants16_Framebuffer(int y, int x, int packedColor);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawCircleOutline16_Framebuffer(int centerX,
                                                                           int centerY,
                                                                           int radius,
                                                                           int packedColor,
                                                                           int auxArg);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_PlotPixel16(unsigned short *dstPixels, int y,
                                                       int x, int color16);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawLine16(unsigned short *dstPixels, int x0,
                                                      int y0, int x1,
                                                      int y1, int color16);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawLine16_Segmented(unsigned short *dstPixels,
                                                                int x0, int y0,
                                                                int x1, int y1,
                                                                int color16,
                                                                int segmentCount);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawLine16_Clipped(unsigned short *dstPixels,
                                                              const zRndr_LineClipRect2I *clipRect,
                                                              int x0, int y0,
                                                              int x1, int y1,
                                                              int color16);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FillSpan16Opaque(int packedColor16,
                                                            int pixelCount);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FillSpan555Solid(int packedColor16,
                                                            int blendAlpha,
                                                            int pixelCount);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FillSpan565Solid(int packedColor16,
                                                            int blendAlpha,
                                                            int pixelCount);

RECOIL_NOINLINE int RECOIL_FASTCALL zRndr_SpanOcclusion_TestSpanDepthOrderPair(
    zRndr::SpanNodePartial *lhs, zRndr::SpanNodePartial *rhs);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_RasterizePolyWithSpanList(zVec3 *vertices,
                                                                     zVec3 *planeVerts,
                                                                     int vertCount,
                                                                     int spanOpContext);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_RasterizePoly(zVec3 *vertices, int vertCount,
                                                         int spanOpContext);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawFlatImmediate(zVec3 *vertices, zVec3 *planeVertices,
                                                             int vertCount,
                                                             int flatSpanOpEdxArg,
                                                             int flatSpanOpEcxArg);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SubmitPolyWithSpanList(
    zVec3 *entryVertices, zVec3 *entryPlaneVertices, int spanOpContext,
    int alpha255, int vertCount, int queueOverwrite);

RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL zRndr_TextureMip_SelectVariantImage(
    zImage_TexDirEntryPartial *entry, const zVec3 *triVerts, int vertCount,
    const zVec2 *vertexUvPairs, const zVec2 *mipParamsA, const zVec2 *mipParamsB,
    const zVec2 *mipParamsC);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawFlatQueued(zImage_TexDirEntryPartial *entry,
                                                          zVec3 *polyVerts, zVec3 *triVerts,
                                                          zVec2 *triUVs, int vertCount,
                                                          int paletteIndex);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawTexturedQueuedAlpha(
    zImage_TexDirEntryPartial *entry, zVec3 *projectedVerts, zVec3 *clippedTriVerts,
    zVec3 *triVerts, zVec2 *triUVs, int vertCount, int variantIndex);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawTexturedQueued(
    zImage_TexDirEntryPartial *entry, zVec3 *projectedVerts, zVec3 *clippedTriVerts,
    zVec3 *triVerts, zVec2 *triUVs, zVec3 *shadeTriplet, int vertCount,
    int fanTriIndex, int texKey);

RECOIL_NOINLINE void RECOIL_FASTCALL Renderer_DrawPolyTLV(zImage_TexDirEntryPartial *entry,
                                                          zVec3 *polyVerts, zVec3 *triVerts,
                                                          zVec2 *triUVs, int vertexCount,
                                                          float alpha, int texKey);

RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_DrawTexturedFanTri(zImage_TexDirEntryPartial *entry, zVec3 *projectedVerts,
                         zVec3 *clippedTriVerts, zVec3 *triVerts, zVec2 *triUVs,
                         int vertCount, int alpha255, int variantIndex);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SubmitTexturedPolyUniformAlphaOrShade(
    zVec3 *projectedPolyVerts, zVec3 *clippedTriVerts, zVec3 *triData9f, zVec2 *triUVs,
    int vertexCount, zImage_TexDirEntryPartial *entry, float alphaOrShadeF,
    int queueOverwrite);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SubmitTexturedPolyPerVertexAlphaOrShade(
    zVec3 *projectedPolyVerts, zVec3 *clippedTriVerts, zVec3 *triData9f, zVec2 *triUVs,
    float *perVertexAlphaOrShadeF, int shadeOrSpanMode, int vertexCount,
    zImage_TexDirEntryPartial *entry, int preservePaletteRemapKey,
    int queueOverwrite);

RECOIL_NOINLINE void RECOIL_CDECL zRndr_FlushTransparentQueue();
RECOIL_NOINLINE void RECOIL_CDECL zRndr_FlushOverwriteQueue();
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_OverlayRect_Submit(unsigned int packedColor16,
                                                              zVidRect32 *rectOrNull,
                                                              double alpha);
RECOIL_NOINLINE void RECOIL_CDECL zRndr_OverlayRect_FlushSw();

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawImmediateLine(int x0, int y0,
                                                             int x1, int y1,
                                                             int color16);

RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_DrawClippedImmediateLineStrip(const zRndr_LinePoint2I *points, int segmentCount,
                                    const void *clipRect, int color16);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_QueueProjectedSample(
    zProjectedPoint *projectedPoint, int packedColor16, int lensFlareSource);

RECOIL_NOINLINE int RECOIL_CDECL zRndr_LensFlare_GetQueuedSampleCount();

RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList(int startIndex);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_SetVisibleSampleStage(
    int stageIndex, zImage_TexDirEntryPartial *stageTexDirEntry);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_DrawSampleStageClipped(
    const zVec2 *sampleCenter, zImage_TexDirEntryPartial *stageTexDirEntry, float sampleRadius,
    const zRndr_LineClipRect2I *clipRect);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_DrawVisibleSampleStages(
    zRndr_LensFlareVisibleSampleDef *visibleSampleDef, float visibilityAlpha);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_DrawVisibleSample(int sampleIndex);

RECOIL_NOINLINE void RECOIL_CDECL zRndr_LensFlare_DrawVisibleSamples();

RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_SpanOcclusion_FilterSampleList(int visibleSampleIndex, zVec3 *outPoint);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FogTargetColorStaged_SetRgb01Clamped(zColorRgb *color);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SetPaletteRemapKey(zVidPaletteRemapRecipe *recipe,
                                                              float shadeLevel);

RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SetPaletteRemapKeyFromRgb01(zColorRgb *rgb01,
                                                                       float shadeLevel);

RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_SetPaletteShadeRecipeIndex(zVidPaletteRemapRecipe *recipe);
