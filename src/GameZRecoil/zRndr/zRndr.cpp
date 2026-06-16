#include "Battlesport/Mfc42Abi.h"

#include "GameZRecoil/zRndr/zRndr.h"

#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zClass.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

namespace {
template <class T>
/**
 * Recovered helper: MinValue
 * Original-source helper evidence: No standalone plan entry was found; recovered from zRndr span, polygon, and scan-conversion callers in this source file.
 * Purpose: Return the smaller of two values without changing caller-owned storage.
 */
const T &MinValue(
    const T &lhs,
    const T &rhs
) {
    return lhs < rhs ? lhs : rhs;
}

template <class T>
/**
 * Recovered helper: MaxValue
 * Original-source helper evidence: No standalone plan entry was found; recovered from zRndr span, polygon, and scan-conversion callers in this source file.
 * Purpose: Return the larger of two values without changing caller-owned storage.
 */
const T &MaxValue(
    const T &lhs,
    const T &rhs
) {
    return lhs < rhs ? rhs : lhs;
}

/**
 * Recovered helper: sort.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr scan conversion callers in this source file.
 * Purpose: Sort scanline intersection samples in ascending order.
 */
void sort(
    float *first,
    float *last
) {
    for (float *it = first + 1; it < last; ++it) {
        float value = *it;
        float *scan = it;
        while (scan > first && value < scan[-1]) {
            *scan = scan[-1];
            --scan;
        }
        *scan = value;
    }
}
} // namespace

extern "C" {
zVideo_BltSourceToPrimaryProc g_zVideo_pfnBltSourceToPrimary = 0;
}

namespace zSys {
int CheckCpuSignatureMask();
}

float g_zRndr_InverseZTolerance = 0.0f;
// Palette-remap selection globals: BN identifies the active packed remap key at
// 0x4e21fc and active shade recipe index at 0x4e2200; both retail values start
// disabled at -1 and are written by the zRndr palette setter cluster.
int g_zRndr_ActivePaletteRemapKey = -1;
int g_zRndr_ActivePaletteShadeRecipeIndex = -1;
int g_zRndr_CircleCenterX = 0;
int g_zRndr_CircleCenterY = 0;
int g_zRndr_CircleDrawAuxArg = 0;

namespace zRndr {
// Default software render target bank from zRndr_Draw.cpp. BN names the
// clipped-framebuffer globals at 0x632050, 0x632054, 0x632058, and 0x63205c;
// lens-flare and span leaves consume them as the active 16-bit framebuffer.
void *g_frameBuffer = 0;
int g_activeRegionWidth = 0;
int g_activeRegionHeight = 0;
ActiveRegionRectPartial g_activeRegionRect = {0};
int g_pitchBytes = 0;
int g_bytesPerPixel = 0;
int g_videoStrideMirror0 = 0;
int g_videoStrideMirror1 = 0;
int g_scanConvertMode = 0;
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
float g_inverseDepthBias = 0.0f;
float g_inverseDepthScale = 0.0f;
float g_spanDepthBias = 0.0f;
float g_spanDepthBiasPlusOne = 0.0f;
float g_spanDepthBiasPlusOneInv = 0.0f;
FogParamsPartial g_fogColorParams = {0};
FogParamsPartial g_fogTargetParamsDirect = {0};
FogParamsPartial g_fogTargetParamsStaged = {0};
FogParamsPartial g_fogParamsActive = {0};
// zRndr span-occlusion subsystem state from zRndr_Draw.cpp. BN names these as
// gRndr_Span* globals; g_spanIterPrevLink stores the previous node observed in
// insertion walkers even though one BN data declaration renders it as a link
// pointer.
SpanOccluderPolyPartial g_spanOccluderPolys[8] = {0};
int g_spanOccluderPolyCount = 0;
SpanNodePartial *g_spanAllocCursor = 0;
SpanNodePartial **g_spanColumnHeadTable = 0;
SpanNodePartial *g_spanPoolBase = 0;
SpanNodePartial *g_spanLastNode = 0;
SpanNodePartial *g_spanIterNode = 0;
SpanNodePartial *g_spanIterPrevLink = 0;
int g_spanReservedWriteOnly = 0;
int g_spanColumnCount = 0;
int g_spanColumnCountPadded = 0;
SpanBuildProc g_pfnBuildSpanList = 0;
SpanBuildProc g_pfnBuildSpanListSecondary = 0;
// Overlay row callback and row-blend factors from zRndr_Overlay.cpp. The
// software flush path rewrites the callback for 555/565 and scalar/MMX rows,
// then row leaves consume the premultiplied packed color and destination scale.
OverlayBlendRowProc g_pfnOverlayBlendRow = 0;
unsigned int g_swOverlayPremulPacked = 0;
unsigned int g_swOverlayPremulPackedRot16 = 0;
int g_swOverlayDstScale5 = 0;
unsigned int g_swOverlayPremulRPair = 0;
unsigned int g_swOverlayPremulBPair = 0;
unsigned int g_swOverlayPremulGPair = 0;
int g_pixelPackRedBits = 0;
int g_pixelPackGreenBits = 0;
int g_pixelPackBlueBits = 0;
// Pixel-pack masks consumed by fog and span color math: red 0x57de40,
// green 0x57de44, and blue 0x57de48.
unsigned int g_pixelPackRedMask = 0;
unsigned int g_pixelPackGreenMask = 0;
unsigned int g_pixelPackBlueMask = 0;
int g_pixelPackRedShift = 0;
int g_pixelPackGreenShift = 0;
int g_pixelPackBlueShift = 0;
// BN keeps queued texture alpha setup in this initialized slot at 0x4e21ec as
// pointer value 0x00000007; the queued and fan-triangle paths overwrite it from
// zVidImagePartial::queuedAlphaMap before queued-alpha use.
enum {
    kQueuedTexAlphaMapStartupSentinel = 7
};
char *g_spanQueuedTexAlphaMap = (char *)(kQueuedTexAlphaMapStartupSentinel);
int g_spanActiveTexShift = 7;
int g_spanActiveTexVMask = 0x07f00000;
int g_spanActiveTexUMask = 0x7f;
// BN names this BSS slot gRndr_ActiveTexPixels. Word span loops load it as
// 16-bit texture pixels, while palettized span loops use the same buffer as
// 8-bit texture indices.
unsigned char *g_spanActiveTexPixels = 0;
unsigned short *g_spanActiveTexPalette = 0;
int g_spanActiveTexUStepFixed20 = 0;
int g_spanActiveTexVStepFixed20 = 0;
// BN names this BSS pointer gRndr_CurrentSpanBaseAddr. Most span leaves use it
// as an ordinary unsigned-short destination cursor; the opaque/switch-vshift
// push-write leaves are separate ESP-pivot source-shape debt.
unsigned short *g_spanCurrentSpanBaseAddr = 0;
int g_spanActiveShadeFixed16 = 0;
int g_spanActiveShadeStepFixed16 = 0;
// BN names 0x56b27c gRndr_ActiveTexAlphaMap. Alpha-map span leaves including
// 0x49c360, 0x49c560, 0x49d1a0, and 0x49d3b0 sample it in lockstep with
// gRndr_ActiveTexPixels using the same U/V masks and fixed-point steps.
char *g_spanActiveTexAlphaMap = 0;
// BN types the zeroed span/MMX scratch vectors from gRndr_Mmx_dUDup2 through
// gRndr_MmxMask_BlueBits as zMmxQword records. Source keeps lo/hi pairs as
// zMmxQword and lane-indexed mask/factor vectors as four 16-bit lanes, which
// preserves the same eight-byte authored zRndr span data shape.
zMmxQword g_mmxUStepDup2 = {0};
// BN names this BSS pointer gRndr_SavedEspSlot. The original switch-vshift
// span loops save the real ESP here before pivoting to the destination span.
zRndr_SpanEspPivotSave *g_spanSavedEspSlot = 0;
zMmxQword g_mmxUMask = {0};
int g_spanActiveConstAlphaBits = 0;
zMmxQword g_mmxVMask = {0};
zMmxQword g_mmxVStepDup2 = {0};
zMmxQword g_mmxUPair = {0};
zMmxQword g_mmxVShiftCounts = {0};
zMmxQword g_mmxVPair = {0};
unsigned short g_mmxBitsBlue255[4] = {0};
unsigned short g_mmxBitsGreen255[4] = {0};
unsigned short g_mmxBitsRed255[4] = {0};
short g_mmxMaskGreenPacked[4] = {0};
unsigned short g_mmxMaskRedPacked[4] = {0};
unsigned short g_mmxFogFactors[4] = {0};
unsigned short g_mmxMaskGreenBits[4] = {0};
unsigned short g_mmxMaskBlueBits[4] = {0};
// BN exposes adjacent zero BSS dwords data_57dab8/data_57dabc immediately
// after the blue mask qword. Same-session BN xrefs show no code or data users,
// so source leaves them as an unmodeled BSS gap instead of authored span/MMX
// state.
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
PointOpProc g_pfnPointOpCandidate = 0;
PointOpProc g_pfnPointOpActive = 0;
SpanRoutineProc g_pfnTexturedQueuedFinalize = 0;
SpanRoutineProc g_pfnTexturedQueuedFinalizeAlt = 0;
// Queued polygon banks from zrndr_draw.c. BN identifies the transparent count
// at 0x57de7c, the transparent records at 0x57de80, the sort index bank at
// 0x5cacf8, the overwrite count at 0x5cb270, and overwrite records at
// 0x5cb274.
TransparentQueuedPolyDrawCmd g_transparentQueue[0x15e] = {0};
OverwriteQueuedPolyDrawCmd g_overwriteQueue[0x15e] = {0};
int g_transparentQueueSortIndices[0x15e] = {0};
int g_transparentQueueCount = 0;
int g_overwriteQueueCount = 0;
// Overlay blend state shared by zRndr_Overlay.cpp setup and the lens-flare
// clipped-framebuffer leaf: enable flag 0x62e9dc, packed color 0x62e9f0, and
// alpha double 0x62e9f8.
int g_overlayBlendEnabled = 0;
int g_overlayBlendRectLeft = 0;
int g_overlayBlendRectTop = 0;
int g_overlayBlendRectRight = 0;
int g_overlayBlendRectBottom = 0;
unsigned int g_overlayBlendPackedColor16 = 0;
double g_overlayBlendAlpha = 0.0;
// zRndr lens-flare frame-state bank. BN identifies the zero-initialized queue
// count at 0x62ea00, the 0x28a-entry sample queue at 0x62ea04, the visible
// count at 0x631ccc, the 64-entry visible pointer list at 0x631cd0, the
// visibility-active flag at 0x56b248, and four stage texture pointers at
// 0x56b250.
int g_lensFlareSampleQueueCount = 0;
LensFlareSamplePartial g_lensFlareSampleQueue[0x28a] = {0};
int g_lensFlareVisibleSampleCount = 0;
int g_lensFlareVisibilityActive = 0;
zImage_TexDirEntryPartial *g_lensFlareVisibleSampleStages[4] = {0};
zRndr_LensFlareVisibleSampleDef *g_lensFlareVisibleSampleDefs[0x40] = {0};
int g_textureMipSelectionEnabled = 0;
int g_textureMipReservedWriteOnly = 0;
int g_renderStateReadyWriteOnlyFlag = 0;
int g_renderStateReservedWriteOnly = 0;
int g_initField00 = 0;
int g_initField04 = 0;
int g_initField08 = 0;
int g_initField0C = 0;
int g_initField10 = 0;
int g_initField14 = 0;
int g_defaultGraphicsFlags = 0;
int *g_graphicsFlags = 0;

RECOIL_STATIC_ASSERT(sizeof(FogParamsPartial) == 0xa0);
RECOIL_STATIC_ASSERT(
    offsetof(
        FogParamsPartial,
        packedColorRed
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        FogParamsPartial,
        packedColorGreen
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        FogParamsPartial,
        packedColorBlue
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        FogParamsPartial,
        packedColor16
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        FogParamsPartial,
        packedColor16Padding
    ) == 0x1a
);
RECOIL_STATIC_ASSERT(
    offsetof(
        FogParamsPartial,
        packedColor16Dup
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        FogParamsPartial,
        packedColorRamp
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(SpanOccluderPolyPartial) == 0x64);
RECOIL_STATIC_ASSERT(sizeof(SpanNodePartial) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(LensFlareSamplePartial) == 0x14);
RECOIL_STATIC_ASSERT(
    offsetof(
        LensFlareSamplePartial,
        x
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        LensFlareSamplePartial,
        y
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        LensFlareSamplePartial,
        reciprocalZ
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        LensFlareSamplePartial,
        packedColor16
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        LensFlareSamplePartial,
        lensFlareSource
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zRndr_LensFlareSource,
        lensFlareEnabled
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zRndr_LensFlareSource,
        fadeNear
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zRndr_LensFlareSource,
        fadeFar
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zRndr_LensFlareVisibleSampleDef,
        depthDivisor
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zRndr_LensFlareVisibleSampleDef,
        lensFlareSource
    ) == 0x10
);
RECOIL_STATIC_ASSERT(sizeof(QueuedVec3) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(QueuedPolyClipOverlay) == 0x324);
RECOIL_STATIC_ASSERT(
    offsetof(
        QueuedPolyClipOverlay,
        clippedTriVerts
    ) == 0x300
);
RECOIL_STATIC_ASSERT(sizeof(TransparentQueuedPolyDrawCmd) == 0x384);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        materialRef
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        vertexCount
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        polyVerts
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        triVerts
    ) == 0x32c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        clippedTriVertOverlay
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        triUVs
    ) == 0x350
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        scanConvertMode
    ) == 0x368
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        hasClippedTriVerts
    ) == 0x36c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        savedInvDepthBias
    ) == 0x370
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        savedInvDepthScale
    ) == 0x374
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        alphaOrShadeBits
    ) == 0x378
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        shadeOrSpanMode
    ) == 0x37c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        TransparentQueuedPolyDrawCmd,
        texKey
    ) == 0x380
);
RECOIL_STATIC_ASSERT(sizeof(OverwriteQueuedPolyDrawCmd) == 0x48c);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        commandTag
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        polyVerts
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        clippedTriVertOverlay
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        triVerts
    ) == 0x328
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        alphaOrShadeF
    ) == 0x34c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        shadeOrSpanMode
    ) == 0x350
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        vertexCount
    ) == 0x354
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        triUVs
    ) == 0x358
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        materialRef
    ) == 0x370
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        perVertexAlphaOrShadeF
    ) == 0x374
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        scanConvertMode
    ) == 0x478
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        hasClippedTriVerts
    ) == 0x47c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        savedInvDepthBias
    ) == 0x480
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        savedInvDepthScale
    ) == 0x484
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OverwriteQueuedPolyDrawCmd,
        texKey
    ) == 0x488
);
RECOIL_STATIC_ASSERT(
    offsetof(
        LensFlareSamplePartial,
        packedColor16
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        LensFlareSamplePartial,
        lensFlareSource
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        SpanNodePartial,
        next
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        SpanNodePartial,
        sampleXMin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        SpanNodePartial,
        sampleXMax
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        SpanNodePartial,
        invDepth
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        SpanNodePartial,
        invDepthStep
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        SpanNodePartial,
        depthSlope
    ) == 0x14
);

namespace {
struct ScanVertex {
    float x;
    float y;
};

struct ScanConvertEdge {
    int xStepFixed;
    int yStart;
    int currentXFixed;
    int reserved;
};

union SpanOcclusionRasterScratch {
    zVec3 reducedVerts[8];
    SpanNodePartial *spanList[0x141];
};

RECOIL_STATIC_ASSERT(sizeof(SpanOcclusionRasterScratch) == 0x504);

/* BN 0x4927d0 uses the original VC5 double-bias fixed-point conversion inline. */
#define ZRNDR_SET_FIXED16_FROM_FLOAT(dst, value)                         \
    do {                                                                 \
        double zRndrFixed16Bits =                                         \
            6755399441055744.0 - (double)((value) * -65536.0f);          \
        (dst) = *(int *)(&zRndrFixed16Bits);                              \
    } while (0)

/**
 * Recovered helper: Fixed16FromFloat.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span and scan-conversion callers that round coordinates into 16.16 fixed point.
 * Purpose: Convert a floating-point value to signed 16.16 fixed-point with symmetric rounding.
 */
int Fixed16FromFloat(
    float value
) {
    const double scaled = (double)(value) * 65536.0;
    return (int)(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

/**
 * Recovered helper: ScanlineStartFromY.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span and polygon raster callers that bias the starting scanline from fixed-point Y.
 * Purpose: Compute the first covered scanline for a polygon edge Y coordinate.
 */
int ScanlineStartFromY(
    float y
) {
    return (Fixed16FromFloat(y) + 0x7fff) >> 16;
}

/**
 * Recovered helper: ScanlineEndFromY.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span and polygon raster callers that bias the ending scanline from fixed-point Y.
 * Purpose: Compute the last covered scanline for a polygon edge Y coordinate.
 */
int ScanlineEndFromY(
    float y
) {
    return (Fixed16FromFloat(y) - 0x8041) >> 16;
}

/**
 * Recovered helper: SpanStartFromX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span builders that bias the starting pixel from fixed-point X.
 * Purpose: Compute the first covered span sample for an edge X coordinate.
 */
int SpanStartFromX(
    float x
) {
    return (Fixed16FromFloat(x) + 0x7fff) >> 16;
}

/**
 * Recovered helper: SpanEndFromX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span builders that bias the ending pixel from fixed-point X.
 * Purpose: Compute the last covered span sample for an edge X coordinate.
 */
int SpanEndFromX(
    float x
) {
    return (Fixed16FromFloat(x) - 0x8001) >> 16;
}

/**
 * Recovered helper: AppendSpanListNode.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion builders that coalesce adjacent visible span nodes.
 * Purpose: Append a visible span node and merge it with the previous node when contiguous.
 */
void AppendSpanListNode(
    SpanNodePartial **spanList,
    int *spanCount,
    SpanNodePartial *node
) {
    if (*spanCount > 0) {
        SpanNodePartial *previous = spanList[*spanCount - 1];
        if (node->sampleXMin == previous->sampleXMax + 1) {
            previous->sampleXMax = node->sampleXMax;
            previous->invDepthStep = node->invDepthStep;
            previous->next = node->next;
            g_spanLastNode = previous;
            g_spanIterNode = previous;
            return;
        }
    }

    spanList[*spanCount] = node;
    ++*spanCount;
    g_spanLastNode = node;
}

/**
 * Recovered helper: SpanDepthAtX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion comparisons using SpanNodePartial depth fields.
 * Purpose: Evaluate a span node's inverse depth at one sample X coordinate.
 */
float SpanDepthAtX(
    const SpanNodePartial *span,
    int x
) {
    if (x == span->sampleXMin) {
        return span->invDepth;
    }
    if (x == span->sampleXMax) {
        return span->invDepthStep;
    }

    return span->invDepth + (float)(x - span->sampleXMin) * span->depthSlope;
}

/**
 * Recovered helper: SpanDepthAtX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span split paths using explicit span depth parts.
 * Purpose: Evaluate inverse depth from a span start, base depth, and depth slope.
 */
float SpanDepthAtX(
    int sampleXMin,
    float invDepth,
    float depthSlope,
    int x
) {
    return invDepth + (float)(x - sampleXMin) * depthSlope;
}

/**
 * Recovered helper: SpanDepthAtXByParts.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span split paths using explicit span depth parts.
 * Purpose: Evaluate inverse depth from a span start, base depth, and depth slope.
 */
float SpanDepthAtXByParts(
    int sampleXMin,
    float invDepth,
    float depthSlope,
    int x
) {
    return invDepth + (float)(x - sampleXMin) * depthSlope;
}

/**
 * Recovered helper: LinkSpanNode.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion insertion paths that update the column head/link fields.
 * Purpose: Link a pending span node into one column and refresh the span iterator globals.
 */
void LinkSpanNode(
    int columnIndex,
    SpanNodePartial *previous,
    SpanNodePartial *node,
    SpanNodePartial *next
) {
    node->next = next;
    if (previous != 0) {
        previous->next = node;
    } else {
        g_spanColumnHeadTable[columnIndex] = node;
    }

    g_spanIterPrevLink = previous;
    g_spanIterNode = node;
}

/**
 * Recovered helper: InsertPendingSpanSorted.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion callers that merge a pending span into sorted column coverage.
 * Purpose: Insert the pending span into one column while coalescing neighboring coverage.
 */
void InsertPendingSpanSorted(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0 ||
        columnIndex >= g_spanColumnCountPadded) {
        return;
    }

    SpanNodePartial *pending = g_spanAllocCursor;
    pending->next = 0;

    SpanNodePartial *previous = 0;
    SpanNodePartial *current = g_spanColumnHeadTable[columnIndex];
    while (current != 0 && current->sampleXMax + 1 < pending->sampleXMin) {
        previous = current;
        current = current->next;
    }

    while (current != 0 && current->sampleXMin <= pending->sampleXMax + 1) {
        pending->sampleXMin = MinValue(
            pending->sampleXMin,
            current->sampleXMin
        );
        pending->sampleXMax = MaxValue(
            pending->sampleXMax,
            current->sampleXMax
        );
        pending->invDepth = MaxValue(
            pending->invDepth,
            current->invDepth
        );
        pending->invDepthStep = MaxValue(
            pending->invDepthStep,
            current->invDepthStep
        );
        current = current->next;
    }

    pending->next = current;
    if (previous != 0) {
        previous->next = pending;
    } else {
        g_spanColumnHeadTable[columnIndex] = pending;
    }

    g_spanIterPrevLink = previous;
    g_spanIterNode = pending;
    AppendSpanListNode(
        spanList,
        spanCount,
        pending
    );
    ++g_spanAllocCursor;
}

/**
 * Recovered helper: InsertPendingSpanNoDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion callers that clip existing column spans without a depth comparison.
 * Purpose: Insert the pending span into one column while removing or splitting overlapped spans.
 */
void InsertPendingSpanNoDepthTest(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0 ||
        columnIndex >= g_spanColumnCountPadded) {
        return;
    }

    SpanNodePartial *pending = g_spanAllocCursor;
    SpanNodePartial *previous = 0;
    SpanNodePartial *current = g_spanColumnHeadTable[columnIndex];

    while (current != 0 && pending->sampleXMin > current->sampleXMax) {
        previous = current;
        current = current->next;
    }

    if (current == 0 || pending->sampleXMax < current->sampleXMin) {
        LinkSpanNode(
            columnIndex,
            previous,
            pending,
            current
        );
        AppendSpanListNode(
            spanList,
            spanCount,
            pending
        );
        ++g_spanAllocCursor;
        return;
    }

    while (current != 0 && current->sampleXMin <= pending->sampleXMax) {
        const int currentMin = current->sampleXMin;
        const int currentMax = current->sampleXMax;
        const float currentInvDepth = current->invDepth;
        const float currentInvDepthStep = current->invDepthStep;
        const float currentDepthSlope = current->depthSlope;

        if (currentMin < pending->sampleXMin) {
            if (currentMax >= pending->sampleXMin) {
                current->sampleXMax = pending->sampleXMin - 1;
                current->invDepthStep = SpanDepthAtX(
                    currentMin,
                    currentInvDepth,
                    currentDepthSlope,
                    current->sampleXMax
                );

                if (currentMax > pending->sampleXMax) {
                    SpanNodePartial *rightSplit = pending + 1;
                    rightSplit->sampleXMin = pending->sampleXMax + 1;
                    rightSplit->sampleXMax = currentMax;
                    rightSplit->invDepth = SpanDepthAtX(
                        currentMin,
                        currentInvDepth,
                        currentDepthSlope,
                        rightSplit->sampleXMin
                    );
                    rightSplit->invDepthStep = currentInvDepthStep;
                    rightSplit->depthSlope = currentDepthSlope;
                    rightSplit->next = current->next;

                    pending->next = rightSplit;
                    current->next = pending;
                    AppendSpanListNode(
                        spanList,
                        spanCount,
                        pending
                    );
                    g_spanIterPrevLink = current;
                    g_spanIterNode = pending;
                    g_spanLastNode = rightSplit;
                    g_spanAllocCursor += 2;
                    return;
                }
            }

            previous = current;
            current = current->next;
            continue;
        }

        if (currentMax <= pending->sampleXMax) {
            SpanNodePartial *next = current->next;
            if (previous != 0) {
                previous->next = next;
            } else {
                g_spanColumnHeadTable[columnIndex] = next;
            }
            current = next;
            continue;
        }

        current->sampleXMin = pending->sampleXMax + 1;
        current->invDepth =
            SpanDepthAtX(
                currentMin,
                currentInvDepth,
                currentDepthSlope,
                current->sampleXMin
            );
        break;
    }

    LinkSpanNode(
        columnIndex,
        previous,
        pending,
        current
    );
    AppendSpanListNode(
        spanList,
        spanCount,
        pending
    );
    ++g_spanAllocCursor;
}

/**
 * Recovered helper: BuildVisibleSpanListWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion visibility builders that compare pending and stored span depth.
 * Purpose: Build visible span fragments for a pending span without mutating the occluder list.
 */
void BuildVisibleSpanListWithDepthTest(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0 ||
        columnIndex >= g_spanColumnCountPadded) {
        return;
    }

    SpanNodePartial *pending = g_spanAllocCursor;
    pending->next = 0;
    SpanNodePartial *current = g_spanColumnHeadTable[columnIndex];

    while (current != 0 && pending->sampleXMin > current->sampleXMax) {
        current = current->next;
    }

    while (current != 0) {
        if (pending->sampleXMax < current->sampleXMin) {
            break;
        }

        SpanNodePartial occluder = *current;
        const bool pendingInFront =
            zRndr_SpanOcclusion_TestSpanDepthOrderPair(
                pending,
                &occluder
            ) != 0;

        const int pendingMin = pending->sampleXMin;
        const int pendingMax = pending->sampleXMax;
        const float pendingInvDepth = pending->invDepth;
        const float pendingInvDepthStep = pending->invDepthStep;
        const float pendingDepthSlope = pending->depthSlope;

        if (pendingInFront) {
            if (occluder.sampleXMax < pendingMax && occluder.sampleXMax >= pendingMin) {
                const int splitMax = occluder.sampleXMax;
                pending->sampleXMax = splitMax;
                pending->invDepthStep =
                    SpanDepthAtX(
                        pendingMin,
                        pendingInvDepth,
                        pendingDepthSlope,
                        splitMax
                    );
                AppendSpanListNode(
                    spanList,
                    spanCount,
                    pending
                );
                ++g_spanAllocCursor;

                pending = g_spanAllocCursor;
                pending->next = 0;
                pending->sampleXMin = splitMax + 1;
                pending->sampleXMax = pendingMax;
                pending->invDepth = SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    pending->sampleXMin
                );
                pending->invDepthStep = pendingInvDepthStep;
                pending->depthSlope = pendingDepthSlope;
                current = current->next;
                continue;
            }

            if (occluder.sampleXMax >= pendingMax) {
                AppendSpanListNode(
                    spanList,
                    spanCount,
                    pending
                );
                ++g_spanAllocCursor;
                return;
            }

            current = current->next;
            continue;
        }

        if (occluder.sampleXMin <= pendingMin) {
            if (occluder.sampleXMax >= pendingMax) {
                return;
            }

            if (occluder.sampleXMax >= pendingMin) {
                pending->sampleXMin = occluder.sampleXMax + 1;
                pending->invDepth = SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    pending->sampleXMin
                );
            }

            current = current->next;
            continue;
        }

        if (occluder.sampleXMin <= pendingMax) {
            const int leftMax = occluder.sampleXMin - 1;
            pending->sampleXMax = leftMax;
            pending->invDepthStep =
                SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    leftMax
                );
            AppendSpanListNode(
                spanList,
                spanCount,
                pending
            );
            ++g_spanAllocCursor;

            if (occluder.sampleXMax >= pendingMax) {
                return;
            }

            pending = g_spanAllocCursor;
            pending->next = 0;
            pending->sampleXMin = occluder.sampleXMax + 1;
            pending->sampleXMax = pendingMax;
            pending->invDepth =
                SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    pending->sampleXMin
                );
            pending->invDepthStep = pendingInvDepthStep;
            pending->depthSlope = pendingDepthSlope;
        }

        current = current->next;
    }

    AppendSpanListNode(
        spanList,
        spanCount,
        pending
    );
    ++g_spanAllocCursor;
}

/**
 * Recovered helper: InsertPendingSpanWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion insertion callers that compare pending and stored span depth.
 * Purpose: Insert the pending span into one column while splitting spans by depth order.
 */
void InsertPendingSpanWithDepthTest(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0 ||
        columnIndex >= g_spanColumnCountPadded) {
        return;
    }

    SpanNodePartial *pending = g_spanAllocCursor;
    pending->next = 0;
    SpanNodePartial *previous = 0;
    SpanNodePartial *current = g_spanColumnHeadTable[columnIndex];

    while (current != 0 && pending->sampleXMin > current->sampleXMax) {
        previous = current;
        current = current->next;
    }

    while (current != 0) {
        if (pending->sampleXMax < current->sampleXMin) {
            break;
        }

        const bool pendingInFront =
            zRndr_SpanOcclusion_TestSpanDepthOrderPair(
                pending,
                current
            ) != 0;

        const int pendingMin = pending->sampleXMin;
        const int pendingMax = pending->sampleXMax;
        const float pendingInvDepth = pending->invDepth;
        const float pendingInvDepthStep = pending->invDepthStep;
        const float pendingDepthSlope = pending->depthSlope;

        if (pendingInFront) {
            const int currentMin = current->sampleXMin;
            const int currentMax = current->sampleXMax;
            const float currentInvDepth = current->invDepth;
            const float currentInvDepthStep = current->invDepthStep;
            const float currentDepthSlope = current->depthSlope;

            if (currentMin < pendingMin) {
                current->sampleXMax = pendingMin - 1;
                current->invDepthStep = SpanDepthAtX(
                    currentMin,
                    currentInvDepth,
                    currentDepthSlope,
                    current->sampleXMax
                );

                if (currentMax > pendingMax) {
                    SpanNodePartial *rightSplit = pending + 1;
                    rightSplit->sampleXMin = pendingMax + 1;
                    rightSplit->sampleXMax = currentMax;
                    rightSplit->invDepth = SpanDepthAtX(
                        currentMin,
                        currentInvDepth,
                        currentDepthSlope,
                        rightSplit->sampleXMin
                    );
                    rightSplit->invDepthStep = currentInvDepthStep;
                    rightSplit->depthSlope = currentDepthSlope;
                    rightSplit->next = current->next;

                    pending->next = rightSplit;
                    current->next = pending;
                    AppendSpanListNode(
                        spanList,
                        spanCount,
                        pending
                    );
                    g_spanIterPrevLink = current;
                    g_spanIterNode = pending;
                    g_spanLastNode = rightSplit;
                    g_spanAllocCursor += 2;
                    return;
                }

                previous = current;
                current = current->next;
                continue;
            }

            if (currentMax <= pendingMax) {
                SpanNodePartial *next = current->next;
                if (previous != 0) {
                    previous->next = next;
                } else {
                    g_spanColumnHeadTable[columnIndex] = next;
                }
                current = next;
                continue;
            }

            current->sampleXMin = pendingMax + 1;
            current->invDepth =
                SpanDepthAtX(
                    currentMin,
                    currentInvDepth,
                    currentDepthSlope,
                    current->sampleXMin
                );
            break;
        }

        if (current->sampleXMin <= pendingMin) {
            if (current->sampleXMax >= pendingMax) {
                return;
            }

            if (current->sampleXMax >= pendingMin) {
                pending->sampleXMin = current->sampleXMax + 1;
                pending->invDepth = SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    pending->sampleXMin
                );
            }

            previous = current;
            current = current->next;
            continue;
        }

        if (current->sampleXMin <= pendingMax) {
            const int leftMax = current->sampleXMin - 1;
            pending->sampleXMax = leftMax;
            pending->invDepthStep =
                SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    leftMax
                );
            LinkSpanNode(
                columnIndex,
                previous,
                pending,
                current
            );
            AppendSpanListNode(
                spanList,
                spanCount,
                pending
            );
            ++g_spanAllocCursor;

            if (current->sampleXMax >= pendingMax) {
                return;
            }

            previous = current;
            pending = g_spanAllocCursor;
            pending->next = 0;
            pending->sampleXMin = current->sampleXMax + 1;
            pending->sampleXMax = pendingMax;
            pending->invDepth =
                SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    pending->sampleXMin
                );
            pending->invDepthStep = pendingInvDepthStep;
            pending->depthSlope = pendingDepthSlope;
            current = current->next;
            continue;
        }

        previous = current;
        current = current->next;
    }

    LinkSpanNode(
        columnIndex,
        previous,
        pending,
        current
    );
    AppendSpanListNode(
        spanList,
        spanCount,
        pending
    );
    ++g_spanAllocCursor;
}

