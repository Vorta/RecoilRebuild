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
template <class T> const T &MinValue(const T &lhs, const T &rhs) {
    return lhs < rhs ? lhs : rhs;
}

template <class T> const T &MaxValue(const T &lhs, const T &rhs) {
    return lhs < rhs ? rhs : lhs;
}

void sort(float *first, float *last)
{
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
}

extern "C" {
zVideo_BltSourceToPrimaryProc g_zVideo_pfnBltSourceToPrimary = 0;
}

namespace zSys {
RECOIL_NOINLINE int RECOIL_CDECL CheckCpuSignatureMask();
}

float g_zRndr_InverseZTolerance = 0.0f;
int g_zRndr_ActivePaletteRemapKey = -1;
int g_zRndr_ActivePaletteShadeRecipeIndex = -1;
int g_zRndr_CircleCenterX = 0;
int g_zRndr_CircleCenterY = 0;
int g_zRndr_CircleDrawAuxArg = 0;

namespace zRndr {
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
OverlayBlendRowProc g_pfnOverlayBlendRow = 0;
unsigned int g_swOverlayPremulPacked = 0;
unsigned int g_swOverlayPremulPackedRot16 = 0;
int g_swOverlayDstScale5 = 0;
int g_pixelPackRedBits = 0;
int g_pixelPackGreenBits = 0;
int g_pixelPackBlueBits = 0;
unsigned int g_pixelPackRedMask = 0;
unsigned int g_pixelPackGreenMask = 0;
unsigned int g_pixelPackBlueMask = 0;
int g_pixelPackRedShift = 0;
int g_pixelPackGreenShift = 0;
int g_pixelPackBlueShift = 0;
unsigned short g_mmxMaskRedPacked[4] = {0};
unsigned short g_mmxMaskGreenPacked[4] = {0};
unsigned short g_mmxMaskGreenBits[4] = {0};
unsigned short g_mmxMaskBlueBits[4] = {0};
unsigned short g_mmxBitsBlue255[4] = {0};
unsigned short g_mmxBitsGreen255[4] = {0};
unsigned short g_mmxBitsRed255[4] = {0};
unsigned short g_mmxFogFactors[4] = {0};
int g_mmxUPair[2] = {0};
int g_mmxVPair[2] = {0};
int g_mmxUStepDup2[2] = {0};
int g_mmxVStepDup2[2] = {0};
int g_mmxUMask[2] = {0};
int g_mmxVMask[2] = {0};
int g_mmxVShiftCounts[2] = {0};
unsigned char *g_spanActiveTexels = 0;
char *g_spanActiveTexAlphaMap = 0;
unsigned short *g_spanActiveTexPalette = 0;
int g_spanTexUAdvance = 0;
int g_spanTexVAdvance = 0;
unsigned short *g_spanCurrentDst16 = 0;
int g_spanActiveConstAlphaBits = 0;
int g_spanActiveTexVMask = 0x07f00000;
int g_spanActiveTexUMask = 0x7f;
int g_spanActiveShadeFixed16 = 0;
int g_spanActiveShadeStepFixed16 = 0;
PointOpProc g_pfnPointOpCandidate = 0;
PointOpProc g_pfnPointOpActive = 0;
SpanRoutineProc g_pfnImmediateRaster4 = 0;
SpanRoutineProc g_pfnImmediateRasterReserved = 0;
SpanRoutineProc g_pfnImmediateRaster5 = 0;
SpanRoutineProc g_pfnSelectedSpanOp = 0;
TexturedQueuedSpanProc g_pfnSelectedSpanOp_Mode0 = 0;
FlatImmediateSpanProc g_pfnFlatImmediateSpanOp = 0;
TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode0 = 0;
TexturedQueuedSpanProc g_pfnTexturedQueuedSpanOp_Mode1 = 0;
SpanRoutineProc g_pfnTexturedQueuedFinalize = 0;
SpanRoutineProc g_pfnTexturedQueuedFinalizeAlt = 0;
TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode0 = 0;
TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode0 = 0;
TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode0 = 0;
TexturedQueuedSpanProc g_pfnTexturedFanTriSpanOp_Mode1 = 0;
TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode0 = 0;
TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode0 = 0;
TexturedQueuedSpanProc g_pfnPolyTlvSpanOp_Mode1 = 0;
TexturedQueuedSpanProc g_pfnPolyTlvSpanOpAlt_Mode1 = 0;
TexturedQueuedSpanProc g_pfnFlatQueuedSpanOp_Mode1 = 0;
TexturedQueuedSpanProc g_pfnFlatQueuedSpanOpAlt_Mode1 = 0;
TransparentQueuedPolyDrawCmd g_transparentQueue[0x15e] = {0};
OverwriteQueuedPolyDrawCmd g_overwriteQueue[0x15e] = {0};
int g_transparentQueueSortIndices[0x15e] = {0};
int g_transparentQueueCount = 0;
int g_overwriteQueueCount = 0;
int g_overlayBlendEnabled = 0;
int g_overlayBlendRectLeft = 0;
int g_overlayBlendRectTop = 0;
int g_overlayBlendRectRight = 0;
int g_overlayBlendRectBottom = 0;
int g_lensFlareSampleQueueCount = 0;
int g_lensFlareVisibleSampleCount = 0;
int g_lensFlareVisibilityActive = 0;
unsigned int g_overlayBlendPackedColor16 = 0;
double g_overlayBlendAlpha = 0.0;
zImage_TexDirEntryPartial *g_lensFlareVisibleSampleStages[4] = {0};
zRndr_LensFlareVisibleSampleDef *g_lensFlareVisibleSampleDefs[0x40] = {0};
LensFlareSamplePartial g_lensFlareSampleQueue[0x28a] = {0};
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
RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorRed) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorGreen) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorBlue) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColor16) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColor16Dup) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorRamp) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(SpanOccluderPolyPartial) == 0x64);
RECOIL_STATIC_ASSERT(sizeof(SpanNodePartial) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(LensFlareSamplePartial) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(QueuedVec3) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(TransparentQueuedPolyDrawCmd) == 0x384);
RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, polyVerts) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, triVerts) == 0x32c);
RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, scanConvertMode) == 0x368);
RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, savedInvDepthBias) == 0x370);
RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, alphaOrShadeBits) == 0x378);
RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, shadeOrSpanMode) == 0x37c);
RECOIL_STATIC_ASSERT(sizeof(OverwriteQueuedPolyDrawCmd) == 0x48c);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, polyVerts) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, triVerts) == 0x328);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, alphaOrShadeF) == 0x34c);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, shadeOrSpanMode) == 0x350);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, vertexCount) == 0x354);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, materialRef) == 0x370);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, scanConvertMode) == 0x478);
RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, savedInvDepthBias) == 0x480);
RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, packedColor16) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, lensFlareSource) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, sampleXMin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, sampleXMax) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, invDepth) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, invDepthStep) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, depthSlope) == 0x14);

namespace {
struct ScanVertex {
    float x;
    float y;
};

int Fixed16FromFloat(float value) {
    const double scaled = static_cast<double>(value) * 65536.0;
    return static_cast<int>(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

int ScanlineStartFromY(float y) {
    return (Fixed16FromFloat(y) + 0x7fff) >> 16;
}

int ScanlineEndFromY(float y) {
    return (Fixed16FromFloat(y) - 0x8041) >> 16;
}

int SpanStartFromX(float x) {
    return (Fixed16FromFloat(x) + 0x7fff) >> 16;
}

int SpanEndFromX(float x) {
    return (Fixed16FromFloat(x) - 0x8001) >> 16;
}

float FloatFromBits(unsigned int bits) {
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

void AppendSpanListNode(SpanNodePartial **spanList, int *spanCount,
                        SpanNodePartial *node) {
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

float SpanDepthAtX(const SpanNodePartial *span, int x) {
    if (x == span->sampleXMin) {
        return span->invDepth;
    }
    if (x == span->sampleXMax) {
        return span->invDepthStep;
    }

    return span->invDepth + static_cast<float>(x - span->sampleXMin) * span->depthSlope;
}

float SpanDepthAtX(int sampleXMin, float invDepth, float depthSlope, int x) {
    return invDepth + static_cast<float>(x - sampleXMin) * depthSlope;
}

float SpanDepthAtXByParts(int sampleXMin, float invDepth, float depthSlope, int x) {
    return invDepth + static_cast<float>(x - sampleXMin) * depthSlope;
}

void LinkSpanNode(int columnIndex, SpanNodePartial *previous, SpanNodePartial *node,
                  SpanNodePartial *next) {
    node->next = next;
    if (previous != 0) {
        previous->next = node;
    } else {
        g_spanColumnHeadTable[columnIndex] = node;
    }

    g_spanIterPrevLink = previous;
    g_spanIterNode = node;
}

void InsertPendingSpanSorted(SpanNodePartial **spanList, int columnIndex,
                             int *spanCount) {
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

void InsertPendingSpanNoDepthTest(SpanNodePartial **spanList, int columnIndex,
                                  int *spanCount) {
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
                current->invDepthStep = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope,
                                                     current->sampleXMax);

                if (currentMax > pending->sampleXMax) {
                    SpanNodePartial *rightSplit = pending + 1;
                    rightSplit->sampleXMin = pending->sampleXMax + 1;
                    rightSplit->sampleXMax = currentMax;
                    rightSplit->invDepth = SpanDepthAtX(currentMin, currentInvDepth,
                                                        currentDepthSlope, rightSplit->sampleXMin);
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
            SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMin);
        break;
    }

    LinkSpanNode(columnIndex, previous, pending, current);
    AppendSpanListNode(spanList, spanCount, pending);
    ++g_spanAllocCursor;
}

void BuildVisibleSpanListWithDepthTest(SpanNodePartial **spanList, int columnIndex,
                                       int *spanCount) {
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
            zRndr_SpanOcclusion_TestSpanDepthOrderPair(pending, &occluder) != 0;

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
                    SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, splitMax);
                AppendSpanListNode(spanList, spanCount, pending);
                ++g_spanAllocCursor;

                pending = g_spanAllocCursor;
                pending->next = 0;
                pending->sampleXMin = splitMax + 1;
                pending->sampleXMax = pendingMax;
                pending->invDepth = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope,
                                                 pending->sampleXMin);
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
                pending->invDepth = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope,
                                                 pending->sampleXMin);
            }

            current = current->next;
            continue;
        }

        if (occluder.sampleXMin <= pendingMax) {
            const int leftMax = occluder.sampleXMin - 1;
            pending->sampleXMax = leftMax;
            pending->invDepthStep =
                SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, leftMax);
            AppendSpanListNode(spanList, spanCount, pending);
            ++g_spanAllocCursor;

            if (occluder.sampleXMax >= pendingMax) {
                return;
            }

            pending = g_spanAllocCursor;
            pending->next = 0;
            pending->sampleXMin = occluder.sampleXMax + 1;
            pending->sampleXMax = pendingMax;
            pending->invDepth =
                SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
            pending->invDepthStep = pendingInvDepthStep;
            pending->depthSlope = pendingDepthSlope;
        }

        current = current->next;
    }

    AppendSpanListNode(spanList, spanCount, pending);
    ++g_spanAllocCursor;
}

void InsertPendingSpanWithDepthTest(SpanNodePartial **spanList, int columnIndex,
                                    int *spanCount) {
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
            zRndr_SpanOcclusion_TestSpanDepthOrderPair(pending, current) != 0;

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
                current->invDepthStep = SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope,
                                                     current->sampleXMax);

                if (currentMax > pendingMax) {
                    SpanNodePartial *rightSplit = pending + 1;
                    rightSplit->sampleXMin = pendingMax + 1;
                    rightSplit->sampleXMax = currentMax;
                    rightSplit->invDepth = SpanDepthAtX(currentMin, currentInvDepth,
                                                        currentDepthSlope, rightSplit->sampleXMin);
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
                SpanDepthAtX(currentMin, currentInvDepth, currentDepthSlope, current->sampleXMin);
            break;
        }

        if (current->sampleXMin <= pendingMin) {
            if (current->sampleXMax >= pendingMax) {
                return;
            }

            if (current->sampleXMax >= pendingMin) {
                pending->sampleXMin = current->sampleXMax + 1;
                pending->invDepth = SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope,
                                                 pending->sampleXMin);
            }

            previous = current;
            current = current->next;
            continue;
        }

        if (current->sampleXMin <= pendingMax) {
            const int leftMax = current->sampleXMin - 1;
            pending->sampleXMax = leftMax;
            pending->invDepthStep =
                SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, leftMax);
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
            pending->invDepth =
                SpanDepthAtX(pendingMin, pendingInvDepth, pendingDepthSlope, pending->sampleXMin);
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

void RECOIL_FASTCALL SpanRoutineUnavailable(int, int) {}

void RECOIL_FASTCALL PointOpUnavailable(void *, int, int, int) {}

void RECOIL_FASTCALL TexturedQueuedSpanRoutineUnavailable(int, int, int,
                                                          int) {}

void RECOIL_FASTCALL FlatImmediateSpanRoutineUnavailable(int, int, int) {
}

void FillReplicatedWordMask(unsigned short (&mask)[4], unsigned short value) {
    mask[0] = value;
    mask[1] = value;
    mask[2] = value;
    mask[3] = value;
}

bool FogParamsDifferFromActive(const FogParamsPartial &params) {
    const float kCommitThreshold = 0.01f;
    return fabs(g_fogParamsActive.colorRgb01[0] - params.colorRgb01[0]) >= kCommitThreshold ||
           fabs(g_fogParamsActive.colorRgb01[1] - params.colorRgb01[1]) >= kCommitThreshold ||
           fabs(g_fogParamsActive.colorRgb01[2] - params.colorRgb01[2]) >= kCommitThreshold;
}

void CommitFogParamsIfChanged(const FogParamsPartial &params) {
    if (FogParamsDifferFromActive(params)) {
        memcpy(&g_fogParamsActive, &params, sizeof(params));
    }
}
} // namespace

// Reimplements 0x4904d0: zRndr::SetPerspectiveAdaptiveCorrection
RECOIL_NOINLINE void RECOIL_STDCALL
SetPerspectiveAdaptiveCorrection(float perspectiveAdaptiveCorrection) {
    const float plusOne = perspectiveAdaptiveCorrection + 1.0f;
    g_spanDepthBias = perspectiveAdaptiveCorrection;
    g_spanDepthBiasPlusOne = plusOne;
    g_spanDepthBiasPlusOneInv =
        (plusOne == 0.0f || plusOne != plusOne) ? 0.0f : 1.0f / plusOne;
}

// Reimplements 0x490480: zRndr::SetPerspectiveAdaptiveSpanParams
RECOIL_NOINLINE void RECOIL_FASTCALL SetPerspectiveAdaptiveSpanParams(int minSpan,
                                                                      int maxSpan,
                                                                      float slope) {
    g_perspectiveAdaptiveMinSpan = minSpan;
    g_perspectiveAdaptiveMaxSpan = maxSpan;
    g_perspectiveAdaptiveSlope = slope;
}

// Reimplements 0x48fd80: zRndr::InitGlobals
RECOIL_NOINLINE int RECOIL_CDECL InitGlobals() {
    g_spanAllocCursor = 0;
    g_spanColumnHeadTable = 0;
    g_spanPoolBase = 0;
    g_spanLastNode = 0;
    g_spanIterNode = 0;
    g_spanIterPrevLink = 0;
    g_spanReservedWriteOnly = 0;
    g_spanColumnCount = 0;
    g_spanColumnCountPadded = 0;
    g_pfnBuildSpanList = 0;
    g_pfnBuildSpanListSecondary = 0;
    g_pfnOverlayBlendRow = 0;
    g_pixelPackRedBits = 0;
    g_pixelPackGreenBits = 0;
    g_pixelPackBlueBits = 0;
    g_pixelPackRedMask = 0;
    g_pixelPackGreenMask = 0;
    g_pixelPackBlueMask = 0;
    g_pixelPackRedShift = 0;
    g_pixelPackGreenShift = 0;
    g_pixelPackBlueShift = 0;

    SetPerspectiveAdaptiveCorrection(FloatFromBits(0x38d1b717));

    g_perspectiveTextureDeltaXInput = 0x20;
    g_perspectiveTextureDeltaXPow2 = 0x20;
    g_perspectiveTextureDeltaXShift = 5;
    g_perspectiveTextureDeltaXPow2F = 32.0f;
    g_perspectiveTextureFarZInv = FloatFromBits(0x3b5a3c21);
    g_perspectiveAdaptiveMinSpan = 0;
    g_perspectiveAdaptiveMaxSpan = 0;
    g_perspectiveAdaptiveSlope = 0.0f;
    g_inverseDepthBias = 0.0f;
    g_inverseDepthScale = 1.0f;
    g_scanConvertMode = 1;
    g_perspectiveTextureEnabled = 1;
    g_transparentQueueCount = 0;
    g_overwriteQueueCount = 0;
    g_overlayBlendEnabled = 0;
    g_lensFlareSampleQueueCount = 0;
    g_lensFlareVisibleSampleCount = 0;

    zColorRgb color = {FloatFromBits(0x3d23d70a), FloatFromBits(0x3d23d70a),
                       FloatFromBits(0x3d23d70a)};
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

    g_zVideo_pfnBltSourceToPrimary = (zVideo_BltSourceToPrimaryProc)(0x0048f560);
    g_defaultGraphicsFlags = -1;
    zOptionEntryPartial *option =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    g_graphicsFlags = option != 0 ? &option->payloadOrBuffer : &g_defaultGraphicsFlags;
    g_perspectiveTextureDeltaXBytes = g_perspectiveTextureDeltaXPow2 * g_bytesPerPixel;
    return 0;
}

// Reimplements 0x48d450: zRndr::OverlayBlendRow555_Scalar
RECOIL_NOINLINE void RECOIL_FASTCALL OverlayBlendRow555_Scalar(unsigned short *rowPixels16,
                                                               int pixelCount) {
    int pairCount = pixelCount >> 1;
    unsigned int *rowPairs = (unsigned int *)(rowPixels16);
    while (pairCount > 0) {
        const unsigned int packedPair = *rowPairs;
        const unsigned int loLanes =
            ((((packedPair & 0x03e07c1fU) * static_cast<unsigned int>(g_swOverlayDstScale5)) >>
              5) +
             g_swOverlayPremulPackedRot16) &
            0x03e07c1fU;
        const unsigned int hiLanes = ((((packedPair >> 5) & 0x03e0f81fU) *
                                        static_cast<unsigned int>(g_swOverlayDstScale5)) +
                                       g_swOverlayPremulPacked) &
                                      0x7c1f03e0U;
        *rowPairs = hiLanes | loLanes;
        ++rowPairs;
        --pairCount;
    }
}

