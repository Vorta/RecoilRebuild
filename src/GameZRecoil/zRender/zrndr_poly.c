// zRender compilation unit between zrndr_init.c and zrndr_draw.c, inferred from
// the retail object boundary [0x492000, 0x499930): its .rdata pooled
// constants [0x4d2df0, 0x4d2e40) and its .bss run [0x56b1f0, 0x56b248) are
// separate from the neighbouring zRender objects; an unreferenced 16-byte
// .rdata run [0x4d2de0, 0x4d2df0) precedes them and is not modeled. Original
// filename unresolved; zrndr_poly.c is a provisional name (2026-10-02).

#include "GameZRecoil/zRender/zrndr.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zRender/zrndr_api.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <math.h>

/* zImage entry point; zimg_texture.cpp defines it with C linkage for this unit. */
zVidImagePartial* __fastcall GetVariantImageAtIndex(zImage_TexDirEntryPartial* entry, int variantIndex);

/**
 * Recovered helper: MinValue
 * Original-source helper evidence: No standalone plan entry was found; recovered from zRndr span, polygon, and
 * scan-conversion callers in this source file. Purpose: Return the smaller of two values without changing
 * caller-owned storage (callers pass side-effect-free operands).
 */
#define MinValue(lhs, rhs) ((lhs) < (rhs) ? (lhs) : (rhs))

/**
 * Recovered helper: MaxValue
 * Original-source helper evidence: No standalone plan entry was found; recovered from zRndr span, polygon, and
 * scan-conversion callers in this source file. Purpose: Return the larger of two values without changing
 * caller-owned storage (callers pass side-effect-free operands).
 */
#define MaxValue(lhs, rhs) ((lhs) < (rhs) ? (rhs) : (lhs))

/**
 * Recovered helper: sort.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr scan conversion callers in this source file.
 * Purpose: Sort scanline intersection samples in ascending order.
 */