/**
 * Recovered inline helper: zRndr active fog comparison
 * Original-source helper evidence: No standalone retail function is expected;
 * observed in 0x49b4c0, 0x49b530, and 0x49b710 as the thresholded
 * fog-parameter comparison before committing to the active record.
 * Purpose: Detect whether the pending fog color differs enough from the active renderer fog state to commit.
 */
static inline bool FogParamsDifferFromActive(
    const FogParamsPartial &params
) {
    const float kCommitThreshold = 0.01f;
    return fabs(g_fogParamsActive.colorRgb01[0] - params.colorRgb01[0]) >= kCommitThreshold ||
           fabs(g_fogParamsActive.colorRgb01[1] - params.colorRgb01[1]) >= kCommitThreshold ||
           fabs(g_fogParamsActive.colorRgb01[2] - params.colorRgb01[2]) >= kCommitThreshold;
}

/**
 * Recovered inline helper: zRndr active fog commit
 * Original-source helper evidence: No standalone retail function is expected;
 * observed in 0x49b4c0, 0x49b530, and 0x49b710 as the shared comparison plus
 * 0xa0-byte copy into the active fog-parameter record.
 * Purpose: Copy pending fog parameters into active renderer state only when the thresholded color comparison changes.
 */
static inline void CommitFogParamsIfChanged(
    const FogParamsPartial &params
) {
    if (FogParamsDifferFromActive(params)) {
        memcpy(
            &g_fogParamsActive,
            &params,
            sizeof(params)
        );
    }
}
} // namespace

/**
 * Reimplements 0x490430: zRndr::SetPerspectiveTextureDeltaX
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Cache the perspective texture span chunk size and byte stride derived from delta X.
 */
void __fastcall SetPerspectiveTextureDeltaX(
    int deltaX
) {
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

/**
 * Reimplements 0x4904a0: zRndr::SetPerspectiveTextureFarZ
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Cache the reciprocal far-Z value used by perspective texture correction.
 */
void __stdcall SetPerspectiveTextureFarZ(
    float farZ
) {
    if (farZ != 0.0) {
        g_perspectiveTextureFarZInv = 1.0f / farZ;
    }
}

/**
 * Reimplements 0x4904d0: zRndr::SetPerspectiveAdaptiveCorrection
 * Purpose: Cache adaptive perspective depth-bias terms used by textured span subdivision.
 */
void __stdcall SetPerspectiveAdaptiveCorrection(
    float perspectiveAdaptiveCorrection
) {
    const float plusOne = perspectiveAdaptiveCorrection + 1.0f;
    g_spanDepthBias = perspectiveAdaptiveCorrection;
    g_spanDepthBiasPlusOne = plusOne;
    if (plusOne == 0.0f) {
        g_spanDepthBiasPlusOneInv = 0.0f;
    } else {
        g_spanDepthBiasPlusOneInv = 1.0f / plusOne;
    }
}

/**
 * Reimplements 0x490480: zRndr::SetPerspectiveAdaptiveSpanParams
 * Purpose: Store the adaptive perspective span-size thresholds selected for the renderer.
 */
void __fastcall SetPerspectiveAdaptiveSpanParams(
    int minSpan,
    int maxSpan,
    float slope
) {
    g_perspectiveAdaptiveMinSpan = minSpan;
    g_perspectiveAdaptiveMaxSpan = maxSpan;
    g_perspectiveAdaptiveSlope = slope;
}

/**
 * Reimplements 0x48fd80: zRndr::InitGlobals
 * Purpose: Initialize renderer span, queue, fog, and dispatch globals to their startup state.
 */
int InitGlobals() {
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
    FogColor_SetRgb01Clamped(&color);
    FogColor_SetRgb01Clamped((zColorRgb *)(g_fogColorParams.colorRgb01));
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
    zOptionEntryPartial *option =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    g_graphicsFlags = option != 0 ? &option->payloadOrBuffer : &g_defaultGraphicsFlags;
    g_perspectiveTextureDeltaXBytes = g_perspectiveTextureDeltaXPow2 * g_bytesPerPixel;
    return 0;
}

/**
 * Reimplements 0x48d450: zRndr::OverlayBlendRow555_Scalar
 * Source-shape evidence: BN zRndr_Overlay.cpp loads and stores two 555 pixels
 * per uint32_t using the precomputed overlay premul and destination-scale globals;
 * the row extent is the inclusive right-left delta passed by FlushSw.
 * Purpose: Blend one 555 overlay row using the cached software overlay alpha and premultiplied source color.
 */
void __fastcall OverlayBlendRow555_Scalar(
    unsigned short *rowPixels16,
    int rightDelta
) {
    int pairCount = rightDelta >> 1;
    unsigned int *rowPairs = (unsigned int *)(rowPixels16);
    do {
        const unsigned int packedPair = *rowPairs;
        const unsigned int loLanes =
            ((((packedPair & 0x03e07c1fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5) +
                g_swOverlayPremulPackedRot16) &
            0x03e07c1fU;
        const unsigned int hiLanes =
            ((((packedPair >> 5) & 0x03e0f81fU) * (unsigned int)(g_swOverlayDstScale5)) +
                g_swOverlayPremulPacked) &
            0x7c1f03e0U;
        *rowPairs = hiLanes | loLanes;
        ++rowPairs;
    } while (pairCount-- != 0);
}

/**
 * Reimplements 0x48d4b0: zRndr::OverlayBlendRow565_Scalar
 * Source-shape evidence: BN zRndr_Overlay.cpp matches the 555 row shape with
 * two 565 pixels per uint32_t and the inclusive right-left delta row extent.
 * Purpose: Blend one 565 overlay row using the active pixel masks and cached overlay alpha.
 */
void __fastcall OverlayBlendRow565_Scalar(
    unsigned short *rowPixels16,
    int rightDelta
) {
    int pairCount = rightDelta >> 1;
    unsigned int *rowPairs = (unsigned int *)(rowPixels16);
    do {
        const unsigned int packedPair = *rowPairs;
        const unsigned int loLanes =
            ((((packedPair & 0x07e0f81fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5) +
                g_swOverlayPremulPackedRot16);
        const unsigned int hiLanes =
            (((packedPair >> 5) & 0x07c0f83fU) * (unsigned int)(g_swOverlayDstScale5)) +
            g_swOverlayPremulPacked;
        *rowPairs = ((loLanes ^ hiLanes) & 0x07e0f81fU) ^ hiLanes;
        ++rowPairs;
    } while (pairCount-- != 0);
}

/**
 * Reimplements 0x48d510: zRndr::OverlayBlendRow555_Mmx
 * Source-shape evidence: BN zRndr_Overlay.cpp builds replicated 555 masks,
 * premul RGB pairs, and destination-scale words on the stack, then processes
 * four 16-bit pixels per MMX qword before emms. The current C++ preserves
 * tier C behavior with the same four-pixel grouping and premul-pair globals
 * while the retail MMX instruction source shape remains pending.
 * Purpose: Preserve the MMX row-blend contract with scalar behavior while the retail MMX source shape remains pending.
 */
void __fastcall OverlayBlendRow555_Mmx(
    unsigned short *rowPixels16,
    int pixelCount
) {
    int groupCount = pixelCount >> 2;
    unsigned short *rowEnd = rowPixels16 + (groupCount << 2);
    int groupIndex = -groupCount;
    while (groupIndex < 0) {
        unsigned short *row = rowEnd + (groupIndex << 2);
        for (int lane = 0; lane < 4; ++lane) {
            const unsigned int dst = row[lane];
            const unsigned int red =
                (((dst & 0x7c00U) >> 5) * (unsigned int)(g_swOverlayDstScale5) +
                    (g_swOverlayPremulRPair & 0xffffU)) &
                0x7c00U;
            const unsigned int green =
                (((dst & 0x03e0U) >> 5) * (unsigned int)(g_swOverlayDstScale5) +
                    (g_swOverlayPremulGPair & 0xffffU)) &
                0x03e0U;
            const unsigned int blue =
                (((dst & 0x001fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5) +
                (g_swOverlayPremulBPair & 0xffffU);
            row[lane] = (unsigned short)(red | green | (blue & 0x001fU));
        }
        ++groupIndex;
    }
}

/**
 * Reimplements 0x48d5f0: zRndr::OverlayBlendRow565_Mmx
 * Source-shape evidence: BN zRndr_Overlay.cpp mirrors the 555 MMX row loop
 * with 565 masks, replicated premul RGB pairs, and four 16-bit pixels per MMX
 * qword before emms. The current C++ preserves tier C behavior with the same
 * four-pixel grouping and premul-pair globals while the retail MMX instruction
 * source shape remains pending.
 * Purpose: Preserve the MMX row-blend contract with scalar behavior while the retail MMX source shape remains pending.
 */
void __fastcall OverlayBlendRow565_Mmx(
    unsigned short *rowPixels16,
    int pixelCount
) {
    int groupCount = pixelCount >> 2;
    unsigned short *rowEnd = rowPixels16 + (groupCount << 2);
    int groupIndex = -groupCount;
    while (groupIndex < 0) {
        unsigned short *row = rowEnd + (groupIndex << 2);
        for (int lane = 0; lane < 4; ++lane) {
            const unsigned int dst = row[lane];
            const unsigned int red =
                (((dst & 0xf800U) >> 5) * (unsigned int)(g_swOverlayDstScale5) +
                    (g_swOverlayPremulRPair & 0xffffU)) &
                0xf800U;
            const unsigned int green =
                (((dst & 0x07e0U) >> 5) * (unsigned int)(g_swOverlayDstScale5) +
                    (g_swOverlayPremulGPair & 0xffffU)) &
                0x07e0U;
            const unsigned int blue =
                (((dst & 0x001fU) * (unsigned int)(g_swOverlayDstScale5)) >> 5) +
                (g_swOverlayPremulBPair & 0xffffU);
            row[lane] = (unsigned short)(red | green | (blue & 0x001fU));
        }
        ++groupIndex;
    }
}

/**
 * Reimplements 0x49e140: zRndr::SpanMmxSetPixelFormatMasks
 * Purpose: Replicate the active 555/565 pixel-format masks into the four-lane MMX span-mask globals.
 */
void __fastcall SpanMmxSetPixelFormatMasks(
    int greenBits
) {
    short redPacked;
    if (greenBits == 5) {
        g_mmxMaskGreenBits[3] = 0x03e0U;
        g_mmxMaskGreenBits[2] = 0x03e0U;
        g_mmxMaskGreenBits[1] = 0x03e0U;
        g_mmxMaskGreenBits[0] = 0x03e0U;
        g_mmxMaskBlueBits[3] = 0x001fU;
        g_mmxMaskBlueBits[2] = 0x001fU;
        g_mmxMaskBlueBits[1] = 0x001fU;
        g_mmxMaskBlueBits[0] = 0x001fU;
        redPacked = (short)(0xfc00U);
    } else {
        g_mmxMaskGreenBits[3] = 0x07e0U;
        g_mmxMaskGreenBits[2] = 0x07e0U;
        g_mmxMaskGreenBits[1] = 0x07e0U;
        g_mmxMaskGreenBits[0] = 0x07e0U;
        g_mmxMaskBlueBits[3] = 0x001fU;
        g_mmxMaskBlueBits[2] = 0x001fU;
        g_mmxMaskBlueBits[1] = 0x001fU;
        g_mmxMaskBlueBits[0] = 0x001fU;
        redPacked = (short)(0xf800U);
    }

    g_mmxMaskRedPacked[3] = (unsigned short)(redPacked);
    g_mmxMaskRedPacked[2] = (unsigned short)(redPacked);
    g_mmxMaskRedPacked[1] = (unsigned short)(redPacked);
    g_mmxMaskRedPacked[0] = (unsigned short)(redPacked);
    int greenPacked = -32;
    g_mmxMaskGreenPacked[3] = greenPacked;
    g_mmxMaskGreenPacked[2] = greenPacked;
    g_mmxMaskGreenPacked[1] = greenPacked;
    g_mmxMaskGreenPacked[0] = greenPacked;
}

/**
 * Reimplements 0x48ff80: zRndr::SelectSpanRoutines
 * Purpose: Refresh pixel-pack state and install the active 16-bit point, line, and span routines.
 */
void SelectSpanRoutines() {
    zVideo::PixelPack_GetRgbBits(
        &g_pixelPackRedBits,
        &g_pixelPackGreenBits,
        &g_pixelPackBlueBits
    );
    zVideo::PixelPack_GetRgbMasks(
        &g_pixelPackRedMask,
        &g_pixelPackGreenMask,
        &g_pixelPackBlueMask
    );
    zVideo::PixelPack_GetPackingParams(
        &g_pixelPackRedShift,
        &g_pixelPackGreenShift,
        &g_pixelPackBlueShift
    );

    if (g_graphicsFlags != 0) {
        *g_graphicsFlags &= ~4;
    }

    const int graphicsFlags = g_graphicsFlags != 0 ? *g_graphicsFlags : 0;
    const bool useShortAdaptiveSpans = (graphicsFlags & 8) != 0;
    SetPerspectiveAdaptiveSpanParams(
        useShortAdaptiveSpans ? 0x10 : 0x20,
        useShortAdaptiveSpans ? 0x40 : 0x200,
        0.100000001f
    );

    if (g_bytesPerPixel != 2) {
        return;
    }

    g_pfnPointOpCandidate = (PointOpProc)zRndr_PlotPixel16;
    g_pfnPointOpActive = (PointOpProc)zRndr_PlotPixel16;
    g_pfnImmediateRaster4 = zRndr_DrawLine16;
    g_pfnImmediateRasterReserved = zRndr_DrawLine16_Segmented;
    g_pfnImmediateRaster5 = zRndr_DrawLine16_Clipped;
    g_pfnSelectedSpanOp = (SpanRoutineProc)zRndr_FillSpan16Opaque;
    g_pfnSelectedSpanOp_Mode0 = SpanMasked16FromTex16SwitchVShift;
    g_pfnFlatImmediateSpanOp = g_pixelPackGreenBits == 5
                                   ? (FlatImmediateSpanProc)zRndr_FillSpan555Solid
                                   : (FlatImmediateSpanProc)zRndr_FillSpan565Solid;

    if ((graphicsFlags & 0x4) != 0) {
        g_pfnTexturedQueuedSpanOp_Mode0 = zSys::CheckCpuSignatureMask() != 0
                                              ? SpanCopy16FromTex16ExplicitVShift
                                              : SpanCopy16FromTex16;
        g_pfnTexturedQueuedSpanOp_Mode1 = SpanCopy16FromPal8SwitchVShift;
        g_pfnTexturedQueuedFinalize = g_pixelPackGreenBits == 5
                                          ? (SpanRoutineProc)FogBlendSpan555Mmx
                                          : (SpanRoutineProc)FogBlendSpan565Mmx;
        g_pfnTexturedQueuedFinalizeAlt = (SpanRoutineProc)SpanMmxSetTexUvMasksAndVShift;
    } else {
        g_pfnTexturedQueuedSpanOp_Mode0 = SpanCopy16FromTex16SwitchVShift;
        g_pfnTexturedQueuedSpanOp_Mode1 = SpanCopy16FromPal8SwitchVShift;
        g_pfnTexturedQueuedFinalize = g_pixelPackGreenBits == 5
                                          ? (SpanRoutineProc)FogBlendSpan555Scalar
                                          : (SpanRoutineProc)FogBlendSpan565Scalar;
        g_pfnTexturedQueuedFinalizeAlt = 0;
    }

    if ((graphicsFlags & 0x4) != 0) {
        SpanMmxSetPixelFormatMasks(g_pixelPackGreenBits);
    }

    const bool transparentSpansEnabled = (graphicsFlags & 2) != 0;
    if (transparentSpansEnabled) {
        if (g_pixelPackGreenBits == 6) {
            if ((graphicsFlags & 4) != 0) {
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
            if ((graphicsFlags & 4) != 0) {
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

/**
 * Reimplements 0x4903f0: zRndr::GetActiveRegionState
 * Source file evidence: GameZRecoil/zRndr/zRndr_Draw.cpp.
 * Data evidence: reads the active-region framebuffer, width, height,
 * bytes-per-pixel, and pitch globals at 0x632050-0x632060.
 * Purpose: Return the active framebuffer pointer and report the cached region dimensions, pixel depth, and pitch.
 */
void *__fastcall GetActiveRegionState(
    int *outWidth,
    int *outHeight,
    int *outBitsPerPixel,
    int *outPitchBytes
) {
    *outWidth = g_activeRegionWidth;
    *outHeight = g_activeRegionHeight;
    *outBitsPerPixel = g_bytesPerPixel << 3;
    *outPitchBytes = g_pitchBytes;
    return g_frameBuffer;
}

/**
 * Reimplements 0x490340: zRndr::SetFrameBufferRegion
 * Purpose: Set the active framebuffer region, pixel depth, pitch, and derived perspective texture stride.
 */
void __fastcall SetFrameBufferRegion(
    void *pixels,
    zOpt_ViewRectSection *activeRegionRect,
    int bitsPerPixel,
    int pitchBytes
) {
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

/**
 * Reimplements 0x4903c0: zRndr::SetActiveRegionSizeFromRect
 * Source file evidence: D:\Proj\GameZRecoil\zModel\zmodel.cpp.
 * Data evidence: writes the active-region width and height globals at
 * 0x632054 and 0x632058 from the HudUiRect extents.
 * Purpose: Refresh cached active region dimensions from a HUD rectangle.
 */
void __fastcall SetActiveRegionSizeFromRect(
    HudUiRect *rect
) {
    if (rect != 0) {
        g_activeRegionWidth = rect->right - rect->left;
        g_activeRegionHeight = rect->bottom - rect->top;
    }
}

/**
 * Reimplements 0x4903e0: zRndr::SetVideoStrideMirrors.
 * Original source path: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: copy the current video stride into the renderer span mirror globals.
 */
void __fastcall SetVideoStrideMirrors(
    int stride
) {
    g_videoStrideMirror1 = stride;
    g_videoStrideMirror0 = stride;
}

/**
 * Reimplements 0x490710: zRndr::SpanOcclusionAddPolygon.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: append one saved span-occluder polygon for the next column-table
 * rebuild.
 *
 * Evidence: BN caps the saved polygon list at seven active entries, copies the
 * submitted xyz vertices into gRndr_SpanOccluderPolys, clamps vertCount to
 * eight, and increments gRndr_SpanOccluderPolyCount.
 */
void __fastcall SpanOcclusionAddPolygon(
    const zVec3 *vertices,
    int vertCount
) {
    const int slotIndex = g_spanOccluderPolyCount;
    if (slotIndex >= 7) {
        return;
    }

    int i = 0;
    int remaining = vertCount;
    const zVec3 *vertex = vertices;
    while (remaining > 0) {
        SpanOccluderPolyPartial *slot = &g_spanOccluderPolys[slotIndex];
        slot->vertices[i][0] = vertex->x;
        slot->vertices[i][1] = vertex->y;
        slot->vertices[i][2] = vertex->z;
        ++i;
        ++vertex;
        --remaining;
    }

    g_spanOccluderPolys[slotIndex].vertCount =
        vertCount > 8 ? 8 : vertCount;
    ++g_spanOccluderPolyCount;
}

/**
 * Reimplements 0x490610: zRndr::SpanOcclusionSubmitOccluderRect.
 * Original file: D:\Proj\Battlesport\zrndr_span.cpp.
 * Purpose: convert one HUD rectangle into a four-vertex span-occluder polygon.
 *
 * Evidence: BN converts rect bounds to four xyz vertices, optionally halves x/y
 * coordinates for replicated rendering, assigns the uniform z value, and calls
 * zRndr::SpanOcclusionAddPolygon(vertices, 4).
 */
void __fastcall SpanOcclusionSubmitOccluderRect(
    const HudUiRect *rect,
    int halveIfReplicate,
    float z
) {
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
        {
            for (int index = 0; index < 4; ++index) {
                vertices[index].x *= 0.5f;
                vertices[index].y *= 0.5f;
            }
        }
    }

    vertices[0].z = z;
    vertices[1].z = z;
    vertices[2].z = z;
    vertices[3].z = z;
    SpanOcclusionAddPolygon(
        vertices,
        4
    );
}

/**
 * Reimplements 0x490520: zRndr::SpanOcclusionInit.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: allocate and initialize software span-occlusion column storage for
 * the active display height.
 *
 * Evidence: BN stores visible and padded column counts, allocates the column
 * head table and span-node pool with calloc, builds the initial column table,
 * clears the saved occluder count, and installs the local and secondary
 * span-list callback pointers.
 */
int __fastcall SpanOcclusionInit(
    int height
) {
    g_spanColumnCount = height;
    g_spanColumnCountPadded = height + 0x80;
    g_spanColumnHeadTable =
        (SpanNodePartial **)(calloc(
            (size_t)(g_spanColumnCountPadded),
            sizeof(SpanNodePartial *)
        ));
    g_spanPoolBase = (SpanNodePartial *)(calloc(
        (size_t)(g_spanColumnCountPadded) << 8,
        sizeof(SpanNodePartial)
    ));

    SpanOcclusionBuildColumnHeadTable();
    g_spanOccluderPolyCount = 0;
    g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
    g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanList;
    return 0;
}

/**
 * Reimplements 0x490590: zRndr::SpanOcclusionBuildColumnHeadTable.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: clear per-column span heads and rebuild them from saved occluder
 * polygons.
 *
 * Evidence: BN clears gRndr_SpanColumnHeadTable for gRndr_SpanColumnCount
 * entries, resets allocation and iteration cursors to the span pool, then
 * rasterizes each saved gRndr_SpanOccluderPolys entry.
 */
void SpanOcclusionBuildColumnHeadTable() {
    SpanNodePartial **columnHead = g_spanColumnHeadTable;
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
        SpanOccluderPolyPartial *poly = g_spanOccluderPolys;
        do {
            SpanOcclusionRasterizeOccluderPoly(
                poly,
                poly->vertCount
            );
            ++polyIndex;
            ++poly;
        } while (polyIndex < g_spanOccluderPolyCount);
    }
}

/**
 * Reimplements 0x4927d0: zRndr::SpanOcclusionRasterizeOccluderPoly.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: rasterize one saved occluder polygon into span nodes for each
 * affected screen column.
 *
 * Evidence: BN reduces duplicate/closing polygon vertices into the same
 * scratch base later passed as the span-list pointer array, builds two
 * fixed-point scan-conversion edge tables through duplicated scan-mode
 * branches, stages pending span-node min/max/depth values in
 * gRndr_SpanAllocCursor, and dispatches through gRndr_pfnBuildSpanList.
 */
void __fastcall SpanOcclusionRasterizeOccluderPoly(
    SpanOccluderPolyPartial *poly,
    int vertCount
) {
    SpanOcclusionRasterScratch scratch;
    int reducedCount = 1;
    scratch.reducedVerts[0].x = poly->vertices[0][0];
    scratch.reducedVerts[0].y = poly->vertices[0][1];

    const float (*sourceVertex)[3] = &poly->vertices[1];
    zVec3 *reducedVertex = &scratch.reducedVerts[1];
    int remainingVertices = vertCount - 1;
    while (remainingVertices > 0) {
        reducedVertex->x = (*sourceVertex)[0];
        reducedVertex->y = (*sourceVertex)[1];
        if (reducedVertex->x != (reducedVertex - 1)->x ||
            reducedVertex->y != (reducedVertex - 1)->y) {
            ++reducedCount;
            ++reducedVertex;
        }
        ++sourceVertex;
        --remainingVertices;
    }

    if (reducedCount > 1 && scratch.reducedVerts[reducedCount - 1].x == scratch.reducedVerts[0].x &&
        scratch.reducedVerts[reducedCount - 1].y == scratch.reducedVerts[0].y) {
        --reducedCount;
    }

    if (reducedCount < 3) {
        return;
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    for (int scanIndex = 1; scanIndex < reducedCount; ++scanIndex) {
        if (scratch.reducedVerts[scanIndex].y < scratch.reducedVerts[topVertexIndex].y) {
            topVertexIndex = scanIndex;
        }
        if (scratch.reducedVerts[scanIndex].y >= scratch.reducedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = scanIndex;
        }
    }

    ScanConvertEdge edgeTableA[0x40];
    ScanConvertEdge edgeTableB[0x40];
    int edgeCountA = 0;
    int edgeCountB = 0;
    int fixed16Value;
    int edgeVertexIndex;
    int edgeYStart;
    float edgeSampleY;

    if (g_scanConvertMode != 0) {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(
            fixed16Value,
            scratch.reducedVerts[edgeVertexIndex].y
        );
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        while (edgeVertexIndex != bottomVertexIndex) {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= reducedCount) {
                nextIndex -= reducedCount;
            }
            const zVec3 &start = scratch.reducedVerts[edgeVertexIndex];
            const zVec3 &end = scratch.reducedVerts[nextIndex];
            if (edgeSampleY <= end.y) {
                const float dy = end.y - start.y;
                edgeTableA[edgeCountA].yStart = edgeYStart;
                edgeTableA[edgeCountA].reserved = 0;
                if (dy != 0.0f) {
                    const float xSlope = (end.x - start.x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableA[edgeCountA].xStepFixed,
                        xSlope
                    );
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableA[edgeCountA].currentXFixed,
                        start.x + (edgeSampleY - start.y) * xSlope
                    );
                } else {
                    edgeTableA[edgeCountA].xStepFixed = 0;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableA[edgeCountA].currentXFixed,
                        start.x
                    );
                }

                ++edgeCountA;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    fixed16Value,
                    end.y
                );
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        }

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(
            fixed16Value,
            scratch.reducedVerts[edgeVertexIndex].y
        );
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        while (edgeVertexIndex != bottomVertexIndex) {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += reducedCount;
            }
            const zVec3 &start = scratch.reducedVerts[edgeVertexIndex];
            const zVec3 &end = scratch.reducedVerts[nextIndex];
            if (edgeSampleY <= end.y) {
                const float dy = end.y - start.y;
                edgeTableB[edgeCountB].yStart = edgeYStart;
                edgeTableB[edgeCountB].reserved = 0;
                if (dy != 0.0f) {
                    const float xSlope = (end.x - start.x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableB[edgeCountB].xStepFixed,
                        xSlope
                    );
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableB[edgeCountB].currentXFixed,
                        start.x + (edgeSampleY - start.y) * xSlope
                    );
                } else {
                    edgeTableB[edgeCountB].xStepFixed = 0;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableB[edgeCountB].currentXFixed,
                        start.x
                    );
                }

                ++edgeCountB;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    fixed16Value,
                    end.y
                );
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        }
    } else {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(
            fixed16Value,
            scratch.reducedVerts[edgeVertexIndex].y
        );
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        while (edgeVertexIndex != bottomVertexIndex) {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= reducedCount) {
                nextIndex -= reducedCount;
            }
            const zVec3 &start = scratch.reducedVerts[edgeVertexIndex];
            const zVec3 &end = scratch.reducedVerts[nextIndex];
            if (edgeSampleY <= end.y) {
                const float dy = end.y - start.y;
                edgeTableB[edgeCountB].yStart = edgeYStart;
                edgeTableB[edgeCountB].reserved = 0;
                if (dy != 0.0f) {
                    const float xSlope = (end.x - start.x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableB[edgeCountB].xStepFixed,
                        xSlope
                    );
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableB[edgeCountB].currentXFixed,
                        start.x + (edgeSampleY - start.y) * xSlope
                    );
                } else {
                    edgeTableB[edgeCountB].xStepFixed = 0;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableB[edgeCountB].currentXFixed,
                        start.x
                    );
                }

                ++edgeCountB;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    fixed16Value,
                    end.y
                );
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        }

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(
            fixed16Value,
            scratch.reducedVerts[edgeVertexIndex].y
        );
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        while (edgeVertexIndex != bottomVertexIndex) {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += reducedCount;
            }
            const zVec3 &start = scratch.reducedVerts[edgeVertexIndex];
            const zVec3 &end = scratch.reducedVerts[nextIndex];
            if (edgeSampleY <= end.y) {
                const float dy = end.y - start.y;
                edgeTableA[edgeCountA].yStart = edgeYStart;
                edgeTableA[edgeCountA].reserved = 0;
                if (dy != 0.0f) {
                    const float xSlope = (end.x - start.x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableA[edgeCountA].xStepFixed,
                        xSlope
                    );
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableA[edgeCountA].currentXFixed,
                        start.x + (edgeSampleY - start.y) * xSlope
                    );
                } else {
                    edgeTableA[edgeCountA].xStepFixed = 0;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edgeTableA[edgeCountA].currentXFixed,
                        start.x
                    );
                }

                ++edgeCountA;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    fixed16Value,
                    end.y
                );
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        }
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(
        fixed16Value,
        scratch.reducedVerts[topVertexIndex].y
    );
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(
        fixed16Value,
        scratch.reducedVerts[bottomVertexIndex].y
    );
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    SpanNodePartial **spanList = scratch.spanList;
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        while (edgeIndexA < edgeCountA && y >= edgeTableA[edgeIndexA].yStart) {
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            ++edgeIndexA;
        }

        while (edgeIndexB < edgeCountB && y >= edgeTableB[edgeIndexB].yStart) {
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            ++edgeIndexB;
        }

        int xMin;
        int xMax;
        if (currentXFixedA > currentXFixedB) {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            g_spanAllocCursor->invDepth = poly->vertices[0][2];
            g_spanAllocCursor->invDepthStep = poly->vertices[0][2];
            g_spanAllocCursor->depthSlope = 0.0f;

            int spanCount = 0;
            g_pfnBuildSpanList(
                spanList,
                y,
                &spanCount
            );
        }
    }
}

