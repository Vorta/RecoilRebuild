#pragma once

/*
 * C declarations of the zRender API for the zRender units (C++ units see the
 * same functions and data with C linkage in zrndr.h and zvid.h; the other C
 * units see only the zrndr.h C view).
 */

#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"

extern int g_zRndr_ActivePaletteRemapKey;
extern int g_zRndr_ActivePaletteShadeRecipeIndex;
extern float gRndr_PerspTexScaledUOverZ0;
extern float gRndr_PerspTexScaledVOverZ0;
extern float gRndr_PerspTexScaledUOverZ1;
extern float gRndr_PerspTexScaledVOverZ1;
extern float gRndr_PerspTexScaledUOverZ2;
extern float gRndr_PerspTexScaledVOverZ2;
extern float gRndr_PerspTexScaledUOverZBase;
extern float gRndr_PerspPlaneOriginX;
extern float gRndr_PerspPlaneOriginY;
extern float gRndr_PerspTexScaledUOverZStepX;
extern float gRndr_PerspTexScaledUOverZStepY;
extern float gRndr_PerspInvDepthBase;
extern float gRndr_PerspInvDepthStepX;
extern float gRndr_PerspInvDepthStepY;
extern float gRndr_PerspTexScaledVOverZStepX;
extern float gRndr_PerspTexScaledVOverZStepY;
extern float gRndr_PerspTexScaledVOverZBase;
extern int g_zRndr_CircleDrawAuxArg;

extern void* g_frameBuffer;
extern int g_activeRegionWidth;
extern int g_activeRegionHeight;
extern ActiveRegionRectPartial g_activeRegionRect;
extern int g_pitchBytes;
extern int g_bytesPerPixel;
extern int g_videoStrideMirror0;
extern int g_videoStrideMirror1;
extern int g_perspectiveTextureDeltaXPow2;
extern int g_perspectiveTextureDeltaXBytes;
extern int g_perspectiveTextureDeltaXInput;
extern int g_perspectiveTextureDeltaXShift;
extern float g_perspectiveTextureDeltaXPow2F;
extern float g_perspectiveTextureFarZInv;
extern int g_perspectiveAdaptiveMinSpan;
extern int g_perspectiveAdaptiveMaxSpan;
extern float g_perspectiveAdaptiveSlope;
extern float g_spanDepthBias;
extern float g_spanDepthBiasPlusOne;
extern float g_spanDepthBiasPlusOneInv;
extern FogParamsPartial g_fogColorParams;
extern FogParamsPartial g_fogTargetParamsStaged;
extern FogParamsPartial g_fogTargetParamsDirect;
extern FogParamsPartial g_fogParamsActive;
extern SpanOccluderPolyPartial g_spanOccluderPolys[8];
extern int g_spanOccluderPolyCount;
extern SpanNodePartial** g_spanColumnHeadTable;
extern SpanNodePartial* g_spanPoolBase;
extern SpanNodePartial* g_spanLastNode;
extern SpanNodePartial* g_spanIterNode;
extern SpanNodePartial* g_spanIterPrevLink;
extern int g_spanReservedWriteOnly;
extern int g_spanColumnCount;
extern int g_spanColumnCountPadded;
extern SpanBuildProc g_pfnBuildSpanList;
extern SpanBuildProc g_pfnBuildSpanListSecondary;
extern OverlayBlendRowProc g_pfnOverlayBlendRow;
extern unsigned int g_swOverlayPremulPacked;
extern unsigned int g_swOverlayPremulPackedRot16;
extern int g_swOverlayDstScale5;
extern unsigned int g_swOverlayPremulRPair;
extern unsigned int g_swOverlayPremulBPair;
extern unsigned int g_swOverlayPremulGPair;
extern int g_pixelPackRedBits;
extern int g_pixelPackBlueBits;
extern unsigned int g_pixelPackRedMask;
extern unsigned int g_pixelPackGreenMask;
extern unsigned int g_pixelPackBlueMask;
extern int g_pixelPackRedShift;
extern int g_pixelPackGreenShift;
extern int g_pixelPackBlueShift;
extern char* g_spanQueuedTexAlphaMap;
extern int g_spanActiveTexShift;
extern int g_spanActiveTexVMask;
extern int g_spanActiveTexUMask;
extern unsigned char* g_spanActiveTexPixels;
extern unsigned short* g_spanActiveTexPalette;
extern int g_spanActiveTexUStepFixed20;
extern int g_spanActiveTexVStepFixed20;
extern unsigned short* g_spanCurrentSpanBaseAddr;
extern int g_spanActiveShadeFixed16;
extern int g_spanActiveShadeStepFixed16;
extern char* g_spanActiveTexAlphaMap;
extern zMmxQword g_mmxUStepDup2;
extern zRndr_SpanEspPivotSave* g_spanSavedEspSlot;
extern zMmxQword g_mmxUMask;
extern int g_spanActiveConstAlphaBits;
extern zMmxQword g_mmxVMask;
extern zMmxQword g_mmxVStepDup2;
extern zMmxQword g_mmxUPair;
extern zMmxQword g_mmxVShiftCounts;
extern zMmxQword g_mmxVPair;
extern unsigned short g_mmxBitsBlue255[4];
extern unsigned short g_mmxBitsGreen255[4];
extern unsigned short g_mmxBitsRed255[4];
extern short g_mmxMaskGreenPacked[4];
extern unsigned short g_mmxMaskRedPacked[4];
extern unsigned short g_mmxFogFactors[4];
extern unsigned short g_mmxMaskGreenBits[4];
extern unsigned short g_mmxMaskBlueBits[4];
extern SpanRoutineProc g_pfnSelectedSpanOp;
extern FlatImmediateSpanProc g_pfnFlatImmediateSpanOp;
extern TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnSelectedSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnSelectedSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode0;
extern TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode1;
extern TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode0;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode0;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode1;
extern TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode1;
extern ImmediateRaster4Proc g_pfnImmediateRaster4;
extern ImmediateRasterSegmentedProc g_pfnImmediateRasterReserved;
extern ImmediateRaster5Proc g_pfnImmediateRaster5;
extern PointOpProc g_pfnPointOpCandidate;
extern PointOpProc g_pfnPointOpActive;
extern SpanRoutineProc g_pfnTexturedQueuedFinalize;
extern SpanRoutineProc g_pfnTexturedQueuedFinalizeAlt;
extern QueuedPolyBanks g_queuedPolyBanks;
extern int g_overlayBlendEnabled;
extern int g_overlayBlendRectLeft;
extern int g_overlayBlendRectTop;
extern int g_overlayBlendRectRight;
extern int g_overlayBlendRectBottom;
extern unsigned int g_overlayBlendPackedColor16;
extern double g_overlayBlendAlpha;
extern LensFlareFrameBank g_lensFlareBank;
extern int g_lensFlareVisibilityActive;
extern zImage_TexDirEntryPartial* g_lensFlareVisibleSampleStages[4];
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
extern int* g_graphicsFlags;