// Reimplements 0x48d4b0: zRndr::OverlayBlendRow565_Scalar
RECOIL_NOINLINE void RECOIL_FASTCALL OverlayBlendRow565_Scalar(unsigned short *rowPixels16,
                                                               int pixelCount) {
    const int srcScale5 = static_cast<int>(g_overlayBlendAlpha * 32.0);
    const unsigned int src = g_overlayBlendPackedColor16;
    const unsigned int redMask = g_pixelPackRedMask;
    const unsigned int greenMask = g_pixelPackGreenMask;
    const unsigned int blueMask = g_pixelPackBlueMask;

    for (int i = 0; i < pixelCount; ++i) {
        const unsigned int dst = rowPixels16[i];
        const unsigned int red =
            (((dst & redMask) * g_swOverlayDstScale5 + (src & redMask) * srcScale5) >> 5) &
            redMask;
        const unsigned int green =
            (((dst & greenMask) * g_swOverlayDstScale5 + (src & greenMask) * srcScale5) >> 5) &
            greenMask;
        const unsigned int blue =
            (((dst & blueMask) * g_swOverlayDstScale5 + (src & blueMask) * srcScale5) >> 5) &
            blueMask;
        rowPixels16[i] = static_cast<unsigned short>(red | green | blue);
    }
}

// Reimplements 0x49e140: zRndr::SpanMmxSetPixelFormatMasks
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMmxSetPixelFormatMasks(int greenBits) {
    const unsigned short greenMask = greenBits == 5 ? 0x03e0U : 0x07e0U;
    const unsigned short redPacked = greenBits == 5 ? 0xfc00U : 0xf800U;

    FillReplicatedWordMask(g_mmxMaskGreenBits, greenMask);
    FillReplicatedWordMask(g_mmxMaskBlueBits, 0x001fU);
    FillReplicatedWordMask(g_mmxMaskRedPacked, redPacked);
    FillReplicatedWordMask(g_mmxMaskGreenPacked, 0xffe0U);
}

// Reimplements 0x48ff80: zRndr::SelectSpanRoutines
RECOIL_NOINLINE void RECOIL_CDECL SelectSpanRoutines() {
    zVideo::PixelPack_GetRgbBits(&g_pixelPackRedBits, &g_pixelPackGreenBits, &g_pixelPackBlueBits);
    zVideo::PixelPack_GetRgbMasks(&g_pixelPackRedMask, &g_pixelPackGreenMask, &g_pixelPackBlueMask);
    zVideo::PixelPack_GetPackingParams(&g_pixelPackRedShift, &g_pixelPackGreenShift,
                                       &g_pixelPackBlueShift);

    if (g_graphicsFlags != 0) {
        *g_graphicsFlags &= ~4;
    }

    const int graphicsFlags = g_graphicsFlags != 0 ? *g_graphicsFlags : 0;
    const bool useShortAdaptiveSpans = (graphicsFlags & 8) != 0;
    SetPerspectiveAdaptiveSpanParams(useShortAdaptiveSpans ? 0x10 : 0x20,
                                     useShortAdaptiveSpans ? 0x40 : 0x200, 0.100000001f);

    if (g_bytesPerPixel != 2) {
        return;
    }

    g_pfnPointOpCandidate = (PointOpProc)zRndr_PlotPixel16;
    g_pfnPointOpActive = (PointOpProc)zRndr_PlotPixel16;
    g_pfnImmediateRaster4 = (SpanRoutineProc)zRndr_DrawLine16;
    g_pfnImmediateRasterReserved = (SpanRoutineProc)zRndr_DrawLine16_Segmented;
    g_pfnImmediateRaster5 = (SpanRoutineProc)zRndr_DrawLine16_Clipped;
    g_pfnSelectedSpanOp = (SpanRoutineProc)zRndr_FillSpan16Opaque;
    g_pfnSelectedSpanOp_Mode0 = SpanMasked16FromTex16SwitchVShift;
    g_pfnFlatImmediateSpanOp =
        g_pixelPackGreenBits == 5 ? (FlatImmediateSpanProc)zRndr_FillSpan555Solid
                                  : (FlatImmediateSpanProc)zRndr_FillSpan565Solid;

    if ((graphicsFlags & 4) != 0) {
        g_pfnTexturedQueuedSpanOp_Mode0 =
            zSys::CheckCpuSignatureMask() != 0 ? SpanCopy16FromTex16ExplicitVShift
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

    if ((graphicsFlags & 4) != 0) {
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

// Reimplements 0x4903f0: zRndr::GetActiveRegionState
RECOIL_NOINLINE void *RECOIL_FASTCALL GetActiveRegionState(int *outWidth,
                                                           int *outHeight,
                                                           int *outBitsPerPixel,
                                                           int *outPitchBytes) {
    *outWidth = g_activeRegionWidth;
    *outHeight = g_activeRegionHeight;
    *outBitsPerPixel = g_bytesPerPixel << 3;
    *outPitchBytes = g_pitchBytes;
    return g_frameBuffer;
}

// Reimplements 0x490340: zRndr::SetFrameBufferRegion
RECOIL_NOINLINE void RECOIL_FASTCALL SetFrameBufferRegion(void *pixels,
                                                          zOpt_ViewRectSection *activeRegionRect,
                                                          int bitsPerPixel,
                                                          int pitchBytes) {
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
        g_bytesPerPixel = static_cast<int>(static_cast<unsigned int>(bitsPerPixel) >> 3);
    }

    g_pitchBytes = pitchBytes;
    g_perspectiveTextureDeltaXBytes = g_perspectiveTextureDeltaXPow2 * g_bytesPerPixel;
}

// Reimplements 0x4903c0: zRndr::SetActiveRegionSizeFromRect
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetActiveRegionSizeFromRect(HudUiRect *rect) {
    if (rect != 0) {
        g_activeRegionWidth = rect->right - rect->left;
        g_activeRegionHeight = rect->bottom - rect->top;
    }
}

// Reimplements 0x4903e0: zRndr::SetVideoStrideMirrors
RECOIL_NOINLINE void RECOIL_FASTCALL SetVideoStrideMirrors(int stride) {
    g_videoStrideMirror1 = stride;
    g_videoStrideMirror0 = stride;
}

// Reimplements 0x490710: zRndr::SpanOcclusionAddPolygon
RECOIL_NOINLINE void RECOIL_FASTCALL SpanOcclusionAddPolygon(const zVec3 *vertices,
                                                             int vertCount) {
    const int slotIndex = g_spanOccluderPolyCount;
    if (slotIndex >= 7) {
        return;
    }

    SpanOccluderPolyPartial *slot = &g_spanOccluderPolys[slotIndex];
    for (int i = 0; i < vertCount; ++i) {
        slot->vertices[i][0] = vertices[i].x;
        slot->vertices[i][1] = vertices[i].y;
        slot->vertices[i][2] = vertices[i].z;
    }

    slot->vertCount = vertCount > 8 ? 8 : vertCount;
    ++g_spanOccluderPolyCount;
}

// Reimplements 0x490610: zRndr::SpanOcclusionSubmitOccluderRect
// (D:\Proj\Battlesport\zrndr_span.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SpanOcclusionSubmitOccluderRect(const HudUiRect *rect, int halveIfReplicate, float z) {
    zVec3 vertices[4];
    vertices[0].x = static_cast<float>(rect->left);
    vertices[0].y = static_cast<float>(rect->top);
    vertices[1].x = vertices[0].x;
    vertices[1].y = static_cast<float>(rect->bottom);
    vertices[2].x = static_cast<float>(rect->right);
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
    SpanOcclusionAddPolygon(vertices, 4);
}

// Reimplements 0x490520: zRndr::SpanOcclusionInit
RECOIL_NOINLINE int RECOIL_FASTCALL SpanOcclusionInit(int height) {
    g_spanColumnCount = height;
    g_spanColumnCountPadded = height + 0x80;
    g_spanColumnHeadTable = static_cast<SpanNodePartial **>(
        calloc(static_cast<size_t>(g_spanColumnCountPadded), sizeof(SpanNodePartial *)));
    g_spanPoolBase = static_cast<SpanNodePartial *>(calloc(
        static_cast<size_t>(g_spanColumnCountPadded) << 8, sizeof(SpanNodePartial)));

    SpanOcclusionBuildColumnHeadTable();
    g_spanOccluderPolyCount = 0;
    g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
    g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanList;
    return 0;
}

// Reimplements 0x490590: zRndr::SpanOcclusionBuildColumnHeadTable
RECOIL_NOINLINE void RECOIL_CDECL SpanOcclusionBuildColumnHeadTable() {
    for (int i = 0; i < g_spanColumnCount; ++i) {
        g_spanColumnHeadTable[i] = 0;
    }

    g_spanIterNode = 0;
    g_spanAllocCursor = g_spanPoolBase;
    g_spanIterPrevLink = 0;

    for (int i_1084 = 0; i_1084 < g_spanOccluderPolyCount; ++i_1084) {
        SpanOcclusionRasterizeOccluderPoly(&g_spanOccluderPolys[i_1084],
                                           g_spanOccluderPolys[i_1084].vertCount);
    }
}

// Reimplements 0x4927d0: zRndr::SpanOcclusionRasterizeOccluderPoly
RECOIL_NOINLINE void RECOIL_FASTCALL
SpanOcclusionRasterizeOccluderPoly(SpanOccluderPolyPartial *poly, int vertCount) {
    if (poly == 0 || vertCount <= 0) {
        return;
    }

    ScanVertex vertices[8] = {0};
    int reducedCount = 1;
    vertices[0].x = poly->vertices[0][0];
    vertices[0].y = poly->vertices[0][1];

    const int sourceCount = MinValue(vertCount, 8);
    for (int i = 1; i < sourceCount; ++i) {
        const float x = poly->vertices[i][0];
        const float y = poly->vertices[i][1];
        if (x != vertices[reducedCount - 1].x || y != vertices[reducedCount - 1].y) {
            vertices[reducedCount].x = x;
            vertices[reducedCount].y = y;
            ++reducedCount;
        }
    }

    if (reducedCount > 1 && vertices[reducedCount - 1].x == vertices[0].x &&
        vertices[reducedCount - 1].y == vertices[0].y) {
        --reducedCount;
    }

    if (reducedCount < 3) {
        return;
    }

    float minY = vertices[0].y;
    float maxY = vertices[0].y;
    for (int i_1121 = 1; i_1121 < reducedCount; ++i_1121) {
        minY = MinValue(minY, vertices[i_1121].y);
        maxY = MaxValue(maxY, vertices[i_1121].y);
    }

    const int yStart = ScanlineStartFromY(minY);
    const int yEnd = ScanlineEndFromY(maxY);
    if (yStart > yEnd) {
        return;
    }

    SpanNodePartial *spanList[64] = {0};
    for (int y = yStart; y <= yEnd; ++y) {
        if (y < 0 || y >= g_spanColumnCountPadded) {
            continue;
        }

        float intersections[16] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;
        for (int i = 0; i < reducedCount; ++i) {
            const ScanVertex &a = vertices[i];
            const ScanVertex &b = vertices[(i + 1) % reducedCount];
            if (a.y == b.y) {
                continue;
            }

            const float edgeMinY = MinValue(a.y, b.y);
            const float edgeMaxY = MaxValue(a.y, b.y);
            if (sampleY < edgeMinY || sampleY >= edgeMaxY) {
                continue;
            }

            const float t = (sampleY - a.y) / (b.y - a.y);
            intersections[intersectionCount++] = a.x + (b.x - a.x) * t;
        }

        if (intersectionCount < 2) {
            continue;
        }

        sort(intersections, intersections + intersectionCount);
        for (int i_1163 = 0; i_1163 + 1 < intersectionCount; i_1163 += 2) {
            const int xMin = SpanStartFromX(intersections[i_1163]);
            const int xMax = SpanEndFromX(intersections[i_1163 + 1]);
            if (xMin > xMax) {
                continue;
            }

            g_spanAllocCursor->sampleXMin = xMin;
            g_spanAllocCursor->sampleXMax = xMax;
            g_spanAllocCursor->invDepth = poly->vertices[0][2];
            g_spanAllocCursor->invDepthStep = poly->vertices[0][2];
            g_spanAllocCursor->depthSlope = 0.0f;

            int spanCount = 0;
            SpanBuildProc buildProc = g_pfnBuildSpanList != 0
                                          ? g_pfnBuildSpanList
                                          : zRndr_SpanOcclusion_InsertSpanNode_Local;
            buildProc(spanList, y, &spanCount);
        }
    }
}

// Reimplements 0x490600: zRndr::SpanOcclusionResetFrame
RECOIL_NOINLINE void RECOIL_CDECL SpanOcclusionResetFrame() {
    g_spanOccluderPolyCount = 0;
}

// Reimplements 0x490780: zRndr::SpanOcclusionShutdown
RECOIL_NOINLINE void RECOIL_CDECL SpanOcclusionShutdown() {
    if (g_spanColumnHeadTable != 0) {
        free(g_spanColumnHeadTable);
    }
    g_spanColumnHeadTable = 0;

    if (g_spanPoolBase != 0) {
        free(g_spanPoolBase);
    }
    g_spanPoolBase = 0;
    g_spanAllocCursor = 0;
    g_spanIterNode = 0;
    g_spanIterPrevLink = 0;
    g_spanLastNode = 0;
}

// Reimplements 0x49e0e0: zRndr::SpanAlphaBlend565_Mmx_FromPal8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565_Mmx_FromPal8(FogParamsPartial *params,
                                                                    int packedRed,
                                                                    int packedGreen,
                                                                    int packedBlue) {
    const unsigned int packedColor16 =
        static_cast<unsigned int>(packedRed | packedGreen | packedBlue);
    params->packedColorRed = packedRed;
    params->packedColorGreen = packedGreen;
    params->packedColorBlue = packedBlue;
    params->packedColor16 = static_cast<int>(packedColor16);
    params->packedColor16Dup = static_cast<int>(packedColor16 | (packedColor16 << 16));

    const unsigned int rampStep = (static_cast<unsigned int>(packedRed | packedGreen) << 11) |
                                   (static_cast<unsigned int>(packedBlue) >> 5);
    unsigned int rampValue = 0;
    for (int i = 31; i >= 0; --i) {
        params->packedColorRamp[i] = static_cast<int>(rampValue);
        rampValue += rampStep;
    }
}

namespace {
int SpanTex16SampleIndex(int texU, int texV, int texVShift,
                                  int texUMask);
unsigned short BlendPixel565Alpha8(unsigned short dstPixel, unsigned short srcPixel,
                                  int alpha);
unsigned int BlendPair565Alpha5(unsigned int dstPair, unsigned short srcPixel, int alpha);
unsigned short BlendPixel555Alpha8(unsigned short dstPixel, unsigned short srcPixel,
                                  int alpha);
unsigned int BlendPair555Alpha5(unsigned int dstPair, unsigned short srcPixel, int alpha);
unsigned short BlendPixel555ConstAlphaMap(unsigned short dstPixel, unsigned short srcPixel,
                                         int alpha);
} // namespace

// Reimplements 0x49c230: zRndr::SpanAlphaBlend565ConstAlphaFromPal8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromPal8(int texU,
                                                                         int texV,
                                                                         int pixelCount,
                                                                         int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexels[vIndex + uIndex];
        if (sourceIndex != 0 && g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = g_spanActiveTexPalette[static_cast<short>(sourceIndex)];
            } else {
                const int dstColor = static_cast<short>(*dst);
                const int srcColor = g_spanActiveTexPalette[dstColor];
                const int greenDelta =
                    (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta =
                    (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = static_cast<unsigned short>(blended);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49c020: zRndr::SpanMasked16FromPal8To565
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromPal8To565(int texU, int texV,
                                                               int pixelCount,
                                                               int texVShift) {
    SpanAlphaBlend565ConstAlphaFromPal8(texU, texV, pixelCount, texVShift);
}

// Reimplements 0x49c150: zRndr::SpanMasked16FromTex16To565
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromTex16To565(int texU,
                                                                int texV,
                                                                int pixelCount,
                                                                int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned short sourceTexel = texels16[vIndex + uIndex];
        if (sourceTexel != 0 && g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = sourceTexel;
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49c360: zRndr::SpanAlphaBlend565FromTex16Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565FromTex16Alpha8(int texU,
                                                                      int texV,
                                                                      int pixelCount,
                                                                      int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    if ((pixelCount & 1) != 0) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            if (alpha >= 0xf8) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    {
    for (int pairCount = pixelCount >> 1; pairCount != 0; --pairCount) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            unsigned int packedPixels = 0;
            if (alpha >= 0xf8) {
                packedPixels = static_cast<unsigned int>(sourceTexel) |
                               (static_cast<unsigned int>(sourceTexel) << 16);
            } else {
                memcpy(&packedPixels, dst, sizeof(packedPixels));
                packedPixels = BlendPair565Alpha5(packedPixels, sourceTexel, alpha);
            }

            memcpy(dst, &packedPixels, sizeof(packedPixels));
        }

        texU += g_spanTexUAdvance * 2;
        texV += g_spanTexVAdvance * 2;
        dst += 2;
    }
    }
}

// Reimplements 0x49c560: zRndr::SpanAlphaBlend555FromTex16Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555FromTex16Alpha8(int texU,
                                                                      int texV,
                                                                      int pixelCount,
                                                                      int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    if ((pixelCount & 1) != 0) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            if (alpha >= 0xf8) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel555Alpha8(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    {
    for (int pairCount = pixelCount >> 1; pairCount != 0; --pairCount) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            unsigned int packedPixels = 0;
            if (alpha >= 0xf8) {
                packedPixels = static_cast<unsigned int>(sourceTexel) |
                               (static_cast<unsigned int>(sourceTexel) << 16);
            } else {
                memcpy(&packedPixels, dst, sizeof(packedPixels));
                packedPixels = BlendPair555Alpha5(packedPixels, sourceTexel, alpha);
            }

            memcpy(dst, &packedPixels, sizeof(packedPixels));
        }

        texU += g_spanTexUAdvance * 2;
        texV += g_spanTexVAdvance * 2;
        dst += 2;
    }
    }
}

// Reimplements 0x49c970: zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromTex16Alpha8(
    int texU, int texV, int pixelCount, int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const double alphaScaled = static_cast<double>(alphaMap[sourceIndex]) *
                                   static_cast<double>(alphaScale);
        const int alpha = static_cast<int>(alphaScaled >= 0.0 ? alphaScaled + 0.5
                                                              : alphaScaled - 0.5);
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49ca90: zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFromTex16Alpha8(
    int texU, int texV, int pixelCount, int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const double alphaScaled = static_cast<double>(alphaMap[sourceIndex]) *
                                   static_cast<double>(alphaScale);
        const int alpha = static_cast<int>(alphaScaled >= 0.0 ? alphaScaled + 0.5
                                                              : alphaScaled - 0.5);
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel555ConstAlphaMap(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49cbb0: zRndr::SpanAlphaBlend565MmxFromTex16Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565MmxFromTex16Alpha8(int texU,
                                                                         int texV,
                                                                         int pixelCount,
                                                                         int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel565Alpha8(*dst, texels16[sourceIndex], alphaMap[sourceIndex]);

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    for (int i_1490 = quadPixels; i_1490 < pixelCount; ++i_1490) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49cea0: zRndr::SpanAlphaBlend555MmxFromTex16Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555MmxFromTex16Alpha8(int texU,
                                                                         int texV,
                                                                         int pixelCount,
                                                                         int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel555Alpha8(*dst, texels16[sourceIndex], alphaMap[sourceIndex]);

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    for (int i_1529 = quadPixels; i_1529 < pixelCount; ++i_1529) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel555Alpha8(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49d1a0: zRndr::SpanAlphaBlend565FromPal8Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565FromPal8Alpha8(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned char *texels8 = g_spanActiveTexels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    if ((pixelCount & 1) != 0) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    for (int i = pixelCount >> 1; i != 0; --i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                dst[0] = sourcePixel;
                dst[1] = sourcePixel;
            } else {
                unsigned int packedPixels = 0;
                memcpy(&packedPixels, dst, sizeof(packedPixels));
                packedPixels = BlendPair565Alpha5(packedPixels, sourcePixel, alpha);
                memcpy(dst, &packedPixels, sizeof(packedPixels));
            }
        }

        texU += 2 * g_spanTexUAdvance;
        texV += 2 * g_spanTexVAdvance;
        dst += 2;
    }
}

// Reimplements 0x49d3b0: zRndr::SpanAlphaBlend555FromPal8Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555FromPal8Alpha8(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned char *texels8 = g_spanActiveTexels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    if ((pixelCount & 1) != 0) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel555Alpha8(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    for (int i = pixelCount >> 1; i != 0; --i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                dst[0] = sourcePixel;
                dst[1] = sourcePixel;
            } else {
                unsigned int packedPixels = 0;
                memcpy(&packedPixels, dst, sizeof(packedPixels));
                packedPixels = BlendPair555Alpha5(packedPixels, sourcePixel, alpha);
                memcpy(dst, &packedPixels, sizeof(packedPixels));
            }
        }

        texU += 2 * g_spanTexUAdvance;
        texV += 2 * g_spanTexVAdvance;
        dst += 2;
    }
}

