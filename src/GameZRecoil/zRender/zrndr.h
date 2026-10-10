#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

typedef struct zVec3 zVec3;
typedef struct zVec2 zVec2;
typedef struct zProjectedPoint zProjectedPoint;
typedef struct zColorRgb zColorRgb;
typedef struct zVidImagePartial zVidImagePartial;
typedef struct zVidPaletteRemapRecipe zVidPaletteRemapRecipe;
typedef struct zVidRect32 zVidRect32;
typedef struct zOpt_ViewRectSection zOpt_ViewRectSection;
typedef struct zImage_TexDirEntryPartial zImage_TexDirEntryPartial;
typedef struct HudUiRect HudUiRect;

/**
 * Authored zRndr lens-flare descriptor. BN xrefs from the queue and visible
 * sample paths use the enable flag at +0x0c and fade window at +0x14/+0x18.
 */
typedef struct zRndr_LensFlareSource {
    float depthFadeInvZMin;
    float depthFadeInvZMax;
    float depthFadeScale;
    int lensFlareEnabled;
    unsigned char padding_10[0x04];
    float fadeNear;
    float fadeFar;
} zRndr_LensFlareSource;

RECOIL_STATIC_ASSERT(sizeof(zRndr_LensFlareSource) == 0x1c);

/**
 * Authored visible lens-flare sample record. Queue entries share this first
 * 0x14-byte layout when promoted into the visible-sample pointer list.
 */
typedef struct zRndr_LensFlareVisibleSampleDef {
    float sampleCenterX;
    float sampleCenterY;
    float depthDivisor;
    unsigned int packedColor16;
    zRndr_LensFlareSource* lensFlareSource;
} zRndr_LensFlareVisibleSampleDef;

RECOIL_STATIC_ASSERT(sizeof(zRndr_LensFlareVisibleSampleDef) == 0x14);

typedef struct zRndr_LinePoint2I {
    int x;
    int y;
} zRndr_LinePoint2I;

typedef struct zRndr_LineClipRect2I {
    int left;
    int top;
    int right;
    int bottom;
} zRndr_LineClipRect2I;

RECOIL_STATIC_ASSERT(sizeof(zRndr_LineClipRect2I) == 0x10);

#ifdef __cplusplus
extern "C" {
#endif
typedef void(__fastcall* zVideo_BltSourceToPrimaryProc)(
    zVidImagePartial* image,
    int dstX,
    int dstY,
    unsigned short colorKey,
    zVidRect32* srcRect
);

extern zVideo_BltSourceToPrimaryProc g_zVideo_pfnBltSourceToPrimary;
#ifdef __cplusplus
}

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

namespace zRndr_GlobalStringTable {
extern "C" {
#endif
void __fastcall LoadDynamicEntriesFromPath(char* path);
#ifdef __cplusplus
}
} // namespace zRndr_GlobalStringTable