/**
 * Reimplements 0x490600: zRndr::SpanOcclusionResetFrame.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: clear saved span-occluder polygons for a new rendered frame.
 *
 * Evidence: BN writes zero to gRndr_SpanOccluderPolyCount and returns.
 */
void SpanOcclusionResetFrame() {
    g_spanOccluderPolyCount = 0;
}

/**
 * Reimplements 0x490780: zRndr::SpanOcclusionShutdown.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: release the software span-occlusion column table and span-node pool.
 *
 * Evidence: BN frees non-null gRndr_SpanColumnHeadTable and gRndr_SpanPoolBase
 * through the CRT free import, clears those two globals, and returns zero in
 * eax before the epilogue.
 */
int SpanOcclusionShutdown() {
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

/**
 * Reimplements 0x49e0e0: zRndr::FogTarget565_SetPackedColorAndRamp
 * Source file evidence: GameZRecoil/zRndr/zRndr_Span.cpp.
 * Data evidence: stores RGB565 component fields, writes packedColor16 as a
 * 16-bit field, replicates the packed 565 color, and fills packedColorRamp[31..0].
 * Purpose: Build the packed fog color and ramp table used by 16-bit fog blending.
 */
void __fastcall FogTarget565_SetPackedColorAndRamp(
    FogParamsPartial *params,
    int packedRed,
    int packedGreen,
    int packedBlue
) {
    const unsigned int packedColor16 = (unsigned int)(packedRed | packedGreen | packedBlue);
    params->packedColorRed = packedRed;
    params->packedColorGreen = packedGreen;
    params->packedColorBlue = packedBlue;
    params->packedColor16 = (unsigned short)(packedColor16);
    params->packedColor16Dup = (int)(packedColor16 | (packedColor16 << 16));

    const unsigned int rampStep =
        ((unsigned int)(packedRed | packedBlue) << 11) | ((unsigned int)(packedGreen) >> 5);
    unsigned int rampValue = 0;
    for (int i = 31; i >= 0; --i) {
        params->packedColorRamp[i] = (int)(rampValue);
        rampValue += rampStep;
    }
}

namespace {
static inline int SpanTex16SampleIndex(
    int texU,
    int texV,
    int texVShift,
    int texUMask
);
static inline unsigned short BlendPixel565Alpha8(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
);
static inline unsigned int BlendPair565Alpha5(
    unsigned int dstPair,
    unsigned short srcPixel,
    int alpha
);
static inline unsigned short BlendPixel555Alpha8(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
);
static inline unsigned int BlendPair555Alpha5(
    unsigned int dstPair,
    unsigned short srcPixel,
    int alpha
);
static inline unsigned short BlendPixel555ConstAlphaMap(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
);
} // namespace

/**
 * Reimplements 0x49c230: zRndr::SpanAlphaBlend565ConstAlphaFromPal8
 * Source-shape evidence: BN uses the sampled pal8 texel for the high-alpha
 * palette copy path, but the partial-alpha path reloads the current destination
 * word and uses that word as the palette index before blending 565 channels.
 * Purpose: Blend palettized texture samples into a 565 span using the active constant alpha.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromPal8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexPixels[vIndex + uIndex];
        if (sourceIndex != 0 && g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = g_spanActiveTexPalette[(short)(sourceIndex)];
            } else {
                const int dstColor = (short)(*dst);
                // BN 0x49c2ba intentionally uses the current destination word
                // as the palette index in this partial-alpha path.
                const int srcColor = g_spanActiveTexPalette[dstColor];
                const int greenDelta =
                    (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta =
                    (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49c020: zRndr::SpanMasked16FromPal8To565
 * Source-shape evidence: BN's retail body owns the same generic V-shift pal8
 * 565 loop as 0x49c230, including the nonzero source gate, alpha > 3 gate,
 * alpha >= 0xfc palette copy, and destination-word palette lookup in the
 * partial-alpha path.
 * Purpose: Write nonzero palettized texture samples into a 565 span using the active constant alpha.
 */
void __fastcall SpanMasked16FromPal8To565(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    const unsigned char *texels = g_spanActiveTexPixels;
    int activeAlpha = g_spanActiveConstAlphaBits;
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *palette = g_spanActiveTexPalette;

    do {
        const int vIndex = (int)((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift);
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned short sourceIndex = texels[vIndex + uIndex];
        if (sourceIndex != 0 && (unsigned int)(activeAlpha) > 3) {
            if ((unsigned int)(activeAlpha) >= 0xfc) {
                *dst = palette[(short)(sourceIndex)];
                activeAlpha = g_spanActiveConstAlphaBits;
            } else {
                const int dstColor = (short)(*dst);
                // BN 0x49c0aa intentionally uses the current destination word
                // as the palette index in this partial-alpha path.
                const int srcColor = palette[dstColor];
                const int dstGreen = dstColor & 0x07e0;
                const int srcGreen = srcColor & 0x07e0;
                const int greenDelta =
                    (srcGreen - dstGreen) * activeAlpha;
                const int dstRed = dstColor & 0xf800;
                const int srcRed = srcColor & 0xf800;
                const int redDelta =
                    (srcRed - dstRed) * activeAlpha;
                int blended = dstColor +
                    ((int)((unsigned int)(redDelta) >> 8) & 0xfffff800);
                const int srcBlue = srcColor & 0x001f;
                const int blendedBlue = blended & 0x001f;
                const int blueDelta =
                    (srcBlue - blendedBlue) * activeAlpha;
                blended +=
                    ((int)((unsigned int)(greenDelta) >> 8) & 0xffffffe0) +
                    (int)((unsigned int)(blueDelta) >> 8);
                *dst = (unsigned short)(blended);
                activeAlpha = g_spanActiveConstAlphaBits;
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    } while (--pixelCount != 0);
}

/**
 * Reimplements 0x49c150: zRndr::SpanMasked16FromTex16To565
 * Source-shape evidence: BN samples a nonzero tex16 mask and copies it only
 * for alpha >= 0xfc; the partial-alpha branch emits channel math that collapses
 * to preserving the current destination word.
 * Purpose: Copy nonzero 16-bit texture samples into a 565 destination span.
 */
void __fastcall SpanMasked16FromTex16To565(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned short sourceTexel = texels16[vIndex + uIndex];
        if (sourceTexel != 0 && g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = sourceTexel;
            } else {
                // BN 0x49c1f2 reaches the partial-alpha branch but adds zero,
                // preserving the destination after the source/nonzero gate.
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49c360: zRndr::SpanAlphaBlend565FromTex16Alpha8
 * Source-shape evidence: BN inlines the odd tex16 alpha-map scalar path,
 * duplicates one sampled texel into a packed pair, reduces alpha to five bits,
 * and blends the two-pixel 565 lanes with packed masks.
 * Purpose: Alpha-blend 16-bit texture samples into a 565 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend565FromTex16Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    if ((pixelCount & 1) != 0) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            if (alpha >= 0xf8) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel565Alpha8(
                    *dst,
                    sourceTexel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    {
        for (int pairCount = pixelCount >> 1; pairCount != 0; --pairCount) {
            const int sourceIndex =
                SpanTex16SampleIndex(
                    texU,
                    texV,
                    texVShift,
                    g_spanActiveTexUMask
                );
            const int alpha = alphaMap[sourceIndex];
            if (alpha >= 8) {
                const unsigned short sourceTexel = texels16[sourceIndex];
                unsigned int packedPixels = 0;
                if (alpha >= 0xf8) {
                    packedPixels =
                        (unsigned int)(sourceTexel) | ((unsigned int)(sourceTexel) << 16);
                } else {
                    memcpy(
                        &packedPixels,
                        dst,
                        sizeof(packedPixels)
                    );
                    packedPixels = BlendPair565Alpha5(
                        packedPixels,
                        sourceTexel,
                        alpha
                    );
                }

                memcpy(
                    dst,
                    &packedPixels,
                    sizeof(packedPixels)
                );
            }

            texU += g_spanActiveTexUStepFixed20 * 2;
            texV += g_spanActiveTexVStepFixed20 * 2;
            dst += 2;
        }
    }
}

/**
 * Reimplements 0x49c560: zRndr::SpanAlphaBlend555FromTex16Alpha8
 * Source-shape evidence: BN matches the tex16 alpha-map odd/pair loop with
 * 555-specific red and green masks in the packed two-pixel blend.
 * Purpose: Alpha-blend 16-bit texture samples into a 555 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend555FromTex16Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    if ((pixelCount & 1) != 0) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            if (alpha >= 0xf8) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel555Alpha8(
                    *dst,
                    sourceTexel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    {
        for (int pairCount = pixelCount >> 1; pairCount != 0; --pairCount) {
            const int sourceIndex =
                SpanTex16SampleIndex(
                    texU,
                    texV,
                    texVShift,
                    g_spanActiveTexUMask
                );
            const int alpha = alphaMap[sourceIndex];
            if (alpha >= 8) {
                const unsigned short sourceTexel = texels16[sourceIndex];
                unsigned int packedPixels = 0;
                if (alpha >= 0xf8) {
                    packedPixels =
                        (unsigned int)(sourceTexel) | ((unsigned int)(sourceTexel) << 16);
                } else {
                    memcpy(
                        &packedPixels,
                        dst,
                        sizeof(packedPixels)
                    );
                    packedPixels = BlendPair555Alpha5(
                        packedPixels,
                        sourceTexel,
                        alpha
                    );
                }

                memcpy(
                    dst,
                    &packedPixels,
                    sizeof(packedPixels)
                );
            }

            texU += g_spanActiveTexUStepFixed20 * 2;
            texV += g_spanActiveTexVStepFixed20 * 2;
            dst += 2;
        }
    }
}

/**
 * Reimplements 0x49c970: zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8
 * Source-shape evidence: BN uses the same active U/V index for tex16 and
 * alpha-map reads, scales the alpha byte by the float stored in
 * gRndr_ActiveConstAlphaBits, skips alpha <= 3, copies for alpha >= 0xfc, and
 * otherwise blends 565 channels.
 * Purpose: Blend 16-bit texture samples into a 565 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromTex16Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    float alphaScale = 0.0f;
    memcpy(
        &alphaScale,
        &g_spanActiveConstAlphaBits,
        sizeof(alphaScale)
    );

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const int alpha = (int)(alphaScaled >= 0.0 ? alphaScaled + 0.5 : alphaScaled - 0.5);
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel565Alpha8(
                    *dst,
                    sourceTexel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49ca90: zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8
 * Source-shape evidence: BN matches the tex16 alpha-map scaling loop with a
 * 555-specific alpha > 7 gate and 555 channel masks.
 * Purpose: Blend 16-bit texture samples into a 555 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFromTex16Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    float alphaScale = 0.0f;
    memcpy(
        &alphaScale,
        &g_spanActiveConstAlphaBits,
        sizeof(alphaScale)
    );

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const int alpha = (int)(alphaScaled >= 0.0 ? alphaScaled + 0.5 : alphaScaled - 0.5);
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel555ConstAlphaMap(
                    *dst,
                    sourceTexel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49cbb0: zRndr::SpanAlphaBlend565MmxFromTex16Alpha8
 * Source-shape evidence: BN builds paired U/V indices with the MMX mask and
 * step globals, stages sampled tex16 pixels and alpha bytes in a stack scratch
 * area, blends packed groups, then runs a scalar tail with the 565 gates.
 * Purpose: Blend tex16 alpha-map samples into a 565 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend565MmxFromTex16Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        *dst = BlendPixel565Alpha8(
            *dst,
            texels16[sourceIndex],
            alphaMap[sourceIndex]
        );

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1490 = quadPixels; i_1490 < pixelCount; ++i_1490) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel565Alpha8(
                    *dst,
                    sourceTexel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49cea0: zRndr::SpanAlphaBlend555MmxFromTex16Alpha8
 * Source-shape evidence: BN matches the 565 MMX alpha-map staging loop but
 * uses the 555 red/green masks and an alpha > 7 scalar-tail gate.
 * Purpose: Blend tex16 alpha-map samples into a 555 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend555MmxFromTex16Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        *dst = BlendPixel555Alpha8(
            *dst,
            texels16[sourceIndex],
            alphaMap[sourceIndex]
        );

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1529 = quadPixels; i_1529 < pixelCount; ++i_1529) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel555Alpha8(
                    *dst,
                    sourceTexel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49d1a0: zRndr::SpanAlphaBlend565FromPal8Alpha8
 * Source-shape evidence: BN expands each sampled pal8 texel through the active
 * palette before the odd scalar and packed two-pixel 565 alpha-map blend.
 * Purpose: Alpha-blend palettized texture samples into a 565 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend565FromPal8Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned char *texels8 = g_spanActiveTexPixels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    if ((pixelCount & 1) != 0) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel565Alpha8(
                    *dst,
                    sourcePixel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i = pixelCount >> 1; i != 0; --i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                dst[0] = sourcePixel;
                dst[1] = sourcePixel;
            } else {
                unsigned int packedPixels = 0;
                memcpy(
                    &packedPixels,
                    dst,
                    sizeof(packedPixels)
                );
                packedPixels = BlendPair565Alpha5(
                    packedPixels,
                    sourcePixel,
                    alpha
                );
                memcpy(
                    dst,
                    &packedPixels,
                    sizeof(packedPixels)
                );
            }
        }

        texU += 2 * g_spanActiveTexUStepFixed20;
        texV += 2 * g_spanActiveTexVStepFixed20;
        dst += 2;
    }
}

/**
 * Reimplements 0x49d3b0: zRndr::SpanAlphaBlend555FromPal8Alpha8
 * Source-shape evidence: BN matches the pal8 alpha-map odd/pair loop with
 * active-palette expansion and 555-specific packed blend masks.
 * Purpose: Alpha-blend palettized texture samples into a 555 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend555FromPal8Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned char *texels8 = g_spanActiveTexPixels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    if ((pixelCount & 1) != 0) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel555Alpha8(
                    *dst,
                    sourcePixel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i = pixelCount >> 1; i != 0; --i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                dst[0] = sourcePixel;
                dst[1] = sourcePixel;
            } else {
                unsigned int packedPixels = 0;
                memcpy(
                    &packedPixels,
                    dst,
                    sizeof(packedPixels)
                );
                packedPixels = BlendPair555Alpha5(
                    packedPixels,
                    sourcePixel,
                    alpha
                );
                memcpy(
                    dst,
                    &packedPixels,
                    sizeof(packedPixels)
                );
            }
        }

        texU += 2 * g_spanActiveTexUStepFixed20;
        texV += 2 * g_spanActiveTexVStepFixed20;
        dst += 2;
    }
}

/**
 * Reimplements 0x49d810: zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8
 * Source-shape evidence: BN samples pal8 texels and the alpha map through the
 * same active U/V index, expands the texel through the active palette, scales
 * alpha by the float constant-alpha value, and applies the 565 alpha gates.
 * Purpose: Blend palettized texture samples into a 565 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromPal8Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned char *texels8 = g_spanActiveTexPixels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    float alphaScale = 0.0f;
    memcpy(
        &alphaScale,
        &g_spanActiveConstAlphaBits,
        sizeof(alphaScale)
    );

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const int alpha = (int)(alphaScaled >= 0.0 ? alphaScaled + 0.5 : alphaScaled - 0.5);
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel565Alpha8(
                    *dst,
                    sourcePixel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49d950: zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8
 * Source-shape evidence: BN matches the pal8 alpha-map scaling loop with the
 * active palette expansion and 555-specific alpha > 7 gate.
 * Purpose: Blend palettized texture samples into a 555 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFromPal8Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned char *texels8 = g_spanActiveTexPixels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    float alphaScale = 0.0f;
    memcpy(
        &alphaScale,
        &g_spanActiveConstAlphaBits,
        sizeof(alphaScale)
    );

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const int alpha = (int)(alphaScaled >= 0.0 ? alphaScaled + 0.5 : alphaScaled - 0.5);
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel555ConstAlphaMap(
                    *dst,
                    sourcePixel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49da80: zRndr::SpanAlphaBlend565MmxFromPal8Alpha8
 * Source-shape evidence: BN stages paired pal8 samples through the active
 * palette, alpha bytes through the active alpha map, and packed 565 blends
 * through the same MMX U/V index body before the scalar tail.
 * Purpose: Blend pal8 alpha-map samples into a 565 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend565MmxFromPal8Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned char *texels8 = g_spanActiveTexPixels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        *dst = BlendPixel565Alpha8(
            *dst,
            palette[texels8[sourceIndex]],
            alphaMap[sourceIndex]
        );

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1733 = quadPixels; i_1733 < pixelCount; ++i_1733) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel565Alpha8(
                    *dst,
                    sourcePixel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49ddb0: zRndr::SpanAlphaBlend555MmxFromPal8Alpha8
 * Source-shape evidence: BN matches the pal8 MMX alpha-map staging loop but
 * uses the 555 red/green masks and an alpha > 7 scalar-tail gate.
 * Purpose: Blend pal8 alpha-map samples into a 555 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend555MmxFromPal8Alpha8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned char *texels8 = g_spanActiveTexPixels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        *dst = BlendPixel555Alpha8(
            *dst,
            palette[texels8[sourceIndex]],
            alphaMap[sourceIndex]
        );

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1773 = quadPixels; i_1773 < pixelCount; ++i_1773) {
        const int sourceIndex = SpanTex16SampleIndex(
            texU,
            texV,
            texVShift,
            g_spanActiveTexUMask
        );
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel555Alpha8(
                    *dst,
                    sourcePixel,
                    alpha
                );
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49c760: zRndr::SpanAlphaBlend565ConstAlphaFromTex16
 * Source-shape evidence: BN samples a 16-bit texel through the active U/V
 * masks, skips only when gRndr_ActiveConstAlphaBits <= 3, copies for alpha
 * >= 0xfc, and otherwise blends 565 channels toward the texel.
 * Purpose: Blend 16-bit texture samples into a 565 span using the active constant alpha.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromTex16(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int srcColor = (short)(texels16[vIndex + uIndex]);
        if (g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int greenDelta =
                    (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta =
                    (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49c860: zRndr::SpanAlphaBlend555ConstAlphaFromTex16
 * Source-shape evidence: BN matches the tex16 constant-alpha loop shape with a
 * stricter alpha > 7 gate and 555 red/green/blue channel masks.
 * Purpose: Blend 16-bit texture samples into a 555 span using the active constant alpha.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFromTex16(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int srcColor = (short)(texels16[vIndex + uIndex]);
        if (g_spanActiveConstAlphaBits > 7) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int redDelta =
                    (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta =
                    (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49d5c0: zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8
 * Source-shape evidence: BN samples an 8-bit texel, expands it through the
 * active palette before the alpha gate, skips only when alpha <= 3, copies for
 * alpha >= 0xfc, and otherwise blends 565 channels toward the palette color.
 * Purpose: Blend palettized texture samples into a 565 span using fast constant alpha.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFastFromPal8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexPixels[vIndex + uIndex];
        const int srcColor = (short)(g_spanActiveTexPalette[sourceIndex]);
        if (g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int greenDelta =
                    (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta =
                    (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

/**
 * Reimplements 0x49d6e0: zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8
 * Source-shape evidence: BN matches the fast pal8 constant-alpha loop shape
 * with alpha <= 7 skip behavior and 555 channel masks.
 * Purpose: Blend palettized texture samples into a 555 span using fast constant alpha.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFastFromPal8(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexPixels[vIndex + uIndex];
        const int srcColor = (short)(g_spanActiveTexPalette[sourceIndex]);
        if (g_spanActiveConstAlphaBits > 7) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int redDelta =
                    (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta =
                    (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}

namespace {
/**
 * Recovered inline helper: zRndr fog packed-color rotate
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200 and 0x49e300 fog blend callers as the rotate-right term in packed 565/555 ramp blending.
 * Purpose: Rotate packed 32-bit color terms right by a caller-selected bit count.
 */
static inline unsigned int RotateRight32(
    unsigned int value,
    int count
) {
    return (value >> count) | (value << (32 - count));
}

/**
 * Recovered inline helper: zRndr fog saturated-coordinate test
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200, 0x49e300, 0x49e400, and 0x49e560 before ramp or solid-fog blending.
 * Purpose: Detect fog coordinates that have reached the fully fogged color.
 */
static inline bool FogCoordIsFullyFogged(
    unsigned int fogCoordFixed24
) {
    return (int)(fogCoordFixed24) >= 0x1000000;
}

/**
 * Recovered inline helper: zRndr fog ramp-range test
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200, 0x49e300, 0x49e400, and 0x49e560 before ramp lookup.
 * Purpose: Detect fog coordinates that should use the packed color ramp.
 */
static inline bool FogCoordUsesRamp(
    unsigned int fogCoordFixed24
) {
    return (int)(fogCoordFixed24) >= 0x80000;
}

/**
 * Recovered inline helper: zRndr fog ramp index
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200, 0x49e300, 0x49e400, and 0x49e560 as the fixed-point ramp lookup expression.
 * Purpose: Convert a fixed-point fog coordinate into the 32-entry ramp index.
 */
static inline unsigned int FogRampIndex(
    unsigned int fogCoordFixed24
) {
    return (0x1000000u - fogCoordFixed24) >> 19;
}

/**
 * Recovered inline helper: zRndr 565 fog pixel blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200 and through the 0x49e400 MMX-shaped fog blend tail.
 * Purpose: Blend one 565 pixel against the active packed fog ramp.
 */
static inline unsigned short FogBlendPixel565(
    unsigned short pixel,
    unsigned int fogCoordFixed24
) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return (unsigned short)(g_fogParamsActive.packedColor16);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return pixel;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int pixel32 = pixel;
    const unsigned int green = ((((pixel32 & 0x07e0u) >> 5) * rampIndex) + rampValue) & 0x07e0u;
    const unsigned int redBlue =
        (((pixel32 & 0xf81fu) * rampIndex + RotateRight32(
            rampValue,
            11
        )) >> 5) & 0xf81fu;
    return (unsigned short)(green + redBlue);
}

/**
 * Recovered inline helper: zRndr 555 fog pixel blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e300 and through the 0x49e560 MMX-shaped fog blend tail.
 * Purpose: Blend one 555 pixel against the active packed fog ramp.
 */
static inline unsigned short FogBlendPixel555(
    unsigned short pixel,
    unsigned int fogCoordFixed24
) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return (unsigned short)(g_fogParamsActive.packedColor16);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return pixel;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int pixel32 = pixel;
    const unsigned int green = ((((pixel32 & 0x03e0u) >> 5) * rampIndex) + rampValue) & 0x03e0u;
    const unsigned int redBlue =
        (((pixel32 & 0x7c1fu) * rampIndex + RotateRight32(
            rampValue,
            11
        )) >> 5) & 0x7c1fu;
    return (unsigned short)(green + redBlue);
}

/**
 * Recovered inline helper: zRndr 565 fog pair blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200 paired-pixel fog loops.
 * Purpose: Blend two packed 565 pixels against the active packed fog ramp.
 */
static inline unsigned int FogBlendPair565(
    unsigned int packedPixels,
    unsigned int fogCoordFixed24
) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return (unsigned int)(g_fogParamsActive.packedColor16Dup);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return packedPixels;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int green =
        ((((packedPixels & 0xf81f07e0u) >> 5) * rampIndex) + rampValue) & 0xf81f07e0u;
    const unsigned int redBlue =
        (((packedPixels & 0x07e0f81fu) * rampIndex + RotateRight32(
            rampValue,
            11
        )) >> 5) &
        0x07e0f81fu;
    return green + redBlue;
}

/**
 * Recovered inline helper: zRndr 555 fog pair blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e300 paired-pixel fog loops.
 * Purpose: Blend two packed 555 pixels against the active packed fog ramp.
 */
static inline unsigned int FogBlendPair555(
    unsigned int packedPixels,
    unsigned int fogCoordFixed24
) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return (unsigned int)(g_fogParamsActive.packedColor16Dup);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return packedPixels;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int green =
        ((((packedPixels & 0x7c1f03e0u) >> 5) * rampIndex) + rampValue) & 0x7c1f03e0u;
    const unsigned int redBlue =
        (((packedPixels & 0x03e07c1fu) * rampIndex + RotateRight32(
            rampValue,
            11
        )) >> 5) &
        0x03e07c1fu;
    return green + redBlue;
}

/**
 * Recovered inline helper: zRndr signed MMX word subtract
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e400 and 0x49e560 MMX-lane fog blend reconstruction.
 * Purpose: Emulate the saturating signed word subtract used by the MMX fog lane.
 */
static inline short SaturatingSubWord(
    unsigned short minuend,
    unsigned short subtrahend
) {
    const int result = (short)(minuend) - (short)(subtrahend);
    if (result > 0x7fff) {
        return 0x7fff;
    }
    if (result < -0x8000) {
        return -32768;
    }
    return (short)(result);
}

/**
 * Recovered inline helper: zRndr signed MMX low-word multiply
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e400 and 0x49e560 MMX-lane fog blend reconstruction.
 * Purpose: Emulate the low-word signed multiply used by the MMX fog lane.
 */
static inline unsigned short MultiplyLowWord(
    short lhs,
    short rhs
) {
    return (unsigned short)((int)(lhs) * (int)(rhs));
}

/**
 * Recovered inline helper: zRndr MMX fog lane blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e400 and 0x49e560 as repeated per-lane MMX fog math.
 * Purpose: Blend one lane of the MMX-shaped fog quad with active mask globals.
 */
static inline unsigned short FogBlendMmxLane(
    unsigned short pixel,
    unsigned short fogFactor,
    int lane,
    int redShift,
    int redTermShift
) {
    const short factor = (short)(fogFactor);
    const short redDelta =
        SaturatingSubWord(
            g_mmxBitsRed255[lane],
            (unsigned short)(pixel >> redShift)
        );
    const short greenDelta = SaturatingSubWord(
        g_mmxBitsGreen255[lane],
        (unsigned short)((pixel & g_mmxMaskGreenBits[lane]) >> 5)
    );
    const short blueDelta = SaturatingSubWord(
        g_mmxBitsBlue255[lane],
        (unsigned short)(pixel & g_mmxMaskBlueBits[lane])
    );

    const unsigned short redProduct = MultiplyLowWord(
        redDelta,
        factor
    );
    const unsigned short greenProduct = MultiplyLowWord(
        greenDelta,
        factor
    );
    const unsigned short blueProduct = MultiplyLowWord(
        blueDelta,
        factor
    );

    const unsigned short redTerm =
        (unsigned short)(redProduct << redTermShift) & g_mmxMaskRedPacked[lane];
    const unsigned short greenTerm =
        (unsigned short)((short)(greenProduct) >> 3) & g_mmxMaskGreenPacked[lane];
    const unsigned short blueTerm = (unsigned short)((short)(blueProduct) >> 8);

    return (unsigned short)(pixel + redTerm + greenTerm + blueTerm);
}

/**
 * Recovered inline helper: zRndr span texture sample index
 * Original-source inline helper evidence: No standalone retail function is expected; observed in span callers including 0x49e6c0, 0x49b7e0, 0x49edc0, 0x49bbf0, and 0x49f180.
 * Purpose: Combine fixed-point texture U and masked V coordinates into the active texture sample index.
 */
static inline int SpanTex16SampleIndex(
    int texU,
    int texV,
    int texVShift,
    int texUMask
) {
    const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
    const int uIndex = (texU >> 20) & texUMask;
    return vIndex + uIndex;
}

/**
 * Recovered inline helper: zRndr 16-bit texture sample
 * Original-source inline helper evidence: No standalone plan/source-map entry; observed in span callers including 0x49e6c0, 0x49ea80, and 0x49ec20.
 * Purpose: Read a 16-bit texel from the active texture using the recovered fixed-point sample-index helper.
 */
static inline unsigned short SpanTex16Sample(
    int texU,
    int texV,
    int texVShift,
    int texUMask
) {
    const unsigned short *texels = (const unsigned short *)(g_spanActiveTexPixels);
    return texels[SpanTex16SampleIndex(
        texU,
        texV,
        texVShift,
        texUMask
    )];
}

/**
 * Recovered inline helper: zRndr palettized texture sample expansion
 * Original-source inline helper evidence: No standalone plan/source-map entry; observed in 0x49edc0, 0x49bbf0, and 0x49f180 palettized texture span patterns.
 * Purpose: Expand an 8-bit texture sample through the active span palette.
 */
static inline unsigned short SpanPal8SampleExpanded(
    int texU,
    int texV,
    int texVShift,
    int texUMask
) {
    const int sourceIndex = SpanTex16SampleIndex(
        texU,
        texV,
        texVShift,
        texUMask
    );
    return g_spanActiveTexPalette[g_spanActiveTexPixels[sourceIndex]];
}

/**
 * Recovered inline helper: zRndr 565 alpha pixel blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed across 0x49c360, 0x49c970, 0x49cbb0, 0x49d1a0, 0x49d810, and 0x49da80 alpha-map span callers.
 * Purpose: Blend one 565 destination pixel toward a source pixel using an 8-bit alpha value.
 */
static inline unsigned short BlendPixel565Alpha8(
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
 * Recovered inline helper: zRndr 565 alpha pair blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49c360 and 0x49d1a0 paired-pixel alpha blend loops.
 * Purpose: Blend two packed 565 destination pixels toward one source pixel using a 5-bit alpha approximation.
 */
static inline unsigned int BlendPair565Alpha5(
    unsigned int dstPair,
    unsigned short srcPixel,
    int alpha
) {
    const unsigned int srcPair = (unsigned int)(srcPixel) | ((unsigned int)(srcPixel) << 16);
    const unsigned int alpha5 = (unsigned int)(alpha >> 3);
    const unsigned int inverseAlpha5 = 0x1fu - alpha5;
    const unsigned int lowTerms =
        ((((dstPair & 0x07e0f81fu) * inverseAlpha5) + ((srcPair & 0x07e0f81fu) * alpha5)) >> 5) &
        0x07e0f81fu;
    const unsigned int highTerms = ((((dstPair >> 5) & 0x07c0f83fu) * inverseAlpha5) +
                                       (((srcPair >> 5) & 0x07c0f83fu) * alpha5)) &
                                   0xf81f07e0u;
    return lowTerms | highTerms;
}

/**
 * Recovered inline helper: zRndr 555 alpha pixel blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed across 0x49c560, 0x49ca90, 0x49cea0, 0x49d3b0, 0x49d950, and 0x49ddb0 alpha-map span callers.
 * Purpose: Blend one 555 destination pixel toward a source pixel using an 8-bit alpha value.
 */
static inline unsigned short BlendPixel555Alpha8(
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
 * Recovered inline helper: zRndr 555 alpha pair blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49c560 and 0x49d3b0 paired-pixel alpha blend loops.
 * Purpose: Blend two packed 555 destination pixels toward one source pixel using a 5-bit alpha approximation.
 */
static inline unsigned int BlendPair555Alpha5(
    unsigned int dstPair,
    unsigned short srcPixel,
    int alpha
) {
    const unsigned int srcPair = (unsigned int)(srcPixel) | ((unsigned int)(srcPixel) << 16);
    const unsigned int alpha5 = (unsigned int)(alpha >> 3);
    const unsigned int inverseAlpha5 = 0x1fu - alpha5;
    const unsigned int lowTerms =
        ((((dstPair & 0x03e07c1fu) * inverseAlpha5) + ((srcPair & 0x03e07c1fu) * alpha5)) >> 5) &
        0x03e07c1fu;
    const unsigned int highTerms = ((((dstPair >> 5) & 0x03e0f81fu) * inverseAlpha5) +
                                       (((srcPair >> 5) & 0x03e0f81fu) * alpha5)) &
                                   0x7c1f03e0u;
    return highTerms | lowTerms;
}

/**
 * Recovered inline helper: zRndr 555 constant-alpha-map pixel blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49ca90 and 0x49d950 scaled alpha-map span callers.
 * Purpose: Blend one 555 destination pixel toward a source pixel using a scaled alpha-map value.
 */
static inline unsigned short BlendPixel555ConstAlphaMap(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
) {
    const int dstColor = (short)(dstPixel);
    const int srcColor = srcPixel;
    const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
    const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
    const int blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * alpha) >> 8;
    return (unsigned short)(dstColor + (redDelta & 0xfffffc00) + (greenDelta & 0xffffffe0) +
                            blueDelta);
}

} // namespace

/**
 * Reimplements 0x49e200: zRndr::FogBlendSpan565Scalar
 * Purpose: Blend a 565 span with the active fog color using scalar pair processing.
 */
void __fastcall FogBlendSpan565Scalar(
    unsigned short *pixels,
    int pixelCount,
    int fogCoordFixed24,
    int fogCoordStepFixed24
) {
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);
    unsigned int pairCount = (unsigned int)(pixelCount) >> 1;

    if ((pixelCount & 1) != 0) {
        *pixels = FogBlendPixel565(
            *pixels,
            fogCoord
        );
        ++pixels;
        fogCoord += fogStep;
    }

    const unsigned int pairFogStep = fogStep + fogStep;
    while (pairCount != 0) {
        const unsigned int packedPixels =
            (unsigned int)(pixels[0]) | ((unsigned int)(pixels[1]) << 16);
        const unsigned int blended = FogBlendPair565(
            packedPixels,
            fogCoord
        );
        pixels[0] = (unsigned short)(blended);
        pixels[1] = (unsigned short)(blended >> 16);

        pixels += 2;
        fogCoord += pairFogStep;
        --pairCount;
    }
}

/**
 * Reimplements 0x49e300: zRndr::FogBlendSpan555Scalar
 * Purpose: Blend a 555 span with the active fog color using scalar pair processing.
 */
void __fastcall FogBlendSpan555Scalar(
    unsigned short *pixels,
    int pixelCount,
    int fogCoordFixed24,
    int fogCoordStepFixed24
) {
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);
    unsigned int pairCount = (unsigned int)(pixelCount) >> 1;

    if ((pixelCount & 1) != 0) {
        *pixels = FogBlendPixel555(
            *pixels,
            fogCoord
        );
        ++pixels;
        fogCoord += fogStep;
    }

    const unsigned int pairFogStep = fogStep + fogStep;
    while (pairCount != 0) {
        const unsigned int packedPixels =
            (unsigned int)(pixels[0]) | ((unsigned int)(pixels[1]) << 16);
        const unsigned int blended = FogBlendPair555(
            packedPixels,
            fogCoord
        );
        pixels[0] = (unsigned short)(blended);
        pixels[1] = (unsigned short)(blended >> 16);

        pixels += 2;
        fogCoord += pairFogStep;
        --pairCount;
    }
}