static void sort(float* first, float* last)
{
    float* it;
    for (it = first + 1; it < last; ++it) {
        float value = *it;
        float* scan = it;
        while (scan > first && value < scan[-1]) {
            *scan = scan[-1];
            --scan;
        }
        *scan = value;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b1f0
 * @recoil-artifact defines .data recoil:data:0x56b1f0: gRndr_PerspTexScaledUOverZ0.
 * BN xrefs from zRndr::DrawTexturedQueued stage vertex 0 scaled U/Z for queued
 * perspective texture interpolation and mip metric selection.
 * Purpose: first queued-texture scaled U/Z scratch sample.
 */
float gRndr_PerspTexScaledUOverZ0 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b1f4
 * @recoil-artifact defines .data recoil:data:0x56b1f4: gRndr_PerspTexScaledVOverZ0.
 * BN xrefs from zRndr::DrawTexturedQueued stage vertex 0 scaled V/Z beside the
 * U sample in the authored perspective scratch bank.
 * Purpose: first queued-texture scaled V/Z scratch sample.
 */
float gRndr_PerspTexScaledVOverZ0 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b1f8
 * @recoil-artifact defines .data recoil:data:0x56b1f8: gRndr_PerspTexScaledUOverZ1.
 * BN xrefs from zRndr::DrawTexturedQueued stage vertex 1 scaled U/Z for plane
 * construction and mip metric selection.
 * Purpose: second queued-texture scaled U/Z scratch sample.
 */
float gRndr_PerspTexScaledUOverZ1 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b1fc
 * @recoil-artifact defines .data recoil:data:0x56b1fc: gRndr_PerspTexScaledVOverZ1.
 * BN xrefs from zRndr::DrawTexturedQueued stage vertex 1 scaled V/Z for plane
 * construction and mip metric selection.
 * Purpose: second queued-texture scaled V/Z scratch sample.
 */
float gRndr_PerspTexScaledVOverZ1 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b200
 * @recoil-artifact defines .data recoil:data:0x56b200: gRndr_PerspTexScaledUOverZ2.
 * BN xrefs from zRndr::DrawTexturedQueued stage vertex 2 scaled U/Z for plane
 * construction and mip metric selection.
 * Purpose: third queued-texture scaled U/Z scratch sample.
 */
float gRndr_PerspTexScaledUOverZ2 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b204
 * @recoil-artifact defines .data recoil:data:0x56b204: gRndr_PerspTexScaledVOverZ2.
 * BN xrefs from zRndr::DrawTexturedQueued stage vertex 2 scaled V/Z for plane
 * construction and mip metric selection.
 * Purpose: third queued-texture scaled V/Z scratch sample.
 */
float gRndr_PerspTexScaledVOverZ2 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b208
 * @recoil-artifact defines .data recoil:data:0x56b208: gRndr_PerspTexScaledUOverZBase.
 * BN xrefs from zRndr::DrawTexturedQueued and clipped-triangle interpolation
 * store the scaled U/Z plane base before span chunk dispatch.
 * Purpose: queued-texture scaled U/Z plane base.
 */
float gRndr_PerspTexScaledUOverZBase = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-grndr-perspscratchreserved0
 * @recoil-artifact defines .data recoil:data:0x56b20c: gRndr_PerspScratchReserved0.
 * Purpose: preserves the authored zero dword between queued-texture
 * perspective scratch fields.
 */
int gRndr_PerspScratchReserved0 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b210
 * @recoil-artifact defines .data recoil:data:0x56b210: gRndr_PerspPlaneOriginX.
 * BN xrefs from zRndr::DrawTexturedQueued store the screen-space X origin used
 * to evaluate the queued texture perspective planes.
 * Purpose: queued-texture perspective plane X origin.
 */
float gRndr_PerspPlaneOriginX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b214
 * @recoil-artifact defines .data recoil:data:0x56b214: gRndr_PerspPlaneOriginY.
 * BN xrefs from zRndr::DrawTexturedQueued store the screen-space Y origin used
 * to evaluate the queued texture perspective planes.
 * Purpose: queued-texture perspective plane Y origin.
 */
float gRndr_PerspPlaneOriginY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b218
 * @recoil-artifact defines .data recoil:data:0x56b218: gRndr_PerspTexScaledUOverZStepX.
 * BN xrefs from zRndr::DrawTexturedQueued store the X gradient for the scaled
 * U/Z perspective plane and pass it to mip metric selection.
 * Purpose: queued-texture scaled U/Z plane X step.
 */
float gRndr_PerspTexScaledUOverZStepX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b21c
 * @recoil-artifact defines .data recoil:data:0x56b21c: gRndr_PerspTexScaledUOverZStepY.
 * BN xrefs from zRndr::DrawTexturedQueued store the Y gradient for the scaled
 * U/Z perspective plane.
 * Purpose: queued-texture scaled U/Z plane Y step.
 */
float gRndr_PerspTexScaledUOverZStepY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b220
 * @recoil-artifact defines .data recoil:data:0x56b220: gRndr_PerspInvDepthBase.
 * BN xrefs from zRndr::DrawTexturedQueued store the reciprocal-depth plane base
 * before span-list depth setup and chunked texture dispatch.
 * Purpose: queued-texture inverse-depth plane base.
 */
float gRndr_PerspInvDepthBase = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-grndr-perspscratchreserved1
 * @recoil-artifact defines .data recoil:data:0x56b224: gRndr_PerspScratchReserved1.
 * Purpose: preserves the authored zero dword between queued-texture
 * perspective scratch fields.
 */
int gRndr_PerspScratchReserved1 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b228
 * @recoil-artifact defines .data recoil:data:0x56b228: gRndr_PerspInvDepthStepX.
 * BN xrefs from zRndr::DrawTexturedQueued store the reciprocal-depth X gradient
 * for span depth setup, chunk selection, and mip metric selection.
 * Purpose: queued-texture inverse-depth plane X step.
 */
float gRndr_PerspInvDepthStepX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b22c
 * @recoil-artifact defines .data recoil:data:0x56b22c: gRndr_PerspInvDepthStepY.
 * BN xrefs from zRndr::DrawTexturedQueued store the reciprocal-depth Y gradient
 * for per-scanline span depth setup.
 * Purpose: queued-texture inverse-depth plane Y step.
 */
float gRndr_PerspInvDepthStepY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b230
 * @recoil-artifact defines .data recoil:data:0x56b230: gRndr_PerspTexScaledVOverZStepX.
 * BN xrefs from zRndr::DrawTexturedQueued store the X gradient for the scaled
 * V/Z perspective plane and pass it to mip metric selection.
 * Purpose: queued-texture scaled V/Z plane X step.
 */
float gRndr_PerspTexScaledVOverZStepX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b234
 * @recoil-artifact defines .data recoil:data:0x56b234: gRndr_PerspTexScaledVOverZStepY.
 * BN xrefs from zRndr::DrawTexturedQueued store the Y gradient for the scaled
 * V/Z perspective plane.
 * Purpose: queued-texture scaled V/Z plane Y step.
 */
float gRndr_PerspTexScaledVOverZStepY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-f-0x56b238
 * @recoil-artifact defines .data recoil:data:0x56b238: gRndr_PerspTexScaledVOverZBase.
 * BN xrefs from zRndr::DrawTexturedQueued and clipped-triangle interpolation
 * store the scaled V/Z plane base before span chunk dispatch.
 * Purpose: queued-texture scaled V/Z plane base.
 */
float gRndr_PerspTexScaledVOverZBase = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-zrndr-circlecenterx
 * @recoil-artifact defines .data recoil:data:0x56b23c: g_zRndr_CircleCenterX.
 * BN xrefs: zRndr circle drawing setup stores the center X coordinate before
 * circle span callbacks consume it.
 * Purpose: staged center X coordinate for software circle rendering.
 */
int g_zRndr_CircleCenterX = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-zrndr-circlecentery
 * @recoil-artifact defines .data recoil:data:0x56b240: g_zRndr_CircleCenterY.
 * BN xrefs: zRndr circle drawing setup stores the center Y coordinate before
 * circle span callbacks consume it.
 * Purpose: staged center Y coordinate for software circle rendering.
 */
int g_zRndr_CircleCenterY = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-zrndr-circledrawauxarg
 * @recoil-artifact defines .data recoil:data:0x56b244: g_zRndr_CircleDrawAuxArg.
 * BN xrefs: zRndr circle drawing setup stores the auxiliary draw argument;
 * current BN shows no later read, so the accepted extent is this write-only
 * staging dword.
 * Purpose: staged callback argument for software circle rendering.
 */
int g_zRndr_CircleDrawAuxArg = 0;

typedef struct ScanVertex {
    float x;
    float y;
} ScanVertex;

typedef struct ScanConvertEdge {
    int xStepFixed;
    int yStart;
    int currentXFixed;
    int reserved;
} ScanConvertEdge;

typedef union SpanOcclusionRasterScratch {
    zVec3 reducedVerts[8];
    SpanNodePartial* spanList[0x141];
} SpanOcclusionRasterScratch;

RECOIL_STATIC_ASSERT(sizeof(SpanOcclusionRasterScratch) == 0x504);

/* BN 0x4927d0 uses the original VC5 double-bias fixed-point conversion inline. */
#define ZRNDR_SET_FIXED16_FROM_FLOAT(dst, value)                                                                       \
    do {                                                                                                               \
        double zRndrFixed16Bits = 6755399441055744.0 - (double)((value) * -65536.0f);                                  \
        (dst) = *(int*)(&zRndrFixed16Bits);                                                                            \
    } while (0)

/**
 * Recovered helper: Fixed16FromFloat.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span and scan-conversion callers that round coordinates into 16.16 fixed point.
 * Purpose: Convert a floating-point value to signed 16.16 fixed-point with symmetric rounding.
 */
static int Fixed16FromFloat(float value)
{
    const double scaled = (double)(value) * 65536.0;
    return (int)(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

/**
 * Recovered helper: ScanlineStartFromY.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span and polygon raster callers that bias the starting scanline from fixed-point Y.
 * Purpose: Compute the first covered scanline for a polygon edge Y coordinate.
 */
static int ScanlineStartFromY(float y)
{
    return (Fixed16FromFloat(y) + 0x7fff) >> 16;
}

/**
 * Recovered helper: ScanlineEndFromY.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span and polygon raster callers that bias the ending scanline from fixed-point Y.
 * Purpose: Compute the last covered scanline for a polygon edge Y coordinate.
 */
static int ScanlineEndFromY(float y)
{
    return (Fixed16FromFloat(y) - 0x8041) >> 16;
}

/**
 * Recovered helper: SpanStartFromX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span builders that bias the starting pixel from fixed-point X.
 * Purpose: Compute the first covered span sample for an edge X coordinate.
 */
static int SpanStartFromX(float x)
{
    return (Fixed16FromFloat(x) + 0x7fff) >> 16;
}

/**
 * Recovered helper: SpanEndFromX.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span builders that bias the ending pixel from fixed-point X.
 * Purpose: Compute the last covered span sample for an edge X coordinate.
 */
static int SpanEndFromX(float x)
{
    return (Fixed16FromFloat(x) - 0x8001) >> 16;
}

/**
 * Recovered helper: AppendSpanListNode.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion builders that coalesce adjacent visible span nodes.
 * Purpose: Append a visible span node and merge it with the previous node when contiguous.
 */
static void AppendSpanListNode(SpanNodePartial** spanList, int* spanCount, SpanNodePartial* node)
{
    if (*spanCount > 0) {
        SpanNodePartial* previous = spanList[*spanCount - 1];
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
static float SpanDepthAtX(const SpanNodePartial* span, int x)
{
    if (x == span->sampleXMin) {
        return span->invDepth;
    }
    if (x == span->sampleXMax) {
        return span->invDepthStep;
    }

    return span->invDepth + (float)(x - span->sampleXMin) * span->depthSlope;
}

/**
 * Recovered helper: SpanDepthAtXByParts.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span split paths using explicit span depth parts.
 * Purpose: Evaluate inverse depth from a span start, base depth, and depth slope.
 */
static float SpanDepthAtXByParts(int sampleXMin, float invDepth, float depthSlope, int x)
{
    return invDepth + (float)(x - sampleXMin) * depthSlope;
}

/**
 * Recovered helper: LinkSpanNode.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion insertion paths that update the column head/link fields.
 * Purpose: Link a pending span node into one column and refresh the span iterator globals.
 */
static void LinkSpanNode(int columnIndex, SpanNodePartial* previous, SpanNodePartial* node, SpanNodePartial* next)
{
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
static void InsertPendingSpanSorted(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    SpanNodePartial* pending;
    SpanNodePartial* previous;
    SpanNodePartial* current;
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
        return;
    }

    pending = g_spanAllocCursor;
    pending->next = 0;

    previous = 0;
    current = g_spanColumnHeadTable[columnIndex];
    while (current != 0 && current->sampleXMax + 1 < pending->sampleXMin) {
        previous = current;
        current = current->next;
    }

    while (current != 0 && current->sampleXMin <= pending->sampleXMax + 1) {
        pending->sampleXMin = MinValue(pending->sampleXMin, current->sampleXMin);
        pending->sampleXMax = MaxValue(pending->sampleXMax, current->sampleXMax);
        pending->invDepth = MaxValue(pending->invDepth, current->invDepth);
        pending->invDepthStep = MaxValue(pending->invDepthStep, current->invDepthStep);
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
    AppendSpanListNode(spanList, spanCount, pending);
    ++g_spanAllocCursor;
}

/**
 * Recovered helper: InsertPendingSpanNoDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion callers that clip existing column spans without a depth comparison.
 * Purpose: Insert the pending span into one column while removing or splitting overlapped spans.
 */
static void InsertPendingSpanNoDepthTest(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    SpanNodePartial* pending;
    SpanNodePartial* previous;
    SpanNodePartial* current;
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
        return;
    }

    pending = g_spanAllocCursor;
    previous = 0;
    current = g_spanColumnHeadTable[columnIndex];

    while (current != 0 && pending->sampleXMin > current->sampleXMax) {
        previous = current;
        current = current->next;
    }

    if (current == 0 || pending->sampleXMax < current->sampleXMin) {
        LinkSpanNode(columnIndex, previous, pending, current);
        AppendSpanListNode(spanList, spanCount, pending);
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
                current->invDepthStep
                    = SpanDepthAtXByParts(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMax);

                if (currentMax > pending->sampleXMax) {
                    SpanNodePartial* rightSplit = pending + 1;
                    rightSplit->sampleXMin = pending->sampleXMax + 1;
                    rightSplit->sampleXMax = currentMax;
                    rightSplit->invDepth
                        = SpanDepthAtXByParts(currentMin, currentInvDepth, currentDepthSlope, rightSplit->sampleXMin);
                    rightSplit->invDepthStep = currentInvDepthStep;
                    rightSplit->depthSlope = currentDepthSlope;
                    rightSplit->next = current->next;

                    pending->next = rightSplit;
                    current->next = pending;
                    AppendSpanListNode(spanList, spanCount, pending);
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
            SpanNodePartial* next = current->next;
            if (previous != 0) {
                previous->next = next;
            } else {
                g_spanColumnHeadTable[columnIndex] = next;
            }
            current = next;
            continue;
        }

        current->sampleXMin = pending->sampleXMax + 1;
        current->invDepth = SpanDepthAtXByParts(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMin);
        break;
    }

    LinkSpanNode(columnIndex, previous, pending, current);
    AppendSpanListNode(spanList, spanCount, pending);
    ++g_spanAllocCursor;
}

/**
 * Recovered helper: BuildVisibleSpanListWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion visibility builders that compare pending and stored span depth.
 * Purpose: Build visible span fragments for a pending span without mutating the occluder list.
 */
static void BuildVisibleSpanListWithDepthTest(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    SpanNodePartial* pending;
    SpanNodePartial* current;
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
        return;
    }

    pending = g_spanAllocCursor;
    pending->next = 0;
    current = g_spanColumnHeadTable[columnIndex];

    while (current != 0 && pending->sampleXMin > current->sampleXMax) {
        current = current->next;
    }

    while (current != 0) {
        SpanNodePartial occluder;
        int pendingInFront;
        int pendingMin;
        int pendingMax;
        float pendingInvDepth;
        float pendingInvDepthStep;
        float pendingDepthSlope;
        if (pending->sampleXMax < current->sampleXMin) {
            break;
        }

        occluder = *current;
        pendingInFront = zRndrSpanOcclusionTestSpanDepthOrderPair(pending, &occluder) != 0;

        pendingMin = pending->sampleXMin;
        pendingMax = pending->sampleXMax;
        pendingInvDepth = pending->invDepth;
        pendingInvDepthStep = pending->invDepthStep;
        pendingDepthSlope = pending->depthSlope;

        if (pendingInFront) {
            if (occluder.sampleXMax < pendingMax && occluder.sampleXMax >= pendingMin) {
                const int splitMax = occluder.sampleXMax;
                pending->sampleXMax = splitMax;
                pending->invDepthStep = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, splitMax);
                AppendSpanListNode(spanList, spanCount, pending);
                ++g_spanAllocCursor;

                pending = g_spanAllocCursor;
                pending->next = 0;
                pending->sampleXMin = splitMax + 1;
                pending->sampleXMax = pendingMax;
                pending->invDepth
                    = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
                pending->invDepthStep = pendingInvDepthStep;
                pending->depthSlope = pendingDepthSlope;
                current = current->next;
                continue;
            }

            if (occluder.sampleXMax >= pendingMax) {
                AppendSpanListNode(spanList, spanCount, pending);
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
                pending->invDepth
                    = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
            }

            current = current->next;
            continue;
        }

        if (occluder.sampleXMin <= pendingMax) {
            const int leftMax = occluder.sampleXMin - 1;
            pending->sampleXMax = leftMax;
            pending->invDepthStep = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, leftMax);
            AppendSpanListNode(spanList, spanCount, pending);
            ++g_spanAllocCursor;

            if (occluder.sampleXMax >= pendingMax) {
                return;
            }

            pending = g_spanAllocCursor;
            pending->next = 0;
            pending->sampleXMin = occluder.sampleXMax + 1;
            pending->sampleXMax = pendingMax;
            pending->invDepth
                = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
            pending->invDepthStep = pendingInvDepthStep;
            pending->depthSlope = pendingDepthSlope;
        }

        current = current->next;
    }

    AppendSpanListNode(spanList, spanCount, pending);
    ++g_spanAllocCursor;
}

/**
 * Recovered helper: InsertPendingSpanWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr span-occlusion insertion callers that compare pending and stored span depth.
 * Purpose: Insert the pending span into one column while splitting spans by depth order.
 */
static void InsertPendingSpanWithDepthTest(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    SpanNodePartial* pending;
    SpanNodePartial* previous;
    SpanNodePartial* current;
    *spanCount = 0;
    if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
        return;
    }

    pending = g_spanAllocCursor;
    pending->next = 0;
    previous = 0;
    current = g_spanColumnHeadTable[columnIndex];

    while (current != 0 && pending->sampleXMin > current->sampleXMax) {
        previous = current;
        current = current->next;
    }

    while (current != 0) {
        int pendingInFront;
        int pendingMin;
        int pendingMax;
        float pendingInvDepth;
        float pendingInvDepthStep;
        float pendingDepthSlope;
        if (pending->sampleXMax < current->sampleXMin) {
            break;
        }

        pendingInFront = zRndrSpanOcclusionTestSpanDepthOrderPair(pending, current) != 0;

        pendingMin = pending->sampleXMin;
        pendingMax = pending->sampleXMax;
        pendingInvDepth = pending->invDepth;
        pendingInvDepthStep = pending->invDepthStep;
        pendingDepthSlope = pending->depthSlope;

        if (pendingInFront) {
            const int currentMin = current->sampleXMin;
            const int currentMax = current->sampleXMax;
            const float currentInvDepth = current->invDepth;
            const float currentInvDepthStep = current->invDepthStep;
            const float currentDepthSlope = current->depthSlope;

            if (currentMin < pendingMin) {
                current->sampleXMax = pendingMin - 1;
                current->invDepthStep
                    = SpanDepthAtXByParts(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMax);

                if (currentMax > pendingMax) {
                    SpanNodePartial* rightSplit = pending + 1;
                    rightSplit->sampleXMin = pendingMax + 1;
                    rightSplit->sampleXMax = currentMax;
                    rightSplit->invDepth
                        = SpanDepthAtXByParts(currentMin, currentInvDepth, currentDepthSlope, rightSplit->sampleXMin);
                    rightSplit->invDepthStep = currentInvDepthStep;
                    rightSplit->depthSlope = currentDepthSlope;
                    rightSplit->next = current->next;

                    pending->next = rightSplit;
                    current->next = pending;
                    AppendSpanListNode(spanList, spanCount, pending);
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
                SpanNodePartial* next = current->next;
                if (previous != 0) {
                    previous->next = next;
                } else {
                    g_spanColumnHeadTable[columnIndex] = next;
                }
                current = next;
                continue;
            }

            current->sampleXMin = pendingMax + 1;
            current->invDepth
                = SpanDepthAtXByParts(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMin);
            break;
        }

        if (current->sampleXMin <= pendingMin) {
            if (current->sampleXMax >= pendingMax) {
                return;
            }

            if (current->sampleXMax >= pendingMin) {
                pending->sampleXMin = current->sampleXMax + 1;
                pending->invDepth
                    = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
            }

            previous = current;
            current = current->next;
            continue;
        }

        if (current->sampleXMin <= pendingMax) {
            const int leftMax = current->sampleXMin - 1;
            pending->sampleXMax = leftMax;
            pending->invDepthStep = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, leftMax);
            LinkSpanNode(columnIndex, previous, pending, current);
            AppendSpanListNode(spanList, spanCount, pending);
            ++g_spanAllocCursor;

            if (current->sampleXMax >= pendingMax) {
                return;
            }

            previous = current;
            pending = g_spanAllocCursor;
            pending->next = 0;
            pending->sampleXMin = current->sampleXMax + 1;
            pending->sampleXMax = pendingMax;
            pending->invDepth
                = SpanDepthAtXByParts(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
            pending->invDepthStep = pendingInvDepthStep;
            pending->depthSlope = pendingDepthSlope;
            current = current->next;
            continue;
        }

        previous = current;
        current = current->next;
    }

    LinkSpanNode(columnIndex, previous, pending, current);
    AppendSpanListNode(spanList, spanCount, pending);
    ++g_spanAllocCursor;
}

/**
 * Recovered inline helper: zRndr 565 lens-flare color blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x498cb0 as
 * the 565 branch of the lens-flare pixel blend path. Purpose: Blend one packed 565 color toward another using
 * an 8-bit alpha value.
 */
static __inline unsigned short BlendPacked565(unsigned short from, unsigned short to, int alpha)
{
    const int red = ((from >> 11) & 0x1f) + ((((to >> 11) & 0x1f) - ((from >> 11) & 0x1f)) * alpha >> 8);
    const int green = ((from >> 5) & 0x3f) + ((((to >> 5) & 0x3f) - ((from >> 5) & 0x3f)) * alpha >> 8);
    const int blue = (from & 0x1f) + (((to & 0x1f) - (from & 0x1f)) * alpha >> 8);
    return (unsigned short)(((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1f));
}

/**
 * Recovered inline helper: zRndr 555 lens-flare color blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x498cb0 as
 * the 555 branch of the lens-flare pixel blend path. Purpose: Blend one packed 555 color toward another using
 * an 8-bit alpha value.
 */
static __inline unsigned short BlendPacked555(unsigned short from, unsigned short to, int alpha)
{
    const int red = ((from >> 10) & 0x1f) + ((((to >> 10) & 0x1f) - ((from >> 10) & 0x1f)) * alpha >> 8);
    const int green = ((from >> 5) & 0x1f) + ((((to >> 5) & 0x1f) - ((from >> 5) & 0x1f)) * alpha >> 8);
    const int blue = (from & 0x1f) + (((to & 0x1f) - (from & 0x1f)) * alpha >> 8);
    return (unsigned short)(((red & 0x1f) << 10) | ((green & 0x1f) << 5) | (blue & 0x1f));
}

/**
 * Recovered inline helper: zRndr lens-flare pixel blend
 * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x498cb0
 * overlay/depth-fade paths and selected by the active 555/565 pixel-pack state. Purpose: Blend a lens-flare
 * pixel using the active 16-bit framebuffer packing.
 */
static __inline unsigned short BlendLensFlarePixel(unsigned short from, unsigned short to, int alpha)
{
    if (g_pixelPackGreenBits == 6) {
        if (alpha <= 3) {
            return from;
        }
        if (alpha >= 0xfc) {
            return to;
        }
        return BlendPacked565(from, to, alpha);
    }

    if (alpha <= 7) {
        return from;
    }
    if (alpha >= 0xfc) {
        return to;
    }
    return BlendPacked555(from, to, alpha);
}

/**
 * Recovered helper: SpanOcclusionInsertPendingSpanSorted.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects the sorted insertion helper.
 * Purpose: Route span-occlusion dispatch to the sorted pending-span insertion helper.
 */
void SpanOcclusionInsertPendingSpanSorted(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    InsertPendingSpanSorted(spanList, columnIndex, spanCount);
}

/**
 * Recovered helper: SpanOcclusionInsertPendingSpanWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects depth-tested insertion.
 * Purpose: Route span-occlusion dispatch to the depth-tested pending-span insertion helper.
 */
void SpanOcclusionInsertPendingSpanWithDepthTest(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    InsertPendingSpanWithDepthTest(spanList, columnIndex, spanCount);
}

/**
 * Recovered helper: SpanOcclusionInsertPendingSpanNoDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects non-depth insertion.
 * Purpose: Route span-occlusion dispatch to the non-depth pending-span insertion helper.
 */
void SpanOcclusionInsertPendingSpanNoDepthTest(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    InsertPendingSpanNoDepthTest(spanList, columnIndex, spanCount);
}

/**
 * Recovered helper: SpanOcclusionBuildVisibleSpanListWithDepthTest.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * address-backed zRndr span-occlusion dispatch setup that selects visibility-list building.
 * Purpose: Route span-occlusion dispatch to the depth-tested visible-span list builder.
 */
void SpanOcclusionBuildVisibleSpanListWithDepthTest(SpanNodePartial** spanList, int columnIndex, int* spanCount)
{
    BuildVisibleSpanListWithDepthTest(spanList, columnIndex, spanCount);
}

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
static float zRndrSpanDepthAtXByPartsLocal(int sampleXMin, float invDepth, float depthSlope, int x)
{
    return invDepth + (float)(x - sampleXMin) * depthSlope;
}

typedef struct Plane2f {
    zVec2 gradient;
    float base;
} Plane2f;

typedef struct TexturedPlanes {
    Plane2f reciprocalZ;
    Plane2f uOverZ;
    Plane2f vOverZ;
    float originX;
    float originY;
} TexturedPlanes;

/**
 * Recovered helper: RoundToFixed20.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr perspective span callers that round texture and shade deltas to fixed-point steps.
 * Purpose: Round a floating-point value to the nearest integer for fixed-point span state.
 */
static int RoundToFixed20(float value)
{
    return (int)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

/**
 * Recovered helper: WrapPolygonIndex.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr scan-edge builders that walk polygon vertices in either direction.
 * Purpose: Wrap a polygon vertex index by one step at either end of the vertex array.
 */
static int WrapPolygonIndex(int index, int vertexCount)
{
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
static int BuildScanConvertEdges(
    const zVec3* vertices,
    int vertexCount,
    int startIndex,
    int stopIndex,
    int step,
    ScanConvertEdge* edges
)
{
    int edgeCount = 0;
    int vertexIndex = startIndex;
    int yStart = ScanlineStartFromY(vertices[vertexIndex].y);
    float sampleY = (float)(yStart) + 0.5f;

    while (vertexIndex != stopIndex && edgeCount < 0x40) {
        const int nextIndex = WrapPolygonIndex(vertexIndex + step, vertexCount);
        const zVec3* start = &vertices[vertexIndex];
        const zVec3* end = &vertices[nextIndex];

        if (sampleY <= end->y) {
            const float dy = end->y - start->y;
            edges[edgeCount].yStart = yStart;
            edges[edgeCount].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end->x - start->x) / dy;
                edges[edgeCount].xStepFixed = Fixed16FromFloat(xSlope);
                edges[edgeCount].currentXFixed = Fixed16FromFloat(start->x + (sampleY - start->y) * xSlope);
            } else {
                edges[edgeCount].xStepFixed = 0;
                edges[edgeCount].currentXFixed = Fixed16FromFloat(start->x);
            }

            ++edgeCount;
            yStart = ScanlineStartFromY(end->y);
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
static Plane2f BuildPlaneFromTriangle(const zVec3* triVerts, const float values[3])
{
    const float dx10 = triVerts[0].x - triVerts[1].x;
    const float dx12 = triVerts[2].x - triVerts[1].x;
    const float dy10 = triVerts[0].y - triVerts[1].y;
    const float dy12 = triVerts[2].y - triVerts[1].y;
    const float determinant = dy12 * dx10 - dy10 * dx12;

    Plane2f plane = { 0 };
    if (determinant != 0.0f) {
        const float dv10 = values[0] - values[1];
        const float dv12 = values[2] - values[1];
        const float inverseDeterminant = -1.0f / determinant;
        // Retail plane setup (0x495850) negates the cross terms against -1/det.
        plane.gradient.x = (dv12 * dy10 - dv10 * dy12) * inverseDeterminant;
        plane.gradient.y = (dv10 * dx12 - dv12 * dx10) * inverseDeterminant;
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
static Plane2f BuildScreenPlaneFromTriangle(const zVec3* triVerts, const float values[3])
{
    Plane2f plane = BuildPlaneFromTriangle(triVerts, values);
    plane.base = values[0] - triVerts[0].x * plane.gradient.x - triVerts[0].y * plane.gradient.y;
    return plane;
}

/**
 * Recovered helper: BuildQueuedTexturePlanes.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued texture draw callers that share perspective-correct texture interpolation.
 * Purpose: Build reciprocal-Z and texture-over-Z planes for queued textured polygon spans.
 */
static TexturedPlanes BuildQueuedTexturePlanes(
    const zVec3* clippedTriVerts,
    const zVec3* triVerts,
    const zVec2* triUVs,
    float imageWidth,
    float imageHeight
)
{
    int useClippedNearPlane;
    TexturedPlanes planes = { 0 };
    float adjustX;
    float adjustY;
    gRndr_PerspTexScaledUOverZ0 = imageWidth * triVerts[0].z * triUVs[0].x;
    gRndr_PerspTexScaledVOverZ0 = imageHeight * triVerts[0].z * triUVs[0].y;
    gRndr_PerspTexScaledUOverZ1 = imageWidth * triVerts[1].z * triUVs[1].x;
    gRndr_PerspTexScaledVOverZ1 = imageHeight * triVerts[1].z * triUVs[1].y;
    gRndr_PerspTexScaledUOverZ2 = imageWidth * triVerts[2].z * triUVs[2].x;
    gRndr_PerspTexScaledVOverZ2 = imageHeight * triVerts[2].z * triUVs[2].y;

    useClippedNearPlane = clippedTriVerts != 0
        && (clippedTriVerts[0].z < 10.0f || clippedTriVerts[1].z < 10.0f || clippedTriVerts[2].z < 10.0f);

    if (useClippedNearPlane) {
        zMathBuildPerspectiveTextureInterpolants(
            clippedTriVerts,
            triUVs,
            (zVec2*)(&gRndr_PerspInvDepthStepX),
            &gRndr_PerspInvDepthBase,
            (zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
            &gRndr_PerspTexScaledUOverZBase,
            (zVec2*)(&gRndr_PerspTexScaledVOverZStepX),
            &gRndr_PerspTexScaledVOverZBase
        );
        gRndr_PerspTexScaledUOverZStepX *= imageWidth;
        gRndr_PerspTexScaledUOverZStepY *= imageWidth;
        gRndr_PerspTexScaledUOverZBase *= imageWidth;
        gRndr_PerspTexScaledVOverZStepX *= imageHeight;
        gRndr_PerspTexScaledVOverZStepY *= imageHeight;
        gRndr_PerspTexScaledVOverZBase *= imageHeight;
        gRndr_PerspPlaneOriginX = g_zMath_ProjOffsetX;
        gRndr_PerspPlaneOriginY = g_zMath_ProjOffsetY;
    } else {
        const float reciprocalValues[3] = { triVerts[0].z, triVerts[1].z, triVerts[2].z };
        const float uValues[3]
            = { gRndr_PerspTexScaledUOverZ0, gRndr_PerspTexScaledUOverZ1, gRndr_PerspTexScaledUOverZ2 };
        const float vValues[3]
            = { gRndr_PerspTexScaledVOverZ0, gRndr_PerspTexScaledVOverZ1, gRndr_PerspTexScaledVOverZ2 };

        const Plane2f reciprocalZ = BuildPlaneFromTriangle(triVerts, reciprocalValues);
        const Plane2f uOverZ = BuildPlaneFromTriangle(triVerts, uValues);
        const Plane2f vOverZ = BuildPlaneFromTriangle(triVerts, vValues);
        gRndr_PerspInvDepthStepX = reciprocalZ.gradient.x;
        gRndr_PerspInvDepthStepY = reciprocalZ.gradient.y;
        gRndr_PerspInvDepthBase = reciprocalZ.base;
        gRndr_PerspTexScaledUOverZStepX = uOverZ.gradient.x;
        gRndr_PerspTexScaledUOverZStepY = uOverZ.gradient.y;
        gRndr_PerspTexScaledUOverZBase = uOverZ.base;
        gRndr_PerspTexScaledVOverZStepX = vOverZ.gradient.x;
        gRndr_PerspTexScaledVOverZStepY = vOverZ.gradient.y;
        gRndr_PerspTexScaledVOverZBase = vOverZ.base;
        gRndr_PerspPlaneOriginX = triVerts[0].x;
        gRndr_PerspPlaneOriginY = triVerts[0].y;
    }

    planes.reciprocalZ.gradient.x = gRndr_PerspInvDepthStepX;
    planes.reciprocalZ.gradient.y = gRndr_PerspInvDepthStepY;
    planes.reciprocalZ.base = gRndr_PerspInvDepthBase;
    planes.uOverZ.gradient.x = gRndr_PerspTexScaledUOverZStepX;
    planes.uOverZ.gradient.y = gRndr_PerspTexScaledUOverZStepY;
    planes.uOverZ.base = gRndr_PerspTexScaledUOverZBase;
    planes.vOverZ.gradient.x = gRndr_PerspTexScaledVOverZStepX;
    planes.vOverZ.gradient.y = gRndr_PerspTexScaledVOverZStepY;
    planes.vOverZ.base = gRndr_PerspTexScaledVOverZBase;
    planes.originX = gRndr_PerspPlaneOriginX;
    planes.originY = gRndr_PerspPlaneOriginY;

    adjustX = planes.originX - 0.5f;
    adjustY = planes.originY - 0.5f;
    planes.reciprocalZ.base -= adjustX * planes.reciprocalZ.gradient.x + adjustY * planes.reciprocalZ.gradient.y;
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
static float EvalPlane(const Plane2f* plane, float x, float y)
{
    return x * plane->gradient.x + y * plane->gradient.y + plane->base;
}

/**
 * Recovered helper: EvalPerspectiveScratchPlane.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued texture span callers that consume the gRndr_Persp* scratch bank.
 * Purpose: Evaluate one queued texture scratch plane at the same sample point as the adjusted local planes.
 */
static float EvalPerspectiveScratchPlane(float stepX, float stepY, float base, float x, float y)
{
    return (x + 0.5f - gRndr_PerspPlaneOriginX) * stepX + (y + 0.5f - gRndr_PerspPlaneOriginY) * stepY + base;
}

/**
 * Recovered helper: SelectPerspectiveChunkPixels.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr perspective texture callers that choose adaptive per-span chunk lengths.
 * Purpose: Select the pixel count used for one perspective-correct texture span chunk.
 */
static int SelectPerspectiveChunkPixels(float minPositiveReciprocalZ, float reciprocalZStepX)
{
    int chunkPixels;
    if (g_perspectiveAdaptiveMinSpan == 0) {
        return MaxValue(1, g_perspectiveTextureDeltaXPow2);
    }

    chunkPixels = g_perspectiveAdaptiveMaxSpan;
    if (reciprocalZStepX != 0.0f) {
        chunkPixels = (int)(fabs(minPositiveReciprocalZ * g_perspectiveAdaptiveSlope / reciprocalZStepX));
    }

    chunkPixels = MinValue(chunkPixels, g_perspectiveAdaptiveMaxSpan);
    chunkPixels = MaxValue(chunkPixels, g_perspectiveAdaptiveMinSpan);
    return MaxValue(1, chunkPixels);
}

/**
 * Recovered helper: DispatchTexturedSpanChunks.
 * Original-source helper evidence: No standalone plan entry was found; recovered from
 * zRndr queued texture draw callers that dispatch spans through selected texture callbacks.
 * Purpose: Split one visible span into texture chunks and dispatch each chunk to the span routine.
 */
static void DispatchTexturedSpanChunks(
    TexturedQueuedSpanProc spanProc,
    const Plane2f* shadePlane,
    SpanNodePartial* span,
    int y,
    int chunkPixels,
    float textureScale,
    int texVShift
)
{
    int remaining = span->sampleXMax - span->sampleXMin + 1;
    int x = span->sampleXMin;
    while (remaining > 0) {
        const int count = MinValue(remaining, chunkPixels);
        const float startX = (float)(x);
        const float endX = (float)(x + count);
        const float sampleY = (float)(y);
        const float startInvZ = EvalPerspectiveScratchPlane(
            gRndr_PerspInvDepthStepX,
            gRndr_PerspInvDepthStepY,
            gRndr_PerspInvDepthBase,
            startX,
            sampleY
        );
        const float endInvZ = EvalPerspectiveScratchPlane(
            gRndr_PerspInvDepthStepX,
            gRndr_PerspInvDepthStepY,
            gRndr_PerspInvDepthBase,
            endX,
            sampleY
        );
        float startU;
        float startV;
        float endU;
        float endV;
        if (startInvZ == 0.0f || endInvZ == 0.0f) {
            x += count;
            remaining -= count;
            continue;
        }

        startU = EvalPerspectiveScratchPlane(
                     gRndr_PerspTexScaledUOverZStepX,
                     gRndr_PerspTexScaledUOverZStepY,
                     gRndr_PerspTexScaledUOverZBase,
                     startX,
                     sampleY
                 )
            / startInvZ;
        startV = EvalPerspectiveScratchPlane(
                     gRndr_PerspTexScaledVOverZStepX,
                     gRndr_PerspTexScaledVOverZStepY,
                     gRndr_PerspTexScaledVOverZBase,
                     startX,
                     sampleY
                 )
            / startInvZ;
        endU = EvalPerspectiveScratchPlane(
                   gRndr_PerspTexScaledUOverZStepX,
                   gRndr_PerspTexScaledUOverZStepY,
                   gRndr_PerspTexScaledUOverZBase,
                   endX,
                   sampleY
               )
            / endInvZ;
        endV = EvalPerspectiveScratchPlane(
                   gRndr_PerspTexScaledVOverZStepX,
                   gRndr_PerspTexScaledVOverZStepY,
                   gRndr_PerspTexScaledVOverZBase,
                   endX,
                   sampleY
               )
            / endInvZ;

        g_spanActiveTexUStepFixed20 = RoundToFixed20((endU - startU) * textureScale / (float)(count));
        g_spanActiveTexVStepFixed20 = RoundToFixed20((endV - startV) * textureScale / (float)(count));
        if (shadePlane != 0) {
            const float startShade = MaxValue(0.0f, MinValue(255.0f, EvalPlane(shadePlane, startX, sampleY)));
            const float endShade = MaxValue(0.0f, MinValue(255.0f, EvalPlane(shadePlane, endX, sampleY)));
            g_spanActiveShadeFixed16 = RoundToFixed20(startShade * 65536.0f);
            g_spanActiveShadeStepFixed16 = RoundToFixed20((endShade - startShade) * 65536.0f / (float)(count));
        }

        spanProc(RoundToFixed20(startU * textureScale), RoundToFixed20(startV * textureScale), count, texVShift);

        g_spanCurrentSpanBaseAddr += count;
        x += count;
        remaining -= count;
    }
}

// BN types the zeroed span/MMX scratch vectors from gRndr_Mmx_dUDup2 through
// gRndr_MmxMask_BlueBits as zMmxQword records. Source keeps lo/hi pairs as
// zMmxQword and lane-indexed mask/factor vectors as four 16-bit lanes, which
// preserves the same eight-byte authored zRndr span data shape.
zMmxQword g_mmxUStepDup2 = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-spansavedespslot
 * @recoil-artifact defines .data recoil:data:0x57da38: gRndr_SavedEspSlot.
 * Purpose: Hold the saved real ESP pointer while the switch-vshift span loops pivot ESP to the destination span.
 */
zRndr_SpanEspPivotSave* g_spanSavedEspSlot = 0;
zMmxQword g_mmxUMask = { 0 };
int g_spanActiveConstAlphaBits = 0;
zMmxQword g_mmxVMask = { 0 };
zMmxQword g_mmxVStepDup2 = { 0 };
zMmxQword g_mmxUPair = { 0 };
zMmxQword g_mmxVShiftCounts = { 0 };
zMmxQword g_mmxVPair = { 0 };
unsigned short g_mmxBitsBlue255[4] = { 0 };
unsigned short g_mmxBitsGreen255[4] = { 0 };
unsigned short g_mmxBitsRed255[4] = { 0 };
short g_mmxMaskGreenPacked[4] = { 0 };
unsigned short g_mmxMaskRedPacked[4] = { 0 };
unsigned short g_mmxFogFactors[4] = { 0 };
unsigned short g_mmxMaskGreenBits[4] = { 0 };
unsigned short g_mmxMaskBlueBits[4] = { 0 };
/*
 * BN exposes adjacent zero BSS dwords data_57dab8/data_57dabc immediately
 * after the blue mask qword. Same-session BN xrefs show no code or data users,
 * so source leaves them as an unmodeled BSS gap instead of authored span/MMX
 * state.
 */

/**
 * Purpose: Convert a float to signed 16.16 fixed point with the double-bias conversion of the
 * span-list rasterizers 0x492000 and 0x492f00.
 * Inputs: value, a float expression. Output: dst, an int lvalue, receives the low dword of
 * 6755399441055744.0 + value * 65536.0, that is value in 16.16 rounded to an integer.
 * Captured state: fixed16Bits, the enclosing rasterizer's double, holds the biased representation
 * for every conversion. The one shared temporary is a source model of retail's observed reuse of a
 * single qword conversion slot in each rasterizer, not recovered lexical scope; its home remains
 * compiler-assigned.
 */
#define ZRNDR_FIXED16_FROM_FLOAT(dst, value)                                                                           \
    do {                                                                                                               \
        fixed16Bits = 6755399441055744.0 - (double)((value) * -65536.0f);                                              \
        (dst) = *(int*)(&fixed16Bits);                                                                                 \
    } while (0)

/**
 * Purpose: Process one outline edge of a span-list rasterizer walk: record it in an edge table when
 * it reaches the current sample row, then advance the walk to the edge's end vertex.
 * Inputs: edgeTable, a ScanConvertEdge array, and edgeCount, its int fill count. Captured walk
 * state: vertices; edgeVertexIndex and nextIndex, the edge's start and end vertex; edgeYStart and
 * edgeSampleY, the current sample row and its pixel-centre y; the fixed16Value and fixed16Bits
 * conversion temporaries.
 * Outputs: when edgeSampleY <= vertices[nextIndex].y, appends edgeTable[edgeCount++] with yStart =
 * edgeYStart and, for a non-horizontal edge only, the 16.16 xStepFixed and currentXFixed at that
 * row (a horizontal edge leaves both unwritten), then moves edgeYStart and edgeSampleY to the first
 * row whose pixel-centre y is not less than vertices[nextIndex].y. Always sets edgeVertexIndex to
 * nextIndex.
 */
#define ZRNDR_SCAN_EDGE_ADD(edgeTable, edgeCount)                                                                      \
    if (edgeSampleY <= vertices[nextIndex].y) {                                                                        \
        const float dy = vertices[nextIndex].y - vertices[edgeVertexIndex].y;                                          \
        ScanConvertEdge* const edge = &edgeTable[edgeCount++];                                                         \
        edge->yStart = edgeYStart;                                                                                     \
        if (dy != 0.0f) {                                                                                              \
            const float xSlope = (vertices[nextIndex].x - vertices[edgeVertexIndex].x) / dy;                           \
            ZRNDR_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);                                                        \
            ZRNDR_FIXED16_FROM_FLOAT(                                                                                  \
                edge->currentXFixed,                                                                                   \
                vertices[edgeVertexIndex].x + ((float)(edgeYStart) - -0.5f - vertices[edgeVertexIndex].y) * xSlope     \
            );                                                                                                         \
        }                                                                                                              \
        ZRNDR_FIXED16_FROM_FLOAT(fixed16Value, vertices[nextIndex].y);                                                 \
        edgeYStart = (fixed16Value + 0x7fff) >> 16;                                                                    \
        edgeSampleY = (float)(edgeYStart) - -0.5f;                                                                     \
    }                                                                                                                  \
    edgeVertexIndex = nextIndex

/**
 * Purpose: Walk the polygon outline from topVertexIndex to bottomVertexIndex in increasing vertex
 * order, wrapping only past vertCount - 1, and record each edge that reaches a sample row.
 * Inputs: edgeTable and edgeCount as for ZRNDR_SCAN_EDGE_ADD. Captured walk state: vertices,
 * vertCount, topVertexIndex and bottomVertexIndex (read); edgeVertexIndex, nextIndex, edgeYStart,
 * edgeSampleY, fixed16Value and fixed16Bits (walk scratch, left at their final values).
 * Outputs: starts the sample row at the top vertex, then runs a post-tested loop that expands
 * ZRNDR_SCAN_EDGE_ADD per edge until edgeVertexIndex reaches bottomVertexIndex; edgeCount ends as
 * the number of recorded edges. Each rasterizer call runs one forward and one backward walk, and
 * g_scanConvertMode selects which table each direction fills; the four expansions model retail's
 * separate loop bodies per direction and table.
 */
#define ZRNDR_SCAN_EDGE_CHAIN_FORWARD(edgeTable, edgeCount)                                                            \
    edgeVertexIndex = topVertexIndex;                                                                                  \
    ZRNDR_FIXED16_FROM_FLOAT(fixed16Value, vertices[edgeVertexIndex].y);                                               \
    edgeYStart = (fixed16Value + 0x7fff) >> 16;                                                                        \
    edgeSampleY = (float)(edgeYStart) - -0.5f;                                                                         \
    do {                                                                                                               \
        nextIndex = edgeVertexIndex + 1;                                                                               \
        if (nextIndex >= vertCount) {                                                                                  \
            nextIndex -= vertCount;                                                                                    \
        }                                                                                                              \
        ZRNDR_SCAN_EDGE_ADD(edgeTable, edgeCount);                                                                     \
    } while (edgeVertexIndex != bottomVertexIndex)

/**
 * Purpose: Walk the polygon outline from topVertexIndex to bottomVertexIndex in decreasing vertex
 * order, wrapping only below 0, and record each edge that reaches a sample row.
 * Inputs: edgeTable and edgeCount as for ZRNDR_SCAN_EDGE_ADD. Captured walk state: vertices,
 * vertCount, topVertexIndex and bottomVertexIndex (read); edgeVertexIndex, nextIndex, edgeYStart,
 * edgeSampleY, fixed16Value and fixed16Bits (walk scratch, left at their final values).
 * Outputs: starts the sample row at the top vertex, then runs a post-tested loop that expands
 * ZRNDR_SCAN_EDGE_ADD per edge until edgeVertexIndex reaches bottomVertexIndex; edgeCount ends as
 * the number of recorded edges. It is the counterpart of ZRNDR_SCAN_EDGE_CHAIN_FORWARD.
 */
#define ZRNDR_SCAN_EDGE_CHAIN_BACKWARD(edgeTable, edgeCount)                                                           \
    edgeVertexIndex = topVertexIndex;                                                                                  \
    ZRNDR_FIXED16_FROM_FLOAT(fixed16Value, vertices[edgeVertexIndex].y);                                               \
    edgeYStart = (fixed16Value + 0x7fff) >> 16;                                                                        \
    edgeSampleY = (float)(edgeYStart) - -0.5f;                                                                         \
    do {                                                                                                               \
        nextIndex = edgeVertexIndex - 1;                                                                               \
        if (nextIndex < 0) {                                                                                           \
            nextIndex += vertCount;                                                                                    \
        }                                                                                                              \
        ZRNDR_SCAN_EDGE_ADD(edgeTable, edgeCount);                                                                     \
    } while (edgeVertexIndex != bottomVertexIndex)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-rasterizepolywithspanlist
 * @recoil-artifact defines .text recoil:function:0x492000: zRndrRasterizePolyWithSpanList
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.poly.plane-gradient.rasterize-span-list recoil:function:0x492000
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.poly.plane-gradient.rasterize-span-list
 *
 *
 * Purpose: Rasterize one polygon through the active span-list builder and selected span routine.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * That original file name is hypothetical and does not imply C++ compilation: the governed
 * reconstruction compiles this definition as C89 in the C unit zrndr_poly.c under the VC5SP3 x86
 * /TC profile (/O2 /Ob0 /G5 /Gr /Zp4).
 * Raw assembly: one in-body plane-gradient island at retail [0x492035,0x4920b4), the hand-written
 * x87 depth-plane solve with ftst/wait (d9 e4 occurs only three times in retail .text: this
 * island and its copies in 0x492f00 and 0x495850).
 * Under the governed VC5SP3 C profile, the retained current-source and same-body native-C controls
 * do not reproduce this retail sequence; their determinant tests use pooled-zero comparisons. The
 * three retail copies and their surrounding operand captures support this address-scoped assembly
 * reconstruction.
 * Island contract:
 * Purpose: Compute this consumer's two-component plane gradient using the retail x87 evaluation
 * sequence.
 * Inputs: plane0, plane1 and plane2 (&planeVerts[0..2]) supply the three vertices' x/y
 * coordinates; planeZ0, planeZ1 and planeZ2, scalar copies of planeVerts[0..2].z, supply the
 * three depth values. slopeOut addresses the consumer's local two-component result
 * invDepthSlope. The island reads every operand from its compiler-assigned EBP-relative operand
 * home.
 * Effects: Clobbers EAX, EBX, ECX and integer arithmetic flags. The block does not modify EDX,
 * ESI, EDI, EBP or ESP. The compiler owns register preservation, operand homes, the frame and
 * surrounding C.
 * x87: Requires an empty register stack; maximum live depth is eight; returns an empty stack on
 * normal completion of either path. The control word is unchanged; status and exception state
 * are not preserved.
 * Branch: Uses C3 from FTST, not a portable C != predicate. The flat arm stores the current
 * determinant to both binary32 output components, then discards the four remaining coordinate
 * deltas. The other arm stores the two computed gradient components. Preserve the exact
 * instruction order.
 * Fallback: the native-C branch is a nonmatching algorithmic fallback. It implements the same
 * plane-gradient equations but is not an unconditional bitwise or floating-point-environment
 * equivalent of the island: intermediate rounding, exceptional values and exception timing may
 * differ. Its determinant != 0.0f test is not a portable spelling of the island's
 * C3-only decision.
 */
void __fastcall zRndrRasterizePolyWithSpanList(zVec3* vertices, zVec3* planeVerts, int vertCount, int spanOpContext)
{
    zVec2 invDepthSlope;
    zVec2* const slopeOut = &invDepthSlope;
    const float planeZ2 = planeVerts[2].z;
    const float planeZ1 = planeVerts[1].z;
    const float planeZ0 = planeVerts[0].z;
    const zVec3* const plane0 = &planeVerts[0];
    const zVec3* const plane2 = &planeVerts[2];
    const zVec3* const plane1 = &planeVerts[1];
    float invDepthBiasBase;
    int topVertexIndex;
    int bottomVertexIndex;
    int i;
    ScanConvertEdge edgeTableA[0x40];
    ScanConvertEdge edgeTableB[0x40];
    int edgeCountA;
    int edgeCountB;
    double fixed16Bits;
    int fixed16Value;
    int edgeVertexIndex;
    int nextIndex;
    int edgeYStart;
    float edgeSampleY;
    int firstScanline;
    int lastScanline;
    SpanNodePartial* visibleSpans[0x141];
    int edgeIndexA;
    int edgeIndexB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    unsigned char* scanlineBase;
    int y;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, plane0
        mov ebx, plane1
        mov ecx, plane2
        fld dword ptr [ebx]zVec3.x
        fld st(0)
        fsubr dword ptr [eax]zVec3.x
        fxch st(1)
        fsubr dword ptr [ecx]zVec3.x
        fld dword ptr [ebx]zVec3.y
        fld st(0)
        fsubr dword ptr [eax]zVec3.y
        fxch st(1)
        fsubr dword ptr [ecx]zVec3.y
        fld st(1)
        fmul st, st(3)
        fld st(1)
        fmul st, st(5)
        fsubrp st(1), st
        ftst
        wait
        fnstsw ax
        test ah, 40h
        jne recoil_plane_gradient_flat
        fld planeZ1
        fld st(0)
        fsubr planeZ0
        fxch st(1)
        fsubr planeZ2
        fxch st(1)
        fxch st(2)
        fld1
        fdivrp st(1), st
        fld st(1)
        fmulp st(5), st
        fld st(2)
        fmulp st(4), st
        fxch st(4)
        fsubp st(3), st
        fld st(3)
        fmulp st(3), st
        fmulp st(5), st
        fmulp st(3), st
        fxch st(3)
        fsubrp st(2), st
        fmulp st(1), st
        fxch st(1)
        mov eax, slopeOut
        fstp dword ptr [eax]zVec2.x
        fstp dword ptr [eax]zVec2.y
        jmp recoil_plane_gradient_done
    recoil_plane_gradient_flat:
        mov eax, slopeOut
        fst dword ptr [eax]zVec2.x
        fstp dword ptr [eax]zVec2.y
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
    recoil_plane_gradient_done:
    }
#else
    {
        const float dx10 = plane0->x - plane1->x;
        const float dx12 = plane2->x - plane1->x;
        const float dy10 = plane0->y - plane1->y;
        const float dy12 = plane2->y - plane1->y;
        const float determinant = dy12 * dx10 - dy10 * dx12;
        slopeOut->x = determinant;
        slopeOut->y = determinant;
        if (determinant != 0.0f) {
            const float dz10 = planeZ0 - planeZ1;
            const float dz12 = planeZ2 - planeZ1;
            const float inverseDeterminant = 1.0f / determinant;
            slopeOut->x = (dy12 * dz10 - dy10 * dz12) * inverseDeterminant;
            slopeOut->y = (dx10 * dz12 - dx12 * dz10) * inverseDeterminant;
        }
    }
#endif

    invDepthBiasBase
        = planeVerts[0].z - (planeVerts[0].x - 0.5f) * invDepthSlope.x - (planeVerts[0].y - 0.5f) * invDepthSlope.y;

    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (i = 1; i < vertCount; ++i) {
        if (vertices[i].y < vertices[topVertexIndex].y) {
            topVertexIndex = i;
        }
        if (vertices[i].y > vertices[bottomVertexIndex].y) {
            bottomVertexIndex = i;
        }
    }

    edgeCountA = 0;
    edgeCountB = 0;
    if (g_scanConvertMode != 0) {
        ZRNDR_SCAN_EDGE_CHAIN_FORWARD(edgeTableA, edgeCountA);
        ZRNDR_SCAN_EDGE_CHAIN_BACKWARD(edgeTableB, edgeCountB);
    } else {
        ZRNDR_SCAN_EDGE_CHAIN_FORWARD(edgeTableB, edgeCountB);
        ZRNDR_SCAN_EDGE_CHAIN_BACKWARD(edgeTableA, edgeCountA);
    }

    ZRNDR_FIXED16_FROM_FLOAT(fixed16Value, vertices[topVertexIndex].y);
    firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_FIXED16_FROM_FLOAT(fixed16Value, vertices[bottomVertexIndex].y);
    lastScanline = (fixed16Value - 0x8041) >> 16;

    edgeIndexA = 0;
    edgeIndexB = 0;
    currentXFixedA = edgeTableA[0].currentXFixed;
    currentXFixedB = edgeTableB[0].currentXFixed;
    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    scanlineBase = (unsigned char*)(g_frameBuffer) + firstScanline * g_pitchBytes;

    for (y = firstScanline; y <= lastScanline; ++y) {
        int xMin;
        int xMax;
        while (y >= edgeTableA[edgeIndexA].yStart && edgeIndexA < edgeCountA) {
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            ++edgeIndexA;
        }

        while (y >= edgeTableB[edgeIndexB].yStart && edgeIndexB < edgeCountB) {
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            ++edgeIndexB;
        }

        if (currentXFixedA <= currentXFixedB) {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            const float rowDepthBase = (float)(y)*invDepthSlope.y + invDepthBiasBase;
            int spanCount;
            int spanIndex;
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            g_spanAllocCursor->invDepth
                = ((float)(xMin)*invDepthSlope.x + rowDepthBase) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*invDepthSlope.x + rowDepthBase) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->depthSlope = invDepthSlope.x;

            g_pfnBuildSpanList(visibleSpans, y, &spanCount);

            for (spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                SpanNodePartial* span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                const int byteOffset = (int)(span->sampleXMin) * g_bytesPerPixel;
                g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + byteOffset);
                g_pfnSelectedSpanOp(spanOpContext, pixelCount);
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanocclusionrasterizeoccluderpoly
 * @recoil-artifact defines .text recoil:function:0x4927d0: zRndr::SpanOcclusionRasterizeOccluderPoly.
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: rasterize one saved occluder polygon into span nodes for each
 * affected screen column.
 *
 * Evidence: BN reduces duplicate/closing polygon vertices into the same
 * scratch base later passed as the span-list pointer array, builds two
 * fixed-point scan-conversion edge tables through duplicated scan-mode
 * branches, stages pending span-node min/max/depth values in
 * gRndr_SpanAllocCursor, and dispatches through gRndr_pfnBuildSpanList.
 */
void __fastcall SpanOcclusionRasterizeOccluderPoly(SpanOccluderPolyPartial* poly, int vertCount)
{
    ScanConvertEdge edgeTableA[0x40];
    ScanConvertEdge edgeTableB[0x40];
    int reducedCount;
    int lastReducedIndex;
    int vertexIndex;
    int topVertexIndex;
    int bottomVertexIndex;
    int scanIndex;
    int edgeCountA;
    int edgeCountB;
    int edgeVertexIndex;
    int fixed16Value;
    int edgeYStart;
    float edgeSampleY;
    ScanConvertEdge* edge;
    int firstScanline;
    int lastScanline;
    int y;
    int edgeIndexA;
    int edgeIndexB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    int xMin;
    int xMax;

    // The reduced vertices and the span list live in sibling blocks; VC5 overlaps them in the frame, which is
    // the shared scratch base BN shows.
    {
        zVec3 reducedVerts[0x40];
        reducedVerts[0].x = poly->vertices[0][0];
        reducedVerts[0].y = poly->vertices[0][1];
        reducedCount = 1;
        lastReducedIndex = 0;
        for (vertexIndex = 1; vertexIndex < vertCount; ++vertexIndex) {
            reducedVerts[reducedCount].x = poly->vertices[vertexIndex][0];
            reducedVerts[reducedCount].y = poly->vertices[vertexIndex][1];
            if (reducedVerts[reducedCount].x != reducedVerts[reducedCount - 1].x
                || reducedVerts[reducedCount].y != reducedVerts[reducedCount - 1].y) {
                ++reducedCount;
                ++lastReducedIndex;
            }
        }

        if (reducedVerts[lastReducedIndex].x == reducedVerts[0].x
            && reducedVerts[lastReducedIndex].y == reducedVerts[0].y) {
            --reducedCount;
        }

        if (reducedCount < 3) {
            return;
        }

        topVertexIndex = 0;
        bottomVertexIndex = 0;
        for (scanIndex = 1; scanIndex < reducedCount; ++scanIndex) {
            if (reducedVerts[scanIndex].y < reducedVerts[topVertexIndex].y) {
                topVertexIndex = scanIndex;
            }
            // Retail keeps the first lowest vertex (strict compare).
            if (reducedVerts[scanIndex].y > reducedVerts[bottomVertexIndex].y) {
                bottomVertexIndex = scanIndex;
            }
        }

        edgeCountA = 0;
        edgeCountB = 0;
        // Same direction-specialized edge walks as zRndrRasterizePoly.
        // Each walk converts its slope, x intercept and next row through one walk-local double, as retail does.
        if (g_scanConvertMode != 0) {
            edgeVertexIndex = topVertexIndex;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            do {
                int nextIndex = edgeVertexIndex + 1;
                if (nextIndex >= reducedCount) {
                    nextIndex -= reducedCount;
                }
                if (edgeSampleY <= reducedVerts[nextIndex].y) {
                    double fixedBits;
                    const float dy = reducedVerts[nextIndex].y - reducedVerts[edgeVertexIndex].y;
                    const zVec3* start = &reducedVerts[edgeVertexIndex];
                    edge = &edgeTableA[edgeCountA++];
                    edge->yStart = edgeYStart;
                    if (dy != 0.0f) {
                        const float xSlope = (reducedVerts[nextIndex].x - start->x) / dy;
                        fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                        edge->xStepFixed = *(int*)(&fixedBits);
                        fixedBits = 6755399441055744.0
                            - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                        edge->currentXFixed = *(int*)(&fixedBits);
                    }
                    fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                    fixed16Value = *(int*)(&fixedBits);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            } while (edgeVertexIndex != bottomVertexIndex);

            edgeVertexIndex = topVertexIndex;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            do {
                int nextIndex = edgeVertexIndex - 1;
                if (nextIndex < 0) {
                    nextIndex += reducedCount;
                }
                if (edgeSampleY <= reducedVerts[nextIndex].y) {
                    double fixedBits;
                    const float dy = reducedVerts[nextIndex].y - reducedVerts[edgeVertexIndex].y;
                    const zVec3* start = &reducedVerts[edgeVertexIndex];
                    edge = &edgeTableB[edgeCountB++];
                    edge->yStart = edgeYStart;
                    if (dy != 0.0f) {
                        const float xSlope = (reducedVerts[nextIndex].x - start->x) / dy;
                        fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                        edge->xStepFixed = *(int*)(&fixedBits);
                        fixedBits = 6755399441055744.0
                            - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                        edge->currentXFixed = *(int*)(&fixedBits);
                    }
                    fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                    fixed16Value = *(int*)(&fixedBits);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            } while (edgeVertexIndex != bottomVertexIndex);
        } else {
            edgeVertexIndex = topVertexIndex;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            do {
                int nextIndex = edgeVertexIndex + 1;
                if (nextIndex >= reducedCount) {
                    nextIndex -= reducedCount;
                }
                if (edgeSampleY <= reducedVerts[nextIndex].y) {
                    double fixedBits;
                    const float dy = reducedVerts[nextIndex].y - reducedVerts[edgeVertexIndex].y;
                    const zVec3* start = &reducedVerts[edgeVertexIndex];
                    edge = &edgeTableB[edgeCountB++];
                    edge->yStart = edgeYStart;
                    if (dy != 0.0f) {
                        const float xSlope = (reducedVerts[nextIndex].x - start->x) / dy;
                        fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                        edge->xStepFixed = *(int*)(&fixedBits);
                        fixedBits = 6755399441055744.0
                            - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                        edge->currentXFixed = *(int*)(&fixedBits);
                    }
                    fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                    fixed16Value = *(int*)(&fixedBits);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            } while (edgeVertexIndex != bottomVertexIndex);

            edgeVertexIndex = topVertexIndex;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            do {
                int nextIndex = edgeVertexIndex - 1;
                if (nextIndex < 0) {
                    nextIndex += reducedCount;
                }
                if (edgeSampleY <= reducedVerts[nextIndex].y) {
                    double fixedBits;
                    const float dy = reducedVerts[nextIndex].y - reducedVerts[edgeVertexIndex].y;
                    const zVec3* start = &reducedVerts[edgeVertexIndex];
                    edge = &edgeTableA[edgeCountA++];
                    edge->yStart = edgeYStart;
                    if (dy != 0.0f) {
                        const float xSlope = (reducedVerts[nextIndex].x - start->x) / dy;
                        fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                        edge->xStepFixed = *(int*)(&fixedBits);
                        fixedBits = 6755399441055744.0
                            - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                        edge->currentXFixed = *(int*)(&fixedBits);
                    }
                    fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                    fixed16Value = *(int*)(&fixedBits);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            } while (edgeVertexIndex != bottomVertexIndex);
        }

        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
        firstScanline = (fixed16Value + 0x7fff) >> 16;
        currentXFixedA = edgeTableA[0].currentXFixed;
        currentXFixedB = edgeTableB[0].currentXFixed;
        edgeIndexB = 0;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[bottomVertexIndex].y);
        lastScanline = (fixed16Value - 0x8041) >> 16;
    }
    edgeIndexA = 0;
    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    {
        SpanNodePartial* spanList[0x141];
        int spanCount;
        for (y = firstScanline; y <= lastScanline; ++y) {
            while (y >= edgeTableA[edgeIndexA].yStart && edgeIndexA < edgeCountA) {
                currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
                xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
                ++edgeIndexA;
            }

            while (y >= edgeTableB[edgeIndexB].yStart && edgeIndexB < edgeCountB) {
                currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
                xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
                ++edgeIndexB;
            }

            if (currentXFixedA <= currentXFixedB) {
                xMin = (currentXFixedA + 0x7fff) >> 16;
                xMax = (currentXFixedB - 0x8001) >> 16;
            } else {
                xMin = (currentXFixedB + 0x7fff) >> 16;
                xMax = (currentXFixedA - 0x8001) >> 16;
            }

            currentXFixedA += xStepFixedA;
            currentXFixedB += xStepFixedB;
            if (xMin <= xMax) {
                g_spanAllocCursor->sampleXMin = xMin;
                g_spanAllocCursor->sampleXMax = xMax;
                g_spanAllocCursor->invDepth = poly->vertices[0][2];
                g_spanAllocCursor->invDepthStep = poly->vertices[0][2];
                g_spanAllocCursor->depthSlope = 0.0f;
                g_pfnBuildSpanList(spanList, y, &spanCount);
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawflatimmediate
 * @recoil-artifact defines .text recoil:function:0x492f00: zRndrDrawFlatImmediate
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.poly.plane-gradient.flat-immediate recoil:function:0x492f00
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.poly.plane-gradient.flat-immediate
 *
 *
 * Purpose: Draw an immediate flat polygon through the flat span callback path.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * That original file name is hypothetical and does not imply C++ compilation: the governed
 * reconstruction compiles this definition as C89 in the C unit zrndr_poly.c under the VC5SP3 x86
 * /TC profile (/O2 /Ob0 /G5 /Gr /Zp4).
 * Raw assembly: one in-body plane-gradient island at retail [0x492f35,0x492fb4), the hand-written
 * x87 depth-plane solve with ftst/wait (d9 e4 occurs only three times in retail .text: this
 * island and its copies in 0x492000 and 0x495850).
 * Under the governed VC5SP3 C profile, the retained current-source and same-body native-C controls
 * do not reproduce this retail sequence; their determinant tests use pooled-zero comparisons. The
 * three retail copies and their surrounding operand captures support this address-scoped assembly
 * reconstruction.
 * Island contract:
 * Purpose: Compute this consumer's two-component plane gradient using the retail x87 evaluation
 * sequence.
 * Inputs: plane0, plane1 and plane2 (&planeVertices[0..2]) supply the three vertices' x/y
 * coordinates; planeZ0, planeZ1 and planeZ2, scalar copies of planeVertices[0..2].z, supply the
 * three depth values. slopeOut addresses the consumer's local two-component result
 * invDepthSlope. The island reads every operand from its compiler-assigned EBP-relative operand
 * home.
 * Effects: Clobbers EAX, EBX, ECX and integer arithmetic flags. The block does not modify EDX,
 * ESI, EDI, EBP or ESP. The compiler owns register preservation, operand homes, the frame and
 * surrounding C.
 * x87: Requires an empty register stack; maximum live depth is eight; returns an empty stack on
 * normal completion of either path. The control word is unchanged; status and exception state
 * are not preserved.
 * Branch: Uses C3 from FTST, not a portable C != predicate. The flat arm stores the current
 * determinant to both binary32 output components, then discards the four remaining coordinate
 * deltas. The other arm stores the two computed gradient components. Preserve the exact
 * instruction order.
 * Fallback: the native-C branch is a nonmatching algorithmic fallback. It implements the same
 * plane-gradient equations but is not an unconditional bitwise or floating-point-environment
 * equivalent of the island: intermediate rounding, exceptional values and exception timing may
 * differ. Its determinant != 0.0f test is not a portable spelling of the island's
 * C3-only decision.
 */
void __fastcall
zRndrDrawFlatImmediate(zVec3* vertices, zVec3* planeVertices, int vertCount, int flatSpanOpEdxArg, int flatSpanOpEcxArg)
{
    zVec2 invDepthSlope;
    zVec2* const slopeOut = &invDepthSlope;
    const float planeZ2 = planeVertices[2].z;
    const float planeZ1 = planeVertices[1].z;
    const float planeZ0 = planeVertices[0].z;
    const zVec3* const plane0 = &planeVertices[0];
    const zVec3* const plane2 = &planeVertices[2];
    const zVec3* const plane1 = &planeVertices[1];
    float invDepthBiasBase;
    int topVertexIndex;
    int bottomVertexIndex;
    int i;
    ScanConvertEdge edgeTableA[0x40];
    ScanConvertEdge edgeTableB[0x40];
    int edgeCountA;
    int edgeCountB;
    double fixed16Bits;
    int fixed16Value;
    int edgeVertexIndex;
    int nextIndex;
    int edgeYStart;
    float edgeSampleY;
    int firstScanline;
    int lastScanline;
    SpanNodePartial* visibleSpans[0x141];
    int edgeIndexA;
    int edgeIndexB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    unsigned char* scanlineBase;
    int y;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, plane0
        mov ebx, plane1
        mov ecx, plane2
        fld dword ptr [ebx]zVec3.x
        fld st(0)
        fsubr dword ptr [eax]zVec3.x
        fxch st(1)
        fsubr dword ptr [ecx]zVec3.x
        fld dword ptr [ebx]zVec3.y
        fld st(0)
        fsubr dword ptr [eax]zVec3.y
        fxch st(1)
        fsubr dword ptr [ecx]zVec3.y
        fld st(1)
        fmul st, st(3)
        fld st(1)
        fmul st, st(5)
        fsubrp st(1), st
        ftst
        wait
        fnstsw ax
        test ah, 40h
        jne recoil_plane_gradient_flat
        fld planeZ1
        fld st(0)
        fsubr planeZ0
        fxch st(1)
        fsubr planeZ2
        fxch st(1)
        fxch st(2)
        fld1
        fdivrp st(1), st
        fld st(1)
        fmulp st(5), st
        fld st(2)
        fmulp st(4), st
        fxch st(4)
        fsubp st(3), st
        fld st(3)
        fmulp st(3), st
        fmulp st(5), st
        fmulp st(3), st
        fxch st(3)
        fsubrp st(2), st
        fmulp st(1), st
        fxch st(1)
        mov eax, slopeOut
        fstp dword ptr [eax]zVec2.x
        fstp dword ptr [eax]zVec2.y
        jmp recoil_plane_gradient_done
    recoil_plane_gradient_flat:
        mov eax, slopeOut
        fst dword ptr [eax]zVec2.x
        fstp dword ptr [eax]zVec2.y
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
    recoil_plane_gradient_done:
    }
#else
    {
        const float dx10 = plane0->x - plane1->x;
        const float dx12 = plane2->x - plane1->x;
        const float dy10 = plane0->y - plane1->y;
        const float dy12 = plane2->y - plane1->y;
        const float determinant = dy12 * dx10 - dy10 * dx12;
        slopeOut->x = determinant;
        slopeOut->y = determinant;
        if (determinant != 0.0f) {
            const float dz10 = planeZ0 - planeZ1;
            const float dz12 = planeZ2 - planeZ1;
            const float inverseDeterminant = 1.0f / determinant;
            slopeOut->x = (dy12 * dz10 - dy10 * dz12) * inverseDeterminant;
            slopeOut->y = (dx10 * dz12 - dx12 * dz10) * inverseDeterminant;
        }
    }
#endif

    invDepthBiasBase = planeVertices[0].z - (planeVertices[0].x - 0.5f) * invDepthSlope.x
        - (planeVertices[0].y - 0.5f) * invDepthSlope.y;

    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (i = 1; i < vertCount; ++i) {
        if (vertices[i].y < vertices[topVertexIndex].y) {
            topVertexIndex = i;
        }
        if (vertices[i].y > vertices[bottomVertexIndex].y) {
            bottomVertexIndex = i;
        }
    }

    edgeCountA = 0;
    edgeCountB = 0;
    if (g_scanConvertMode != 0) {
        ZRNDR_SCAN_EDGE_CHAIN_FORWARD(edgeTableA, edgeCountA);
        ZRNDR_SCAN_EDGE_CHAIN_BACKWARD(edgeTableB, edgeCountB);
    } else {
        ZRNDR_SCAN_EDGE_CHAIN_FORWARD(edgeTableB, edgeCountB);
        ZRNDR_SCAN_EDGE_CHAIN_BACKWARD(edgeTableA, edgeCountA);
    }

    ZRNDR_FIXED16_FROM_FLOAT(fixed16Value, vertices[topVertexIndex].y);
    firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_FIXED16_FROM_FLOAT(fixed16Value, vertices[bottomVertexIndex].y);
    lastScanline = (fixed16Value - 0x8041) >> 16;

    edgeIndexA = 0;
    edgeIndexB = 0;
    currentXFixedA = edgeTableA[0].currentXFixed;
    currentXFixedB = edgeTableB[0].currentXFixed;
    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    scanlineBase = (unsigned char*)(g_frameBuffer) + firstScanline * g_pitchBytes;

    for (y = firstScanline; y <= lastScanline; ++y) {
        int xMin;
        int xMax;
        while (y >= edgeTableA[edgeIndexA].yStart && edgeIndexA < edgeCountA) {
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            ++edgeIndexA;
        }

        while (y >= edgeTableB[edgeIndexB].yStart && edgeIndexB < edgeCountB) {
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            ++edgeIndexB;
        }

        if (currentXFixedA <= currentXFixedB) {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            const float rowDepthBase = (float)(y)*invDepthSlope.y + invDepthBiasBase;
            int spanCount;
            int spanIndex;
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            g_spanAllocCursor->invDepth
                = ((float)(xMin)*invDepthSlope.x + rowDepthBase) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*invDepthSlope.x + rowDepthBase) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->depthSlope = invDepthSlope.x;

            g_pfnBuildSpanListSecondary(visibleSpans, y, &spanCount);

            for (spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                SpanNodePartial* span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                const int byteOffset = (int)(span->sampleXMin) * g_bytesPerPixel;
                g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + byteOffset);
                g_pfnFlatImmediateSpanOp(flatSpanOpEcxArg, flatSpanOpEdxArg, pixelCount);
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-rasterizepoly
 * @recoil-artifact defines .text recoil:function:0x4936d0: zRndrRasterizePoly
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Scan-convert a polygon and dispatch each covered span to the selected span routine.
 */
void __fastcall zRndrRasterizePoly(zVec3* vertices, int vertCount, int spanOpContext)
{
    zVec3 reducedVerts[0x40];
    ScanConvertEdge edgeTableA[0x40];
    ScanConvertEdge edgeTableB[0x40];
    int reducedCount;
    int lastReducedIndex;
    int vertexIndex;
    int topVertexIndex;
    int bottomVertexIndex;
    int edgeCountA;
    int edgeCountB;
    int edgeVertexIndex;
    int fixed16Value;
    int edgeYStart;
    float edgeSampleY;
    ScanConvertEdge* edge;
    int firstScanline;
    int lastScanline;
    int y;
    int edgeIndexA;
    int edgeIndexB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    int xStart;
    int xEnd;
    int pixelCount;
    unsigned char* scanlineBase;

    // Retail tracks the reduced count and the last reduced index separately; the closing-vertex test uses the index.
    reducedVerts[0].x = vertices[0].x;
    reducedVerts[0].y = vertices[0].y;
    reducedCount = 1;
    lastReducedIndex = 0;
    for (vertexIndex = 1; vertexIndex < vertCount; ++vertexIndex) {
        reducedVerts[reducedCount].x = vertices[vertexIndex].x;
        reducedVerts[reducedCount].y = vertices[vertexIndex].y;
        if (reducedVerts[reducedCount].x != reducedVerts[reducedCount - 1].x
            || reducedVerts[reducedCount].y != reducedVerts[reducedCount - 1].y) {
            ++reducedCount;
            ++lastReducedIndex;
        }
    }

    if (reducedVerts[lastReducedIndex].x == reducedVerts[0].x
        && reducedVerts[lastReducedIndex].y == reducedVerts[0].y) {
        --reducedCount;
    }

    if (reducedCount < 3) {
        return;
    }

    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (vertexIndex = 1; vertexIndex < reducedCount; ++vertexIndex) {
        if (reducedVerts[vertexIndex].y < reducedVerts[topVertexIndex].y) {
            topVertexIndex = vertexIndex;
        }
        // Retail keeps the first lowest vertex (strict compare).
        if (reducedVerts[vertexIndex].y > reducedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = vertexIndex;
        }
    }

    edgeCountA = 0;
    edgeCountB = 0;
    // Retail expands one direction-specialized edge walk per table and scan mode. Each walk starts from the top vertex
    // (its fixed-point Y is computed once and reused), stores only yStart for horizontal edges, and derives the
    // x intercept from the sample row (edgeYStart + 0.5). Each walk converts through one walk-local double and reads
    // both edge endpoints through start/end references (retail next-index/next-offset register roles).
    if (g_scanConvertMode != 0) {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= reducedCount) {
                nextIndex -= reducedCount;
            }
            if (edgeSampleY <= reducedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &reducedVerts[edgeVertexIndex];
                const zVec3* end = &reducedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += reducedCount;
            }
            if (edgeSampleY <= reducedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &reducedVerts[edgeVertexIndex];
                const zVec3* end = &reducedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    } else {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= reducedCount) {
                nextIndex -= reducedCount;
            }
            if (edgeSampleY <= reducedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &reducedVerts[edgeVertexIndex];
                const zVec3* end = &reducedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += reducedCount;
            }
            if (edgeSampleY <= reducedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &reducedVerts[edgeVertexIndex];
                const zVec3* end = &reducedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(reducedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
    firstScanline = (fixed16Value + 0x7fff) >> 16;
    // Retail converts the bottom vertex before forming scanlineBase (fld at 0x493cac precedes the add at 0x493cb6).
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[bottomVertexIndex].y);
    scanlineBase = (unsigned char*)(g_frameBuffer) + firstScanline * g_pitchBytes;
    lastScanline = (fixed16Value - 0x8041) >> 16;
    edgeIndexB = 0;
    edgeIndexA = 0;
    currentXFixedB = edgeTableB[0].currentXFixed;
    currentXFixedA = edgeTableA[0].currentXFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    xStepFixedA = edgeTableA[0].xStepFixed;
    if (firstScanline > lastScanline) {
        return;
    }

    for (y = firstScanline; y <= lastScanline; ++y) {
        while (y >= edgeTableA[edgeIndexA].yStart && edgeIndexA < edgeCountA) {
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            ++edgeIndexA;
        }

        while (y >= edgeTableB[edgeIndexB].yStart && edgeIndexB < edgeCountB) {
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            ++edgeIndexB;
        }

        if (currentXFixedA <= currentXFixedB) {
            xStart = (currentXFixedA + 0x7fff) >> 16;
            xEnd = (currentXFixedB - 0x8001) >> 16;
        } else {
            xStart = (currentXFixedB + 0x7fff) >> 16;
            xEnd = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xStart <= xEnd) {
            pixelCount = xEnd - xStart;
            if (pixelCount > 0) {
                g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + xStart * g_bytesPerPixel);
                g_pfnSelectedSpanOp(spanOpContext, pixelCount);
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawflatqueued
 * @recoil-artifact defines .text recoil:function:0x493df0: zRndrDrawFlatQueued
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Draw a queued flat/textured polygon through the active span callback path.
 */
void __fastcall zRndrDrawFlatQueued(
    zImage_TexDirEntryPartial* entry,
    zVec3* polyVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertCount,
    int paletteIndex
)
{
    ScanConvertEdge edgeTableB[0x40];
    ScanConvertEdge edgeTableA[0x40];
    SpanNodePartial* spanList[0x141];
    zVidImagePartial* image;
    zVidImagePartial* selectedImage;
    float dx01;
    float dx21;
    float dy01;
    float dy21;
    float determinant;
    float inverseDeterminant;
    float imageWidth;
    float imageHeight;
    zVec2 scaledUV[3];
    zVec2 recipZGrad;
    zVec2 uGrad;
    zVec2 vGrad;
    float dz01;
    float dz21;
    float du01;
    float du21;
    float dv01;
    float dv21;
    int vertexIndex;
    int topVertexIndex;
    int bottomVertexIndex;
    int edgeCountA;
    int edgeCountB;
    int edgeVertexIndex;
    int fixed16Value;
    int edgeYStart;
    float edgeSampleY;
    ScanConvertEdge* edge;
    int firstScanline;
    int lastScanline;
    unsigned char* scanlineBase;
    TexturedQueuedSpanProc spanProc;
    unsigned short* palette;
    int edgeIndexA;
    int edgeIndexB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    float originX;
    float originY;
    int y;
    int xMin;
    int xMax;
    float planeY;
    float rowRecipZ;
    int spanCount;
    float rowU;
    float rowV;
    int spanIndex;
    SpanNodePartial* span;
    int count;
    float startX;
    float endX;
    float stepScale;
    float startInvZ;
    float endInvZ;
    float startU;
    float startV;
    float endU;
    float endV;
    double texUStartBits;
    double texVStartBits;
    double texUStepBits;
    double texVStepBits;

    image = entry != 0 ? entry->image : 0;
    dx21 = triVerts[2].x - triVerts[1].x;
    dy01 = triVerts[0].y - triVerts[1].y;
    dx01 = triVerts[0].x - triVerts[1].x;
    dy21 = triVerts[2].y - triVerts[1].y;
    determinant = dy21 * dx01 - dx21 * dy01;
    imageWidth = (float)(image->width);
    imageHeight = (float)(image->height);
    scaledUV[0].x = imageWidth * triVerts[0].z * triUVs[0].x;
    scaledUV[0].y = imageHeight * triVerts[0].z * triUVs[0].y;
    scaledUV[1].x = imageWidth * triVerts[1].z * triUVs[1].x;
    scaledUV[1].y = imageHeight * triVerts[1].z * triUVs[1].y;
    scaledUV[2].x = imageWidth * triVerts[2].z * triUVs[2].x;
    scaledUV[2].y = imageHeight * triVerts[2].z * triUVs[2].y;
    if (determinant != 0.0) {
        inverseDeterminant = -1.0f / determinant;
        dz01 = triVerts[0].z - triVerts[1].z;
        dz21 = triVerts[2].z - triVerts[1].z;
        du01 = scaledUV[0].x - scaledUV[1].x;
        du21 = scaledUV[2].x - scaledUV[1].x;
        dv01 = scaledUV[0].y - scaledUV[1].y;
        dv21 = scaledUV[2].y - scaledUV[1].y;
        recipZGrad.x = (dz21 * dy01 - dz01 * dy21) * inverseDeterminant;
        recipZGrad.y = (dz01 * dx21 - dz21 * dx01) * inverseDeterminant;
        uGrad.x = (du21 * dy01 - du01 * dy21) * inverseDeterminant;
        uGrad.y = (du01 * dx21 - du21 * dx01) * inverseDeterminant;
        vGrad.x = (dv21 * dy01 - dv01 * dy21) * inverseDeterminant;
        vGrad.y = (dv01 * dx21 - dv21 * dx01) * inverseDeterminant;
    } else {
        recipZGrad.x = recipZGrad.y = uGrad.x = uGrad.y = vGrad.x = vGrad.y = 0.0f;
    }

    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (vertexIndex = 1; vertexIndex < vertCount; ++vertexIndex) {
        if (polyVerts[vertexIndex].y < polyVerts[topVertexIndex].y) {
            topVertexIndex = vertexIndex;
        }
        // Retail keeps the first lowest vertex (strict compare).
        if (polyVerts[vertexIndex].y > polyVerts[bottomVertexIndex].y) {
            bottomVertexIndex = vertexIndex;
        }
    }

    edgeCountA = 0;
    edgeCountB = 0;
    // Each walk converts its slope, x intercept and next row through one walk-local double, as retail does.
    if (g_scanConvertMode != 0) {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                double fixedBits;
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                double fixedBits;
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    } else {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                double fixedBits;
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                double fixedBits;
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    }

    currentXFixedA = edgeTableA[0].currentXFixed;
    currentXFixedB = edgeTableB[0].currentXFixed;
    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
    firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[bottomVertexIndex].y);
    lastScanline = (fixed16Value - 0x8041) >> 16;
    edgeIndexA = 0;
    edgeIndexB = 0;
    scanlineBase = (unsigned char*)(g_frameBuffer) + firstScanline * g_pitchBytes;

    if (entry != 0 && entry->nextVariant != 0) {
        selectedImage = zRndrTextureMipSelectVariantImage(entry, triVerts, 3, scaledUV, &recipZGrad, &uGrad, &vGrad);
    } else {
        selectedImage = image;
    }

    g_spanActiveTexPixels = (unsigned char*)(selectedImage->pixels);
    if (selectedImage->alphaMap != 0) {
        g_spanActiveTexAlphaMap = selectedImage->alphaMap;
        g_pfnSelectedSpanOp_Mode0 = g_pfnFlatQueuedSpanOp_Mode0;
        g_pfnSelectedSpanOp_Mode1 = g_pfnFlatQueuedSpanOpAlt_Mode0;
    } else {
        g_spanActiveTexAlphaMap = 0;
        g_pfnSelectedSpanOp_Mode0 = g_pfnFlatQueuedSpanOp_Mode1;
        g_pfnSelectedSpanOp_Mode1 = g_pfnFlatQueuedSpanOpAlt_Mode1;
    }

    palette = (unsigned short*)(selectedImage->palette);
    if (palette == 0) {
        spanProc = g_pfnSelectedSpanOp_Mode0;
    } else {
        g_spanActiveTexPalette = paletteIndex == -1 ? palette : &palette[(paletteIndex + 1) * 0x100];
        spanProc = g_pfnSelectedSpanOp_Mode1;
    }
    g_spanActiveTexShift = selectedImage->uShiftFrom20;
    g_spanActiveTexUMask = selectedImage->uMask;
    g_spanActiveTexVMask = selectedImage->vMaskFixed20;

    originX = triVerts[0].x - 0.5f;
    originY = triVerts[0].y - 0.5f;

    for (y = firstScanline; y <= lastScanline; ++y) {
        while (y >= edgeTableA[edgeIndexA].yStart && edgeIndexA < edgeCountA) {
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            ++edgeIndexA;
        }

        while (y >= edgeTableB[edgeIndexB].yStart && edgeIndexB < edgeCountB) {
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            ++edgeIndexB;
        }

        if (currentXFixedA <= currentXFixedB) {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            planeY = (float)(y)-originY;
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            rowRecipZ = planeY * recipZGrad.y + triVerts[0].z;
            g_spanAllocCursor->invDepth
                = (((float)(xMin)-originX) * recipZGrad.x + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->invDepthStep
                = (((float)(xMax)-originX) * recipZGrad.x + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->depthSlope = recipZGrad.x;
            g_pfnBuildSpanListSecondary(spanList, y, &spanCount);
            if (spanCount != 0) {
                rowU = planeY * uGrad.y + scaledUV[0].x;
                rowV = planeY * vGrad.y + scaledUV[0].y;
                for (spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    span = spanList[spanIndex];
                    count = span->sampleXMax - span->sampleXMin + 1;
                    g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + span->sampleXMin * g_bytesPerPixel);
                    startX = (float)(span->sampleXMin) - originX;
                    endX = (float)(span->sampleXMax) - originX;
                    stepScale = 1048576.0f / (float)(count);
                    startInvZ = 1.0f / (startX * recipZGrad.x + rowRecipZ);
                    startU = (startX * uGrad.x + rowU) * startInvZ;
                    startV = (startX * vGrad.x + rowV) * startInvZ;
                    texUStartBits = 6755399441055744.0 - (double)(startU * -1048576.0f);
                    texVStartBits = 6755399441055744.0 - (double)(startV * -1048576.0f);
                    endInvZ = 1.0f / (endX * recipZGrad.x + rowRecipZ);
                    endU = (endX * uGrad.x + rowU) * endInvZ;
                    endV = (endX * vGrad.x + rowV) * endInvZ;
                    texUStepBits = (double)((endU - startU) * stepScale) - -6755399441055744.0;
                    texVStepBits = (double)((endV - startV) * stepScale) - -6755399441055744.0;
                    g_spanActiveTexUStepFixed20 = *(int*)(&texUStepBits);
                    g_spanActiveTexVStepFixed20 = *(int*)(&texVStepBits);
                    spanProc(*(int*)(&texUStartBits), *(int*)(&texVStartBits), count, g_spanActiveTexShift);
                }
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-renderer-drawpolytlv
 * @recoil-artifact defines .text recoil:function:0x494af0: RendererDrawPolyTLV
 *
 *
 * Purpose: Draw a transformed lit polygon through the active software texture span path.
 */
void __fastcall RendererDrawPolyTLV(
    zImage_TexDirEntryPartial* entry,
    zVec3* polyVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertexCount,
    float alpha,
    int texKey
)
{
    ScanConvertEdge edgeTableB[0x40];
    ScanConvertEdge edgeTableA[0x40];
    SpanNodePartial* spanList[0x141];
    zVidImagePartial* image;
    zVidImagePartial* selectedImage;
    float dx01;
    float dx21;
    float dy01;
    float dy21;
    float determinant;
    float inverseDeterminant;
    float imageWidth;
    float imageHeight;
    zVec2 scaledUV[3];
    zVec2 recipZGrad;
    zVec2 uGrad;
    zVec2 vGrad;
    float dz01;
    float dz21;
    float du01;
    float du21;
    float dv01;
    float dv21;
    int vertexIndex;
    int topVertexIndex;
    int bottomVertexIndex;
    int edgeCountA;
    int edgeCountB;
    int edgeVertexIndex;
    int fixed16Value;
    double fixedBits;
    int edgeYStart;
    float edgeSampleY;
    ScanConvertEdge* edge;
    int firstScanline;
    int lastScanline;
    unsigned char* scanlineBase;
    TexturedQueuedSpanProc spanProc;
    unsigned short* palette;
    int edgeIndexA;
    int edgeIndexB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    int y;
    int xMin;
    int xMax;
    float planeY;
    float rowRecipZ;
    int spanCount;
    float rowU;
    float rowV;
    int spanIndex;
    SpanNodePartial* span;
    int count;
    float startX;
    float endX;
    float startInvZ;
    float endInvZ;
    float startU;
    float startV;
    float endU;
    float endV;
    double alphaBits;
    double texUStartBits;
    double texVStartBits;
    double texUStepBits;
    double texVStepBits;

    image = entry != 0 ? entry->image : 0;
    dx21 = triVerts[2].x - triVerts[1].x;
    dy01 = triVerts[0].y - triVerts[1].y;
    dx01 = triVerts[0].x - triVerts[1].x;
    dy21 = triVerts[2].y - triVerts[1].y;
    determinant = dy21 * dx01 - dx21 * dy01;
    imageWidth = (float)(image->width);
    imageHeight = (float)(image->height);
    scaledUV[0].x = imageWidth * triVerts[0].z * triUVs[0].x;
    scaledUV[0].y = imageHeight * triVerts[0].z * triUVs[0].y;
    scaledUV[1].x = imageWidth * triVerts[1].z * triUVs[1].x;
    scaledUV[1].y = imageHeight * triVerts[1].z * triUVs[1].y;
    scaledUV[2].x = imageWidth * triVerts[2].z * triUVs[2].x;
    scaledUV[2].y = imageHeight * triVerts[2].z * triUVs[2].y;
    if (determinant != 0.0) {
        inverseDeterminant = -1.0f / determinant;
        dz01 = triVerts[0].z - triVerts[1].z;
        dz21 = triVerts[2].z - triVerts[1].z;
        du01 = scaledUV[0].x - scaledUV[1].x;
        du21 = scaledUV[2].x - scaledUV[1].x;
        dv01 = scaledUV[0].y - scaledUV[1].y;
        dv21 = scaledUV[2].y - scaledUV[1].y;
        recipZGrad.x = (dz21 * dy01 - dz01 * dy21) * inverseDeterminant;
        recipZGrad.y = (dz01 * dx21 - dz21 * dx01) * inverseDeterminant;
        uGrad.x = (du21 * dy01 - du01 * dy21) * inverseDeterminant;
        uGrad.y = (du01 * dx21 - du21 * dx01) * inverseDeterminant;
        vGrad.x = (dv21 * dy01 - dv01 * dy21) * inverseDeterminant;
        vGrad.y = (dv01 * dx21 - dv21 * dx01) * inverseDeterminant;
    } else {
        recipZGrad.x = recipZGrad.y = uGrad.x = uGrad.y = vGrad.x = vGrad.y = 0.0f;
    }

    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (vertexIndex = 1; vertexIndex < vertexCount; ++vertexIndex) {
        if (polyVerts[vertexIndex].y < polyVerts[topVertexIndex].y) {
            topVertexIndex = vertexIndex;
        }
        // Retail keeps the first lowest vertex (strict compare).
        if (polyVerts[vertexIndex].y > polyVerts[bottomVertexIndex].y) {
            bottomVertexIndex = vertexIndex;
        }
    }

    edgeCountA = 0;
    edgeCountB = 0;
    // The walks convert slope, x intercept and next row through the shared fixedBits double.
    if (g_scanConvertMode != 0) {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertexCount) {
                nextIndex -= vertexCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertexCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    } else {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertexCount) {
                nextIndex -= vertexCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertexCount;
            }
            if (edgeSampleY <= polyVerts[nextIndex].y) {
                const float dy = polyVerts[nextIndex].y - polyVerts[edgeVertexIndex].y;
                const zVec3* start = &polyVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (polyVerts[nextIndex].x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(polyVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    }

    currentXFixedA = edgeTableA[0].currentXFixed;
    currentXFixedB = edgeTableB[0].currentXFixed;
    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
    firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[bottomVertexIndex].y);
    lastScanline = (fixed16Value - 0x8041) >> 16;
    edgeIndexA = 0;
    edgeIndexB = 0;
    scanlineBase = (unsigned char*)(g_frameBuffer) + firstScanline * g_pitchBytes;

    if (entry != 0 && entry->nextVariant != 0) {
        selectedImage = zRndrTextureMipSelectVariantImage(entry, triVerts, 3, scaledUV, &recipZGrad, &uGrad, &vGrad);
    } else {
        selectedImage = image;
    }

    g_spanActiveTexPixels = (unsigned char*)(selectedImage->pixels);
    if (selectedImage->alphaMap != 0) {
        // The alpha-mapped span routines take the raw float bits of the constant alpha.
        g_spanActiveConstAlphaBits = *(int*)(&alpha);
        g_spanActiveTexAlphaMap = selectedImage->alphaMap;
        g_pfnSelectedSpanOp_Mode0 = g_pfnPolyTlvSpanOp_Mode0;
        g_pfnSelectedSpanOp_Mode1 = g_pfnPolyTlvSpanOpAlt_Mode0;
    } else {
        alphaBits = (double)(alpha * 255.0f) - -6755399441055744.0;
        g_spanActiveConstAlphaBits = *(int*)(&alphaBits);
        g_spanActiveTexAlphaMap = 0;
        g_pfnSelectedSpanOp_Mode0 = g_pfnPolyTlvSpanOp_Mode1;
        g_pfnSelectedSpanOp_Mode1 = g_pfnPolyTlvSpanOpAlt_Mode1;
    }

    palette = (unsigned short*)(selectedImage->palette);
    if (palette == 0) {
        spanProc = g_pfnSelectedSpanOp_Mode0;
    } else {
        g_spanActiveTexPalette = texKey == -1 ? palette : &palette[(texKey + 1) * 0x100];
        spanProc = g_pfnSelectedSpanOp_Mode1;
    }
    g_spanActiveTexShift = selectedImage->uShiftFrom20;
    g_spanActiveTexUMask = selectedImage->uMask;
    g_spanActiveTexVMask = selectedImage->vMaskFixed20;

    for (y = firstScanline; y <= lastScanline; ++y) {
        while (y >= edgeTableA[edgeIndexA].yStart && edgeIndexA < edgeCountA) {
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            ++edgeIndexA;
        }

        while (y >= edgeTableB[edgeIndexB].yStart && edgeIndexB < edgeCountB) {
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            ++edgeIndexB;
        }

        if (currentXFixedA <= currentXFixedB) {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            planeY = ((float)(y) + 0.5f) - triVerts[0].y;
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            rowRecipZ = planeY * recipZGrad.y + triVerts[0].z;
            g_spanAllocCursor->invDepth
                = ((((float)(xMin) + 0.5f) - triVerts[0].x) * recipZGrad.x + rowRecipZ) * g_inverseDepthScale
                + g_inverseDepthBias;
            g_spanAllocCursor->invDepthStep
                = ((((float)(xMax) + 0.5f) - triVerts[0].x) * recipZGrad.x + rowRecipZ) * g_inverseDepthScale
                + g_inverseDepthBias;
            g_spanAllocCursor->depthSlope = recipZGrad.x;
            g_pfnBuildSpanListSecondary(spanList, y, &spanCount);
            if (spanCount != 0) {
                rowU = planeY * uGrad.y + scaledUV[0].x;
                rowV = planeY * vGrad.y + scaledUV[0].y;
                for (spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    span = spanList[spanIndex];
                    count = span->sampleXMax - span->sampleXMin + 1;
                    if (count > 0) {
                        g_spanCurrentSpanBaseAddr
                            = (unsigned short*)(scanlineBase + span->sampleXMin * g_bytesPerPixel);
                        startX = ((float)(span->sampleXMin) + 0.5f) - triVerts[0].x;
                        endX = ((float)(span->sampleXMax) + 0.5f) - triVerts[0].x;
                        startInvZ = 1.0f / (startX * recipZGrad.x + rowRecipZ);
                        startU = (startX * uGrad.x + rowU) * startInvZ;
                        startV = (startX * vGrad.x + rowV) * startInvZ;
                        texUStartBits = 6755399441055744.0 - (double)(startU * -1048576.0f);
                        texVStartBits = 6755399441055744.0 - (double)(startV * -1048576.0f);
                        endInvZ = 1.0f / (endX * recipZGrad.x + rowRecipZ);
                        endU = (endX * uGrad.x + rowU) * endInvZ;
                        endV = (endX * vGrad.x + rowV) * endInvZ;
                        texUStepBits = 6755399441055744.0 - (double)((endU - startU) * -1048576.0f);
                        texVStepBits = 6755399441055744.0 - (double)((endV - startV) * -1048576.0f);
                        // Retail divides the fixed-point delta by the pixel count (idiv).
                        g_spanActiveTexUStepFixed20 = *(int*)(&texUStepBits) / count;
                        g_spanActiveTexVStepFixed20 = *(int*)(&texVStepBits) / count;
                        spanProc(*(int*)(&texUStartBits), *(int*)(&texVStartBits), count, g_spanActiveTexShift);
                    }
                }
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawtexturedqueued
 * @recoil-artifact defines .text recoil:function:0x495850: zRndrDrawTexturedQueued
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.poly.plane-gradient.textured-queued recoil:function:0x495850
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.poly.plane-gradient.textured-queued
 *
 *
 * Purpose: Draw a depth-sorted textured polygon using perspective-correct queued spans.
 * Raw assembly: one in-body plane-gradient island at retail [0x495b67,0x495be6) over the
 * per-triangle shade plane, the hand-written x87 plane solve with ftst/wait (d9 e4 occurs only
 * three times in retail .text: this island and its copies in 0x492000 and 0x492f00).
 * Under the governed VC5SP3 C profile, the retained current-source and same-body native-C controls
 * do not reproduce this retail sequence; their determinant tests use pooled-zero comparisons. The
 * three retail copies and their surrounding operand captures support this address-scoped assembly
 * reconstruction.
 * Island contract:
 * Purpose: Compute this consumer's two-component shade gradient using the retail x87 evaluation
 * sequence.
 * Inputs: projectedVerts, shadeVertex1 and shadeVertex2 (&projectedVerts[0..2]) supply the three
 * vertices' x/y coordinates; shade0, shade1 and shade2, scalar copies of shadeTriplet->x/y/z,
 * supply the three shade values. shadeGradientOut addresses the consumer's local two-component
 * result shadeGradient. The island reads every operand from its compiler-assigned EBP-relative
 * operand home; here several captured values and the output pointer use reused argument slots.
 * Effects: Clobbers EAX, EBX, ECX and integer arithmetic flags. The block does not modify EDX,
 * ESI, EDI, EBP or ESP. The compiler owns register preservation, operand homes, the frame and
 * surrounding C.
 * x87: Requires an empty register stack; maximum live depth is eight; returns an empty stack on
 * normal completion of either path. The control word is unchanged; status and exception state
 * are not preserved.
 * Branch: Uses C3 from FTST, not a portable C != predicate. The flat arm stores the current
 * determinant to both binary32 output components, then discards the four remaining coordinate
 * deltas. The other arm stores the two computed gradient components. Preserve the exact
 * instruction order.
 * Fallback: the native-C branch is a nonmatching algorithmic fallback. It implements the same
 * plane-gradient equations but is not an unconditional bitwise or floating-point-environment
 * equivalent of the island: intermediate rounding, exceptional values and exception timing may
 * differ. Its shadeDeterminant != 0.0f test is not a portable spelling of the island's
 * C3-only decision.
 * Behaviour: as in retail, the body has no defensive early returns or substitute values: no
 * argument, vertex-count or global null check, no empty edge-table or empty row-range return, no
 * null mip-image return, no zero-widthScale substitute, no minimum-one span-width clamp and no
 * null or empty span skip. Retail's own null- and zero-dependent selections remain, including the
 * entry-to-image ternary, the clippedTriVerts test, the mip-variant condition, the palette test
 * and the zero inverse-depth-gradient span-width choice. The missing guards do not make malformed
 * inputs safe.
 */
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
)
{
    ScanConvertEdge edgeTableA[0x40];
    ScanConvertEdge edgeTableB[0x40];
    SpanNodePartial* spanList[0x141];
    zVidImagePartial* image;
    float imageWidth;
    float imageHeight;
    zVec2 delta10;
    zVec2 delta12;
    float determinant;
    float inverseDeterminant;
    float dz10;
    float dz12;
    float du10;
    float du12;
    float dv10;
    float dv12;
    zVec2 shadeGradient;
    zVec2* shadeGradientOut;
    float shade0;
    float shade1;
    float shade2;
    const zVec3* shadeVertex1;
    const zVec3* shadeVertex2;
    float textureScale;
    float minPositiveZ;
    float vertexZ;
    int vertexIndex;
    int topVertexIndex;
    int bottomVertexIndex;
    int edgeCountA;
    int edgeCountB;
    int edgeVertexIndex;
    int fixed16Value;
    int edgeYStart;
    float edgeSampleY;
    ScanConvertEdge* edge;
    int lastScanline;
    unsigned char* scanlineBase;
    int chunkPixels;
    int chunkBytes;
    float chunkWidth;
    float chunkInverseWidth;
    float chunkShadeScale;
    float chunkScale;
    float chunkZStep;
    float chunkUStep;
    float chunkVStep;
    float chunkShadeStep;
    TexturedQueuedSpanProc spanProc;
    int shadeRecipe;
    int edgeIndexA;
    int edgeIndexB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    int y;
    int xMin;
    int xMax;
    float sampleY;
    float planeY;
    float shadeY;
    float rowRecipZ;
    float rowStartX;
    float rowEndX;
    float rowU;
    float rowV;
    float rowShade;
    int spanCount;
    int spanIndex;
    SpanNodePartial* span;
    int remaining;
    float startX;
    float endX;
    float planeX;
    float z;
    float uz;
    float vz;
    float shade;
    float invZ;
    float startU;
    float startV;
    float endU;
    float endV;
    float startShade;
    float endShade;
    float countInverse;
    float countShadeScale;
    float countScale;
    double fixedBits;
    double uStartBits;
    double vStartBits;
    double stepBits;

    // Fan triangles after the first reuse the texture planes built for triangle 0 (retail tests the index first).
    if (fanTriIndex == 0) {
        image = entry != 0 ? entry->image : 0;
        imageWidth = (float)(image->width);
        imageHeight = (float)(image->height);
        gRndr_PerspTexScaledUOverZ0 = triUVs[0].x * (imageWidth * triVerts[0].z);
        gRndr_PerspTexScaledVOverZ0 = triUVs[0].y * (imageHeight * triVerts[0].z);
        gRndr_PerspTexScaledUOverZ1 = triUVs[1].x * (imageWidth * triVerts[1].z);
        gRndr_PerspTexScaledVOverZ1 = triUVs[1].y * (imageHeight * triVerts[1].z);
        gRndr_PerspTexScaledUOverZ2 = triUVs[2].x * (imageWidth * triVerts[2].z);
        gRndr_PerspTexScaledVOverZ2 = triUVs[2].y * (imageHeight * triVerts[2].z);
        if (clippedTriVerts != 0
            && (clippedTriVerts[0].z < 10.0f || clippedTriVerts[1].z < 10.0f || clippedTriVerts[2].z < 10.0f)) {
            zMathBuildPerspectiveTextureInterpolants(
                clippedTriVerts,
                triUVs,
                (zVec2*)(&gRndr_PerspInvDepthStepX),
                &gRndr_PerspInvDepthBase,
                (zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
                &gRndr_PerspTexScaledUOverZBase,
                (zVec2*)(&gRndr_PerspTexScaledVOverZStepX),
                &gRndr_PerspTexScaledVOverZBase
            );
            gRndr_PerspTexScaledUOverZStepX *= imageWidth;
            gRndr_PerspPlaneOriginX = g_zMath_ProjOffsetX;
            gRndr_PerspPlaneOriginY = g_zMath_ProjOffsetY;
            gRndr_PerspTexScaledUOverZStepY *= imageWidth;
            gRndr_PerspTexScaledUOverZBase *= imageWidth;
            gRndr_PerspTexScaledVOverZStepX *= imageHeight;
            gRndr_PerspTexScaledVOverZStepY *= imageHeight;
            gRndr_PerspTexScaledVOverZBase *= imageHeight;
        } else {
            delta10.x = triVerts[0].x - triVerts[1].x;
            delta10.y = triVerts[0].y - triVerts[1].y;
            delta12.x = triVerts[2].x - triVerts[1].x;
            delta12.y = triVerts[2].y - triVerts[1].y;
            determinant = delta12.y * delta10.x - delta12.x * delta10.y;
            if (determinant != 0.0) {
                inverseDeterminant = -1.0f / determinant;
                dz10 = triVerts[0].z - triVerts[1].z;
                dz12 = triVerts[2].z - triVerts[1].z;
                du10 = gRndr_PerspTexScaledUOverZ0 - gRndr_PerspTexScaledUOverZ1;
                du12 = gRndr_PerspTexScaledUOverZ2 - gRndr_PerspTexScaledUOverZ1;
                dv10 = gRndr_PerspTexScaledVOverZ0 - gRndr_PerspTexScaledVOverZ1;
                dv12 = gRndr_PerspTexScaledVOverZ2 - gRndr_PerspTexScaledVOverZ1;
                gRndr_PerspInvDepthStepX = (dz12 * delta10.y - dz10 * delta12.y) * inverseDeterminant;
                gRndr_PerspInvDepthStepY = (dz10 * delta12.x - dz12 * delta10.x) * inverseDeterminant;
                gRndr_PerspTexScaledUOverZStepX = (du12 * delta10.y - du10 * delta12.y) * inverseDeterminant;
                gRndr_PerspTexScaledUOverZStepY = (du10 * delta12.x - du12 * delta10.x) * inverseDeterminant;
                gRndr_PerspTexScaledVOverZStepX = (dv12 * delta10.y - dv10 * delta12.y) * inverseDeterminant;
                gRndr_PerspTexScaledVOverZStepY = (dv10 * delta12.x - dv12 * delta10.x) * inverseDeterminant;
            } else {
                gRndr_PerspInvDepthStepX = gRndr_PerspInvDepthStepY = gRndr_PerspTexScaledUOverZStepX
                    = gRndr_PerspTexScaledUOverZStepY = gRndr_PerspTexScaledVOverZStepX
                    = gRndr_PerspTexScaledVOverZStepY = 0.0f;
            }
            gRndr_PerspInvDepthBase = triVerts[0].z;
            gRndr_PerspTexScaledUOverZBase = gRndr_PerspTexScaledUOverZ0;
            gRndr_PerspTexScaledVOverZBase = gRndr_PerspTexScaledVOverZ0;
            gRndr_PerspPlaneOriginX = triVerts[0].x;
            gRndr_PerspPlaneOriginY = triVerts[0].y;
        }
    }

    // Per-triangle shade plane over the projected vertices (retail 0x495b67-0x495be6).
    shadeGradientOut = &shadeGradient;
    shade2 = shadeTriplet->z;
    shade1 = shadeTriplet->y;
    shade0 = shadeTriplet->x;
    shadeVertex2 = &projectedVerts[2];
    shadeVertex1 = &projectedVerts[1];
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, projectedVerts
        mov ebx, shadeVertex1
        mov ecx, shadeVertex2
        fld dword ptr [ebx]zVec3.x
        fld st(0)
        fsubr dword ptr [eax]zVec3.x
        fxch st(1)
        fsubr dword ptr [ecx]zVec3.x
        fld dword ptr [ebx]zVec3.y
        fld st(0)
        fsubr dword ptr [eax]zVec3.y
        fxch st(1)
        fsubr dword ptr [ecx]zVec3.y
        fld st(1)
        fmul st, st(3)
        fld st(1)
        fmul st, st(5)
        fsubrp st(1), st
        ftst
        wait
        fnstsw ax
        test ah, 40h
        jne recoil_plane_gradient_flat
        fld shade1
        fld st(0)
        fsubr shade0
        fxch st(1)
        fsubr shade2
        fxch st(1)
        fxch st(2)
        fld1
        fdivrp st(1), st
        fld st(1)
        fmulp st(5), st
        fld st(2)
        fmulp st(4), st
        fxch st(4)
        fsubp st(3), st
        fld st(3)
        fmulp st(3), st
        fmulp st(5), st
        fmulp st(3), st
        fxch st(3)
        fsubrp st(2), st
        fmulp st(1), st
        fxch st(1)
        mov eax, shadeGradientOut
        fstp dword ptr [eax]zVec2.x
        fstp dword ptr [eax]zVec2.y
        jmp recoil_plane_gradient_done
    recoil_plane_gradient_flat:
        mov eax, shadeGradientOut
        fst dword ptr [eax]zVec2.x
        fstp dword ptr [eax]zVec2.y
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
    recoil_plane_gradient_done:
    }
#else
    {
        const float shadeDx10 = projectedVerts->x - shadeVertex1->x;
        const float shadeDx12 = shadeVertex2->x - shadeVertex1->x;
        const float shadeDy10 = projectedVerts->y - shadeVertex1->y;
        const float shadeDy12 = shadeVertex2->y - shadeVertex1->y;
        const float shadeDeterminant = shadeDy12 * shadeDx10 - shadeDy10 * shadeDx12;
        shadeGradientOut->x = shadeDeterminant;
        shadeGradientOut->y = shadeDeterminant;
        if (shadeDeterminant != 0.0f) {
            const float shade10 = shade0 - shade1;
            const float shade12 = shade2 - shade1;
            const float inverseShadeDeterminant = 1.0f / shadeDeterminant;
            shadeGradientOut->x = (shadeDy12 * shade10 - shadeDy10 * shade12) * inverseShadeDeterminant;
            shadeGradientOut->y = (shadeDx10 * shade12 - shadeDx12 * shade10) * inverseShadeDeterminant;
        }
    }

#endif

    if (entry != 0 && entry->nextVariant != 0)
    {
        image = zRndrTextureMipSelectVariantImage(
            entry,
            triVerts,
            3,
            (zVec2*)(&gRndr_PerspTexScaledUOverZ0),
            (zVec2*)(&gRndr_PerspInvDepthStepX),
            (zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
            (zVec2*)(&gRndr_PerspTexScaledVOverZStepX)
        );
        textureScale = 1048576.0f / image->widthScale;
    }
    else
    {
        image = entry != 0 ? entry->image : 0;
        textureScale = 1048576.0f;
    }

    minPositiveZ = 1000.0f;
    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (vertexIndex = 0; vertexIndex < vertCount; ++vertexIndex) {
        vertexZ = (projectedVerts[vertexIndex].x - gRndr_PerspPlaneOriginX) * gRndr_PerspInvDepthStepX;
        vertexZ += (projectedVerts[vertexIndex].y - gRndr_PerspPlaneOriginY) * gRndr_PerspInvDepthStepY
            + gRndr_PerspInvDepthBase;
        if (vertexZ > 0.0f && vertexZ < minPositiveZ) {
            minPositiveZ = vertexZ;
        }
    }
    for (vertexIndex = 1; vertexIndex < vertCount; ++vertexIndex) {
        if (projectedVerts[vertexIndex].y < projectedVerts[topVertexIndex].y) {
            topVertexIndex = vertexIndex;
        }
        // Retail keeps the first lowest vertex (strict compare).
        if (projectedVerts[vertexIndex].y > projectedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = vertexIndex;
        }
    }

    edgeCountA = 0;
    edgeCountB = 0;
    // Each walk converts its slope, x intercept and next row through one walk-local double, a source
    // model of retail's observed conversion-slot reuse rather than recovered lexical scope.
    if (g_scanConvertMode != 0) {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                const zVec3* end = &projectedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(projectedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                const zVec3* end = &projectedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(projectedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    } else {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                const zVec3* end = &projectedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(projectedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                double fixedBits;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                const zVec3* end = &projectedVerts[nextIndex];
                const float dy = end->y - start->y;
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (end->x - start->x) / dy;
                    fixedBits = 6755399441055744.0 - (double)(xSlope * -65536.0f);
                    edge->xStepFixed = *(int*)(&fixedBits);
                    fixedBits = 6755399441055744.0
                        - (double)((start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope) * -65536.0f);
                    edge->currentXFixed = *(int*)(&fixedBits);
                }
                fixedBits = 6755399441055744.0 - (double)(projectedVerts[nextIndex].y * -65536.0f);
                fixed16Value = *(int*)(&fixedBits);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
    // Retail converts the first row straight into the row counter.
    y = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[bottomVertexIndex].y);
    lastScanline = (fixed16Value - 0x8041) >> 16;

    if (g_perspectiveAdaptiveMinSpan != 0) {
        if (gRndr_PerspInvDepthStepX == 0.0f) {
            chunkPixels = g_perspectiveAdaptiveMaxSpan;
        } else {
            fixedBits
                = fabs(minPositiveZ * g_perspectiveAdaptiveSlope / gRndr_PerspInvDepthStepX) - -6755399441055744.0;
            chunkPixels = *(int*)(&fixedBits);
            if (chunkPixels > g_perspectiveAdaptiveMaxSpan) {
                chunkPixels = g_perspectiveAdaptiveMaxSpan;
            } else if (chunkPixels < g_perspectiveAdaptiveMinSpan) {
                chunkPixels = g_perspectiveAdaptiveMinSpan;
            }
        }
        chunkWidth = (float)(chunkPixels);
        chunkBytes = chunkPixels * g_bytesPerPixel;
    } else {
        chunkPixels = g_perspectiveTextureDeltaXPow2;
        chunkBytes = g_perspectiveTextureDeltaXBytes;
        chunkWidth = g_perspectiveTextureDeltaXPow2F;
    }
    chunkInverseWidth = 1.0f / chunkWidth;
    edgeIndexA = 0;
    edgeIndexB = 0;
    chunkShadeScale = chunkInverseWidth * 65536.0f;
    chunkScale = chunkInverseWidth * textureScale;
    scanlineBase = (unsigned char*)(g_frameBuffer) + y * g_pitchBytes;
    chunkZStep = chunkWidth * gRndr_PerspInvDepthStepX;
    g_spanActiveTexPixels = (unsigned char*)(image->pixels);
    g_spanActiveTexShift = image->uShiftFrom20;
    g_spanActiveTexUMask = image->uMask;
    g_spanActiveTexVMask = image->vMaskFixed20;
    chunkUStep = chunkWidth * gRndr_PerspTexScaledUOverZStepX;
    chunkVStep = chunkWidth * gRndr_PerspTexScaledVOverZStepX;
    chunkShadeStep = chunkWidth * shadeGradient.x;
    if (image->palette == 0) {
        spanProc = g_pfnTexturedQueuedSpanOp_Mode0;
    } else if (texKey == -1) {
        shadeRecipe = g_zRndr_ActivePaletteShadeRecipeIndex;
        if (shadeRecipe < 0) {
            shadeRecipe = zVidPaletteRemapFindRecipeIndexFromRgb((zColorRgb*)(g_fogParamsActive.colorRgb01));
        }
        if (shadeRecipe < 0) {
            spanProc = g_pfnTexturedQueuedSpanOp_Mode1;
            g_spanActiveTexPalette = (unsigned short*)(image->palette);
        } else {
            spanProc = SpanShade16FromPal8SwitchVShift;
            g_spanActiveTexPalette = &((unsigned short*)(image->palette))[0x100 + shadeRecipe * 0x2000];
        }
    } else {
        spanProc = g_pfnTexturedQueuedSpanOp_Mode1;
        g_spanActiveTexPalette = &((unsigned short*)(image->palette))[(texKey + 1) * 0x100];
    }

    currentXFixedA = edgeTableA[0].currentXFixed;
    currentXFixedB = edgeTableB[0].currentXFixed;
    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    for (; y <= lastScanline; ++y) {
        while (y >= edgeTableA[edgeIndexA].yStart && edgeIndexA < edgeCountA) {
            currentXFixedA = edgeTableA[edgeIndexA].currentXFixed;
            xStepFixedA = edgeTableA[edgeIndexA].xStepFixed;
            ++edgeIndexA;
        }

        while (y >= edgeTableB[edgeIndexB].yStart && edgeIndexB < edgeCountB) {
            currentXFixedB = edgeTableB[edgeIndexB].currentXFixed;
            xStepFixedB = edgeTableB[edgeIndexB].xStepFixed;
            ++edgeIndexB;
        }

        if (currentXFixedA <= currentXFixedB) {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            sampleY = (float)(y) + 0.5f;
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            planeY = sampleY - gRndr_PerspPlaneOriginY;
            shadeY = sampleY - projectedVerts[0].y;
            rowRecipZ = planeY * gRndr_PerspInvDepthStepY + gRndr_PerspInvDepthBase;
            // Retail forms both plane-space x extents before the two depth products.
            rowStartX = ((float)(xMin) + 0.5f) - gRndr_PerspPlaneOriginX;
            rowEndX = ((float)(xMax) + 0.5f) - gRndr_PerspPlaneOriginX;
            g_spanAllocCursor->invDepth
                = (rowStartX * gRndr_PerspInvDepthStepX + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->invDepthStep
                = (rowEndX * gRndr_PerspInvDepthStepX + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->depthSlope = gRndr_PerspInvDepthStepX;
            g_pfnBuildSpanList(spanList, y, &spanCount);
            if (spanCount != 0) {
                rowU = planeY * gRndr_PerspTexScaledUOverZStepY + gRndr_PerspTexScaledUOverZBase;
                rowV = planeY * gRndr_PerspTexScaledVOverZStepY + gRndr_PerspTexScaledVOverZBase;
                rowShade = shadeY * shadeGradient.y + shadeTriplet->x;
                for (spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    span = spanList[spanIndex];
                    startX = (float)(span->sampleXMin) + 0.5f;
                    endX = (float)(span->sampleXMax) + 0.5f;
                    remaining = span->sampleXMax - span->sampleXMin + 1;
                    planeX = startX - gRndr_PerspPlaneOriginX;
                    g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + span->sampleXMin * g_bytesPerPixel);
                    z = planeX * gRndr_PerspInvDepthStepX + rowRecipZ;
                    uz = planeX * gRndr_PerspTexScaledUOverZStepX + rowU;
                    vz = planeX * gRndr_PerspTexScaledVOverZStepX + rowV;
                    shade = (startX - projectedVerts[0].x) * shadeGradient.x + rowShade;
                    invZ = 1.0f / z;
                    startU = invZ * uz;
                    startV = invZ * vz;
                    startShade = shade;
                    if (shade > 255.0f) {
                        startShade = 255.0f;
                    } else if (shade < 0.0f) {
                        startShade = 0.0f;
                    }
                    // Each chunk is handed to the span routine from its far end, stepping back toward its start.
                    while (remaining > chunkPixels) {
                        z += chunkZStep;
                        uz += chunkUStep;
                        vz += chunkVStep;
                        shade += chunkShadeStep;
                        invZ = 1.0f / z;
                        endShade = shade;
                        endU = invZ * uz;
                        endV = invZ * vz;
                        if (shade > 255.0f) {
                            endShade = 255.0f;
                        } else if (shade < 0.0f) {
                            endShade = 0.0f;
                        }
                        uStartBits = (double)(endU * textureScale) - -6755399441055744.0;
                        vStartBits = (double)(endV * textureScale) - -6755399441055744.0;
                        stepBits = (double)((startU - endU) * chunkScale) - -6755399441055744.0;
                        g_spanActiveTexUStepFixed20 = *(int*)(&stepBits);
                        stepBits = (double)((startV - endV) * chunkScale) - -6755399441055744.0;
                        g_spanActiveTexVStepFixed20 = *(int*)(&stepBits);
                        if (spanProc != SpanShade16FromPal8SwitchVShift) {
                            ZRNDR_SET_FIXED16_FROM_FLOAT(g_spanActiveShadeFixed16, startShade);
                            stepBits = (double)((endShade - startShade) * chunkShadeScale) - -6755399441055744.0;
                            g_spanActiveShadeStepFixed16 = *(int*)(&stepBits);
                            spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), chunkPixels, g_spanActiveTexShift);
                            ((void(__fastcall*)(unsigned short*, int, int, int))(g_pfnTexturedQueuedFinalize))(
                                g_spanCurrentSpanBaseAddr,
                                chunkPixels,
                                g_spanActiveShadeFixed16,
                                g_spanActiveShadeStepFixed16
                            );
                        } else {
                            ZRNDR_SET_FIXED16_FROM_FLOAT(g_spanActiveShadeFixed16, endShade);
                            stepBits = (double)((startShade - endShade) * chunkShadeScale) - -6755399441055744.0;
                            g_spanActiveShadeStepFixed16 = *(int*)(&stepBits);
                            spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), chunkPixels, g_spanActiveTexShift);
                        }
                        g_spanCurrentSpanBaseAddr
                            = (unsigned short*)((unsigned char*)(g_spanCurrentSpanBaseAddr) + chunkBytes);
                        startShade = endShade;
                        remaining -= chunkPixels;
                        startU = endU;
                        startV = endV;
                    }

                    countInverse = 1.0f / (float)(remaining);
                    countShadeScale = countInverse * 65536.0f;
                    countScale = countInverse * textureScale;
                    planeX = endX - gRndr_PerspPlaneOriginX;
                    shade = (endX - projectedVerts[0].x) * shadeGradient.x + rowShade;
                    invZ = 1.0f / (planeX * gRndr_PerspInvDepthStepX + rowRecipZ);
                    endU = (planeX * gRndr_PerspTexScaledUOverZStepX + rowU) * invZ;
                    endV = (planeX * gRndr_PerspTexScaledVOverZStepX + rowV) * invZ;
                    endShade = shade;
                    if (shade > 255.0f) {
                        endShade = 255.0f;
                    } else if (shade < 0.0f) {
                        endShade = 0.0f;
                    }
                    uStartBits = (double)(endU * textureScale) - -6755399441055744.0;
                    vStartBits = (double)(endV * textureScale) - -6755399441055744.0;
                    stepBits = (double)((startU - endU) * countScale) - -6755399441055744.0;
                    g_spanActiveTexUStepFixed20 = *(int*)(&stepBits);
                    stepBits = (double)((startV - endV) * countScale) - -6755399441055744.0;
                    g_spanActiveTexVStepFixed20 = *(int*)(&stepBits);
                    if (spanProc != SpanShade16FromPal8SwitchVShift) {
                        ZRNDR_SET_FIXED16_FROM_FLOAT(g_spanActiveShadeFixed16, startShade);
                        stepBits = (double)((endShade - startShade) * countShadeScale) - -6755399441055744.0;
                        g_spanActiveShadeStepFixed16 = *(int*)(&stepBits);
                        spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), remaining, g_spanActiveTexShift);
                        ((void(__fastcall*)(unsigned short*, int, int, int))(g_pfnTexturedQueuedFinalize))(
                            g_spanCurrentSpanBaseAddr,
                            remaining,
                            g_spanActiveShadeFixed16,
                            g_spanActiveShadeStepFixed16
                        );
                    } else {
                        ZRNDR_SET_FIXED16_FROM_FLOAT(g_spanActiveShadeFixed16, endShade);
                        stepBits = (double)((startShade - endShade) * countShadeScale) - -6755399441055744.0;
                        g_spanActiveShadeStepFixed16 = *(int*)(&stepBits);
                        spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), remaining, g_spanActiveTexShift);
                    }
                }
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawtexturedqueuedalpha
 * @recoil-artifact defines .text recoil:function:0x4969d0: zRndrDrawTexturedQueuedAlpha
 *
 *
 * Purpose: Queue an alpha-blended textured polygon for deferred depth-sorted rendering.
 */
void __fastcall zRndrDrawTexturedQueuedAlpha(
    zImage_TexDirEntryPartial* entry,
    zVec3* projectedVerts,
    zVec3* clippedTriVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertCount,
    int variantIndex
)
{
    ScanConvertEdge edgeTableB[0x40];
    ScanConvertEdge edgeTableA[0x40];
    SpanNodePartial* spanList[0x141];
    zVidImagePartial* image;
    float imageWidth;
    float imageHeight;
    zVec2 scaledUV[3];
    zVec2 recipZGrad;
    zVec2 uGrad;
    zVec2 vGrad;
    float recipZBase;
    float uBase;
    float vBase;
    float originX;
    float originY;
    float dx01;
    float dx21;
    float dy01;
    float dy21;
    float determinant;
    float inverseDeterminant;
    float dz01;
    float dz21;
    float du01;
    float du21;
    float dv01;
    float dv21;
    float textureScale;
    float minPositiveZ;
    float vertexZ;
    float adjustX;
    float adjustY;
    int vertexIndex;
    int topVertexIndex;
    int bottomVertexIndex;
    int edgeCountA;
    int edgeCountB;
    int edgeVertexIndex;
    int fixed16Value;
    int edgeYStart;
    float edgeSampleY;
    ScanConvertEdge* edge;
    int firstScanline;
    int lastScanline;
    unsigned char* scanlineBase;
    int chunkPixels;
    int chunkBytes;
    float chunkWidth;
    double chunkBits;
    float chunkScale;
    float chunkZStep;
    float chunkUStep;
    float chunkVStep;
    TexturedQueuedSpanProc spanProc;
    unsigned short* palette;
    int* nextEdgeA;
    int* nextEdgeB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    int endScanline;
    int nextEventScanline;
    int y;
    int xMin;
    int xMax;
    float sampleY;
    float rowRecipZ;
    float rowU;
    float rowV;
    int spanCount;
    int spanIndex;
    SpanNodePartial* span;
    int remaining;
    float startX;
    float z;
    float invZ;
    float uz;
    float vz;
    float startU;
    float startV;
    float endU;
    float endV;
    float endX;
    float countF;
    float endZ;
    float endScale;
    double uStartBits;
    double vStartBits;
    double uStepBits;
    double vStepBits;

    image = entry != 0 ? entry->image : 0;
    imageWidth = (float)(image->width);
    imageHeight = (float)(image->height);
    scaledUV[0].x = imageWidth * triVerts[0].z * triUVs[0].x;
    scaledUV[0].y = imageHeight * triVerts[0].z * triUVs[0].y;
    scaledUV[1].x = imageWidth * triVerts[1].z * triUVs[1].x;
    scaledUV[1].y = imageHeight * triVerts[1].z * triUVs[1].y;
    scaledUV[2].x = imageWidth * triVerts[2].z * triUVs[2].x;
    scaledUV[2].y = imageHeight * triVerts[2].z * triUVs[2].y;
    if (clippedTriVerts != 0
        && (clippedTriVerts[0].z < 10.0f || clippedTriVerts[1].z < 10.0f || clippedTriVerts[2].z < 10.0f)) {
        zMathBuildPerspectiveTextureInterpolants(
            clippedTriVerts,
            triUVs,
            &recipZGrad,
            &recipZBase,
            &uGrad,
            &uBase,
            &vGrad,
            &vBase
        );
        uGrad.x *= imageWidth;
        uGrad.y *= imageWidth;
        uBase *= imageWidth;
        vGrad.x *= imageHeight;
        vGrad.y *= imageHeight;
        vBase *= imageHeight;
        originX = g_zMath_ProjOffsetX;
        originY = g_zMath_ProjOffsetY;
    } else {
        dx21 = triVerts[2].x - triVerts[1].x;
        dy01 = triVerts[0].y - triVerts[1].y;
        dx01 = triVerts[0].x - triVerts[1].x;
        dy21 = triVerts[2].y - triVerts[1].y;
        determinant = dy21 * dx01 - dx21 * dy01;
        if (determinant != 0.0) {
            inverseDeterminant = -1.0f / determinant;
            dz01 = triVerts[0].z - triVerts[1].z;
            dz21 = triVerts[2].z - triVerts[1].z;
            du01 = scaledUV[0].x - scaledUV[1].x;
            du21 = scaledUV[2].x - scaledUV[1].x;
            dv01 = scaledUV[0].y - scaledUV[1].y;
            dv21 = scaledUV[2].y - scaledUV[1].y;
            recipZGrad.x = (dz21 * dy01 - dz01 * dy21) * inverseDeterminant;
            recipZGrad.y = (dz01 * dx21 - dz21 * dx01) * inverseDeterminant;
            uGrad.x = (du21 * dy01 - du01 * dy21) * inverseDeterminant;
            uGrad.y = (du01 * dx21 - du21 * dx01) * inverseDeterminant;
            vGrad.x = (dv21 * dy01 - dv01 * dy21) * inverseDeterminant;
            vGrad.y = (dv01 * dx21 - dv21 * dx01) * inverseDeterminant;
        } else {
            recipZGrad.x = recipZGrad.y = uGrad.x = uGrad.y = vGrad.x = vGrad.y = 0.0f;
        }
        recipZBase = triVerts[0].z;
        uBase = scaledUV[0].x;
        vBase = scaledUV[0].y;
        originX = triVerts[0].x;
        originY = triVerts[0].y;
    }

    if (entry != 0 && entry->nextVariant != 0) {
        image = zRndrTextureMipSelectVariantImage(entry, triVerts, 3, scaledUV, &recipZGrad, &uGrad, &vGrad);
        textureScale = 1048576.0f / image->widthScale;
    } else {
        textureScale = 1048576.0f;
    }

    // Retail estimates the nearest positive depth from the x term of the reciprocal-Z plane only.
    minPositiveZ = 1000.0f;
    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (vertexIndex = 0; vertexIndex < vertCount; ++vertexIndex) {
        vertexZ = (projectedVerts[vertexIndex].x - originX) * recipZGrad.x + recipZBase;
        if (vertexZ > 0.0f && vertexZ < minPositiveZ) {
            minPositiveZ = vertexZ;
        }
    }
    for (vertexIndex = 1; vertexIndex < vertCount; ++vertexIndex) {
        if (projectedVerts[vertexIndex].y < projectedVerts[topVertexIndex].y) {
            topVertexIndex = vertexIndex;
        }
        // Retail keeps the first lowest vertex (strict compare).
        if (projectedVerts[vertexIndex].y > projectedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = vertexIndex;
        }
    }

    adjustX = originX - 0.5f;
    adjustY = originY - 0.5f;
    recipZBase -= adjustX * recipZGrad.x + adjustY * recipZGrad.y;
    uBase -= adjustX * uGrad.x + adjustY * uGrad.y;
    vBase -= adjustX * vGrad.x + adjustY * vGrad.y;

    edgeCountA = 0;
    edgeCountB = 0;
    if (g_scanConvertMode != 0) {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    } else {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    }

    // Retail terminates both edge tables with a far-below sentinel start row.
    edgeTableA[edgeCountA].yStart = 9999;
    edgeTableB[edgeCountB].yStart = 9999;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
    firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[bottomVertexIndex].y);
    lastScanline = (fixed16Value - 0x8041) >> 16;
    scanlineBase = (unsigned char*)(g_frameBuffer) + firstScanline * g_pitchBytes;

    if (g_perspectiveAdaptiveMinSpan != 0) {
        if (recipZGrad.x == 0.0f) {
            chunkPixels = g_perspectiveAdaptiveMaxSpan;
        } else {
            chunkBits = fabs(minPositiveZ * g_perspectiveAdaptiveSlope / recipZGrad.x) - -6755399441055744.0;
            chunkPixels = *(int*)(&chunkBits);
            if (chunkPixels > g_perspectiveAdaptiveMaxSpan) {
                chunkPixels = g_perspectiveAdaptiveMaxSpan;
            } else if (chunkPixels < g_perspectiveAdaptiveMinSpan) {
                chunkPixels = g_perspectiveAdaptiveMinSpan;
            }
        }
        chunkBytes = chunkPixels * g_bytesPerPixel;
        chunkWidth = (float)(chunkPixels);
    } else {
        chunkBytes = g_perspectiveTextureDeltaXBytes;
        chunkPixels = g_perspectiveTextureDeltaXPow2;
        chunkWidth = g_perspectiveTextureDeltaXPow2F;
    }
    chunkScale = textureScale / chunkWidth;

    g_spanActiveTexPixels = (unsigned char*)(image->pixels);
    g_spanQueuedTexAlphaMap = image->queuedAlphaMap;
    g_spanActiveTexShift = image->uShiftFrom20;
    g_spanActiveTexUMask = image->uMask;
    g_spanActiveTexVMask = image->vMaskFixed20;
    chunkZStep = chunkWidth * recipZGrad.x;
    chunkUStep = chunkWidth * uGrad.x;
    chunkVStep = chunkWidth * vGrad.x;
    palette = (unsigned short*)(image->palette);
    if (palette == 0) {
        spanProc = g_pfnTexturedQueuedSpanOp_Mode0;
    } else {
        g_spanActiveTexPalette = variantIndex == -1 ? palette : &palette[(variantIndex + 1) * 0x100];
        spanProc = g_pfnTexturedQueuedSpanOp_Mode1;
    }

    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    currentXFixedA = edgeTableA[0].currentXFixed;
    currentXFixedB = edgeTableB[0].currentXFixed;
    endScanline = lastScanline + 1;
    nextEventScanline = endScanline;
    if (edgeTableA[1].yStart < nextEventScanline) {
        nextEventScanline = edgeTableA[1].yStart;
    }
    if (edgeTableB[1].yStart < nextEventScanline) {
        nextEventScanline = edgeTableB[1].yStart;
    }
    nextEdgeB = &edgeTableB[1].yStart;
    nextEdgeA = &edgeTableA[1].yStart;

    // Retail advances the edges only at the precomputed next event row and exits there past the last row.
    for (y = firstScanline;; ++y) {
        if (y >= nextEventScanline) {
            if (y >= endScanline) {
                return;
            }
            if (y >= *nextEdgeA) {
                currentXFixedA = nextEdgeA[1];
                xStepFixedA = nextEdgeA[-1];
                nextEdgeA += 4;
            }
            if (y >= *nextEdgeB) {
                currentXFixedB = nextEdgeB[1];
                xStepFixedB = nextEdgeB[-1];
                nextEdgeB += 4;
            }
            nextEventScanline = endScanline;
            if (*nextEdgeA < nextEventScanline) {
                nextEventScanline = *nextEdgeA;
            }
            if (*nextEdgeB < nextEventScanline) {
                nextEventScanline = *nextEdgeB;
            }
        }

        if (currentXFixedA <= currentXFixedB) {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            sampleY = (float)(y);
            rowRecipZ = sampleY * recipZGrad.y + recipZBase;
            g_spanAllocCursor->invDepth
                = ((float)(xMin)*recipZGrad.x + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*recipZGrad.x + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->depthSlope = recipZGrad.x;
            g_pfnBuildSpanList(spanList, y, &spanCount);
            if (spanCount != 0) {
                rowU = sampleY * uGrad.y + uBase;
                rowV = sampleY * vGrad.y + vBase;
                for (spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    span = spanList[spanIndex];
                    g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + span->sampleXMin * g_bytesPerPixel);
                    remaining = span->sampleXMax - span->sampleXMin + 1;
                    startX = (float)(span->sampleXMin);
                    z = startX * recipZGrad.x + rowRecipZ;
                    uz = startX * uGrad.x + rowU;
                    vz = startX * vGrad.x + rowV;
                    invZ = 1.0f / z;
                    startU = invZ * uz;
                    startV = invZ * vz;
                    // Each chunk is handed to the span routine from its far end, stepping back toward its start.
                    while (remaining > chunkPixels) {
                        z += chunkZStep;
                        invZ = 1.0f / z;
                        uz += chunkUStep;
                        vz += chunkVStep;
                        endU = invZ * uz;
                        endV = invZ * vz;
                        uStartBits = (double)(endU * textureScale) - -6755399441055744.0;
                        vStartBits = (double)(endV * textureScale) - -6755399441055744.0;
                        uStepBits = (double)((startU - endU) * chunkScale) - -6755399441055744.0;
                        vStepBits = (double)((startV - endV) * chunkScale) - -6755399441055744.0;
                        g_spanActiveTexUStepFixed20 = *(int*)(&uStepBits);
                        g_spanActiveTexVStepFixed20 = *(int*)(&vStepBits);
                        spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), chunkPixels, g_spanActiveTexShift);
                        startU = endU;
                        startV = endV;
                        remaining -= chunkPixels;
                        g_spanCurrentSpanBaseAddr
                            = (unsigned short*)((unsigned char*)(g_spanCurrentSpanBaseAddr) + chunkBytes);
                    }
                    endX = (float)(span->sampleXMax);
                    countF = (float)(remaining);
                    endZ = endX * recipZGrad.x + rowRecipZ;
                    endScale = textureScale / (countF * endZ);
                    uz = endX * uGrad.x + rowU;
                    vz = endX * vGrad.x + rowV;
                    uStartBits = (double)(countF * endScale * uz) - -6755399441055744.0;
                    vStartBits = (double)(countF * endScale * vz) - -6755399441055744.0;
                    uStepBits = (double)((startU * endZ - uz) * endScale) - -6755399441055744.0;
                    vStepBits = (double)((startV * endZ - vz) * endScale) - -6755399441055744.0;
                    g_spanActiveTexUStepFixed20 = *(int*)(&uStepBits);
                    g_spanActiveTexVStepFixed20 = *(int*)(&vStepBits);
                    spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), remaining, g_spanActiveTexShift);
                }
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawtexturedfantri
 * @recoil-artifact defines .text recoil:function:0x497ac0: zRndrDrawTexturedFanTri
 *
 *
 * Purpose: Draw one textured triangle from a fan using the selected active span callback.
 */
void __fastcall zRndrDrawTexturedFanTri(
    zImage_TexDirEntryPartial* entry,
    zVec3* projectedVerts,
    zVec3* clippedTriVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    int vertCount,
    int alpha255,
    int variantIndex
)
{
    ScanConvertEdge edgeTableB[0x40];
    ScanConvertEdge edgeTableA[0x40];
    SpanNodePartial* spanList[0x141];
    zVidImagePartial* image;
    float imageWidth;
    float imageHeight;
    zVec2 scaledUV[3];
    zVec2 recipZGrad;
    zVec2 uGrad;
    zVec2 vGrad;
    float recipZBase;
    float uBase;
    float vBase;
    float originX;
    float originY;
    float dx01;
    float dx21;
    float dy01;
    float dy21;
    float determinant;
    float inverseDeterminant;
    float dz01;
    float dz21;
    float du01;
    float du21;
    float dv01;
    float dv21;
    float textureScale;
    float minPositiveZ;
    float vertexZ;
    float adjustX;
    float adjustY;
    int vertexIndex;
    int topVertexIndex;
    int bottomVertexIndex;
    int edgeCountA;
    int edgeCountB;
    int edgeVertexIndex;
    int fixed16Value;
    int edgeYStart;
    float edgeSampleY;
    ScanConvertEdge* edge;
    int firstScanline;
    int lastScanline;
    unsigned char* scanlineBase;
    int chunkPixels;
    int chunkBytes;
    float chunkWidth;
    double chunkBits;
    float chunkScale;
    float chunkZStep;
    float chunkUStep;
    float chunkVStep;
    TexturedQueuedSpanProc spanProc;
    unsigned short* palette;
    int* nextEdgeA;
    int* nextEdgeB;
    int currentXFixedA;
    int currentXFixedB;
    int xStepFixedA;
    int xStepFixedB;
    int endScanline;
    int nextEventScanline;
    int y;
    int xMin;
    int xMax;
    float sampleY;
    float rowRecipZ;
    float rowU;
    float rowV;
    int spanCount;
    int spanIndex;
    SpanNodePartial* span;
    int remaining;
    float startX;
    float z;
    float invZ;
    float uz;
    float vz;
    float startU;
    float startV;
    float endU;
    float endV;
    float endX;
    float countF;
    float endZ;
    float endScale;
    double uStartBits;
    double vStartBits;
    double uStepBits;
    double vStepBits;

    image = entry != 0 ? entry->image : 0;
    imageWidth = (float)(image->width);
    imageHeight = (float)(image->height);
    scaledUV[0].x = imageWidth * triVerts[0].z * triUVs[0].x;
    scaledUV[0].y = imageHeight * triVerts[0].z * triUVs[0].y;
    scaledUV[1].x = imageWidth * triVerts[1].z * triUVs[1].x;
    scaledUV[1].y = imageHeight * triVerts[1].z * triUVs[1].y;
    scaledUV[2].x = imageWidth * triVerts[2].z * triUVs[2].x;
    scaledUV[2].y = imageHeight * triVerts[2].z * triUVs[2].y;
    if (clippedTriVerts != 0
        && (clippedTriVerts[0].z < 10.0f || clippedTriVerts[1].z < 10.0f || clippedTriVerts[2].z < 10.0f)) {
        zMathBuildPerspectiveTextureInterpolants(
            clippedTriVerts,
            triUVs,
            &recipZGrad,
            &recipZBase,
            &uGrad,
            &uBase,
            &vGrad,
            &vBase
        );
        uGrad.x *= imageWidth;
        uGrad.y *= imageWidth;
        uBase *= imageWidth;
        vGrad.x *= imageHeight;
        vGrad.y *= imageHeight;
        vBase *= imageHeight;
        originX = g_zMath_ProjOffsetX;
        originY = g_zMath_ProjOffsetY;
    } else {
        dx21 = triVerts[2].x - triVerts[1].x;
        dy01 = triVerts[0].y - triVerts[1].y;
        dx01 = triVerts[0].x - triVerts[1].x;
        dy21 = triVerts[2].y - triVerts[1].y;
        determinant = dy21 * dx01 - dx21 * dy01;
        if (determinant != 0.0) {
            inverseDeterminant = -1.0f / determinant;
            dz01 = triVerts[0].z - triVerts[1].z;
            dz21 = triVerts[2].z - triVerts[1].z;
            du01 = scaledUV[0].x - scaledUV[1].x;
            du21 = scaledUV[2].x - scaledUV[1].x;
            dv01 = scaledUV[0].y - scaledUV[1].y;
            dv21 = scaledUV[2].y - scaledUV[1].y;
            recipZGrad.x = (dz21 * dy01 - dz01 * dy21) * inverseDeterminant;
            recipZGrad.y = (dz01 * dx21 - dz21 * dx01) * inverseDeterminant;
            uGrad.x = (du21 * dy01 - du01 * dy21) * inverseDeterminant;
            uGrad.y = (du01 * dx21 - du21 * dx01) * inverseDeterminant;
            vGrad.x = (dv21 * dy01 - dv01 * dy21) * inverseDeterminant;
            vGrad.y = (dv01 * dx21 - dv21 * dx01) * inverseDeterminant;
        } else {
            recipZGrad.x = recipZGrad.y = uGrad.x = uGrad.y = vGrad.x = vGrad.y = 0.0f;
        }
        recipZBase = triVerts[0].z;
        uBase = scaledUV[0].x;
        vBase = scaledUV[0].y;
        originX = triVerts[0].x;
        originY = triVerts[0].y;
    }

    if (entry != 0 && entry->nextVariant != 0) {
        image = zRndrTextureMipSelectVariantImage(entry, triVerts, 3, scaledUV, &recipZGrad, &uGrad, &vGrad);
        textureScale = 1048576.0f / image->widthScale;
    } else {
        textureScale = 1048576.0f;
    }

    // Retail estimates the nearest positive depth from the x term of the reciprocal-Z plane only.
    minPositiveZ = 1000.0f;
    topVertexIndex = 0;
    bottomVertexIndex = 0;
    for (vertexIndex = 0; vertexIndex < vertCount; ++vertexIndex) {
        vertexZ = (projectedVerts[vertexIndex].x - originX) * recipZGrad.x + recipZBase;
        if (vertexZ > 0.0f && vertexZ < minPositiveZ) {
            minPositiveZ = vertexZ;
        }
    }
    for (vertexIndex = 1; vertexIndex < vertCount; ++vertexIndex) {
        if (projectedVerts[vertexIndex].y < projectedVerts[topVertexIndex].y) {
            topVertexIndex = vertexIndex;
        }
        // Retail keeps the first lowest vertex (strict compare).
        if (projectedVerts[vertexIndex].y > projectedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = vertexIndex;
        }
    }

    adjustX = originX - 0.5f;
    adjustY = originY - 0.5f;
    recipZBase -= adjustX * recipZGrad.x + adjustY * recipZGrad.y;
    uBase -= adjustX * uGrad.x + adjustY * uGrad.y;
    vBase -= adjustX * vGrad.x + adjustY * vGrad.y;

    edgeCountA = 0;
    edgeCountB = 0;
    if (g_scanConvertMode != 0) {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    } else {
        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex + 1;
            if (nextIndex >= vertCount) {
                nextIndex -= vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableB[edgeCountB++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);

        edgeVertexIndex = topVertexIndex;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
        edgeYStart = (fixed16Value + 0x7fff) >> 16;
        edgeSampleY = (float)(edgeYStart) + 0.5f;
        do {
            int nextIndex = edgeVertexIndex - 1;
            if (nextIndex < 0) {
                nextIndex += vertCount;
            }
            if (edgeSampleY <= projectedVerts[nextIndex].y) {
                const float dy = projectedVerts[nextIndex].y - projectedVerts[edgeVertexIndex].y;
                const zVec3* start = &projectedVerts[edgeVertexIndex];
                edge = &edgeTableA[edgeCountA++];
                edge->yStart = edgeYStart;
                if (dy != 0.0f) {
                    const float xSlope = (projectedVerts[nextIndex].x - start->x) / dy;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(edge->xStepFixed, xSlope);
                    ZRNDR_SET_FIXED16_FROM_FLOAT(
                        edge->currentXFixed,
                        start->x + (((float)(edgeYStart) + 0.5f) - start->y) * xSlope
                    );
                }
                ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[nextIndex].y);
                edgeYStart = (fixed16Value + 0x7fff) >> 16;
                edgeSampleY = (float)(edgeYStart) + 0.5f;
            }
            edgeVertexIndex = nextIndex;
        } while (edgeVertexIndex != bottomVertexIndex);
    }

    // Retail terminates both edge tables with a far-below sentinel start row.
    edgeTableA[edgeCountA].yStart = 9999;
    edgeTableB[edgeCountB].yStart = 9999;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
    firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[bottomVertexIndex].y);
    lastScanline = (fixed16Value - 0x8041) >> 16;
    scanlineBase = (unsigned char*)(g_frameBuffer) + firstScanline * g_pitchBytes;

    if (g_perspectiveAdaptiveMinSpan != 0) {
        if (recipZGrad.x == 0.0f) {
            chunkPixels = g_perspectiveAdaptiveMaxSpan;
        } else {
            chunkBits = fabs(minPositiveZ * g_perspectiveAdaptiveSlope / recipZGrad.x) - -6755399441055744.0;
            chunkPixels = *(int*)(&chunkBits);
            if (chunkPixels > g_perspectiveAdaptiveMaxSpan) {
                chunkPixels = g_perspectiveAdaptiveMaxSpan;
            } else if (chunkPixels < g_perspectiveAdaptiveMinSpan) {
                chunkPixels = g_perspectiveAdaptiveMinSpan;
            }
        }
        chunkBytes = chunkPixels * g_bytesPerPixel;
        chunkWidth = (float)(chunkPixels);
    } else {
        chunkBytes = g_perspectiveTextureDeltaXBytes;
        chunkPixels = g_perspectiveTextureDeltaXPow2;
        chunkWidth = g_perspectiveTextureDeltaXPow2F;
    }
    chunkScale = textureScale / chunkWidth;

    g_spanActiveTexPixels = (unsigned char*)(image->pixels);
    g_spanQueuedTexAlphaMap = image->queuedAlphaMap;
    g_spanActiveTexShift = image->uShiftFrom20;
    g_spanActiveTexUMask = image->uMask;
    g_spanActiveTexVMask = image->vMaskFixed20;
    g_spanActiveConstAlphaBits = alpha255;
    chunkZStep = chunkWidth * recipZGrad.x;
    chunkUStep = chunkWidth * uGrad.x;
    chunkVStep = chunkWidth * vGrad.x;
    palette = (unsigned short*)(image->palette);
    if (palette == 0) {
        spanProc = g_pfnTexturedFanTriSpanOp_Mode0;
    } else {
        g_spanActiveTexPalette = variantIndex == -1 ? palette : &palette[(variantIndex + 1) * 0x100];
        spanProc = g_pfnTexturedFanTriSpanOp_Mode1;
    }

    xStepFixedA = edgeTableA[0].xStepFixed;
    xStepFixedB = edgeTableB[0].xStepFixed;
    currentXFixedA = edgeTableA[0].currentXFixed;
    currentXFixedB = edgeTableB[0].currentXFixed;
    endScanline = lastScanline + 1;
    nextEventScanline = endScanline;
    if (edgeTableA[1].yStart < nextEventScanline) {
        nextEventScanline = edgeTableA[1].yStart;
    }
    if (edgeTableB[1].yStart < nextEventScanline) {
        nextEventScanline = edgeTableB[1].yStart;
    }
    nextEdgeB = &edgeTableB[1].yStart;
    nextEdgeA = &edgeTableA[1].yStart;

    // Retail advances the edges only at the precomputed next event row and exits there past the last row.
    for (y = firstScanline;; ++y) {
        if (y >= nextEventScanline) {
            if (y >= endScanline) {
                return;
            }
            if (y >= *nextEdgeA) {
                currentXFixedA = nextEdgeA[1];
                xStepFixedA = nextEdgeA[-1];
                nextEdgeA += 4;
            }
            if (y >= *nextEdgeB) {
                currentXFixedB = nextEdgeB[1];
                xStepFixedB = nextEdgeB[-1];
                nextEdgeB += 4;
            }
            nextEventScanline = endScanline;
            if (*nextEdgeA < nextEventScanline) {
                nextEventScanline = *nextEdgeA;
            }
            if (*nextEdgeB < nextEventScanline) {
                nextEventScanline = *nextEdgeB;
            }
        }

        if (currentXFixedA <= currentXFixedB) {
            xMin = (currentXFixedA + 0x7fff) >> 16;
            xMax = (currentXFixedB - 0x8001) >> 16;
        } else {
            xMin = (currentXFixedB + 0x7fff) >> 16;
            xMax = (currentXFixedA - 0x8001) >> 16;
        }

        currentXFixedA += xStepFixedA;
        currentXFixedB += xStepFixedB;
        if (xMin <= xMax) {
            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            sampleY = (float)(y);
            rowRecipZ = sampleY * recipZGrad.y + recipZBase;
            g_spanAllocCursor->invDepth
                = ((float)(xMin)*recipZGrad.x + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*recipZGrad.x + rowRecipZ) * g_inverseDepthScale + g_inverseDepthBias;
            g_spanAllocCursor->depthSlope = recipZGrad.x;
            g_pfnBuildSpanListSecondary(spanList, y, &spanCount);
            if (spanCount != 0) {
                rowU = sampleY * uGrad.y + uBase;
                rowV = sampleY * vGrad.y + vBase;
                for (spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    span = spanList[spanIndex];
                    g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + span->sampleXMin * g_bytesPerPixel);
                    remaining = span->sampleXMax - span->sampleXMin + 1;
                    startX = (float)(span->sampleXMin);
                    z = startX * recipZGrad.x + rowRecipZ;
                    uz = startX * uGrad.x + rowU;
                    vz = startX * vGrad.x + rowV;
                    invZ = 1.0f / z;
                    startU = invZ * uz;
                    startV = invZ * vz;
                    while (remaining > chunkPixels) {
                        z += chunkZStep;
                        invZ = 1.0f / z;
                        uz += chunkUStep;
                        vz += chunkVStep;
                        endU = invZ * uz;
                        endV = invZ * vz;
                        uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                        vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                        uStepBits = (double)((endU - startU) * chunkScale) - -6755399441055744.0;
                        vStepBits = (double)((endV - startV) * chunkScale) - -6755399441055744.0;
                        g_spanActiveTexUStepFixed20 = *(int*)(&uStepBits);
                        g_spanActiveTexVStepFixed20 = *(int*)(&vStepBits);
                        spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), chunkPixels, g_spanActiveTexShift);
                        startU = endU;
                        startV = endV;
                        remaining -= chunkPixels;
                        g_spanCurrentSpanBaseAddr
                            = (unsigned short*)((unsigned char*)(g_spanCurrentSpanBaseAddr) + chunkBytes);
                    }
                    endX = (float)(span->sampleXMax);
                    countF = (float)(remaining);
                    endZ = endX * recipZGrad.x + rowRecipZ;
                    endScale = textureScale / (countF * endZ);
                    uz = endX * uGrad.x + rowU;
                    vz = endX * vGrad.x + rowV;
                    uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                    vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                    uStepBits = (double)((uz - startU * endZ) * endScale) - -6755399441055744.0;
                    vStepBits = (double)((vz - startV * endZ) * endScale) - -6755399441055744.0;
                    g_spanActiveTexUStepFixed20 = *(int*)(&uStepBits);
                    g_spanActiveTexVStepFixed20 = *(int*)(&vStepBits);
                    spanProc(*(int*)(&uStartBits), *(int*)(&vStartBits), remaining, g_spanActiveTexShift);
                }
            }
        }

        scanlineBase += g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawimmediateline
 * @recoil-artifact defines .text recoil:function:0x498bd0: zRndrDrawImmediateLine
 * @recoil-match byte
 *
 * Source file evidence: zRndr immediate line draw cluster in this source file.
 * Purpose: Dispatch one unclipped immediate line to the selected software line raster routine.
 */
void __fastcall zRndrDrawImmediateLine(int x0, int y0, int x1, int y1, int color16)
{
    g_pfnImmediateRaster4((unsigned short*)(g_frameBuffer), x0, y0, x1, y1, color16);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawclippedimmediatelinestrip
 * @recoil-artifact defines .text recoil:function:0x498c00: zRndrDrawClippedImmediateLineStrip
 * @recoil-match byte
 *
 * Source file evidence: zRndr immediate line draw cluster in this source file.
 * Purpose: Dispatch each segment of a clipped immediate line strip to the selected raster routine.
 */
void __fastcall
zRndrDrawClippedImmediateLineStrip(const zRndr_LinePoint2I* points, int segmentCount, const void* clipRect, int color16)
{
    const zRndr_LineClipRect2I* clip;
    const zRndr_LinePoint2I* point;
    int remaining;
    if (segmentCount <= 0) {
        return;
    }

    clip = (const zRndr_LineClipRect2I*)(clipRect);
    point = points + 1;
    remaining = segmentCount;
    do {
        g_pfnImmediateRaster5(
            (unsigned short*)(g_frameBuffer),
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-testpointvisibility
 * @recoil-artifact defines .text recoil:function:0x498c40: zRndrSpanOcclusionTestPointVisibility.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: stage one projected point as a single-pixel pending span and test
 * column visibility.
 *
 * Evidence: BN writes samplePoint z/x fields into gRndr_SpanAllocCursor,
 * truncates x/y through the original integer conversion path, calls
 * zRndrSpanOcclusionTestColumnVisibility, and returns one only when visible.
 */
int __fastcall zRndrSpanOcclusionTestPointVisibility(zVec3* samplePoint)
{
    int isVisible;
    g_spanAllocCursor->invDepth = samplePoint->z;
    g_spanAllocCursor->invDepthStep = samplePoint->z;
    g_spanAllocCursor->depthSlope = 0.0f;
    g_spanAllocCursor->sampleXMin = (int)(samplePoint->x);
    g_spanAllocCursor->sampleXMax = g_spanAllocCursor->sampleXMin;

    zRndrSpanOcclusionTestColumnVisibility((int)(samplePoint->y), &isVisible);
    return isVisible > 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-lensflare-drawqueuedsample16-clippedframebuffer
 * @recoil-artifact defines .text recoil:function:0x498cb0: zRndr::LensFlareDrawQueuedSample16ClippedFramebuffer
 *
 *
 * Purpose: Draw one queued lens-flare sample into the clipped 16-bit framebuffer.
 */
void __fastcall
LensFlareDrawQueuedSample16ClippedFramebuffer(LensFlareSamplePartial* sample, float screenScale, int yOffsetPixels)
{
    int packedColor;
    int redDelta;
    float reciprocalZ;
    int y;
    int x;
    unsigned short* pixel;
    zRndr_LensFlareSource* lensFlareSource;
    int overlayAlpha;
    int overlayColor;
    int blended;
    int greenDelta;
    int blueDelta;
    int blendTowardFramebuffer;
    int fadeAlpha;
    int frameColor;

    lensFlareSource = (zRndr_LensFlareSource*)((unsigned int)(sample->lensFlareSource));
    blendTowardFramebuffer = 0;
    if (lensFlareSource != 0 && lensFlareSource->depthFadeInvZMax != 0.0f) {
        if (sample->reciprocalZ == 0.0f) {
            return;
        }

        reciprocalZ = 1.0f / sample->reciprocalZ;
        if (reciprocalZ >= lensFlareSource->depthFadeInvZMax) {
            return;
        }

        if (reciprocalZ > lensFlareSource->depthFadeInvZMin) {
            blendTowardFramebuffer = 1;
        }
    }

    y = (int)(sample->y * screenScale) + yOffsetPixels;
    x = (int)(screenScale * sample->x);
    if ((unsigned int)(x) > (unsigned int)(g_activeRegionWidth)
        || (unsigned int)(y) > (unsigned int)(g_activeRegionHeight)) {
        return;
    }

    pixel = (unsigned short*)(g_frameBuffer) + ((unsigned int)(g_pitchBytes) >> 1) * y + x;
    packedColor = sample->packedColor16;

    if (g_overlayBlendEnabled != 0) {
        overlayAlpha = (int)(g_overlayBlendAlpha * 255.0);
        if (g_pixelPackGreenBits == 6) {
            if (overlayAlpha > 3) {
                if (overlayAlpha >= 0xfc) {
                    packedColor = g_overlayBlendPackedColor16 & 0xffff;
                } else {
                    overlayColor = g_overlayBlendPackedColor16;
                    redDelta = ((((overlayColor & 0xf800) - (packedColor & 0xf800)) * overlayAlpha) >> 8) & 0xfffff800;
                    blended = packedColor + redDelta;
                    greenDelta = ((overlayColor & 0x07e0) - (packedColor & 0x07e0)) * overlayAlpha;
                    blueDelta = ((overlayColor & 0x001f) - (blended & 0x001f)) * overlayAlpha;
                    greenDelta = (greenDelta >> 8) & 0xffffffe0;
                    blueDelta >>= 8;
                    packedColor = greenDelta + blueDelta + blended;
                }
            }
        } else if (overlayAlpha > 7) {
            if (overlayAlpha >= 0xfc) {
                packedColor = g_overlayBlendPackedColor16 & 0xffff;
            } else {
                overlayColor = g_overlayBlendPackedColor16;
                greenDelta = ((overlayColor & 0x03e0) - (packedColor & 0x03e0)) * overlayAlpha;
                blueDelta = ((overlayColor & 0x001f) - (packedColor & 0x001f)) * overlayAlpha;
                redDelta = ((overlayColor & 0x7c00) - (packedColor & 0x7c00)) * overlayAlpha;
                greenDelta = (greenDelta >> 8) & 0xffffffe0;
                blueDelta >>= 8;
                redDelta = (redDelta >> 8) & 0xfffffc00;
                greenDelta += blueDelta;
                redDelta += greenDelta;
                packedColor += redDelta;
            }
        }
    }

    // Retail shares one packed-colour store for the unblended and fully faded exits; the fade blends return early.
    if (blendTowardFramebuffer != 0) {
        fadeAlpha = (int)((lensFlareSource->depthFadeInvZMax - reciprocalZ) * lensFlareSource->depthFadeScale);
        if (g_pixelPackGreenBits == 6) {
            if (fadeAlpha <= 3) {
                return;
            }
            if (fadeAlpha < 0xfc) {
                frameColor = *pixel;
                greenDelta = ((packedColor & 0x07e0) - (frameColor & 0x07e0)) * fadeAlpha;
                redDelta = ((((packedColor & 0xf800) - (frameColor & 0xf800)) * fadeAlpha) >> 8) & 0xfffff800;
                frameColor += redDelta;
                blueDelta = ((packedColor & 0x001f) - (frameColor & 0x001f)) * fadeAlpha;
                greenDelta = (greenDelta >> 8) & 0xffffffe0;
                blueDelta >>= 8;
                blueDelta += greenDelta;
                frameColor += blueDelta;
                *pixel = (unsigned short)(frameColor);
                return;
            }
        } else {
            if (fadeAlpha <= 7) {
                return;
            }
            if (fadeAlpha < 0xfc) {
                frameColor = *pixel;
                redDelta = (((packedColor & 0x7c00) - (frameColor & 0x7c00)) * fadeAlpha) >> 8;
                greenDelta = (((packedColor & 0x03e0) - (frameColor & 0x03e0)) * fadeAlpha) >> 8;
                redDelta &= 0xfffffc00;
                greenDelta &= 0xffffffe0;
                // Retail folds both destination updates into one 16-bit read-modify-write add.
                *pixel += redDelta;
                blueDelta = (((packedColor & 0x001f) - (frameColor & 0x001f)) * fadeAlpha) >> 8;
                *pixel += blueDelta + greenDelta;
                return;
            }
        }
    }

    *pixel = (unsigned short)(packedColor);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-testsample
 * @recoil-artifact defines .text recoil:function:0x498f90: zRndrSpanOcclusionTestSample.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: dispatch one visible sample point through the active zRndr point
 * operation.
 * Evidence: BN loads gRndr_pFrameBuffer and gRndr_pfnPointOpActive, passes y/x
 * and color16 in the observed fastcall/stack shape, and performs no additional
 * span state updates.
 */
void __fastcall zRndrSpanOcclusionTestSample(int x, int y, int color16)
{
    g_pfnPointOpActive(g_frameBuffer, y, x, color16);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawcircleoutline16-framebuffer
 * @recoil-artifact defines .text recoil:function:0x498fb0: zRndrDrawCircleOutline16Framebuffer.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: zRndr_Draw.cpp.
 * Purpose: draw a 16-bit framebuffer circle outline through midpoint octant
 * batches.
 *
 * Evidence: BN stores the circle center and auxiliary argument globals, skips
 * non-positive radius values, dispatches the initial y=0 octants, then advances
 * the midpoint decision variable until x <= y.
 */
void __fastcall zRndrDrawCircleOutline16Framebuffer(int centerX, int centerY, int radius, int packedColor, int auxArg)
{
    int x = radius;
    int y = 0;
    int decisionVar = 1 - x;
    if (x <= 0) {
        return;
    }

    g_zRndr_CircleCenterX = centerX;
    g_zRndr_CircleCenterY = centerY;
    g_zRndr_CircleDrawAuxArg = auxArg;

    zRndrDrawCircleOctants16Framebuffer(0, x, packedColor);
    do {
        if (decisionVar < 0) {
            decisionVar += (y << 1) + 3;
        } else {
            decisionVar += ((y - x) << 1) + 5;
            --x;
        }

        ++y;
        zRndrDrawCircleOctants16Framebuffer(y, x, packedColor);
    } while (x > y);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawcircleoctants16-framebuffer
 * @recoil-artifact defines .text recoil:function:0x499020: zRndrDrawCircleOctants16Framebuffer.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: zRndr_Draw.cpp.
 * Purpose: emit the eight symmetric framebuffer points for one circle-outline
 * midpoint step.
 *
 * Evidence: BN reads g_zRndr_CircleCenterX/Y, forwards gRndr_pFrameBuffer,
 * and calls gRndr_pfnPointOpActive for each octant using fastcall y/x inputs
 * plus the caller-supplied packed color.
 */
void __fastcall zRndrDrawCircleOctants16Framebuffer(int y, int x, int packedColor)
{
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY + y, g_zRndr_CircleCenterX + x, packedColor);
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY + y, g_zRndr_CircleCenterX - x, packedColor);
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY - y, g_zRndr_CircleCenterX + x, packedColor);
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY - y, g_zRndr_CircleCenterX - x, packedColor);
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY + x, g_zRndr_CircleCenterX + y, packedColor);
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY + x, g_zRndr_CircleCenterX - y, packedColor);
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY - x, g_zRndr_CircleCenterX - y, packedColor);
    g_pfnPointOpActive(g_frameBuffer, g_zRndr_CircleCenterY - x, g_zRndr_CircleCenterX + y, packedColor);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-texturemip-selectvariantimage
 * @recoil-artifact defines .text recoil:function:0x499130: zRndrTextureMipSelectVariantImage
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Select a mip/variant image for a textured polygon from its projected texture metric.
 */
zVidImagePartial* __fastcall zRndrTextureMipSelectVariantImage(
    zImage_TexDirEntryPartial* entry,
    const zVec3* triVerts,
    int vertCount,
    const zVec2* vertexUvPairs,
    const zVec2* mipParamsA,
    const zVec2* mipParamsB,
    const zVec2* mipParamsC
)
{
    float selectedZ;
    int selectedVertex;
    int i;
    const zVec3* selected;
    float invZAtX;
    float invZAtY;
    float uOverZ;
    // Retail keeps the delta pairs in two stack arrays (v pair [esp+0x10], u pair [esp+0x18]).
    float uDeltas[2];
    float vOverZ;
    float vDeltas[2];
    float mipMetric;
    double variantIndexBits;
    int variantIndex;
    if (g_textureMipSelectionEnabled == 0) {
        return entry != 0 ? entry->image : 0;
    }

    selectedZ = triVerts[0].z;
    selectedVertex = 0;
    for (i = 1; i < vertCount; ++i) {
        if (selectedZ < triVerts[i].z) {
            selectedZ = triVerts[i].z;
            selectedVertex = i;
        }
    }

    selected = &triVerts[selectedVertex];
    invZAtX = 1.0f / (mipParamsA->x + selected->z);
    invZAtY = 1.0f / (mipParamsA->y + selected->z);
    // Retail forms 1/z after both offset reciprocals and never gives it a home: no invZ local.
    uOverZ = vertexUvPairs[selectedVertex].x * (1.0f / selected->z);
    uDeltas[0] = (mipParamsB->x + vertexUvPairs[selectedVertex].x) * invZAtX - uOverZ;
    uDeltas[1] = (mipParamsB->y + vertexUvPairs[selectedVertex].x) * invZAtY - uOverZ;
    vOverZ = vertexUvPairs[selectedVertex].y * (1.0f / selected->z);
    vDeltas[0] = (mipParamsC->x + vertexUvPairs[selectedVertex].y) * invZAtX - vOverZ;
    vDeltas[1] = (mipParamsC->y + vertexUvPairs[selectedVertex].y) * invZAtY - vOverZ;
    mipMetric = uDeltas[0];
    if (mipMetric <= uDeltas[1]) {
        mipMetric = uDeltas[1];
    }
    if (vDeltas[0] > mipMetric) {
        mipMetric = vDeltas[0];
    }
    if (vDeltas[1] > mipMetric) {
        mipMetric = vDeltas[1];
    }

    variantIndexBits = (double)(mipMetric) - -6755399441055744.0;
    variantIndex = (*(int*)(&variantIndexBits)) >> 1;
    return GetVariantImageAtIndex(entry, variantIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-plotpixel16
 * @recoil-artifact defines .text recoil:function:0x4992b0: zRndrPlotPixel16
 * @recoil-match byte
 *
 * Purpose: Plot one 16-bit pixel into the active framebuffer row pitch.
 */
void __fastcall zRndrPlotPixel16(unsigned short* dstPixels, int y, int x, int color16)
{
    const unsigned int pitchWords = (unsigned int)(g_pitchBytes) >> 1;
    dstPixels[pitchWords * y + x] = (unsigned short)(color16);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawline16
 * @recoil-artifact defines .text recoil:function:0x4992d0: zRndrDrawLine16
 * @recoil-match byte
 *
 * Purpose: Rasterize an unclipped 16-bit Bresenham line into the active framebuffer.
 */
void __fastcall zRndrDrawLine16(unsigned short* dstPixels, int x0, int y0, int x1, int y1, int color16)
{
    // Retail walks an index; VC5 rebuilds the pointer per branch (0x499319, 0x499369).
    const int pitch = (int)(((unsigned int)g_pitchBytes) >> 1);
    int index = pitch * y0 + x0;

    int dy = y1 - y0;
    int rowStep;
    int dx;
    int xStep;
    if (dy < 0) {
        dy = -dy;
        rowStep = -pitch;
    } else {
        rowStep = pitch;
    }

    dx = x1 - x0;
    if (dx < 0) {
        dx = -dx;
        xStep = -1;
    } else {
        xStep = 1;
    }

    if (dx > dy) {
        int error = dx >> 1;
        int count = dx + 1;
        do {
            dstPixels[index] = (unsigned short)color16;
            error += dy;
            index += xStep;
            if (error > dx) {
                error -= dx;
                index += rowStep;
            }
        } while (--count != 0);
    } else {
        int error = dy >> 1;
        int count = dy + 1;
        do {
            dstPixels[index] = (unsigned short)color16;
            error += dx;
            index += rowStep;
            if (error > dy) {
                error -= dy;
                index += xStep;
            }
        } while (--count != 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawline16-segmented
 * @recoil-artifact defines .text recoil:function:0x4993a0: zRndrDrawLine16Segmented
 * @recoil-match byte
 *
 * Purpose: Rasterize a segmented 16-bit Bresenham line into the active framebuffer.
 */
void __fastcall
zRndrDrawLine16Segmented(unsigned short* dstPixels, int x0, int y0, int x1, int y1, int color16, int segmentCount)
{
    // Retail walks an index like zRndrDrawLine16; VC5 rebuilds the pointer per branch (0x49941b, 0x49949e).
    const int pitch = (int)(((unsigned int)g_pitchBytes) >> 1);
    int drawSegment = 1;
    int index = pitch * y0 + x0;
    int error;
    int count;
    int segmentCounter;

    int dy = y1 - y0;
    int rowStep;
    int dx;
    int xStep;
    if (dy < 0) {
        dy = -dy;
        rowStep = -pitch;
    } else {
        rowStep = pitch;
    }

    dx = x1 - x0;
    if (dx < 0) {
        dx = -dx;
        xStep = -1;
    } else {
        xStep = 1;
    }

    // Retail VC5 reuses the consumed segmentCount argument slot for the branch segment limit.
    if (dx > dy) {
        error = dx >> 1;
        count = dx + 1;
        segmentCount = count / segmentCount;
        segmentCounter = 0;
        do {
            if (drawSegment != 0) {
                dstPixels[index] = (unsigned short)color16;
            }
            index += xStep;
            error += dy;
            if (error > dx) {
                error -= dx;
                index += rowStep;
            }
            if (segmentCounter++ >= segmentCount) {
                segmentCounter = 0;
                drawSegment = drawSegment == 0 ? 1 : 0;
            }
        } while (--count != 0);
    } else {
        error = dy >> 1;
        count = dy + 1;
        segmentCount = count / segmentCount;
        segmentCounter = 0;
        do {
            if (drawSegment != 0) {
                dstPixels[index] = (unsigned short)color16;
            }
            index += rowStep;
            error += dx;
            if (error > dy) {
                error -= dy;
                index += xStep;
            }
            if (segmentCounter++ >= segmentCount) {
                segmentCounter = 0;
                drawSegment = drawSegment == 0 ? 1 : 0;
            }
        } while (--count != 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawline16-clipped
 * @recoil-artifact defines .text recoil:function:0x499500: zRndrDrawLine16Clipped
 * @recoil-match byte
 *
 * Purpose: Clip and rasterize a 16-bit line into the active framebuffer.
 */
void __fastcall zRndrDrawLine16Clipped(
    unsigned short* dstPixels,
    const zRndr_LineClipRect2I* clipRect,
    int x0,
    int y0,
    int x1,
    int y1,
    int color16
)
{
    int outcode0;
    int outcode1;
    int dx;
    int dy;
    float yPerX;
    float xPerY;
    int rowStep;
    int index;
    int xStep;
    int error;
    int count;

    // Retail zeroes both outcodes before the first test (0x499512, 0x499514).
    outcode0 = outcode1 = 0;
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

    dx = x1 - x0;
    dy = y1 - y0;
    if ((outcode0 | outcode1) != 0) {
        yPerX = dx == 0 ? 0.0f : (float)(dy) / dx;
        xPerY = dy == 0 ? 0.0f : (float)(dx) / dy;
        if (x0 < clipRect->left) {
            y0 += (int)((clipRect->left - x0) * yPerX);
            x0 = clipRect->left;
        } else if (x0 > clipRect->right) {
            y0 += (int)((clipRect->right - x0) * yPerX);
            x0 = clipRect->right;
        }
        if (x1 < clipRect->left) {
            y1 += (int)((clipRect->left - x1) * yPerX);
            x1 = clipRect->left;
        } else if (x1 > clipRect->right) {
            y1 += (int)((clipRect->right - x1) * yPerX);
            x1 = clipRect->right;
        }
        if (y0 < clipRect->top) {
            if (y1 < clipRect->top) {
                return;
            }
            x0 += (int)((clipRect->top - y0) * xPerY);
            y0 = clipRect->top;
        } else if (y0 > clipRect->bottom) {
            if (y1 > clipRect->bottom) {
                return;
            }
            x0 += (int)((clipRect->bottom - y0) * xPerY);
            y0 = clipRect->bottom;
        }
        if (y1 < clipRect->top) {
            x1 += (int)((clipRect->top - y1) * xPerY);
            y1 = clipRect->top;
        } else if (y1 > clipRect->bottom) {
            x1 += (int)((clipRect->bottom - y1) * xPerY);
            y1 = clipRect->bottom;
        }
        dx = x1 - x0;
        dy = y1 - y0;
    }

    // Retail walks an index like zRndrDrawLine16; VC5 rebuilds the pointer per branch (0x499758, 0x499798).
    rowStep = (unsigned int)(g_pitchBytes) >> 1;
    xStep = 1;
    index = rowStep * y0 + x0;
    if (dy < 0) {
        dy = -dy;
        rowStep = -rowStep;
    }
    if (dx < 0) {
        dx = -dx;
        xStep = -1;
    }

    if (dx > dy) {
        error = dx >> 1;
        count = dx + 1;
        do {
            dstPixels[index] = (unsigned short)(color16);
            error += dy;
            index += xStep;
            if (error > dx) {
                error -= dx;
                index += rowStep;
            }
        } while (--count);
        return;
    }

    error = dy >> 1;
    count = dy + 1;
    do {
        dstPixels[index] = (unsigned short)(color16);
        error += dx;
        index += rowStep;
        if (error > dy) {
            error -= dy;
            index += xStep;
        }
    } while (--count);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-fillspan16opaque
 * @recoil-artifact defines .text recoil:function:0x4997d0: zRndrFillSpan16Opaque
 *
 *
 * Purpose: Fill the active reverse span with one opaque 16-bit color.
 *
 * Evidence: BN reads gRndr_CurrentSpanBaseAddr, computes the end of the span,
 * and writes pixels backward with push ax/eax. It does not touch
 * gRndr_SavedEspSlot, so source keeps this leaf as a typed reverse fill rather
 * than part of the switch-vshift ESP-pivot source family.
 */
void __fastcall zRndrFillSpan16Opaque(int packedColor16, int pixelCount)
{
    const unsigned short color16 = (unsigned short)(packedColor16);
    unsigned short* cursor = g_spanCurrentSpanBaseAddr + pixelCount;
    unsigned int remaining = (unsigned int)(pixelCount);

    while (remaining != 0) {
        --cursor;
        *cursor = color16;
        --remaining;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-fillspan555solid
 * @recoil-artifact defines .text recoil:function:0x499810: zRndrFillSpan555Solid
 * @recoil-match source
 *
 * Purpose: Blend a solid color into the active 555 span using the supplied alpha.
 *
 * Evidence: BN uses gRndr_CurrentSpanBaseAddr as an ordinary word pointer for
 * this solid-fill leaf; there is no ESP-pivot write shape here.
 */
void __fastcall zRndrFillSpan555Solid(int packedColor16, int blendAlpha, int pixelCount)
{
    unsigned short* cursor = g_spanCurrentSpanBaseAddr;
    do {
        if (blendAlpha > 7) {
            if (blendAlpha >= 0xfc) {
                *cursor = (unsigned short)(packedColor16);
            } else {
                const int dst = (short)(*cursor);
                int redDelta, greenDelta, blueDelta;
                redDelta = (((packedColor16 & 0x7c00) - (dst & 0x7c00)) * blendAlpha) >> 8;
                greenDelta = (((packedColor16 & 0x03e0) - (dst & 0x03e0)) * blendAlpha) >> 8;
                redDelta &= 0xfffffc00;
                greenDelta &= 0xffffffe0;
                // Retail folds both cursor updates into one 16-bit read-modify-write add.
                *cursor += redDelta;
                blueDelta = (((packedColor16 & 0x001f) - (dst & 0x001f)) * blendAlpha) >> 8;
                *cursor += blueDelta + greenDelta;
            }
        }

        ++cursor;
        --pixelCount;
    } while (pixelCount != 0);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-fillspan565solid
 * @recoil-artifact defines .text recoil:function:0x4998a0: zRndrFillSpan565Solid
 * @recoil-match source
 *
 * Purpose: Blend a solid color into the active 565 span using the supplied alpha.
 *
 * Evidence: BN uses gRndr_CurrentSpanBaseAddr as an ordinary word pointer here;
 * the limited reconstruction marker records only BN's partial-register display.
 */
void __fastcall zRndrFillSpan565Solid(int packedColor16, int blendAlpha, int pixelCount)
{
    unsigned short* cursor = g_spanCurrentSpanBaseAddr;
    do {
        if (blendAlpha > 3) {
            if (blendAlpha >= 0xfc) {
                *cursor = (unsigned short)(packedColor16);
            } else {
                int redDelta, greenDelta, blueDelta;
                int dst = (short)(*cursor);
                redDelta = (((packedColor16 & 0xf800) - (dst & 0xf800)) * blendAlpha) >> 8;
                greenDelta = (((packedColor16 & 0x07e0) - (dst & 0x07e0)) * blendAlpha) >> 8;
                redDelta &= 0xfffff800;
                greenDelta &= 0xffffffe0;
                dst += redDelta;
                blueDelta = (((packedColor16 & 0x001f) - (dst & 0x001f)) * blendAlpha) >> 8;
                // Retail adds blue + green first, then adds that sum into the red-adjusted dst (0x49990a/0x49990c).
                blueDelta += greenDelta;
                *cursor = (unsigned short)(dst + blueDelta);
            }
        }

        ++cursor;
        --pixelCount;
    } while (pixelCount != 0);
}