namespace zRndr {
extern "C" {
#endif
int GlobalStringTableReleaseDynamicEntries();
#ifdef __cplusplus
}
#endif

typedef struct ActiveRegionRectPartial {
    int x;
    int y;
    int right;
    int bottom;
} ActiveRegionRectPartial;

/**
 * Authored zRndr fog-parameter record. BN types the four adjacent zeroed BSS
 * records as zRndr_FogParams in Color, Staged, Direct, Active order at
 * 0x631dd0, 0x631e70, 0x631f10, and 0x631fb0, and scalar fog spans read packedColor16,
 * packedColor16Dup, and packedColorRamp from the active record. BN shows the
 * 0x49e0e0 helper stores packedColor16 as a word and leaves the following word
 * as padding before packedColor16Dup.
 */
typedef struct FogParamsPartial {
    float colorRgb01[3];
    int packedColorRed;
    int packedColorGreen;
    int packedColorBlue;
    unsigned short packedColor16;
    unsigned short packedColor16Padding;
    int packedColor16Dup;
    int packedColorRamp[32];
} FogParamsPartial;

typedef struct SpanOccluderPolyPartial {
    float vertices[8][3];
    int vertCount;
} SpanOccluderPolyPartial;

typedef struct SpanNodePartial {
    struct SpanNodePartial* next;
    int sampleXMin;
    int sampleXMax;
    float invDepth;
    float invDepthStep;
    float depthSlope;
} SpanNodePartial;

/**
 * BN evidence: the switch-vshift span routines use gRndr_SavedEspSlot as the
 * stack-pivot scratch pointer while writing 16-bit spans backward.
 */
typedef struct zRndr_SpanEspPivotSave {
    int* savedEbp;
    int savedEdi;
    int savedEsi;
    int savedEbx;
} zRndr_SpanEspPivotSave;

RECOIL_STATIC_ASSERT(sizeof(zRndr_SpanEspPivotSave) == 0x10);

/**
 * BN models the MMX scratch globals as qword records; lo is the dword at the
 * symbol base used by MOVQ loads/stores.
 * BN names the MMX scratch globals as zMmxQword lo/hi records; these are
 * authored renderer data records, not provider-owned MMX intrinsic state.
 */
typedef struct zMmxQword {
    int lo;
    int hi;
} zMmxQword;

RECOIL_STATIC_ASSERT(sizeof(zMmxQword) == 0x08);

typedef struct LensFlareSamplePartial {
    float x;
    float y;
    float reciprocalZ;
    int packedColor16;
    int lensFlareSource;
} LensFlareSamplePartial;

typedef struct QueuedVec3 {
    float x;
    float y;
    float z;
} QueuedVec3;

typedef struct QueuedPolyClipOverlay {
    QueuedVec3 polyVertsPrefix[64];
    QueuedVec3 clippedTriVerts[3];
} QueuedPolyClipOverlay;

typedef struct TransparentQueuedPolyDrawCmd {
    zImage_TexDirEntryPartial* materialRef;
    int vertexCount;
    union {
        QueuedVec3 polyVerts[67];
        QueuedPolyClipOverlay clippedTriVertOverlay;
    };
    QueuedVec3 triVerts[3];
    float triUVs[6];
    int scanConvertMode;
    int hasClippedTriVerts;
    float savedInvDepthBias;
    float savedInvDepthScale;
    int alphaOrShadeBits;
    int shadeOrSpanMode;
    int texKey;
} TransparentQueuedPolyDrawCmd;

typedef struct OverwriteQueuedPolyDrawCmd {
    int commandTag;
    union {
        QueuedVec3 polyVerts[67];
        QueuedPolyClipOverlay clippedTriVertOverlay;
    };
    QueuedVec3 triVerts[3];
    float alphaOrShadeF;
    int shadeOrSpanMode;
    int vertexCount;
    float triUVs[6];
    zImage_TexDirEntryPartial* materialRef;
    float perVertexAlphaOrShadeF[65];
    int scanConvertMode;
    int hasClippedTriVerts;
    float savedInvDepthBias;
    float savedInvDepthScale;
    int texKey;
} OverwriteQueuedPolyDrawCmd;

typedef void(__fastcall* SpanBuildProc)(SpanNodePartial** spanList, int columnIndex, int* spanCount);
/**
 * Software overlay row callback selected by zRndrOverlayRectFlushSw. BN
 * ties this callback type to the shared premul/destination-scale globals and
 * the four 555/565 scalar/MMX row leaves from zRndr_Overlay.cpp.
 */
typedef void(__fastcall* OverlayBlendRowProc)(unsigned short* rowPixels16, int pixelCount);
typedef void(__fastcall* SpanRoutineProc)(int spanOpContext, int pixelCount);
typedef void(__fastcall* ImmediateRaster4Proc)(unsigned short* dstPixels, int x0, int y0, int x1, int y1, int color16);
typedef void(__fastcall* ImmediateRasterSegmentedProc)(
    unsigned short* dstPixels,
    int x0,
    int y0,
    int x1,
    int y1,
    int color16,
    int segmentCount
);
typedef void(__fastcall* ImmediateRaster5Proc)(
    unsigned short* dstPixels,
    const zRndr_LineClipRect2I* clipRect,
    int x0,
    int y0,
    int x1,
    int y1,
    int color16
);
typedef void(__fastcall* PointOpProc)(void* frameBuffer, int y, int x, int color16);
typedef void(__fastcall* FlatImmediateSpanProc)(int flatSpanOpEcxArg, int flatSpanOpEdxArg, int pixelCount);
typedef void(__fastcall* TexturedQueuedSpanProc)(int texU, int texV, int pixelCount, int texVShift);

#ifdef __cplusplus
extern void* g_frameBuffer;
extern int g_activeRegionWidth;
extern int g_activeRegionHeight;
extern ActiveRegionRectPartial g_activeRegionRect;
extern int g_pitchBytes;
extern int g_bytesPerPixel;
extern int g_videoStrideMirror0;
extern int g_videoStrideMirror1;
extern "C" {
#endif
extern int g_scanConvertMode;
extern int g_perspectiveTextureEnabled;
#ifdef __cplusplus
}
extern int g_perspectiveTextureDeltaXPow2;
extern int g_perspectiveTextureDeltaXBytes;
extern int g_perspectiveTextureDeltaXInput;
extern int g_perspectiveTextureDeltaXShift;
extern float g_perspectiveTextureDeltaXPow2F;
extern float g_perspectiveTextureFarZInv;
extern int g_perspectiveAdaptiveMinSpan;
extern int g_perspectiveAdaptiveMaxSpan;
extern float g_perspectiveAdaptiveSlope;
extern "C" {
#endif
extern float g_inverseDepthBias;
extern float g_inverseDepthScale;
#ifdef __cplusplus
}
extern float g_spanDepthBias;
extern float g_spanDepthBiasPlusOne;
extern float g_spanDepthBiasPlusOneInv;
extern FogParamsPartial g_fogColorParams;
extern FogParamsPartial g_fogTargetParamsStaged;
extern FogParamsPartial g_fogTargetParamsDirect;
extern FogParamsPartial g_fogParamsActive;
extern SpanOccluderPolyPartial g_spanOccluderPolys[8];
extern int g_spanOccluderPolyCount;
extern "C" {
#endif
extern SpanNodePartial* g_spanAllocCursor;
#ifdef __cplusplus
}
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
extern "C" {
#endif
extern int g_pixelPackGreenBits;
#ifdef __cplusplus
}
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
#endif
typedef struct {
    int transparentQueueCount;
    TransparentQueuedPolyDrawCmd transparentQueue[0x15e];
    int transparentQueueSortIndices[0x15e];
    int overwriteQueueCount;
    OverwriteQueuedPolyDrawCmd overwriteQueue[0x15e];
} QueuedPolyBanks;
#ifdef __cplusplus
extern QueuedPolyBanks g_queuedPolyBanks;
#endif
#define g_transparentQueueCount g_queuedPolyBanks.transparentQueueCount
#define g_transparentQueue g_queuedPolyBanks.transparentQueue
#define g_transparentQueueSortIndices g_queuedPolyBanks.transparentQueueSortIndices
#define g_overwriteQueueCount g_queuedPolyBanks.overwriteQueueCount
#define g_overwriteQueue g_queuedPolyBanks.overwriteQueue
#ifdef __cplusplus
extern int g_overlayBlendEnabled;
extern int g_overlayBlendRectLeft;
extern int g_overlayBlendRectTop;
extern int g_overlayBlendRectRight;
extern int g_overlayBlendRectBottom;
extern unsigned int g_overlayBlendPackedColor16;
extern double g_overlayBlendAlpha;
#endif
typedef struct LensFlareFrameBank {
    int sampleQueueCount;
    LensFlareSamplePartial sampleQueue[0x28a];
    int visibleSampleCount;
    zRndr_LensFlareVisibleSampleDef* visibleSampleDefs[0x40];
} LensFlareFrameBank;
#ifdef __cplusplus
extern LensFlareFrameBank g_lensFlareBank;
#endif
#define g_lensFlareSampleQueueCount g_lensFlareBank.sampleQueueCount
#define g_lensFlareSampleQueue g_lensFlareBank.sampleQueue
#define g_lensFlareVisibleSampleCount g_lensFlareBank.visibleSampleCount
#define g_lensFlareVisibleSampleDefs g_lensFlareBank.visibleSampleDefs
#ifdef __cplusplus
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
extern "C" {
#endif
void __stdcall SetInverseZTolerance(float inverseZTolerance);
#ifdef __cplusplus
}
void __fastcall SetPerspectiveTextureDeltaX(int deltaX);
void __stdcall SetPerspectiveTextureFarZ(float farZ);
void __stdcall SetPerspectiveAdaptiveCorrection(float perspectiveAdaptiveCorrection);
void __fastcall SetPerspectiveAdaptiveSpanParams(int minSpan, int maxSpan, float slope);
extern "C" void* __fastcall
GetActiveRegionState(int* outWidth, int* outHeight, int* outBitsPerPixel, int* outPitchBytes) throw();
void __fastcall
SetFrameBufferRegion(void* pixels, zOpt_ViewRectSection* activeRegionRect, int bitsPerPixel, int pitchBytes);
void __fastcall SetActiveRegionSizeFromRect(HudUiRect* rect);
void __fastcall SetVideoStrideMirrors(int stride);
extern "C" void __fastcall SpanOcclusionAddPolygon(const zVec3* vertices, int vertCount);
void __fastcall SpanOcclusionSubmitOccluderRect(const HudUiRect* rect, int halveIfReplicate, float z);
int __fastcall SpanOcclusionInit(int height);
extern "C" void __cdecl SpanOcclusionBuildColumnHeadTable();
void __fastcall SpanOcclusionRasterizeOccluderPoly(SpanOccluderPolyPartial* poly, int vertCount);
extern "C" void __cdecl SpanOcclusionResetFrame();
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
extern "C" {
#endif
void __fastcall FogColorSetRgb01Clamped(zColorRgb* color);
void __fastcall SetFogTargetColorRgb01Clamped(zColorRgb* color);
void __cdecl CommitDirectFogParamsIfChanged();
void __cdecl CommitFogColorParamsIfChanged();
void __cdecl CommitStagedFogParamsIfChanged();
void __fastcall BlendPackedColor565WithFogInPlace(int* ioPackedColor, int blend255);
#ifdef __cplusplus
}
void __cdecl LensFlareResetSampleQueue();
void __fastcall
LensFlareDrawQueuedSample16ClippedFramebuffer(LensFlareSamplePartial* sample, float screenScale, int yOffsetPixels);
void __fastcall LensFlareDrawQueuedSamplesScaled16ClippedFramebuffer(float screenScale, int yOffsetPixels);
} // namespace zRndr