/**
 * Reimplements 0x49e400: zRndr::FogBlendSpan565Mmx
 * Purpose: Blend a 565 span through scalar edge handling and the MMX-shaped quad body.
 */
void __fastcall FogBlendSpan565Mmx(
    unsigned short *pixels,
    int pixelCount,
    int fogCoordFixed24,
    int fogCoordStepFixed24
) {
    unsigned short *cursor = pixels;
    int remaining = pixelCount;
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);

    int headPixels = (int)((unsigned int)(pixels) & 3u);
    if ((unsigned int)(headPixels) >= (unsigned int)(remaining)) {
        headPixels = remaining;
    }

    if (headPixels != 0) {
        FogBlendSpan565Scalar(
            cursor,
            headPixels,
            (int)(fogCoord),
            fogCoordStepFixed24
        );
        cursor += headPixels;
        fogCoord += (unsigned int)(headPixels)*fogStep;
        remaining -= headPixels;
    }

    const int tailPixels = remaining & 3;
    unsigned int quadCount = (unsigned int)(remaining) >> 2;
    while (quadCount != 0) {
        fogCoord += fogStep;
        g_mmxFogFactors[0] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[1] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[2] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[3] = (unsigned short)(fogCoord >> 16);

        cursor[0] =
            FogBlendMmxLane(
                cursor[0],
                g_mmxFogFactors[0],
                0,
                11,
                3
            );
        cursor[1] =
            FogBlendMmxLane(
                cursor[1],
                g_mmxFogFactors[1],
                1,
                11,
                3
            );
        cursor[2] =
            FogBlendMmxLane(
                cursor[2],
                g_mmxFogFactors[2],
                2,
                11,
                3
            );
        cursor[3] =
            FogBlendMmxLane(
                cursor[3],
                g_mmxFogFactors[3],
                3,
                11,
                3
            );

        fogCoord += fogStep;
        fogCoord += fogStep;
        cursor += 4;
        --quadCount;
    }

    if (tailPixels != 0) {
        FogBlendSpan565Scalar(
            cursor,
            tailPixels,
            (int)(fogCoord),
            fogCoordStepFixed24
        );
    }
}

/**
 * Reimplements 0x49e560: zRndr::FogBlendSpan555Mmx
 * Purpose: Blend a 555 span through scalar edge handling and the MMX-shaped quad body.
 */
void __fastcall FogBlendSpan555Mmx(
    unsigned short *pixels,
    int pixelCount,
    int fogCoordFixed24,
    int fogCoordStepFixed24
) {
    unsigned short *cursor = pixels;
    int remaining = pixelCount;
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);

    int headPixels = (int)((unsigned int)(pixels) & 3u);
    if ((unsigned int)(headPixels) >= (unsigned int)(remaining)) {
        headPixels = remaining;
    }

    if (headPixels != 0) {
        FogBlendSpan555Scalar(
            cursor,
            headPixels,
            (int)(fogCoord),
            fogCoordStepFixed24
        );
        cursor += headPixels;
        fogCoord += (unsigned int)(headPixels)*fogStep;
        remaining -= headPixels;
    }

    const int tailPixels = remaining & 3;
    unsigned int quadCount = (unsigned int)(remaining) >> 2;
    while (quadCount != 0) {
        fogCoord += fogStep;
        g_mmxFogFactors[0] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[1] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[2] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[3] = (unsigned short)(fogCoord >> 16);

        cursor[0] =
            FogBlendMmxLane(
                cursor[0],
                g_mmxFogFactors[0],
                0,
                10,
                2
            );
        cursor[1] =
            FogBlendMmxLane(
                cursor[1],
                g_mmxFogFactors[1],
                1,
                10,
                2
            );
        cursor[2] =
            FogBlendMmxLane(
                cursor[2],
                g_mmxFogFactors[2],
                2,
                10,
                2
            );
        cursor[3] =
            FogBlendMmxLane(
                cursor[3],
                g_mmxFogFactors[3],
                3,
                10,
                2
            );

        fogCoord += fogStep;
        fogCoord += fogStep;
        cursor += 4;
        --quadCount;
    }

    if (tailPixels != 0) {
        FogBlendSpan555Scalar(
            cursor,
            tailPixels,
            (int)(fogCoord),
            fogCoordStepFixed24
        );
    }
}

/**
 * Reimplements 0x49e6c0: zRndr::SpanCopy16FromTex16SwitchVShift
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source-shape evidence: BN assembly uses a texVShift 10..17 jump table, saves
 * real ESP in gRndr_SavedEspSlot, pivots ESP to gRndr_CurrentSpanBaseAddr +
 * count, samples gRndr_ActiveTexPixels as 16-bit texels, pushes every sampled
 * word backward, and restores ESP at case exit. This C++ body preserves
 * behavior while the retail ESP-pivot source shape remains owner/data debt for
 * the span-family pass; current VC5SP3 /O2 output emits normal pointer stores,
 * and production raw assembly is not an accepted span-source substitute.
 * Purpose: Copy 16-bit texels into the active span using the variable-texVShift reverse span contract.
 */
void __fastcall SpanCopy16FromTex16SwitchVShift(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    switch (texVShift) {
    default:
        return;

    case 10: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 11: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 12: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0xff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 13: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x7f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 14: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 15: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 16: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x0f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 17: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x07) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }
    }
}

/**
 * Reimplements 0x49b7e0: zRndr::SpanMasked16FromTex16SwitchVShift
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source-shape evidence: BN assembly uses a texVShift 10..17 jump table, saves
 * through gRndr_SavedEspSlot, pivots ESP to gRndr_CurrentSpanBaseAddr + count,
 * samples gRndr_ActiveTexPixels as 16-bit texels, and either pushes a nonzero
 * word or subtracts two bytes so zero texels leave the destination transparent.
 * This C++ body preserves behavior while the retail ESP-pivot source shape
 * remains owner/data debt for the span-family pass; the retained negative-byte
 * countdown mirrors BN's loop counter, but current VC5SP3 /O2 output still
 * emits normal pointer stores, and production raw assembly is not an accepted
 * span-source substitute.
 * Purpose: Write nonzero 16-bit texels into the active span using the texVShift-specialized reverse span loops.
 */
void __fastcall SpanMasked16FromTex16SwitchVShift(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    switch (texVShift) {
    default:
        return;

    case 10: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 11: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 12: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0xff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 13: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x7f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 14: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 15: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 16: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x0f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 17: {
        const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x07) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }
    }
}

/**
 * Reimplements 0x49ea40: zRndr::SpanMmxSetTexUvMasksAndVShift
 * Purpose: Mirror the active texture U/V masks and selected V shift into the two-lane MMX span globals.
 */
void __fastcall SpanMmxSetTexUvMasksAndVShift(
    int texVShift
) {
    const int texVMask = g_spanActiveTexVMask;
    g_mmxVShiftCounts.hi = 0;
    g_mmxVMask.hi = texVMask;
    g_mmxVMask.lo = texVMask;

    const int texUMask = g_spanActiveTexUMask << 20;
    g_mmxVShiftCounts.lo = texVShift;
    g_mmxUMask.hi = texUMask;
    g_mmxUMask.lo = texUMask;
}

/**
 * Reimplements 0x49ea80: zRndr::SpanCopy16FromTex16
 * Source-shape evidence: BN handles an optional unaligned leading texel, sets
 * paired MMX U/V and doubled-step scratch globals, samples two tex16 indices
 * per packed loop through the active MMX masks, then writes an odd tail texel.
 * This C++ body keeps the scalar edge samples and inlines the packed two-pixel
 * scalar loop while the retail MMX packed-index source model remains blocked.
 * Purpose: Copy a 16-bit textured span while priming the paired MMX U/V scratch records.
 */
void __fastcall SpanCopy16FromTex16(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    if (((unsigned int)(dst) & 3u) != 0) {
        const int sourceIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
        ++dst;
        --pixelCount;
        if (pixelCount == 0) {
            return;
        }
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
    }

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    int pairCount = pixelCount >> 1;
    while (pairCount != 0) {
        const int firstIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short first = texels16[firstIndex];
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;

        const int secondIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short second = texels16[secondIndex];
        *((unsigned int *)(dst)) = ((unsigned int)(second) << 16) | first;
        dst += 2;
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        --pairCount;
    }

    if ((pixelCount & 1) != 0) {
        const int sourceIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
    }
}

/**
 * Reimplements 0x49ec20: zRndr::SpanCopy16FromTex16ExplicitVShift
 * Source-shape evidence: BN matches the generic tex16 copy body with the
 * caller-supplied V shift feeding the MMX packed-index loop and odd tail. This
 * C++ body keeps the scalar edge samples and inlines the packed two-pixel
 * scalar loop while the retail MMX packed-index source model remains blocked.
 * Purpose: Copy a 16-bit textured span with the caller-supplied V shift and MMX U/V scratch records.
 */
void __fastcall SpanCopy16FromTex16ExplicitVShift(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    unsigned short *dst = g_spanCurrentSpanBaseAddr;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexPixels);
    if (((unsigned int)(dst) & 3u) != 0) {
        const int sourceIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
        ++dst;
        --pixelCount;
        if (pixelCount == 0) {
            return;
        }
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
    }

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    int pairCount = pixelCount >> 1;
    while (pairCount != 0) {
        const int firstIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short first = texels16[firstIndex];
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;

        const int secondIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short second = texels16[secondIndex];
        *((unsigned int *)(dst)) = ((unsigned int)(second) << 16) | first;
        dst += 2;
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        --pairCount;
    }

    if ((pixelCount & 1) != 0) {
        const int sourceIndex =
            ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) +
            ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
    }
}

/**
 * Reimplements 0x49edc0: zRndr::SpanCopy16FromPal8SwitchVShift
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source-shape evidence: BN assembly/HLIL shows a texVShift 10..17 jump table;
 * each case saves through gRndr_SavedEspSlot, pivots ESP to
 * gRndr_CurrentSpanBaseAddr + count, samples an 8-bit texel from
 * gRndr_ActiveTexPixels, expands it through gRndr_ActiveTexPalette, then pushes
 * the 16-bit palette word backward into the span. This C++ body models the same
 * case dispatch and descending writes while the shared retail pivot source shape
 * remains unresolved.
 * Purpose: Copy palettized texels into the active 16-bit span using the variable-texVShift reverse span contract.
 */
void __fastcall SpanCopy16FromPal8SwitchVShift(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    switch (texVShift) {
    default:
        return;

    case 10: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 11: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 12: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0xff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 13: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x7f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 14: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 15: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 16: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x0f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 17: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x07) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }
    }
}

/**
 * Reimplements 0x49bbf0: zRndr::SpanMasked16FromPal8SwitchVShift
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source-shape evidence: BN assembly/HLIL shows a texVShift 10..17 jump table;
 * each case saves through gRndr_SavedEspSlot, pivots ESP to
 * gRndr_CurrentSpanBaseAddr + count, samples an 8-bit texel from
 * gRndr_ActiveTexPixels, skips zero texels by reserving the destination word,
 * and expands nonzero texels through gRndr_ActiveTexPalette before pushing the
 * 16-bit palette word backward into the span. This C++ body models the same
 * case dispatch, transparent-zero skip, and descending writes while the shared
 * retail pivot source shape remains unresolved.
 * Purpose: Write nonzero palettized texels into the active 16-bit span using the variable-texVShift reverse span contract.
 */
void __fastcall SpanMasked16FromPal8SwitchVShift(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    switch (texVShift) {
    default:
        return;

    case 10: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 11: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 12: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0xff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 13: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x7f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 14: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 15: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 16: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x0f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 17: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x07) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }
    }
}

/**
 * Reimplements 0x49f180: zRndr::SpanShade16FromPal8SwitchVShift
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source-shape evidence: BN assembly/HLIL shows a texVShift 10..17 jump table;
 * each case saves through gRndr_SavedEspSlot, pivots ESP to
 * gRndr_CurrentSpanBaseAddr + count, samples an 8-bit texel from
 * gRndr_ActiveTexPixels, adds the current shade bucket from
 * gRndr_ActiveShadeFixed16, advances by gRndr_ActiveShadeStepFixed16, then
 * pushes the shade-adjusted 16-bit palette word backward into the span. This
 * C++ body models the same case dispatch and descending writes while the shared
 * retail pivot source shape remains unresolved.
 * Purpose: Shade palettized texels through the active palette and write them into the reverse active span.
 */
void __fastcall SpanShade16FromPal8SwitchVShift(
    int texU,
    int texV,
    int pixelCount,
    int texVShift
) {
    switch (texVShift) {
    default:
        return;

    case 10: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 11: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1ff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 12: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0xff) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 13: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x7f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 14: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x3f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 15: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x1f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 16: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x0f) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 17: {
        unsigned short *dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex =
                ((texU >> 20) & 0x07) +
                ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16 = (int)((unsigned int)(g_spanActiveShadeFixed16) +
                                             (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }
    }
}

/**
 * Reimplements 0x49b1e0: zRndr::FogColor_SetRgb01Clamped
 * Purpose: Clamp and commit the active fog color, then rebuild its packed 16-bit ramp.
 */
void __fastcall FogColor_SetRgb01Clamped(
    zColorRgb *color
) {
    if (color->red > 1.0f) {
        color->red = 1.0f;
    } else if (!(color->red >= 0.0f)) {
        color->red = 0.0f;
    }

    if (color->green > 1.0f) {
        color->green = 1.0f;
    } else if (!(color->green >= 0.0f)) {
        color->green = 0.0f;
    }

    if (color->blue > 1.0f) {
        color->blue = 1.0f;
    } else if (!(color->blue >= 0.0f)) {
        color->blue = 0.0f;
    }

    if (fabs(g_fogColorParams.colorRgb01[0] - color->red) < 0.01f &&
        fabs(g_fogColorParams.colorRgb01[1] - color->green) < 0.01f &&
        fabs(g_fogColorParams.colorRgb01[2] - color->blue) < 0.01f) {
        return;
    }

    g_fogColorParams.colorRgb01[0] = color->red;
    g_fogColorParams.colorRgb01[1] = color->green;
    g_fogColorParams.colorRgb01[2] = color->blue;

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat *)(color));
    }

    const int red = (int)(color->red * 255.0f + 0.5f);
    const int green = (int)(color->green * 255.0f + 0.5f);
    const unsigned int blue = (unsigned int)(color->blue * 255.0f + 0.5f);
    FogTarget565_SetPackedColorAndRamp(
        &g_fogColorParams,
        (red << g_zVideo_PixelPack.packedBase) & (int)(g_zVideo_PixelPack.rMask),
        (green << g_zVideo_PixelPack.sumMinus8) & (int)(g_zVideo_PixelPack.gMask),
        blue >> g_zVideo_PixelPack.bShiftTo8
    );
}

/**
 * Reimplements 0x49b350: zRndr::SetFogTargetColorRgb01Clamped
 * Purpose: Clamp and commit the immediate fog target color, then rebuild its packed 16-bit ramp.
 */
void __fastcall SetFogTargetColorRgb01Clamped(
    zColorRgb *color
) {
    if (color->red > 1.0f) {
        color->red = 1.0f;
    } else if (!(color->red >= 0.0f)) {
        color->red = 0.0f;
    }

    if (color->green > 1.0f) {
        color->green = 1.0f;
    } else if (!(color->green >= 0.0f)) {
        color->green = 0.0f;
    }

    if (color->blue > 1.0f) {
        color->blue = 1.0f;
    } else if (!(color->blue >= 0.0f)) {
        color->blue = 0.0f;
    }

    if (fabs(g_fogParamsActive.colorRgb01[0] - color->red) < 0.01f &&
        fabs(g_fogParamsActive.colorRgb01[1] - color->green) < 0.01f &&
        fabs(g_fogParamsActive.colorRgb01[2] - color->blue) < 0.01f) {
        return;
    }

    g_fogParamsActive.colorRgb01[0] = color->red;
    g_fogParamsActive.colorRgb01[1] = color->green;
    g_fogParamsActive.colorRgb01[2] = color->blue;

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo::SetFogTargetColorFromRgb01((zVideo_ColorRgbFloat *)(color));
    }

    const int red = (int)(color->red * 255.0f + 0.5f);
    const int green = (int)(color->green * 255.0f + 0.5f);
    const int blue = (int)(color->blue * 255.0f + 0.5f);
    FogTarget565_SetPackedColorAndRamp(
        &g_fogParamsActive,
        (red << g_zVideo_PixelPack.packedBase) & (int)(g_zVideo_PixelPack.rMask),
        (green << g_zVideo_PixelPack.sumMinus8) & (int)(g_zVideo_PixelPack.gMask),
        blue >> g_zVideo_PixelPack.bShiftTo8
    );
}

/**
 * Reimplements 0x49b4c0: zRndr::CommitDirectFogParamsIfChanged
 * Purpose: Copy direct fog target parameters into the active fog state when they differ.
 */
void CommitDirectFogParamsIfChanged() {
    CommitFogParamsIfChanged(g_fogTargetParamsDirect);
}

/**
 * Reimplements 0x49b530: zRndr::CommitFogColorParamsIfChanged
 * Purpose: Copy fog color parameters into the active fog state when they differ.
 */
void CommitFogColorParamsIfChanged() {
    CommitFogParamsIfChanged(g_fogColorParams);
}

/**
 * Reimplements 0x49b710: zRndr::CommitStagedFogParamsIfChanged
 * Purpose: Copy staged fog target parameters into the active fog state when they differ.
 */
void CommitStagedFogParamsIfChanged() {
    CommitFogParamsIfChanged(g_fogTargetParamsStaged);
}

/**
 * Reimplements 0x49b780: zRndr::BlendPackedColor565WithFogInPlace
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Fog.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Blend a packed 565 color in place toward the active fog color.
 */
void __fastcall BlendPackedColor565WithFogInPlace(
    int *ioPackedColor,
    int blend255
) {
    const int fogGreen = g_fogParamsActive.packedColorGreen;
    const int packedColor = *ioPackedColor;
    const int greenMask = (int)(g_pixelPackGreenMask);
    const int blueMask = (int)(g_pixelPackBlueMask);

    const int greenDelta =
        ((fogGreen - (greenMask & packedColor)) * blend255) >> 8;
    const int blueDelta =
        ((g_fogParamsActive.packedColorBlue - (blueMask & packedColor)) * blend255) >> 8;
    const int redMask = (int)(g_pixelPackRedMask);
    const int redDelta =
        ((g_fogParamsActive.packedColorRed - (redMask & packedColor)) * blend255) >> 8;

    int blendedColor = redDelta & redMask;
    blendedColor += blueDelta;
    blendedColor += greenDelta & greenMask;
    blendedColor += packedColor;

    *ioPackedColor = blendedColor;
}

/**
 * Reimplements 0x49a910: zRndr::LensFlare_ResetSampleQueue
 * Purpose: Reset the queued lens-flare sample count for the frame.
 */
void LensFlare_ResetSampleQueue() {
    g_lensFlareSampleQueueCount = 0;
}

namespace {
/**
 * Recovered inline helper: zRndr 565 lens-flare color blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x498cb0 as the 565 branch of the lens-flare pixel blend path.
 * Purpose: Blend one packed 565 color toward another using an 8-bit alpha value.
 */
static inline unsigned short BlendPacked565(
    unsigned short from,
    unsigned short to,
    int alpha
) {
    const int red =
        ((from >> 11) & 0x1f) + ((((to >> 11) & 0x1f) - ((from >> 11) & 0x1f)) * alpha >> 8);
    const int green =
        ((from >> 5) & 0x3f) + ((((to >> 5) & 0x3f) - ((from >> 5) & 0x3f)) * alpha >> 8);
    const int blue = (from & 0x1f) + (((to & 0x1f) - (from & 0x1f)) * alpha >> 8);
    return (unsigned short)(((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1f));
}

/**
 * Recovered inline helper: zRndr 555 lens-flare color blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x498cb0 as the 555 branch of the lens-flare pixel blend path.
 * Purpose: Blend one packed 555 color toward another using an 8-bit alpha value.
 */
static inline unsigned short BlendPacked555(
    unsigned short from,
    unsigned short to,
    int alpha
) {
    const int red =
        ((from >> 10) & 0x1f) + ((((to >> 10) & 0x1f) - ((from >> 10) & 0x1f)) * alpha >> 8);
    const int green =
        ((from >> 5) & 0x1f) + ((((to >> 5) & 0x1f) - ((from >> 5) & 0x1f)) * alpha >> 8);
    const int blue = (from & 0x1f) + (((to & 0x1f) - (from & 0x1f)) * alpha >> 8);
    return (unsigned short)(((red & 0x1f) << 10) | ((green & 0x1f) << 5) | (blue & 0x1f));
}

/**
 * Recovered inline helper: zRndr lens-flare pixel blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x498cb0 overlay/depth-fade paths and selected by the active 555/565 pixel-pack state.
 * Purpose: Blend a lens-flare pixel using the active 16-bit framebuffer packing.
 */
static inline unsigned short BlendLensFlarePixel(
    unsigned short from,
    unsigned short to,
    int alpha
) {
    if (g_pixelPackGreenBits == 6) {
        if (alpha <= 3) {
            return from;
        }
        if (alpha >= 0xfc) {
            return to;
        }
        return BlendPacked565(
            from,
            to,
            alpha
        );
    }

    if (alpha <= 7) {
        return from;
    }
    if (alpha >= 0xfc) {
        return to;
    }
    return BlendPacked555(
        from,
        to,
        alpha
    );
}
} // namespace

/**
 * Reimplements 0x498cb0: zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer
 * Purpose: Draw one queued lens-flare sample into the clipped 16-bit framebuffer.
 */
void __fastcall LensFlare_DrawQueuedSample16_ClippedFramebuffer(
    LensFlareSamplePartial *sample,
    int yOffsetPixels,
    float screenScale
) {
    if (sample == 0 || g_frameBuffer == 0) {
        return;
    }

    zRndr_LensFlareSource *lensFlareSource =
        (zRndr_LensFlareSource *)((unsigned int)(sample->lensFlareSource));
    bool blendTowardFramebuffer = false;
    float reciprocalZ = 0.0f;
    if (lensFlareSource != 0 && lensFlareSource->depthFadeInvZMax != 0.0f) {
        if (sample->reciprocalZ == 0.0f) {
            return;
        }

        reciprocalZ = 1.0f / sample->reciprocalZ;
        if (reciprocalZ >= lensFlareSource->depthFadeInvZMax) {
            return;
        }

        blendTowardFramebuffer = reciprocalZ > lensFlareSource->depthFadeInvZMin;
    }

    const int y = (int)(sample->y * screenScale) + yOffsetPixels;
    const int x = (int)(sample->x * screenScale);
    if (x < 0 || y < 0 || x > g_activeRegionWidth || y > g_activeRegionHeight) {
        return;
    }

    unsigned short packedColor = (unsigned short)(sample->packedColor16);
    unsigned char *frameBase = (unsigned char *)(g_frameBuffer);
    unsigned short *pixel =
        (unsigned short *)(frameBase + (int)(y)*g_pitchBytes + (int)(x) * sizeof(unsigned short));

    if (g_overlayBlendEnabled != 0) {
        const int overlayAlpha = (int)(g_overlayBlendAlpha * 255.0);
        packedColor = BlendLensFlarePixel(
            packedColor,
            (unsigned short)(g_overlayBlendPackedColor16),
            overlayAlpha
        );
    }

    if (!blendTowardFramebuffer) {
        *pixel = packedColor;
        return;
    }

    const int fadeAlpha =
        (int)((lensFlareSource->depthFadeInvZMax - reciprocalZ) * lensFlareSource->depthFadeScale);
    if (g_pixelPackGreenBits == 6 ? fadeAlpha <= 3 : fadeAlpha <= 7) {
        return;
    }

    if (fadeAlpha >= 0xfc) {
        *pixel = packedColor;
        return;
    }

    *pixel = BlendLensFlarePixel(
        *pixel,
        packedColor,
        fadeAlpha
    );
}

/**
 * Reimplements 0x49a8c0: zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer
 * Source file evidence: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Draw every queued lens-flare sample with a shared screen scale and Y offset.
 */
void __fastcall LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer(
    int yOffsetPixels,
    float screenScale
) {
    {
        for (int sampleIndex = 0; sampleIndex < g_lensFlareSampleQueueCount; ++sampleIndex) {
            LensFlare_DrawQueuedSample16_ClippedFramebuffer(
                &g_lensFlareSampleQueue[sampleIndex],
                yOffsetPixels,
                screenScale
            );
        }
    }

    g_lensFlareSampleQueueCount = 0;
    g_overlayBlendEnabled = 0;
}

/**
 * Recovered helper: SpanOcclusionInsertPendingSpanSorted.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects the sorted insertion helper.
 * Purpose: Route span-occlusion dispatch to the sorted pending-span insertion helper.
 */
void SpanOcclusionInsertPendingSpanSorted(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    InsertPendingSpanSorted(
        spanList,
        columnIndex,
        spanCount
    );
}

/**
 * Recovered helper: SpanOcclusionInsertPendingSpanWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects depth-tested insertion.
 * Purpose: Route span-occlusion dispatch to the depth-tested pending-span insertion helper.
 */
void SpanOcclusionInsertPendingSpanWithDepthTest(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    InsertPendingSpanWithDepthTest(
        spanList,
        columnIndex,
        spanCount
    );
}

/**
 * Recovered helper: SpanOcclusionInsertPendingSpanNoDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects non-depth insertion.
 * Purpose: Route span-occlusion dispatch to the non-depth pending-span insertion helper.
 */
void SpanOcclusionInsertPendingSpanNoDepthTest(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    InsertPendingSpanNoDepthTest(
        spanList,
        columnIndex,
        spanCount
    );
}

/**
 * Recovered helper: SpanOcclusionBuildVisibleSpanListWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects visibility-list building.
 * Purpose: Route span-occlusion dispatch to the depth-tested visible-span list builder.
 */
void SpanOcclusionBuildVisibleSpanListWithDepthTest(
    SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    BuildVisibleSpanListWithDepthTest(
        spanList,
        columnIndex,
        spanCount
    );
}
} // namespace zRndr

