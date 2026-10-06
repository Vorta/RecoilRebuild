// zRender compilation unit between zrndr_init.c and zrndr_draw.c, inferred from
// the retail object boundary [0x492000, 0x499930): its .rdata pooled
// constants [0x4d2df0, 0x4d2e40) and its .bss run [0x56b1f0, 0x56b248) are
// separate from the neighbouring zRender objects; an unreferenced 16-byte
// .rdata run [0x4d2de0, 0x4d2df0) precedes them and is not modeled. Original
// filename unresolved; zrndr_poly.c is a provisional name (2026-10-02).

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

namespace
{
    template<class T>
        /**
         * Recovered helper: MinValue
         * Original-source helper evidence: No standalone plan entry was found; recovered from zRndr span, polygon, and
         * scan-conversion callers in this source file. Purpose: Return the smaller of two values without changing
         * caller-owned storage.
         */
        const T& MinValue(const T& lhs, const T& rhs)
    {
        return lhs < rhs ? lhs : rhs;
    }

    template<class T>
        /**
         * Recovered helper: MaxValue
         * Original-source helper evidence: No standalone plan entry was found; recovered from zRndr span, polygon, and
         * scan-conversion callers in this source file. Purpose: Return the larger of two values without changing
         * caller-owned storage.
         */
        const T& MaxValue(const T& lhs, const T& rhs)
    {
        return lhs < rhs ? rhs : lhs;
    }

    /**
     * Recovered helper: sort.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * address-backed zRndr scan conversion callers in this source file.
     * Purpose: Sort scanline intersection samples in ascending order.
     */
    void sort(float* first, float* last)
    {
        for (float* it = first + 1; it < last; ++it) {
            float value = *it;
            float* scan = it;
            while (scan > first && value < scan[-1]) {
                *scan = scan[-1];
                --scan;
            }
            *scan = value;
        }
    }
} // namespace
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

namespace zRndr
{

    namespace
    {
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
            SpanNodePartial* spanList[0x141];
        };

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
        int Fixed16FromFloat(float value)
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
        int ScanlineStartFromY(float y)
        {
            return (Fixed16FromFloat(y) + 0x7fff) >> 16;
        }

        /**
         * Recovered helper: ScanlineEndFromY.
         * Original-source helper evidence: No standalone plan entry was found; recovered from
         * zRndr span and polygon raster callers that bias the ending scanline from fixed-point Y.
         * Purpose: Compute the last covered scanline for a polygon edge Y coordinate.
         */
        int ScanlineEndFromY(float y)
        {
            return (Fixed16FromFloat(y) - 0x8041) >> 16;
        }

        /**
         * Recovered helper: SpanStartFromX.
         * Original-source helper evidence: No standalone plan entry was found; recovered from
         * zRndr span builders that bias the starting pixel from fixed-point X.
         * Purpose: Compute the first covered span sample for an edge X coordinate.
         */
        int SpanStartFromX(float x)
        {
            return (Fixed16FromFloat(x) + 0x7fff) >> 16;
        }

        /**
         * Recovered helper: SpanEndFromX.
         * Original-source helper evidence: No standalone plan entry was found; recovered from
         * zRndr span builders that bias the ending pixel from fixed-point X.
         * Purpose: Compute the last covered span sample for an edge X coordinate.
         */
        int SpanEndFromX(float x)
        {
            return (Fixed16FromFloat(x) - 0x8001) >> 16;
        }

        /**
         * Recovered helper: AppendSpanListNode.
         * Original-source helper evidence: No standalone plan entry was found; recovered from
         * zRndr span-occlusion builders that coalesce adjacent visible span nodes.
         * Purpose: Append a visible span node and merge it with the previous node when contiguous.
         */
        void AppendSpanListNode(SpanNodePartial * *spanList, int* spanCount, SpanNodePartial* node)
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
        float SpanDepthAtX(const SpanNodePartial* span, int x)
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
         * Recovered helper: SpanDepthAtX.
         * Original-source helper evidence: No standalone plan entry was found; recovered from
         * zRndr span split paths using explicit span depth parts.
         * Purpose: Evaluate inverse depth from a span start, base depth, and depth slope.
         */
        float SpanDepthAtX(int sampleXMin, float invDepth, float depthSlope, int x)
        {
            return invDepth + (float)(x - sampleXMin) * depthSlope;
        }

        /**
         * Recovered helper: SpanDepthAtXByParts.
         * Original-source helper evidence: No standalone plan entry was found; recovered from
         * zRndr span split paths using explicit span depth parts.
         * Purpose: Evaluate inverse depth from a span start, base depth, and depth slope.
         */
        float SpanDepthAtXByParts(int sampleXMin, float invDepth, float depthSlope, int x)
        {
            return invDepth + (float)(x - sampleXMin) * depthSlope;
        }

        /**
         * Recovered helper: LinkSpanNode.
         * Original-source helper evidence: No standalone plan entry was found; recovered from
         * zRndr span-occlusion insertion paths that update the column head/link fields.
         * Purpose: Link a pending span node into one column and refresh the span iterator globals.
         */
        void LinkSpanNode(int columnIndex, SpanNodePartial* previous, SpanNodePartial* node, SpanNodePartial* next)
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
        void InsertPendingSpanSorted(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
        {
            *spanCount = 0;
            if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
                return;
            }

            SpanNodePartial* pending = g_spanAllocCursor;
            pending->next = 0;

            SpanNodePartial* previous = 0;
            SpanNodePartial* current = g_spanColumnHeadTable[columnIndex];
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
        void InsertPendingSpanNoDepthTest(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
        {
            *spanCount = 0;
            if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
                return;
            }

            SpanNodePartial* pending = g_spanAllocCursor;
            SpanNodePartial* previous = 0;
            SpanNodePartial* current = g_spanColumnHeadTable[columnIndex];

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
                            = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMax);

                        if (currentMax > pending->sampleXMax) {
                            SpanNodePartial* rightSplit = pending + 1;
                            rightSplit->sampleXMin = pending->sampleXMax + 1;
                            rightSplit->sampleXMax = currentMax;
                            rightSplit->invDepth
                                = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, rightSplit->sampleXMin);
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
                current->invDepth = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMin);
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
        void BuildVisibleSpanListWithDepthTest(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
        {
            *spanCount = 0;
            if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
                return;
            }

            SpanNodePartial* pending = g_spanAllocCursor;
            pending->next = 0;
            SpanNodePartial* current = g_spanColumnHeadTable[columnIndex];

            while (current != 0 && pending->sampleXMin > current->sampleXMax) {
                current = current->next;
            }

            while (current != 0) {
                if (pending->sampleXMax < current->sampleXMin) {
                    break;
                }

                SpanNodePartial occluder = *current;
                const bool pendingInFront = zRndrSpanOcclusionTestSpanDepthOrderPair(pending, &occluder) != 0;

                const int pendingMin = pending->sampleXMin;
                const int pendingMax = pending->sampleXMax;
                const float pendingInvDepth = pending->invDepth;
                const float pendingInvDepthStep = pending->invDepthStep;
                const float pendingDepthSlope = pending->depthSlope;

                if (pendingInFront) {
                    if (occluder.sampleXMax < pendingMax && occluder.sampleXMax >= pendingMin) {
                        const int splitMax = occluder.sampleXMax;
                        pending->sampleXMax = splitMax;
                        pending->invDepthStep = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, splitMax);
                        AppendSpanListNode(spanList, spanCount, pending);
                        ++g_spanAllocCursor;

                        pending = g_spanAllocCursor;
                        pending->next = 0;
                        pending->sampleXMin = splitMax + 1;
                        pending->sampleXMax = pendingMax;
                        pending->invDepth
                            = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
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
                            = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
                    }

                    current = current->next;
                    continue;
                }

                if (occluder.sampleXMin <= pendingMax) {
                    const int leftMax = occluder.sampleXMin - 1;
                    pending->sampleXMax = leftMax;
                    pending->invDepthStep = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, leftMax);
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
                        = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
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
        void InsertPendingSpanWithDepthTest(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
        {
            *spanCount = 0;
            if (g_spanColumnHeadTable == 0 || g_spanAllocCursor == 0 || columnIndex < 0) {
                return;
            }

            SpanNodePartial* pending = g_spanAllocCursor;
            pending->next = 0;
            SpanNodePartial* previous = 0;
            SpanNodePartial* current = g_spanColumnHeadTable[columnIndex];

            while (current != 0 && pending->sampleXMin > current->sampleXMax) {
                previous = current;
                current = current->next;
            }

            while (current != 0) {
                if (pending->sampleXMax < current->sampleXMin) {
                    break;
                }

                const bool pendingInFront = zRndrSpanOcclusionTestSpanDepthOrderPair(pending, current) != 0;

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
                        current->invDepthStep
                            = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMax);

                        if (currentMax > pendingMax) {
                            SpanNodePartial* rightSplit = pending + 1;
                            rightSplit->sampleXMin = pendingMax + 1;
                            rightSplit->sampleXMax = currentMax;
                            rightSplit->invDepth
                                = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, rightSplit->sampleXMin);
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
                        = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMin);
                    break;
                }

                if (current->sampleXMin <= pendingMin) {
                    if (current->sampleXMax >= pendingMax) {
                        return;
                    }

                    if (current->sampleXMax >= pendingMin) {
                        pending->sampleXMin = current->sampleXMax + 1;
                        pending->invDepth
                            = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
                    }

                    previous = current;
                    current = current->next;
                    continue;
                }

                if (current->sampleXMin <= pendingMax) {
                    const int leftMax = current->sampleXMin - 1;
                    pending->sampleXMax = leftMax;
                    pending->invDepthStep = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, leftMax);
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
                        = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
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

    } // namespace

    // namespace

    // namespace

    namespace
    {
        /**
         * Recovered inline helper: zRndr 565 lens-flare color blend
         * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x498cb0 as
         * the 565 branch of the lens-flare pixel blend path. Purpose: Blend one packed 565 color toward another using
         * an 8-bit alpha value.
         */
        static inline unsigned short BlendPacked565(unsigned short from, unsigned short to, int alpha)
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
        static inline unsigned short BlendPacked555(unsigned short from, unsigned short to, int alpha)
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
        static inline unsigned short BlendLensFlarePixel(unsigned short from, unsigned short to, int alpha)
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
    } // namespace