void __fastcall
zRndr_SpanOcclusion_InsertSpanNode_Local(zRndr::SpanNodePartial** spanList, int columnIndex, int* spanCount);

void __fastcall
zRndrSpanOcclusionInsertSpanNodeNoDepthTest(zRndr::SpanNodePartial** spanList, int columnIndex, int* spanCount);

void __fastcall zRndrSpanOcclusionBuildSpanList(zRndr::SpanNodePartial** spanList, int columnIndex, int* spanCount);

void __fastcall zRndrSpanOcclusionBuildSpanListFast(zRndr::SpanNodePartial** spanList, int columnIndex, int* spanCount);

extern "C" {
#endif
void __fastcall zRndrSpanOcclusionTestColumnVisibility(int columnIndex, int* isVisible);
#ifdef __cplusplus
}

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

int __fastcall zRndrSpanOcclusionTestSpanDepthOrderPair(zRndr::SpanNodePartial* lhs, zRndr::SpanNodePartial* rhs);

void __fastcall zRndrRasterizePolyWithSpanList(zVec3* vertices, zVec3* planeVerts, int vertCount, int spanOpContext);

void __fastcall zRndrRasterizePoly(zVec3* vertices, int vertCount, int spanOpContext);

void __fastcall zRndrDrawFlatImmediate(
    zVec3* vertices,
    zVec3* planeVertices,
    int vertCount,
    int flatSpanOpEdxArg,
    int flatSpanOpEcxArg
);