/**
 * Recovered helper: zRndrSpanDepthAtXByPartsLocal.
 * Original shape: no standalone retail function is currently identified in the
 * inspected BN/plan evidence.
 * Purpose: evaluate a span node's interpolated inverse depth at one x sample.
 *
 * Original helper evidence: source-faithful helper recovered from repeated
 * span-occlusion caller bodies including 0x4907c0 and visibility helpers,
 * which all compute invDepth + (x - sampleXMin) * depthSlope from
 * zRndr_SpanNode fields.
 */
static float zRndrSpanDepthAtXByPartsLocal(
    int sampleXMin,
    float invDepth,
    float depthSlope,
    int x
) {
    return invDepth + (float)(x - sampleXMin) * depthSlope;
}

/**
 * Reimplements 0x490ae0: zRndr_SpanOcclusion_InsertSpanNode_Local.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: insert the pending span into a column using depth-tested occlusion
 * splitting.
 *
 * Evidence: BN identifies this as the callback installed in
 * gRndr_pfnBuildSpanList; the wrapper forwards spanList, columnIndex, and
 * spanCount into the recovered depth-tested insertion helper.
 */
void __fastcall zRndr_SpanOcclusion_InsertSpanNode_Local(
    zRndr::SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    using namespace zRndr;

    SpanNodePartial **columnHeadTable = g_spanColumnHeadTable;
    SpanNodePartial *current = columnHeadTable[columnIndex];
    SpanNodePartial *pending = g_spanAllocCursor;
    *spanCount = 0;

    if (current == 0 || pending->sampleXMax < current->sampleXMin) {
        pending->next = current;
        columnHeadTable[columnIndex] = pending;
        spanList[*spanCount] = pending;
        ++*spanCount;
        ++g_spanAllocCursor;
        return;
    }

    pending->next = 0;
    g_spanIterNode = current;
    g_spanIterPrevLink = 0;

    SpanNodePartial *previous = 0;
    while (current != 0) {
        previous = g_spanIterPrevLink;
        current = g_spanIterNode;
        while (current != 0 && pending->sampleXMin > current->sampleXMax) {
            previous = current;
            current = current->next;
        }

        if (current == 0) {
            g_spanIterNode = current;
            g_spanIterPrevLink = previous;
            break;
        }

        if (pending->sampleXMax < current->sampleXMin) {
            g_spanIterNode = current;
            g_spanIterPrevLink = previous;
            break;
        }

        const bool pendingInFront =
            zRndr_SpanOcclusion_TestSpanDepthOrderPair(
                pending,
                current
            ) != 0;

        const int pendingMin = pending->sampleXMin;
        const int pendingMax = pending->sampleXMax;
        const float pendingInvDepth = pending->invDepth;
        const float pendingInvDepthStep = pending->invDepthStep;
        const float pendingDepthSlope = pending->depthSlope;

        if (pendingInFront) {
            const int currentMin = current->sampleXMin;
            const int currentMax = current->sampleXMax;
            const float currentInvDepth = current->invDepth;
            const float currentInvDepthStep = current->invDepthStep;
            const float currentDepthSlope = current->depthSlope;

            if (currentMin < pendingMin) {
                current->sampleXMax = pendingMin - 1;
                current->invDepthStep = SpanDepthAtX(
                    currentMin,
                    currentInvDepth,
                    currentDepthSlope,
                    current->sampleXMax
                );

                if (currentMax > pendingMax) {
                    SpanNodePartial *rightSplit = pending + 1;
                    rightSplit->sampleXMin = pendingMax + 1;
                    rightSplit->sampleXMax = currentMax;
                    rightSplit->invDepth = SpanDepthAtX(
                        currentMin,
                        currentInvDepth,
                        currentDepthSlope,
                        rightSplit->sampleXMin
                    );
                    rightSplit->invDepthStep = currentInvDepthStep;
                    rightSplit->depthSlope = currentDepthSlope;
                    rightSplit->next = current->next;

                    pending->next = rightSplit;
                    current->next = pending;
                    AppendSpanListNode(
                        spanList,
                        spanCount,
                        pending
                    );
                    g_spanIterPrevLink = current;
                    g_spanIterNode = pending;
                    g_spanLastNode = rightSplit;
                    g_spanAllocCursor += 2;
                    return;
                }

                previous = current;
                current = current->next;
                continue;
            }

            if (currentMax <= pendingMax) {
                SpanNodePartial *next = current->next;
                if (previous != 0) {
                    previous->next = next;
                } else {
                    g_spanColumnHeadTable[columnIndex] = next;
                }
                current = next;
                continue;
            }

            current->sampleXMin = pendingMax + 1;
            current->invDepth =
                SpanDepthAtX(
                    currentMin,
                    currentInvDepth,
                    currentDepthSlope,
                    current->sampleXMin
                );
            break;
        }

        if (current->sampleXMin <= pendingMin) {
            if (current->sampleXMax >= pendingMax) {
                return;
            }

            if (current->sampleXMax >= pendingMin) {
                pending->sampleXMin = current->sampleXMax + 1;
                pending->invDepth = SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    pending->sampleXMin
                );
            }

            previous = current;
            current = current->next;
            continue;
        }

        if (current->sampleXMin <= pendingMax) {
            const int leftMax = current->sampleXMin - 1;
            pending->sampleXMax = leftMax;
            pending->invDepthStep =
                SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    leftMax
                );
            LinkSpanNode(
                columnIndex,
                previous,
                pending,
                current
            );
            AppendSpanListNode(
                spanList,
                spanCount,
                pending
            );
            ++g_spanAllocCursor;

            if (current->sampleXMax >= pendingMax) {
                return;
            }

            previous = current;
            pending = g_spanAllocCursor;
            pending->next = 0;
            pending->sampleXMin = current->sampleXMax + 1;
            pending->sampleXMax = pendingMax;
            pending->invDepth =
                SpanDepthAtX(
                    pendingMin,
                    pendingInvDepth,
                    pendingDepthSlope,
                    pending->sampleXMin
                );
            pending->invDepthStep = pendingInvDepthStep;
            pending->depthSlope = pendingDepthSlope;
            current = current->next;
            continue;
        }

        previous = current;
        current = current->next;
    }

    LinkSpanNode(
        columnIndex,
        previous,
        pending,
        current
    );
    AppendSpanListNode(
        spanList,
        spanCount,
        pending
    );
    ++g_spanAllocCursor;
}

/**
 * Reimplements 0x4912a0: zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: insert the pending span into a column without depth-order testing.
 *
 * Evidence: BN keeps the same fastcall callback shape as the local inserter and
 * forwards into the no-depth insertion helper that preserves the column-list
 * split/visible-span contract.
 */
void __fastcall zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest(
    zRndr::SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    zRndr::SpanOcclusionInsertPendingSpanNoDepthTest(
        spanList,
        columnIndex,
        spanCount
    );
}

/**
 * Reimplements 0x491840: zRndr_SpanOcclusion_BuildSpanList.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: build visible fragments for one pending span against the current
 * column's occlusion list.
 *
 * Evidence: BN identifies this as the secondary span-list callback installed in
 * gRndr_pfnBuildSpanListSecondary; it forwards the callback arguments into the
 * recovered depth-tested visible-span builder.
 */
void __fastcall zRndr_SpanOcclusion_BuildSpanList(
    zRndr::SpanNodePartial **spanList,
    int columnIndex,
    int *spanCount
) {
    zRndr::SpanOcclusionBuildVisibleSpanListWithDepthTest(
        spanList,
        columnIndex,
        spanCount
    );
}

/**
 * Reimplements 0x491da0: zRndr_SpanOcclusion_BuildSpanListFast.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: emit the pending span as the only visible span and advance the span
 * allocation cursor.
 *
 * Evidence: BN writes null next, stores gRndr_SpanAllocCursor into spanList[0],
 * writes spanCount = 1, increments the cursor by one zRndr_SpanNode, and
 * returns.
 */
void __fastcall zRndr_SpanOcclusion_BuildSpanListFast(
    zRndr::SpanNodePartial **spanList,
    int,
    int *spanCount
) {
    zRndr::g_spanAllocCursor->next = 0;
    spanList[0] = zRndr::g_spanAllocCursor;
    *spanCount = 1;
    ++zRndr::g_spanAllocCursor;
}

/**
 * Reimplements 0x491dd0: zRndr_SpanOcclusion_TestColumnVisibility.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: test whether the pending span node remains visible in one occlusion
 * column.
 *
 * Evidence: BN validates null/out-of-range guards, copies
 * gRndr_SpanAllocCursor into a local candidate span, walks the column head
 * list, uses zRndr_SpanOcclusion_TestSpanDepthOrderPair for overlap depth
 * decisions, trims candidate ranges, and writes the out visibility flag.
 */
void __fastcall zRndr_SpanOcclusion_TestColumnVisibility(
    int columnIndex,
    int *isVisible
) {
    *isVisible = 0;
    if (zRndr::g_spanColumnHeadTable == 0 || zRndr::g_spanAllocCursor == 0 || columnIndex < 0 ||
        columnIndex >= zRndr::g_spanColumnCountPadded) {
        return;
    }

    zRndr::SpanNodePartial candidate = *zRndr::g_spanAllocCursor;
    candidate.next = 0;

    zRndr::SpanNodePartial *current = zRndr::g_spanColumnHeadTable[columnIndex];
    if (current == 0 || candidate.sampleXMax < current->sampleXMin) {
        *isVisible = 1;
        return;
    }

    while (current != 0) {
        while (current != 0 && candidate.sampleXMin > current->sampleXMax) {
            current = current->next;
        }

        if (current == 0 || candidate.sampleXMax < current->sampleXMin) {
            *isVisible = 1;
            return;
        }

        zRndr::SpanNodePartial occluder = *current;
        if (zRndr_SpanOcclusion_TestSpanDepthOrderPair(
            &candidate,
            &occluder
        ) != 0) {
            *isVisible = 1;
            return;
        }

        if (occluder.sampleXMin <= candidate.sampleXMin) {
            if (occluder.sampleXMax >= candidate.sampleXMax) {
                return;
            }

            if (occluder.sampleXMax >= candidate.sampleXMin) {
                candidate.sampleXMin = occluder.sampleXMax + 1;
                candidate.invDepth = zRndrSpanDepthAtXByPartsLocal(
                    zRndr::g_spanAllocCursor->sampleXMin,
                    zRndr::g_spanAllocCursor->invDepth,
                    zRndr::g_spanAllocCursor->depthSlope,
                    candidate.sampleXMin
                );
            }
        } else if (occluder.sampleXMin <= candidate.sampleXMax) {
            *isVisible = 1;
            return;
        }

        current = current->next;
    }

    *isVisible = 1;
}

/**
 * Reimplements 0x498c40: zRndr_SpanOcclusion_TestPointVisibility.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: stage one projected point as a single-pixel pending span and test
 * column visibility.
 *
 * Evidence: BN writes samplePoint z/x fields into gRndr_SpanAllocCursor,
 * truncates x/y through the original integer conversion path, calls
 * zRndr_SpanOcclusion_TestColumnVisibility, and returns one only when visible.
 */
int __fastcall zRndr_SpanOcclusion_TestPointVisibility(
    zVec3 *samplePoint
) {
    if (samplePoint == 0 || zRndr::g_spanAllocCursor == 0) {
        return 0;
    }

    zRndr::g_spanAllocCursor->invDepth = samplePoint->z;
    zRndr::g_spanAllocCursor->invDepthStep = samplePoint->z;
    zRndr::g_spanAllocCursor->depthSlope = 0.0f;
    zRndr::g_spanAllocCursor->sampleXMin = (int)(samplePoint->x);
    zRndr::g_spanAllocCursor->sampleXMax = zRndr::g_spanAllocCursor->sampleXMin;

    int isVisible = 0;
    zRndr_SpanOcclusion_TestColumnVisibility(
        (int)(samplePoint->y),
        &isVisible
    );
    return isVisible > 0 ? 1 : 0;
}

/**
 * Reimplements 0x498f90: zRndr_SpanOcclusion_TestSample.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: dispatch one visible sample point through the active zRndr point
 * operation.
 *
 * Evidence: BN loads gRndr_pFrameBuffer and gRndr_pfnPointOpActive, passes y/x
 * and color16 in the observed fastcall/stack shape, and performs no additional
 * span state updates.
 */
void __fastcall zRndr_SpanOcclusion_TestSample(
    int x,
    int y,
    int color16
) {
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        y,
        x,
        color16
    );
}

/**
 * Reimplements 0x499020: zRndr_DrawCircleOctants16_Framebuffer.
 * Original file: zRndr_Draw.cpp.
 * Purpose: emit the eight symmetric framebuffer points for one circle-outline
 * midpoint step.
 *
 * Evidence: BN reads g_zRndr_CircleCenterX/Y, forwards gRndr_pFrameBuffer,
 * and calls gRndr_pfnPointOpActive for each octant using fastcall y/x inputs
 * plus the caller-supplied packed color.
 */
void __fastcall zRndr_DrawCircleOctants16_Framebuffer(
    int y,
    int x,
    int packedColor
) {
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY + y,
        g_zRndr_CircleCenterX + x,
        packedColor
    );
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY + y,
        g_zRndr_CircleCenterX - x,
        packedColor
    );
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY - y,
        g_zRndr_CircleCenterX + x,
        packedColor
    );
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY - y,
        g_zRndr_CircleCenterX - x,
        packedColor
    );
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY + x,
        g_zRndr_CircleCenterX + y,
        packedColor
    );
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY + x,
        g_zRndr_CircleCenterX - y,
        packedColor
    );
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY - x,
        g_zRndr_CircleCenterX - y,
        packedColor
    );
    zRndr::g_pfnPointOpActive(
        zRndr::g_frameBuffer,
        g_zRndr_CircleCenterY - x,
        g_zRndr_CircleCenterX + y,
        packedColor
    );
}

/**
 * Reimplements 0x498fb0: zRndr_DrawCircleOutline16_Framebuffer.
 * Original file: zRndr_Draw.cpp.
 * Purpose: draw a 16-bit framebuffer circle outline through midpoint octant
 * batches.
 *
 * Evidence: BN stores the circle center and auxiliary argument globals, skips
 * non-positive radius values, dispatches the initial y=0 octants, then advances
 * the midpoint decision variable until x <= y.
 */
void __fastcall zRndr_DrawCircleOutline16_Framebuffer(
    int centerX,
    int centerY,
    int radius,
    int packedColor,
    int auxArg
) {
    int x = radius;
    int y = 0;
    int decisionVar = 1 - x;
    if (x <= 0) {
        return;
    }

    g_zRndr_CircleCenterX = centerX;
    g_zRndr_CircleCenterY = centerY;
    g_zRndr_CircleDrawAuxArg = auxArg;

    zRndr_DrawCircleOctants16_Framebuffer(
        0,
        x,
        packedColor
    );
    do {
        if (decisionVar < 0) {
            decisionVar += (y << 1) + 3;
        } else {
            decisionVar += ((y - x) << 1) + 5;
            --x;
        }

        ++y;
        zRndr_DrawCircleOctants16_Framebuffer(
            y,
            x,
            packedColor
        );
    } while (x > y);
}

/**
 * Reimplements 0x4992b0: zRndr_PlotPixel16
 * Purpose: Plot one 16-bit pixel into the active framebuffer row pitch.
 */
void __fastcall zRndr_PlotPixel16(
    unsigned short *dstPixels,
    int y,
    int x,
    int color16
) {
    const unsigned int pitchWords = (unsigned int)(zRndr::g_pitchBytes) >> 1;
    dstPixels[pitchWords * y + x] = (unsigned short)(color16);
}

/**
 * Reimplements 0x4992d0: zRndr_DrawLine16
 * Purpose: Rasterize an unclipped 16-bit Bresenham line into the active framebuffer.
 */
void __fastcall zRndr_DrawLine16(
    unsigned short *dstPixels,
    int x0,
    int y0,
    int x1,
    int y1,
    int color16
) {
    const unsigned int pitchWordsUnsigned = ((unsigned int)zRndr::g_pitchBytes) >> 1;
    int rowStep = (int)(pitchWordsUnsigned);
    int startIndex = (int)(pitchWordsUnsigned * y0 + x0);

    int dy = y1 - y0;
    if (dy < 0) {
        dy = -dy;
        rowStep = -rowStep;
    }

    int dx = x1 - x0;
    int xStep = 1;
    if (dx < 0) {
        dx = -dx;
        xStep = -1;
    }

    if (dx > dy) {
        unsigned short *cursor = &dstPixels[startIndex];
        const unsigned short packedColor = (unsigned short)(color16);
        int error = dx >> 1;
        int count = dx + 1;
        do {
            *cursor = packedColor;
            error += dy;
            cursor += xStep;
            if (error > dx) {
                error -= dx;
                cursor += rowStep;
            }
            --count;
        } while (count != 0);
        return;
    }

    unsigned short *cursor = &dstPixels[startIndex];
    const unsigned short packedColor = (unsigned short)(color16);
    int error = dy >> 1;
    int count = dy + 1;
    do {
        *cursor = packedColor;
        error += dx;
        cursor += rowStep;
        if (error > dy) {
            error -= dy;
            cursor += xStep;
        }
        --count;
    } while (count != 0);
}

/**
 * Reimplements 0x4993a0: zRndr_DrawLine16_Segmented
 * Purpose: Rasterize a segmented 16-bit Bresenham line into the active framebuffer.
 */
void __fastcall zRndr_DrawLine16_Segmented(
    unsigned short *dstPixels,
    int x0,
    int y0,
    int x1,
    int y1,
    int color16,
    int segmentCount
) {
    const unsigned int pitchWordsUnsigned = (unsigned int)(zRndr::g_pitchBytes) >> 1;
    int rowStep = (int)(pitchWordsUnsigned);
    int drawSegment = 1;
    int startIndex = (int)(pitchWordsUnsigned * y0 + x0);

    int dy = y1 - y0;
    if (dy < 0) {
        dy = -dy;
        rowStep = -rowStep;
    }

    int dx = x1 - x0;
    int xStep = 1;
    if (dx < 0) {
        dx = -dx;
        xStep = -1;
    }

    const unsigned short packedColor = (unsigned short)(color16);
    int segmentCounter = 0;

    // Retail VC5 reuses the consumed segmentCount argument slot for the branch segment limit.
    if (dx > dy) {
        segmentCount = (dx + 1) / segmentCount;
        int error = dx >> 1;
        int count = dx + 1;
        unsigned short *cursor = &dstPixels[startIndex];
        do {
            if (drawSegment != 0) {
                *cursor = packedColor;
            }

            error += dy;
            cursor += xStep;
            if (error > dx) {
                error -= dx;
                cursor += rowStep;
            }

            if (segmentCounter++ >= segmentCount) {
                segmentCounter = 0;
                drawSegment = drawSegment == 0 ? 1 : 0;
            }

            --count;
        } while (count != 0);
        return;
    }

    segmentCount = (dy + 1) / segmentCount;
    int error = dy >> 1;
    int count = dy + 1;
    unsigned short *cursor = &dstPixels[startIndex];
    do {
        if (drawSegment != 0) {
            *cursor = packedColor;
        }

        error += dx;
        cursor += rowStep;
        if (error > dy) {
            error -= dy;
            cursor += xStep;
        }

        if (segmentCounter++ >= segmentCount) {
            segmentCounter = 0;
            drawSegment = drawSegment == 0 ? 1 : 0;
        }

        --count;
    } while (count != 0);
}

/**
 * Reimplements 0x499500: zRndr_DrawLine16_Clipped
 * Purpose: Clip and rasterize a 16-bit line into the active framebuffer.
 */