int __cdecl InitGlobals();
void __fastcall SetPerspectiveTextureDeltaX(int deltaX);
void __stdcall SetPerspectiveTextureFarZ(float farZ);
void __stdcall SetPerspectiveAdaptiveCorrection(float perspectiveAdaptiveCorrection);
void __fastcall SetPerspectiveAdaptiveSpanParams(int minSpan, int maxSpan, float slope);
void* __fastcall GetActiveRegionState(int* outWidth, int* outHeight, int* outBitsPerPixel, int* outPitchBytes);
void __fastcall
SetFrameBufferRegion(void* pixels, zOpt_ViewRectSection* activeRegionRect, int bitsPerPixel, int pitchBytes);
void __fastcall SetActiveRegionSizeFromRect(HudUiRect* rect);
void __fastcall SetVideoStrideMirrors(int stride);
void __fastcall SpanOcclusionAddPolygon(const zVec3* vertices, int vertCount);
void __fastcall SpanOcclusionSubmitOccluderRect(const HudUiRect* rect, int halveIfReplicate, float z);
int __fastcall SpanOcclusionInit(int height);
void __cdecl SpanOcclusionBuildColumnHeadTable();
void __fastcall SpanOcclusionRasterizeOccluderPoly(SpanOccluderPolyPartial* poly, int vertCount);
void __cdecl SpanOcclusionResetFrame();
int __cdecl SpanOcclusionShutdown();
void __fastcall OverlayBlendRow555Scalar(unsigned short* rowPixels16, int rightDelta);
void __fastcall OverlayBlendRow565Scalar(unsigned short* rowPixels16, int rightDelta);
void __fastcall OverlayBlendRow555Mmx(unsigned short* rowPixels16, int pixelCount);
void __fastcall OverlayBlendRow565Mmx(unsigned short* rowPixels16, int pixelCount);
void __fastcall SpanMmxSetPixelFormatMasks(int greenBits);
void __cdecl SelectSpanRoutines();
void __fastcall
FogTarget565SetPackedColorAndRamp(FogParamsPartial* params, int packedRed, int packedGreen, int packedBlue);
void __fastcall SpanAlphaBlend565ConstAlphaFromPal8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanMasked16FromPal8To565(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanMasked16FromTex16To565(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565FromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555FromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565ConstAlphaFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555ConstAlphaFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565MmxFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555MmxFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565FromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555FromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565ConstAlphaFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555ConstAlphaFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565MmxFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555MmxFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565ConstAlphaFromTex16(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555ConstAlphaFromTex16(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend565ConstAlphaFastFromPal8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanAlphaBlend555ConstAlphaFastFromPal8(int texU, int texV, int pixelCount, int texVShift);
void __fastcall
FogBlendSpan565Scalar(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24);
void __fastcall
FogBlendSpan555Scalar(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24);
void __fastcall
FogBlendSpan565Mmx(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24);
void __fastcall
FogBlendSpan555Mmx(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24);
void __fastcall SpanCopy16FromTex16SwitchVShift(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanMasked16FromTex16SwitchVShift(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanMmxSetTexUvMasksAndVShift(int texVShift);
void __fastcall SpanCopy16FromTex16(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanCopy16FromTex16ExplicitVShift(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanCopy16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanMasked16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift);
void __fastcall SpanShade16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift);
void __cdecl LensFlareResetSampleQueue();
void __fastcall
LensFlareDrawQueuedSample16ClippedFramebuffer(LensFlareSamplePartial* sample, float screenScale, int yOffsetPixels);
void __fastcall LensFlareDrawQueuedSamplesScaled16ClippedFramebuffer(float screenScale, int yOffsetPixels);

void __fastcall zRndr_SpanOcclusion_InsertSpanNode_Local(SpanNodePartial** spanList, int columnIndex, int* spanCount);
void __fastcall
zRndrSpanOcclusionInsertSpanNodeNoDepthTest(SpanNodePartial** spanList, int columnIndex, int* spanCount);
void __fastcall zRndrSpanOcclusionBuildSpanList(SpanNodePartial** spanList, int columnIndex, int* spanCount);
void __fastcall zRndrSpanOcclusionBuildSpanListFast(SpanNodePartial** spanList, int columnIndex, int* spanCount);
int __fastcall zRndrSpanOcclusionTestPointVisibility(zVec3* samplePoint);
void __fastcall zRndrSpanOcclusionTestSample(int x, int y, int color16);
void __fastcall zRndrDrawCircleOctants16Framebuffer(int y, int x, int packedColor);
void __fastcall zRndrDrawCircleOutline16Framebuffer(int centerX, int centerY, int radius, int packedColor, int auxArg);
void __fastcall zRndrPlotPixel16(unsigned short* dstPixels, int y, int x, int color16);
void __fastcall zRndrDrawLine16(unsigned short* dstPixels, int x0, int y0, int x1, int y1, int color16);
void __fastcall
zRndrDrawLine16Segmented(unsigned short* dstPixels, int x0, int y0, int x1, int y1, int color16, int segmentCount);
void __fastcall zRndrDrawLine16Clipped(
    unsigned short* dstPixels,
    const zRndr_LineClipRect2I* clipRect,
    int x0,
    int y0,
    int x1,
    int y1,
    int color16
);
void __fastcall zRndrFillSpan16Opaque(int packedColor16, int pixelCount);
void __fastcall zRndrFillSpan555Solid(int packedColor16, int blendAlpha, int pixelCount);
void __fastcall zRndrFillSpan565Solid(int packedColor16, int blendAlpha, int pixelCount);
int __fastcall zRndrSpanOcclusionTestSpanDepthOrderPair(SpanNodePartial* lhs, SpanNodePartial* rhs);
void __fastcall zRndrRasterizePolyWithSpanList(zVec3* vertices, zVec3* planeVerts, int vertCount, int spanOpContext);
void __fastcall zRndrRasterizePoly(zVec3* vertices, int vertCount, int spanOpContext);
void __fastcall zRndrDrawFlatImmediate(
    zVec3* vertices,
    zVec3* planeVertices,
    int vertCount,
    int flatSpanOpEdxArg,
    int flatSpanOpEcxArg
);
zVidImagePartial* __fastcall zRndrTextureMipSelectVariantImage(
    zImage_TexDirEntryPartial* entry,
    const zVec3* triVerts,
    int vertCount,
    const zVec2* vertexUvPairs,
    const zVec2* mipParamsA,
    const zVec2* mipParamsB,
    const zVec2* mipParamsC
);
void __fastcall zRndrDrawFlatQueued(
    zImage_TexDirEntryPartial* entry,
    zVec3* polyVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertCount,
    int paletteIndex
);
void __fastcall zRndrDrawTexturedQueuedAlpha(
    zImage_TexDirEntryPartial* entry,
    zVec3* projectedVerts,
    zVec3* clippedTriVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertCount,
    int variantIndex
);
void __fastcall zRndrDrawTexturedQueued(
    zImage_TexDirEntryPartial* entry,
    zVec3* projectedVerts,
    zVec3* clippedTriVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    zVec3* shadeTriplet,
    int vertCount,
    int fanTriIndex,
    int texKey
);
void __fastcall RendererDrawPolyTLV(
    zImage_TexDirEntryPartial* entry,
    zVec3* polyVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertexCount,
    float alpha,
    int texKey
);
void __fastcall zRndrDrawTexturedFanTri(
    zImage_TexDirEntryPartial* entry,
    zVec3* projectedVerts,
    zVec3* clippedTriVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertCount,
    int alpha255,
    int variantIndex
);
void __cdecl zRndrFlushTransparentQueue();
void __cdecl zRndrFlushOverwriteQueue();
void __fastcall zRndrOverlayRectSubmit(unsigned short packedColor16, double alpha, zVidRect32* rectOrNull);
void __fastcall zRndrOverlayRectFlushSw();
void __fastcall zRndrDrawImmediateLine(int x0, int y0, int x1, int y1, int color16);
void __fastcall zRndrDrawClippedImmediateLineStrip(
    const zRndr_LinePoint2I* points,
    int segmentCount,
    const void* clipRect,
    int color16
);
int __cdecl zRndrLensFlareGetQueuedSampleCount();
void __fastcall zRndrLensFlareDrawQueuedSamples16AndBuildVisibleList(int startIndex);
int __fastcall zRndrLensFlareBuildVisibleSampleListFromQueue(int startIndex);
void __fastcall zRndrLensFlareSetVisibleSampleStage(int stageIndex, zImage_TexDirEntryPartial* stageTexDirEntry);
void __fastcall zRndrLensFlareDrawSampleStageClipped(
    const zVec2* sampleCenter,
    zImage_TexDirEntryPartial* stageTexDirEntry,
    float sampleRadius,
    const zRndr_LineClipRect2I* clipRect
);
void __fastcall
zRndrLensFlareDrawVisibleSampleStages(zRndr_LensFlareVisibleSampleDef* visibleSampleDef, float visibilityAlpha);
void __fastcall zRndrLensFlareDrawVisibleSample(int sampleIndex);
void __cdecl zRndrLensFlareDrawVisibleSamples();
void __fastcall zRndrSpanOcclusionFilterSampleList(int visibleSampleIndex, zVec3* outPoint);

/* zhud_ui.h defines the HUD rectangle for the C++ units; the region and occluder setters read it. */
struct HudUiRect {
    int left;
    int top;
    int right;
    int bottom;
};

/* zVideo members the zRender units define (zvid.h declares them for C++ units). */
void __cdecl NoiseInitBuffers();
void __cdecl NoiseShutdownBuffers();
void __fastcall DrawNoiseRect(zVidRect32* rectOrNull, double intensity);
int __cdecl InitFrameScratchBuffers();
int __cdecl ShutdownFrameScratchBuffers();
void __fastcall ApplyBlueTintRect(zVidRect32* rectOrNull);
void __fastcall ApplyGreenMaskRect(zVidRect32* rectOrNull);
void __fastcall DrawAlphaBlendedLine(
    zVidRect32* clipRect,
    int x1,
    int y1,
    int x0,
    int y0,
    unsigned short color16,
    float alphaEnd,
    float alphaStart,
    int clipInset
);
void __fastcall DrawColoredLinesBatch(zVideoFxColoredLineRecord* lines, int count, zVidRect32* clipRectOrNull);
void __fastcall FxPass3CopySurfacePixelToScratchClipped(int dstDx, int dstDy, int srcDx, int srcDy);
void __fastcall FxPass3ApplyToCurrentSurface(
    int centerX,
    int centerY,
    int currentRadius,
    int maxRadius,
    int extent,
    float sinFreq,
    float sinPhase,
    zVidRect32* clipRectOrNull
);
void __fastcall buffBlurRegionCombined(zVidRect32* rectOrNull, int mode);
void __fastcall buffBlurRegionVertical(zVidRect32* rectOrNull, int mode);
void __fastcall buffBlurRegionHorizontal(zVidRect32* rectOrNull, int mode);
void __fastcall buffBlurRegionByMode(zVidRect32* rectOrNull, int mode);
void __fastcall CalcPow2ScratchFields(zVidImagePartial* image);
void __fastcall
BlitToActiveTarget(zVidImagePartial* image, int dstX, int dstY, unsigned short colorKey, zVidRect32* srcRect);
void __fastcall
BlitToFramebufferClipped(zVidImagePartial* image, int dstX, int dstY, unsigned short clipFlags, zVidRect32* srcRect);