extern "C" {
#endif
void __fastcall zRndrSubmitPolyWithSpanList(
    zVec3* entryVertices,
    zVec3* entryPlaneVertices,
    int spanOpContext,
    int alpha255,
    int vertCount,
    int queueOverwrite
);
#ifdef __cplusplus
}

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

extern "C" {
#endif
void __fastcall zRndrSubmitTexturedPolyUniformAlphaOrShade(
    zVec3* projectedPolyVerts,
    zVec3* clippedTriVerts,
    zVec3* triData9f,
    zVec2* triUVs,
    int vertexCount,
    zImage_TexDirEntryPartial* entry,
    float alphaOrShadeF,
    int queueOverwrite
);

void __fastcall zRndrSubmitTexturedPolyPerVertexAlphaOrShade(
    zVec3* projectedPolyVerts,
    zVec3* clippedTriVerts,
    zVec3* triData9f,
    zVec2* triUVs,
    float* perVertexAlphaOrShadeF,
    int shadeOrSpanMode,
    int vertexCount,
    zImage_TexDirEntryPartial* entry,
    int preservePaletteRemapKey,
    int queueOverwrite
);
#ifdef __cplusplus
}

extern "C" void __cdecl zRndrFlushTransparentQueue();
extern "C" void __cdecl zRndrFlushOverwriteQueue();
void __fastcall zRndrOverlayRectSubmit(unsigned short packedColor16, double alpha, zVidRect32* rectOrNull);
extern "C" void __fastcall zRndrOverlayRectFlushSw();