void __fastcall zRndr_DrawLine16_Clipped(
    unsigned short *dstPixels,
    const zRndr_LineClipRect2I *clipRect,
    int x0,
    int y0,
    int x1,
    int y1,
    int color16
) {
    int outcode0 = 0;
    if (x0 < clipRect->left) {
        outcode0 = 1;
    } else if (x0 > clipRect->right) {
        outcode0 = 2;
    }

    if (y0 < clipRect->top) {
        outcode0 |= 4;
    } else if (y0 > clipRect->bottom) {
        outcode0 |= 8;
    }

    int outcode1 = 0;
    if (x1 < clipRect->left) {
        outcode1 = 1;
    } else if (x1 > clipRect->right) {
        outcode1 = 2;
    }

    if (y1 < clipRect->top) {
        outcode1 |= 4;
    } else if (y1 > clipRect->bottom) {
        outcode1 |= 8;
    }

    if ((outcode0 & outcode1) != 0) {
        return;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    if ((outcode0 | outcode1) != 0) {
        const float yPerX = dx != 0 ? (float)(dy) / (float)(dx) : 0.0f;
        const float xPerY = dy != 0 ? (float)(dx) / (float)(dy) : 0.0f;

        if (x0 < clipRect->left) {
            y0 += (int)((float)(clipRect->left - x0) * yPerX);
            x0 = clipRect->left;
        } else if (x0 > clipRect->right) {
            y0 += (int)((float)(clipRect->right - x0) * yPerX);
            x0 = clipRect->right;
        }

        if (x1 < clipRect->left) {
            y1 += (int)((float)(clipRect->left - x1) * yPerX);
            x1 = clipRect->left;
        } else if (x1 > clipRect->right) {
            y1 += (int)((float)(clipRect->right - x1) * yPerX);
            x1 = clipRect->right;
        }

        if (y0 < clipRect->top) {
            if (y1 < clipRect->top) {
                return;
            }
            x0 += (int)((float)(clipRect->top - y0) * xPerY);
            y0 = clipRect->top;
        } else if (y0 > clipRect->bottom) {
            if (y1 > clipRect->bottom) {
                return;
            }
            x0 += (int)((float)(clipRect->bottom - y0) * xPerY);
            y0 = clipRect->bottom;
        }

        if (y1 < clipRect->top) {
            x1 += (int)((float)(clipRect->top - y1) * xPerY);
            y1 = clipRect->top;
        } else if (y1 > clipRect->bottom) {
            x1 += (int)((float)(clipRect->bottom - y1) * xPerY);
            y1 = clipRect->bottom;
        }

        dx = x1 - x0;
        dy = y1 - y0;
    }

    const unsigned int pitchWordsUnsigned = (unsigned int)(zRndr::g_pitchBytes) >> 1;
    int rowStep = (int)(pitchWordsUnsigned);
    int startIndex = (int)(pitchWordsUnsigned * y0 + x0);

    if (dy < 0) {
        dy = -dy;
        rowStep = -rowStep;
    }

    int xStep = 1;
    if (dx < 0) {
        dx = -dx;
        xStep = -1;
    }

    unsigned short *cursor = &dstPixels[startIndex];
    const unsigned short packedColor = (unsigned short)(color16);

    if (dx > dy) {
        int error = dx >> 1;
        int count = dx + 1;
        do {
            *cursor = packedColor;
            error += dy;
            cursor += xStep;
            if (error > dx) {
                error -= dx;
                cursor += rowStep;
            }
            --count;
        } while (count != 0);
        return;
    }

    int error = dy >> 1;
    int count = dy + 1;
    do {
        *cursor = packedColor;
        error += dx;
        cursor += rowStep;
        if (error > dy) {
            error -= dy;
            cursor += xStep;
        }
        --count;
    } while (count != 0);
}

/**
 * Reimplements 0x4997d0: zRndr_FillSpan16Opaque
 * Purpose: Fill the active reverse span with one opaque 16-bit color.
 *
 * Evidence: BN pivots ESP from gRndr_CurrentSpanBaseAddr and writes the span
 * backward with push ax/eax after forming a packed two-pixel value. This C++
 * body models the odd-word and paired-dword write order while the retail
 * ESP-pivot source-family shape remains unresolved.
 */
void __fastcall zRndr_FillSpan16Opaque(
    int packedColor16,
    int pixelCount
) {
    const unsigned int packedPair = ((unsigned int)(packedColor16) << 16) | (unsigned int)(packedColor16);
    unsigned short *cursor = zRndr::g_spanCurrentSpanBaseAddr + pixelCount;

    if ((pixelCount & 1) != 0) {
        --cursor;
        *cursor = (unsigned short)(packedColor16);
    }

    unsigned int pairCount = (unsigned int)(pixelCount) >> 1;
    if ((pairCount & 1) != 0) {
        cursor -= 2;
        *((unsigned int *)(cursor)) = packedPair;
    }

    pairCount >>= 1;
    while (pairCount != 0) {
        cursor -= 2;
        *((unsigned int *)(cursor)) = packedPair;
        cursor -= 2;
        *((unsigned int *)(cursor)) = packedPair;
        --pairCount;
    }
}

/**
 * Reimplements 0x499810: zRndr_FillSpan555Solid
 * Purpose: Blend a solid color into the active 555 span using the supplied alpha.
 *
 * Evidence: BN uses gRndr_CurrentSpanBaseAddr as an ordinary word pointer for
 * this solid-fill leaf; there is no ESP-pivot write shape here.
 */
void __fastcall zRndr_FillSpan555Solid(
    int packedColor16,
    int blendAlpha,
    int pixelCount
) {
    unsigned short *cursor = zRndr::g_spanCurrentSpanBaseAddr;
    do {
        if (blendAlpha > 7) {
            if (blendAlpha >= 0xfc) {
                *cursor = (unsigned short)(packedColor16);
            } else {
                int dst = (short)(*cursor);
                int greenDelta = (packedColor16 & 0x03e0) - (dst & 0x03e0);
                greenDelta *= blendAlpha;
                int redDelta = (packedColor16 & 0x7c00) - (dst & 0x7c00);
                redDelta *= blendAlpha;
                redDelta = (redDelta >> 8) & 0xfffffc00;
                const int redAdjusted = dst + redDelta;
                int blueDelta = (packedColor16 & 0x001f) - (dst & 0x001f);
                blueDelta *= blendAlpha;
                greenDelta = (greenDelta >> 8) & 0xffffffe0;
                blueDelta >>= 8;
                *cursor = (unsigned short)(redAdjusted + blueDelta + greenDelta);
            }
        }

        ++cursor;
        --pixelCount;
    } while (pixelCount != 0);
}

/**
 * Reimplements 0x4998a0: zRndr_FillSpan565Solid
 * Purpose: Blend a solid color into the active 565 span using the supplied alpha.
 *
 * Evidence: BN uses gRndr_CurrentSpanBaseAddr as an ordinary word pointer here;
 * the limited reconstruction marker records only BN's partial-register display.
 */
void __fastcall zRndr_FillSpan565Solid(
    int packedColor16,
    int blendAlpha,
    int pixelCount
) {
    unsigned short *cursor = zRndr::g_spanCurrentSpanBaseAddr;
    do {
        if (blendAlpha > 3) {
            if (blendAlpha >= 0xfc) {
                *cursor = (unsigned short)(packedColor16);
            } else {
                const int dst = (short)(*cursor);
                int greenDelta = (packedColor16 & 0x07e0) - (dst & 0x07e0);
                greenDelta *= blendAlpha;
                int redDelta = (packedColor16 & 0xf800) - (dst & 0xf800);
                redDelta *= blendAlpha;
                redDelta = (redDelta >> 8) & 0xfffff800;
                const int redAdjusted = dst + redDelta;
                int blueDelta = (packedColor16 & 0x001f) - (redAdjusted & 0x001f);
                blueDelta *= blendAlpha;
                greenDelta = (greenDelta >> 8) & 0xffffffe0;
                blueDelta >>= 8;
                *cursor = (unsigned short)(redAdjusted + blueDelta + greenDelta);
            }
        }

        ++cursor;
        --pixelCount;
    } while (pixelCount != 0);
}

/**
 * Reimplements 0x4907c0: zRndr_SpanOcclusion_TestSpanDepthOrderPair.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: decide whether one overlapping span node is in front of another
 * using the recovered inverse-depth bias thresholds.
 *
 * Evidence: BN evaluates interpolated inverse-depth values at endpoint and
 * overlap samples through zRndr_SpanNode fields, compares against
 * gRndr_SpanDepthBiasPlusOne and gRndr_SpanDepthBiasPlusOneInv, and returns the
 * depth-order predicate used by span-occlusion insertion and visibility tests.
 */
int __fastcall zRndr_SpanOcclusion_TestSpanDepthOrderPair(
    zRndr::SpanNodePartial *lhs,
    zRndr::SpanNodePartial *rhs
) {
    if (lhs == 0 || rhs == 0) {
        return 0;
    }

    if (rhs->sampleXMin == rhs->sampleXMax) {
        const float lhsDepth = zRndrSpanDepthAtXByPartsLocal(
            lhs->sampleXMin,
            lhs->invDepth,
            lhs->depthSlope,
            rhs->sampleXMax
        );
        return lhsDepth * zRndr::g_spanDepthBiasPlusOne >= rhs->invDepth ? 1 : 0;
    }

    if (lhs->sampleXMin == lhs->sampleXMax) {
        const float rhsDepth = zRndrSpanDepthAtXByPartsLocal(
            rhs->sampleXMin,
            rhs->invDepth,
            rhs->depthSlope,
            lhs->sampleXMax
        );
        return lhs->invDepth >= rhsDepth * zRndr::g_spanDepthBiasPlusOneInv ? 1 : 0;
    }

    const int overlapMin = MaxValue(
        lhs->sampleXMin,
        rhs->sampleXMin
    );
    const int overlapMax = MinValue(
        lhs->sampleXMax,
        rhs->sampleXMax
    );
    if (overlapMin > overlapMax) {
        return 0;
    }

    const float lhsStart =
        zRndrSpanDepthAtXByPartsLocal(
            lhs->sampleXMin,
            lhs->invDepth,
            lhs->depthSlope,
            overlapMin
        );
    const float lhsEnd =
        zRndrSpanDepthAtXByPartsLocal(
            lhs->sampleXMin,
            lhs->invDepth,
            lhs->depthSlope,
            overlapMax
        );
    const float rhsStart =
        zRndrSpanDepthAtXByPartsLocal(
            rhs->sampleXMin,
            rhs->invDepth,
            rhs->depthSlope,
            overlapMin
        );
    const float rhsEnd =
        zRndrSpanDepthAtXByPartsLocal(
            rhs->sampleXMin,
            rhs->invDepth,
            rhs->depthSlope,
            overlapMax
        );
    const float startDelta = lhsStart - rhsStart;
    const float endDelta = lhsEnd - rhsEnd;

    if (startDelta >= zRndr::g_spanDepthBias && endDelta >= zRndr::g_spanDepthBias) {
        return 1;
    }
    if (startDelta <= -zRndr::g_spanDepthBias && endDelta <= -zRndr::g_spanDepthBias) {
        return 0;
    }

    const int midX = overlapMin + ((overlapMax - overlapMin) >> 1);
    const float lhsMid =
        zRndrSpanDepthAtXByPartsLocal(
            lhs->sampleXMin,
            lhs->invDepth,
            lhs->depthSlope,
            midX
        );
    const float rhsMid =
        zRndrSpanDepthAtXByPartsLocal(
            rhs->sampleXMin,
            rhs->invDepth,
            rhs->depthSlope,
            midX
        );
    return lhsMid >= rhsMid ? 1 : 0;
}

namespace {
struct Plane2f {
    zVec2 gradient;
    float base;
};

struct TexturedPlanes {
    Plane2f reciprocalZ;
    Plane2f uOverZ;
    Plane2f vOverZ;
    float originX;
    float originY;
};

/**
 * Recovered helper: RoundToFixed20.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr perspective span callers that round texture and shade deltas to fixed-point steps.
 * Purpose: Round a floating-point value to the nearest integer for fixed-point span state.
 */
int RoundToFixed20(
    float value
) {
    return (int)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

/**
 * Recovered helper: Fixed16FromFloat.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr polygon scan conversion callers that round coordinates into 16.16 fixed point.
 * Purpose: Convert a floating-point value to signed 16.16 fixed-point with symmetric rounding.
 */
int Fixed16FromFloat(
    float value
) {
    const double scaled = (double)(value) * 65536.0;
    return (int)(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

/**
 * Recovered helper: ScanlineStartFromY.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr polygon scan conversion callers that bias the starting scanline from fixed-point Y.
 * Purpose: Compute the first covered scanline for a polygon edge Y coordinate.
 */
int ScanlineStartFromY(
    float y
) {
    return (Fixed16FromFloat(y) + 0x7fff) >> 16;
}

/**
 * Recovered helper: ScanlineEndFromY.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr polygon scan conversion callers that bias the ending scanline from fixed-point Y.
 * Purpose: Compute the last covered scanline for a polygon edge Y coordinate.
 */
int ScanlineEndFromY(
    float y
) {
    return (Fixed16FromFloat(y) - 0x8041) >> 16;
}

/**
 * Recovered helper: SpanStartFromX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr polygon span callers that bias the starting sample from fixed-point X.
 * Purpose: Compute the first covered span sample for an edge X coordinate.
 */
int SpanStartFromX(
    float x
) {
    return (Fixed16FromFloat(x) + 0x7fff) >> 16;
}

/**
 * Recovered helper: SpanEndFromX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr polygon span callers that bias the ending sample from fixed-point X.
 * Purpose: Compute the last covered span sample for an edge X coordinate.
 */
int SpanEndFromX(
    float x
) {
    return (Fixed16FromFloat(x) - 0x8001) >> 16;
}

struct ScanConvertEdge {
    int xStepFixed;
    int yStart;
    int currentXFixed;
    int reserved;
};

/**
 * Recovered helper: WrapPolygonIndex.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr scan-edge builders that walk polygon vertices in either direction.
 * Purpose: Wrap a polygon vertex index by one step at either end of the vertex array.
 */
int WrapPolygonIndex(
    int index,
    int vertexCount
) {
    if (index < 0) {
        return index + vertexCount;
    }

    if (index >= vertexCount) {
        return index - vertexCount;
    }

    return index;
}

/**
 * Recovered helper: BuildScanConvertEdges.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr rasterization callers that build left and right edge tables for scan conversion.
 * Purpose: Build fixed-point edge-walk records for one side of a polygon.
 */
int BuildScanConvertEdges(
    const zVec3 *vertices,
    int vertexCount,
    int startIndex,
    int stopIndex,
    int step,
    ScanConvertEdge *edges
) {
    int edgeCount = 0;
    int vertexIndex = startIndex;
    int yStart = ScanlineStartFromY(vertices[vertexIndex].y);
    float sampleY = (float)(yStart) + 0.5f;

    while (vertexIndex != stopIndex && edgeCount < 0x40) {
        const int nextIndex = WrapPolygonIndex(
            vertexIndex + step,
            vertexCount
        );
        const zVec3 &start = vertices[vertexIndex];
        const zVec3 &end = vertices[nextIndex];

        if (sampleY <= end.y) {
            const float dy = end.y - start.y;
            edges[edgeCount].yStart = yStart;
            edges[edgeCount].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                edges[edgeCount].xStepFixed = Fixed16FromFloat(xSlope);
                edges[edgeCount].currentXFixed =
                    Fixed16FromFloat(start.x + (sampleY - start.y) * xSlope);
            } else {
                edges[edgeCount].xStepFixed = 0;
                edges[edgeCount].currentXFixed = Fixed16FromFloat(start.x);
            }

            ++edgeCount;
            yStart = ScanlineStartFromY(end.y);
            sampleY = (float)(yStart) + 0.5f;
        }

        vertexIndex = nextIndex;
    }

    return edgeCount;
}

/**
 * Recovered helper: BuildPlaneFromTriangle.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued textured polygon callers that derive interpolation planes from triangles.
 * Purpose: Build a screen-space interpolation plane from triangle vertices and values.
 */
Plane2f BuildPlaneFromTriangle(
    const zVec3 *triVerts,
    const float values[3]
) {
    const float dx10 = triVerts[0].x - triVerts[1].x;
    const float dx12 = triVerts[2].x - triVerts[1].x;
    const float dy10 = triVerts[0].y - triVerts[1].y;
    const float dy12 = triVerts[2].y - triVerts[1].y;
    const float determinant = dy12 * dx10 - dy10 * dx12;

    Plane2f plane = {0};
    if (determinant != 0.0f) {
        const float dv10 = values[0] - values[1];
        const float dv12 = values[2] - values[1];
        const float inverseDeterminant = -1.0f / determinant;
        plane.gradient.x = (dy12 * dv10 - dy10 * dv12) * inverseDeterminant;
        plane.gradient.y = (dx10 * dv12 - dx12 * dv10) * inverseDeterminant;
    }

    plane.base = values[0];
    return plane;
}

/**
 * Recovered helper: BuildScreenPlaneFromTriangle.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued textured polygon callers that need a screen-origin adjusted plane.
 * Purpose: Build a screen-space interpolation plane with its base adjusted to screen origin.
 */
Plane2f BuildScreenPlaneFromTriangle(
    const zVec3 *triVerts,
    const float values[3]
) {
    Plane2f plane = BuildPlaneFromTriangle(
        triVerts,
        values
    );
    plane.base = values[0] - triVerts[0].x * plane.gradient.x - triVerts[0].y * plane.gradient.y;
    return plane;
}

/**
 * Recovered helper: BuildQueuedTexturePlanes.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued texture draw callers that share perspective-correct texture interpolation.
 * Purpose: Build reciprocal-Z and texture-over-Z planes for queued textured polygon spans.
 */
TexturedPlanes BuildQueuedTexturePlanes(
    const zVec3 *clippedTriVerts,
    const zVec3 *triVerts,
    const zVec2 *triUVs,
    float imageWidth,
    float imageHeight
) {
    TexturedPlanes planes = {0};
    const bool useClippedNearPlane =
        clippedTriVerts != 0 && (clippedTriVerts[0].z < 10.0f || clippedTriVerts[1].z < 10.0f ||
                                    clippedTriVerts[2].z < 10.0f);

    if (useClippedNearPlane) {
        zMath_BuildPerspectiveTextureInterpolants(
            clippedTriVerts,
            triUVs,
            &planes.reciprocalZ.gradient,
            &planes.reciprocalZ.base,
            &planes.uOverZ.gradient,
            &planes.uOverZ.base,
            &planes.vOverZ.gradient,
            &planes.vOverZ.base
        );
        planes.uOverZ.gradient.x *= imageWidth;
        planes.uOverZ.gradient.y *= imageWidth;
        planes.uOverZ.base *= imageWidth;
        planes.vOverZ.gradient.x *= imageHeight;
        planes.vOverZ.gradient.y *= imageHeight;
        planes.vOverZ.base *= imageHeight;
        planes.originX = g_zMath_ProjOffsetX;
        planes.originY = g_zMath_ProjOffsetY;
    } else {
        float reciprocalValues[3] = {0};
        float uValues[3] = {0};
        float vValues[3] = {0};
        for (int i = 0; i < 3; ++i) {
            reciprocalValues[i] = triVerts[i].z;
            uValues[i] = imageWidth * triVerts[i].z * triUVs[i].x;
            vValues[i] = imageHeight * triVerts[i].z * triUVs[i].y;
        }

        planes.reciprocalZ = BuildPlaneFromTriangle(
            triVerts,
            reciprocalValues
        );
        planes.uOverZ = BuildPlaneFromTriangle(
            triVerts,
            uValues
        );
        planes.vOverZ = BuildPlaneFromTriangle(
            triVerts,
            vValues
        );
        planes.originX = triVerts[0].x;
        planes.originY = triVerts[0].y;
    }

    const float adjustX = planes.originX - 0.5f;
    const float adjustY = planes.originY - 0.5f;
    planes.reciprocalZ.base -=
        adjustX * planes.reciprocalZ.gradient.x + adjustY * planes.reciprocalZ.gradient.y;
    planes.uOverZ.base -= adjustX * planes.uOverZ.gradient.x + adjustY * planes.uOverZ.gradient.y;
    planes.vOverZ.base -= adjustX * planes.vOverZ.gradient.x + adjustY * planes.vOverZ.gradient.y;
    return planes;
}

/**
 * Recovered helper: EvalPlane.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued texture draw callers that sample interpolation planes per span/chunk.
 * Purpose: Evaluate a two-dimensional interpolation plane at one screen coordinate.
 */
float EvalPlane(
    const Plane2f &plane,
    float x,
    float y
) {
    return x * plane.gradient.x + y * plane.gradient.y + plane.base;
}

/**
 * Recovered helper: SelectPerspectiveChunkPixels.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr perspective texture callers that choose adaptive per-span chunk lengths.
 * Purpose: Select the pixel count used for one perspective-correct texture span chunk.
 */
int SelectPerspectiveChunkPixels(
    float minPositiveReciprocalZ,
    float reciprocalZStepX
) {
    if (zRndr::g_perspectiveAdaptiveMinSpan == 0) {
        return MaxValue(
            1,
            zRndr::g_perspectiveTextureDeltaXPow2
        );
    }

    int chunkPixels = zRndr::g_perspectiveAdaptiveMaxSpan;
    if (reciprocalZStepX != 0.0f) {
        chunkPixels = (int)(fabs(
            minPositiveReciprocalZ * zRndr::g_perspectiveAdaptiveSlope / reciprocalZStepX
        ));
    }

    chunkPixels = MinValue(
        chunkPixels,
        zRndr::g_perspectiveAdaptiveMaxSpan
    );
    chunkPixels = MaxValue(
        chunkPixels,
        zRndr::g_perspectiveAdaptiveMinSpan
    );
    return MaxValue(
        1,
        chunkPixels
    );
}

/**
 * Recovered helper: DispatchTexturedSpanChunks.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued texture draw callers that dispatch spans through selected texture callbacks.
 * Purpose: Split one visible span into texture chunks and dispatch each chunk to the span routine.
 */
void DispatchTexturedSpanChunks(
    zRndr::TexturedQueuedSpanProc spanProc,
    const TexturedPlanes &planes,
    const Plane2f *shadePlane,
    zRndr::SpanNodePartial *span,
    int y,
    int chunkPixels,
    float textureScale,
    int texVShift
) {
    int remaining = span->sampleXMax - span->sampleXMin + 1;
    int x = span->sampleXMin;
    while (remaining > 0) {
        const int count = MinValue(
            remaining,
            chunkPixels
        );
        const float startX = (float)(x);
        const float endX = (float)(x + count);
        const float sampleY = (float)(y);
        const float startInvZ = EvalPlane(
            planes.reciprocalZ,
            startX,
            sampleY
        );
        const float endInvZ = EvalPlane(
            planes.reciprocalZ,
            endX,
            sampleY
        );
        if (startInvZ == 0.0f || endInvZ == 0.0f) {
            x += count;
            remaining -= count;
            continue;
        }

        const float startU = EvalPlane(
            planes.uOverZ,
            startX,
            sampleY
        ) / startInvZ;
        const float startV = EvalPlane(
            planes.vOverZ,
            startX,
            sampleY
        ) / startInvZ;
        const float endU = EvalPlane(
            planes.uOverZ,
            endX,
            sampleY
        ) / endInvZ;
        const float endV = EvalPlane(
            planes.vOverZ,
            endX,
            sampleY
        ) / endInvZ;

        zRndr::g_spanActiveTexUStepFixed20 = RoundToFixed20((endU - startU) * textureScale / (float)(count));
        zRndr::g_spanActiveTexVStepFixed20 = RoundToFixed20((endV - startV) * textureScale / (float)(count));
        if (shadePlane != 0) {
            const float startShade =
                MaxValue(
                    0.0f,
                    MinValue(255.0f, EvalPlane(
                        *shadePlane,
                        startX,
                        sampleY
                    ))
                );
            const float endShade =
                MaxValue(
                    0.0f,
                    MinValue(255.0f, EvalPlane(
                        *shadePlane,
                        endX,
                        sampleY
                    ))
                );
            zRndr::g_spanActiveShadeFixed16 = RoundToFixed20(startShade * 65536.0f);
            zRndr::g_spanActiveShadeStepFixed16 =
                RoundToFixed20((endShade - startShade) * 65536.0f / (float)(count));
        }

        spanProc(
            RoundToFixed20(startU * textureScale),
            RoundToFixed20(startV * textureScale),
            count,
            texVShift
        );

        zRndr::g_spanCurrentSpanBaseAddr += count;
        x += count;
        remaining -= count;
    }
}
} // namespace

/**
 * Reimplements 0x492000: zRndr_RasterizePolyWithSpanList
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Rasterize one polygon through the active span-list builder and selected span routine.
 */
void __fastcall zRndr_RasterizePolyWithSpanList(
    zVec3 *vertices,
    zVec3 *planeVerts,
    int vertCount,
    int spanOpContext
) {
    const float dx10 = planeVerts[0].x - planeVerts[1].x;
    const float dx12 = planeVerts[2].x - planeVerts[1].x;
    const float dy10 = planeVerts[0].y - planeVerts[1].y;
    const float dy12 = planeVerts[2].y - planeVerts[1].y;
    const float determinant = dy12 * dx10 - dy10 * dx12;
    float invDepthSlopeX = determinant;
    float invDepthSlopeY = determinant;
    if (determinant != 0.0f) {
        const float dz10 = planeVerts[0].z - planeVerts[1].z;
        const float dz12 = planeVerts[2].z - planeVerts[1].z;
        const float inverseDeterminant = 1.0f / determinant;
        invDepthSlopeX = (dy12 * dz10 - dy10 * dz12) * inverseDeterminant;
        invDepthSlopeY = (dx10 * dz12 - dx12 * dz10) * inverseDeterminant;
    }

    const float invDepthBiasBase = planeVerts[0].z - (planeVerts[0].x - 0.5f) * invDepthSlopeX -
                                   (planeVerts[0].y - 0.5f) * invDepthSlopeY;

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    for (int i = 1; i < vertCount; ++i) {
        if (vertices[i].y < vertices[topVertexIndex].y) {
            topVertexIndex = i;
        }
        if (vertices[i].y >= vertices[bottomVertexIndex].y) {
            bottomVertexIndex = i;
        }
    }

    ScanConvertEdge edgeTableA[0x40];
    ScanConvertEdge edgeTableB[0x40];
    int edgeCountA = 0;
    int edgeCountB = 0;
    int fixed16Value;
    int edgeVertexIndex;
    int edgeYStart;
    float edgeSampleY;

    int edgeStepA;
    int edgeStepB;
    if (zRndr::g_scanConvertMode != 0) {
        edgeStepA = 1;
        edgeStepB = -1;
    } else {
        edgeStepA = -1;
        edgeStepB = 1;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(
        fixed16Value,
        vertices[edgeVertexIndex].y
    );
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepA;
        if (nextIndex < 0) {
            nextIndex += vertCount;
        }
        if (nextIndex >= vertCount) {
            nextIndex -= vertCount;
        }
        const zVec3 &start = vertices[edgeVertexIndex];
        const zVec3 &end = vertices[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].xStepFixed,
                    xSlope
                );
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x
                );
            }

            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(
                fixed16Value,
                end.y
            );
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(
        fixed16Value,
        vertices[edgeVertexIndex].y
    );
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepB;
        if (nextIndex < 0) {
            nextIndex += vertCount;
        }
        if (nextIndex >= vertCount) {
            nextIndex -= vertCount;
        }
        const zVec3 &start = vertices[edgeVertexIndex];
        const zVec3 &end = vertices[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].xStepFixed,
                    xSlope
                );
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x
                );
            }

            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(
                fixed16Value,
                end.y
            );
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(
        fixed16Value,
        vertices[topVertexIndex].y
    );
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(
        fixed16Value,
        vertices[bottomVertexIndex].y
    );
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x141];
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char *scanlineBase =
        (unsigned char *)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        while (edgeIndexA < edgeCountA && y >= edgeTableA[edgeIndexA].yStart) {
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            ++edgeIndexA;
        }

        while (edgeIndexB < edgeCountB && y >= edgeTableB[edgeIndexB].yStart) {
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            ++edgeIndexB;
        }

        int xMin;
        int xMax;
        if (currentXFixedA > currentXFixedB) {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            const float rowDepthBase = (float)(y) * invDepthSlopeY + invDepthBiasBase;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                ((float)(xMin) * invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                ((float)(xMax) * invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = invDepthSlopeX;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanList(
                visibleSpans,
                y,
                &spanCount
            );

            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                const int byteOffset = (int)(span->sampleXMin) * zRndr::g_bytesPerPixel;
                zRndr::g_spanCurrentSpanBaseAddr = (unsigned short *)(scanlineBase + byteOffset);
                zRndr::g_pfnSelectedSpanOp(
                    spanOpContext,
                    pixelCount
                );
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
    }
}

/**
 * Reimplements 0x492f00: zRndr_DrawFlatImmediate
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Draw an immediate flat polygon through the flat span callback path.
 */
void __fastcall zRndr_DrawFlatImmediate(
    zVec3 *vertices,
    zVec3 *planeVertices,
    int vertCount,
    int flatSpanOpEdxArg,
    int flatSpanOpEcxArg
) {
    const float dx10 = planeVertices[0].x - planeVertices[1].x;
    const float dx12 = planeVertices[2].x - planeVertices[1].x;
    const float dy10 = planeVertices[0].y - planeVertices[1].y;
    const float dy12 = planeVertices[2].y - planeVertices[1].y;
    const float determinant = dy12 * dx10 - dy10 * dx12;
    float invDepthSlopeX = determinant;
    float invDepthSlopeY = determinant;
    if (determinant != 0.0f) {
        const float dz10 = planeVertices[0].z - planeVertices[1].z;
        const float dz12 = planeVertices[2].z - planeVertices[1].z;
        const float inverseDeterminant = 1.0f / determinant;
        invDepthSlopeX = (dy12 * dz10 - dy10 * dz12) * inverseDeterminant;
        invDepthSlopeY = (dx10 * dz12 - dx12 * dz10) * inverseDeterminant;
    }

    const float invDepthBiasBase = planeVertices[0].z -
                                   (planeVertices[0].x - 0.5f) * invDepthSlopeX -
                                   (planeVertices[0].y - 0.5f) * invDepthSlopeY;

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    for (int i = 1; i < vertCount; ++i) {
        if (vertices[i].y < vertices[topVertexIndex].y) {
            topVertexIndex = i;
        }
        if (vertices[i].y >= vertices[bottomVertexIndex].y) {
            bottomVertexIndex = i;
        }
    }

    ScanConvertEdge edgeTableA[0x40] = {0};
    ScanConvertEdge edgeTableB[0x40] = {0};
    int edgeCountA;
    int edgeCountB;
    if (zRndr::g_scanConvertMode != 0) {
        edgeCountA = BuildScanConvertEdges(
            vertices,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            1,
            edgeTableA
        );
        edgeCountB = BuildScanConvertEdges(
            vertices,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            -1,
            edgeTableB
        );
    } else {
        edgeCountA = BuildScanConvertEdges(
            vertices,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            -1,
            edgeTableA
        );
        edgeCountB = BuildScanConvertEdges(
            vertices,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            1,
            edgeTableB
        );
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    const int firstScanline = ScanlineStartFromY(vertices[topVertexIndex].y);
    const int lastScanline = ScanlineEndFromY(vertices[bottomVertexIndex].y);
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40];
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char *scanlineBase =
        (unsigned char *)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        while (edgeIndexA < edgeCountA && y >= edgeTableA[edgeIndexA].yStart) {
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            ++edgeIndexA;
        }

        while (edgeIndexB < edgeCountB && y >= edgeTableB[edgeIndexB].yStart) {
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            ++edgeIndexB;
        }

        int xMin;
        int xMax;
        if (currentXFixedA > currentXFixedB) {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            const float rowDepthBase = (float)(y)*invDepthSlopeY + invDepthBiasBase;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                ((float)(xMin)*invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                ((float)(xMax)*invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = invDepthSlopeX;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanListSecondary(
                visibleSpans,
                y,
                &spanCount
            );

            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                const int byteOffset = (int)(span->sampleXMin) * zRndr::g_bytesPerPixel;
                zRndr::g_spanCurrentSpanBaseAddr = (unsigned short *)(scanlineBase + byteOffset);
                zRndr::g_pfnFlatImmediateSpanOp(
                    flatSpanOpEcxArg,
                    flatSpanOpEdxArg,
                    pixelCount
                );
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
    }
}

/**
 * Reimplements 0x4936d0: zRndr_RasterizePoly
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Scan-convert a polygon and dispatch each covered span to the selected span routine.
 */
void __fastcall zRndr_RasterizePoly(
    zVec3 *vertices,
    int vertCount,
    int spanOpContext
) {
    zVec3 reducedVerts[0x40];
    int reducedCount = 1;
    reducedVerts[0].x = vertices[0].x;
    reducedVerts[0].y = vertices[0].y;
    for (int reduceIndex = 1; reduceIndex < vertCount && reducedCount < 0x40; ++reduceIndex) {
        const zVec3 &previous = reducedVerts[reducedCount - 1];
        reducedVerts[reducedCount].x = vertices[reduceIndex].x;
        reducedVerts[reducedCount].y = vertices[reduceIndex].y;
        if (reducedVerts[reducedCount].x == previous.x &&
            reducedVerts[reducedCount].y == previous.y) {
            continue;
        }

        ++reducedCount;
    }

    if (reducedCount > 1 && reducedVerts[reducedCount - 1].x == reducedVerts[0].x &&
        reducedVerts[reducedCount - 1].y == reducedVerts[0].y) {
        --reducedCount;
    }

    if (reducedCount < 3) {
        return;
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    for (int scanIndex = 1; scanIndex < reducedCount; ++scanIndex) {
        if (reducedVerts[scanIndex].y < reducedVerts[topVertexIndex].y) {
            topVertexIndex = scanIndex;
        }
        if (reducedVerts[scanIndex].y >= reducedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = scanIndex;
        }
    }

    ScanConvertEdge edgeTableA[0x40] = {0};
    ScanConvertEdge edgeTableB[0x40] = {0};
    int edgeCountA;
    int edgeCountB;
    if (zRndr::g_scanConvertMode != 0) {
        edgeCountA = BuildScanConvertEdges(
            reducedVerts,
            reducedCount,
            topVertexIndex,
            bottomVertexIndex,
            1,
            edgeTableA
        );
        edgeCountB = BuildScanConvertEdges(
            reducedVerts,
            reducedCount,
            topVertexIndex,
            bottomVertexIndex,
            -1,
            edgeTableB
        );
    } else {
        edgeCountA = BuildScanConvertEdges(
            reducedVerts,
            reducedCount,
            topVertexIndex,
            bottomVertexIndex,
            -1,
            edgeTableA
        );
        edgeCountB = BuildScanConvertEdges(
            reducedVerts,
            reducedCount,
            topVertexIndex,
            bottomVertexIndex,
            1,
            edgeTableB
        );
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    const int firstScanline = ScanlineStartFromY(reducedVerts[topVertexIndex].y);
    const int lastScanline = ScanlineEndFromY(reducedVerts[bottomVertexIndex].y);
    if (firstScanline > lastScanline) {
        return;
    }

    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char *scanlineBase =
        (unsigned char *)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        while (edgeIndexA < edgeCountA && y >= edgeTableA[edgeIndexA].yStart) {
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            ++edgeIndexA;
        }

        while (edgeIndexB < edgeCountB && y >= edgeTableB[edgeIndexB].yStart) {
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            ++edgeIndexB;
        }

        int xStart;
        int xEnd;
        if (currentXFixedA > currentXFixedB) {
            xStart = (currentXFixedB + 0x7fff) >> 16;
            xEnd = (currentXFixedA - 0x8001) >> 16;
        } else {
            xStart = (currentXFixedA + 0x7fff) >> 16;
            xEnd = (currentXFixedB - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;

        if (xStart <= xEnd) {
            const int pixelCount = xEnd - xStart;
            if (pixelCount > 0) {
                zRndr::g_spanCurrentSpanBaseAddr =
                    (unsigned short *)(scanlineBase + xStart * zRndr::g_bytesPerPixel);
                zRndr::g_pfnSelectedSpanOp(
                    spanOpContext,
                    pixelCount
                );
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
    }
}

/**
 * Reimplements 0x499a20: zRndr_SubmitPolyWithSpanList
 * Original file: D:\Proj\GameZRecoil\zRender\zrndr_draw.c.
 * Source file evidence: embedded zError file path in this function.
 * Purpose: Submit a flat polygon for immediate drawing or deferred transparent/overwrite queues.
 */
void __fastcall zRndr_SubmitPolyWithSpanList(
    zVec3 *entryVertices,
    zVec3 *entryPlaneVertices,
    int spanOpContext,
    int alpha255,
    int vertCount,
    int queueOverwrite
) {
    const int kMaxQueuedPolys = 0x15e;
    const char *kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(
                0x400,
                kSourceFile,
                0x9c,
                " Not enough MAX_OVERWRITE_POLYS: need %d\n",
                queueIndex
            );
            return;
        }

        ++zRndr::g_overwriteQueueCount;
        zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
        cmd.hasClippedTriVerts = 0;
        memcpy(
            cmd.polyVerts,
            entryVertices,
            (size_t)(vertCount) * sizeof(zVec3)
        );
        memcpy(
            cmd.triVerts,
            entryPlaneVertices,
            3 * sizeof(zVec3)
        );
        cmd.alphaOrShadeF = (float)(alpha255);
        cmd.materialRef = 0;
        cmd.vertexCount = vertCount;
        cmd.shadeOrSpanMode = spanOpContext;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        return;
    }

    if (alpha255 >= 0xff) {
        zRndr_RasterizePolyWithSpanList(
            entryVertices,
            entryPlaneVertices,
            vertCount,
            spanOpContext
        );
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(
            0x400,
            kSourceFile,
            0xb9,
            " Not enough MAX_TRANSPARENT_POLYS: need %d\n",
            queueIndex
        );
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[queueIndex];
    memcpy(
        cmd.polyVerts,
        entryVertices,
        (size_t)(vertCount) * sizeof(zVec3)
    );
    memcpy(
        cmd.triVerts,
        entryPlaneVertices,
        3 * sizeof(zVec3)
    );
    cmd.materialRef = 0;
    cmd.vertexCount = vertCount;
    cmd.shadeOrSpanMode = spanOpContext;
    cmd.alphaOrShadeBits = alpha255;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    ++zRndr::g_transparentQueueCount;
}

/**
 * Reimplements 0x499c40: zRndr_SubmitTexturedPolyUniformAlphaOrShade
 * Original file: D:\Proj\GameZRecoil\zRender\zrndr_draw.c.
 * Source file evidence: embedded zError file path in this function.
 * Purpose: Submit a textured polygon with one alpha/shade value to the immediate or queued paths.
 */
void __fastcall zRndr_SubmitTexturedPolyUniformAlphaOrShade(
    zVec3 *projectedPolyVerts,
    zVec3 *clippedTriVerts,
    zVec3 *triData9f,
    zVec2 *triUVs,
    int vertexCount,
    zImage_TexDirEntryPartial *entry,
    float alphaOrShadeF,
    int queueOverwrite
) {
    const int kMaxQueuedPolys = 0x15e;
    const char *kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(
                0x400,
                kSourceFile,
                0xfa,
                " Not enough MAX_OVERWRITE_POLYS: need %d\n",
                queueIndex
            );
            return;
        }

        ++zRndr::g_overwriteQueueCount;
        zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
        cmd.commandTag = 1;
        cmd.vertexCount = vertexCount;
        cmd.materialRef = entry;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.alphaOrShadeF = alphaOrShadeF;
        memcpy(
            cmd.polyVerts,
            projectedPolyVerts,
            (size_t)(vertexCount) * sizeof(zVec3)
        );
        memcpy(
            cmd.triVerts,
            triData9f,
            3 * sizeof(zVec3)
        );
        memcpy(
            cmd.triUVs,
            triUVs,
            3 * sizeof(zVec2)
        );
        if (clippedTriVerts != 0) {
            memcpy(
                cmd.clippedTriVertOverlay.clippedTriVerts,
                clippedTriVerts,
                3 * sizeof(zVec3)
            );
            cmd.hasClippedTriVerts = 1;
        } else {
            cmd.hasClippedTriVerts = 0;
        }
        cmd.texKey = g_zRndr_ActivePaletteRemapKey;
        return;
    }

    zVidImagePartial *image = entry != 0 ? entry->image : 0;
    if ((image->formatFlagsPacked & 2) == 0 && alphaOrShadeF < 1.0f) {
        zRndr_DrawTexturedQueuedAlpha(
            entry,
            projectedPolyVerts,
            clippedTriVerts,
            triData9f,
            triUVs,
            vertexCount,
            g_zRndr_ActivePaletteRemapKey
        );
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(
            0x400,
            kSourceFile,
            0x126,
            " Not enough MAX_TRANSPARENT_POLYS: need %d\n",
            queueIndex
        );
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[queueIndex];
    cmd.vertexCount = vertexCount;
    cmd.materialRef = entry;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    if ((image->formatFlagsPacked & 2) != 0) {
        memcpy(
            &cmd.alphaOrShadeBits,
            &alphaOrShadeF,
            sizeof(float)
        );
    } else {
        cmd.alphaOrShadeBits = (int)(alphaOrShadeF * 255.0f);
    }
    memcpy(
        cmd.polyVerts,
        projectedPolyVerts,
        (size_t)(vertexCount) * sizeof(zVec3)
    );
    memcpy(
        cmd.triVerts,
        triData9f,
        3 * sizeof(zVec3)
    );
    memcpy(
        cmd.triUVs,
        triUVs,
        3 * sizeof(zVec2)
    );
    if (clippedTriVerts != 0) {
        memcpy(
            cmd.clippedTriVertOverlay.clippedTriVerts,
            clippedTriVerts,
            3 * sizeof(zVec3)
        );
        cmd.hasClippedTriVerts = 1;
    } else {
        cmd.hasClippedTriVerts = 0;
    }
    cmd.texKey = g_zRndr_ActivePaletteRemapKey;
    ++zRndr::g_transparentQueueCount;
}

/**
 * Reimplements 0x499ec0: zRndr_SubmitTexturedPolyPerVertexAlphaOrShade
 * Original file: D:\Proj\GameZRecoil\zRender\zrndr_draw.c.
 * Source file evidence: embedded zError file path in this function.
 * Purpose: Submit a textured polygon with per-vertex alpha/shade values to draw or queue paths.
 */
void __fastcall zRndr_SubmitTexturedPolyPerVertexAlphaOrShade(
    zVec3 *projectedPolyVerts,
    zVec3 *clippedTriVerts,
    zVec3 *triData9f,
    zVec2 *triUVs,
    float *perVertexAlphaOrShadeF,
    int shadeOrSpanMode,
    int vertexCount,
    zImage_TexDirEntryPartial *entry,
    int preservePaletteRemapKey,
    int queueOverwrite
) {
    const int kMaxQueuedPolys = 0x15e;
    const char *kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    zVidImagePartial *image = entry != 0 ? entry->image : 0;
    int texKey = g_zRndr_ActivePaletteRemapKey;
    int usingDerivedPaletteKey = 0;

    if (texKey == -1 && preservePaletteRemapKey == 0 && entry != 0 &&
        entry->image->paletteMetaPacked > 0) {
        texKey = zVid_PaletteRemap_FindRecipeIndexFromRgb(
            (zColorRgb *)(zRndr::g_fogParamsActive.colorRgb01)
        );
        if (texKey >= 0) {
            int shadeBucket = (int)(perVertexAlphaOrShadeF[0] * 0.125f);
            if (shadeBucket > 0x1f) {
                shadeBucket = 0x1f;
            }
            if (shadeBucket < 0) {
                shadeBucket = 0;
            }
            texKey = (texKey << 5) + shadeBucket;
        }
    }

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(
                0x400,
                kSourceFile,
                0x19e,
                " Not enough MAX_OVERWRITE_POLYS: need %d\n",
                queueIndex
            );
            return;
        }

        ++zRndr::g_overwriteQueueCount;
        zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
        cmd.commandTag = usingDerivedPaletteKey != 0 ? 1 : 2;
        cmd.vertexCount = vertexCount;
        cmd.materialRef = entry;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.alphaOrShadeF = (float)(shadeOrSpanMode);
        memcpy(
            cmd.polyVerts,
            projectedPolyVerts,
            (size_t)(vertexCount) * sizeof(zVec3)
        );
        memcpy(
            cmd.triVerts,
            triData9f,
            3 * sizeof(zVec3)
        );
        memcpy(
            cmd.triUVs,
            triUVs,
            3 * sizeof(zVec2)
        );
        if (usingDerivedPaletteKey == 0) {
            memcpy(
                cmd.perVertexAlphaOrShadeF,
                perVertexAlphaOrShadeF,
                (size_t)(vertexCount) * sizeof(float)
            );
        }
        if (clippedTriVerts != 0) {
            memcpy(
                cmd.clippedTriVertOverlay.clippedTriVerts,
                clippedTriVerts,
                3 * sizeof(zVec3)
            );
            cmd.hasClippedTriVerts = 1;
        } else {
            cmd.hasClippedTriVerts = 0;
        }
        cmd.texKey = texKey;
        return;
    }

    if ((image->formatFlagsPacked & 2) == 0) {
        if (vertexCount - 2 <= 0) {
            return;
        }

        zVec3 fanVerts[3];
        zVec3 shadeTriplet;
        fanVerts[0] = projectedPolyVerts[0];
        shadeTriplet.x = perVertexAlphaOrShadeF[0];
        {
            for (int fanTriIndex = 0; fanTriIndex < vertexCount - 2; ++fanTriIndex) {
                fanVerts[1] = projectedPolyVerts[fanTriIndex + 1];
                fanVerts[2] = projectedPolyVerts[fanTriIndex + 2];
                shadeTriplet.y = perVertexAlphaOrShadeF[fanTriIndex + 1];
                shadeTriplet.z = perVertexAlphaOrShadeF[fanTriIndex + 2];
                zRndr_DrawTexturedQueued(
                    entry,
                    fanVerts,
                    clippedTriVerts,
                    triData9f,
                    triUVs,
                    &shadeTriplet,
                    3,
                    fanTriIndex,
                    texKey
                );
            }
        }
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(
            0x400,
            kSourceFile,
            0x1e1,
            " Not enough MAX_TRANSPARENT_POLYS: need %d\n",
            queueIndex
        );
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[queueIndex];
    cmd.vertexCount = vertexCount;
    cmd.materialRef = entry;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    cmd.alphaOrShadeBits = 0xff;
    memcpy(
        cmd.polyVerts,
        projectedPolyVerts,
        (size_t)(vertexCount) * sizeof(zVec3)
    );
    memcpy(
        cmd.triVerts,
        triData9f,
        3 * sizeof(zVec3)
    );
    memcpy(
        cmd.triUVs,
        triUVs,
        3 * sizeof(zVec2)
    );
    cmd.texKey = texKey;
    ++zRndr::g_transparentQueueCount;
}

/**
 * Reimplements 0x48d6d0: zRndr_OverlayRect_Submit
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Overlay.cpp.
 * Source file evidence: recovered original path on the prior source label.
 * Purpose: Submit an overlay rectangle to Direct3D or stage it for software overlay blending.
 */
void __fastcall zRndr_OverlayRect_Submit(
    unsigned int packedColor16,
    zVidRect32 *rectOrNull,
    double alpha
) {
    zVidRect32 rect;
    int xMax;
    if (rectOrNull != 0) {
        rect.left = rectOrNull->left;
        rect.top = rectOrNull->top;
        xMax = rectOrNull->right;
        rect.bottom = rectOrNull->bottom;
    } else {
        rect.left = 0;
        rect.top = 0;
        rect.bottom = g_zVideo_FxSurfaceHeight;
        xMax = g_zVideo_FxSurfaceWidth - 1;
    }

    if (g_zVideo_ActiveRendererPath != 0) {
        rect.right = xMax + 1;
        zVideo_dd3d::QueueSolidQuad(
            packedColor16 & 0xffffU,
            &rect,
            alpha
        );
        return;
    }

    zRndr::g_overlayBlendRectTop = rect.top;
    zRndr::g_overlayBlendRectRight = xMax;
    zRndr::g_overlayBlendRectBottom = rect.bottom;
    zRndr::g_overlayBlendRectLeft = rect.left;
    zRndr::g_overlayBlendEnabled = 1;
    zRndr::g_overlayBlendPackedColor16 = packedColor16 & 0xffffU;
    zRndr::g_overlayBlendAlpha = alpha;
}

/**
 * Reimplements 0x48d7a0: zRndr_OverlayRect_FlushSw
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Overlay.cpp.
 * Source file evidence: recovered original path on the prior source label.
 * Source-shape evidence: BN selects the 555/565 scalar or MMX row callback,
 * computes packed premul and destination-scale globals through x87/_ftol, then
 * calls the selected row callback for each FX-surface row.
 * Purpose: Blend the staged software overlay rectangle into the active 16-bit video surface.
 */
void zRndr_OverlayRect_FlushSw() {
    if (zRndr::g_overlayBlendEnabled == 0) {
        return;
    }

    const int graphicsFlags = zRndr::g_graphicsFlags != 0 ? *zRndr::g_graphicsFlags : 0;
    if ((graphicsFlags & 4) != 0) {
        if (zRndr::g_pixelPackGreenBits == 5) {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555_Mmx;
        } else {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow565_Mmx;
        }
    } else {
        if (zRndr::g_pixelPackGreenBits == 5) {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555_Scalar;
        } else {
            zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow565_Scalar;
        }
    }

    unsigned int redMask;
    unsigned int greenMask;
    unsigned int blueMask;
    zVideo::PixelPack_GetRgbMasks(
        &redMask,
        &greenMask,
        &blueMask
    );

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
    zRndr::g_swOverlayPremulPacked =
        (((blueMask & premulBPair) | (redMask & premulRPair)) << 16) | (greenMask & premulGPair);
    zRndr::g_swOverlayPremulPackedRot16 =
        (zRndr::g_swOverlayPremulPacked >> 16) | (zRndr::g_swOverlayPremulPacked << 16);
    zRndr::g_swOverlayDstScale5 = (int)((1.0 - zRndr::g_overlayBlendAlpha) * 32.0);

    int rowY = zRndr::g_overlayBlendRectTop;
    const int rectLeft = zRndr::g_overlayBlendRectLeft;
    const int pixelCount = zRndr::g_overlayBlendRectRight - rectLeft;
    unsigned short *rowPixels16 =
        g_zVideo_FxSurfacePixels16 + g_zVideo_FxSurfacePitchPixels16 * rowY + rectLeft;
    while (rowY < zRndr::g_overlayBlendRectBottom) {
        zRndr::g_pfnOverlayBlendRow(
            rowPixels16,
            pixelCount
        );
        ++rowY;
        rowPixels16 += g_zVideo_FxSurfacePitchPixels16;
    }
}

/**
 * Reimplements 0x49a2b0: zRndr_FlushTransparentQueue
 * Source file evidence: zRndr queued draw cluster in this source file.
 * Purpose: Sort and draw queued transparent polygons, then reset the transparent queue.
 */
void zRndr_FlushTransparentQueue() {
    {
        for (int i = 0; i < zRndr::g_transparentQueueCount; ++i) {
            zRndr::g_transparentQueueSortIndices[i] = zRndr::g_transparentQueueCount - i - 1;
        }
    }

    bool swapped = false;
    do {
        {
            for (int i = 0; i < zRndr::g_transparentQueueCount - 1; ++i) {
                const int lhsIndex = zRndr::g_transparentQueueSortIndices[i];
                const int rhsIndex = zRndr::g_transparentQueueSortIndices[i + 1];
                if (zRndr::g_transparentQueue[rhsIndex].triVerts[0].z <
                    zRndr::g_transparentQueue[lhsIndex].triVerts[0].z) {
                    zRndr::g_transparentQueueSortIndices[i] = rhsIndex;
                    zRndr::g_transparentQueueSortIndices[i + 1] = lhsIndex;
                    swapped = true;
                }
            }
        }
    } while (swapped);

    {
        for (int i = 0; i < zRndr::g_transparentQueueCount; ++i) {
            const int queueIndex = zRndr::g_transparentQueueSortIndices[i];
            zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[queueIndex];

            zRndr::g_inverseDepthBias = cmd.savedInvDepthBias;
            zRndr::g_inverseDepthScale = cmd.savedInvDepthScale;
            zRndr::g_scanConvertMode = cmd.scanConvertMode;

            if (cmd.materialRef == 0) {
                zRndr_DrawFlatImmediate(
                    (zVec3 *)(cmd.polyVerts),
                    (zVec3 *)(cmd.triVerts),
                    cmd.vertexCount,
                    cmd.alphaOrShadeBits,
                    cmd.shadeOrSpanMode
                );
                continue;
            }

            zVec3 *clippedTriVerts =
                cmd.hasClippedTriVerts != 0
                    ? (zVec3 *)(cmd.clippedTriVertOverlay.clippedTriVerts)
                    : 0;
            zVec3 *polyVerts = (zVec3 *)(cmd.polyVerts);
            zVec3 *triVerts = (zVec3 *)(cmd.triVerts);
            zVec2 *triUVs = (zVec2 *)(cmd.triUVs);
            if ((cmd.materialRef->image->formatFlagsPacked & 2) == 0) {
                float alpha = 0.0f;
                memcpy(
                    &alpha,
                    &cmd.alphaOrShadeBits,
                    sizeof(float)
                );
                if (alpha < 1.0f) {
                    Renderer_DrawPolyTLV(
                        cmd.materialRef,
                        polyVerts,
                        triVerts,
                        triUVs,
                        cmd.vertexCount,
                        alpha,
                        cmd.texKey
                    );
                } else {
                    zRndr_DrawFlatQueued(
                        cmd.materialRef,
                        polyVerts,
                        triVerts,
                        triUVs,
                        cmd.vertexCount,
                        cmd.texKey
                    );
                }
            } else {
                zRndr_DrawTexturedFanTri(
                    cmd.materialRef,
                    polyVerts,
                    clippedTriVerts,
                    triVerts,
                    triUVs,
                    cmd.vertexCount,
                    cmd.alphaOrShadeBits,
                    cmd.texKey
                );
            }
        }
    }

    zRndr::g_transparentQueueCount = 0;
}

/**
 * Reimplements 0x49a490: zRndr_FlushOverwriteQueue
 * Source file evidence: zRndr queued draw cluster in this source file.
 * Purpose: Draw queued overwrite polygons through the appropriate flat or textured paths.
 */
void zRndr_FlushOverwriteQueue() {
    zRndr::g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanListFast;

    {
        for (int queueIndex = 0; queueIndex < zRndr::g_overwriteQueueCount; ++queueIndex) {
            zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
            const int commandTag = cmd.commandTag;
            zVec3 *polyVerts = (zVec3 *)(cmd.polyVerts);
            zVec3 *clippedTriVerts =
                cmd.hasClippedTriVerts != 0
                    ? (zVec3 *)(cmd.clippedTriVertOverlay.clippedTriVerts)
                    : 0;
            zVec3 *triVerts = (zVec3 *)(cmd.triVerts);
            zVec2 *triUVs = (zVec2 *)(cmd.triUVs);
            float *perVertexAlphaOrShadeF = cmd.perVertexAlphaOrShadeF;
            const int texKey = cmd.texKey;

            zRndr::g_inverseDepthBias = cmd.savedInvDepthBias;
            zRndr::g_inverseDepthScale = cmd.savedInvDepthScale;
            zRndr::g_scanConvertMode = cmd.scanConvertMode;

            zVidImagePartial *image = cmd.materialRef != 0 ? cmd.materialRef->image : 0;
            bool useFallback = false;

            if (commandTag == 0) {
                if (cmd.alphaOrShadeF >= 255.0f) {
                    zRndr_RasterizePolyWithSpanList(
                        polyVerts,
                        triVerts,
                        cmd.vertexCount,
                        cmd.shadeOrSpanMode
                    );
                    continue;
                }
                useFallback = true;
            } else if (commandTag == 1) {
                if (image != 0 && (image->formatFlagsPacked & 2) == 0 &&
                    cmd.alphaOrShadeF >= 1.0f) {
                    zRndr_DrawTexturedQueuedAlpha(
                        cmd.materialRef,
                        polyVerts,
                        clippedTriVerts,
                        triVerts,
                        triUVs,
                        cmd.vertexCount,
                        texKey
                    );
                    continue;
                }

                if (image != 0 && (image->formatFlagsPacked & 2) != 0) {
                    cmd.alphaOrShadeF *= 255.0f;
                }
                useFallback = true;
            } else if (commandTag == 2) {
                if (image != 0 && (image->formatFlagsPacked & 2) == 0) {
                    if (cmd.vertexCount - 2 > 0) {
                        zVec3 fanVerts[3];
                        zVec3 shadeTriplet;
                        fanVerts[0] = polyVerts[0];
                        shadeTriplet.x = perVertexAlphaOrShadeF[0];
                        for (int fanTriIndex = 0; fanTriIndex < cmd.vertexCount - 2;
                            ++fanTriIndex) {
                            fanVerts[1] = polyVerts[fanTriIndex + 1];
                            fanVerts[2] = polyVerts[fanTriIndex + 2];
                            shadeTriplet.y = perVertexAlphaOrShadeF[fanTriIndex + 1];
                            shadeTriplet.z = perVertexAlphaOrShadeF[fanTriIndex + 2];
                            zRndr_DrawTexturedQueued(
                                cmd.materialRef,
                                fanVerts,
                                clippedTriVerts,
                                triVerts,
                                triUVs,
                                &shadeTriplet,
                                3,
                                fanTriIndex,
                                texKey
                            );
                        }
                    }
                    continue;
                }

                cmd.alphaOrShadeF = 255.0f;
                useFallback = true;
            }

            if (!useFallback) {
                continue;
            }

            if (image == 0) {
                zRndr_DrawFlatImmediate(
                    polyVerts,
                    triVerts,
                    cmd.vertexCount,
                    (int)(cmd.alphaOrShadeF),
                    cmd.shadeOrSpanMode
                );
            } else if ((image->formatFlagsPacked & 2) == 0) {
                if (cmd.alphaOrShadeF < 1.0f) {
                    Renderer_DrawPolyTLV(
                        cmd.materialRef,
                        polyVerts,
                        triVerts,
                        triUVs,
                        cmd.vertexCount,
                        cmd.alphaOrShadeF,
                        texKey
                    );
                } else {
                    zRndr_DrawFlatQueued(
                        cmd.materialRef,
                        polyVerts,
                        triVerts,
                        triUVs,
                        cmd.vertexCount,
                        texKey
                    );
                }
            } else {
                zRndr_DrawTexturedFanTri(
                    cmd.materialRef,
                    polyVerts,
                    clippedTriVerts,
                    triVerts,
                    triUVs,
                    cmd.vertexCount,
                    (int)(cmd.alphaOrShadeF),
                    texKey
                );
            }
        }
    }

    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
    zRndr::g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanList;
}

/**
 * Reimplements 0x499130: zRndr_TextureMip_SelectVariantImage
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Select a mip/variant image for a textured polygon from its projected texture metric.
 */
zVidImagePartial *__fastcall zRndr_TextureMip_SelectVariantImage(
    zImage_TexDirEntryPartial *entry,
    const zVec3 *triVerts,
    int vertCount,
    const zVec2 *vertexUvPairs,
    const zVec2 *mipParamsA,
    const zVec2 *mipParamsB,
    const zVec2 *mipParamsC
) {
    if (zRndr::g_textureMipSelectionEnabled == 0) {
        return entry != 0 ? entry->image : 0;
    }

    float selectedZ = triVerts[0].z;
    const zVec3 *candidateVertex = triVerts + 1;
    int selectedVertex = 0;
    int i = 1;
    for (; i < vertCount; ++i, ++candidateVertex) {
        if (selectedZ < candidateVertex->z) {
            selectedZ = candidateVertex->z;
            selectedVertex = i;
        }
    }

    const float selectedVertexZ = triVerts[selectedVertex].z;
    const float invZ = 1.0f / selectedVertexZ;
    const float invZAtX = 1.0f / (selectedVertexZ + mipParamsA->x);
    const float invZAtY = 1.0f / (selectedVertexZ + mipParamsA->y);
    const float uOverZ = vertexUvPairs[selectedVertex].x * invZ;
    const float vOverZ = vertexUvPairs[selectedVertex].y * invZ;

    const float mipDeltas[4] = {
        (vertexUvPairs[selectedVertex].x + mipParamsB->x) * invZAtX - uOverZ,
        (vertexUvPairs[selectedVertex].x + mipParamsB->y) * invZAtY - uOverZ,
        (vertexUvPairs[selectedVertex].y + mipParamsC->x) * invZAtX - vOverZ,
        (vertexUvPairs[selectedVertex].y + mipParamsC->y) * invZAtY - vOverZ
    };

    float mipMetric = mipDeltas[0];
    if (mipMetric < mipDeltas[1]) {
        mipMetric = mipDeltas[1];
    }
    if (mipMetric < mipDeltas[2]) {
        mipMetric = mipDeltas[2];
    }
    if (mipMetric < mipDeltas[3]) {
        mipMetric = mipDeltas[3];
    }

    const double variantIndexBits = (double)(mipMetric) - -6755399441055744.0;
    const int variantIndex = (*(int *)(&variantIndexBits)) >> 1;
    return entry->GetVariantImageAtIndex(variantIndex);
}

/**
 * Reimplements 0x493df0: zRndr_DrawFlatQueued
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Draw a queued flat/textured polygon through the active span callback path.
 */
void __fastcall zRndr_DrawFlatQueued(
    zImage_TexDirEntryPartial *entry,
    zVec3 *polyVerts,
    zVec3 *triVerts,
    zVec2 *triUVs,
    int vertCount,
    int paletteIndex
) {
    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes = BuildQueuedTexturePlanes(
        0,
        triVerts,
        triUVs,
        imageWidth,
        imageHeight
    );

    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry,
            triVerts,
            3,
            vertexUvPairs,
            &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient,
            &planes.vOverZ.gradient
        );
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    for (int i_4366 = 1; i_4366 < vertCount; ++i_4366) {
        if (polyVerts[i_4366].y < polyVerts[topVertexIndex].y) {
            topVertexIndex = i_4366;
        }
        if (polyVerts[i_4366].y >= polyVerts[bottomVertexIndex].y) {
            bottomVertexIndex = i_4366;
        }
    }

    ScanConvertEdge edgeTableA[0x40] = {0};
    ScanConvertEdge edgeTableB[0x40] = {0};
    int edgeCountA;
    int edgeCountB;
    if (zRndr::g_scanConvertMode != 0) {
        edgeCountA = BuildScanConvertEdges(
            polyVerts,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            1,
            edgeTableA
        );
        edgeCountB = BuildScanConvertEdges(
            polyVerts,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            -1,
            edgeTableB
        );
    } else {
        edgeCountA = BuildScanConvertEdges(
            polyVerts,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            -1,
            edgeTableA
        );
        edgeCountB = BuildScanConvertEdges(
            polyVerts,
            vertCount,
            topVertexIndex,
            bottomVertexIndex,
            1,
            edgeTableB
        );
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    const int firstScanline = ScanlineStartFromY(polyVerts[topVertexIndex].y);
    const int lastScanline = ScanlineEndFromY(polyVerts[bottomVertexIndex].y);
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char *)(selectedImage->pixels);

    zRndr::TexturedQueuedSpanProc spanOpMode0;
    zRndr::TexturedQueuedSpanProc spanOpMode1;
    if (selectedImage->alphaMap != 0) {
        zRndr::g_spanActiveTexAlphaMap = selectedImage->alphaMap;
        spanOpMode0 = zRndr::g_pfnFlatQueuedSpanOp_Mode0;
        spanOpMode1 = zRndr::g_pfnFlatQueuedSpanOpAlt_Mode0;
    } else {
        zRndr::g_spanActiveTexAlphaMap = 0;
        spanOpMode0 = zRndr::g_pfnFlatQueuedSpanOp_Mode1;
        spanOpMode1 = zRndr::g_pfnFlatQueuedSpanOpAlt_Mode1;
    }
    zRndr::g_pfnSelectedSpanOp_Mode0 = spanOpMode0;
    zRndr::g_pfnSelectedSpanOp_Mode1 = spanOpMode1;

    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnSelectedSpanOp_Mode0;
    unsigned short *palette = (unsigned short *)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette =
            paletteIndex == -1 ? palette : &palette[(paletteIndex + 1) * 0x100];
        spanProc = zRndr::g_pfnSelectedSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char *scanlineBase =
        (unsigned char *)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;
    const int texVShift = zRndr::g_spanActiveTexShift;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        while (edgeIndexA < edgeCountA && y >= edgeTableA[edgeIndexA].yStart) {
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            ++edgeIndexA;
        }

        while (edgeIndexB < edgeCountB && y >= edgeTableB[edgeIndexB].yStart) {
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            ++edgeIndexB;
        }

        int xMin;
        int xMax;
        if (currentXFixedA > currentXFixedB) {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            const float rowReciprocalZ =
                (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanListSecondary(
                visibleSpans,
                y,
                &spanCount
            );
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr =
                        (unsigned short *)(scanlineBase +
                                           (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    DispatchTexturedSpanChunks(
                        spanProc,
                        planes,
                        0,
                        span,
                        y,
                        span->sampleXMax - span->sampleXMin + 1,
                        1048576.0f,
                        texVShift
                    );
                }
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
    }
}

/**
 * Reimplements 0x495850: zRndr_DrawTexturedQueued
 * Purpose: Draw a depth-sorted textured polygon using perspective-correct queued spans.
 */
void __fastcall zRndr_DrawTexturedQueued(
    zImage_TexDirEntryPartial *entry,
    zVec3 *projectedVerts,
    zVec3 *clippedTriVerts,
    zVec3 *triVerts,
    zVec2 *triUVs,
    zVec3 *shadeTriplet,
    int vertCount,
    int,
    int texKey
) {
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 || triVerts == 0 || triUVs == 0 ||
        shadeTriplet == 0 || vertCount <= 0 || zRndr::g_spanAllocCursor == 0 ||
        zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);
    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(
            clippedTriVerts,
            triVerts,
            triUVs,
            imageWidth,
            imageHeight
        );

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry,
            triVerts,
            3,
            vertexUvPairs,
            &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient,
            &planes.vOverZ.gradient
        );
        if (selectedImage == 0) {
            return;
        }
        textureScale =
            1048576.0f / (selectedImage->widthScale != 0.0f ? selectedImage->widthScale : 1.0f);
    }

    const float shadeValues[3] = {
        shadeTriplet->x,
        shadeTriplet->y,
        shadeTriplet->z,
    };
    Plane2f shadePlane = BuildScreenPlaneFromTriangle(
        projectedVerts,
        shadeValues
    );

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    float minPositiveReciprocalZ = 1000.0f;
    for (int i_4544 = 0; i_4544 < vertCount; ++i_4544) {
        const float reciprocalZ =
            EvalPlane(
                planes.reciprocalZ,
                projectedVerts[i_4544].x,
                projectedVerts[i_4544].y
            );
        if (reciprocalZ > 0.0f && reciprocalZ < minPositiveReciprocalZ) {
            minPositiveReciprocalZ = reciprocalZ;
        }
        if (projectedVerts[i_4544].y < projectedVerts[topVertexIndex].y) {
            topVertexIndex = i_4544;
        }
        if (projectedVerts[i_4544].y >= projectedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = i_4544;
        }
    }

    const int firstScanline = (int)(floor(projectedVerts[topVertexIndex].y + 0.5f));
    const int lastScanline = (int)(floor(projectedVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char *)(selectedImage->pixels);
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode0;
    Plane2f *activeShadePlane = 0;

    unsigned short *palette = (unsigned short *)(selectedImage->palette);
    if (palette != 0) {
        spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode1;
        if (texKey != -1) {
            zRndr::g_spanActiveTexPalette = &palette[(texKey + 1) * 0x100];
        } else {
            int shadeRecipe = g_zRndr_ActivePaletteShadeRecipeIndex;
            if (shadeRecipe < 0) {
                shadeRecipe = zVid_PaletteRemap_FindRecipeIndexFromRgb(
                    (zColorRgb *)(zRndr::g_fogParamsActive.colorRgb01)
                );
            }

            if (shadeRecipe >= 0) {
                spanProc = zRndr::SpanShade16FromPal8SwitchVShift;
                zRndr::g_spanActiveTexPalette = &palette[0x100 + shadeRecipe * 0x2000];
                activeShadePlane = &shadePlane;
            } else {
                zRndr::g_spanActiveTexPalette = palette;
            }
        }
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    zRndr::SpanBuildProc buildProc = zRndr::g_pfnBuildSpanList != 0
                                         ? zRndr::g_pfnBuildSpanList
                                         : zRndr_SpanOcclusion_InsertSpanNode_Local;
    unsigned char *frameBase = (unsigned char *)(zRndr::g_frameBuffer);
    const int chunkPixels =
        SelectPerspectiveChunkPixels(
            minPositiveReciprocalZ,
            planes.reciprocalZ.gradient.x
        );
    const int texVShift = zRndr::g_spanActiveTexShift;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = (float)(y) + 0.5f;
        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = projectedVerts[i];
            const zVec3 &b = projectedVerts[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }
            const float minY = MinValue(
                a.y,
                b.y
            );
            const float maxY = MaxValue(
                a.y,
                b.y
            );
            if (sampleY < minY || sampleY >= maxY) {
                continue;
            }
            const float t = (sampleY - a.y) / (b.y - a.y);
            intersections[intersectionCount++] = a.x + (b.x - a.x) * t;
            if (intersectionCount == 0x40) {
                break;
            }
        }

        if (intersectionCount < 2) {
            continue;
        }

        sort(
            intersections,
            intersections + intersectionCount
        );
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_4640 = 0; i_4640 < pairCount; i_4640 += 2) {
            const int xMin = (int)(floor(intersections[i_4640] + 0.5f));
            const int xMax = (int)(ceil(intersections[i_4640 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(
                visibleSpans,
                y,
                &spanCount
            );
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr =
                        (unsigned short *)(scanlineBase +
                                           (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    DispatchTexturedSpanChunks(
                        spanProc,
                        planes,
                        activeShadePlane,
                        span,
                        y,
                        chunkPixels,
                        textureScale,
                        texVShift
                    );
                }
            }
        }
    }
}

/**
 * Reimplements 0x494af0: Renderer_DrawPolyTLV
 * Purpose: Draw a transformed lit polygon through the active software texture span path.
 */
void __fastcall Renderer_DrawPolyTLV(
    zImage_TexDirEntryPartial *entry,
    zVec3 *polyVerts,
    zVec3 *triVerts,
    zVec2 *triUVs,
    int vertexCount,
    float alpha,
    int texKey
) {
    if (entry == 0 || entry->image == 0 || polyVerts == 0 || triVerts == 0 || triUVs == 0 ||
        vertexCount <= 0 || zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);
    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes = BuildQueuedTexturePlanes(
        0,
        triVerts,
        triUVs,
        imageWidth,
        imageHeight
    );

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry,
            triVerts,
            3,
            vertexUvPairs,
            &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient,
            &planes.vOverZ.gradient
        );
        if (selectedImage == 0) {
            return;
        }
        textureScale =
            1048576.0f / (selectedImage->widthScale != 0.0f ? selectedImage->widthScale : 1.0f);
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    for (int i_4720 = 1; i_4720 < vertexCount; ++i_4720) {
        if (polyVerts[i_4720].y < polyVerts[topVertexIndex].y) {
            topVertexIndex = i_4720;
        }
        if (polyVerts[i_4720].y >= polyVerts[bottomVertexIndex].y) {
            bottomVertexIndex = i_4720;
        }
    }

    const int firstScanline = ScanlineStartFromY(polyVerts[topVertexIndex].y);
    const int lastScanline = ScanlineEndFromY(polyVerts[bottomVertexIndex].y);
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char *)(selectedImage->pixels);

    zRndr::TexturedQueuedSpanProc spanProc = 0;
    zRndr::TexturedQueuedSpanProc paletteSpanProc = 0;
    if (selectedImage->alphaMap != 0) {
        zRndr::g_spanActiveTexAlphaMap = selectedImage->alphaMap;
        int alphaBits = 0;
        memcpy(
            &alphaBits,
            &alpha,
            sizeof(alphaBits)
        );
        zRndr::g_spanActiveConstAlphaBits = alphaBits;
        spanProc = zRndr::g_pfnPolyTlvSpanOp_Mode0;
        paletteSpanProc = zRndr::g_pfnPolyTlvSpanOpAlt_Mode0;
    } else {
        zRndr::g_spanActiveTexAlphaMap = 0;
        const double alphaScaled = (double)(alpha) * 255.0;
        zRndr::g_spanActiveConstAlphaBits =
            (int)(alphaScaled >= 0.0 ? alphaScaled + 0.5 : alphaScaled - 0.5);
        spanProc = zRndr::g_pfnPolyTlvSpanOp_Mode1;
        paletteSpanProc = zRndr::g_pfnPolyTlvSpanOpAlt_Mode1;
    }
    zRndr::g_pfnSelectedSpanOp_Mode0 = spanProc;
    zRndr::g_pfnSelectedSpanOp_Mode1 = paletteSpanProc;
    spanProc = zRndr::g_pfnSelectedSpanOp_Mode0;

    unsigned short *palette = (unsigned short *)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette = texKey == -1 ? palette : &palette[(texKey + 1) * 0x100];
        spanProc = zRndr::g_pfnSelectedSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    zRndr::SpanBuildProc buildProc =
        zRndr::g_pfnBuildSpanListSecondary != 0
            ? zRndr::g_pfnBuildSpanListSecondary
            : (zRndr::g_pfnBuildSpanList != 0 ? zRndr::g_pfnBuildSpanList
                                              : zRndr_SpanOcclusion_InsertSpanNode_Local);
    unsigned char *frameBase = (unsigned char *)(zRndr::g_frameBuffer);
    const int texVShift = zRndr::g_spanActiveTexShift;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = (float)(y) + 0.5f;
        for (int i = 0; i < vertexCount; ++i) {
            const zVec3 &a = polyVerts[i];
            const zVec3 &b = polyVerts[(i + 1) % vertexCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(
                a.y,
                b.y
            );
            const float maxY = MaxValue(
                a.y,
                b.y
            );
            if (sampleY < minY || sampleY >= maxY) {
                continue;
            }

            const float t = (sampleY - a.y) / (b.y - a.y);
            intersections[intersectionCount++] = a.x + (b.x - a.x) * t;
            if (intersectionCount == 0x40) {
                break;
            }
        }

        if (intersectionCount < 2) {
            continue;
        }

        sort(
            intersections,
            intersections + intersectionCount
        );
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_4808 = 0; i_4808 < pairCount; i_4808 += 2) {
            const int xMin = SpanStartFromX(intersections[i_4808]);
            const int xMax = SpanEndFromX(intersections[i_4808 + 1]);
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(
                visibleSpans,
                y,
                &spanCount
            );
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr =
                        (unsigned short *)(scanlineBase +
                                           (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    DispatchTexturedSpanChunks(
                        spanProc,
                        planes,
                        0,
                        span,
                        y,
                        span->sampleXMax - span->sampleXMin + 1,
                        textureScale,
                        texVShift
                    );
                }
            }
        }
    }
}

/**
 * Reimplements 0x4969d0: zRndr_DrawTexturedQueuedAlpha
 * Purpose: Queue an alpha-blended textured polygon for deferred depth-sorted rendering.
 */
void __fastcall zRndr_DrawTexturedQueuedAlpha(
    zImage_TexDirEntryPartial *entry,
    zVec3 *projectedVerts,
    zVec3 *clippedTriVerts,
    zVec3 *triVerts,
    zVec2 *triUVs,
    int vertCount,
    int variantIndex
) {
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 || triVerts == 0 || triUVs == 0 ||
        vertCount <= 0 || zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(
            clippedTriVerts,
            triVerts,
            triUVs,
            imageWidth,
            imageHeight
        );

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry,
            triVerts,
            3,
            vertexUvPairs,
            &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient,
            &planes.vOverZ.gradient
        );
        if (selectedImage == 0) {
            return;
        }

        const float widthScale =
            selectedImage->widthScale != 0.0f ? selectedImage->widthScale : 1.0f;
        textureScale = 1048576.0f / widthScale;
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    float minPositiveReciprocalZ = 1000.0f;
    for (int i_4890 = 0; i_4890 < vertCount; ++i_4890) {
        const float reciprocalZ =
            EvalPlane(
                planes.reciprocalZ,
                projectedVerts[i_4890].x,
                projectedVerts[i_4890].y
            );
        if (reciprocalZ > 0.0f && reciprocalZ < minPositiveReciprocalZ) {
            minPositiveReciprocalZ = reciprocalZ;
        }

        if (projectedVerts[i_4890].y < projectedVerts[topVertexIndex].y) {
            topVertexIndex = i_4890;
        }
        if (projectedVerts[i_4890].y >= projectedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = i_4890;
        }
    }

    const int firstScanline = (int)(floor(projectedVerts[topVertexIndex].y + 0.5f));
    const int lastScanline = (int)(floor(projectedVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char *)(selectedImage->pixels);
    zRndr::g_spanQueuedTexAlphaMap = selectedImage->queuedAlphaMap;
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode0;

    unsigned short *palette = (unsigned short *)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette =
            variantIndex == -1 ? palette : &palette[(variantIndex + 1) * 0x100];
        spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    zRndr::SpanBuildProc buildProc = zRndr::g_pfnBuildSpanList != 0
                                         ? zRndr::g_pfnBuildSpanList
                                         : zRndr_SpanOcclusion_InsertSpanNode_Local;
    unsigned char *frameBase = (unsigned char *)(zRndr::g_frameBuffer);
    const int chunkPixels =
        SelectPerspectiveChunkPixels(
            minPositiveReciprocalZ,
            planes.reciprocalZ.gradient.x
        );
    const int texVShift = zRndr::g_spanActiveTexShift;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = (float)(y) + 0.5f;
        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = projectedVerts[i];
            const zVec3 &b = projectedVerts[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(
                a.y,
                b.y
            );
            const float maxY = MaxValue(
                a.y,
                b.y
            );
            if (sampleY < minY || sampleY >= maxY) {
                continue;
            }

            const float t = (sampleY - a.y) / (b.y - a.y);
            intersections[intersectionCount++] = a.x + (b.x - a.x) * t;
            if (intersectionCount == 0x40) {
                break;
            }
        }

        if (intersectionCount < 2) {
            continue;
        }

        sort(
            intersections,
            intersections + intersectionCount
        );
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_4973 = 0; i_4973 < pairCount; i_4973 += 2) {
            const int xMin = (int)(floor(intersections[i_4973] + 0.5f));
            const int xMax = (int)(ceil(intersections[i_4973 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(
                visibleSpans,
                y,
                &spanCount
            );
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr =
                        (unsigned short *)(scanlineBase +
                                           (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    DispatchTexturedSpanChunks(
                        spanProc,
                        planes,
                        0,
                        span,
                        y,
                        chunkPixels,
                        textureScale,
                        texVShift
                    );
                }
            }
        }
    }
}

/**
 * Reimplements 0x497ac0: zRndr_DrawTexturedFanTri
 * Purpose: Draw one textured triangle from a fan using the selected active span callback.
 */
void __fastcall zRndr_DrawTexturedFanTri(
    zImage_TexDirEntryPartial *entry,
    zVec3 *projectedVerts,
    zVec3 *clippedTriVerts,
    zVec3 *triVerts,
    zVec2 *triUVs,
    int vertCount,
    int alpha255,
    int variantIndex
) {
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 || triVerts == 0 || triUVs == 0 ||
        vertCount <= 0 || zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(
            clippedTriVerts,
            triVerts,
            triUVs,
            imageWidth,
            imageHeight
        );

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry,
            triVerts,
            3,
            vertexUvPairs,
            &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient,
            &planes.vOverZ.gradient
        );
        if (selectedImage == 0) {
            return;
        }

        const float widthScale =
            selectedImage->widthScale != 0.0f ? selectedImage->widthScale : 1.0f;
        textureScale = 1048576.0f / widthScale;
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    float minPositiveReciprocalZ = 1000.0f;
    for (int i_5057 = 0; i_5057 < vertCount; ++i_5057) {
        const float reciprocalZ =
            EvalPlane(
                planes.reciprocalZ,
                projectedVerts[i_5057].x,
                projectedVerts[i_5057].y
            );
        if (reciprocalZ > 0.0f && reciprocalZ < minPositiveReciprocalZ) {
            minPositiveReciprocalZ = reciprocalZ;
        }

        if (projectedVerts[i_5057].y < projectedVerts[topVertexIndex].y) {
            topVertexIndex = i_5057;
        }
        if (projectedVerts[i_5057].y >= projectedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = i_5057;
        }
    }

    const int firstScanline = (int)(floor(projectedVerts[topVertexIndex].y + 0.5f));
    const int lastScanline = (int)(floor(projectedVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char *)(selectedImage->pixels);
    zRndr::g_spanQueuedTexAlphaMap = selectedImage->queuedAlphaMap;
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::g_spanActiveConstAlphaBits = alpha255;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedFanTriSpanOp_Mode0;

    unsigned short *palette = (unsigned short *)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette =
            variantIndex == -1 ? palette : &palette[(variantIndex + 1) * 0x100];
        spanProc = zRndr::g_pfnTexturedFanTriSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    unsigned char *frameBase = (unsigned char *)(zRndr::g_frameBuffer);
    const int chunkPixels =
        SelectPerspectiveChunkPixels(
            minPositiveReciprocalZ,
            planes.reciprocalZ.gradient.x
        );
    const int texVShift = zRndr::g_spanActiveTexShift;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = (float)(y) + 0.5f;
        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = projectedVerts[i];
            const zVec3 &b = projectedVerts[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(
                a.y,
                b.y
            );
            const float maxY = MaxValue(
                a.y,
                b.y
            );
            if (sampleY < minY || sampleY >= maxY) {
                continue;
            }

            const float t = (sampleY - a.y) / (b.y - a.y);
            intersections[intersectionCount++] = a.x + (b.x - a.x) * t;
            if (intersectionCount == 0x40) {
                break;
            }
        }

        if (intersectionCount < 2) {
            continue;
        }

        sort(
            intersections,
            intersections + intersectionCount
        );
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_5143 = 0; i_5143 < pairCount; i_5143 += 2) {
            const int xMin = (int)(floor(intersections[i_5143] + 0.5f));
            const int xMax = (int)(ceil(intersections[i_5143 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanListSecondary(
                visibleSpans,
                y,
                &spanCount
            );
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr =
                        (unsigned short *)(scanlineBase +
                                           (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    DispatchTexturedSpanChunks(
                        spanProc,
                        planes,
                        0,
                        span,
                        y,
                        chunkPixels,
                        textureScale,
                        texVShift
                    );
                }
            }
        }
    }
}

/**
 * Reimplements 0x498bd0: zRndr_DrawImmediateLine
 * Source file evidence: zRndr immediate line draw cluster in this source file.
 * Purpose: Dispatch one unclipped immediate line to the selected software line raster routine.
 */
void __fastcall zRndr_DrawImmediateLine(
    int x0,
    int y0,
    int x1,
    int y1,
    int color16
) {
    zRndr::g_pfnImmediateRaster4(
        (unsigned short *)(zRndr::g_frameBuffer),
        x0,
        y0,
        x1,
        y1,
        color16
    );
}

/**
 * Reimplements 0x498c00: zRndr_DrawClippedImmediateLineStrip
 * Source file evidence: zRndr immediate line draw cluster in this source file.
 * Purpose: Dispatch each segment of a clipped immediate line strip to the selected raster routine.
 */
void __fastcall zRndr_DrawClippedImmediateLineStrip(
    const zRndr_LinePoint2I *points,
    int segmentCount,
    const void *clipRect,
    int color16
) {
    if (segmentCount <= 0) {
        return;
    }

    const zRndr_LineClipRect2I *clip = (const zRndr_LineClipRect2I *)(clipRect);
    const zRndr_LinePoint2I *point = points + 1;
    int remaining = segmentCount;
    do {
        zRndr::g_pfnImmediateRaster5(
            (unsigned short *)(zRndr::g_frameBuffer),
            clip,
            point[-1].x,
            point[-1].y,
            point[0].x,
            point[0].y,
            color16
        );
        ++point;
        --remaining;
    } while (remaining != 0);
}

/**
 * Reimplements 0x49a830: zRndr_LensFlare_QueueProjectedSample
 * Purpose: Queue a projected lens-flare sample after applying the active inverse-depth transform.
 */
void __fastcall zRndr_LensFlare_QueueProjectedSample(
    zProjectedPoint *projectedPoint,
    int packedColor16,
    int lensFlareSource
) {
    if (zRndr::g_lensFlareSampleQueueCount >= 0x28a) {
        return;
    }

    projectedPoint->reciprocalZ =
        zRndr::g_inverseDepthBias + zRndr::g_inverseDepthScale * projectedPoint->reciprocalZ;

    zRndr::LensFlareSamplePartial *sample =
        &zRndr::g_lensFlareSampleQueue[zRndr::g_lensFlareSampleQueueCount];
    sample->x = projectedPoint->x;
    sample->y = projectedPoint->y;
    sample->reciprocalZ = projectedPoint->reciprocalZ;
    sample->packedColor16 = packedColor16;
    sample->lensFlareSource = lensFlareSource;
    ++zRndr::g_lensFlareSampleQueueCount;
}

/**
 * Reimplements 0x49a8b0: zRndr_LensFlare_GetQueuedSampleCount
 * Purpose: Return the number of lens-flare samples queued for the frame.
 */
int zRndr_LensFlare_GetQueuedSampleCount() {
    return zRndr::g_lensFlareSampleQueueCount;
}

/**
 * Reimplements 0x49a920: zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList
 * Source file evidence: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Cull queued lens-flare samples and build the visible-sample list for 16-bit drawing.
 */
void __fastcall zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList(
    int startIndex
) {
    if (startIndex >= zRndr::g_lensFlareSampleQueueCount) {
        return;
    }

    zRndr::LensFlareSamplePartial *sample = &zRndr::g_lensFlareSampleQueue[startIndex];
    while (startIndex < zRndr::g_lensFlareSampleQueueCount) {
        if (zRndr_SpanOcclusion_TestPointVisibility((zVec3 *)(sample)) == 0) {
            const int newCount = zRndr::g_lensFlareSampleQueueCount - 1;
            zRndr::g_lensFlareSampleQueueCount = newCount;
            if (startIndex >= newCount) {
                break;
            }

            memcpy(
                sample,
                &zRndr::g_lensFlareSampleQueue[newCount],
                sizeof(*sample)
            );
            continue;
        }

        zRndr_LensFlareSource *source = (zRndr_LensFlareSource *)(sample->lensFlareSource);
        if (source != 0 && source->lensFlareEnabled != 0 &&
            sample < &zRndr::g_lensFlareSampleQueue[0x40] && sample->reciprocalZ != 0.0f) {
            zRndr::g_lensFlareVisibleSampleDefs[zRndr::g_lensFlareVisibleSampleCount] =
                (zRndr_LensFlareVisibleSampleDef *)(sample);
            ++zRndr::g_lensFlareVisibleSampleCount;
        }

        ++startIndex;
        ++sample;
    }
}

/**
 * Reimplements 0x49a9c0: zRndr_LensFlare::BuildVisibleSampleListFromQueue
 * Source file evidence: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Build the visible lens-flare sample list from queued samples without visibility testing.
 */
int __fastcall zRndr_LensFlare_BuildVisibleSampleListFromQueue(
    int startIndex
) {
    int visibleSampleCount = 0;
    zRndr::g_lensFlareVisibleSampleCount = 0;
    if (startIndex >= zRndr::g_lensFlareSampleQueueCount) {
        return visibleSampleCount;
    }

    zRndr::LensFlareSamplePartial *sample = &zRndr::g_lensFlareSampleQueue[startIndex];
    while (startIndex < zRndr::g_lensFlareSampleQueueCount) {
        zRndr_LensFlareSource *source = (zRndr_LensFlareSource *)(sample->lensFlareSource);
        if (source != 0 && source->lensFlareEnabled != 0 &&
            sample < &zRndr::g_lensFlareSampleQueue[0x40] && sample->reciprocalZ != 0.0f) {
            zRndr::g_lensFlareVisibleSampleDefs[visibleSampleCount] =
                (zRndr_LensFlareVisibleSampleDef *)(sample);
            visibleSampleCount = zRndr::g_lensFlareVisibleSampleCount + 1;
            zRndr::g_lensFlareVisibleSampleCount = visibleSampleCount;
        }

        ++startIndex;
        ++sample;
    }

    return visibleSampleCount;
}

/**
 * Reimplements 0x49aa40: zRndr_LensFlare_SetVisibleSampleStage
 * Purpose: Store one lens-flare stage texture and refresh the visibility-active flag.
 */
void __fastcall zRndr_LensFlare_SetVisibleSampleStage(
    int stageIndex,
    zImage_TexDirEntryPartial *stageTexDirEntry
) {
    if (stageIndex >= 0 && stageIndex < 4) {
        zRndr::g_lensFlareVisibleSampleStages[stageIndex] = stageTexDirEntry;
    }

    if (zRndr::g_lensFlareVisibleSampleStages[0] != 0 &&
        zRndr::g_lensFlareVisibleSampleStages[1] != 0 &&
        zRndr::g_lensFlareVisibleSampleStages[2] != 0 &&
        zRndr::g_lensFlareVisibleSampleStages[3] != 0) {
        zRndr::g_lensFlareVisibilityActive = 1;
    } else {
        zRndr::g_lensFlareVisibilityActive = 0;
    }
}

/**
 * Reimplements 0x49aa90: zRndr_LensFlare_DrawSampleStageClipped
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_LensFlare.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Draw one clipped lens-flare stage quad through hardware or software rendering.
 */
void __fastcall zRndr_LensFlare_DrawSampleStageClipped(
    const zVec2 *sampleCenter,
    zImage_TexDirEntryPartial *stageTexDirEntry,
    float sampleRadius,
    const zRndr_LineClipRect2I *clipRect
) {
    if (sampleRadius < 1.0f) {
        return;
    }

    float left = sampleCenter->x - sampleRadius;
    float top = sampleCenter->y - sampleRadius;
    float right = sampleCenter->x + sampleRadius;
    float bottom = sampleCenter->y + sampleRadius;

    float clipLeft = 0.0f;
    float clipTop = 0.0f;
    float clipRight = 0.0f;
    float clipBottom = 0.0f;
    if (clipRect != 0) {
        clipLeft = (float)(clipRect->left);
        clipTop = (float)(clipRect->top);
        clipRight = (float)(clipRect->right);
        clipBottom = (float)(clipRect->bottom);
        if (left > clipRight - 2.0f || top > clipBottom - 2.0f || right < clipLeft + 1.0f ||
            bottom < clipTop + 1.0f) {
            return;
        }
    } else {
        clipRight = (float)(zRndr::g_activeRegionWidth);
        clipBottom = (float)(zRndr::g_activeRegionHeight);
        if (left > clipRight - 2.0f || top > clipBottom - 2.0f || right < 1.0f || bottom < 1.0f) {
            return;
        }
    }

    const float uvScale = 0.5f / sampleRadius;
    float uLeft = 0.0f;
    float uRight = 1.0f;
    float vTop = 1.0f;
    float vBottom = 0.0f;

    if (left < clipLeft) {
        uLeft = (clipLeft - left) * uvScale;
        left = clipLeft;
    }

    if (top < clipTop) {
        vTop = 1.0f - (clipTop - top) * uvScale;
        top = clipTop;
    }

    const float rightMax = clipRight - 1.0f;
    if (right > rightMax) {
        uRight = 1.0f - ((right + 1.0f) - clipRight) * uvScale;
        right = rightMax;
    }

    const float bottomMax = clipBottom - 1.0f;
    if (bottom > bottomMax) {
        vBottom = ((bottom + 1.0f) - clipBottom) * uvScale;
        bottom = bottomMax;
    }

    zVec3 projectedVerts[4] = {
        {right, bottom, 0.5f},
        {right, top, 0.5f},
        {left, top, 0.5f},
        {left, bottom, 0.5f},
    };
    zVec2 triUVs[4] = {
        {uRight, vBottom},
        {uRight, vTop},
        {uLeft, vTop},
        {uLeft, vBottom},
    };

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo_SubmitPolyRenderClassProc submitPoly = g_zVideo_pfnSubmitPolyRenderClass;
        zVideo_RenderClass *renderClass =
            stageTexDirEntry != 0 ? (zVideo_RenderClass *)(stageTexDirEntry->texture) : 0;
        submitPoly(
            (zVideo_XyzVertex *)(projectedVerts),
            (zVideo_TexCoord *)(triUVs),
            4,
            renderClass,
            0x10,
            1.0f,
            0
        );
        return;
    }

    const float kSoftwareScale = 0.100000001f;
    zVec3 clippedTriVerts[4] = {
        {right * kSoftwareScale, bottom * kSoftwareScale, kSoftwareScale},
        {right * kSoftwareScale, top * kSoftwareScale, kSoftwareScale},
        {left * kSoftwareScale, top * kSoftwareScale, kSoftwareScale},
        {left * kSoftwareScale, bottom * kSoftwareScale, kSoftwareScale},
    };
    {
        int vertexIndex1;
        for (vertexIndex1 = 0;
            vertexIndex1 < (int)(sizeof(projectedVerts) / sizeof((projectedVerts)[0]));
            ++vertexIndex1) {
            zVec3 &vertex = (projectedVerts)[vertexIndex1];
            vertex.z = 10.0f;
        }
    }

    zRndr_SubmitTexturedPolyUniformAlphaOrShade(
        projectedVerts,
        clippedTriVerts,
        projectedVerts,
        triUVs,
        4,
        stageTexDirEntry,
        1.0f,
        0
    );
}

/**
 * Reimplements 0x49b020: zRndr_LensFlare_DrawVisibleSampleStages
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_LensFlare.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Draw the four staged lens-flare quads for one visible sample.
 */
void __fastcall zRndr_LensFlare_DrawVisibleSampleStages(
    zRndr_LensFlareVisibleSampleDef *visibleSampleDef,
    float visibilityAlpha
) {
    const float activeWidth = (float)(zRndr::g_activeRegionWidth);
    const float activeHeight = (float)(zRndr::g_activeRegionHeight);
    const float baseRadius = visibilityAlpha * activeWidth * 0.03125f;
    const float largeRadius = baseRadius + baseRadius;
    const float halfClipWidth = activeWidth * 0.5f;
    const float halfClipHeight = activeHeight * 0.5f;
    const float sampleOffsetX = visibleSampleDef->sampleCenterX - halfClipWidth;
    const float sampleOffsetY = visibleSampleDef->sampleCenterY - halfClipHeight;
    const zRndr_LineClipRect2I *clipRect =
        (const zRndr_LineClipRect2I *)(&zRndr::g_activeRegionRect);

    zVec2 sampleCenter = {visibleSampleDef->sampleCenterX, visibleSampleDef->sampleCenterY};
    zRndr_LensFlare_DrawSampleStageClipped(
        &sampleCenter,
        zRndr::g_lensFlareVisibleSampleStages[0],
        largeRadius,
        clipRect
    );

    sampleCenter.x = halfClipWidth + sampleOffsetX * 0.5f;
    sampleCenter.y = halfClipHeight + sampleOffsetY * 0.5f;
    zRndr_LensFlare_DrawSampleStageClipped(
        &sampleCenter,
        zRndr::g_lensFlareVisibleSampleStages[1],
        largeRadius,
        clipRect
    );

    sampleCenter.x = halfClipWidth + sampleOffsetX * 0.100000001f;
    sampleCenter.y = halfClipHeight + sampleOffsetY * 0.100000001f;
    zRndr_LensFlare_DrawSampleStageClipped(
        &sampleCenter,
        zRndr::g_lensFlareVisibleSampleStages[2],
        baseRadius,
        clipRect
    );

    sampleCenter.x = halfClipWidth - sampleOffsetX;
    sampleCenter.y = halfClipHeight - sampleOffsetY;
    zRndr_LensFlare_DrawSampleStageClipped(
        &sampleCenter,
        zRndr::g_lensFlareVisibleSampleStages[3],
        baseRadius * 3.0f,
        clipRect
    );
}

/**
 * Reimplements 0x49afb0: zRndr_LensFlare_DrawVisibleSample
 * Purpose: Draw one visible lens-flare sample after applying near/far fade.
 */
void __fastcall zRndr_LensFlare_DrawVisibleSample(
    int sampleIndex
) {
    zRndr_LensFlareVisibleSampleDef *visibleSampleDef =
        zRndr::g_lensFlareVisibleSampleDefs[sampleIndex];
    zRndr_LensFlareSource *lensFlareSource = visibleSampleDef->lensFlareSource;

    if (zRndr::g_lensFlareVisibilityActive == 0) {
        return;
    }

    const float visibility = 1.0f / visibleSampleDef->depthDivisor;
    if (!(visibility < lensFlareSource->fadeFar)) {
        return;
    }

    if (visibility < lensFlareSource->fadeNear) {
        zRndr_LensFlare_DrawVisibleSampleStages(
            visibleSampleDef,
            1.0f
        );
        return;
    }

    zRndr_LensFlare_DrawVisibleSampleStages(
        visibleSampleDef,
        (lensFlareSource->fadeFar - visibility) /
            (lensFlareSource->fadeFar - lensFlareSource->fadeNear)
    );
}

/**
 * Reimplements 0x49b1a0: zRndr_LensFlare_DrawVisibleSamples
 * Purpose: Draw all visible lens-flare samples and clear the visible-sample list.
 */
void zRndr_LensFlare_DrawVisibleSamples() {
    if (zRndr::g_lensFlareVisibilityActive == 0) {
        return;
    }

    for (int sampleIndex = 0; sampleIndex < zRndr::g_lensFlareVisibleSampleCount; ++sampleIndex) {
        zRndr_LensFlare_DrawVisibleSample(sampleIndex);
    }

    zRndr::g_lensFlareVisibleSampleCount = 0;
}

/**
 * Reimplements 0x49aa30: zRndr_SpanOcclusion_FilterSampleList
 * Purpose: Unproject one visible lens-flare sample into an occlusion-test point.
 */
void __fastcall zRndr_SpanOcclusion_FilterSampleList(
    int visibleSampleIndex,
    zVec3 *outPoint
) {
    zRndr_LensFlareVisibleSampleDef *sample =
        zRndr::g_lensFlareVisibleSampleDefs[visibleSampleIndex];
    zMath_UnprojectPointBatchZBuf(
        (const zProjectedPoint *)(sample),
        outPoint,
        1
    );
}

/**
 * Reimplements 0x49b5a0: zRndr_FogTargetColorStaged_SetRgb01Clamped
 * Purpose: Clamp and stage the pending fog target color, then rebuild its packed 16-bit ramp.
 */
void __fastcall zRndr_FogTargetColorStaged_SetRgb01Clamped(
    zColorRgb *color
) {
    if (color->red > 1.0f) {
        color->red = 1.0f;
    } else if (!(color->red >= 0.0f)) {
        color->red = 0.0f;
    }

    if (color->green > 1.0f) {
        color->green = 1.0f;
    } else if (!(color->green >= 0.0f)) {
        color->green = 0.0f;
    }

    if (color->blue > 1.0f) {
        color->blue = 1.0f;
    } else if (!(color->blue >= 0.0f)) {
        color->blue = 0.0f;
    }

    zRndr::FogParamsPartial *staged = &zRndr::g_fogTargetParamsStaged;
    if (fabs(staged->colorRgb01[0] - color->red) < 0.01f &&
        fabs(staged->colorRgb01[1] - color->green) < 0.01f &&
        fabs(staged->colorRgb01[2] - color->blue) < 0.01f) {
        return;
    }

    staged->colorRgb01[0] = color->red;
    staged->colorRgb01[1] = color->green;
    staged->colorRgb01[2] = color->blue;

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo_SetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat *)(color));
    }

    const int red = (int)(color->red * 255.0f + 0.5f);
    const int green = (int)(color->green * 255.0f + 0.5f);
    const int blue = (int)(color->blue * 255.0f + 0.5f);
    zRndr::FogTarget565_SetPackedColorAndRamp(
        staged,
        (red << g_zVideo_PixelPack.packedBase) & (int)(g_zVideo_PixelPack.rMask),
        (green << g_zVideo_PixelPack.sumMinus8) & (int)(g_zVideo_PixelPack.gMask),
        blue >> g_zVideo_PixelPack.bShiftTo8
    );
}