// Reimplements 0x49d810: zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromPal8Alpha8(
    int texU, int texV, int pixelCount, int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned char *texels8 = g_spanActiveTexels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        const double alphaScaled = static_cast<double>(alphaMap[sourceIndex]) *
                                   static_cast<double>(alphaScale);
        const int alpha = static_cast<int>(alphaScaled >= 0.0 ? alphaScaled + 0.5
                                                              : alphaScaled - 0.5);
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49d950: zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFromPal8Alpha8(
    int texU, int texV, int pixelCount, int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned char *texels8 = g_spanActiveTexels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        const double alphaScaled = static_cast<double>(alphaMap[sourceIndex]) *
                                   static_cast<double>(alphaScale);
        const int alpha = static_cast<int>(alphaScaled >= 0.0 ? alphaScaled + 0.5
                                                              : alphaScaled - 0.5);
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel555ConstAlphaMap(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49da80: zRndr::SpanAlphaBlend565MmxFromPal8Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565MmxFromPal8Alpha8(int texU,
                                                                        int texV,
                                                                        int pixelCount,
                                                                        int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned char *texels8 = g_spanActiveTexels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel565Alpha8(*dst, palette[texels8[sourceIndex]], alphaMap[sourceIndex]);

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    for (int i_1733 = quadPixels; i_1733 < pixelCount; ++i_1733) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49ddb0: zRndr::SpanAlphaBlend555MmxFromPal8Alpha8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555MmxFromPal8Alpha8(int texU,
                                                                        int texV,
                                                                        int pixelCount,
                                                                        int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned char *texels8 = g_spanActiveTexels;
    const unsigned char *alphaMap = (const unsigned char *)(g_spanActiveTexAlphaMap);
    const unsigned short *palette = g_spanActiveTexPalette;

    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel555Alpha8(*dst, palette[texels8[sourceIndex]], alphaMap[sourceIndex]);

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }

    for (int i_1773 = quadPixels; i_1773 < pixelCount; ++i_1773) {
        const int sourceIndex =
            SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel555Alpha8(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49c760: zRndr::SpanAlphaBlend565ConstAlphaFromTex16
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFromTex16(int texU,
                                                                          int texV,
                                                                          int pixelCount,
                                                                          int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int srcColor = static_cast<short>(texels16[vIndex + uIndex]);
        if (g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = static_cast<unsigned short>(srcColor);
            } else {
                const int dstColor = static_cast<short>(*dst);
                const int greenDelta =
                    (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta =
                    (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = static_cast<unsigned short>(blended);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49c860: zRndr::SpanAlphaBlend555ConstAlphaFromTex16
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFromTex16(int texU,
                                                                          int texV,
                                                                          int pixelCount,
                                                                          int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    const unsigned short *texels16 = (const unsigned short *)(g_spanActiveTexels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int srcColor = static_cast<short>(texels16[vIndex + uIndex]);
        if (g_spanActiveConstAlphaBits > 7) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = static_cast<unsigned short>(srcColor);
            } else {
                const int dstColor = static_cast<short>(*dst);
                const int redDelta =
                    (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta =
                    (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = static_cast<unsigned short>(blended);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49d5c0: zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend565ConstAlphaFastFromPal8(
    int texU, int texV, int pixelCount, int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexels[vIndex + uIndex];
        const int srcColor =
            static_cast<short>(g_spanActiveTexPalette[sourceIndex]);
        if (g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = static_cast<unsigned short>(srcColor);
            } else {
                const int dstColor = static_cast<short>(*dst);
                const int greenDelta =
                    (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta =
                    (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = static_cast<unsigned short>(blended);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

// Reimplements 0x49d6e0: zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8
RECOIL_NOINLINE void RECOIL_FASTCALL SpanAlphaBlend555ConstAlphaFastFromPal8(
    int texU, int texV, int pixelCount, int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexels[vIndex + uIndex];
        const int srcColor =
            static_cast<short>(g_spanActiveTexPalette[sourceIndex]);
        if (g_spanActiveConstAlphaBits > 7) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = static_cast<unsigned short>(srcColor);
            } else {
                const int dstColor = static_cast<short>(*dst);
                const int redDelta =
                    (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta =
                    (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int blueDelta =
                    (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = static_cast<unsigned short>(blended);
            }
        }

        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
        ++dst;
    }
}

namespace {
unsigned int RotateRight32(unsigned int value, int count) {
    return (value >> count) | (value << (32 - count));
}

bool FogCoordIsFullyFogged(unsigned int fogCoordFixed24) {
    return static_cast<int>(fogCoordFixed24) >= 0x1000000;
}

bool FogCoordUsesRamp(unsigned int fogCoordFixed24) {
    return static_cast<int>(fogCoordFixed24) >= 0x80000;
}

unsigned int FogRampIndex(unsigned int fogCoordFixed24) {
    return (0x1000000u - fogCoordFixed24) >> 19;
}

unsigned short FogBlendPixel565(unsigned short pixel, unsigned int fogCoordFixed24) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return static_cast<unsigned short>(g_fogParamsActive.packedColor16);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return pixel;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue =
        static_cast<unsigned int>(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int pixel32 = pixel;
    const unsigned int green = ((((pixel32 & 0x07e0u) >> 5) * rampIndex) + rampValue) & 0x07e0u;
    const unsigned int redBlue =
        (((pixel32 & 0xf81fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0xf81fu;
    return static_cast<unsigned short>(green + redBlue);
}

unsigned short FogBlendPixel555(unsigned short pixel, unsigned int fogCoordFixed24) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return static_cast<unsigned short>(g_fogParamsActive.packedColor16);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return pixel;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue =
        static_cast<unsigned int>(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int pixel32 = pixel;
    const unsigned int green = ((((pixel32 & 0x03e0u) >> 5) * rampIndex) + rampValue) & 0x03e0u;
    const unsigned int redBlue =
        (((pixel32 & 0x7c1fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0x7c1fu;
    return static_cast<unsigned short>(green + redBlue);
}

unsigned int FogBlendPair565(unsigned int packedPixels, unsigned int fogCoordFixed24) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return static_cast<unsigned int>(g_fogParamsActive.packedColor16Dup);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return packedPixels;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue =
        static_cast<unsigned int>(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int green =
        ((((packedPixels & 0xf81f07e0u) >> 5) * rampIndex) + rampValue) & 0xf81f07e0u;
    const unsigned int redBlue =
        (((packedPixels & 0x07e0f81fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) &
        0x07e0f81fu;
    return green + redBlue;
}

unsigned int FogBlendPair555(unsigned int packedPixels, unsigned int fogCoordFixed24) {
    if (FogCoordIsFullyFogged(fogCoordFixed24)) {
        return static_cast<unsigned int>(g_fogParamsActive.packedColor16Dup);
    }

    if (!FogCoordUsesRamp(fogCoordFixed24)) {
        return packedPixels;
    }

    const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
    const unsigned int rampValue =
        static_cast<unsigned int>(g_fogParamsActive.packedColorRamp[rampIndex]);
    const unsigned int green =
        ((((packedPixels & 0x7c1f03e0u) >> 5) * rampIndex) + rampValue) & 0x7c1f03e0u;
    const unsigned int redBlue =
        (((packedPixels & 0x03e07c1fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) &
        0x03e07c1fu;
    return green + redBlue;
}

short SaturatingSubWord(unsigned short minuend, unsigned short subtrahend) {
    const int result =
        static_cast<short>(minuend) - static_cast<short>(subtrahend);
    if (result > 0x7fff) {
        return 0x7fff;
    }
    if (result < -0x8000) {
        return -32768;
    }
    return static_cast<short>(result);
}

unsigned short MultiplyLowWord(short lhs, short rhs) {
    return static_cast<unsigned short>(static_cast<int>(lhs) *
                                      static_cast<int>(rhs));
}

unsigned short FogBlendMmxLane(unsigned short pixel, unsigned short fogFactor, int lane,
                              int redShift, int redTermShift) {
    const short factor = static_cast<short>(fogFactor);
    const short redDelta =
        SaturatingSubWord(g_mmxBitsRed255[lane], static_cast<unsigned short>(pixel >> redShift));
    const short greenDelta =
        SaturatingSubWord(g_mmxBitsGreen255[lane],
                          static_cast<unsigned short>((pixel & g_mmxMaskGreenBits[lane]) >> 5));
    const short blueDelta = SaturatingSubWord(
        g_mmxBitsBlue255[lane], static_cast<unsigned short>(pixel & g_mmxMaskBlueBits[lane]));

    const unsigned short redProduct = MultiplyLowWord(redDelta, factor);
    const unsigned short greenProduct = MultiplyLowWord(greenDelta, factor);
    const unsigned short blueProduct = MultiplyLowWord(blueDelta, factor);

    const unsigned short redTerm =
        static_cast<unsigned short>(redProduct << redTermShift) & g_mmxMaskRedPacked[lane];
    const unsigned short greenTerm =
        static_cast<unsigned short>(static_cast<short>(greenProduct) >> 3) &
        g_mmxMaskGreenPacked[lane];
    const unsigned short blueTerm =
        static_cast<unsigned short>(static_cast<short>(blueProduct) >> 8);

    return static_cast<unsigned short>(pixel + redTerm + greenTerm + blueTerm);
}

unsigned int BlendMmxQuad(unsigned short *pixels, unsigned int fogCoordFixed24,
                           unsigned int fogCoordStepFixed24, int redShift,
                           int redTermShift) {
    unsigned int coord = fogCoordFixed24;
    coord += fogCoordStepFixed24;
    g_mmxFogFactors[0] = static_cast<unsigned short>(coord >> 16);
    coord += fogCoordStepFixed24;
    g_mmxFogFactors[1] = static_cast<unsigned short>(coord >> 16);
    coord += fogCoordStepFixed24;
    g_mmxFogFactors[2] = static_cast<unsigned short>(coord >> 16);
    coord += fogCoordStepFixed24;
    g_mmxFogFactors[3] = static_cast<unsigned short>(coord >> 16);

    {
    for (int lane = 0; lane < 4; ++lane) {
        pixels[lane] =
            FogBlendMmxLane(pixels[lane], g_mmxFogFactors[lane], lane, redShift, redTermShift);
    }
    }

    coord += fogCoordStepFixed24;
    coord += fogCoordStepFixed24;
    return coord;
}

void FogBlendSpanMmxCore(unsigned short *pixels, int pixelCount,
                         int fogCoordFixed24, int fogCoordStepFixed24,
                         int redShift, int redTermShift,
                         void(RECOIL_FASTCALL *tailScalar)(unsigned short *, int,
                                                           int, int)) {
    unsigned short *cursor = pixels;
    int remaining = pixelCount;
    unsigned int fogCoord = static_cast<unsigned int>(fogCoordFixed24);
    const unsigned int fogStep = static_cast<unsigned int>(fogCoordStepFixed24);

    int headPixels =
        static_cast<int>((unsigned int)(pixels) & 3u);
    if (static_cast<unsigned int>(headPixels) >= static_cast<unsigned int>(remaining)) {
        headPixels = remaining;
    }

    if (headPixels != 0) {
        tailScalar(cursor, headPixels, static_cast<int>(fogCoord), fogCoordStepFixed24);
        cursor += headPixels;
        fogCoord += static_cast<unsigned int>(headPixels) * fogStep;
        remaining -= headPixels;
    }

    const int tailPixels = remaining & 3;
    unsigned int quadCount = static_cast<unsigned int>(remaining) >> 2;
    while (quadCount != 0) {
        fogCoord = BlendMmxQuad(cursor, fogCoord, fogStep, redShift, redTermShift);
        cursor += 4;
        --quadCount;
    }

    if (tailPixels != 0) {
        tailScalar(cursor, tailPixels, static_cast<int>(fogCoord), fogCoordStepFixed24);
    }
}

int SpanTex16SampleIndex(int texU, int texV, int texVShift,
                                  int texUMask) {
    const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
    const int uIndex = (texU >> 20) & texUMask;
    return vIndex + uIndex;
}

unsigned short SpanTex16Sample(int texU, int texV, int texVShift,
                              int texUMask) {
    const unsigned short *texels = (const unsigned short *)(g_spanActiveTexels);
    return texels[SpanTex16SampleIndex(texU, texV, texVShift, texUMask)];
}

unsigned short SpanPal8SampleExpanded(int texU, int texV, int texVShift,
                                     int texUMask) {
    const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, texUMask);
    return g_spanActiveTexPalette[g_spanActiveTexels[sourceIndex]];
}

unsigned short BlendPixel565Alpha8(unsigned short dstPixel, unsigned short srcPixel,
                                  int alpha) {
    const int dstColor = static_cast<short>(dstPixel);
    const int srcColor = srcPixel;
    const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
    const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
    int blended = dstColor + (redDelta & 0xfffff800);
    const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return static_cast<unsigned short>(blended);
}

unsigned int BlendPair565Alpha5(unsigned int dstPair, unsigned short srcPixel,
                                 int alpha) {
    const unsigned int srcPair =
        static_cast<unsigned int>(srcPixel) | (static_cast<unsigned int>(srcPixel) << 16);
    const unsigned int alpha5 = static_cast<unsigned int>(alpha >> 3);
    const unsigned int inverseAlpha5 = 0x1fu - alpha5;
    const unsigned int lowTerms =
        ((((dstPair & 0x07e0f81fu) * inverseAlpha5) + ((srcPair & 0x07e0f81fu) * alpha5)) >> 5) &
        0x07e0f81fu;
    const unsigned int highTerms = ((((dstPair >> 5) & 0x07c0f83fu) * inverseAlpha5) +
                                     (((srcPair >> 5) & 0x07c0f83fu) * alpha5)) &
                                    0xf81f07e0u;
    return lowTerms | highTerms;
}

unsigned short BlendPixel555Alpha8(unsigned short dstPixel, unsigned short srcPixel,
                                  int alpha) {
    const int dstColor = static_cast<short>(dstPixel);
    const int srcColor = srcPixel;
    const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
    int blended = dstColor + (redDelta & 0xfffffc00);
    const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
    const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return static_cast<unsigned short>(blended);
}

unsigned int BlendPair555Alpha5(unsigned int dstPair, unsigned short srcPixel,
                                 int alpha) {
    const unsigned int srcPair =
        static_cast<unsigned int>(srcPixel) | (static_cast<unsigned int>(srcPixel) << 16);
    const unsigned int alpha5 = static_cast<unsigned int>(alpha >> 3);
    const unsigned int inverseAlpha5 = 0x1fu - alpha5;
    const unsigned int lowTerms =
        ((((dstPair & 0x03e07c1fu) * inverseAlpha5) + ((srcPair & 0x03e07c1fu) * alpha5)) >> 5) &
        0x03e07c1fu;
    const unsigned int highTerms = ((((dstPair >> 5) & 0x03e0f81fu) * inverseAlpha5) +
                                     (((srcPair >> 5) & 0x03e0f81fu) * alpha5)) &
                                    0x7c1f03e0u;
    return highTerms | lowTerms;
}

unsigned short BlendPixel555ConstAlphaMap(unsigned short dstPixel, unsigned short srcPixel,
                                         int alpha) {
    const int dstColor = static_cast<short>(dstPixel);
    const int srcColor = srcPixel;
    const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
    const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
    const int blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * alpha) >> 8;
    return static_cast<unsigned short>(dstColor + (redDelta & 0xfffffc00) +
                                      (greenDelta & 0xffffffe0) + blueDelta);
}

void SpanCopy16FromTex16Forward(int texU, int texV, int pixelCount,
                                int texVShift) {
    unsigned short *dst = g_spanCurrentDst16;
    for (int i = 0; i < pixelCount; ++i) {
        dst[i] = SpanTex16Sample(texU, texV, texVShift, g_spanActiveTexUMask);
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }
}
} // namespace

// Reimplements 0x49e200: zRndr::FogBlendSpan565Scalar
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan565Scalar(unsigned short *pixels,
                                                           int pixelCount,
                                                           int fogCoordFixed24,
                                                           int fogCoordStepFixed24) {
    unsigned int fogCoord = static_cast<unsigned int>(fogCoordFixed24);
    const unsigned int fogStep = static_cast<unsigned int>(fogCoordStepFixed24);
    unsigned int pairCount = static_cast<unsigned int>(pixelCount) >> 1;

    if ((pixelCount & 1) != 0) {
        *pixels = FogBlendPixel565(*pixels, fogCoord);
        ++pixels;
        fogCoord += fogStep;
    }

    const unsigned int pairFogStep = fogStep + fogStep;
    while (pairCount != 0) {
        const unsigned int packedPixels =
            static_cast<unsigned int>(pixels[0]) | (static_cast<unsigned int>(pixels[1]) << 16);
        const unsigned int blended = FogBlendPair565(packedPixels, fogCoord);
        pixels[0] = static_cast<unsigned short>(blended);
        pixels[1] = static_cast<unsigned short>(blended >> 16);

        pixels += 2;
        fogCoord += pairFogStep;
        --pairCount;
    }
}

// Reimplements 0x49e300: zRndr::FogBlendSpan555Scalar
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan555Scalar(unsigned short *pixels,
                                                           int pixelCount,
                                                           int fogCoordFixed24,
                                                           int fogCoordStepFixed24) {
    unsigned int fogCoord = static_cast<unsigned int>(fogCoordFixed24);
    const unsigned int fogStep = static_cast<unsigned int>(fogCoordStepFixed24);
    unsigned int pairCount = static_cast<unsigned int>(pixelCount) >> 1;

    if ((pixelCount & 1) != 0) {
        *pixels = FogBlendPixel555(*pixels, fogCoord);
        ++pixels;
        fogCoord += fogStep;
    }

    const unsigned int pairFogStep = fogStep + fogStep;
    while (pairCount != 0) {
        const unsigned int packedPixels =
            static_cast<unsigned int>(pixels[0]) | (static_cast<unsigned int>(pixels[1]) << 16);
        const unsigned int blended = FogBlendPair555(packedPixels, fogCoord);
        pixels[0] = static_cast<unsigned short>(blended);
        pixels[1] = static_cast<unsigned short>(blended >> 16);

        pixels += 2;
        fogCoord += pairFogStep;
        --pairCount;
    }
}

// Reimplements 0x49e400: zRndr::FogBlendSpan565Mmx
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan565Mmx(unsigned short *pixels,
                                                        int pixelCount,
                                                        int fogCoordFixed24,
                                                        int fogCoordStepFixed24) {
    FogBlendSpanMmxCore(pixels, pixelCount, fogCoordFixed24, fogCoordStepFixed24, 11, 3,
                        FogBlendSpan565Scalar);
}

// Reimplements 0x49e560: zRndr::FogBlendSpan555Mmx
RECOIL_NOINLINE void RECOIL_FASTCALL FogBlendSpan555Mmx(unsigned short *pixels,
                                                        int pixelCount,
                                                        int fogCoordFixed24,
                                                        int fogCoordStepFixed24) {
    FogBlendSpanMmxCore(pixels, pixelCount, fogCoordFixed24, fogCoordStepFixed24, 10, 2,
                        FogBlendSpan555Scalar);
}

// Reimplements 0x49e6c0: zRndr::SpanCopy16FromTex16SwitchVShift
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromTex16SwitchVShift(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift) {
    if (texVShift < 10 || texVShift > 17) {
        return;
    }

    const int texUMask = (1 << (20 - texVShift)) - 1;
    for (int i = 0; i < pixelCount; ++i) {
        g_spanCurrentDst16[pixelCount - i - 1] = SpanTex16Sample(texU, texV, texVShift, texUMask);
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }
}

// Reimplements 0x49b7e0: zRndr::SpanMasked16FromTex16SwitchVShift
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromTex16SwitchVShift(int texU,
                                                                       int texV,
                                                                       int pixelCount,
                                                                       int texVShift) {
    if (texVShift < 10 || texVShift > 17) {
        return;
    }

    const int texUMask = (1 << (20 - texVShift)) - 1;
    for (int i = 0; i < pixelCount; ++i) {
        const unsigned short source = SpanTex16Sample(texU, texV, texVShift, texUMask);
        if (source != 0) {
            g_spanCurrentDst16[pixelCount - i - 1] = source;
        }
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }
}

// Reimplements 0x49ea40: zRndr::SpanMmxSetTexUvMasksAndVShift
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMmxSetTexUvMasksAndVShift(int texVShift) {
    g_mmxVShiftCounts[0] = texVShift;
    g_mmxVShiftCounts[1] = 0;
    g_mmxVMask[0] = g_spanActiveTexVMask;
    g_mmxVMask[1] = g_spanActiveTexVMask;
    g_mmxUMask[0] = g_spanActiveTexUMask << 20;
    g_mmxUMask[1] = g_spanActiveTexUMask << 20;
}

// Reimplements 0x49ea80: zRndr::SpanCopy16FromTex16
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromTex16(int texU, int texV,
                                                         int pixelCount,
                                                         int texVShift) {
    if (((unsigned int)(g_spanCurrentDst16) & 3u) != 0) {
        *g_spanCurrentDst16 = SpanTex16Sample(texU, texV, texVShift, g_spanActiveTexUMask);
        ++g_spanCurrentDst16;
        --pixelCount;
        if (pixelCount == 0) {
            return;
        }
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }

    g_mmxVPair[1] = texV;
    g_mmxVPair[0] = texV + g_spanTexVAdvance;
    g_mmxUPair[1] = texU;
    g_mmxUPair[0] = texU + g_spanTexUAdvance;
    g_mmxVStepDup2[0] = g_spanTexVAdvance * 2;
    g_mmxVStepDup2[1] = g_spanTexVAdvance * 2;
    g_mmxUStepDup2[0] = g_spanTexUAdvance * 2;
    g_mmxUStepDup2[1] = g_spanTexUAdvance * 2;

    SpanCopy16FromTex16Forward(texU, texV, pixelCount, texVShift);
}

// Reimplements 0x49ec20: zRndr::SpanCopy16FromTex16ExplicitVShift
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromTex16ExplicitVShift(int texU,
                                                                       int texV,
                                                                       int pixelCount,
                                                                       int texVShift) {
    if (((unsigned int)(g_spanCurrentDst16) & 3u) != 0) {
        *g_spanCurrentDst16 = SpanTex16Sample(texU, texV, texVShift, g_spanActiveTexUMask);
        ++g_spanCurrentDst16;
        --pixelCount;
        if (pixelCount == 0) {
            return;
        }
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }

    g_mmxVPair[1] = texV;
    g_mmxVPair[0] = texV + g_spanTexVAdvance;
    g_mmxUPair[1] = texU;
    g_mmxUPair[0] = texU + g_spanTexUAdvance;
    g_mmxVStepDup2[0] = g_spanTexVAdvance * 2;
    g_mmxVStepDup2[1] = g_spanTexVAdvance * 2;
    g_mmxUStepDup2[0] = g_spanTexUAdvance * 2;
    g_mmxUStepDup2[1] = g_spanTexUAdvance * 2;

    SpanCopy16FromTex16Forward(texU, texV, pixelCount, texVShift);
}

// Reimplements 0x49edc0: zRndr::SpanCopy16FromPal8SwitchVShift
RECOIL_NOINLINE void RECOIL_FASTCALL SpanCopy16FromPal8SwitchVShift(int texU,
                                                                    int texV,
                                                                    int pixelCount,
                                                                    int texVShift) {
    if (texVShift < 10 || texVShift > 17) {
        return;
    }

    const int texUMask = (1 << (20 - texVShift)) - 1;
    for (int i = 0; i < pixelCount; ++i) {
        g_spanCurrentDst16[pixelCount - i - 1] =
            SpanPal8SampleExpanded(texU, texV, texVShift, texUMask);
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }
}

// Reimplements 0x49bbf0: zRndr::SpanMasked16FromPal8SwitchVShift
RECOIL_NOINLINE void RECOIL_FASTCALL SpanMasked16FromPal8SwitchVShift(int texU,
                                                                      int texV,
                                                                      int pixelCount,
                                                                      int texVShift) {
    if (texVShift < 10 || texVShift > 17) {
        return;
    }

    const int texUMask = (1 << (20 - texVShift)) - 1;
    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, texUMask);
        const unsigned char source = g_spanActiveTexels[sourceIndex];
        if (source != 0) {
            g_spanCurrentDst16[pixelCount - i - 1] = g_spanActiveTexPalette[source];
        }
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }
}

// Reimplements 0x49f180: zRndr::SpanShade16FromPal8SwitchVShift
RECOIL_NOINLINE void RECOIL_FASTCALL SpanShade16FromPal8SwitchVShift(int texU,
                                                                     int texV,
                                                                     int pixelCount,
                                                                     int texVShift) {
    if (texVShift < 10 || texVShift > 17) {
        return;
    }

    const int texUMask = (1 << (20 - texVShift)) - 1;
    for (int i = 0; i < pixelCount; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, texUMask);
        const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
        const int paletteIndex = g_spanActiveTexels[sourceIndex] + shadeBucket;
        g_spanActiveShadeFixed16 =
            static_cast<int>(static_cast<unsigned int>(g_spanActiveShadeFixed16) +
                                      static_cast<unsigned int>(g_spanActiveShadeStepFixed16));
        g_spanCurrentDst16[pixelCount - i - 1] = g_spanActiveTexPalette[paletteIndex];
        texU += g_spanTexUAdvance;
        texV += g_spanTexVAdvance;
    }
}

// Reimplements 0x49b1e0: zRndr::FogColor_SetRgb01Clamped
RECOIL_NOINLINE void RECOIL_FASTCALL FogColor_SetRgb01Clamped(zColorRgb *color) {
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

    const int red = static_cast<int>(color->red * 255.0f + 0.5f);
    const int green = static_cast<int>(color->green * 255.0f + 0.5f);
    const int blue = static_cast<int>(color->blue * 255.0f + 0.5f);
    SpanAlphaBlend565_Mmx_FromPal8(
        &g_fogColorParams,
        (red << g_zVideo_PixelPack_RShift) & static_cast<int>(g_zVideo_PixelPack_RMask),
        (green << g_zVideo_PixelPack_GShift) & static_cast<int>(g_zVideo_PixelPack_GMask),
        blue >> g_zVideo_PixelPack_BShiftTo8);
}

// Reimplements 0x49b350: zRndr::SetFogTargetColorRgb01Clamped
RECOIL_NOINLINE void RECOIL_FASTCALL SetFogTargetColorRgb01Clamped(zColorRgb *color) {
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

    const int red = static_cast<int>(color->red * 255.0f + 0.5f);
    const int green = static_cast<int>(color->green * 255.0f + 0.5f);
    const int blue = static_cast<int>(color->blue * 255.0f + 0.5f);
    SpanAlphaBlend565_Mmx_FromPal8(
        &g_fogParamsActive,
        (red << g_zVideo_PixelPack_RShift) & static_cast<int>(g_zVideo_PixelPack_RMask),
        (green << g_zVideo_PixelPack_GShift) & static_cast<int>(g_zVideo_PixelPack_GMask),
        blue >> g_zVideo_PixelPack_BShiftTo8);
}

// Reimplements 0x49b4c0: zRndr::CommitDirectFogParamsIfChanged
RECOIL_NOINLINE void RECOIL_CDECL CommitDirectFogParamsIfChanged() {
    CommitFogParamsIfChanged(g_fogTargetParamsDirect);
}

// Reimplements 0x49b530: zRndr::CommitFogColorParamsIfChanged
RECOIL_NOINLINE void RECOIL_CDECL CommitFogColorParamsIfChanged() {
    CommitFogParamsIfChanged(g_fogColorParams);
}

// Reimplements 0x49b710: zRndr::CommitStagedFogParamsIfChanged
RECOIL_NOINLINE void RECOIL_CDECL CommitStagedFogParamsIfChanged() {
    CommitFogParamsIfChanged(g_fogTargetParamsStaged);
}

// Reimplements 0x49b780: zRndr::BlendPackedColor565WithFogInPlace
RECOIL_NOINLINE void RECOIL_FASTCALL BlendPackedColor565WithFogInPlace(int *ioPackedColor,
                                                                       int blend255) {
    const int packedColor = *ioPackedColor;
    const int redMask = static_cast<int>(g_pixelPackRedMask);
    const int greenMask = static_cast<int>(g_pixelPackGreenMask);
    const int blueMask = static_cast<int>(g_pixelPackBlueMask);

    const int redDelta =
        ((g_fogParamsActive.packedColorRed - (redMask & packedColor)) * blend255) >> 8;
    const int greenDelta =
        ((g_fogParamsActive.packedColorGreen - (greenMask & packedColor)) * blend255) >> 8;
    const int blueDelta =
        ((g_fogParamsActive.packedColorBlue - (blueMask & packedColor)) * blend255) >> 8;

    *ioPackedColor = packedColor + (redDelta & redMask) + (greenDelta & greenMask) + blueDelta;
}

// Reimplements 0x49a910: zRndr::LensFlare_ResetSampleQueue
RECOIL_NOINLINE void RECOIL_CDECL LensFlare_ResetSampleQueue() {
    g_lensFlareSampleQueueCount = 0;
}

namespace {
unsigned short BlendPacked565(unsigned short from, unsigned short to, int alpha) {
    const int red =
        ((from >> 11) & 0x1f) + ((((to >> 11) & 0x1f) - ((from >> 11) & 0x1f)) * alpha >> 8);
    const int green =
        ((from >> 5) & 0x3f) + ((((to >> 5) & 0x3f) - ((from >> 5) & 0x3f)) * alpha >> 8);
    const int blue = (from & 0x1f) + (((to & 0x1f) - (from & 0x1f)) * alpha >> 8);
    return static_cast<unsigned short>(((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1f));
}

unsigned short BlendPacked555(unsigned short from, unsigned short to, int alpha) {
    const int red =
        ((from >> 10) & 0x1f) + ((((to >> 10) & 0x1f) - ((from >> 10) & 0x1f)) * alpha >> 8);
    const int green =
        ((from >> 5) & 0x1f) + ((((to >> 5) & 0x1f) - ((from >> 5) & 0x1f)) * alpha >> 8);
    const int blue = (from & 0x1f) + (((to & 0x1f) - (from & 0x1f)) * alpha >> 8);
    return static_cast<unsigned short>(((red & 0x1f) << 10) | ((green & 0x1f) << 5) | (blue & 0x1f));
}

unsigned short BlendLensFlarePixel(unsigned short from, unsigned short to, int alpha) {
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

// Reimplements 0x498cb0: zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer
RECOIL_NOINLINE void RECOIL_FASTCALL LensFlare_DrawQueuedSample16_ClippedFramebuffer(
    LensFlareSamplePartial *sample, int yOffsetPixels, float screenScale) {
    if (sample == 0 || g_frameBuffer == 0) {
        return;
    }

    zRndr_LensFlareSource *lensFlareSource = (zRndr_LensFlareSource *)(
        static_cast<unsigned int>(sample->lensFlareSource));
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

    const int y = static_cast<int>(sample->y * screenScale) + yOffsetPixels;
    const int x = static_cast<int>(sample->x * screenScale);
    if (x < 0 || y < 0 || x > g_activeRegionWidth || y > g_activeRegionHeight) {
        return;
    }

    unsigned short packedColor = static_cast<unsigned short>(sample->packedColor16);
    unsigned char *frameBase = static_cast<unsigned char *>(g_frameBuffer);
    unsigned short *pixel =
        (unsigned short *)(frameBase + static_cast<int>(y) * g_pitchBytes +
                                          static_cast<int>(x) * sizeof(unsigned short));

    if (g_overlayBlendEnabled != 0) {
        const int overlayAlpha = static_cast<int>(g_overlayBlendAlpha * 255.0);
        packedColor = BlendLensFlarePixel(
            packedColor, static_cast<unsigned short>(g_overlayBlendPackedColor16), overlayAlpha);
    }

    if (!blendTowardFramebuffer) {
        *pixel = packedColor;
        return;
    }

    const int fadeAlpha = static_cast<int>(
        (lensFlareSource->depthFadeInvZMax - reciprocalZ) * lensFlareSource->depthFadeScale);
    if (g_pixelPackGreenBits == 6 ? fadeAlpha <= 3 : fadeAlpha <= 7) {
        return;
    }

    if (fadeAlpha >= 0xfc) {
        *pixel = packedColor;
        return;
    }

    *pixel = BlendLensFlarePixel(*pixel, packedColor, fadeAlpha);
}

// Reimplements 0x49a8c0: zRndr::LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer
// (D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
LensFlare_DrawQueuedSamplesScaled16_ClippedFramebuffer(int yOffsetPixels,
                                                       float screenScale) {
    {
    for (int sampleIndex = 0; sampleIndex < g_lensFlareSampleQueueCount; ++sampleIndex) {
        LensFlare_DrawQueuedSample16_ClippedFramebuffer(&g_lensFlareSampleQueue[sampleIndex],
                                                        yOffsetPixels, screenScale);
    }
    }

    g_lensFlareSampleQueueCount = 0;
    g_overlayBlendEnabled = 0;
}

void SpanOcclusionInsertPendingSpanSorted(SpanNodePartial **spanList, int columnIndex,
                                          int *spanCount) {
    InsertPendingSpanSorted(spanList, columnIndex, spanCount);
}

void SpanOcclusionInsertPendingSpanWithDepthTest(SpanNodePartial **spanList, int columnIndex,
                                                 int *spanCount) {
    InsertPendingSpanWithDepthTest(spanList, columnIndex, spanCount);
}

void SpanOcclusionInsertPendingSpanNoDepthTest(SpanNodePartial **spanList, int columnIndex,
                                               int *spanCount) {
    InsertPendingSpanNoDepthTest(spanList, columnIndex, spanCount);
}

void SpanOcclusionBuildVisibleSpanListWithDepthTest(SpanNodePartial **spanList, int columnIndex,
                                                    int *spanCount) {
    BuildVisibleSpanListWithDepthTest(spanList, columnIndex, spanCount);
}
} // namespace zRndr

static float zRndrSpanDepthAtXByPartsLocal(int sampleXMin, float invDepth, float depthSlope,
                                           int x) {
    return invDepth + static_cast<float>(x - sampleXMin) * depthSlope;
}

// Reimplements 0x490ae0: zRndr_SpanOcclusion_InsertSpanNode_Local
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_InsertSpanNode_Local(
    zRndr::SpanNodePartial **spanList, int columnIndex, int *spanCount) {
    zRndr::SpanOcclusionInsertPendingSpanWithDepthTest(spanList, columnIndex, spanCount);
}

// Reimplements 0x4912a0: zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest(
    zRndr::SpanNodePartial **spanList, int columnIndex, int *spanCount) {
    zRndr::SpanOcclusionInsertPendingSpanNoDepthTest(spanList, columnIndex, spanCount);
}

// Reimplements 0x491840: zRndr_SpanOcclusion_BuildSpanList
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_BuildSpanList(
    zRndr::SpanNodePartial **spanList, int columnIndex, int *spanCount) {
    zRndr::SpanOcclusionBuildVisibleSpanListWithDepthTest(spanList, columnIndex, spanCount);
}

// Reimplements 0x491da0: zRndr_SpanOcclusion_BuildSpanListFast
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_BuildSpanListFast(
    zRndr::SpanNodePartial **spanList, int, int *spanCount) {
    zRndr::g_spanAllocCursor->next = 0;
    spanList[0] = zRndr::g_spanAllocCursor;
    *spanCount = 1;
    ++zRndr::g_spanAllocCursor;
}

// Reimplements 0x491dd0: zRndr_SpanOcclusion_TestColumnVisibility
RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_SpanOcclusion_TestColumnVisibility(int columnIndex, int *isVisible) {
    *isVisible = 0;
    if (zRndr::g_spanColumnHeadTable == 0 || zRndr::g_spanAllocCursor == 0 ||
        columnIndex < 0 || columnIndex >= zRndr::g_spanColumnCountPadded) {
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
        if (zRndr_SpanOcclusion_TestSpanDepthOrderPair(&candidate, &occluder) != 0) {
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
                    zRndr::g_spanAllocCursor->sampleXMin, zRndr::g_spanAllocCursor->invDepth,
                    zRndr::g_spanAllocCursor->depthSlope, candidate.sampleXMin);
            }
        } else if (occluder.sampleXMin <= candidate.sampleXMax) {
            *isVisible = 1;
            return;
        }

        current = current->next;
    }

    *isVisible = 1;
}

// Reimplements 0x498c40: zRndr_SpanOcclusion_TestPointVisibility
RECOIL_NOINLINE int RECOIL_FASTCALL
zRndr_SpanOcclusion_TestPointVisibility(zVec3 *samplePoint) {
    if (samplePoint == 0 || zRndr::g_spanAllocCursor == 0) {
        return 0;
    }

    zRndr::g_spanAllocCursor->invDepth = samplePoint->z;
    zRndr::g_spanAllocCursor->invDepthStep = samplePoint->z;
    zRndr::g_spanAllocCursor->depthSlope = 0.0f;
    zRndr::g_spanAllocCursor->sampleXMin = static_cast<int>(samplePoint->x);
    zRndr::g_spanAllocCursor->sampleXMax = zRndr::g_spanAllocCursor->sampleXMin;

    int isVisible = 0;
    zRndr_SpanOcclusion_TestColumnVisibility(static_cast<int>(samplePoint->y), &isVisible);
    return isVisible > 0 ? 1 : 0;
}

// Reimplements 0x498f90: zRndr_SpanOcclusion_TestSample
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SpanOcclusion_TestSample(int x, int y,
                                                                    int color16) {
    if (zRndr::g_pfnPointOpActive != 0) {
        zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, y, x, color16);
    }
}

// Reimplements 0x499020: zRndr_DrawCircleOctants16_Framebuffer
RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_DrawCircleOctants16_Framebuffer(int y, int x, int packedColor) {
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + y,
                              g_zRndr_CircleCenterX + x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + y,
                              g_zRndr_CircleCenterX - x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - y,
                              g_zRndr_CircleCenterX + x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - y,
                              g_zRndr_CircleCenterX - x, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + x,
                              g_zRndr_CircleCenterX + y, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY + x,
                              g_zRndr_CircleCenterX - y, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - x,
                              g_zRndr_CircleCenterX - y, packedColor);
    zRndr::g_pfnPointOpActive(zRndr::g_frameBuffer, g_zRndr_CircleCenterY - x,
                              g_zRndr_CircleCenterX + y, packedColor);
}

// Reimplements 0x498fb0: zRndr_DrawCircleOutline16_Framebuffer
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawCircleOutline16_Framebuffer(int centerX,
                                                                           int centerY,
                                                                           int radius,
                                                                           int packedColor,
                                                                           int auxArg) {
    int x = radius;
    int y = 0;
    int decisionVar = 1 - x;
    if (x <= 0) {
        return;
    }

    g_zRndr_CircleCenterX = centerX;
    g_zRndr_CircleCenterY = centerY;
    g_zRndr_CircleDrawAuxArg = auxArg;

    zRndr_DrawCircleOctants16_Framebuffer(0, x, packedColor);
    do {
        if (decisionVar >= 0) {
            const int delta = y - x;
            --x;
            decisionVar += (delta << 1) + 5;
        } else {
            decisionVar += (y << 1) + 3;
        }

        ++y;
        zRndr_DrawCircleOctants16_Framebuffer(y, x, packedColor);
    } while (x > y);
}

// Reimplements 0x4992b0: zRndr_PlotPixel16
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_PlotPixel16(unsigned short *dstPixels, int y,
                                                       int x, int color16) {
    const unsigned int pitchWords = static_cast<unsigned int>(zRndr::g_pitchBytes) >> 1;
    dstPixels[pitchWords * y + x] = static_cast<unsigned short>(color16);
}

// Reimplements 0x4992d0: zRndr_DrawLine16
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawLine16(unsigned short *dstPixels, int x0,
                                                      int y0, int x1,
                                                      int y1, int color16) {
    const unsigned int pitchWordsUnsigned = ((unsigned int)zRndr::g_pitchBytes) >> 1;
    int rowStep = static_cast<int>(pitchWordsUnsigned);
    int startIndex = static_cast<int>(pitchWordsUnsigned * y0 + x0);

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

    unsigned short *cursor = &dstPixels[startIndex];
    const unsigned short packedColor = static_cast<unsigned short>(color16);

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

// Reimplements 0x4993a0: zRndr_DrawLine16_Segmented
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawLine16_Segmented(unsigned short *dstPixels,
                                                                int x0, int y0,
                                                                int x1, int y1,
                                                                int color16,
                                                                int segmentCount) {
    const unsigned int pitchWordsUnsigned = static_cast<unsigned int>(zRndr::g_pitchBytes) >> 1;
    int rowStep = static_cast<int>(pitchWordsUnsigned);
    int startIndex = static_cast<int>(pitchWordsUnsigned * y0 + x0);

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

    unsigned short *cursor = &dstPixels[startIndex];
    const unsigned short packedColor = static_cast<unsigned short>(color16);
    int drawSegment = 1;
    int segmentCounter = 0;

    if (dx > dy) {
        const int segmentLimit = (dx + 1) / segmentCount;
        int error = dx >> 1;
        int count = dx + 1;
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

            const int previousCounter = segmentCounter;
            ++segmentCounter;
            if (previousCounter >= segmentLimit) {
                segmentCounter = 0;
                drawSegment = drawSegment == 0 ? 1 : 0;
            }

            --count;
        } while (count != 0);
        return;
    }

    const int segmentLimit = (dy + 1) / segmentCount;
    int error = dy >> 1;
    int count = dy + 1;
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

        const int previousCounter = segmentCounter;
        ++segmentCounter;
        if (previousCounter >= segmentLimit) {
            segmentCounter = 0;
            drawSegment = drawSegment == 0 ? 1 : 0;
        }

        --count;
    } while (count != 0);
}

// Reimplements 0x499500: zRndr_DrawLine16_Clipped
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawLine16_Clipped(unsigned short *dstPixels,
                                                              const zRndr_LineClipRect2I *clipRect,
                                                              int x0, int y0,
                                                              int x1, int y1,
                                                              int color16) {
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
        const float yPerX = dx != 0 ? static_cast<float>(dy) / static_cast<float>(dx) : 0.0f;
        const float xPerY = dy != 0 ? static_cast<float>(dx) / static_cast<float>(dy) : 0.0f;

        if (x0 < clipRect->left) {
            y0 += static_cast<int>(static_cast<float>(clipRect->left - x0) * yPerX);
            x0 = clipRect->left;
        } else if (x0 > clipRect->right) {
            y0 += static_cast<int>(static_cast<float>(clipRect->right - x0) * yPerX);
            x0 = clipRect->right;
        }

        if (x1 < clipRect->left) {
            y1 += static_cast<int>(static_cast<float>(clipRect->left - x1) * yPerX);
            x1 = clipRect->left;
        } else if (x1 > clipRect->right) {
            y1 += static_cast<int>(static_cast<float>(clipRect->right - x1) * yPerX);
            x1 = clipRect->right;
        }

        if (y0 < clipRect->top) {
            if (y1 < clipRect->top) {
                return;
            }
            x0 += static_cast<int>(static_cast<float>(clipRect->top - y0) * xPerY);
            y0 = clipRect->top;
        } else if (y0 > clipRect->bottom) {
            if (y1 > clipRect->bottom) {
                return;
            }
            x0 += static_cast<int>(static_cast<float>(clipRect->bottom - y0) * xPerY);
            y0 = clipRect->bottom;
        }

        if (y1 < clipRect->top) {
            x1 += static_cast<int>(static_cast<float>(clipRect->top - y1) * xPerY);
            y1 = clipRect->top;
        } else if (y1 > clipRect->bottom) {
            x1 += static_cast<int>(static_cast<float>(clipRect->bottom - y1) * xPerY);
            y1 = clipRect->bottom;
        }

        dx = x1 - x0;
        dy = y1 - y0;
    }

    const unsigned int pitchWordsUnsigned = static_cast<unsigned int>(zRndr::g_pitchBytes) >> 1;
    int rowStep = static_cast<int>(pitchWordsUnsigned);
    int startIndex = static_cast<int>(pitchWordsUnsigned * y0 + x0);

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
    const unsigned short packedColor = static_cast<unsigned short>(color16);

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

// Reimplements 0x4997d0: zRndr_FillSpan16Opaque
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FillSpan16Opaque(int packedColor16,
                                                            int pixelCount) {
    unsigned short *cursor = zRndr::g_spanCurrentDst16 + pixelCount;
    const unsigned short color = static_cast<unsigned short>(packedColor16);
    while (pixelCount > 0) {
        --cursor;
        *cursor = color;
        --pixelCount;
    }
}

// Reimplements 0x499810: zRndr_FillSpan555Solid
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FillSpan555Solid(int packedColor16,
                                                            int blendAlpha,
                                                            int pixelCount) {
    unsigned short *cursor = zRndr::g_spanCurrentDst16;
    do {
        if (blendAlpha > 7) {
            if (blendAlpha >= 0xfc) {
                *cursor = static_cast<unsigned short>(packedColor16);
            } else {
                const int dst = static_cast<short>(*cursor);
                const int redDelta =
                    ((((packedColor16 & 0x7c00) - (dst & 0x7c00)) * blendAlpha) >> 8) & 0xfffffc00;
                const int greenDelta =
                    ((((packedColor16 & 0x03e0) - (dst & 0x03e0)) * blendAlpha) >> 8) & 0xffffffe0;
                const int blueDelta =
                    (((packedColor16 & 0x001f) - (dst & 0x001f)) * blendAlpha) >> 8;
                *cursor = static_cast<unsigned short>(*cursor + redDelta + greenDelta + blueDelta);
            }
        }

        ++cursor;
        --pixelCount;
    } while (pixelCount != 0);
}

// Reimplements 0x4998a0: zRndr_FillSpan565Solid
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FillSpan565Solid(int packedColor16,
                                                            int blendAlpha,
                                                            int pixelCount) {
    unsigned short *cursor = zRndr::g_spanCurrentDst16;
    do {
        if (blendAlpha > 3) {
            if (blendAlpha >= 0xfc) {
                *cursor = static_cast<unsigned short>(packedColor16);
            } else {
                const int dst = static_cast<short>(*cursor);
                const int greenDelta =
                    ((((packedColor16 & 0x07e0) - (dst & 0x07e0)) * blendAlpha) >> 8) & 0xffffffe0;
                const int redDelta =
                    ((((packedColor16 & 0xf800) - (dst & 0xf800)) * blendAlpha) >> 8) & 0xfffff800;
                const int redAdjusted = dst + redDelta;
                const int blueDelta =
                    (((packedColor16 & 0x001f) - (redAdjusted & 0x001f)) * blendAlpha) >> 8;
                *cursor = static_cast<unsigned short>(redAdjusted + greenDelta + blueDelta);
            }
        }

        ++cursor;
        --pixelCount;
    } while (pixelCount != 0);
}

RECOIL_NOINLINE int RECOIL_FASTCALL zRndr_SpanOcclusion_TestSpanDepthOrderPair(
    zRndr::SpanNodePartial *lhs, zRndr::SpanNodePartial *rhs) {
    if (lhs == 0 || rhs == 0) {
        return 0;
    }

    if (rhs->sampleXMin == rhs->sampleXMax) {
        const float lhsDepth =
            zRndrSpanDepthAtXByPartsLocal(lhs->sampleXMin, lhs->invDepth, lhs->depthSlope,
                                rhs->sampleXMax);
        return lhsDepth * zRndr::g_spanDepthBiasPlusOne >= rhs->invDepth ? 1 : 0;
    }

    if (lhs->sampleXMin == lhs->sampleXMax) {
        const float rhsDepth =
            zRndrSpanDepthAtXByPartsLocal(rhs->sampleXMin, rhs->invDepth, rhs->depthSlope,
                                lhs->sampleXMax);
        return lhs->invDepth >= rhsDepth * zRndr::g_spanDepthBiasPlusOneInv ? 1 : 0;
    }

    const int overlapMin = MaxValue(lhs->sampleXMin, rhs->sampleXMin);
    const int overlapMax = MinValue(lhs->sampleXMax, rhs->sampleXMax);
    if (overlapMin > overlapMax) {
        return 0;
    }

    const float lhsStart =
        zRndrSpanDepthAtXByPartsLocal(lhs->sampleXMin, lhs->invDepth, lhs->depthSlope, overlapMin);
    const float lhsEnd =
        zRndrSpanDepthAtXByPartsLocal(lhs->sampleXMin, lhs->invDepth, lhs->depthSlope, overlapMax);
    const float rhsStart =
        zRndrSpanDepthAtXByPartsLocal(rhs->sampleXMin, rhs->invDepth, rhs->depthSlope, overlapMin);
    const float rhsEnd =
        zRndrSpanDepthAtXByPartsLocal(rhs->sampleXMin, rhs->invDepth, rhs->depthSlope, overlapMax);
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
        zRndrSpanDepthAtXByPartsLocal(lhs->sampleXMin, lhs->invDepth, lhs->depthSlope, midX);
    const float rhsMid =
        zRndrSpanDepthAtXByPartsLocal(rhs->sampleXMin, rhs->invDepth, rhs->depthSlope, midX);
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

int RoundToFixed20(float value) {
    return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

int Fixed16FromFloat(float value) {
    const double scaled = static_cast<double>(value) * 65536.0;
    return static_cast<int>(scaled >= 0.0 ? scaled + 0.5 : scaled - 0.5);
}

int ScanlineStartFromY(float y) {
    return (Fixed16FromFloat(y) + 0x7fff) >> 16;
}

int ScanlineEndFromY(float y) {
    return (Fixed16FromFloat(y) - 0x8041) >> 16;
}

int SpanStartFromX(float x) {
    return (Fixed16FromFloat(x) + 0x7fff) >> 16;
}

int SpanEndFromX(float x) {
    return (Fixed16FromFloat(x) - 0x8001) >> 16;
}

Plane2f BuildPlaneFromTriangle(const zVec3 *triVerts, const float values[3]) {
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

Plane2f BuildScreenPlaneFromTriangle(const zVec3 *triVerts, const float values[3]) {
    Plane2f plane = BuildPlaneFromTriangle(triVerts, values);
    plane.base = values[0] - triVerts[0].x * plane.gradient.x - triVerts[0].y * plane.gradient.y;
    return plane;
}

TexturedPlanes BuildQueuedTexturePlanes(const zVec3 *clippedTriVerts, const zVec3 *triVerts,
                                        const zVec2 *triUVs, float imageWidth, float imageHeight) {
    TexturedPlanes planes = {0};
    const bool useClippedNearPlane = clippedTriVerts != 0 &&
                                     (clippedTriVerts[0].z < 10.0f ||
                                      clippedTriVerts[1].z < 10.0f || clippedTriVerts[2].z < 10.0f);

    if (useClippedNearPlane) {
        zMath_BuildPerspectiveTextureInterpolants(
            clippedTriVerts, triUVs, &planes.reciprocalZ.gradient, &planes.reciprocalZ.base,
            &planes.uOverZ.gradient, &planes.uOverZ.base, &planes.vOverZ.gradient,
            &planes.vOverZ.base);
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

        planes.reciprocalZ = BuildPlaneFromTriangle(triVerts, reciprocalValues);
        planes.uOverZ = BuildPlaneFromTriangle(triVerts, uValues);
        planes.vOverZ = BuildPlaneFromTriangle(triVerts, vValues);
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

float EvalPlane(const Plane2f &plane, float x, float y) {
    return x * plane.gradient.x + y * plane.gradient.y + plane.base;
}

int SelectPerspectiveChunkPixels(float minPositiveReciprocalZ, float reciprocalZStepX) {
    if (zRndr::g_perspectiveAdaptiveMinSpan == 0) {
        return MaxValue(1, zRndr::g_perspectiveTextureDeltaXPow2);
    }

    int chunkPixels = zRndr::g_perspectiveAdaptiveMaxSpan;
    if (reciprocalZStepX != 0.0f) {
        chunkPixels = static_cast<int>(fabs(
            minPositiveReciprocalZ * zRndr::g_perspectiveAdaptiveSlope / reciprocalZStepX));
    }

    chunkPixels = MinValue(chunkPixels, zRndr::g_perspectiveAdaptiveMaxSpan);
    chunkPixels = MaxValue(chunkPixels, zRndr::g_perspectiveAdaptiveMinSpan);
    return MaxValue(1, chunkPixels);
}

void DispatchTexturedSpanChunks(zRndr::TexturedQueuedSpanProc spanProc,
                                const TexturedPlanes &planes, const Plane2f *shadePlane,
                                zRndr::SpanNodePartial *span, int y,
                                int chunkPixels, float textureScale,
                                int texVShift) {
    int remaining = span->sampleXMax - span->sampleXMin + 1;
    int x = span->sampleXMin;
    while (remaining > 0) {
        const int count = MinValue(remaining, chunkPixels);
        const float startX = static_cast<float>(x);
        const float endX = static_cast<float>(x + count);
        const float sampleY = static_cast<float>(y);
        const float startInvZ = EvalPlane(planes.reciprocalZ, startX, sampleY);
        const float endInvZ = EvalPlane(planes.reciprocalZ, endX, sampleY);
        if (startInvZ == 0.0f || endInvZ == 0.0f) {
            x += count;
            remaining -= count;
            continue;
        }

        const float startU = EvalPlane(planes.uOverZ, startX, sampleY) / startInvZ;
        const float startV = EvalPlane(planes.vOverZ, startX, sampleY) / startInvZ;
        const float endU = EvalPlane(planes.uOverZ, endX, sampleY) / endInvZ;
        const float endV = EvalPlane(planes.vOverZ, endX, sampleY) / endInvZ;

        zRndr::g_spanTexUAdvance =
            RoundToFixed20((endU - startU) * textureScale / static_cast<float>(count));
        zRndr::g_spanTexVAdvance =
            RoundToFixed20((endV - startV) * textureScale / static_cast<float>(count));
        if (shadePlane != 0) {
            const float startShade =
                MaxValue(0.0f, MinValue(255.0f, EvalPlane(*shadePlane, startX, sampleY)));
            const float endShade =
                MaxValue(0.0f, MinValue(255.0f, EvalPlane(*shadePlane, endX, sampleY)));
            zRndr::g_spanActiveShadeFixed16 = RoundToFixed20(startShade * 65536.0f);
            zRndr::g_spanActiveShadeStepFixed16 =
                RoundToFixed20((endShade - startShade) * 65536.0f / static_cast<float>(count));
        }

        spanProc(RoundToFixed20(startU * textureScale), RoundToFixed20(startV * textureScale),
                 count, texVShift);

        zRndr::g_spanCurrentDst16 += count;
        x += count;
        remaining -= count;
    }
}
} // namespace

// Reimplements 0x492000: zRndr_RasterizePolyWithSpanList
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_RasterizePolyWithSpanList(zVec3 *vertices,
                                                                     zVec3 *planeVerts,
                                                                     int vertCount,
                                                                     int spanOpContext) {
    if (vertices == 0 || planeVerts == 0 || vertCount <= 0 ||
        zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

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

    const int firstScanline =
        static_cast<int>(floor(vertices[topVertexIndex].y + 0.5f));
    const int lastScanline =
        static_cast<int>(floor(vertices[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    zRndr::SpanBuildProc buildProc = zRndr::g_pfnBuildSpanList != 0
                                         ? zRndr::g_pfnBuildSpanList
                                         : zRndr_SpanOcclusion_InsertSpanNode_Local;

    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;

        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = vertices[i];
            const zVec3 &b = vertices[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        const int pairCount = intersectionCount & ~1;
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        for (int i_3517 = 0; i_3517 < pairCount; i_3517 += 2) {
            const int xMin =
                static_cast<int>(floor(intersections[i_3517] + 0.5f));
            const int xMax =
                static_cast<int>(ceil(intersections[i_3517 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowDepthBase = static_cast<float>(y) * invDepthSlopeY + invDepthBiasBase;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                (static_cast<float>(xMin) * invDepthSlopeX + rowDepthBase) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                (static_cast<float>(xMax) * invDepthSlopeX + rowDepthBase) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = invDepthSlopeX;

            int spanCount = 0;
            buildProc(visibleSpans, y, &spanCount);

            {
            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                if (pixelCount <= 0) {
                    continue;
                }

                const int byteOffset =
                    static_cast<int>(span->sampleXMin) * zRndr::g_bytesPerPixel;
                zRndr::g_spanCurrentDst16 =
                    (unsigned short *)(scanlineBase + byteOffset);
                if (zRndr::g_pfnSelectedSpanOp != 0) {
                    zRndr::g_pfnSelectedSpanOp(spanOpContext, pixelCount);
                }
            }
            }
        }
    }
}

// Reimplements 0x492f00: zRndr_DrawFlatImmediate
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawFlatImmediate(zVec3 *vertices, zVec3 *planeVertices,
                                                             int vertCount,
                                                             int flatSpanOpEdxArg,
                                                             int flatSpanOpEcxArg) {
    if (vertices == 0 || planeVertices == 0 || vertCount <= 0 ||
        zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0 ||
        zRndr::g_pfnFlatImmediateSpanOp == 0) {
        return;
    }

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

    const int firstScanline =
        static_cast<int>(floor(vertices[topVertexIndex].y + 0.5f));
    const int lastScanline =
        static_cast<int>(floor(vertices[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    zRndr::SpanBuildProc buildProc = zRndr::g_pfnBuildSpanListSecondary != 0
                                         ? zRndr::g_pfnBuildSpanListSecondary
                                         : zRndr_SpanOcclusion_InsertSpanNode_Local;

    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;

        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = vertices[i];
            const zVec3 &b = vertices[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_3650 = 0; i_3650 < pairCount; i_3650 += 2) {
            const int xMin =
                static_cast<int>(floor(intersections[i_3650] + 0.5f));
            const int xMax =
                static_cast<int>(ceil(intersections[i_3650 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowDepthBase = static_cast<float>(y) * invDepthSlopeY + invDepthBiasBase;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                (static_cast<float>(xMin) * invDepthSlopeX + rowDepthBase) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                (static_cast<float>(xMax) * invDepthSlopeX + rowDepthBase) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = invDepthSlopeX;

            int spanCount = 0;
            buildProc(visibleSpans, y, &spanCount);

            {
            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                const int pixelCount = span->sampleXMax - span->sampleXMin + 1;
                if (pixelCount <= 0) {
                    continue;
                }

                zRndr::g_spanCurrentDst16 = (unsigned short *)(
                    scanlineBase +
                    static_cast<int>(span->sampleXMin) * zRndr::g_bytesPerPixel);
                zRndr::g_pfnFlatImmediateSpanOp(flatSpanOpEcxArg, flatSpanOpEdxArg, pixelCount);
            }
            }
        }
    }
}

// Reimplements 0x4936d0: zRndr_RasterizePoly
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_RasterizePoly(zVec3 *vertices, int vertCount,
                                                         int spanOpContext) {
    if (vertices == 0 || vertCount <= 0 || zRndr::g_frameBuffer == 0 ||
        zRndr::g_pfnSelectedSpanOp == 0) {
        return;
    }

    zVec3 reducedVerts[0x40] = {0};
    int reducedCount = 1;
    reducedVerts[0] = vertices[0];
    for (int i = 1; i < vertCount && reducedCount < 0x40; ++i) {
        const zVec3 &previous = reducedVerts[reducedCount - 1];
        if (vertices[i].x == previous.x && vertices[i].y == previous.y) {
            continue;
        }

        reducedVerts[reducedCount++] = vertices[i];
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
    for (int i_3724 = 1; i_3724 < reducedCount; ++i_3724) {
        if (reducedVerts[i_3724].y < reducedVerts[topVertexIndex].y) {
            topVertexIndex = i_3724;
        }
        if (reducedVerts[i_3724].y >= reducedVerts[bottomVertexIndex].y) {
            bottomVertexIndex = i_3724;
        }
    }

    const int firstScanline =
        static_cast<int>(floor(reducedVerts[topVertexIndex].y + 0.5f));
    const int lastScanline =
        static_cast<int>(floor(reducedVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;
        for (int i = 0; i < reducedCount; ++i) {
            const zVec3 &a = reducedVerts[i];
            const zVec3 &b = reducedVerts[(i + 1) % reducedCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_3773 = 0; i_3773 < pairCount; i_3773 += 2) {
            const int xMin =
                static_cast<int>(floor(intersections[i_3773] + 0.5f));
            const int xMax =
                static_cast<int>(ceil(intersections[i_3773 + 1] - 0.5f));
            const int pixelCount = xMax - xMin;
            if (pixelCount <= 0) {
                continue;
            }

            zRndr::g_spanCurrentDst16 = (unsigned short *)(
                scanlineBase + static_cast<int>(xMin) * zRndr::g_bytesPerPixel);
            zRndr::g_pfnSelectedSpanOp(spanOpContext, pixelCount);
        }
    }
}

// Reimplements 0x499a20: zRndr_SubmitPolyWithSpanList
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SubmitPolyWithSpanList(
    zVec3 *entryVertices, zVec3 *entryPlaneVertices, int spanOpContext,
    int alpha255, int vertCount, int queueOverwrite) {
    const int kMaxQueuedPolys = 0x15e;
    const char *kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(0x400, kSourceFile, 0x9c,
                              " Not enough MAX_OVERWRITE_POLYS: need %d\n", queueIndex);
            return;
        }

        ++zRndr::g_overwriteQueueCount;
        zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
        cmd.hasClippedTriVerts = 0;
        memcpy(cmd.polyVerts, entryVertices,
                    static_cast<size_t>(vertCount) * sizeof(zVec3));
        memcpy(cmd.triVerts, entryPlaneVertices, 3 * sizeof(zVec3));
        cmd.alphaOrShadeF = static_cast<float>(alpha255);
        cmd.materialRef = 0;
        cmd.vertexCount = vertCount;
        cmd.shadeOrSpanMode = spanOpContext;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        return;
    }

    if (alpha255 >= 0xff) {
        zRndr_RasterizePolyWithSpanList(entryVertices, entryPlaneVertices, vertCount,
                                        spanOpContext);
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(0x400, kSourceFile, 0xb9, " Not enough MAX_TRANSPARENT_POLYS: need %d\n",
                          queueIndex);
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[queueIndex];
    memcpy(cmd.polyVerts, entryVertices, static_cast<size_t>(vertCount) * sizeof(zVec3));
    memcpy(cmd.triVerts, entryPlaneVertices, 3 * sizeof(zVec3));
    cmd.materialRef = 0;
    cmd.vertexCount = vertCount;
    cmd.shadeOrSpanMode = spanOpContext;
    cmd.alphaOrShadeBits = alpha255;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    ++zRndr::g_transparentQueueCount;
}

// Reimplements 0x499c40: zRndr_SubmitTexturedPolyUniformAlphaOrShade
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SubmitTexturedPolyUniformAlphaOrShade(
    zVec3 *projectedPolyVerts, zVec3 *clippedTriVerts, zVec3 *triData9f, zVec2 *triUVs,
    int vertexCount, zImage_TexDirEntryPartial *entry, float alphaOrShadeF,
    int queueOverwrite) {
    const int kMaxQueuedPolys = 0x15e;
    const char *kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(0x400, kSourceFile, 0xfa,
                              " Not enough MAX_OVERWRITE_POLYS: need %d\n", queueIndex);
            return;
        }

        ++zRndr::g_overwriteQueueCount;
        zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
        unsigned char *raw = (unsigned char *)(&cmd);
        *(int *)(raw + 0x000) = 1;
        cmd.vertexCount = vertexCount;
        cmd.materialRef = entry;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.alphaOrShadeF = alphaOrShadeF;
        memcpy(raw + 0x004, projectedPolyVerts,
                    static_cast<size_t>(vertexCount) * sizeof(zVec3));
        memcpy(raw + 0x328, triData9f, 3 * sizeof(zVec3));
        memcpy(raw + 0x358, triUVs, 3 * sizeof(zVec2));
        if (clippedTriVerts != 0) {
            memcpy(raw + 0x304, clippedTriVerts, 3 * sizeof(zVec3));
            *(int *)(raw + 0x47c) = 1;
        } else {
            *(int *)(raw + 0x47c) = 0;
        }
        *(int *)(raw + 0x488) = g_zRndr_ActivePaletteRemapKey;
        return;
    }

    zVidImagePartial *image = entry != 0 ? entry->image : 0;
    if ((image->formatFlagsPacked & 2) == 0 && alphaOrShadeF >= 1.0f) {
        zRndr_DrawTexturedQueuedAlpha(entry, projectedPolyVerts, clippedTriVerts, triData9f, triUVs,
                                      vertexCount, g_zRndr_ActivePaletteRemapKey);
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(0x400, kSourceFile, 0x126, " Not enough MAX_TRANSPARENT_POLYS: need %d\n",
                          queueIndex);
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[queueIndex];
    unsigned char *raw = (unsigned char *)(&cmd);
    cmd.vertexCount = vertexCount;
    cmd.materialRef = entry;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    if ((image->formatFlagsPacked & 2) != 0) {
        memcpy(&cmd.alphaOrShadeBits, &alphaOrShadeF, sizeof(float));
    } else {
        cmd.alphaOrShadeBits = static_cast<int>(alphaOrShadeF * 255.0f);
    }
    memcpy(raw + 0x008, projectedPolyVerts,
                static_cast<size_t>(vertexCount) * sizeof(zVec3));
    memcpy(raw + 0x32c, triData9f, 3 * sizeof(zVec3));
    memcpy(raw + 0x350, triUVs, 3 * sizeof(zVec2));
    if (clippedTriVerts != 0) {
        memcpy(raw + 0x308, clippedTriVerts, 3 * sizeof(zVec3));
        cmd.hasClippedTriVerts = 1;
    } else {
        cmd.hasClippedTriVerts = 0;
    }
    cmd.texKey = g_zRndr_ActivePaletteRemapKey;
    ++zRndr::g_transparentQueueCount;
}

// Reimplements 0x499ec0: zRndr_SubmitTexturedPolyPerVertexAlphaOrShade
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SubmitTexturedPolyPerVertexAlphaOrShade(
    zVec3 *projectedPolyVerts, zVec3 *clippedTriVerts, zVec3 *triData9f, zVec2 *triUVs,
    float *perVertexAlphaOrShadeF, int shadeOrSpanMode, int vertexCount,
    zImage_TexDirEntryPartial *entry, int preservePaletteRemapKey,
    int queueOverwrite) {
    const int kMaxQueuedPolys = 0x15e;
    const char *kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    zVidImagePartial *image = entry != 0 ? entry->image : 0;
    int texKey = g_zRndr_ActivePaletteRemapKey;
    int usingDerivedPaletteKey = 0;

    if (texKey == -1 && preservePaletteRemapKey == 0 && entry != 0 &&
        entry->image->paletteMetaPacked > 0) {
        texKey = zVid_PaletteRemap_FindRecipeIndexFromRgb(
            (zColorRgb *)(zRndr::g_fogParamsActive.colorRgb01));
        if (texKey >= 0) {
            int shadeBucket =
                static_cast<int>(perVertexAlphaOrShadeF[0] * 0.125f);
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
            zError::ReportOld(0x400, kSourceFile, 0x19e,
                              " Not enough MAX_OVERWRITE_POLYS: need %d\n", queueIndex);
            return;
        }

        ++zRndr::g_overwriteQueueCount;
        zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
        unsigned char *raw = (unsigned char *)(&cmd);
        *(int *)(raw + 0x000) = usingDerivedPaletteKey != 0 ? 1 : 2;
        cmd.vertexCount = vertexCount;
        cmd.materialRef = entry;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.alphaOrShadeF = static_cast<float>(shadeOrSpanMode);
        memcpy(raw + 0x004, projectedPolyVerts,
                    static_cast<size_t>(vertexCount) * sizeof(zVec3));
        memcpy(raw + 0x328, triData9f, 3 * sizeof(zVec3));
        memcpy(raw + 0x358, triUVs, 3 * sizeof(zVec2));
        if (usingDerivedPaletteKey == 0) {
            memcpy(raw + 0x374, perVertexAlphaOrShadeF,
                        static_cast<size_t>(vertexCount) * sizeof(float));
        }
        if (clippedTriVerts != 0) {
            memcpy(raw + 0x304, clippedTriVerts, 3 * sizeof(zVec3));
            *(int *)(raw + 0x47c) = 1;
        } else {
            *(int *)(raw + 0x47c) = 0;
        }
        *(int *)(raw + 0x488) = texKey;
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
            zRndr_DrawTexturedQueued(entry, fanVerts, clippedTriVerts, triData9f, triUVs,
                                     &shadeTriplet, 3, fanTriIndex, texKey);
        }
        }
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(0x400, kSourceFile, 0x1e1, " Not enough MAX_TRANSPARENT_POLYS: need %d\n",
                          queueIndex);
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[queueIndex];
    unsigned char *raw = (unsigned char *)(&cmd);
    cmd.vertexCount = vertexCount;
    cmd.materialRef = entry;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    cmd.alphaOrShadeBits = 0xff;
    memcpy(raw + 0x008, projectedPolyVerts,
                static_cast<size_t>(vertexCount) * sizeof(zVec3));
    memcpy(raw + 0x32c, triData9f, 3 * sizeof(zVec3));
    memcpy(raw + 0x350, triUVs, 3 * sizeof(zVec2));
    cmd.texKey = texKey;
    ++zRndr::g_transparentQueueCount;
}

// Reimplements 0x48d6d0: zRndr_OverlayRect_Submit
// (D:\Proj\GameZRecoil\zRndr\zRndr_Overlay.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_OverlayRect_Submit(unsigned int packedColor16,
                                                              zVidRect32 *rectOrNull,
                                                              double alpha) {
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
        zVideo_dd3d::QueueSolidQuad(packedColor16 & 0xffffU, &rect, alpha);
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

// Reimplements 0x48d7a0: zRndr_OverlayRect_FlushSw
// (D:\Proj\GameZRecoil\zRndr\zRndr_Overlay.cpp)
RECOIL_NOINLINE void RECOIL_CDECL zRndr_OverlayRect_FlushSw() {
    if (zRndr::g_overlayBlendEnabled == 0) {
        return;
    }

    const int graphicsFlags =
        zRndr::g_graphicsFlags != 0 ? *zRndr::g_graphicsFlags : 0;
    if (zRndr::g_pixelPackGreenBits == 5) {
        zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow555_Scalar;
    } else {
        zRndr::g_pfnOverlayBlendRow = zRndr::OverlayBlendRow565_Scalar;
    }
    (void)graphicsFlags;

    unsigned int redMask;
    unsigned int greenMask;
    unsigned int blueMask;
    zVideo::PixelPack_GetRgbMasks(&redMask, &greenMask, &blueMask);

    const int srcScale5 =
        static_cast<int>(zRndr::g_overlayBlendAlpha * 32.0);
    const unsigned int overlayColor16 = zRndr::g_overlayBlendPackedColor16;
    const unsigned int premulR = ((redMask & overlayColor16) * srcScale5) >> 5;
    const unsigned int premulG = ((greenMask & overlayColor16) * srcScale5) >> 5;
    const unsigned int premulB = ((blueMask & overlayColor16) * srcScale5) >> 5;
    const unsigned int premulRPair = premulR | (premulR << 16);
    const unsigned int premulGPair = premulG | (premulG << 16);
    const unsigned int premulBPair = premulB | (premulB << 16);
    zRndr::g_swOverlayPremulPacked =
        (((blueMask & premulBPair) | (redMask & premulRPair)) << 16) |
        (greenMask & premulGPair);
    zRndr::g_swOverlayPremulPackedRot16 =
        (zRndr::g_swOverlayPremulPacked >> 16) | (zRndr::g_swOverlayPremulPacked << 16);
    zRndr::g_swOverlayDstScale5 =
        static_cast<int>((1.0 - zRndr::g_overlayBlendAlpha) * 32.0);

    int rowY = zRndr::g_overlayBlendRectTop;
    const int rectLeft = zRndr::g_overlayBlendRectLeft;
    const int pixelCount = zRndr::g_overlayBlendRectRight - rectLeft;
    unsigned short *rowPixels16 =
        g_zVideo_FxSurfacePixels16 + g_zVideo_FxSurfacePitchPixels16 * rowY + rectLeft;
    while (rowY < zRndr::g_overlayBlendRectBottom) {
        zRndr::g_pfnOverlayBlendRow(rowPixels16, pixelCount);
        ++rowY;
        rowPixels16 += g_zVideo_FxSurfacePitchPixels16;
    }
}

// Reimplements 0x49a2b0: zRndr_FlushTransparentQueue
RECOIL_NOINLINE void RECOIL_CDECL zRndr_FlushTransparentQueue() {
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
        unsigned char *raw = (unsigned char *)(&cmd);

        zRndr::g_inverseDepthBias = cmd.savedInvDepthBias;
        zRndr::g_inverseDepthScale = cmd.savedInvDepthScale;
        zRndr::g_scanConvertMode = cmd.scanConvertMode;

        if (cmd.materialRef == 0) {
            zRndr_DrawFlatImmediate((zVec3 *)(cmd.polyVerts),
                                    (zVec3 *)(cmd.triVerts), cmd.vertexCount,
                                    cmd.alphaOrShadeBits, cmd.shadeOrSpanMode);
            continue;
        }

        zVec3 *clippedTriVerts =
            cmd.hasClippedTriVerts != 0 ? (zVec3 *)(raw + 0x308) : 0;
        zVec3 *polyVerts = (zVec3 *)(cmd.polyVerts);
        zVec3 *triVerts = (zVec3 *)(cmd.triVerts);
        zVec2 *triUVs = (zVec2 *)(cmd.triUVs);
        if ((cmd.materialRef->image->formatFlagsPacked & 2) == 0) {
            float alpha = 0.0f;
            memcpy(&alpha, &cmd.alphaOrShadeBits, sizeof(float));
            if (alpha < 1.0f) {
                Renderer_DrawPolyTLV(cmd.materialRef, polyVerts, triVerts, triUVs, cmd.vertexCount,
                                     alpha, cmd.texKey);
            } else {
                zRndr_DrawFlatQueued(cmd.materialRef, polyVerts, triVerts, triUVs, cmd.vertexCount,
                                     cmd.texKey);
            }
        } else {
            zRndr_DrawTexturedFanTri(cmd.materialRef, polyVerts, clippedTriVerts, triVerts, triUVs,
                                     cmd.vertexCount, cmd.alphaOrShadeBits, cmd.texKey);
        }
    }
    }

    zRndr::g_transparentQueueCount = 0;
}

// Reimplements 0x49a490: zRndr_FlushOverwriteQueue
RECOIL_NOINLINE void RECOIL_CDECL zRndr_FlushOverwriteQueue() {
    zRndr::g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanListFast;

    {
    for (int queueIndex = 0; queueIndex < zRndr::g_overwriteQueueCount; ++queueIndex) {
        zRndr::OverwriteQueuedPolyDrawCmd &cmd = zRndr::g_overwriteQueue[queueIndex];
        unsigned char *raw = (unsigned char *)(&cmd);
        const int commandTag = *(int *)(raw + 0x000);
        zVec3 *polyVerts = (zVec3 *)(raw + 0x004);
        zVec3 *clippedTriVerts = *(int *)(raw + 0x47c) != 0
                                     ? (zVec3 *)(raw + 0x304)
                                     : 0;
        zVec3 *triVerts = (zVec3 *)(raw + 0x328);
        zVec2 *triUVs = (zVec2 *)(raw + 0x358);
        float *perVertexAlphaOrShadeF = (float *)(raw + 0x374);
        const int texKey = *(int *)(raw + 0x488);

        zRndr::g_inverseDepthBias = cmd.savedInvDepthBias;
        zRndr::g_inverseDepthScale = cmd.savedInvDepthScale;
        zRndr::g_scanConvertMode = cmd.scanConvertMode;

        zVidImagePartial *image = cmd.materialRef != 0 ? cmd.materialRef->image : 0;
        bool useFallback = false;

        if (commandTag == 0) {
            if (cmd.alphaOrShadeF >= 255.0f) {
                zRndr_RasterizePolyWithSpanList(polyVerts, triVerts, cmd.vertexCount,
                                                cmd.shadeOrSpanMode);
                continue;
            }
            useFallback = true;
        } else if (commandTag == 1) {
            if (image != 0 && (image->formatFlagsPacked & 2) == 0 &&
                cmd.alphaOrShadeF >= 1.0f) {
                zRndr_DrawTexturedQueuedAlpha(cmd.materialRef, polyVerts, clippedTriVerts, triVerts,
                                              triUVs, cmd.vertexCount, texKey);
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
                        zRndr_DrawTexturedQueued(cmd.materialRef, fanVerts, clippedTriVerts,
                                                 triVerts, triUVs, &shadeTriplet, 3, fanTriIndex,
                                                 texKey);
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
            zRndr_DrawFlatImmediate(polyVerts, triVerts, cmd.vertexCount,
                                    static_cast<int>(cmd.alphaOrShadeF),
                                    cmd.shadeOrSpanMode);
        } else if ((image->formatFlagsPacked & 2) == 0) {
            if (cmd.alphaOrShadeF < 1.0f) {
                Renderer_DrawPolyTLV(cmd.materialRef, polyVerts, triVerts, triUVs, cmd.vertexCount,
                                     cmd.alphaOrShadeF, texKey);
            } else {
                zRndr_DrawFlatQueued(cmd.materialRef, polyVerts, triVerts, triUVs, cmd.vertexCount,
                                     texKey);
            }
        } else {
            zRndr_DrawTexturedFanTri(cmd.materialRef, polyVerts, clippedTriVerts, triVerts, triUVs,
                                     cmd.vertexCount, static_cast<int>(cmd.alphaOrShadeF),
                                     texKey);
        }
    }
    }

    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
    zRndr::g_pfnBuildSpanListSecondary = zRndr_SpanOcclusion_BuildSpanList;
}

// Reimplements 0x499130: zRndr_TextureMip_SelectVariantImage
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL zRndr_TextureMip_SelectVariantImage(
    zImage_TexDirEntryPartial *entry, const zVec3 *triVerts, int vertCount,
    const zVec2 *vertexUvPairs, const zVec2 *mipParamsA, const zVec2 *mipParamsB,
    const zVec2 *mipParamsC) {
    if (zRndr::g_textureMipSelectionEnabled == 0) {
        return entry != 0 ? entry->image : 0;
    }

    int selectedVertex = 0;
    float selectedZ = triVerts[0].z;
    for (int i = 1; i < vertCount; ++i) {
        if (selectedZ < triVerts[i].z) {
            selectedZ = triVerts[i].z;
            selectedVertex = i;
        }
    }

    const zVec2 &selectedUv = vertexUvPairs[selectedVertex];
    const float invZ = 1.0f / selectedZ;
    const float uOverZ = selectedUv.x * invZ;
    const float vOverZ = selectedUv.y * invZ;
    const float invZAtX = 1.0f / (selectedZ + mipParamsA->x);
    const float invZAtY = 1.0f / (selectedZ + mipParamsA->y);

    const float uDeltaX = (selectedUv.x + mipParamsB->x) * invZAtX - uOverZ;
    const float uDeltaY = (selectedUv.x + mipParamsB->y) * invZAtY - uOverZ;
    const float vDeltaX = (selectedUv.y + mipParamsC->x) * invZAtX - vOverZ;
    const float vDeltaY = (selectedUv.y + mipParamsC->y) * invZAtY - vOverZ;

    float mipMetric = uDeltaX;
    if (uDeltaY > mipMetric) {
        mipMetric = uDeltaY;
    }
    if (vDeltaX > mipMetric) {
        mipMetric = vDeltaX;
    }
    if (vDeltaY > mipMetric) {
        mipMetric = vDeltaY;
    }

    const int variantIndex = static_cast<int>(mipMetric) >> 1;
    return entry->GetVariantImageAtIndex(variantIndex);
}

// Reimplements 0x493df0: zRndr_DrawFlatQueued
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawFlatQueued(zImage_TexDirEntryPartial *entry,
                                                          zVec3 *polyVerts, zVec3 *triVerts,
                                                          zVec2 *triUVs, int vertCount,
                                                          int paletteIndex) {
    if (entry == 0 || entry->image == 0 || polyVerts == 0 ||
        triVerts == 0 || triUVs == 0 || vertCount <= 0 ||
        zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = static_cast<float>(selectedImage->width);
    const float imageHeight = static_cast<float>(selectedImage->height);

    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(0, triVerts, triUVs, imageWidth, imageHeight);

    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry, triVerts, 3, vertexUvPairs, &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient, &planes.vOverZ.gradient);
        if (selectedImage == 0) {
            return;
        }
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

    const int firstScanline =
        static_cast<int>(floor(polyVerts[topVertexIndex].y + 0.5f));
    const int lastScanline =
        static_cast<int>(floor(polyVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexels = static_cast<unsigned char *>(selectedImage->pixels);
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;

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

    zRndr::TexturedQueuedSpanProc spanProc = spanOpMode0;
    unsigned short *palette = static_cast<unsigned short *>(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette =
            paletteIndex == -1 ? palette : &palette[(paletteIndex + 1) * 0x100];
        spanProc = spanOpMode1;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    zRndr::SpanBuildProc buildProc =
        zRndr::g_pfnBuildSpanListSecondary != 0
            ? zRndr::g_pfnBuildSpanListSecondary
            : (zRndr::g_pfnBuildSpanList != 0 ? zRndr::g_pfnBuildSpanList
                                                    : zRndr_SpanOcclusion_InsertSpanNode_Local);
    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    const int texVShift =
        selectedImage->uShiftFrom20 != 0 ? selectedImage->uShiftFrom20 : 20;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;
        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = polyVerts[i];
            const zVec3 &b = polyVerts[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_4455 = 0; i_4455 < pairCount; i_4455 += 2) {
            const int xMin =
                static_cast<int>(floor(intersections[i_4455] + 0.5f));
            const int xMax =
                static_cast<int>(ceil(intersections[i_4455 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                static_cast<float>(y) * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                (static_cast<float>(xMin) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                (static_cast<float>(xMax) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(visibleSpans, y, &spanCount);
            {
            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                if (span == 0 || span->sampleXMin > span->sampleXMax) {
                    continue;
                }

                zRndr::g_spanCurrentDst16 = (unsigned short *)(
                    scanlineBase +
                    static_cast<int>(span->sampleXMin) * zRndr::g_bytesPerPixel);
                DispatchTexturedSpanChunks(spanProc, planes, 0, span, y,
                                           span->sampleXMax - span->sampleXMin + 1, 1048576.0f,
                                           texVShift);
            }
            }
        }
    }
}

// Reimplements 0x495850: zRndr_DrawTexturedQueued
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawTexturedQueued(
    zImage_TexDirEntryPartial *entry, zVec3 *projectedVerts, zVec3 *clippedTriVerts,
    zVec3 *triVerts, zVec2 *triUVs, zVec3 *shadeTriplet, int vertCount, int,
    int texKey) {
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 ||
        triVerts == 0 || triUVs == 0 || shadeTriplet == 0 || vertCount <= 0 ||
        zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = static_cast<float>(selectedImage->width);
    const float imageHeight = static_cast<float>(selectedImage->height);
    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(clippedTriVerts, triVerts, triUVs, imageWidth, imageHeight);

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry, triVerts, 3, vertexUvPairs, &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient, &planes.vOverZ.gradient);
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
    Plane2f shadePlane = BuildScreenPlaneFromTriangle(projectedVerts, shadeValues);

    int topVertexIndex = 0;
    int bottomVertexIndex = 0;
    float minPositiveReciprocalZ = 1000.0f;
    for (int i_4544 = 0; i_4544 < vertCount; ++i_4544) {
        const float reciprocalZ =
            EvalPlane(planes.reciprocalZ, projectedVerts[i_4544].x, projectedVerts[i_4544].y);
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

    const int firstScanline =
        static_cast<int>(floor(projectedVerts[topVertexIndex].y + 0.5f));
    const int lastScanline =
        static_cast<int>(floor(projectedVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexels = static_cast<unsigned char *>(selectedImage->pixels);
    zRndr::g_spanActiveTexAlphaMap = selectedImage->queuedAlphaMap;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode0;
    Plane2f *activeShadePlane = 0;

    unsigned short *palette = static_cast<unsigned short *>(selectedImage->palette);
    if (palette != 0) {
        spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode1;
        if (texKey != -1) {
            zRndr::g_spanActiveTexPalette = &palette[(texKey + 1) * 0x100];
        } else {
            int shadeRecipe = g_zRndr_ActivePaletteShadeRecipeIndex;
            if (shadeRecipe < 0) {
                shadeRecipe = zVid_PaletteRemap_FindRecipeIndexFromRgb(
                    (zColorRgb *)(zRndr::g_fogParamsActive.colorRgb01));
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
    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    const int chunkPixels =
        SelectPerspectiveChunkPixels(minPositiveReciprocalZ, planes.reciprocalZ.gradient.x);
    const int texVShift =
        selectedImage->uShiftFrom20 != 0 ? selectedImage->uShiftFrom20 : 20;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;
        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = projectedVerts[i];
            const zVec3 &b = projectedVerts[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }
            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_4640 = 0; i_4640 < pairCount; i_4640 += 2) {
            const int xMin =
                static_cast<int>(floor(intersections[i_4640] + 0.5f));
            const int xMax =
                static_cast<int>(ceil(intersections[i_4640 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                static_cast<float>(y) * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                (static_cast<float>(xMin) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                (static_cast<float>(xMax) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(visibleSpans, y, &spanCount);
            {
            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                if (span == 0 || span->sampleXMin > span->sampleXMax) {
                    continue;
                }

                zRndr::g_spanCurrentDst16 = (unsigned short *)(
                    scanlineBase +
                    static_cast<int>(span->sampleXMin) * zRndr::g_bytesPerPixel);
                DispatchTexturedSpanChunks(spanProc, planes, activeShadePlane, span, y, chunkPixels,
                                           textureScale, texVShift);
            }
            }
        }
    }
}

// Reimplements 0x494af0: Renderer_DrawPolyTLV
RECOIL_NOINLINE void RECOIL_FASTCALL Renderer_DrawPolyTLV(zImage_TexDirEntryPartial *entry,
                                                          zVec3 *polyVerts, zVec3 *triVerts,
                                                          zVec2 *triUVs, int vertexCount,
                                                          float alpha, int texKey) {
    if (entry == 0 || entry->image == 0 || polyVerts == 0 ||
        triVerts == 0 || triUVs == 0 || vertexCount <= 0 ||
        zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = static_cast<float>(selectedImage->width);
    const float imageHeight = static_cast<float>(selectedImage->height);
    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(0, triVerts, triUVs, imageWidth, imageHeight);

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry, triVerts, 3, vertexUvPairs, &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient, &planes.vOverZ.gradient);
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

    zRndr::g_spanActiveTexels = static_cast<unsigned char *>(selectedImage->pixels);
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;

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
        const double alphaScaled = static_cast<double>(alpha) * 255.0;
        zRndr::g_spanActiveConstAlphaBits =
            static_cast<int>(alphaScaled >= 0.0 ? alphaScaled + 0.5 : alphaScaled - 0.5);
        spanProc = zRndr::g_pfnPolyTlvSpanOp_Mode1;
        paletteSpanProc = zRndr::g_pfnPolyTlvSpanOpAlt_Mode1;
    }

    unsigned short *palette = static_cast<unsigned short *>(selectedImage->palette);
    if (palette != 0) {
        zRndr::g_spanActiveTexPalette = texKey == -1 ? palette : &palette[(texKey + 1) * 0x100];
        spanProc = paletteSpanProc;
    } else {
        zRndr::g_spanActiveTexPalette = 0;
    }

    if (spanProc == 0) {
        return;
    }

    zRndr::SpanNodePartial *visibleSpans[0x40] = {0};
    zRndr::SpanBuildProc buildProc =
        zRndr::g_pfnBuildSpanListSecondary != 0
            ? zRndr::g_pfnBuildSpanListSecondary
            : (zRndr::g_pfnBuildSpanList != 0 ? zRndr::g_pfnBuildSpanList
                                                    : zRndr_SpanOcclusion_InsertSpanNode_Local);
    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    const int texVShift =
        selectedImage->uShiftFrom20 != 0 ? selectedImage->uShiftFrom20 : 20;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;
        for (int i = 0; i < vertexCount; ++i) {
            const zVec3 &a = polyVerts[i];
            const zVec3 &b = polyVerts[(i + 1) % vertexCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_4808 = 0; i_4808 < pairCount; i_4808 += 2) {
            const int xMin = SpanStartFromX(intersections[i_4808]);
            const int xMax = SpanEndFromX(intersections[i_4808 + 1]);
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                static_cast<float>(y) * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                (static_cast<float>(xMin) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                (static_cast<float>(xMax) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(visibleSpans, y, &spanCount);
            {
            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                if (span == 0 || span->sampleXMin > span->sampleXMax) {
                    continue;
                }

                zRndr::g_spanCurrentDst16 = (unsigned short *)(
                    scanlineBase +
                    static_cast<int>(span->sampleXMin) * zRndr::g_bytesPerPixel);
                DispatchTexturedSpanChunks(spanProc, planes, 0, span, y,
                                           span->sampleXMax - span->sampleXMin + 1, textureScale,
                                           texVShift);
            }
            }
        }
    }
}

// Reimplements 0x4969d0: zRndr_DrawTexturedQueuedAlpha
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawTexturedQueuedAlpha(
    zImage_TexDirEntryPartial *entry, zVec3 *projectedVerts, zVec3 *clippedTriVerts,
    zVec3 *triVerts, zVec2 *triUVs, int vertCount, int variantIndex) {
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 ||
        triVerts == 0 || triUVs == 0 || vertCount <= 0 ||
        zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = static_cast<float>(selectedImage->width);
    const float imageHeight = static_cast<float>(selectedImage->height);

    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(clippedTriVerts, triVerts, triUVs, imageWidth, imageHeight);

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry, triVerts, 3, vertexUvPairs, &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient, &planes.vOverZ.gradient);
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
            EvalPlane(planes.reciprocalZ, projectedVerts[i_4890].x, projectedVerts[i_4890].y);
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

    const int firstScanline =
        static_cast<int>(floor(projectedVerts[topVertexIndex].y + 0.5f));
    const int lastScanline =
        static_cast<int>(floor(projectedVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexels = static_cast<unsigned char *>(selectedImage->pixels);
    zRndr::g_spanActiveTexAlphaMap = selectedImage->queuedAlphaMap;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedQueuedSpanOp_Mode0;

    unsigned short *palette = static_cast<unsigned short *>(selectedImage->palette);
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
    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    const int chunkPixels =
        SelectPerspectiveChunkPixels(minPositiveReciprocalZ, planes.reciprocalZ.gradient.x);
    const int texVShift =
        selectedImage->uShiftFrom20 != 0 ? selectedImage->uShiftFrom20 : 20;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;
        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = projectedVerts[i];
            const zVec3 &b = projectedVerts[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_4973 = 0; i_4973 < pairCount; i_4973 += 2) {
            const int xMin =
                static_cast<int>(floor(intersections[i_4973] + 0.5f));
            const int xMax =
                static_cast<int>(ceil(intersections[i_4973 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                static_cast<float>(y) * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                (static_cast<float>(xMin) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                (static_cast<float>(xMax) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(visibleSpans, y, &spanCount);
            {
            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                if (span == 0 || span->sampleXMin > span->sampleXMax) {
                    continue;
                }

                zRndr::g_spanCurrentDst16 = (unsigned short *)(
                    scanlineBase +
                    static_cast<int>(span->sampleXMin) * zRndr::g_bytesPerPixel);
                DispatchTexturedSpanChunks(spanProc, planes, 0, span, y, chunkPixels,
                                           textureScale, texVShift);
            }
            }
        }
    }
}

// Reimplements 0x497ac0: zRndr_DrawTexturedFanTri
RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_DrawTexturedFanTri(zImage_TexDirEntryPartial *entry, zVec3 *projectedVerts,
                         zVec3 *clippedTriVerts, zVec3 *triVerts, zVec2 *triUVs,
                         int vertCount, int alpha255, int variantIndex) {
    if (entry == 0 || entry->image == 0 || projectedVerts == 0 ||
        triVerts == 0 || triUVs == 0 || vertCount <= 0 ||
        zRndr::g_spanAllocCursor == 0 || zRndr::g_frameBuffer == 0) {
        return;
    }

    zVidImagePartial *selectedImage = entry->image;
    const float imageWidth = static_cast<float>(selectedImage->width);
    const float imageHeight = static_cast<float>(selectedImage->height);

    zVec2 vertexUvPairs[3] = {0};
    for (int i = 0; i < 3; ++i) {
        vertexUvPairs[i].x = imageWidth * triVerts[i].z * triUVs[i].x;
        vertexUvPairs[i].y = imageHeight * triVerts[i].z * triUVs[i].y;
    }

    TexturedPlanes planes =
        BuildQueuedTexturePlanes(clippedTriVerts, triVerts, triUVs, imageWidth, imageHeight);

    float textureScale = 1048576.0f;
    if (entry->nextVariant != 0) {
        selectedImage = zRndr_TextureMip_SelectVariantImage(
            entry, triVerts, 3, vertexUvPairs, &planes.reciprocalZ.gradient,
            &planes.uOverZ.gradient, &planes.vOverZ.gradient);
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
            EvalPlane(planes.reciprocalZ, projectedVerts[i_5057].x, projectedVerts[i_5057].y);
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

    const int firstScanline =
        static_cast<int>(floor(projectedVerts[topVertexIndex].y + 0.5f));
    const int lastScanline =
        static_cast<int>(floor(projectedVerts[bottomVertexIndex].y - 0.5f));
    if (firstScanline > lastScanline) {
        return;
    }

    zRndr::g_spanActiveTexels = static_cast<unsigned char *>(selectedImage->pixels);
    zRndr::g_spanActiveTexAlphaMap = selectedImage->queuedAlphaMap;
    zRndr::g_spanActiveTexVMask = selectedImage->vMaskFixed20;
    zRndr::g_spanActiveTexUMask = selectedImage->uMask;
    zRndr::g_spanActiveConstAlphaBits = alpha255;
    zRndr::TexturedQueuedSpanProc spanProc = zRndr::g_pfnTexturedFanTriSpanOp_Mode0;

    unsigned short *palette = static_cast<unsigned short *>(selectedImage->palette);
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
    zRndr::SpanBuildProc buildProc =
        zRndr::g_pfnBuildSpanListSecondary != 0
            ? zRndr::g_pfnBuildSpanListSecondary
            : (zRndr::g_pfnBuildSpanList != 0 ? zRndr::g_pfnBuildSpanList
                                                    : zRndr_SpanOcclusion_InsertSpanNode_Local);
    unsigned char *frameBase = static_cast<unsigned char *>(zRndr::g_frameBuffer);
    const int chunkPixels =
        SelectPerspectiveChunkPixels(minPositiveReciprocalZ, planes.reciprocalZ.gradient.x);
    const int texVShift =
        selectedImage->uShiftFrom20 != 0 ? selectedImage->uShiftFrom20 : 20;

    for (int y = firstScanline; y <= lastScanline; ++y) {
        float intersections[0x40] = {0};
        int intersectionCount = 0;
        const float sampleY = static_cast<float>(y) + 0.5f;
        for (int i = 0; i < vertCount; ++i) {
            const zVec3 &a = projectedVerts[i];
            const zVec3 &b = projectedVerts[(i + 1) % vertCount];
            if (a.y == b.y) {
                continue;
            }

            const float minY = MinValue(a.y, b.y);
            const float maxY = MaxValue(a.y, b.y);
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

        sort(intersections, intersections + intersectionCount);
        unsigned char *scanlineBase = frameBase + y * zRndr::g_pitchBytes;
        const int pairCount = intersectionCount & ~1;
        for (int i_5143 = 0; i_5143 < pairCount; i_5143 += 2) {
            const int xMin =
                static_cast<int>(floor(intersections[i_5143] + 0.5f));
            const int xMax =
                static_cast<int>(ceil(intersections[i_5143 + 1] - 0.5f)) - 1;
            if (xMin > xMax) {
                continue;
            }

            const float rowReciprocalZ =
                static_cast<float>(y) * planes.reciprocalZ.gradient.y + planes.reciprocalZ.base;
            zRndr::g_spanAllocCursor->sampleXMin = xMin;
            zRndr::g_spanAllocCursor->sampleXMax = xMax;
            zRndr::g_spanAllocCursor->invDepth =
                (static_cast<float>(xMin) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->invDepthStep =
                (static_cast<float>(xMax) * planes.reciprocalZ.gradient.x + rowReciprocalZ) *
                    zRndr::g_inverseDepthScale +
                zRndr::g_inverseDepthBias;
            zRndr::g_spanAllocCursor->depthSlope = planes.reciprocalZ.gradient.x;

            int spanCount = 0;
            buildProc(visibleSpans, y, &spanCount);
            {
            for (int spanIndex = 0; spanIndex < spanCount; ++spanIndex) {
                zRndr::SpanNodePartial *span = visibleSpans[spanIndex];
                if (span == 0 || span->sampleXMin > span->sampleXMax) {
                    continue;
                }

                zRndr::g_spanCurrentDst16 = (unsigned short *)(
                    scanlineBase +
                    static_cast<int>(span->sampleXMin) * zRndr::g_bytesPerPixel);
                DispatchTexturedSpanChunks(spanProc, planes, 0, span, y, chunkPixels,
                                           textureScale, texVShift);
            }
            }
        }
    }
}

// Reimplements 0x498bd0: zRndr_DrawImmediateLine
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_DrawImmediateLine(int x0, int y0,
                                                             int x1, int y1,
                                                             int color16) {
    typedef void (RECOIL_FASTCALL *ImmediateRaster4Proc)(void *frameBuffer, int x0, int y0,
                                int x1, int y1, int color16);

    ((ImmediateRaster4Proc)(zRndr::g_pfnImmediateRaster4))(zRndr::g_frameBuffer, x0,
                                                                         y0, x1, y1, color16);
}

// Reimplements 0x498c00: zRndr_DrawClippedImmediateLineStrip
RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_DrawClippedImmediateLineStrip(const zRndr_LinePoint2I *points, int segmentCount,
                                    const void *clipRect, int color16) {
    if (segmentCount <= 0) {
        return;
    }

    typedef void (RECOIL_FASTCALL *ImmediateRaster5Proc)(void *frameBuffer, const void *clipRect, int x0, int y0, int x1,
        int y1, int color16);

    ImmediateRaster5Proc const drawClipped =
        (ImmediateRaster5Proc)(zRndr::g_pfnImmediateRaster5);

    const zRndr_LinePoint2I *point = points + 1;
    do {
        drawClipped(zRndr::g_frameBuffer, clipRect, point[-1].x, point[-1].y, point[0].x,
                    point[0].y, color16);
        ++point;
        --segmentCount;
    } while (segmentCount != 0);
}

// Reimplements 0x49a830: zRndr_LensFlare_QueueProjectedSample
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_QueueProjectedSample(
    zProjectedPoint *projectedPoint, int packedColor16, int lensFlareSource) {
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

// Reimplements 0x49a8b0: zRndr_LensFlare_GetQueuedSampleCount
RECOIL_NOINLINE int RECOIL_CDECL zRndr_LensFlare_GetQueuedSampleCount() {
    return zRndr::g_lensFlareSampleQueueCount;
}

// Reimplements 0x49a920: zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList
// (D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_LensFlare_DrawQueuedSamples16_AndBuildVisibleList(int startIndex) {
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

            memcpy(sample, &zRndr::g_lensFlareSampleQueue[newCount], sizeof(*sample));
            continue;
        }

        zRndr_LensFlareSource *source =
            (zRndr_LensFlareSource *)(sample->lensFlareSource);
        if (source != 0 && source->lensFlareEnabled != 0 &&
            sample < &zRndr::g_lensFlareSampleQueue[0x40] &&
            sample->reciprocalZ != 0.0f) {
            zRndr::g_lensFlareVisibleSampleDefs[zRndr::g_lensFlareVisibleSampleCount] =
                (zRndr_LensFlareVisibleSampleDef *)(sample);
            ++zRndr::g_lensFlareVisibleSampleCount;
        }

        ++startIndex;
        ++sample;
    }
}

// Reimplements 0x49aa40: zRndr_LensFlare_SetVisibleSampleStage
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_SetVisibleSampleStage(
    int stageIndex, zImage_TexDirEntryPartial *stageTexDirEntry) {
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

// Reimplements 0x49aa90: zRndr_LensFlare_DrawSampleStageClipped
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_DrawSampleStageClipped(
    const zVec2 *sampleCenter, zImage_TexDirEntryPartial *stageTexDirEntry, float sampleRadius,
    const zRndr_LineClipRect2I *clipRect) {
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
        clipLeft = static_cast<float>(clipRect->left);
        clipTop = static_cast<float>(clipRect->top);
        clipRight = static_cast<float>(clipRect->right);
        clipBottom = static_cast<float>(clipRect->bottom);
        if (left > clipRight - 2.0f || top > clipBottom - 2.0f || right < clipLeft + 1.0f ||
            bottom < clipTop + 1.0f) {
            return;
        }
    } else {
        clipRight = static_cast<float>(zRndr::g_activeRegionWidth);
        clipBottom = static_cast<float>(zRndr::g_activeRegionHeight);
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
        typedef void (RECOIL_FASTCALL *SubmitPolyRenderClassProc)(zVideo_XyzVertex * vertices, zVideo_TexCoord * texCoords,
                                    int vertexCount, zVideo_RenderClass *renderClass,
                                    unsigned int renderParam, float alpha, int queueMode);
        SubmitPolyRenderClassProc submitPoly = (SubmitPolyRenderClassProc)(
            static_cast<unsigned int>(g_zVideo_pfnSubmitPolyRenderClass));
        zVideo_RenderClass *renderClass =
            stageTexDirEntry != 0
                ? (zVideo_RenderClass *)(stageTexDirEntry->texture)
                : 0;
        submitPoly((zVideo_XyzVertex *)(projectedVerts),
                   (zVideo_TexCoord *)(triUVs), 4, renderClass, 0x10, 1.0f, 0);
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
        for (vertexIndex1 = 0; vertexIndex1 < (int)(sizeof(projectedVerts) / sizeof((projectedVerts)[0])); ++vertexIndex1) {
            zVec3 & vertex = (projectedVerts)[vertexIndex1];
        vertex.z = 10.0f;
    }
    }

    zRndr_SubmitTexturedPolyUniformAlphaOrShade(projectedVerts, clippedTriVerts, projectedVerts,
                                                triUVs, 4, stageTexDirEntry, 1.0f, 0);
}

// Reimplements 0x49b020: zRndr_LensFlare_DrawVisibleSampleStages
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_DrawVisibleSampleStages(
    zRndr_LensFlareVisibleSampleDef *visibleSampleDef, float visibilityAlpha) {
    const float activeWidth = static_cast<float>(zRndr::g_activeRegionWidth);
    const float activeHeight = static_cast<float>(zRndr::g_activeRegionHeight);
    const float baseRadius = visibilityAlpha * activeWidth * 0.03125f;
    const float largeRadius = baseRadius + baseRadius;
    const float halfClipWidth = activeWidth * 0.5f;
    const float halfClipHeight = activeHeight * 0.5f;
    const float sampleOffsetX = visibleSampleDef->sampleCenterX - halfClipWidth;
    const float sampleOffsetY = visibleSampleDef->sampleCenterY - halfClipHeight;
    const zRndr_LineClipRect2I *clipRect =
        (const zRndr_LineClipRect2I *)(&zRndr::g_activeRegionRect);

    zVec2 sampleCenter = {visibleSampleDef->sampleCenterX, visibleSampleDef->sampleCenterY};
    zRndr_LensFlare_DrawSampleStageClipped(&sampleCenter, zRndr::g_lensFlareVisibleSampleStages[0],
                                           largeRadius, clipRect);

    sampleCenter.x = halfClipWidth + sampleOffsetX * 0.5f;
    sampleCenter.y = halfClipHeight + sampleOffsetY * 0.5f;
    zRndr_LensFlare_DrawSampleStageClipped(&sampleCenter, zRndr::g_lensFlareVisibleSampleStages[1],
                                           largeRadius, clipRect);

    sampleCenter.x = halfClipWidth + sampleOffsetX * 0.100000001f;
    sampleCenter.y = halfClipHeight + sampleOffsetY * 0.100000001f;
    zRndr_LensFlare_DrawSampleStageClipped(&sampleCenter, zRndr::g_lensFlareVisibleSampleStages[2],
                                           baseRadius, clipRect);

    sampleCenter.x = halfClipWidth - sampleOffsetX;
    sampleCenter.y = halfClipHeight - sampleOffsetY;
    zRndr_LensFlare_DrawSampleStageClipped(&sampleCenter, zRndr::g_lensFlareVisibleSampleStages[3],
                                           baseRadius * 3.0f, clipRect);
}

// Reimplements 0x49afb0: zRndr_LensFlare_DrawVisibleSample
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_LensFlare_DrawVisibleSample(int sampleIndex) {
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
        zRndr_LensFlare_DrawVisibleSampleStages(visibleSampleDef, 1.0f);
        return;
    }

    zRndr_LensFlare_DrawVisibleSampleStages(
        visibleSampleDef, (lensFlareSource->fadeFar - visibility) /
                              (lensFlareSource->fadeFar - lensFlareSource->fadeNear));
}

// Reimplements 0x49b1a0: zRndr_LensFlare_DrawVisibleSamples
RECOIL_NOINLINE void RECOIL_CDECL zRndr_LensFlare_DrawVisibleSamples() {
    if (zRndr::g_lensFlareVisibilityActive == 0) {
        return;
    }

    for (int sampleIndex = 0; sampleIndex < zRndr::g_lensFlareVisibleSampleCount;
         ++sampleIndex) {
        zRndr_LensFlare_DrawVisibleSample(sampleIndex);
    }

    zRndr::g_lensFlareVisibleSampleCount = 0;
}

// Reimplements 0x49aa30: zRndr_SpanOcclusion_FilterSampleList
RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_SpanOcclusion_FilterSampleList(int visibleSampleIndex, zVec3 *outPoint) {
    zRndr_LensFlareVisibleSampleDef *sample =
        zRndr::g_lensFlareVisibleSampleDefs[visibleSampleIndex];
    zMath_UnprojectPointBatchZBuf((const zProjectedPoint *)(sample), outPoint, 1);
}

// Reimplements 0x49b5a0: zRndr_FogTargetColorStaged_SetRgb01Clamped
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_FogTargetColorStaged_SetRgb01Clamped(zColorRgb *color) {
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

    if (g_zVideo_RendererType != 0) {
        zVideo_SetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat *)(color));
    }

    const int red = static_cast<int>(color->red * 255.0f + 0.5f);
    const int green = static_cast<int>(color->green * 255.0f + 0.5f);
    const int blue = static_cast<int>(color->blue * 255.0f + 0.5f);
    zRndr::SpanAlphaBlend565_Mmx_FromPal8(
        staged,
        (red << g_zVideo_PixelPack_RShift) & static_cast<int>(g_zVideo_PixelPack_RMask),
        (green << g_zVideo_PixelPack_GShift) & static_cast<int>(g_zVideo_PixelPack_GMask),
        blue >> g_zVideo_PixelPack_BShiftTo8);
}

// Reimplements 0x499930: zRndr_SetPaletteRemapKey
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SetPaletteRemapKey(zVidPaletteRemapRecipe *recipe,
                                                              float shadeLevel) {
    if (recipe == 0) {
        g_zRndr_ActivePaletteRemapKey = -1;
        return;
    }

    const int recipeIndex = zVid_PaletteRemap_BuildPaletteVariant(recipe);
    int shadeBucket = static_cast<int>(shadeLevel * 0.125f);
    if (shadeBucket > 31) {
        shadeBucket = 31;
    } else if (shadeBucket < 0) {
        shadeBucket = 0;
    }

    g_zRndr_ActivePaletteRemapKey = (recipeIndex << 5) + shadeBucket;
}

// Reimplements 0x499990: zRndr_SetPaletteRemapKeyFromRgb01
RECOIL_NOINLINE void RECOIL_FASTCALL zRndr_SetPaletteRemapKeyFromRgb01(zColorRgb *rgb01,
                                                                       float shadeLevel) {
    if (rgb01 == 0) {
        g_zRndr_ActivePaletteRemapKey = -1;
        return;
    }

    zVidPaletteRemapRecipe recipe = {0};
    recipe.color1R = rgb01->red;
    recipe.color1G = rgb01->green;
    recipe.color1B = rgb01->blue;
    recipe.color1Strength = 1.0f;
    zRndr_SetPaletteRemapKey(&recipe, shadeLevel);
}

// Reimplements 0x499a00: zRndr_SetPaletteShadeRecipeIndex
RECOIL_NOINLINE void RECOIL_FASTCALL
zRndr_SetPaletteShadeRecipeIndex(zVidPaletteRemapRecipe *recipe) {
    g_zRndr_ActivePaletteShadeRecipeIndex =
        recipe != 0 ? zVid_PaletteRemap_BuildPaletteVariant(recipe) : -1;
}