void __fastcall zRndrDrawImmediateLine(int x0, int y0, int x1, int y1, int color16);

void __fastcall zRndrDrawClippedImmediateLineStrip(
    const zRndr_LinePoint2I* points,
    int segmentCount,
    const void* clipRect,
    int color16
);

extern "C" {
#endif
void __fastcall
zRndrLensFlareQueueProjectedSample(zProjectedPoint* projectedPoint, int packedColor16, int lensFlareSource);
#ifdef __cplusplus
}

extern "C" int __cdecl zRndrLensFlareGetQueuedSampleCount();

extern "C" void __fastcall zRndrLensFlareDrawQueuedSamples16AndBuildVisibleList(int startIndex);

extern "C" int __fastcall zRndrLensFlareBuildVisibleSampleListFromQueue(int startIndex);

void __fastcall zRndrLensFlareSetVisibleSampleStage(int stageIndex, zImage_TexDirEntryPartial* stageTexDirEntry);

void __fastcall zRndrLensFlareDrawSampleStageClipped(
    const zVec2* sampleCenter,
    zImage_TexDirEntryPartial* stageTexDirEntry,
    float sampleRadius,
    const zRndr_LineClipRect2I* clipRect
);

void __fastcall
zRndrLensFlareDrawVisibleSampleStages(zRndr_LensFlareVisibleSampleDef* visibleSampleDef, float visibilityAlpha);

extern "C" void __fastcall zRndrLensFlareDrawVisibleSample(int sampleIndex);

extern "C" void __cdecl zRndrLensFlareDrawVisibleSamples();

extern "C" void __fastcall zRndrSpanOcclusionFilterSampleList(int visibleSampleIndex, zVec3* outPoint);

extern "C" {
#endif
void __fastcall zRndrFogTargetColorStagedSetRgb01Clamped(zColorRgb* color);

void __fastcall zRndrSetPaletteRemapKey(zVidPaletteRemapRecipe* recipe, float shadeLevel);

void __fastcall zRndrSetPaletteRemapKeyFromRgb01(zColorRgb* rgb01, float shadeLevel);

void __fastcall zRndrSetPaletteShadeRecipeIndex(zVidPaletteRemapRecipe* recipe);
#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
/* C units call zFloat::Set255f (zrndr_init.c) by its C name; C++ units see it in zmth_decls.h. */
void __fastcall Set255f(float* value);
#endif