    /**
     * Recovered helper: SpanOcclusionInsertPendingSpanSorted.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * address-backed zRndr span-occlusion dispatch setup that selects the sorted insertion helper.
     * Purpose: Route span-occlusion dispatch to the sorted pending-span insertion helper.
     */
    void SpanOcclusionInsertPendingSpanSorted(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
    {
        InsertPendingSpanSorted(spanList, columnIndex, spanCount);
    }

    /**
     * Recovered helper: SpanOcclusionInsertPendingSpanWithDepthTest.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * address-backed zRndr span-occlusion dispatch setup that selects depth-tested insertion.
     * Purpose: Route span-occlusion dispatch to the depth-tested pending-span insertion helper.
     */
    void SpanOcclusionInsertPendingSpanWithDepthTest(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
    {
        InsertPendingSpanWithDepthTest(spanList, columnIndex, spanCount);
    }

    /**
     * Recovered helper: SpanOcclusionInsertPendingSpanNoDepthTest.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * address-backed zRndr span-occlusion dispatch setup that selects non-depth insertion.
     * Purpose: Route span-occlusion dispatch to the non-depth pending-span insertion helper.
     */
    void SpanOcclusionInsertPendingSpanNoDepthTest(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
    {
        InsertPendingSpanNoDepthTest(spanList, columnIndex, spanCount);
    }

    /**
     * Recovered helper: SpanOcclusionBuildVisibleSpanListWithDepthTest.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * address-backed zRndr span-occlusion dispatch setup that selects visibility-list building.
     * Purpose: Route span-occlusion dispatch to the depth-tested visible-span list builder.
     */
    void SpanOcclusionBuildVisibleSpanListWithDepthTest(SpanNodePartial * *spanList, int columnIndex, int* spanCount)
    {
        BuildVisibleSpanListWithDepthTest(spanList, columnIndex, spanCount);
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
static float zRndrSpanDepthAtXByPartsLocal(int sampleXMin, float invDepth, float depthSlope, int x)
{
    return invDepth + (float)(x - sampleXMin) * depthSlope;
}

namespace
{
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
    int RoundToFixed20(float value)
    {
        return (int)(value >= 0.0f ? value + 0.5f : value - 0.5f);
    }

    /**
     * Recovered helper: Fixed16FromFloat.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr polygon scan conversion callers that round coordinates into 16.16 fixed point.
     * Purpose: Convert a floating-point value to signed 16.16 fixed-point with symmetric rounding.
     */
    int Fixed16FromFloat(float value)
    {
        const double scaled = (double)(value) * 65536.0;
        return (int)(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
    }

    /**
     * Recovered helper: ScanlineStartFromY.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr polygon scan conversion callers that bias the starting scanline from fixed-point Y.
     * Purpose: Compute the first covered scanline for a polygon edge Y coordinate.
     */
    int ScanlineStartFromY(float y)
    {
        return (Fixed16FromFloat(y) + 0x7fff) >> 16;
    }

    /**
     * Recovered helper: ScanlineEndFromY.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr polygon scan conversion callers that bias the ending scanline from fixed-point Y.
     * Purpose: Compute the last covered scanline for a polygon edge Y coordinate.
     */
    int ScanlineEndFromY(float y)
    {
        return (Fixed16FromFloat(y) - 0x8041) >> 16;
    }

    /**
     * Recovered helper: SpanStartFromX.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr polygon span callers that bias the starting sample from fixed-point X.
     * Purpose: Compute the first covered span sample for an edge X coordinate.
     */
    int SpanStartFromX(float x)
    {
        return (Fixed16FromFloat(x) + 0x7fff) >> 16;
    }

    /**
     * Recovered helper: SpanEndFromX.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr polygon span callers that bias the ending sample from fixed-point X.
     * Purpose: Compute the last covered span sample for an edge X coordinate.
     */
    int SpanEndFromX(float x)
    {
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
    int WrapPolygonIndex(int index, int vertexCount)
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
    int BuildScanConvertEdges(
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
            const zVec3& start = vertices[vertexIndex];
            const zVec3& end = vertices[nextIndex];

            if (sampleY <= end.y) {
                const float dy = end.y - start.y;
                edges[edgeCount].yStart = yStart;
                edges[edgeCount].reserved = 0;
                if (dy != 0.0f) {
                    const float xSlope = (end.x - start.x) / dy;
                    edges[edgeCount].xStepFixed = Fixed16FromFloat(xSlope);
                    edges[edgeCount].currentXFixed = Fixed16FromFloat(start.x + (sampleY - start.y) * xSlope);
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
    Plane2f BuildPlaneFromTriangle(const zVec3* triVerts, const float values[3])
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
    Plane2f BuildScreenPlaneFromTriangle(const zVec3* triVerts, const float values[3])
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
    TexturedPlanes BuildQueuedTexturePlanes(
        const zVec3* clippedTriVerts,
        const zVec3* triVerts,
        const zVec2* triUVs,
        float imageWidth,
        float imageHeight
    )
    {
        gRndr_PerspTexScaledUOverZ0 = imageWidth * triVerts[0].z * triUVs[0].x;
        gRndr_PerspTexScaledVOverZ0 = imageHeight * triVerts[0].z * triUVs[0].y;
        gRndr_PerspTexScaledUOverZ1 = imageWidth * triVerts[1].z * triUVs[1].x;
        gRndr_PerspTexScaledVOverZ1 = imageHeight * triVerts[1].z * triUVs[1].y;
        gRndr_PerspTexScaledUOverZ2 = imageWidth * triVerts[2].z * triUVs[2].x;
        gRndr_PerspTexScaledVOverZ2 = imageHeight * triVerts[2].z * triUVs[2].y;

        const bool useClippedNearPlane = clippedTriVerts != 0
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

        TexturedPlanes planes = { 0 };
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

        const float adjustX = planes.originX - 0.5f;
        const float adjustY = planes.originY - 0.5f;
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
    float EvalPlane(const Plane2f& plane, float x, float y)
    {
        return x * plane.gradient.x + y * plane.gradient.y + plane.base;
    }

    /**
     * Recovered helper: EvalPerspectiveScratchPlane.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr queued texture span callers that consume the gRndr_Persp* scratch bank.
     * Purpose: Evaluate one queued texture scratch plane at the same sample point as the adjusted local planes.
     */
    float EvalPerspectiveScratchPlane(float stepX, float stepY, float base, float x, float y)
    {
        return (x + 0.5f - gRndr_PerspPlaneOriginX) * stepX + (y + 0.5f - gRndr_PerspPlaneOriginY) * stepY + base;
    }

    /**
     * Recovered helper: SelectPerspectiveChunkPixels.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr perspective texture callers that choose adaptive per-span chunk lengths.
     * Purpose: Select the pixel count used for one perspective-correct texture span chunk.
     */
    int SelectPerspectiveChunkPixels(float minPositiveReciprocalZ, float reciprocalZStepX)
    {
        if (zRndr::g_perspectiveAdaptiveMinSpan == 0) {
            return MaxValue(1, zRndr::g_perspectiveTextureDeltaXPow2);
        }

        int chunkPixels = zRndr::g_perspectiveAdaptiveMaxSpan;
        if (reciprocalZStepX != 0.0f) {
            chunkPixels = (int)(fabs(minPositiveReciprocalZ * zRndr::g_perspectiveAdaptiveSlope / reciprocalZStepX));
        }

        chunkPixels = MinValue(chunkPixels, zRndr::g_perspectiveAdaptiveMaxSpan);
        chunkPixels = MaxValue(chunkPixels, zRndr::g_perspectiveAdaptiveMinSpan);
        return MaxValue(1, chunkPixels);
    }

    /**
     * Recovered helper: DispatchTexturedSpanChunks.
     * Original-source helper evidence: No standalone plan entry was found; recovered from
     * zRndr queued texture draw callers that dispatch spans through selected texture callbacks.
     * Purpose: Split one visible span into texture chunks and dispatch each chunk to the span routine.
     */
    void DispatchTexturedSpanChunks(
        zRndr::TexturedQueuedSpanProc spanProc,
        const Plane2f* shadePlane,
        zRndr::SpanNodePartial* span,
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
            if (startInvZ == 0.0f || endInvZ == 0.0f) {
                x += count;
                remaining -= count;
                continue;
            }

            const float startU = EvalPerspectiveScratchPlane(
                                     gRndr_PerspTexScaledUOverZStepX,
                                     gRndr_PerspTexScaledUOverZStepY,
                                     gRndr_PerspTexScaledUOverZBase,
                                     startX,
                                     sampleY
                                 )
                / startInvZ;
            const float startV = EvalPerspectiveScratchPlane(
                                     gRndr_PerspTexScaledVOverZStepX,
                                     gRndr_PerspTexScaledVOverZStepY,
                                     gRndr_PerspTexScaledVOverZBase,
                                     startX,
                                     sampleY
                                 )
                / startInvZ;
            const float endU = EvalPerspectiveScratchPlane(
                                   gRndr_PerspTexScaledUOverZStepX,
                                   gRndr_PerspTexScaledUOverZStepY,
                                   gRndr_PerspTexScaledUOverZBase,
                                   endX,
                                   sampleY
                               )
                / endInvZ;
            const float endV = EvalPerspectiveScratchPlane(
                                   gRndr_PerspTexScaledVOverZStepX,
                                   gRndr_PerspTexScaledVOverZStepY,
                                   gRndr_PerspTexScaledVOverZBase,
                                   endX,
                                   sampleY
                               )
                / endInvZ;

            zRndr::g_spanActiveTexUStepFixed20 = RoundToFixed20((endU - startU) * textureScale / (float)(count));
            zRndr::g_spanActiveTexVStepFixed20 = RoundToFixed20((endV - startV) * textureScale / (float)(count));
            if (shadePlane != 0) {
                const float startShade = MaxValue(0.0f, MinValue(255.0f, EvalPlane(*shadePlane, startX, sampleY)));
                const float endShade = MaxValue(0.0f, MinValue(255.0f, EvalPlane(*shadePlane, endX, sampleY)));
                zRndr::g_spanActiveShadeFixed16 = RoundToFixed20(startShade * 65536.0f);
                zRndr::g_spanActiveShadeStepFixed16
                    = RoundToFixed20((endShade - startShade) * 65536.0f / (float)(count));
            }

            spanProc(RoundToFixed20(startU * textureScale), RoundToFixed20(startV * textureScale), count, texVShift);

            zRndr::g_spanCurrentSpanBaseAddr += count;
            x += count;
            remaining -= count;
        }
    }
} // namespace

// Retail code keeps an EBP frame for this large scan-conversion body under the
// VC5SP3 /O2 profile; disable only frame-pointer omission for the function.
#pragma optimize("y", off)
#pragma optimize("y", on)

namespace zRndr
{
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
    // BN exposes adjacent zero BSS dwords data_57dab8/data_57dabc immediately
    // after the blue mask qword. Same-session BN xrefs show no code or data users,
    // so source leaves them as an unmodeled BSS gap instead of authored span/MMX
    // state.
} // namespace zRndr

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-rasterizepolywithspanlist
 * @recoil-artifact defines .text recoil:function:0x492000: zRndrRasterizePolyWithSpanList
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Rasterize one polygon through the active span-list builder and selected span routine.
 */
void __fastcall zRndrRasterizePolyWithSpanList(zVec3* vertices, zVec3* planeVerts, int vertCount, int spanOpContext)
{
    const float planeZ0 = planeVerts[0].z;
    const float planeZ1 = planeVerts[1].z;
    const float planeZ2 = planeVerts[2].z;
    const float dx10 = planeVerts[0].x - planeVerts[1].x;
    const float dx12 = planeVerts[2].x - planeVerts[1].x;
    const float dy10 = planeVerts[0].y - planeVerts[1].y;
    const float dy12 = planeVerts[2].y - planeVerts[1].y;
    const float determinant = dy12 * dx10 - dy10 * dx12;
    float invDepthSlopeX = determinant;
    float invDepthSlopeY = determinant;
    if (determinant != 0.0f) {
        const float dz10 = planeZ0 - planeZ1;
        const float dz12 = planeZ2 - planeZ1;
        const float inverseDeterminant = 1.0f / determinant;
        invDepthSlopeX = (dy12 * dz10 - dy10 * dz12) * inverseDeterminant;
        invDepthSlopeY = (dx10 * dz12 - dx12 * dz10) * inverseDeterminant;
    }

    const float invDepthBiasBase
        = planeZ0 - (planeVerts[0].x - 0.5f) * invDepthSlopeX - (planeVerts[0].y - 0.5f) * invDepthSlopeY;

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
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[edgeVertexIndex].y);
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
        const zVec3& start = vertices[edgeVertexIndex];
        const zVec3& end = vertices[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }

            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[edgeVertexIndex].y);
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
        const zVec3& start = vertices[edgeVertexIndex];
        const zVec3& end = vertices[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }

            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::SpanNodePartial* visibleSpans[0x141];
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;

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
            zRndr::g_spanAllocCursor->invDepth
                = ((float)(xMin)*invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = invDepthSlopeX;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanList(visibleSpans, y, &spanCount);

            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial* span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                const int byteOffset = (int)(span->sampleXMin) * zRndr::g_bytesPerPixel;
                zRndr::g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + byteOffset);
                zRndr::g_pfnSelectedSpanOp(spanOpContext, pixelCount);
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
    }
}

namespace zRndr
{
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
    void __fastcall SpanOcclusionRasterizeOccluderPoly(SpanOccluderPolyPartial * poly, int vertCount)
    {
        SpanOcclusionRasterScratch scratch;
        int reducedCount = 1;
        scratch.reducedVerts[0].x = poly->vertices[0][0];
        scratch.reducedVerts[0].y = poly->vertices[0][1];

        const float (*sourceVertex)[3] = &poly->vertices[1];
        zVec3* reducedVertex = &scratch.reducedVerts[1];
        int remainingVertices = vertCount - 1;
        while (remainingVertices > 0) {
            reducedVertex->x = (*sourceVertex)[0];
            reducedVertex->y = (*sourceVertex)[1];
            if (reducedVertex->x != (reducedVertex - 1)->x || reducedVertex->y != (reducedVertex - 1)->y) {
                ++reducedCount;
                ++reducedVertex;
            }
            ++sourceVertex;
            --remainingVertices;
        }

        if (reducedCount > 1 && scratch.reducedVerts[reducedCount - 1].x == scratch.reducedVerts[0].x
            && scratch.reducedVerts[reducedCount - 1].y == scratch.reducedVerts[0].y) {
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
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, scratch.reducedVerts[edgeVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            while (edgeVertexIndex != bottomVertexIndex) {
                int nextIndex = edgeVertexIndex + 1;
                if (nextIndex >= reducedCount) {
                    nextIndex -= reducedCount;
                }
                const zVec3& start = scratch.reducedVerts[edgeVertexIndex];
                const zVec3& end = scratch.reducedVerts[nextIndex];
                if (edgeSampleY <= end.y) {
                    const float dy = end.y - start.y;
                    edgeTableA[edgeCountA].yStart = edgeYStart;
                    edgeTableA[edgeCountA].reserved = 0;
                    if (dy != 0.0f) {
                        const float xSlope = (end.x - start.x) / dy;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, xSlope);
                        ZRNDR_SET_FIXED16_FROM_FLOAT(
                            edgeTableA[edgeCountA].currentXFixed,
                            start.x + (edgeSampleY - start.y) * xSlope
                        );
                    } else {
                        edgeTableA[edgeCountA].xStepFixed = 0;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
                    }

                    ++edgeCountA;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            }

            edgeVertexIndex = topVertexIndex;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, scratch.reducedVerts[edgeVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            while (edgeVertexIndex != bottomVertexIndex) {
                int nextIndex = edgeVertexIndex - 1;
                if (nextIndex < 0) {
                    nextIndex += reducedCount;
                }
                const zVec3& start = scratch.reducedVerts[edgeVertexIndex];
                const zVec3& end = scratch.reducedVerts[nextIndex];
                if (edgeSampleY <= end.y) {
                    const float dy = end.y - start.y;
                    edgeTableB[edgeCountB].yStart = edgeYStart;
                    edgeTableB[edgeCountB].reserved = 0;
                    if (dy != 0.0f) {
                        const float xSlope = (end.x - start.x) / dy;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, xSlope);
                        ZRNDR_SET_FIXED16_FROM_FLOAT(
                            edgeTableB[edgeCountB].currentXFixed,
                            start.x + (edgeSampleY - start.y) * xSlope
                        );
                    } else {
                        edgeTableB[edgeCountB].xStepFixed = 0;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
                    }

                    ++edgeCountB;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            }
        } else {
            edgeVertexIndex = topVertexIndex;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, scratch.reducedVerts[edgeVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            while (edgeVertexIndex != bottomVertexIndex) {
                int nextIndex = edgeVertexIndex + 1;
                if (nextIndex >= reducedCount) {
                    nextIndex -= reducedCount;
                }
                const zVec3& start = scratch.reducedVerts[edgeVertexIndex];
                const zVec3& end = scratch.reducedVerts[nextIndex];
                if (edgeSampleY <= end.y) {
                    const float dy = end.y - start.y;
                    edgeTableB[edgeCountB].yStart = edgeYStart;
                    edgeTableB[edgeCountB].reserved = 0;
                    if (dy != 0.0f) {
                        const float xSlope = (end.x - start.x) / dy;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, xSlope);
                        ZRNDR_SET_FIXED16_FROM_FLOAT(
                            edgeTableB[edgeCountB].currentXFixed,
                            start.x + (edgeSampleY - start.y) * xSlope
                        );
                    } else {
                        edgeTableB[edgeCountB].xStepFixed = 0;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
                    }

                    ++edgeCountB;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            }

            edgeVertexIndex = topVertexIndex;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, scratch.reducedVerts[edgeVertexIndex].y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
            while (edgeVertexIndex != bottomVertexIndex) {
                int nextIndex = edgeVertexIndex - 1;
                if (nextIndex < 0) {
                    nextIndex += reducedCount;
                }
                const zVec3& start = scratch.reducedVerts[edgeVertexIndex];
                const zVec3& end = scratch.reducedVerts[nextIndex];
                if (edgeSampleY <= end.y) {
                    const float dy = end.y - start.y;
                    edgeTableA[edgeCountA].yStart = edgeYStart;
                    edgeTableA[edgeCountA].reserved = 0;
                    if (dy != 0.0f) {
                        const float xSlope = (end.x - start.x) / dy;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, xSlope);
                        ZRNDR_SET_FIXED16_FROM_FLOAT(
                            edgeTableA[edgeCountA].currentXFixed,
                            start.x + (edgeSampleY - start.y) * xSlope
                        );
                    } else {
                        edgeTableA[edgeCountA].xStepFixed = 0;
                        ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
                    }

                    ++edgeCountA;
                    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
                    edgeYStart = (fixed16Value + 0x7fff) >> 16;
                    edgeSampleY = (float)(edgeYStart) + 0.5f;
                }
                edgeVertexIndex = nextIndex;
            }
        }

        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, scratch.reducedVerts[topVertexIndex].y);
        const int firstScanline = (fixed16Value + 0x7fff) >> 16;
        ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, scratch.reducedVerts[bottomVertexIndex].y);
        const int lastScanline = (fixed16Value - 0x8041) >> 16;
        if (firstScanline > lastScanline) {
            return;
        }

        SpanNodePartial** spanList = scratch.spanList;
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
                g_pfnBuildSpanList(spanList, y, &spanCount);
            }
        }
    }
} // namespace zRndr

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawflatimmediate
 * @recoil-artifact defines .text recoil:function:0x492f00: zRndrDrawFlatImmediate
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Draw an immediate flat polygon through the flat span callback path.
 */
void __fastcall
zRndrDrawFlatImmediate(zVec3* vertices, zVec3* planeVertices, int vertCount, int flatSpanOpEdxArg, int flatSpanOpEcxArg)
{
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

    const float invDepthBiasBase = planeVertices[0].z - (planeVertices[0].x - 0.5f) * invDepthSlopeX
        - (planeVertices[0].y - 0.5f) * invDepthSlopeY;

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

    ScanConvertEdge edgeTableA[0x40] = { 0 };
    ScanConvertEdge edgeTableB[0x40] = { 0 };
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
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[edgeVertexIndex].y);
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
        const zVec3& start = vertices[edgeVertexIndex];
        const zVec3& end = vertices[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }

            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[edgeVertexIndex].y);
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
        const zVec3& start = vertices[edgeVertexIndex];
        const zVec3& end = vertices[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }

            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, vertices[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::SpanNodePartial* visibleSpans[0x40];
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;

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
            zRndr::g_spanAllocCursor->invDepth
                = ((float)(xMin)*invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*invDepthSlopeX + rowDepthBase) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = invDepthSlopeX;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanListSecondary(visibleSpans, y, &spanCount);

            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial* span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                const int byteOffset = (int)(span->sampleXMin) * zRndr::g_bytesPerPixel;
                zRndr::g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + byteOffset);
                zRndr::g_pfnFlatImmediateSpanOp(flatSpanOpEcxArg, flatSpanOpEdxArg, pixelCount);
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
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
    int reducedCount = 1;
    reducedVerts[0].x = vertices[0].x;
    reducedVerts[0].y = vertices[0].y;
    for (int reduceIndex = 1; reduceIndex < vertCount && reducedCount < 0x40; ++reduceIndex) {
        const zVec3& previous = reducedVerts[reducedCount - 1];
        reducedVerts[reducedCount].x = vertices[reduceIndex].x;
        reducedVerts[reducedCount].y = vertices[reduceIndex].y;
        if (reducedVerts[reducedCount].x == previous.x && reducedVerts[reducedCount].y == previous.y) {
            continue;
        }

        ++reducedCount;
    }

    if (reducedCount > 1 && reducedVerts[reducedCount - 1].x == reducedVerts[0].x
        && reducedVerts[reducedCount - 1].y == reducedVerts[0].y) {
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

    ScanConvertEdge edgeTableA[0x40] = { 0 };
    ScanConvertEdge edgeTableB[0x40] = { 0 };
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
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepA;
        if (nextIndex < 0) {
            nextIndex += reducedCount;
        }
        if (nextIndex >= reducedCount) {
            nextIndex -= reducedCount;
        }
        const zVec3& start = reducedVerts[edgeVertexIndex];
        const zVec3& end = reducedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }

            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepB;
        if (nextIndex < 0) {
            nextIndex += reducedCount;
        }
        if (nextIndex >= reducedCount) {
            nextIndex -= reducedCount;
        }
        const zVec3& start = reducedVerts[edgeVertexIndex];
        const zVec3& end = reducedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }

            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, reducedVerts[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;

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
                zRndr::g_spanCurrentSpanBaseAddr = (unsigned short*)(scanlineBase + xStart * zRndr::g_bytesPerPixel);
                zRndr::g_pfnSelectedSpanOp(spanOpContext, pixelCount);
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
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
    zVidImagePartial* selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    gRndr_PerspTexScaledUOverZ0 = imageWidth * triVerts[0].z * triUVs[0].x;
    gRndr_PerspTexScaledVOverZ0 = imageHeight * triVerts[0].z * triUVs[0].y;
    gRndr_PerspTexScaledUOverZ1 = imageWidth * triVerts[1].z * triUVs[1].x;
    gRndr_PerspTexScaledVOverZ1 = imageHeight * triVerts[1].z * triUVs[1].y;
    gRndr_PerspTexScaledUOverZ2 = imageWidth * triVerts[2].z * triUVs[2].x;
    gRndr_PerspTexScaledVOverZ2 = imageHeight * triVerts[2].z * triUVs[2].y;

    const float reciprocalValues[3] = { triVerts[0].z, triVerts[1].z, triVerts[2].z };
    const float uValues[3] = { gRndr_PerspTexScaledUOverZ0, gRndr_PerspTexScaledUOverZ1, gRndr_PerspTexScaledUOverZ2 };
    const float vValues[3] = { gRndr_PerspTexScaledVOverZ0, gRndr_PerspTexScaledVOverZ1, gRndr_PerspTexScaledVOverZ2 };
    const float planeDx10 = triVerts[0].x - triVerts[1].x;
    const float planeDx12 = triVerts[2].x - triVerts[1].x;
    const float planeDy10 = triVerts[0].y - triVerts[1].y;
    const float planeDy12 = triVerts[2].y - triVerts[1].y;
    const float planeDeterminant = planeDy12 * planeDx10 - planeDy10 * planeDx12;
    Plane2f reciprocalZ = { 0 };
    Plane2f uOverZ = { 0 };
    Plane2f vOverZ = { 0 };
    if (planeDeterminant != 0.0f) {
        const float inversePlaneDeterminant = -1.0f / planeDeterminant;
        const float reciprocal10 = reciprocalValues[0] - reciprocalValues[1];
        const float reciprocal12 = reciprocalValues[2] - reciprocalValues[1];
        const float u10 = uValues[0] - uValues[1];
        const float u12 = uValues[2] - uValues[1];
        const float v10 = vValues[0] - vValues[1];
        const float v12 = vValues[2] - vValues[1];
        reciprocalZ.gradient.x = (planeDy12 * reciprocal10 - planeDy10 * reciprocal12) * inversePlaneDeterminant;
        reciprocalZ.gradient.y = (planeDx10 * reciprocal12 - planeDx12 * reciprocal10) * inversePlaneDeterminant;
        uOverZ.gradient.x = (planeDy12 * u10 - planeDy10 * u12) * inversePlaneDeterminant;
        uOverZ.gradient.y = (planeDx10 * u12 - planeDx12 * u10) * inversePlaneDeterminant;
        vOverZ.gradient.x = (planeDy12 * v10 - planeDy10 * v12) * inversePlaneDeterminant;
        vOverZ.gradient.y = (planeDx10 * v12 - planeDx12 * v10) * inversePlaneDeterminant;
    }
    reciprocalZ.base = reciprocalValues[0];
    uOverZ.base = uValues[0];
    vOverZ.base = vValues[0];
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

    TexturedPlanes planes = { 0 };
    planes.reciprocalZ = reciprocalZ;
    planes.uOverZ = uOverZ;
    planes.vOverZ = vOverZ;
    planes.originX = gRndr_PerspPlaneOriginX;
    planes.originY = gRndr_PerspPlaneOriginY;
    const float adjustX = planes.originX - 0.5f;
    const float adjustY = planes.originY - 0.5f;
    planes.reciprocalZ.base -= adjustX * planes.reciprocalZ.gradient.x + adjustY * planes.reciprocalZ.gradient.y;
    planes.uOverZ.base -= adjustX * planes.uOverZ.gradient.x + adjustY * planes.uOverZ.gradient.y;
    planes.vOverZ.base -= adjustX * planes.vOverZ.gradient.x + adjustY * planes.vOverZ.gradient.y;

    if (entry->nextVariant != 0) {
        selectedImage = zRndrTextureMipSelectVariantImage(
            entry,
            triVerts,
            3,
            (const zVec2*)(&gRndr_PerspTexScaledUOverZ0),
            (const zVec2*)(&gRndr_PerspInvDepthStepX),
            (const zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
            (const zVec2*)(&gRndr_PerspTexScaledVOverZStepX)
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

    ScanConvertEdge edgeTableA[0x40] = { 0 };
    ScanConvertEdge edgeTableB[0x40] = { 0 };
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
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[edgeVertexIndex].y);
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
        const zVec3& start = polyVerts[edgeVertexIndex];
        const zVec3& end = polyVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }

            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[edgeVertexIndex].y);
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
        const zVec3& start = polyVerts[edgeVertexIndex];
        const zVec3& end = polyVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }

            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char*)(selectedImage->pixels);

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
    unsigned short* palette = (unsigned short*)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette = paletteIndex == -1 ? palette : &palette[(paletteIndex + 1) * 0x100];
        spanProc = zRndr::g_pfnSelectedSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;

    zRndr::SpanNodePartial* visibleSpans[0x40] = { 0 };
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;
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
            const float rowReciprocalZ = (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth
                = ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanListSecondary(visibleSpans, y, &spanCount);
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial* span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr
                        = (unsigned short*)(scanlineBase + (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    const int count = span->sampleXMax - span->sampleXMin + 1;
                    const float startX = (float)(span->sampleXMin);
                    const float endX = (float)(span->sampleXMin + count);
                    const float sampleY = (float)(y);
                    const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                    const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                    const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                    const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                        + gRndr_PerspInvDepthBase;
                    const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                        + gRndr_PerspInvDepthBase;
                    if (startInvZ != 0.0f && endInvZ != 0.0f) {
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const float texUStep = (endU - startU) * 1048576.0f / (float)(count);
                        const float texVStep = (endV - startV) * 1048576.0f / (float)(count);
                        const float texUStart = startU * 1048576.0f;
                        const float texVStart = startV * 1048576.0f;
                        const double texUStepBits = (double)(texUStep) - -6755399441055744.0;
                        const double texVStepBits = (double)(texVStep) - -6755399441055744.0;
                        const double texUStartBits = (double)(texUStart) - -6755399441055744.0;
                        const double texVStartBits = (double)(texVStart) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&texUStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&texVStepBits);
                        spanProc(*(const int*)(&texUStartBits), *(const int*)(&texVStartBits), count, texVShift);
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                    }
                }
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
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
    if (entry == 0 || entry->image == 0 || polyVerts == 0 || triVerts == 0 || triUVs == 0 || vertexCount <= 0
        || zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial* selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    gRndr_PerspTexScaledUOverZ0 = imageWidth * triVerts[0].z * triUVs[0].x;
    gRndr_PerspTexScaledVOverZ0 = imageHeight * triVerts[0].z * triUVs[0].y;
    gRndr_PerspTexScaledUOverZ1 = imageWidth * triVerts[1].z * triUVs[1].x;
    gRndr_PerspTexScaledVOverZ1 = imageHeight * triVerts[1].z * triUVs[1].y;
    gRndr_PerspTexScaledUOverZ2 = imageWidth * triVerts[2].z * triUVs[2].x;
    gRndr_PerspTexScaledVOverZ2 = imageHeight * triVerts[2].z * triUVs[2].y;

    const float reciprocalValues[3] = { triVerts[0].z, triVerts[1].z, triVerts[2].z };
    const float uValues[3] = { gRndr_PerspTexScaledUOverZ0, gRndr_PerspTexScaledUOverZ1, gRndr_PerspTexScaledUOverZ2 };
    const float vValues[3] = { gRndr_PerspTexScaledVOverZ0, gRndr_PerspTexScaledVOverZ1, gRndr_PerspTexScaledVOverZ2 };
    const float planeDx10 = triVerts[0].x - triVerts[1].x;
    const float planeDx12 = triVerts[2].x - triVerts[1].x;
    const float planeDy10 = triVerts[0].y - triVerts[1].y;
    const float planeDy12 = triVerts[2].y - triVerts[1].y;
    const float planeDeterminant = planeDy12 * planeDx10 - planeDy10 * planeDx12;
    Plane2f reciprocalZ = { 0 };
    Plane2f uOverZ = { 0 };
    Plane2f vOverZ = { 0 };
    if (planeDeterminant != 0.0f) {
        const float inversePlaneDeterminant = -1.0f / planeDeterminant;
        const float reciprocal10 = reciprocalValues[0] - reciprocalValues[1];
        const float reciprocal12 = reciprocalValues[2] - reciprocalValues[1];
        const float u10 = uValues[0] - uValues[1];
        const float u12 = uValues[2] - uValues[1];
        const float v10 = vValues[0] - vValues[1];
        const float v12 = vValues[2] - vValues[1];
        reciprocalZ.gradient.x = (planeDy12 * reciprocal10 - planeDy10 * reciprocal12) * inversePlaneDeterminant;
        reciprocalZ.gradient.y = (planeDx10 * reciprocal12 - planeDx12 * reciprocal10) * inversePlaneDeterminant;
        uOverZ.gradient.x = (planeDy12 * u10 - planeDy10 * u12) * inversePlaneDeterminant;
        uOverZ.gradient.y = (planeDx10 * u12 - planeDx12 * u10) * inversePlaneDeterminant;
        vOverZ.gradient.x = (planeDy12 * v10 - planeDy10 * v12) * inversePlaneDeterminant;
        vOverZ.gradient.y = (planeDx10 * v12 - planeDx12 * v10) * inversePlaneDeterminant;
    }
    reciprocalZ.base = reciprocalValues[0];
    uOverZ.base = uValues[0];
    vOverZ.base = vValues[0];
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

    TexturedPlanes planes = { 0 };
    planes.reciprocalZ = reciprocalZ;
    planes.uOverZ = uOverZ;
    planes.vOverZ = vOverZ;
    planes.originX = gRndr_PerspPlaneOriginX;
    planes.originY = gRndr_PerspPlaneOriginY;
    const float adjustX = planes.originX - 0.5f;
    const float adjustY = planes.originY - 0.5f;
    planes.reciprocalZ.base -= adjustX * planes.reciprocalZ.gradient.x + adjustY * planes.reciprocalZ.gradient.y;
    planes.uOverZ.base -= adjustX * planes.uOverZ.gradient.x + adjustY * planes.uOverZ.gradient.y;
    planes.vOverZ.base -= adjustX * planes.vOverZ.gradient.x + adjustY * planes.vOverZ.gradient.y;

    if (entry->nextVariant != 0) {
        selectedImage = zRndrTextureMipSelectVariantImage(
            entry,
            triVerts,
            3,
            (const zVec2*)(&gRndr_PerspTexScaledUOverZ0),
            (const zVec2*)(&gRndr_PerspInvDepthStepX),
            (const zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
            (const zVec2*)(&gRndr_PerspTexScaledVOverZStepX)
        );
        if (selectedImage == 0) {
            return;
        }
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

    ScanConvertEdge edgeTableA[0x40] = { 0 };
    ScanConvertEdge edgeTableB[0x40] = { 0 };
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
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepA;
        if (nextIndex < 0) {
            nextIndex += vertexCount;
        }
        if (nextIndex >= vertexCount) {
            nextIndex -= vertexCount;
        }
        const zVec3& start = polyVerts[edgeVertexIndex];
        const zVec3& end = polyVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }
            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepB;
        if (nextIndex < 0) {
            nextIndex += vertexCount;
        }
        if (nextIndex >= vertexCount) {
            nextIndex -= vertexCount;
        }
        const zVec3& start = polyVerts[edgeVertexIndex];
        const zVec3& end = polyVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float xSlope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, xSlope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * xSlope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }
            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    if (edgeCountA == 0 || edgeCountB == 0) {
        return;
    }

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, polyVerts[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char*)(selectedImage->pixels);

    zRndr::TexturedQueuedSpanProc spanProc = 0;
    zRndr::TexturedQueuedSpanProc paletteSpanProc = 0;
    if (selectedImage->alphaMap != 0) {
        zRndr::g_spanActiveTexAlphaMap = selectedImage->alphaMap;
        int alphaBits = 0;
        memcpy(&alphaBits, &alpha, sizeof(alphaBits));
        zRndr::g_spanActiveConstAlphaBits = alphaBits;
        spanProc = zRndr::g_pfnPolyTlvSpanOp_Mode0;
        paletteSpanProc = zRndr::g_pfnPolyTlvSpanOpAlt_Mode0;
    } else {
        zRndr::g_spanActiveTexAlphaMap = 0;
        const double alphaScaled = (double)(alpha) * 255.0;
        const double alphaFixedBits = alphaScaled - -6755399441055744.0;
        zRndr::g_spanActiveConstAlphaBits = *(const int*)(&alphaFixedBits);
        spanProc = zRndr::g_pfnPolyTlvSpanOp_Mode1;
        paletteSpanProc = zRndr::g_pfnPolyTlvSpanOpAlt_Mode1;
    }
    zRndr::g_pfnSelectedSpanOp_Mode0 = spanProc;
    zRndr::g_pfnSelectedSpanOp_Mode1 = paletteSpanProc;
    spanProc = zRndr::g_pfnSelectedSpanOp_Mode0;

    unsigned short* palette = (unsigned short*)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette = texKey == -1 ? palette : &palette[(texKey + 1) * 0x100];
        spanProc = zRndr::g_pfnSelectedSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;

    zRndr::SpanNodePartial* visibleSpans[0x40] = { 0 };
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;
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
            const float rowReciprocalZ = (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth
                = ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanListSecondary(visibleSpans, y, &spanCount);
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial* span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr
                        = (unsigned short*)(scanlineBase + (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    const int count = span->sampleXMax - span->sampleXMin + 1;
                    const float startX = (float)(span->sampleXMin);
                    const float endX = (float)(span->sampleXMin + count);
                    const float sampleY = (float)(y);
                    const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                    const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                    const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                    const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                        + gRndr_PerspInvDepthBase;
                    const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                        + gRndr_PerspInvDepthBase;
                    if (startInvZ != 0.0f && endInvZ != 0.0f) {
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const float texUStep = (endU - startU) * 1048576.0f / (float)(count);
                        const float texVStep = (endV - startV) * 1048576.0f / (float)(count);
                        const float texUStart = startU * 1048576.0f;
                        const float texVStart = startV * 1048576.0f;
                        const double texUStepBits = (double)(texUStep) - -6755399441055744.0;
                        const double texVStepBits = (double)(texVStep) - -6755399441055744.0;
                        const double texUStartBits = (double)(texUStart) - -6755399441055744.0;
                        const double texVStartBits = (double)(texVStart) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&texUStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&texVStepBits);
                        spanProc(*(const int*)(&texUStartBits), *(const int*)(&texVStartBits), count, texVShift);
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                    }
                }
            }
        }

        scanlineBase += zRndr::g_pitchBytes;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawtexturedqueued
 * @recoil-artifact defines .text recoil:function:0x495850: zRndrDrawTexturedQueued
 *
 *
 * Purpose: Draw a depth-sorted textured polygon using perspective-correct queued spans.
 */
void __fastcall zRndrDrawTexturedQueued(
    zImage_TexDirEntryPartial* entry,
    zVec3* projectedVerts,
    zVec3* clippedTriVerts,
    zVec3* triVerts,
    zVec2* triUVs,
    zVec3* shadeTriplet,
    int vertCount,
    int,
    int texKey
)
{
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 || triVerts == 0 || triUVs == 0 || shadeTriplet == 0
        || vertCount <= 0 || zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial* selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    gRndr_PerspTexScaledUOverZ0 = imageWidth * triVerts[0].z * triUVs[0].x;
    gRndr_PerspTexScaledVOverZ0 = imageHeight * triVerts[0].z * triUVs[0].y;
    gRndr_PerspTexScaledUOverZ1 = imageWidth * triVerts[1].z * triUVs[1].x;
    gRndr_PerspTexScaledVOverZ1 = imageHeight * triVerts[1].z * triUVs[1].y;
    gRndr_PerspTexScaledUOverZ2 = imageWidth * triVerts[2].z * triUVs[2].x;
    gRndr_PerspTexScaledVOverZ2 = imageHeight * triVerts[2].z * triUVs[2].y;

    const bool useClippedNearPlane = clippedTriVerts != 0
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
        const float dx10 = triVerts[0].x - triVerts[1].x;
        const float dx12 = triVerts[2].x - triVerts[1].x;
        const float dy10 = triVerts[0].y - triVerts[1].y;
        const float dy12 = triVerts[2].y - triVerts[1].y;
        const float determinant = dy12 * dx10 - dy10 * dx12;
        Plane2f reciprocalZ = { 0 };
        Plane2f uOverZ = { 0 };
        Plane2f vOverZ = { 0 };
        if (determinant != 0.0f) {
            const float inverseDeterminant = -1.0f / determinant;
            const float reciprocal10 = reciprocalValues[0] - reciprocalValues[1];
            const float reciprocal12 = reciprocalValues[2] - reciprocalValues[1];
            const float u10 = uValues[0] - uValues[1];
            const float u12 = uValues[2] - uValues[1];
            const float v10 = vValues[0] - vValues[1];
            const float v12 = vValues[2] - vValues[1];
            reciprocalZ.gradient.x = (dy12 * reciprocal10 - dy10 * reciprocal12) * inverseDeterminant;
            reciprocalZ.gradient.y = (dx10 * reciprocal12 - dx12 * reciprocal10) * inverseDeterminant;
            uOverZ.gradient.x = (dy12 * u10 - dy10 * u12) * inverseDeterminant;
            uOverZ.gradient.y = (dx10 * u12 - dx12 * u10) * inverseDeterminant;
            vOverZ.gradient.x = (dy12 * v10 - dy10 * v12) * inverseDeterminant;
            vOverZ.gradient.y = (dx10 * v12 - dx12 * v10) * inverseDeterminant;
        }
        gRndr_PerspInvDepthStepX = reciprocalZ.gradient.x;
        gRndr_PerspInvDepthStepY = reciprocalZ.gradient.y;
        gRndr_PerspInvDepthBase = reciprocalValues[0];
        gRndr_PerspTexScaledUOverZStepX = uOverZ.gradient.x;
        gRndr_PerspTexScaledUOverZStepY = uOverZ.gradient.y;
        gRndr_PerspTexScaledUOverZBase = uValues[0];
        gRndr_PerspTexScaledVOverZStepX = vOverZ.gradient.x;
        gRndr_PerspTexScaledVOverZStepY = vOverZ.gradient.y;
        gRndr_PerspTexScaledVOverZBase = vValues[0];
        gRndr_PerspPlaneOriginX = triVerts[0].x;
        gRndr_PerspPlaneOriginY = triVerts[0].y;
    }

    TexturedPlanes planes = { 0 };
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
    const float adjustX = planes.originX - 0.5f;
    const float adjustY = planes.originY - 0.5f;
    planes.reciprocalZ.base -= adjustX * planes.reciprocalZ.gradient.x + adjustY * planes.reciprocalZ.gradient.y;
    planes.uOverZ.base -= adjustX * planes.uOverZ.gradient.x + adjustY * planes.uOverZ.gradient.y;
    planes.vOverZ.base -= adjustX * planes.vOverZ.gradient.x + adjustY * planes.vOverZ.gradient.y;

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndrTextureMipSelectVariantImage(
            entry,
            triVerts,
            3,
            (const zVec2*)(&gRndr_PerspTexScaledUOverZ0),
            (const zVec2*)(&gRndr_PerspInvDepthStepX),
            (const zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
            (const zVec2*)(&gRndr_PerspTexScaledVOverZStepX)
        );
        if (selectedImage == 0) {
            return;
        }
        textureScale = 1048576.0f / (selectedImage->widthScale != 0.0f ? selectedImage->widthScale : 1.0f);
    }

    const float shadeValues[3] = {
        shadeTriplet->x,
        shadeTriplet->y,
        shadeTriplet->z,
    };
    const float shadeDx10 = projectedVerts[0].x - projectedVerts[1].x;
    const float shadeDx12 = projectedVerts[2].x - projectedVerts[1].x;
    const float shadeDy10 = projectedVerts[0].y - projectedVerts[1].y;
    const float shadeDy12 = projectedVerts[2].y - projectedVerts[1].y;
    const float shadeDeterminant = shadeDy12 * shadeDx10 - shadeDy10 * shadeDx12;
    Plane2f shadePlane = { 0 };
    if (shadeDeterminant != 0.0f) {
        const float inverseShadeDeterminant = -1.0f / shadeDeterminant;
        const float shade10 = shadeValues[0] - shadeValues[1];
        const float shade12 = shadeValues[2] - shadeValues[1];
        shadePlane.gradient.x = (shadeDy12 * shade10 - shadeDy10 * shade12) * inverseShadeDeterminant;
        shadePlane.gradient.y = (shadeDx10 * shade12 - shadeDx12 * shade10) * inverseShadeDeterminant;
    }
    shadePlane.base
        = shadeValues[0] - projectedVerts[0].x * shadePlane.gradient.x - projectedVerts[0].y * shadePlane.gradient.y;

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    float minPositiveReciprocalZ = 1000.0f;
    for (int i_4544 = 0; i_4544 < vertCount; ++i_4544) {
        const float reciprocalZ = projectedVerts[i_4544].x * planes.reciprocalZ.gradient.x
            + projectedVerts[i_4544].y * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
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

    ScanConvertEdge edgeTableA[0x40] = { 0 };
    ScanConvertEdge edgeTableB[0x40] = { 0 };
    int edgeCountA = 0;
    int edgeCountB = 0;
    int fixed16Value;
    int edgeVertexIndex;
    int edgeYStart;
    float edgeSampleY;
    const int edgeStepA = zRndr::g_scanConvertMode != 0 ? 1 : -1;
    const int edgeStepB = -edgeStepA;

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepA;
        if (nextIndex < 0)
            nextIndex += vertCount;
        if (nextIndex >= vertCount)
            nextIndex -= vertCount;
        const zVec3& start = projectedVerts[edgeVertexIndex];
        const zVec3& end = projectedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float slope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, slope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * slope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }
            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepB;
        if (nextIndex < 0)
            nextIndex += vertCount;
        if (nextIndex >= vertCount)
            nextIndex -= vertCount;
        const zVec3& start = projectedVerts[edgeVertexIndex];
        const zVec3& end = projectedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float slope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, slope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * slope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }
            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }
    if (edgeCountA == 0 || edgeCountB == 0)
        return;

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char*)(selectedImage->pixels);
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode0;
    Plane2f* activeShadePlane = 0;

    unsigned short* palette = (unsigned short*)(selectedImage->palette);
    if (palette != 0) {
        spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode1;
        if (texKey != -1) {
            zRndr::g_spanActiveTexPalette = &palette[(texKey + 1) * 0x100];
        } else {
            int shadeRecipe = g_zRndr_ActivePaletteShadeRecipeIndex;
            if (shadeRecipe < 0) {
                shadeRecipe = zVidPaletteRemapFindRecipeIndexFromRgb((zColorRgb*)(zRndr::g_fogParamsActive.colorRgb01));
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

    zRndr::SpanNodePartial* visibleSpans[0x40] = { 0 };
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;
    int chunkPixels;
    if (zRndr::g_perspectiveAdaptiveMinSpan == 0) {
        chunkPixels = zRndr::g_perspectiveTextureDeltaXPow2;
    } else {
        float selectedChunk = (float)(zRndr::g_perspectiveAdaptiveMaxSpan);
        if (planes.reciprocalZ.gradient.x != 0.0f) {
            selectedChunk = minPositiveReciprocalZ * zRndr::g_perspectiveAdaptiveSlope / planes.reciprocalZ.gradient.x;
            if (selectedChunk < 0.0f)
                selectedChunk = -selectedChunk;
        }
        const double selectedChunkBits = (double)(selectedChunk) - -6755399441055744.0;
        chunkPixels = *(const int*)(&selectedChunkBits);
        if (chunkPixels > zRndr::g_perspectiveAdaptiveMaxSpan) {
            chunkPixels = zRndr::g_perspectiveAdaptiveMaxSpan;
        }
        if (chunkPixels < zRndr::g_perspectiveAdaptiveMinSpan) {
            chunkPixels = zRndr::g_perspectiveAdaptiveMinSpan;
        }
    }
    if (chunkPixels < 1)
        chunkPixels = 1;
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

            const float rowReciprocalZ = (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth
                = ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanList(visibleSpans, y, &spanCount);
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial* span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr
                        = (unsigned short*)(scanlineBase + (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    int remaining = span->sampleXMax - span->sampleXMin + 1;
                    int x = span->sampleXMin;
                    while (remaining > chunkPixels) {
                        const int count = chunkPixels;
                        const float startX = (float)(x);
                        const float endX = (float)(x + count);
                        const float sampleY = (float)(y);
                        const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                        const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX
                            + planeY * gRndr_PerspInvDepthStepY + gRndr_PerspInvDepthBase;
                        const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                            + gRndr_PerspInvDepthBase;
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const double uStepBits
                            = (double)((endU - startU) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double vStepBits
                            = (double)((endV - startV) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                        const double vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&uStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&vStepBits);
                        if (activeShadePlane != 0) {
                            float startShade = startX * activeShadePlane->gradient.x
                                + sampleY * activeShadePlane->gradient.y + activeShadePlane->base;
                            float endShade = endX * activeShadePlane->gradient.x
                                + sampleY * activeShadePlane->gradient.y + activeShadePlane->base;
                            if (startShade < 0.0f)
                                startShade = 0.0f;
                            if (startShade > 255.0f)
                                startShade = 255.0f;
                            if (endShade < 0.0f)
                                endShade = 0.0f;
                            if (endShade > 255.0f)
                                endShade = 255.0f;
                            const double shadeStartBits = (double)(startShade * 65536.0f) - -6755399441055744.0;
                            const double shadeStepBits
                                = (double)((endShade - startShade) * 65536.0f / (float)(count)) - -6755399441055744.0;
                            zRndr::g_spanActiveShadeFixed16 = *(const int*)(&shadeStartBits);
                            zRndr::g_spanActiveShadeStepFixed16 = *(const int*)(&shadeStepBits);
                        }
                        if (spanProc != zRndr::SpanShade16FromPal8SwitchVShift) {
                            spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                            ((void(__fastcall*)(unsigned short*, int, int, int))(zRndr::g_pfnTexturedQueuedFinalize))(
                                zRndr::g_spanCurrentSpanBaseAddr,
                                count,
                                zRndr::g_spanActiveShadeFixed16,
                                zRndr::g_spanActiveShadeStepFixed16
                            );
                        } else {
                            spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                        }
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                        x += count;
                        remaining -= count;
                    }

                    if (remaining > 0) {
                        const int count = remaining;
                        const float startX = (float)(x);
                        const float endX = (float)(x + count);
                        const float sampleY = (float)(y);
                        const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                        const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX
                            + planeY * gRndr_PerspInvDepthStepY + gRndr_PerspInvDepthBase;
                        const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                            + gRndr_PerspInvDepthBase;
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const double uStepBits
                            = (double)((endU - startU) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double vStepBits
                            = (double)((endV - startV) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                        const double vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&uStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&vStepBits);
                        if (activeShadePlane != 0) {
                            float startShade = startX * activeShadePlane->gradient.x
                                + sampleY * activeShadePlane->gradient.y + activeShadePlane->base;
                            float endShade = endX * activeShadePlane->gradient.x
                                + sampleY * activeShadePlane->gradient.y + activeShadePlane->base;
                            if (startShade < 0.0f)
                                startShade = 0.0f;
                            if (startShade > 255.0f)
                                startShade = 255.0f;
                            if (endShade < 0.0f)
                                endShade = 0.0f;
                            if (endShade > 255.0f)
                                endShade = 255.0f;
                            const double shadeStartBits = (double)(startShade * 65536.0f) - -6755399441055744.0;
                            const double shadeStepBits
                                = (double)((endShade - startShade) * 65536.0f / (float)(count)) - -6755399441055744.0;
                            zRndr::g_spanActiveShadeFixed16 = *(const int*)(&shadeStartBits);
                            zRndr::g_spanActiveShadeStepFixed16 = *(const int*)(&shadeStepBits);
                        }
                        if (spanProc != zRndr::SpanShade16FromPal8SwitchVShift) {
                            spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                            ((void(__fastcall*)(unsigned short*, int, int, int))(zRndr::g_pfnTexturedQueuedFinalize))(
                                zRndr::g_spanCurrentSpanBaseAddr,
                                count,
                                zRndr::g_spanActiveShadeFixed16,
                                zRndr::g_spanActiveShadeStepFixed16
                            );
                        } else {
                            spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                        }
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                    }
                }
            }
        }
        scanlineBase += zRndr::g_pitchBytes;
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
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 || triVerts == 0 || triUVs == 0 || vertCount <= 0
        || zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial* selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    gRndr_PerspTexScaledUOverZ0 = imageWidth * triVerts[0].z * triUVs[0].x;
    gRndr_PerspTexScaledVOverZ0 = imageHeight * triVerts[0].z * triUVs[0].y;
    gRndr_PerspTexScaledUOverZ1 = imageWidth * triVerts[1].z * triUVs[1].x;
    gRndr_PerspTexScaledVOverZ1 = imageHeight * triVerts[1].z * triUVs[1].y;
    gRndr_PerspTexScaledUOverZ2 = imageWidth * triVerts[2].z * triUVs[2].x;
    gRndr_PerspTexScaledVOverZ2 = imageHeight * triVerts[2].z * triUVs[2].y;

    const bool useClippedNearPlane = clippedTriVerts != 0
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
        const float dx10 = triVerts[0].x - triVerts[1].x;
        const float dx12 = triVerts[2].x - triVerts[1].x;
        const float dy10 = triVerts[0].y - triVerts[1].y;
        const float dy12 = triVerts[2].y - triVerts[1].y;
        const float determinant = dy12 * dx10 - dy10 * dx12;
        Plane2f reciprocalZ = { 0 };
        Plane2f uOverZ = { 0 };
        Plane2f vOverZ = { 0 };
        if (determinant != 0.0f) {
            const float inverseDeterminant = -1.0f / determinant;
            const float reciprocal10 = reciprocalValues[0] - reciprocalValues[1];
            const float reciprocal12 = reciprocalValues[2] - reciprocalValues[1];
            const float u10 = uValues[0] - uValues[1];
            const float u12 = uValues[2] - uValues[1];
            const float v10 = vValues[0] - vValues[1];
            const float v12 = vValues[2] - vValues[1];
            reciprocalZ.gradient.x = (dy12 * reciprocal10 - dy10 * reciprocal12) * inverseDeterminant;
            reciprocalZ.gradient.y = (dx10 * reciprocal12 - dx12 * reciprocal10) * inverseDeterminant;
            uOverZ.gradient.x = (dy12 * u10 - dy10 * u12) * inverseDeterminant;
            uOverZ.gradient.y = (dx10 * u12 - dx12 * u10) * inverseDeterminant;
            vOverZ.gradient.x = (dy12 * v10 - dy10 * v12) * inverseDeterminant;
            vOverZ.gradient.y = (dx10 * v12 - dx12 * v10) * inverseDeterminant;
        }
        gRndr_PerspInvDepthStepX = reciprocalZ.gradient.x;
        gRndr_PerspInvDepthStepY = reciprocalZ.gradient.y;
        gRndr_PerspInvDepthBase = reciprocalValues[0];
        gRndr_PerspTexScaledUOverZStepX = uOverZ.gradient.x;
        gRndr_PerspTexScaledUOverZStepY = uOverZ.gradient.y;
        gRndr_PerspTexScaledUOverZBase = uValues[0];
        gRndr_PerspTexScaledVOverZStepX = vOverZ.gradient.x;
        gRndr_PerspTexScaledVOverZStepY = vOverZ.gradient.y;
        gRndr_PerspTexScaledVOverZBase = vValues[0];
        gRndr_PerspPlaneOriginX = triVerts[0].x;
        gRndr_PerspPlaneOriginY = triVerts[0].y;
    }

    TexturedPlanes planes = { 0 };
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
    const float adjustX = planes.originX - 0.5f;
    const float adjustY = planes.originY - 0.5f;
    planes.reciprocalZ.base -= adjustX * planes.reciprocalZ.gradient.x + adjustY * planes.reciprocalZ.gradient.y;
    planes.uOverZ.base -= adjustX * planes.uOverZ.gradient.x + adjustY * planes.uOverZ.gradient.y;
    planes.vOverZ.base -= adjustX * planes.vOverZ.gradient.x + adjustY * planes.vOverZ.gradient.y;

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndrTextureMipSelectVariantImage(
            entry,
            triVerts,
            3,
            (const zVec2*)(&gRndr_PerspTexScaledUOverZ0),
            (const zVec2*)(&gRndr_PerspInvDepthStepX),
            (const zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
            (const zVec2*)(&gRndr_PerspTexScaledVOverZStepX)
        );
        if (selectedImage == 0) {
            return;
        }

        const float widthScale = selectedImage->widthScale != 0.0f ? selectedImage->widthScale : 1.0f;
        textureScale = 1048576.0f / widthScale;
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    float minPositiveReciprocalZ = 1000.0f;
    for (int i_4890 = 0; i_4890 < vertCount; ++i_4890) {
        const float reciprocalZ = projectedVerts[i_4890].x * planes.reciprocalZ.gradient.x
            + projectedVerts[i_4890].y * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
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

    ScanConvertEdge edgeTableA[0x40] = { 0 };
    ScanConvertEdge edgeTableB[0x40] = { 0 };
    int edgeCountA = 0;
    int edgeCountB = 0;
    int fixed16Value;
    int edgeVertexIndex;
    int edgeYStart;
    float edgeSampleY;
    const int edgeStepA = zRndr::g_scanConvertMode != 0 ? 1 : -1;
    const int edgeStepB = -edgeStepA;

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepA;
        if (nextIndex < 0)
            nextIndex += vertCount;
        if (nextIndex >= vertCount)
            nextIndex -= vertCount;
        const zVec3& start = projectedVerts[edgeVertexIndex];
        const zVec3& end = projectedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float slope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, slope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * slope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }
            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepB;
        if (nextIndex < 0)
            nextIndex += vertCount;
        if (nextIndex >= vertCount)
            nextIndex -= vertCount;
        const zVec3& start = projectedVerts[edgeVertexIndex];
        const zVec3& end = projectedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float slope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, slope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * slope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }
            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }
    if (edgeCountA == 0 || edgeCountB == 0)
        return;

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char*)(selectedImage->pixels);
    zRndr::g_spanQueuedTexAlphaMap = selectedImage->queuedAlphaMap;
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode0;

    unsigned short* palette = (unsigned short*)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette = variantIndex == -1 ? palette : &palette[(variantIndex + 1) * 0x100];
        spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial* visibleSpans[0x40] = { 0 };
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;
    int chunkPixels;
    if (zRndr::g_perspectiveAdaptiveMinSpan == 0) {
        chunkPixels = zRndr::g_perspectiveTextureDeltaXPow2;
    } else {
        float selectedChunk = (float)(zRndr::g_perspectiveAdaptiveMaxSpan);
        if (planes.reciprocalZ.gradient.x != 0.0f) {
            selectedChunk = minPositiveReciprocalZ * zRndr::g_perspectiveAdaptiveSlope / planes.reciprocalZ.gradient.x;
            if (selectedChunk < 0.0f)
                selectedChunk = -selectedChunk;
        }
        const double selectedChunkBits = (double)(selectedChunk) - -6755399441055744.0;
        chunkPixels = *(const int*)(&selectedChunkBits);
        if (chunkPixels > zRndr::g_perspectiveAdaptiveMaxSpan) {
            chunkPixels = zRndr::g_perspectiveAdaptiveMaxSpan;
        }
        if (chunkPixels < zRndr::g_perspectiveAdaptiveMinSpan) {
            chunkPixels = zRndr::g_perspectiveAdaptiveMinSpan;
        }
    }
    if (chunkPixels < 1)
        chunkPixels = 1;
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

            const float rowReciprocalZ = (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth
                = ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanList(visibleSpans, y, &spanCount);
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial* span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr
                        = (unsigned short*)(scanlineBase + (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    int remaining = span->sampleXMax - span->sampleXMin + 1;
                    int x = span->sampleXMin;
                    while (remaining > chunkPixels) {
                        const int count = chunkPixels;
                        const float startX = (float)(x);
                        const float endX = (float)(x + count);
                        const float sampleY = (float)(y);
                        const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                        const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX
                            + planeY * gRndr_PerspInvDepthStepY + gRndr_PerspInvDepthBase;
                        const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                            + gRndr_PerspInvDepthBase;
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const double uStepBits
                            = (double)((endU - startU) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double vStepBits
                            = (double)((endV - startV) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                        const double vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&uStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&vStepBits);
                        spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                        x += count;
                        remaining -= count;
                    }

                    if (remaining > 0) {
                        const int count = remaining;
                        const float startX = (float)(x);
                        const float endX = (float)(x + count);
                        const float sampleY = (float)(y);
                        const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                        const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX
                            + planeY * gRndr_PerspInvDepthStepY + gRndr_PerspInvDepthBase;
                        const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                            + gRndr_PerspInvDepthBase;
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const double uStepBits
                            = (double)((endU - startU) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double vStepBits
                            = (double)((endV - startV) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                        const double vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&uStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&vStepBits);
                        spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                    }
                }
            }
        }
        scanlineBase += zRndr::g_pitchBytes;
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
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 || triVerts == 0 || triUVs == 0 || vertCount <= 0
        || zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial* selectedImage = entry->image;
    const float imageWidth = (float)(selectedImage->width);
    const float imageHeight = (float)(selectedImage->height);

    gRndr_PerspTexScaledUOverZ0 = imageWidth * triVerts[0].z * triUVs[0].x;
    gRndr_PerspTexScaledVOverZ0 = imageHeight * triVerts[0].z * triUVs[0].y;
    gRndr_PerspTexScaledUOverZ1 = imageWidth * triVerts[1].z * triUVs[1].x;
    gRndr_PerspTexScaledVOverZ1 = imageHeight * triVerts[1].z * triUVs[1].y;
    gRndr_PerspTexScaledUOverZ2 = imageWidth * triVerts[2].z * triUVs[2].x;
    gRndr_PerspTexScaledVOverZ2 = imageHeight * triVerts[2].z * triUVs[2].y;

    const bool useClippedNearPlane = clippedTriVerts != 0
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
        const float dx10 = triVerts[0].x - triVerts[1].x;
        const float dx12 = triVerts[2].x - triVerts[1].x;
        const float dy10 = triVerts[0].y - triVerts[1].y;
        const float dy12 = triVerts[2].y - triVerts[1].y;
        const float determinant = dy12 * dx10 - dy10 * dx12;
        Plane2f reciprocalZ = { 0 };
        Plane2f uOverZ = { 0 };
        Plane2f vOverZ = { 0 };
        if (determinant != 0.0f) {
            const float inverseDeterminant = -1.0f / determinant;
            const float reciprocal10 = reciprocalValues[0] - reciprocalValues[1];
            const float reciprocal12 = reciprocalValues[2] - reciprocalValues[1];
            const float u10 = uValues[0] - uValues[1];
            const float u12 = uValues[2] - uValues[1];
            const float v10 = vValues[0] - vValues[1];
            const float v12 = vValues[2] - vValues[1];
            reciprocalZ.gradient.x = (dy12 * reciprocal10 - dy10 * reciprocal12) * inverseDeterminant;
            reciprocalZ.gradient.y = (dx10 * reciprocal12 - dx12 * reciprocal10) * inverseDeterminant;
            uOverZ.gradient.x = (dy12 * u10 - dy10 * u12) * inverseDeterminant;
            uOverZ.gradient.y = (dx10 * u12 - dx12 * u10) * inverseDeterminant;
            vOverZ.gradient.x = (dy12 * v10 - dy10 * v12) * inverseDeterminant;
            vOverZ.gradient.y = (dx10 * v12 - dx12 * v10) * inverseDeterminant;
        }
        gRndr_PerspInvDepthStepX = reciprocalZ.gradient.x;
        gRndr_PerspInvDepthStepY = reciprocalZ.gradient.y;
        gRndr_PerspInvDepthBase = reciprocalValues[0];
        gRndr_PerspTexScaledUOverZStepX = uOverZ.gradient.x;
        gRndr_PerspTexScaledUOverZStepY = uOverZ.gradient.y;
        gRndr_PerspTexScaledUOverZBase = uValues[0];
        gRndr_PerspTexScaledVOverZStepX = vOverZ.gradient.x;
        gRndr_PerspTexScaledVOverZStepY = vOverZ.gradient.y;
        gRndr_PerspTexScaledVOverZBase = vValues[0];
        gRndr_PerspPlaneOriginX = triVerts[0].x;
        gRndr_PerspPlaneOriginY = triVerts[0].y;
    }

    TexturedPlanes planes = { 0 };
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
    const float adjustX = planes.originX - 0.5f;
    const float adjustY = planes.originY - 0.5f;
    planes.reciprocalZ.base -= adjustX * planes.reciprocalZ.gradient.x + adjustY * planes.reciprocalZ.gradient.y;
    planes.uOverZ.base -= adjustX * planes.uOverZ.gradient.x + adjustY * planes.uOverZ.gradient.y;
    planes.vOverZ.base -= adjustX * planes.vOverZ.gradient.x + adjustY * planes.vOverZ.gradient.y;

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndrTextureMipSelectVariantImage(
            entry,
            triVerts,
            3,
            (const zVec2*)(&gRndr_PerspTexScaledUOverZ0),
            (const zVec2*)(&gRndr_PerspInvDepthStepX),
            (const zVec2*)(&gRndr_PerspTexScaledUOverZStepX),
            (const zVec2*)(&gRndr_PerspTexScaledVOverZStepX)
        );
        if (selectedImage == 0) {
            return;
        }

        const float widthScale = selectedImage->widthScale != 0.0f ? selectedImage->widthScale : 1.0f;
        textureScale = 1048576.0f / widthScale;
    }

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    float minPositiveReciprocalZ = 1000.0f;
    for (int i_5057 = 0; i_5057 < vertCount; ++i_5057) {
        const float reciprocalZ = projectedVerts[i_5057].x * planes.reciprocalZ.gradient.x
            + projectedVerts[i_5057].y * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
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

    ScanConvertEdge edgeTableA[0x40] = { 0 };
    ScanConvertEdge edgeTableB[0x40] = { 0 };
    int edgeCountA = 0;
    int edgeCountB = 0;
    int fixed16Value;
    int edgeVertexIndex;
    int edgeYStart;
    float edgeSampleY;
    const int edgeStepA = zRndr::g_scanConvertMode != 0 ? 1 : -1;
    const int edgeStepB = -edgeStepA;

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepA;
        if (nextIndex < 0)
            nextIndex += vertCount;
        if (nextIndex >= vertCount)
            nextIndex -= vertCount;
        const zVec3& start = projectedVerts[edgeVertexIndex];
        const zVec3& end = projectedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableA[edgeCountA].yStart = edgeYStart;
            edgeTableA[edgeCountA].reserved = 0;
            if (dy != 0.0f) {
                const float slope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].xStepFixed, slope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableA[edgeCountA].currentXFixed,
                    start.x + (edgeSampleY - start.y) * slope
                );
            } else {
                edgeTableA[edgeCountA].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableA[edgeCountA].currentXFixed, start.x);
            }
            ++edgeCountA;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }

    edgeVertexIndex = topVertexIndex;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[edgeVertexIndex].y);
    edgeYStart = (fixed16Value + 0x7fff) >> 16;
    edgeSampleY = (float)(edgeYStart) + 0.5f;
    while (edgeVertexIndex != bottomVertexIndex) {
        int nextIndex = edgeVertexIndex + edgeStepB;
        if (nextIndex < 0)
            nextIndex += vertCount;
        if (nextIndex >= vertCount)
            nextIndex -= vertCount;
        const zVec3& start = projectedVerts[edgeVertexIndex];
        const zVec3& end = projectedVerts[nextIndex];
        if (edgeSampleY <= end.y) {
            const float dy = end.y - start.y;
            edgeTableB[edgeCountB].yStart = edgeYStart;
            edgeTableB[edgeCountB].reserved = 0;
            if (dy != 0.0f) {
                const float slope = (end.x - start.x) / dy;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].xStepFixed, slope);
                ZRNDR_SET_FIXED16_FROM_FLOAT(
                    edgeTableB[edgeCountB].currentXFixed,
                    start.x + (edgeSampleY - start.y) * slope
                );
            } else {
                edgeTableB[edgeCountB].xStepFixed = 0;
                ZRNDR_SET_FIXED16_FROM_FLOAT(edgeTableB[edgeCountB].currentXFixed, start.x);
            }
            ++edgeCountB;
            ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, end.y);
            edgeYStart = (fixed16Value + 0x7fff) >> 16;
            edgeSampleY = (float)(edgeYStart) + 0.5f;
        }
        edgeVertexIndex = nextIndex;
    }
    if (edgeCountA == 0 || edgeCountB == 0)
        return;

    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[topVertexIndex].y);
    const int firstScanline = (fixed16Value + 0x7fff) >> 16;
    ZRNDR_SET_FIXED16_FROM_FLOAT(fixed16Value, projectedVerts[bottomVertexIndex].y);
    const int lastScanline = (fixed16Value - 0x8041) >> 16;
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexPixels = (unsigned char*)(selectedImage->pixels);
    zRndr::g_spanQueuedTexAlphaMap = selectedImage->queuedAlphaMap;
    zRndr::g_spanActiveTexShift = selectedImage->uShiftFrom20;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::g_spanActiveConstAlphaBits = alpha255;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedFanTriSpanOp_Mode0;

    unsigned short* palette = (unsigned short*)(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette = variantIndex == -1 ? palette : &palette[(variantIndex + 1) * 0x100];
        spanProc = zRndr::g_pfnTexturedFanTriSpanOp_Mode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial* visibleSpans[0x40] = { 0 };
    int edgeIndexA = 0;
    int edgeIndexB = 0;
    int currentXFixedA = edgeTableA[0].currentXFixed;
    int currentXFixedB = edgeTableB[0].currentXFixed;
    int xStepFixedA = edgeTableA[0].xStepFixed;
    int xStepFixedB = edgeTableB[0].xStepFixed;
    unsigned char* scanlineBase = (unsigned char*)(zRndr::g_frameBuffer) + firstScanline * zRndr::g_pitchBytes;
    int chunkPixels;
    if (zRndr::g_perspectiveAdaptiveMinSpan == 0) {
        chunkPixels = zRndr::g_perspectiveTextureDeltaXPow2;
    } else {
        float selectedChunk = (float)(zRndr::g_perspectiveAdaptiveMaxSpan);
        if (planes.reciprocalZ.gradient.x != 0.0f) {
            selectedChunk = minPositiveReciprocalZ * zRndr::g_perspectiveAdaptiveSlope / planes.reciprocalZ.gradient.x;
            if (selectedChunk < 0.0f)
                selectedChunk = -selectedChunk;
        }
        const double selectedChunkBits = (double)(selectedChunk) - -6755399441055744.0;
        chunkPixels = *(const int*)(&selectedChunkBits);
        if (chunkPixels > zRndr::g_perspectiveAdaptiveMaxSpan) {
            chunkPixels = zRndr::g_perspectiveAdaptiveMaxSpan;
        }
        if (chunkPixels < zRndr::g_perspectiveAdaptiveMinSpan) {
            chunkPixels = zRndr::g_perspectiveAdaptiveMinSpan;
        }
    }
    if (chunkPixels < 1)
        chunkPixels = 1;
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

            const float rowReciprocalZ = (float)(y)*planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth
                = ((float)(xMin)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep
                = ((float)(xMax)*planes.reciprocalZ.gradient.x + rowReciprocalZ) * zRndr::g_inverseDepthScale
                + zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            zRndr::g_pfnBuildSpanListSecondary(visibleSpans, y, &spanCount);
            {
                for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                    zRndr::SpanNodePartial* span = visibleSpans[spanIndex];
                    if (span == 0 || span->sampleXMin > span->sampleXMax) {
                        continue;
                    }

                    zRndr::g_spanCurrentSpanBaseAddr
                        = (unsigned short*)(scanlineBase + (int)(span->sampleXMin) * zRndr::g_bytesPerPixel);
                    int remaining = span->sampleXMax - span->sampleXMin + 1;
                    int x = span->sampleXMin;
                    while (remaining > chunkPixels) {
                        const int count = chunkPixels;
                        const float startX = (float)(x);
                        const float endX = (float)(x + count);
                        const float sampleY = (float)(y);
                        const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                        const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX
                            + planeY * gRndr_PerspInvDepthStepY + gRndr_PerspInvDepthBase;
                        const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                            + gRndr_PerspInvDepthBase;
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const double uStepBits
                            = (double)((endU - startU) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double vStepBits
                            = (double)((endV - startV) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                        const double vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&uStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&vStepBits);
                        spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                        x += count;
                        remaining -= count;
                    }

                    if (remaining > 0) {
                        const int count = remaining;
                        const float startX = (float)(x);
                        const float endX = (float)(x + count);
                        const float sampleY = (float)(y);
                        const float startPlaneX = startX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float endPlaneX = endX + 0.5f - gRndr_PerspPlaneOriginX;
                        const float planeY = sampleY + 0.5f - gRndr_PerspPlaneOriginY;
                        const float startInvZ = startPlaneX * gRndr_PerspInvDepthStepX
                            + planeY * gRndr_PerspInvDepthStepY + gRndr_PerspInvDepthBase;
                        const float endInvZ = endPlaneX * gRndr_PerspInvDepthStepX + planeY * gRndr_PerspInvDepthStepY
                            + gRndr_PerspInvDepthBase;
                        const float startU
                            = (startPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / startInvZ;
                        const float startV
                            = (startPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / startInvZ;
                        const float endU
                            = (endPlaneX * gRndr_PerspTexScaledUOverZStepX + planeY * gRndr_PerspTexScaledUOverZStepY
                                  + gRndr_PerspTexScaledUOverZBase)
                            / endInvZ;
                        const float endV
                            = (endPlaneX * gRndr_PerspTexScaledVOverZStepX + planeY * gRndr_PerspTexScaledVOverZStepY
                                  + gRndr_PerspTexScaledVOverZBase)
                            / endInvZ;
                        const double uStepBits
                            = (double)((endU - startU) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double vStepBits
                            = (double)((endV - startV) * textureScale / (float)(count)) - -6755399441055744.0;
                        const double uStartBits = (double)(startU * textureScale) - -6755399441055744.0;
                        const double vStartBits = (double)(startV * textureScale) - -6755399441055744.0;
                        zRndr::g_spanActiveTexUStepFixed20 = *(const int*)(&uStepBits);
                        zRndr::g_spanActiveTexVStepFixed20 = *(const int*)(&vStepBits);
                        spanProc(*(const int*)(&uStartBits), *(const int*)(&vStartBits), count, texVShift);
                        zRndr::g_spanCurrentSpanBaseAddr += count;
                    }
                }
            }
        }
        scanlineBase += zRndr::g_pitchBytes;
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
    zRndr::g_pfnImmediateRaster4((unsigned short*)(zRndr::g_frameBuffer), x0, y0, x1, y1, color16);
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
    if (segmentCount <= 0) {
        return;
    }

    const zRndr_LineClipRect2I* clip = (const zRndr_LineClipRect2I*)(clipRect);
    const zRndr_LinePoint2I* point = points + 1;
    int remaining = segmentCount;
    do {
        zRndr::g_pfnImmediateRaster5(
            (unsigned short*)(zRndr::g_frameBuffer),
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
    zRndr::g_spanAllocCursor->invDepth = samplePoint->z;
    zRndr::g_spanAllocCursor->invDepthStep = samplePoint->z;
    zRndr::g_spanAllocCursor->depthSlope = 0.0f;
    zRndr::g_spanAllocCursor->sampleXMin = (int)(samplePoint->x);
    zRndr::g_spanAllocCursor->sampleXMax = zRndr::g_spanAllocCursor->sampleXMin;

    int isVisible;
    zRndrSpanOcclusionTestColumnVisibility((int)(samplePoint->y), &isVisible);
    return isVisible > 0 ? 1 : 0;
}

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-lensflare-drawqueuedsample16-clippedframebuffer
     * @recoil-artifact defines .text recoil:function:0x498cb0: zRndr::LensFlareDrawQueuedSample16ClippedFramebuffer
     *
     *
     * Purpose: Draw one queued lens-flare sample into the clipped 16-bit framebuffer.
     */
    void __fastcall LensFlareDrawQueuedSample16ClippedFramebuffer(
        LensFlareSamplePartial * sample,
        float screenScale,
        int yOffsetPixels
    )
    {
        zRndr_LensFlareSource* lensFlareSource = (zRndr_LensFlareSource*)((unsigned int)(sample->lensFlareSource));
        int blendTowardFramebuffer = 0;
        float reciprocalZ;
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

        const int y = (int)(sample->y * screenScale) + yOffsetPixels;
        const int x = (int)(screenScale * sample->x);
        if ((unsigned int)(x) > (unsigned int)(g_activeRegionWidth)
            || (unsigned int)(y) > (unsigned int)(g_activeRegionHeight)) {
            return;
        }

        unsigned short* const pixel = (unsigned short*)(g_frameBuffer) + ((unsigned int)(g_pitchBytes) >> 1) * y + x;
        int packedColor = sample->packedColor16;

        if (g_overlayBlendEnabled != 0) {
            const int overlayAlpha = (int)(g_overlayBlendAlpha * 255.0);
            if (g_pixelPackGreenBits == 6) {
                if (overlayAlpha > 3) {
                    if (overlayAlpha >= 0xfc) {
                        packedColor = g_overlayBlendPackedColor16 & 0xffff;
                    } else {
                        const int overlayColor = g_overlayBlendPackedColor16;
                        packedColor
                            += (((overlayColor & 0xf800) - (packedColor & 0xf800)) * overlayAlpha >> 8) & ~0x7ff;
                        packedColor += (((overlayColor & 0x7e0) - (packedColor & 0x7e0)) * overlayAlpha >> 8) & ~0x1f;
                        packedColor += ((overlayColor & 0x1f) - (packedColor & 0x1f)) * overlayAlpha >> 8;
                    }
                }
            } else if (overlayAlpha > 7) {
                if (overlayAlpha >= 0xfc) {
                    packedColor = g_overlayBlendPackedColor16 & 0xffff;
                } else {
                    const int overlayColor = g_overlayBlendPackedColor16;
                    packedColor += ((((overlayColor & 0x7c00) - (packedColor & 0x7c00)) * overlayAlpha >> 8) & ~0x3ff)
                        + ((((overlayColor & 0x3e0) - (packedColor & 0x3e0)) * overlayAlpha >> 8) & ~0x1f)
                        + (((overlayColor & 0x1f) - (packedColor & 0x1f)) * overlayAlpha >> 8);
                }
            }
        }

        if (blendTowardFramebuffer == 0) {
            *pixel = (unsigned short)(packedColor);
            return;
        }

        const int fadeAlpha
            = (int)((lensFlareSource->depthFadeInvZMax - reciprocalZ) * lensFlareSource->depthFadeScale);
        if (g_pixelPackGreenBits == 6) {
            if (fadeAlpha <= 3) {
                return;
            }
            if (fadeAlpha >= 0xfc) {
                *pixel = (unsigned short)(packedColor);
                return;
            }
            int frameColor = *pixel;
            frameColor += (((packedColor & 0xf800) - (frameColor & 0xf800)) * fadeAlpha >> 8) & ~0x7ff;
            frameColor += (((packedColor & 0x7e0) - (frameColor & 0x7e0)) * fadeAlpha >> 8) & ~0x1f;
            frameColor += ((packedColor & 0x1f) - (frameColor & 0x1f)) * fadeAlpha >> 8;
            *pixel = (unsigned short)(frameColor);
        } else {
            if (fadeAlpha <= 7) {
                return;
            }
            if (fadeAlpha >= 0xfc) {
                *pixel = (unsigned short)(packedColor);
                return;
            }
            const int frameColor = *pixel;
            *pixel += ((((packedColor & 0x7c00) - (frameColor & 0x7c00)) * fadeAlpha >> 8) & ~0x3ff)
                + ((((packedColor & 0x3e0) - (frameColor & 0x3e0)) * fadeAlpha >> 8) & ~0x1f)
                + (((packedColor & 0x1f) - (frameColor & 0x1f)) * fadeAlpha >> 8);
        }
    }
} // namespace zRndr

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
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, y, x, color16);
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
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + y, g_zRndr_CircleCenterX + x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + y, g_zRndr_CircleCenterX - x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - y, g_zRndr_CircleCenterX + x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - y, g_zRndr_CircleCenterX - x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + x, g_zRndr_CircleCenterX + y, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + x, g_zRndr_CircleCenterX - y, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - x, g_zRndr_CircleCenterX - y, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - x, g_zRndr_CircleCenterX + y, packedColor);
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
    if (zRndr::g_textureMipSelectionEnabled == 0) {
        return entry != 0 ? entry->image : 0;
    }

    float selectedZ = triVerts[0].z;
    const zVec3* candidateVertex = triVerts + 1;
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

    const float mipDeltas[4] = { (vertexUvPairs[selectedVertex].x + mipParamsB->x) * invZAtX - uOverZ,
        (vertexUvPairs[selectedVertex].x + mipParamsB->y) * invZAtY - uOverZ,
        (vertexUvPairs[selectedVertex].y + mipParamsC->x) * invZAtX - vOverZ,
        (vertexUvPairs[selectedVertex].y + mipParamsC->y) * invZAtY - vOverZ };

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
    const int variantIndex = (*(int*)(&variantIndexBits)) >> 1;
    return entry->GetVariantImageAtIndex(variantIndex);
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
    const unsigned int pitchWords = (unsigned int)(zRndr::g_pitchBytes) >> 1;
    dstPixels[pitchWords * y + x] = (unsigned short)(color16);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawline16
 * @recoil-artifact defines .text recoil:function:0x4992d0: zRndrDrawLine16
 *
 *
 * Purpose: Rasterize an unclipped 16-bit Bresenham line into the active framebuffer.
 */
void __fastcall zRndrDrawLine16(unsigned short* dstPixels, int x0, int y0, int x1, int y1, int color16)
{
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
        unsigned short* cursor = &dstPixels[startIndex];
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

    unsigned short* cursor = &dstPixels[startIndex];
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawline16-segmented
 * @recoil-artifact defines .text recoil:function:0x4993a0: zRndrDrawLine16Segmented
 *
 *
 * Purpose: Rasterize a segmented 16-bit Bresenham line into the active framebuffer.
 */
void __fastcall
zRndrDrawLine16Segmented(unsigned short* dstPixels, int x0, int y0, int x1, int y1, int color16, int segmentCount)
{
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
        unsigned short* cursor = &dstPixels[startIndex];
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
    unsigned short* cursor = &dstPixels[startIndex];
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-drawline16-clipped
 * @recoil-artifact defines .text recoil:function:0x499500: zRndrDrawLine16Clipped
 *
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

    unsigned short* cursor = &dstPixels[startIndex];
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
    unsigned short* cursor = zRndr::g_spanCurrentSpanBaseAddr + pixelCount;
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
 *
 *
 * Purpose: Blend a solid color into the active 555 span using the supplied alpha.
 *
 * Evidence: BN uses gRndr_CurrentSpanBaseAddr as an ordinary word pointer for
 * this solid-fill leaf; there is no ESP-pivot write shape here.
 */
void __fastcall zRndrFillSpan555Solid(int packedColor16, int blendAlpha, int pixelCount)
{
    unsigned short* cursor = zRndr::g_spanCurrentSpanBaseAddr;
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-fillspan565solid
 * @recoil-artifact defines .text recoil:function:0x4998a0: zRndrFillSpan565Solid
 *
 *
 * Purpose: Blend a solid color into the active 565 span using the supplied alpha.
 *
 * Evidence: BN uses gRndr_CurrentSpanBaseAddr as an ordinary word pointer here;
 * the limited reconstruction marker records only BN's partial-register display.
 */
void __fastcall zRndrFillSpan565Solid(int packedColor16, int blendAlpha, int pixelCount)
{
    unsigned short* cursor = zRndr::g_spanCurrentSpanBaseAddr;
    do {
        if (blendAlpha > 3) {
            if (blendAlpha >= 0xfc) {
                *cursor = (unsigned short)(packedColor16);
            } else {
                int dst = (short)(*cursor);
                int greenDelta = (packedColor16 & 0x07e0) - (dst & 0x07e0);
                int redDelta = (packedColor16 & 0xf800) - (dst & 0xf800);
                greenDelta *= blendAlpha;
                redDelta *= blendAlpha;
                redDelta = (redDelta >> 8) & 0xfffff800;
                dst += redDelta;
                int blueDelta = (packedColor16 & 0x001f) - (dst & 0x001f);
                blueDelta *= blendAlpha;
                greenDelta = (greenDelta >> 8) & 0xffffffe0;
                blueDelta >>= 8;
                *cursor = (unsigned short)(dst + blueDelta + greenDelta);
            }
        }

        ++cursor;
        --pixelCount;
    } while (pixelCount != 0);
}