/**
 * Reimplements 0x499930: zRndr_SetPaletteRemapKey.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Select the active palette remap key from a recipe and shade level.
 */
void __fastcall zRndr_SetPaletteRemapKey(
    zVidPaletteRemapRecipe *recipe,
    float shadeLevel
) {
    if (recipe == 0) {
        g_zRndr_ActivePaletteRemapKey = -1;
        return;
    }

    const int recipeIndex = zVid_PaletteRemap_BuildPaletteVariant(recipe);
    int shadeBucket = (int)(shadeLevel * 0.125f);
    if (shadeBucket > 31) {
        shadeBucket = 31;
    } else if (shadeBucket < 0) {
        shadeBucket = 0;
    }

    g_zRndr_ActivePaletteRemapKey = (recipeIndex << 5) + shadeBucket;
}

/**
 * Reimplements 0x499990: zRndr_SetPaletteRemapKeyFromRgb01.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Build a single-color palette remap recipe from RGB values and select its remap key.
 */
void __fastcall zRndr_SetPaletteRemapKeyFromRgb01(
    zColorRgb *rgb01,
    float shadeLevel
) {
    if (rgb01 == 0) {
        g_zRndr_ActivePaletteRemapKey = -1;
        return;
    }

    zVidPaletteRemapRecipe recipe = {0};
    recipe.color1R = rgb01->red;
    recipe.color1G = rgb01->green;
    recipe.color1B = rgb01->blue;
    recipe.color1Strength = 1.0f;
    zRndr_SetPaletteRemapKey(
        &recipe,
        shadeLevel
    );
}

/**
 * Reimplements 0x499a00: zRndr_SetPaletteShadeRecipeIndex.
 * Original file: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Select the active palette shade recipe variant index.
 */
void __fastcall zRndr_SetPaletteShadeRecipeIndex(
    zVidPaletteRemapRecipe *recipe
) {
    g_zRndr_ActivePaletteShadeRecipeIndex =
        recipe != 0 ? zVid_PaletteRemap_BuildPaletteVariant(recipe) : -1;
}
