
#include "zgeo.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" const char g_zGeometry_WeilerIntersectBufferEntryFailedFmt[0x32]
    = "%s %d: weiler_intersect call to bufEntry failed.\n";

namespace {
/**
 * Data evidence: BN 0x4dfdd0 is int32_t[0x51], xrefed by 0x468fa0, and matches these case ids byte-for-byte.
 * Purpose: Map the four ternary edge-side sign classes to the Weiler intersection case id.
 */
const int kIntersect2dCaseIdBySignClass[0x51] = { 0,
    0,
    0,
    0,
    2,
    0,
    0,
    0,
    0,
    0,
    2,
    2,
    14,
    2,
    2,
    8,
    23,
    2,
    0,
    2,
    2,
    13,
    2,
    2,
    5,
    22,
    0,
    0,
    18,
    6,
    2,
    2,
    15,
    2,
    2,
    0,
    0,
    2,
    2,
    2,
    3,
    2,
    2,
    2,
    0,
    0,
    2,
    2,
    12,
    2,
    2,
    7,
    21,
    0,
    0,
    19,
    4,
    2,
    2,
    16,
    2,
    2,
    0,
    2,
    20,
    9,
    2,
    2,
    17,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    2,
    0,
    0,
    0,
    0 };

/**
 * Data evidence: BN 0x468f80 is uint8_t[0x18], xrefed by 0x468c40, and matches these output kinds byte-for-byte.
 * Purpose: Map each Weiler intersection case id to the output crossing construction branch.
 */
const unsigned char kIntersect2dOutputKindByXingType[0x18]
    = { 0, 6, 6, 6, 1, 1, 2, 2, 3, 3, 6, 6, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5 };

/**
 * Data evidence: BN 0x4dff14..0x4e0349 is the contiguous zgeo_weiler.cpp diagnostic
 * literal owner linked by geometry_model_assets.zgeometry_weiler_initialized_data.
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x27
 * @recoil-artifact defines .data recoil:data:0x4dff14: g_zGeometry_WeilerInitFailedMsg.
 * Purpose: Preserve the source-visible error/source literals used by Weiler diagnostics.
 */
const char g_zGeometry_WeilerInitFailedMsg[0x27] = "weiler_init call to weilerInit failed.";
const char g_zGeometry_SourceFile_ZgeoWeilerCpp[0x2e] = "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_weiler.cpp";
const char g_zGeometry_BadClipRegionForWeilerClipMsg[0x27] = "Bad clip region passed to Weiler Clip.";
const char g_zGeometry_WeilerGatherContoursFailedFmt[0x33] = "%s %d: weiler_clip call to gatherContours failed.\n";
const char g_zGeometry_WeilerBadParametersFmt[0x30] = "%s %d: Bad parameter(s) passed to Weiler Clip.\n";
const char g_zGeometry_WeilerInitNewContourFailedFmt[0x30] = "%s %d: weilerInit call to _new_contour failed.\n";
const char g_zGeometry_WeilerInitBufferEntryFailedFmt[0x2c] = "%s %d: weilerInit call to bufEntry failed.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x17
 * @recoil-artifact defines .data recoil:data:0x4e0054: g_zGeometry_ForwardSegmentFailedMsg.
 * Purpose: Preserve the weed-out diagnostic label for failed forward segment traversal.
 */
const char g_zGeometry_ForwardSegmentFailedMsg[0x17] = "Forward Segment Failed";
const char g_zGeometry_WeedOutCoincidentSegForwardFailedFmt[0x38]
    = "%s %d: _weed_out_coincident call to segForward failed.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x12
 * @recoil-artifact defines .data recoil:data:0x4e00a4: g_zGeometry_WeedOutErrorFmt.
 * Purpose: Preserve the old zError format used by coincident-edge weed-out failures.
 */
const char g_zGeometry_WeedOutErrorFmt[0x12] = "WeedOut Error: %s";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x16
 * @recoil-artifact defines .data recoil:data:0x4e00b8: g_zGeometry_WeilerCase_BCompletelyInsideA.
 * Purpose: Preserve the Weiler case label reported when contour B is inside contour A.
 */
const char g_zGeometry_WeilerCase_BCompletelyInsideA[0x16] = "B_COMPLETELY_INSIDE_A";
const char g_zGeometry_WeilerDivideEdgeFailedFmt[0x37] = "%s %d: _weiler_intersect call to _divide_edge failed.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x1a-0x4e013c
 * @recoil-artifact defines .data recoil:data:0x4e013c: g_zGeometry_WeilerIntersectErrorFmt.
 * Purpose: Preserve the old zError format used by Weiler intersection failures.
 */
const char g_zGeometry_WeilerIntersectErrorFmt[0x1a] = "weilerIntersect Error: %s";
const char g_zGeometry_NewContourBufferEntryFailedMsg[0x2a] = "New_contour could not obtain buffer entry";
const char g_zGeometry_MergeContoursNewContourFailedFmt[0x37]
    = "%s %d: _merge_contours failed to receive new contour.\n";
const char g_zGeometry_ContourMergeValidationFailedMsg[0x22] = "contourMerge:  Failed validation\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x19
 * @recoil-artifact defines .data recoil:data:0x4e01e0: g_zGeometry_OutputContoursFoundMsg.
 * Purpose: Preserve the trace literal emitted when output contours are found.
 */
const char g_zGeometry_OutputContoursFoundMsg[0x19] = "Found to output contours";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x1a-0x4e01fc
 * @recoil-artifact defines .data recoil:data:0x4e01fc: g_zGeometry_OutputContoursFailedMsg.
 * Purpose: Preserve the diagnostic literal emitted when output contour generation fails.
 */
const char g_zGeometry_OutputContoursFailedMsg[0x1a] = "Failed to output contours";
const char g_zGeometry_OutputContourBufferEntryFailedFmt[0x2f] = "%s %d: outputContour call to bufEntry failed.\n";
const char g_zGeometry_DivideEdgeBufferEntryFailedFmt[0x2e] = "%s %d: _divide_edge call to bufEntry failed.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x10
 * @recoil-artifact defines .data recoil:data:0x4e0278: g_zGeometry_BufferEntryFailedMsg.
 * Purpose: Preserve the shared Weiler buffer-entry failure diagnostic label.
 */
const char g_zGeometry_BufferEntryFailedMsg[0x10] = "bufEntry failed";
const char g_zGeometry_GenerateOutsideResultsBufferEntryFailedFmt[0x35]
    = "%s %d: _gen._outside_rslts call to buf_entry failed\n";
const char g_zGeometry_Intersect2dBufferEntryFailedFmt[0x2e] = "%s %d: _intersect2d call to buf_entry failed\n";
const char g_zGeometry_ValidateXingNullFmt[0x2e] = "validateXing failed (xing %d) xing_p is NULL!";
const char g_zGeometry_ValidateXingTypeFmt[0x2a] = "validateXing failed (xing %d) (type = %d)";

struct WeilerPreclassifyContourPacket {
    zGeometry_WeilerContourOutputPartial contourA;
    zGeometry_WeilerContourOutputPartial contourB;
    zGeometry_WeilerContourOutputPartial contourC;
    zGeometry_WeilerContourOutputPartial contourD;
};

RECOIL_STATIC_ASSERT(sizeof(WeilerPreclassifyContourPacket) == 0x30);

// Retail 0x464f70 tests whether two coincident segments run the same way in both x and y.
#define WEILER_SAME_DIRECTION_XY(aStart, aEnd, cStart, cEnd)                                                           \
    ((((aEnd)->x >= (aStart)->x && (cEnd)->x >= (cStart)->x)                                                           \
         || ((aEnd)->x <= (aStart)->x && (cEnd)->x <= (cStart)->x))                                                    \
        && (((aEnd)->y >= (aStart)->y && (cEnd)->y >= (cStart)->y)                                                     \
            || ((aEnd)->y <= (aStart)->y && (cEnd)->y <= (cStart)->y)))

struct WeilerPointBoundsXY {
    float minX;
    float maxX;
    float minY;
    float maxY;
};

} // namespace

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-getinputcontourapointlist
 * @recoil-artifact defines .text recoil:function:0x464670: zGeometry_Weiler::GetInputContourAPointList
 * @recoil-match byte
 *
 * Purpose: expose contour A's point buffer and count from an initialized
 * Weiler state.
 */
int __fastcall GetInputContourAPointList(zGeometry_WeilerStatePartial* self, zVec3** outPoints)
{
    if (self == 0) {
        return 0;
    }

    *outPoints = (zVec3*)(self->inputContourABuffer.base);
    return self->inputContourABuffer.count;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-init-0x464680
 * @recoil-artifact defines .text recoil:function:0x464680: zGeometry_Weiler::Init
 * @recoil-match byte
 *
 * Purpose: Allocate and initialize Weiler clip state from an input contour.
 */
zGeometry_WeilerStatePartial* __fastcall Init(zVec3* points, int pointCount, int contourSource)
{
    if (pointCount == 0 || points == 0) {
        zError::ReportOld(
            0x200,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x20d,
            g_zGeometry_BadClipRegionForWeilerClipMsg
        );
    }

    const int dedupedPointCount = zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY(points, pointCount);

    zGeometry_WeilerStatePartial* const result
        = (zGeometry_WeilerStatePartial*)(calloc(1, sizeof(zGeometry_WeilerStatePartial)));

    zGeometry_WeilerBuffer::Init(&result->segmentBuffer, 0x80, sizeof(zGeometry_WeilerContourSegmentPartial));
    zGeometry_WeilerBuffer::Init(&result->contourBuffer, 0x80, 0x0c);
    zGeometry_WeilerBuffer::Init(&result->xingBuffer, 0x80, 0x30);
    zGeometry_WeilerBuffer::Init(&result->inputContourABuffer, dedupedPointCount, sizeof(zVec3));

    const size_t pointBytes = (size_t)(dedupedPointCount) * sizeof(zVec3);
    memcpy(result->inputContourABuffer.base, points, pointBytes);
    result->inputContourABuffer.count = dedupedPointCount;
    result->inputContourBBuffer.count = 0;
    result->inputContourBBuffer.base = 0;

    if (contourSource != 0) {
        zGeometry_Weiler::TogglePointAxesForContourSource(result);
    }

    result->contourSource = contourSource;
    zGeometry_Weiler::RecenterPointSetsIfOutOfRange(result);

    if (zGeometry_Weiler::InitInputContourPair(result, points, dedupedPointCount, 1) == 0) {
        zError::ReportOld(0x200, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x24c, g_zGeometry_WeilerInitFailedMsg);
        zGeometry_Weiler::DestroyState(result);
        return 0;
    }

    return result;
}

} // namespace zGeometry_Weiler

namespace zGeometry_ClipPolygon {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-resetweilerstatefromcontourpoints
 * @recoil-artifact defines .text recoil:function:0x464790: zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints
 * @recoil-match byte
 *
 * Purpose: Replace the clip polygon's Weiler state from a point list while preserving the old contour source.
 */
int __fastcall
ResetWeilerStateFromContourPoints(zGeometry_ClipPolygonPartial* clipPolygon, zVec3* points, int pointCount)
{
    if (points == 0 || pointCount == 0) {
        return 0;
    }

    zGeometry_WeilerStatePartial* const newState
        = zGeometry_Weiler::Init(points, pointCount, clipPolygon->weilerState->contourSource);
    zGeometry_Weiler::DestroyState(clipPolygon->weilerState);
    clipPolygon->weilerState = newState;
    return 1;
}

} // namespace zGeometry_ClipPolygon

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-destroystate
 * @recoil-artifact defines .text recoil:function:0x4647d0: zGeometry_Weiler::DestroyState
 * @recoil-match byte
 *
 * Purpose: Release Weiler clip state buffers and state storage.
 */
void __fastcall DestroyState(zGeometry_WeilerStatePartial* self)
{
    if (self == 0) {
        return;
    }

    zGeometry_WeilerBuffer::Destroy(&self->segmentBuffer);
    zGeometry_WeilerBuffer::Destroy(&self->contourBuffer);
    zGeometry_WeilerBuffer::Destroy(&self->xingBuffer);
    zGeometry_WeilerBuffer::Destroy(&self->inputContourABuffer);
    free(self);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-clippointlist
 * @recoil-artifact defines .text recoil:function:0x464810: zGeometry_Weiler::ClipPointList
 * @recoil-match byte
 *
 * Purpose: Initialize clip output state, handle preclassified contour relationships, dispatch the Weiler clipping
 * pipeline, and restore caller-visible output state.
 */
int __fastcall ClipPointList(
    zGeometry_WeilerStatePartial* self,
    int clipMode,
    zVec3* points,
    int pointCount,
    zGeometry_WeilerClipOutputPartial* outClip
)
{
    if (self == 0 || self->inputContourABuffer.count == 0 || pointCount == 0 || points == 0 || outClip == 0) {
        fprintf(stderr, g_zGeometry_WeilerBadParametersFmt, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x2a0);
        return 0;
    }

    self->clipMode = clipMode;
    self->inputContourBBuffer.base = points;
    self->inputContourBBuffer.count = pointCount;

    if (self->contourSource != 0) {
        zGeometry_Weiler::TogglePointAxesForContourSource(self);
    }

    if (self->pointsRecentered) {
        zGeometry_Weiler::RecenterPointSetsIfOutOfRange(self);
    }

    zGeometry_WeilerBuffer::Init(&self->polygonSetABuffer, 0x80, sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&self->polygonSetBBuffer, 0x80, sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&self->polygonSetCBuffer, 0x80, sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&self->pointListBuffer, 0x80, sizeof(zVec3));

    self->outClip = outClip;
    outClip->polygonSetA.polygonCount = 0;
    outClip->polygonSetA.polygons = (zGeometry_PolygonPointSpanPartial*)(self->polygonSetABuffer.base);
    outClip->polygonSetB.polygonCount = 0;
    outClip->polygonSetB.polygons = (zGeometry_PolygonPointSpanPartial*)(self->polygonSetBBuffer.base);
    outClip->polygonSetC.polygonCount = 0;
    outClip->polygonSetC.polygons = (zGeometry_PolygonPointSpanPartial*)(self->polygonSetCBuffer.base);
    outClip->pointList.pointCount = 0;
    outClip->pointList.points = (zVec3*)(self->pointListBuffer.base);

    const int preclassifiedMode = zGeometry_Weiler::ClassifyInputContourPairBounds(self);
    if (preclassifiedMode == 1) {
        return 1;
    }

    if (preclassifiedMode == 4) {
        if (zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(self, 3) != 0) {
            if (self->pointsRecentered) {
                zGeometry_Weiler::RestorePointTranslation(self);
            }

            if (self->contourSource != 0) {
                zGeometry_Weiler::TogglePointAxesForContourSource(self);
            }

            return 3;
        }

        zGeometry_WeilerClipOutput::Destroy(outClip);
        if (self->pointsRecentered) {
            zGeometry_Weiler::RestorePointTranslation(self);
        }

        if (self->contourSource != 0) {
            zGeometry_Weiler::TogglePointAxesForContourSource(self);
        }

        return 0;
    }

    zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs(self);
    if (zGeometry_Weiler::InitInputContourPair(self, (zVec3*)(self->inputContourBBuffer.base), pointCount, 2) == 0) {
        if (self->pointsRecentered) {
            zGeometry_Weiler::RestorePointTranslation(self);
        }

        if (self->contourSource != 0) {
            zGeometry_Weiler::TogglePointAxesForContourSource(self);
        }

        zGeometry_WeilerClipOutput::Destroy(outClip);
        return 0;
    }

    zGeometry_Weiler::BuildPointSideTablesForContourPair(self);
    if (zGeometry_Weiler::PreclassifyInputContourPair(self) == 0) {
        if (self->pointsRecentered) {
            zGeometry_Weiler::RestorePointTranslation(self);
        }

        if (self->contourSource != 0) {
            zGeometry_Weiler::TogglePointAxesForContourSource(self);
        }

        zGeometry_WeilerClipOutput::Destroy(outClip);
        return 0;
    }

    int clipResult = zGeometry_Weiler::ClassifyContainedContour(self);
    if (clipResult == 1) {
        if (self->pointsRecentered) {
            zGeometry_Weiler::RestorePointTranslation(self);
        }

        if (self->contourSource != 0) {
            zGeometry_Weiler::TogglePointAxesForContourSource(self);
        }

        zGeometry_WeilerClipOutput::Destroy(outClip);
        return 0;
    }

    if (clipResult != 0) {
        if (zGeometry_Weiler::MergeContours(self) == 0) {
            if (self->pointsRecentered) {
                zGeometry_Weiler::RestorePointTranslation(self);
            }

            if (self->contourSource != 0) {
                zGeometry_Weiler::TogglePointAxesForContourSource(self);
            }

            zGeometry_WeilerClipOutput::Destroy(outClip);
            return 0;
        }

        zGeometry_Weiler::NewContour(self);

        if (self->allContoursSingleSided != true || preclassifiedMode == 2) {
            if (zGeometry_Weiler::OutputContoursForClipMode(self) == 0) {
                fprintf(stderr, g_zGeometry_WeilerGatherContoursFailedFmt, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x3b2);
                if (self->pointsRecentered) {
                    zGeometry_Weiler::RestorePointTranslation(self);
                }

                if (self->contourSource != 0) {
                    zGeometry_Weiler::TogglePointAxesForContourSource(self);
                }

                zGeometry_WeilerClipOutput::Destroy(outClip);
                return 0;
            }

            if (self->pointsRecentered) {
                zGeometry_Weiler::RestorePointTranslation(self);
            }

            zGeometry_Weiler::RestoreOutputZFromInputPlane(self);
            if (self->contourSource != 0) {
                zGeometry_Weiler::TogglePointAxesForContourSource(self);
            }

            return 2;
        }

        self->outClip->polygonSetB.polygonCount = 0;
        self->outClip->polygonSetA.polygonCount = 0;
    }

    int outputMode;
    if (preclassifiedMode == 2) {
        if (zGeometry_Weiler::GenerateOutsideResults(self) == 0
            || zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(self, 4) == 0) {
            zGeometry_WeilerClipOutput::Destroy(outClip);
            if (self->pointsRecentered) {
                zGeometry_Weiler::RestorePointTranslation(self);
            }

            if (self->contourSource != 0) {
                zGeometry_Weiler::TogglePointAxesForContourSource(self);
            }

            return 0;
        }

        outputMode = 4;
    } else if (preclassifiedMode == 3) {
        if (zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(self, preclassifiedMode) == 0) {
            zGeometry_WeilerClipOutput::Destroy(outClip);
            if (self->pointsRecentered) {
                zGeometry_Weiler::RestorePointTranslation(self);
            }

            if (self->contourSource != 0) {
                zGeometry_Weiler::TogglePointAxesForContourSource(self);
            }

            return 0;
        }

        outputMode = 3;
    } else {
        outputMode = 1;
    }

    if (self->pointsRecentered) {
        zGeometry_Weiler::RestorePointTranslation(self);
    }

    if (outputMode != 1) {
        zGeometry_Weiler::RestoreOutputZFromInputPlane(self);
    }

    if (self->contourSource != 0) {
        zGeometry_Weiler::TogglePointAxesForContourSource(self);
    }

    return outputMode;
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerClipOutput {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-destroy-0x464b30
 * @recoil-artifact defines .text recoil:function:0x464b30: zGeometry_WeilerClipOutput::Destroy
 * @recoil-match byte
 *
 * Purpose: Free and clear the point list and three polygon-set buffers owned by a clip output.
 */
void __fastcall Destroy(zGeometry_WeilerClipOutputPartial* self)
{
    if (self == 0) {
        return;
    }

    if (self->pointList.points != 0) {
        free(self->pointList.points);
        self->pointList.points = 0;
    }

    if (self->polygonSetA.polygons != 0) {
        free(self->polygonSetA.polygons);
        self->polygonSetA.polygons = 0;
    }

    if (self->polygonSetB.polygons != 0) {
        free(self->polygonSetB.polygons);
        self->polygonSetB.polygons = 0;
    }

    if (self->polygonSetC.polygons != 0) {
        free(self->polygonSetC.polygons);
        self->polygonSetC.polygons = 0;
    }
}

} // namespace zGeometry_WeilerClipOutput

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-initinputcontourpair
 * @recoil-artifact defines .text recoil:function:0x464b90: zGeometry_Weiler::InitInputContourPair
 * @recoil-match byte
 *
 * Purpose: Allocate forward and reverse contour segment rings for an input contour.
 */
int __fastcall InitInputContourPair(zGeometry_WeilerStatePartial* self, zVec3* points, int pointCount, int contourType)
{
    zGeometry_WeilerContourSegmentPartial* segments
        = (zGeometry_WeilerContourSegmentPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
            &self->segmentBuffer,
            pointCount * 2,
            0
        ));
    if (segments == 0) {
        fprintf(stderr, g_zGeometry_WeilerInitBufferEntryFailedFmt, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x455);
        return 0;
    }

    zGeometry_WeilerContourSegmentArray::InitFromPointList(segments, points, pointCount, contourType);
    segments->contourOutput = 0;
    if (zGeometry_Weiler::EnsureContourOutput(self, segments) == 0) {
        fprintf(stderr, g_zGeometry_WeilerInitNewContourFailedFmt, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x468);
        return 0;
    }

    zGeometry_WeilerContourSegmentArray::UpdateBounds(segments, pointCount);

    zGeometry_WeilerContourSegmentPartial* const reverseSegments = &segments[pointCount];
    zGeometry_WeilerContourSegmentArray::InitFromPointList(reverseSegments, points, pointCount, 4);
    reverseSegments->contourOutput = 0;
    if (zGeometry_Weiler::EnsureContourOutput(self, reverseSegments) == 0) {
        fprintf(stderr, g_zGeometry_WeilerInitNewContourFailedFmt, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x485);
        return 0;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifyinputcontourpairbounds
 * @recoil-artifact defines .text recoil:function:0x464c90: zGeometry_Weiler::ClassifyInputContourPairBounds
 *
 *
 * Purpose: Preclassify the two input contours by comparing their XY bounding boxes.
 */
int __fastcall ClassifyInputContourPairBounds(zGeometry_WeilerStatePartial* self)
{
    const int inputPointCountA = self->inputContourABuffer.count;
    const int inputPointCountB = self->inputContourBBuffer.count;
    zVec3* const inputPointsA = (zVec3*)(self->inputContourABuffer.base);
    zVec3* const inputPointsB = (zVec3*)(self->inputContourBBuffer.base);

    WeilerPointBoundsXY boundsA;
    WeilerPointBoundsXY boundsB;
    boundsB.minX = inputPointsB[0].x;
    boundsB.maxX = inputPointsB[0].x;
    boundsB.minY = inputPointsB[0].y;
    boundsB.maxY = inputPointsB[0].y;

    for (int inputPointIndexB = 1; inputPointIndexB < inputPointCountB; ++inputPointIndexB) {
        zVec3* const inputPointB = &inputPointsB[inputPointIndexB];
        if (inputPointB->x < boundsB.minX) {
            boundsB.minX = inputPointB->x;
        }

        if (inputPointB->x > boundsB.maxX) {
            boundsB.maxX = inputPointB->x;
        }

        if (inputPointB->y < boundsB.minY) {
            boundsB.minY = inputPointB->y;
        }

        if (inputPointB->y > boundsB.maxY) {
            boundsB.maxY = inputPointB->y;
        }
    }

    boundsA.minX = inputPointsA[0].x;
    boundsA.maxX = inputPointsA[0].x;
    boundsA.minY = inputPointsA[0].y;
    boundsA.maxY = inputPointsA[0].y;

    for (int inputPointIndexA = 1; inputPointIndexA < inputPointCountA; ++inputPointIndexA) {
        zVec3* const inputPointA = &inputPointsA[inputPointIndexA];
        if (inputPointA->x < boundsA.minX) {
            boundsA.minX = inputPointA->x;
        }

        if (inputPointA->x > boundsA.maxX) {
            boundsA.maxX = inputPointA->x;
        }

        if (inputPointA->y < boundsA.minY) {
            boundsA.minY = inputPointA->y;
        }

        if (inputPointA->y > boundsA.maxY) {
            boundsA.maxY = inputPointA->y;
        }
    }

    if (boundsB.minX >= boundsA.maxX || boundsB.maxX <= boundsA.minX || boundsB.minY >= boundsA.maxY
        || boundsB.maxY <= boundsA.minY) {
        return 1;
    }

    if (boundsB.minX <= boundsA.minX && boundsB.maxX >= boundsA.maxX && boundsB.minY <= boundsA.minY
        && boundsB.maxY >= boundsA.maxY) {
        return zGeometry_Weiler::OutputPreclassifiedContourPairResult(
            inputPointCountA,
            inputPointsA,
            inputPointCountB,
            inputPointsB,
            2
        );
    }

    if (boundsB.minX >= boundsA.minX && boundsB.maxX <= boundsA.maxX && boundsB.minY >= boundsA.minY
        && boundsB.maxY <= boundsA.maxY) {
        return zGeometry_Weiler::OutputPreclassifiedContourPairResult(
            inputPointCountB,
            inputPointsB,
            inputPointCountA,
            inputPointsA,
            3
        );
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-outputpreclassifiedcontourpairresult
 * @recoil-artifact defines .text recoil:function:0x464ea0: zGeometry_Weiler::OutputPreclassifiedContourPairResult
 * @recoil-match byte
 *
 * Purpose: Resolve a preclassified containment result by rejecting unmatched outside points.
 */
int __fastcall OutputPreclassifiedContourPairResult(
    int contourAPointCount,
    zVec3* contourAPoints,
    int contourBPointCount,
    zVec3* contourBPoints,
    int resultCode
)
{
    if (resultCode == 2 && contourAPointCount == contourBPointCount) {
        bool pointMatched = true;
        zVec3* contourBPoint = contourBPoints;

        for (int i = contourBPointCount; pointMatched && i != 0; --i) {
            int j = contourAPointCount;
            zVec3* contourAPoint = contourAPoints;
            pointMatched = false;

            for (; j != 0; --j, ++contourAPoint) {
                if (fabs((double)(contourAPoint->x) - (double)(contourBPoint->x)) <= 0.0010000000474974513
                    && fabs((double)(contourAPoint->y) - (double)(contourBPoint->y)) <= 0.0010000000474974513) {
                    pointMatched = true;
                    break;
                }
            }

            ++contourBPoint;
        }

        if (pointMatched) {
            return 4;
        }
    }

    while (contourAPointCount-- != 0) {
        if (zGeometry_Weiler::ClassifyPointInContourPointListXY(contourAPoints, contourBPointCount, contourBPoints)
            < 0) {
            return 0;
        }

        ++contourAPoints;
    }

    return resultCode;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-preclassifyinputcontourpair
 * @recoil-artifact defines .text recoil:function:0x464f70: zGeometry_Weiler::PreclassifyInputContourPair
 * @recoil-match byte
 *
 * Purpose: Preclassify overlapping input contours by splitting coincident segments and merging contour type flags.
 */
bool __fastcall PreclassifyInputContourPair(zGeometry_WeilerStatePartial* self)
{
    // Recovered from retail 0x464f70: unsigned A/B segment loops (the A bound is re-read from the state), A's
    // endpoints kept as a two-entry array (retail homes them at the frame top), the four endpoint-on-segment
    // tests evaluated before the switch, and an x/y direction-agreement test for the coincident cases.
    WeilerPreclassifyContourPacket* const contourPacket = (WeilerPreclassifyContourPacket*)(self->contourBuffer.base);
    const int contourBPointCount = self->inputContourBBuffer.count;
    zGeometry_WeilerContourSegmentPartial* contourA = contourPacket->contourA.firstSegment;
    zGeometry_WeilerContourSegmentPartial* contourB = contourPacket->contourB.firstSegment;
    zGeometry_WeilerContourSegmentPartial* const contourCFirst = contourPacket->contourC.firstSegment;
    zGeometry_WeilerContourSegmentPartial* const contourDFirst = contourPacket->contourD.firstSegment;
    zGeometry_WeilerContourSegmentPartial* contourC;
    zGeometry_WeilerContourSegmentPartial* contourD;
    float* contourASides;
    unsigned int contourAIndex = 0;
    float* contourBSides = self->contourBPointSideByContourAEdge;
    zVec3* aPoint[2]; // [0] start, [1] end of the current A segment
    zVec3* cStart;
    zVec3* cEnd;
    unsigned int contourBIndex;

    while (contourAIndex < self->inputContourABuffer.count) {
        contourASides = &self->contourAPointSideByContourBEdge[contourAIndex];
        contourC = contourCFirst;
        aPoint[0] = contourA->startPoint;
        aPoint[1] = contourA->endPoint;
        contourD = contourDFirst;

        for (contourBIndex = 0; contourBIndex < contourBPointCount; ++contourBIndex) {
            cStart = contourC->startPoint;
            cEnd = contourC->endPoint;

            if (fabs(contourASides[0]) < 0.0000099999997473787516 && fabs(contourASides[1]) < 0.0000099999997473787516
                && fabs(contourBSides[0]) < 0.0000099999997473787516
                && fabs(contourBSides[1]) < 0.0000099999997473787516) {
                const int aStartOnC = zGeometry_Vec3::IsBetweenEndpointsXY(aPoint[0], cStart, cEnd);
                const int aEndOnC = zGeometry_Vec3::IsBetweenEndpointsXY(aPoint[1], cStart, cEnd);
                const int cStartOnA = zGeometry_Vec3::IsBetweenEndpointsXY(cStart, aPoint[0], aPoint[1]);
                const int cEndOnA = zGeometry_Vec3::IsBetweenEndpointsXY(cEnd, aPoint[0], aPoint[1]);

                switch ((((aStartOnC << 1 | aEndOnC) << 1 | cStartOnA) << 1 | cEndOnA)) {
                case 3:
                    if (WEILER_SAME_DIRECTION_XY(aPoint[0], aPoint[1], cStart, cEnd)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(self, contourA, contourB, cEnd, 0, 0)
                            == 0) {
                            zError::ReportOld(
                                0x100,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x568,
                                g_zGeometry_WeedOutErrorFmt,
                                g_zGeometry_WeilerCase_BCompletelyInsideA
                            );
                            return 0;
                        }

                        aPoint[1] = cStart;
                        contourB->endPoint = cStart;
                        contourA->endPoint = cStart;
                        contourC->contourType |= contourA->contourType;
                        contourD->contourType |= contourB->contourType;
                    } else {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(self, contourA, contourB, cStart, 0, 0)
                            == 0) {
                            zError::ReportOld(
                                0x100,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x572,
                                g_zGeometry_WeedOutErrorFmt,
                                g_zGeometry_WeilerCase_BCompletelyInsideA
                            );
                            return 0;
                        }

                        aPoint[1] = cEnd;
                        contourB->endPoint = cEnd;
                        contourA->endPoint = cEnd;
                        contourC->contourType |= contourB->contourType;
                        contourD->contourType |= contourA->contourType;
                    }

                    zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                    break;

                case 5:
                    if (!(fabs(aPoint[1]->x - cEnd->x) <= 0.0010000000474974513
                            && fabs(aPoint[1]->y - cEnd->y) <= 0.0010000000474974513)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self,
                                contourA,
                                contourB,
                                cEnd,
                                contourD->contourType,
                                contourC->contourType
                            )
                            == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x593
                            );
                            return 0;
                        }

                        contourB->endPoint = cEnd;
                        contourA->endPoint = cEnd;
                        contourD->endPoint = aPoint[1];
                        contourC->endPoint = aPoint[1];
                        aPoint[1] = contourA->endPoint;
                        zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                        zGeometry_WeilerContourSegment::UpdateBounds(contourC);
                    }
                    break;

                case 6:
                    if (!(fabs(aPoint[1]->x - cStart->x) <= 0.0010000000474974513
                            && fabs(aPoint[1]->y - cStart->y) <= 0.0010000000474974513)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(self, contourC, contourD, aPoint[1], 0, 0)
                            == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x5b6
                            );
                            return 0;
                        }

                        contourD->endPoint = aPoint[1];
                        contourC->endPoint = aPoint[1];
                        aPoint[1] = cStart;
                        contourB->endPoint = cStart;
                        contourA->endPoint = cStart;
                        contourC->contourType |= contourA->contourType;
                        contourD->contourType |= contourB->contourType;
                        zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                        zGeometry_WeilerContourSegment::UpdateBounds(contourC);
                    }
                    break;

                case 7:
                    if (WEILER_SAME_DIRECTION_XY(aPoint[0], aPoint[1], cStart, cEnd)) {
                        aPoint[1] = cEnd;
                        contourB->endPoint = cEnd;
                        contourA->endPoint = cEnd;
                        contourC->contourType |= contourB->contourType;
                        contourD->contourType |= contourA->contourType;
                    } else {
                        aPoint[1] = cStart;
                        contourB->endPoint = cStart;
                        contourA->endPoint = cStart;
                        contourC->contourType |= contourA->contourType;
                        contourD->contourType |= contourB->contourType;
                    }

                    zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                    break;

                case 9:
                    if (!(fabs(aPoint[0]->x - cEnd->x) <= 0.0010000000474974513
                            && fabs(aPoint[0]->y - cEnd->y) <= 0.0010000000474974513)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(self, contourA, contourB, cEnd, 0, 0)
                            == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x5f0
                            );
                            return 0;
                        }

                        aPoint[1] = cEnd;
                        contourB->endPoint = cEnd;
                        contourA->endPoint = cEnd;
                        contourD->endPoint = aPoint[0];
                        contourC->endPoint = aPoint[0];
                        contourA->contourType |= contourC->contourType;
                        contourB->contourType |= contourD->contourType;
                        zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                        zGeometry_WeilerContourSegment::UpdateBounds(contourC);
                    }
                    break;

                case 10:
                    if (!(fabs(aPoint[0]->x - cStart->x) <= 0.0010000000474974513
                            && fabs(aPoint[0]->y - cStart->y) <= 0.0010000000474974513)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(self, contourA, contourB, cStart, 0, 0)
                            == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x610
                            );
                            return 0;
                        }

                        contourB->endPoint = cStart;
                        contourA->endPoint = cStart;
                        contourD->startPoint = aPoint[0];
                        contourC->startPoint = aPoint[0];
                        aPoint[0] = cStart;
                        contourA->contourType |= contourD->contourType;
                        contourB->contourType |= contourC->contourType;
                        zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                        zGeometry_WeilerContourSegment::UpdateBounds(contourC);
                    }
                    break;

                case 11:
                    if (WEILER_SAME_DIRECTION_XY(aPoint[0], aPoint[1], cStart, cEnd)) {
                        aPoint[0] = cEnd;
                        contourB->startPoint = cEnd;
                        contourA->startPoint = cEnd;
                        contourC->contourType |= contourA->contourType;
                        contourD->contourType |= contourB->contourType;
                    } else {
                        aPoint[0] = cStart;
                        contourB->startPoint = cStart;
                        contourA->startPoint = cStart;
                        contourC->contourType |= contourB->contourType;
                        contourD->contourType |= contourA->contourType;
                    }

                    zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                    break;

                case 12:
                    if (WEILER_SAME_DIRECTION_XY(aPoint[0], aPoint[1], cStart, cEnd)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(self, contourC, contourD, aPoint[1], 0, 0)
                            == 0) {
                            zError::ReportOld(
                                0x100,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x64a,
                                g_zGeometry_WeedOutErrorFmt,
                                g_zGeometry_ForwardSegmentFailedMsg
                            );
                            return 0;
                        }

                        contourD->endPoint = aPoint[0];
                        contourC->endPoint = aPoint[0];
                    } else {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(self, contourC, contourD, aPoint[0], 0, 0)
                            == 0) {
                            zError::ReportOld(
                                0x100,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x650,
                                g_zGeometry_WeedOutErrorFmt,
                                g_zGeometry_ForwardSegmentFailedMsg
                            );
                            return 0;
                        }

                        contourD->endPoint = aPoint[1];
                        contourC->endPoint = aPoint[1];
                    }

                    // Retail merges the type flags before refreshing C's bounds.
                    contourA->contourType |= contourC->contourType;
                    contourB->contourType |= contourD->contourType;
                    zGeometry_WeilerContourSegment::UpdateBounds(contourC);
                    break;

                case 13:
                    if (WEILER_SAME_DIRECTION_XY(aPoint[0], aPoint[1], cStart, cEnd)) {
                        contourD->endPoint = aPoint[0];
                        contourC->endPoint = aPoint[0];
                        contourA->contourType |= contourC->contourType;
                        contourB->contourType |= contourD->contourType;
                    } else {
                        contourD->endPoint = aPoint[1];
                        contourC->endPoint = aPoint[1];
                        contourA->contourType |= contourD->contourType;
                        contourB->contourType |= contourC->contourType;
                    }

                    zGeometry_WeilerContourSegment::UpdateBounds(contourC);
                    break;

                case 14:
                    if (WEILER_SAME_DIRECTION_XY(aPoint[0], aPoint[1], cStart, cEnd)) {
                        contourD->startPoint = aPoint[1];
                        contourC->startPoint = aPoint[1];
                        contourA->contourType |= contourC->contourType;
                        contourB->contourType |= contourD->contourType;
                    } else {
                        contourD->startPoint = aPoint[0];
                        contourC->startPoint = aPoint[0];
                        contourA->contourType |= contourD->contourType;
                        contourB->contourType |= contourC->contourType;
                    }

                    zGeometry_WeilerContourSegment::UpdateBounds(contourC);
                    break;

                case 15:
                    if (WEILER_SAME_DIRECTION_XY(aPoint[0], aPoint[1], cStart, cEnd)) {
                        contourA->contourType |= contourC->contourType;
                        contourB->contourType |= contourD->contourType;
                    } else {
                        contourB->contourType |= contourC->contourType;
                        contourA->contourType |= contourD->contourType;
                    }

                    if (contourC == contourCFirst) {
                        contourPacket->contourC.firstSegment = contourC->next;
                        contourPacket->contourD.firstSegment = contourD->next;
                        contourC->next->contourOutput = &contourPacket->contourC;
                        contourD->next->contourOutput = &contourPacket->contourD;
                    }

                    contourC->prev->next = contourC->next;
                    contourC->next->prev = contourC->prev;
                    contourD->prev->next = contourD->next;
                    contourD->next->prev = contourD->prev;
                    break;
                }
            }

            contourASides += self->inputContourABuffer.count + 1;
            ++contourBSides;
            ++contourC;
            ++contourD;
        }

        ++contourBSides;
        ++contourA;
        ++contourB;
        ++contourAIndex;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifycontainedcontour
 * @recoil-artifact defines .text recoil:function:0x465ac0: zGeometry_Weiler::ClassifyContainedContour
 *
 *
 * Purpose: Classify contained contour pairs by intersecting segment rings, splitting at crossings, and repairing
 * crossing back-references.
 */
int __fastcall ClassifyContainedContour(zGeometry_WeilerStatePartial* self)
{
    // Recovered from retail 0x465ac0: one switch over the Intersect2d case id, with per-case edge splits,
    // adjacent-edge classification switches, xing links and a skip over freshly split C/D segments.
    WeilerPreclassifyContourPacket* const contourPacket = (WeilerPreclassifyContourPacket*)(self->contourBuffer.base);
    zGeometry_WeilerContourSegmentPartial* contourASegment = contourPacket->contourA.firstSegment;
    zGeometry_WeilerContourSegmentPartial* contourBSegment = contourPacket->contourB.firstSegment;
    zGeometry_WeilerContourSegmentPartial* contourCSegment = contourPacket->contourC.firstSegment;
    zGeometry_WeilerContourSegmentPartial* contourDSegment = contourPacket->contourD.firstSegment;
    zGeometry_WeilerContourSegmentPartial* const contourAFirst = contourASegment;
    zGeometry_WeilerContourSegmentPartial* contourCFirst;
    zGeometry_WeilerContourSegmentPartial* segment;
    zGeometry_WeilerXingPartial* xing;
    zVec3* aStart;
    zVec3* aEnd;
    zVec3* cStart;
    zVec3* cEnd;
    int aggregateIntersectResult = 0;
    int skipNextSegment = 0;
    int outerIndex = 0;
    int innerIndex;
    int intersectResult;

    do {
        aStart = contourASegment->startPoint;
        aEnd = contourASegment->endPoint;
        innerIndex = 0;
        contourCFirst = contourCSegment;

        do {
            cStart = contourCSegment->startPoint;
            cEnd = contourCSegment->endPoint;
            if (contourASegment->boundsDirty != 0) {
                zGeometry_WeilerContourSegment::UpdateBounds(contourASegment);
            }

            if (contourCSegment->boundsDirty != 0) {
                zGeometry_WeilerContourSegment::UpdateBounds(contourCSegment);
            }

            if (contourASegment->minX <= contourCSegment->maxX && contourASegment->maxX >= contourCSegment->minX
                && contourASegment->minY <= contourCSegment->maxY && contourASegment->maxY >= contourCSegment->minY
                && !(
                    (contourCSegment->startXing != 0
                        && (contourCSegment->startXing == contourASegment->startXing
                            || contourCSegment->startXing == contourASegment->endXing
                            || contourCSegment->startXing == contourBSegment->startXing
                            || contourCSegment->startXing == contourBSegment->endXing))
                    || (contourCSegment->endXing != 0
                        && (contourCSegment->endXing == contourASegment->startXing
                            || contourCSegment->endXing == contourASegment->endXing
                            || contourCSegment->endXing == contourBSegment->startXing
                            || contourCSegment->endXing == contourBSegment->endXing))
                    || (contourDSegment->startXing != 0
                        && (contourDSegment->startXing == contourASegment->startXing
                            || contourDSegment->startXing == contourASegment->endXing
                            || contourDSegment->startXing == contourBSegment->startXing
                            || contourDSegment->startXing == contourBSegment->endXing))
                    || (contourDSegment->endXing != 0
                        && (contourDSegment->endXing == contourASegment->startXing
                            || contourDSegment->endXing == contourASegment->endXing
                            || contourDSegment->endXing == contourBSegment->startXing
                            || contourDSegment->endXing == contourBSegment->endXing))
                )) {
                intersectResult = zGeometry_Weiler::Intersect2d(self, &xing, *aStart, *aEnd, *cStart, *cEnd);
                if (intersectResult == 1) {
                    zError::ReportOld(
                        0x100,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0x735,
                        g_zGeometry_WeilerIntersectErrorFmt
                    );
                    return 1;
                }

                if (intersectResult != 0) {
                    if (xing != 0) {
                        xing->segment6 = 0;
                        xing->segment7 = 0;
                        xing->segment4 = 0;
                        xing->segment5 = 0;
                        xing->segment2 = 0;
                        xing->segment3 = 0;
                        xing->segment0 = 0;
                        xing->segment1 = 0;
                    }

                    switch (intersectResult) {
                    case 3: {
                        const int aStartOnC = zGeometry_Vec3::IsBetweenEndpointsXY(aStart, cStart, cEnd);
                        const int aEndOnC = zGeometry_Vec3::IsBetweenEndpointsXY(aEnd, cStart, cEnd);
                        const int cStartOnA = zGeometry_Vec3::IsBetweenEndpointsXY(cStart, aStart, aEnd);
                        const int cEndOnA = zGeometry_Vec3::IsBetweenEndpointsXY(cEnd, aStart, aEnd);
                        switch ((((aStartOnC * 2 | aEndOnC) << 1 | cStartOnA) << 1 | cEndOnA)) {
                        case 5:
                            xing = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
                                &self->xingBuffer,
                                1,
                                0
                            ));
                            if (xing == 0) {
                                zError::ReportOld(
                                    0x100,
                                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                    0x76a,
                                    g_zGeometry_WeilerIntersectErrorFmt
                                );
                                return 1;
                            }
                            xing->segment6 = 0;
                            xing->segment7 = 0;
                            xing->segment4 = 0;
                            xing->segment5 = 0;
                            xing->segment2 = 0;
                            xing->segment3 = 0;
                            xing->segment0 = 0;
                            xing->segment1 = 0;
                            if (contourCSegment != contourCSegment->next
                                && fabs(contourCSegment->endPoint->x - contourCSegment->next->startPoint->x)
                                    <= 0.0010000000474974513
                                && fabs(contourCSegment->endPoint->y - contourCSegment->next->startPoint->y)
                                    <= 0.0010000000474974513) {
                                xing->xingType = 0x14;
                                contourDSegment->endXing = xing;
                                contourASegment->endXing = xing;
                            } else {
                                xing->xingType = 0x17;
                                contourCSegment->endXing = xing;
                                contourBSegment->endXing = xing;
                            }
                            xing->point = *aEnd;
                            break;

                        case 6:
                            xing = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
                                &self->xingBuffer,
                                1,
                                0
                            ));
                            if (xing == 0) {
                                fprintf(
                                    stderr,
                                    g_zGeometry_WeilerIntersectBufferEntryFailedFmt,
                                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                    0x78a
                                );
                                return 1;
                            }
                            xing->segment6 = 0;
                            xing->segment7 = 0;
                            xing->segment4 = 0;
                            xing->segment5 = 0;
                            xing->segment2 = 0;
                            xing->segment3 = 0;
                            xing->segment0 = 0;
                            xing->segment1 = 0;
                            xing->xingType = 0x11;
                            xing->point = *aEnd;
                            contourDSegment->startXing = xing;
                            contourBSegment->endXing = xing;
                            break;

                        case 9:
                            xing = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
                                &self->xingBuffer,
                                1,
                                0
                            ));
                            if (xing == 0) {
                                fprintf(
                                    stderr,
                                    g_zGeometry_WeilerIntersectBufferEntryFailedFmt,
                                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                    0x7a2
                                );
                                return 1;
                            }
                            xing->segment6 = 0;
                            xing->segment7 = 0;
                            xing->segment4 = 0;
                            xing->segment5 = 0;
                            xing->segment2 = 0;
                            xing->segment3 = 0;
                            xing->segment0 = 0;
                            xing->segment1 = 0;
                            xing->xingType = 0x15;
                            xing->point = *cEnd;
                            contourDSegment->endXing = xing;
                            contourBSegment->startXing = xing;
                            break;

                        case 10:
                            xing = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
                                &self->xingBuffer,
                                1,
                                0
                            ));
                            if (xing == 0) {
                                fprintf(
                                    stderr,
                                    g_zGeometry_WeilerIntersectBufferEntryFailedFmt,
                                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                    0x7bd
                                );
                                return 1;
                            }
                            xing->segment6 = 0;
                            xing->segment7 = 0;
                            xing->segment4 = 0;
                            xing->segment5 = 0;
                            xing->segment2 = 0;
                            xing->segment3 = 0;
                            xing->segment0 = 0;
                            xing->segment1 = 0;
                            if (contourCSegment != contourCSegment->prev
                                && fabs(contourCSegment->startPoint->x - contourCSegment->prev->endPoint->x)
                                    <= 0.0010000000474974513
                                && fabs(contourCSegment->startPoint->y - contourCSegment->prev->endPoint->y)
                                    <= 0.0010000000474974513) {
                                xing->xingType = 0xc;
                                contourDSegment->startXing = xing;
                                contourASegment->startXing = xing;
                            } else {
                                xing->xingType = 0xf;
                                contourCSegment->startXing = xing;
                                contourBSegment->startXing = xing;
                            }
                            xing->point = *aStart;
                            break;
                        }
                    } break;

                    case 4:
                    case 5:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourASegment, 1) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourBSegment, 1)
                                == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourCSegment, 1)
                                == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourDSegment, 1)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x7ea
                            );
                            return 1;
                        }
                        skipNextSegment = 1;
                        aEnd = contourASegment->endPoint;
                        break;

                    case 13:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourASegment, 1) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourBSegment, 0)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x7fc
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourCSegment->prev,
                            contourCSegment,
                            contourBSegment
                        )) {
                        case 2:
                            xing->xingType = 0x18;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            break;
                        case 1:
                            xing->xingType = 0x18;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 7:
                            xing->xingType = 5;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 0:
                            contourDSegment->startXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        }
                        aEnd = contourASegment->endPoint;
                        break;

                    case 16:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourASegment, 0) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourBSegment, 1)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x829
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourCSegment->prev,
                            contourCSegment,
                            contourBSegment
                        )) {
                        case 2:
                            xing->xingType = 0x19;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            break;
                        case 1:
                            xing->xingType = 0x19;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 7:
                            xing->xingType = 4;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 0:
                            contourDSegment->startXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        }
                        aEnd = contourASegment->endPoint;
                        break;

                    case 19:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourASegment, 1) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourBSegment, 0)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x85e
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourCSegment,
                            contourCSegment->next,
                            contourASegment
                        )) {
                        case 2:
                            xing->xingType = 0x18;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            break;
                        case 1:
                            xing->xingType = 0x18;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 7:
                            xing->xingType = 4;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 0:
                            contourDSegment->endXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        }
                        aEnd = contourASegment->endPoint;
                        break;

                    case 22:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourASegment, 0) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourBSegment, 1)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x893
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourCSegment,
                            contourCSegment->next,
                            contourBSegment
                        )) {
                        case 2:
                            xing->xingType = 0x19;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            break;
                        case 1:
                            xing->xingType = 0x19;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 7:
                            xing->xingType = 5;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 0:
                            contourDSegment->endXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        }
                        aEnd = contourASegment->endPoint;
                        break;

                    case 6:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourCSegment, 1) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourDSegment, 0)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x8c9
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourASegment->prev,
                            contourASegment,
                            contourCSegment
                        )) {
                        case 2:
                            xing->xingType = 0xa;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->contourType |= 3;
                            contourASegment->prev->contourType |= 3;
                            break;
                        case 1:
                            xing->xingType = 0xa;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            contourASegment->contourType |= 3;
                            contourASegment->prev->contourType |= 3;
                            break;
                        case 7:
                            xing->xingType = 4;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 0:
                            contourBSegment->startXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        }
                        skipNextSegment = 1;
                        break;

                    case 7:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourCSegment, 0) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourDSegment, 1)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x904
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourASegment->prev,
                            contourASegment,
                            contourDSegment
                        )) {
                        case 2:
                            xing->xingType = 0xb;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            break;
                        case 1:
                            xing->xingType = 0xb;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 7:
                            xing->xingType = 5;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 0:
                            contourBSegment->startXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        }
                        skipNextSegment = 1;
                        break;

                    case 8:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourCSegment, 1) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourDSegment, 0)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x939
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourASegment,
                            contourASegment->next,
                            contourCSegment
                        )) {
                        case 2:
                            xing->xingType = 0xa;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourASegment->contourType |= 3;
                            contourASegment->next->contourType |= 3;
                            break;
                        case 1:
                            xing->xingType = 0xa;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourASegment->contourType |= 3;
                            contourASegment->next->contourType |= 3;
                            break;
                        case 7:
                            xing->xingType = 5;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        case 0:
                            contourBSegment->endXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        }
                        skipNextSegment = 1;
                        break;

                    case 9:
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourCSegment, 0) == 0
                            || zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, contourDSegment, 1)
                                == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x974
                            );
                            return 1;
                        }
                        switch (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                            contourASegment,
                            contourASegment->next,
                            contourDSegment
                        )) {
                        case 2:
                            xing->xingType = 0xb;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            break;
                        case 1:
                            xing->xingType = 0xb;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        case 7:
                            xing->xingType = 4;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        case 0:
                            contourBSegment->endXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        }
                        skipNextSegment = 1;
                        break;

                    case 12: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment->prev,
                            contourCSegment,
                            contourASegment->prev,
                            contourASegment,
                            self
                        );
                        switch (edgeClass) {
                        case 4:
                            xing->xingType = 0x18;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 3:
                            xing->xingType = 0x18;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 0:
                            contourDSegment->startXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        }
                    } break;

                    case 15: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment->prev,
                            contourCSegment,
                            contourBSegment->prev,
                            contourBSegment,
                            self
                        );
                        switch (edgeClass) {
                        case 6:
                            xing->xingType = 0xa;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->contourType |= 2;
                            contourASegment->prev->contourType |= 2;
                            break;
                        case 5:
                            xing->xingType = 0x19;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 0:
                            contourCSegment->startXing = xing;
                            contourBSegment->startXing = xing;
                            break;
                        }
                    } break;

                    case 14: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment->prev,
                            contourCSegment,
                            contourBSegment,
                            contourBSegment->next,
                            self
                        );
                        switch (edgeClass) {
                        case 4:
                            xing->xingType = 0x18;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        case 3:
                            xing->xingType = 0x18;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 0:
                            contourCSegment->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        }
                    } break;

                    case 17: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment->prev,
                            contourCSegment,
                            contourBSegment,
                            contourBSegment->next,
                            self
                        );
                        switch (edgeClass) {
                        case 5:
                        case 6:
                            xing->xingType = 0x19;
                            // Retail tests for class 3 here although only classes 5 and 6 reach this arm.
                            if (edgeClass == 3) {
                                contourASegment->next->startXing = xing;
                                contourASegment->endXing = xing;
                            } else {
                                contourBSegment->next->startXing = xing;
                                contourBSegment->endXing = xing;
                            }
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->prev->endXing = xing;
                            contourDSegment->startXing = xing;
                            contourCSegment->prev->endXing = xing;
                            contourCSegment->startXing = xing;
                            break;
                        case 0:
                            contourDSegment->startXing = xing;
                            contourBSegment->endXing = xing;
                            break;
                        }
                    } break;

                    case 18: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment,
                            contourCSegment->next,
                            contourBSegment->prev,
                            contourBSegment,
                            self
                        );
                        switch (edgeClass) {
                        case 4:
                            xing->xingType = 0x18;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 3:
                            xing->xingType = 0x18;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 0:
                            contourCSegment->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        }
                    } break;

                    case 21: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment,
                            contourCSegment->next,
                            contourBSegment->prev,
                            contourBSegment,
                            self
                        );
                        switch (edgeClass) {
                        case 4:
                            xing->xingType = 0x18;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 3:
                            xing->xingType = 0x18;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->prev->endXing = xing;
                            contourBSegment->startXing = xing;
                            contourASegment->prev->endXing = xing;
                            contourASegment->startXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 0:
                            contourDSegment->endXing = xing;
                            contourBSegment->startXing = xing;
                            break;
                        }
                    } break;

                    case 20: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment,
                            contourCSegment->next,
                            contourBSegment,
                            contourBSegment->next,
                            self
                        );
                        switch (edgeClass) {
                        case 4:
                            xing->xingType = 0x18;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        case 3:
                            xing->xingType = 0x18;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 0:
                            contourDSegment->endXing = xing;
                            contourASegment->endXing = xing;
                            break;
                        }
                    } break;

                    case 23: {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourCSegment,
                            contourCSegment->next,
                            contourBSegment,
                            contourBSegment->next,
                            self
                        );
                        switch (edgeClass) {
                        case 6:
                            xing->xingType = 0xa;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            contourASegment->contourType |= 2;
                            contourASegment->next->contourType |= 2;
                            break;
                        case 5:
                            xing->xingType = 0x19;
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            break;
                        case 8:
                        case 9:
                            if (edgeClass == 8) {
                                xing->xingType = 5;
                            } else {
                                xing->xingType = 4;
                            }
                            contourBSegment->next->startXing = xing;
                            contourBSegment->endXing = xing;
                            contourASegment->next->startXing = xing;
                            contourASegment->endXing = xing;
                            contourDSegment->next->startXing = xing;
                            contourDSegment->endXing = xing;
                            contourCSegment->next->startXing = xing;
                            contourCSegment->endXing = xing;
                            break;
                        case 0:
                            contourCSegment->endXing = xing;
                            contourBSegment->endXing = xing;
                            break;
                        }
                    } break;
                    }

                    if (skipNextSegment != 0) {
                        ++innerIndex;
                        contourCSegment = contourCSegment->next;
                        contourDSegment = contourDSegment->next;
                        skipNextSegment = 0;
                    }
                }

                aggregateIntersectResult |= intersectResult;
            }

            ++innerIndex;
            contourCSegment = contourCSegment->next;
            contourDSegment = contourDSegment->next;
        } while (contourCSegment != contourCFirst);

        ++outerIndex;
        contourASegment = contourASegment->next;
        contourBSegment = contourBSegment->next;
    } while (contourASegment != contourAFirst);

    if (self->xingBuffer.count != 0) {
        segment = contourAFirst;
        do {
            xing = segment->startXing;
            if (xing != 0) {
                xing->segment2 = segment;
            }

            xing = segment->endXing;
            if (xing != 0) {
                xing->segment0 = segment;
            }

            xing = contourBSegment->startXing;
            if (xing != 0) {
                xing->segment3 = contourBSegment;
            }

            xing = contourBSegment->endXing;
            if (xing != 0) {
                xing->segment1 = contourBSegment;
            }

            segment = segment->next;
            contourBSegment = contourBSegment->next;
        } while (segment != contourAFirst);

        segment = contourCFirst;
        do {
            xing = segment->startXing;
            if (xing != 0) {
                xing->segment6 = segment;
            }

            xing = segment->endXing;
            if (xing != 0) {
                xing->segment4 = segment;
            }

            xing = contourDSegment->startXing;
            if (xing != 0) {
                xing->segment7 = contourDSegment;
            }

            xing = contourDSegment->endXing;
            if (xing != 0) {
                xing->segment5 = contourDSegment;
            }

            segment = segment->next;
            contourDSegment = contourDSegment->next;
        } while (segment != contourCFirst);
    }

    return aggregateIntersectResult;
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerBuffer {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-init-0x467600
 * @recoil-artifact defines .text recoil:function:0x467600: zGeometry_WeilerBuffer::Init
 * @recoil-match byte
 *
 * Purpose: Allocate zero-filled Weiler buffer storage and initialize append state.
 */
void __fastcall Init(zGeometry_WeilerBufferPartial* self, int initialCapacity, int elementSize)
{
    void* const base = calloc(initialCapacity, elementSize);
    self->capacity = initialCapacity;
    self->base = base;
    self->elementSize = elementSize;
    self->count = 0;
    self->appendPtr = base;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-destroy-0x467630
 * @recoil-artifact defines .text recoil:function:0x467630: zGeometry_WeilerBuffer::Destroy
 * @recoil-match byte
 *
 * Purpose: Release backing storage and clear buffer bookkeeping.
 */
void __fastcall Destroy(zGeometry_WeilerBufferPartial* self)
{
    if (self->base != 0) {
        free(self->base);
        self->capacity = 0;
        self->count = 0;
        self->elementSize = 0;
        self->base = 0;
        self->appendPtr = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-getappendspace
 * @recoil-artifact defines .text recoil:function:0x467660: zGeometry_WeilerBuffer::GetAppendSpace
 * @recoil-match byte
 *
 * Purpose: Reserve contiguous append slots, growing backing storage when needed.
 */
void* __fastcall GetAppendSpace(zGeometry_WeilerBufferPartial* self, int appendCount, void** outBase)
{
    const int newCount = self->count + appendCount;
    if ((unsigned int)(newCount) >= (unsigned int)(self->capacity)) {
        self->capacity += appendCount + 0x10;
        void* const base = realloc(self->base, self->capacity * self->elementSize);
        self->base = base;
        self->appendPtr = (void*)((unsigned int)(base) + self->elementSize * self->count);

        if (outBase != 0) {
            *outBase = base;
        }
    }

    void* const result = self->appendPtr;
    self->count = newCount;
    self->appendPtr = (void*)((unsigned int)(self->appendPtr) + appendCount * self->elementSize);
    return result;
}

} // namespace zGeometry_WeilerBuffer

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-ensurecontouroutput
 * @recoil-artifact defines .text recoil:function:0x4676c0: zGeometry_Weiler::EnsureContourOutput
 * @recoil-match byte
 *
 * Purpose: Ensure a contour segment has an attached contour output record.
 */
int __fastcall EnsureContourOutput(zGeometry_WeilerStatePartial* self, zGeometry_WeilerContourSegmentPartial* segment)
{
    if (segment->contourOutput == 0) {
        zGeometry_WeilerContourOutputPartial* const contourOutput
            = (zGeometry_WeilerContourOutputPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
                &self->contourBuffer,
                1,
                0
            ));

        if (contourOutput == 0) {
            zError::ReportOld(
                0x200,
                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                0xc6f,
                g_zGeometry_NewContourBufferEntryFailedMsg
            );
            return 0;
        }

        contourOutput->firstSegment = segment;
        contourOutput->contourType = segment->contourType;
        segment->contourOutput = contourOutput;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-mergecontours
 * @recoil-artifact defines .text recoil:function:0x467710: zGeometry_Weiler::MergeContours
 * @recoil-match byte
 *
 * Purpose: Merge classified Weiler contour segments into contour output chains.
 */
int __fastcall MergeContours(zGeometry_WeilerStatePartial* self)
{
    zGeometry_WeilerXingPartial* const xingBase = (zGeometry_WeilerXingPartial*)(self->xingBuffer.base);
    if (zGeometry_Weiler::ValidateXings(self->xingBuffer.count, xingBase, 0) == 0) {
        zError::ReportOld(
            0x100,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0xc9a,
            g_zGeometry_ContourMergeValidationFailedMsg
        );
        return 0;
    }

    zGeometry_WeilerXingPartial* xing = xingBase;
    for (unsigned int xingIndex = 0; xingIndex < (unsigned int)(self->xingBuffer.count); ++xingIndex, ++xing) {
        zGeometry_WeilerContourSegmentPartial* const segment0 = xing->segment0;
        zGeometry_WeilerContourSegmentPartial* const segment1 = xing->segment1;
        zGeometry_WeilerContourSegmentPartial* const segment2 = xing->segment2;
        zGeometry_WeilerContourSegmentPartial* const segment3 = xing->segment3;
        zGeometry_WeilerContourSegmentPartial* const segment4 = xing->segment4;
        zGeometry_WeilerContourSegmentPartial* const segment5 = xing->segment5;
        zGeometry_WeilerContourSegmentPartial* const segment6 = xing->segment6;
        zGeometry_WeilerContourSegmentPartial* const segment7 = xing->segment7;

        switch (xing->xingType - 3) {
        case 0:
            break;

        case 1:
            segment0->next = segment5;
            segment1->next = segment7;
            segment2->prev = segment4;
            segment3->prev = segment6;
            segment4->next = segment2;
            segment5->next = segment0;
            segment6->prev = segment3;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xcc5
                );
                return 0;
            }
            break;

        case 2:
            segment0->next = segment6;
            segment1->next = segment4;
            segment2->prev = segment7;
            segment3->prev = segment5;
            segment4->next = segment1;
            segment5->next = segment3;
            segment6->prev = segment0;
            segment7->prev = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xce0
                );
                return 0;
            }
            break;

        case 10:
            segment0->next = segment6;
            segment2->prev = segment7;
            segment6->prev = segment0;
            segment7->prev = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xcf5
                );
                return 0;
            }
            break;

        case 13:
            segment1->next = segment7;
            segment3->prev = segment6;
            segment6->prev = segment3;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xd0a
                );
                return 0;
            }
            break;

        case 16:
            segment0->next = segment5;
            segment2->prev = segment4;
            segment4->next = segment2;
            segment5->next = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xd1f
                );
                return 0;
            }
            break;

        case 19:
            segment1->next = segment4;
            segment3->prev = segment5;
            segment4->next = segment1;
            segment5->next = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xd34
                );
                return 0;
            }
            break;

        case 21:
            if (segment5 != 0) {
                segment0->next = segment5;
                segment2->prev = segment7;
                segment5->next = segment0;
                segment7->prev = segment2;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment5) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xd4a
                    );
                    return 0;
                }
            } else {
                segment0->next = segment6;
                segment2->prev = segment4;
                segment4->next = segment2;
                segment6->prev = segment0;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xd5a
                    );
                    return 0;
                }
            }
            break;

        case 22:
            if (segment5 != 0) {
                segment5->next = segment3;
                segment3->prev = segment5;
                segment7->prev = segment1;
                segment1->next = segment7;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment5) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xd71
                    );
                    return 0;
                }
            } else {
                segment4->next = segment3;
                segment3->prev = segment4;
                segment6->prev = segment1;
                segment1->next = segment6;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xd82
                    );
                    return 0;
                }
            }
            break;

        case 9:
            segment2->prev = segment7;
            segment7->prev = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xd95
                );
                return 0;
            }
            break;

        case 11:
            segment0->next = segment6;
            segment6->prev = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xda7
                );
                return 0;
            }
            break;

        case 12:
            segment3->prev = segment6;
            segment6->prev = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xdb9
                );
                return 0;
            }
            break;

        case 14:
            segment1->next = segment7;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xdcb
                );
                return 0;
            }
            break;

        case 15:
            segment2->prev = segment4;
            segment4->next = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xddd
                );
                return 0;
            }
            break;

        case 17:
            segment0->next = segment5;
            segment5->next = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xdef
                );
                return 0;
            }
            break;

        case 18:
            segment3->prev = segment5;
            segment5->next = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment3) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xe01
                );
                return 0;
            }
            break;

        case 20:
            segment1->next = segment4;
            segment4->next = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xe13
                );
                return 0;
            }
            break;

        case 3:
            segment2->prev = segment4;
            segment3->prev = segment6;
            segment4->next = segment2;
            segment6->prev = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xe28
                );
                return 0;
            }
            break;

        case 4:
            segment2->prev = segment7;
            segment3->prev = segment5;
            segment5->next = segment3;
            segment7->prev = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment5) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xe3d
                );
                return 0;
            }
            break;

        case 5:
            segment0->next = segment6;
            segment1->next = segment4;
            segment4->next = segment1;
            segment6->prev = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xe52
                );
                return 0;
            }
            break;

        case 6:
            segment0->next = segment5;
            segment1->next = segment7;
            segment5->next = segment0;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0
                || zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xe67
                );
                return 0;
            }
            break;

        case 7:
            if (segment1 != 0) {
                segment1->next = segment4;
                segment3->prev = segment6;
                segment4->next = segment1;
                segment6->prev = segment3;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment3) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xe7d
                    );
                    return 0;
                }
            } else {
                segment0->next = segment6;
                segment2->prev = segment4;
                segment4->next = segment2;
                segment6->prev = segment0;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xe8d
                    );
                    return 0;
                }
            }
            break;

        case 8:
            if (segment1 != 0) {
                segment1->next = segment7;
                segment7->prev = segment1;
                segment3->prev = segment5;
                segment5->next = segment3;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment3) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xea4
                    );
                    return 0;
                }
            } else {
                segment0->next = segment7;
                segment7->prev = segment0;
                segment2->prev = segment5;
                segment5->next = segment2;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0
                    || zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xeb4
                    );
                    return 0;
                }
            }
            break;

        default:
            break;
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-newcontour
 * @recoil-artifact defines .text recoil:function:0x4680b0: zGeometry_Weiler::NewContour
 *
 *
 * Purpose: Rebuild contour output type and point counts, clear stale segment output ownership, and track
 * all-single-sided state.
 */
void __fastcall NewContour(zGeometry_WeilerStatePartial* self)
{
    int contourCount = self->contourBuffer.count;
    zGeometry_WeilerContourOutputPartial* contour = (zGeometry_WeilerContourOutputPartial*)(self->contourBuffer.base);

    self->allContoursSingleSided = true;
    if (contourCount == 0) {
        return;
    }

    while (contourCount != 0) {
        if (contour->firstSegment != 0) {
            zGeometry_WeilerContourSegmentPartial* const firstSegment = contour->firstSegment;
            int primarySide = firstSegment->contourType & 3;
            contour->contourType = firstSegment->contourType;

            if (firstSegment->contourType == 6) {
                zGeometry_WeilerContourSegmentPartial* const oldPrev = firstSegment->prev;
                firstSegment->prev = firstSegment->next;
                firstSegment->next = oldPrev;
            }

            contour->pointCount = 1;
            zGeometry_WeilerContourSegmentPartial* segment
                = zGeometry_Weiler::GetNextContourSegmentForTraversal(firstSegment);

            while (segment != firstSegment) {
                if (primarySide == 0 && (segment->contourType & 3) != 0) {
                    primarySide = 1;
                    contour->contourType |= segment->contourType;
                    contour->pointCount = 1;

                    zGeometry_WeilerContourSegmentPartial* const oldNext = firstSegment->next;
                    firstSegment->next = firstSegment->prev;
                    firstSegment->prev = oldNext;

                    zVec3* const oldStart = firstSegment->startPoint;
                    firstSegment->startPoint = firstSegment->endPoint;
                    firstSegment->endPoint = oldStart;

                    segment = zGeometry_Weiler::GetNextContourSegmentForTraversal(firstSegment);
                } else {
                    ++contour->pointCount;
                    contour->contourType |= segment->contourType;

                    zGeometry_WeilerContourOutputPartial* const oldOutput = segment->contourOutput;
                    if (oldOutput != 0) {
                        oldOutput->firstSegment = 0;
                        segment->contourOutput = 0;
                    }
                    segment = zGeometry_Weiler::GetNextContourSegmentForTraversal(segment);
                }
            }

            if ((contour->contourType & 3) == 3) {
                self->allContoursSingleSided = false;
            }
        }

        ++contour;
        --contourCount;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-outputcontoursforclipmode
 * @recoil-artifact defines .text recoil:function:0x4681a0: zGeometry_Weiler::OutputContoursForClipMode
 * @recoil-match byte
 *
 * Purpose: Route contour outputs to polygon sets A, B, and C according to clip mode bits and contour type.
 */
int __fastcall OutputContoursForClipMode(zGeometry_WeilerStatePartial* self)
{
    int contourCount = self->contourBuffer.count;
    zGeometry_WeilerContourOutputPartial* contour = (zGeometry_WeilerContourOutputPartial*)(self->contourBuffer.base);

    if (contourCount == 0) {
        return 1;
    }

    while (contourCount != 0) {
        if (contour->firstSegment != 0) {
            if ((self->clipMode & 1) != 0 && contour->contourType == 3
                && zGeometry_Weiler::OutputContourToPolygonSet(
                       self,
                       contour,
                       &self->polygonSetABuffer,
                       &self->outClip->polygonSetA
                   ) == 0) {
                zError::ReportOld(
                    0x200,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0xf5e,
                    g_zGeometry_OutputContoursFailedMsg
                );
                return 0;
            }

            if ((self->clipMode & 2) != 0) {
                const int contourType = contour->contourType;
                if ((contourType == 6 || contourType == 2)
                    && zGeometry_Weiler::OutputContourToPolygonSet(
                           self,
                           contour,
                           &self->polygonSetBBuffer,
                           &self->outClip->polygonSetB
                       ) == 0) {
                    zError::ReportOld(
                        0x200,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xf71,
                        g_zGeometry_OutputContoursFailedMsg
                    );
                    return 0;
                }
            }

            if ((self->clipMode & 4) != 0) {
                const int contourType = contour->contourType;
                if ((contourType == 1 || contourType == 5)
                    && zGeometry_Weiler::OutputContourToPolygonSet(
                           self,
                           contour,
                           &self->polygonSetCBuffer,
                           &self->outClip->polygonSetC
                       ) == 0) {
                    zError::ReportOld(
                        0x100,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0xf7d,
                        g_zGeometry_OutputContoursFoundMsg
                    );
                    return 0;
                }
            }
        }

        --contourCount;
        ++contour;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-outputcontourtopolygonset
 * @recoil-artifact defines .text recoil:function:0x4682c0: zGeometry_Weiler::OutputContourToPolygonSet
 * @recoil-match byte
 *
 * Purpose: Append a polygon span and copy contour segment points into the output point list.
 */
int __fastcall OutputContourToPolygonSet(
    zGeometry_WeilerStatePartial* self,
    zGeometry_WeilerContourOutputPartial* contour,
    zGeometry_WeilerBufferPartial* polygonBuffer,
    zGeometry_PolygonSpanArrayPartial* polygonSet
)
{
    zGeometry_WeilerContourSegmentPartial* segment = contour->firstSegment;
    zGeometry_WeilerContourSegmentPartial* const lastSegment = segment->prev;
    zGeometry_WeilerClipOutputPartial* const outClip = self->outClip;

    zGeometry_PolygonPointSpanPartial* const polygon
        = (zGeometry_PolygonPointSpanPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
            polygonBuffer,
            1,
            (void**)(&polygonSet->polygons)
        ));
    if (polygon == 0) {
        return 0;
    }

    polygon->pointDwordOffset = outClip->pointList.pointCount * 3;
    polygon->pointCount = contour->pointCount;
    ++polygonSet->polygonCount;

    zVec3* outPoint = (zVec3*)(zGeometry_WeilerBuffer::GetAppendSpace(
        &self->pointListBuffer,
        contour->pointCount,
        (void**)(&outClip->pointList.points)
    ));
    if (outPoint == 0) {
        fprintf(stderr, g_zGeometry_OutputContourBufferEntryFailedFmt, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xfb9);
        return 0;
    }

    outClip->pointList.pointCount += contour->pointCount;

    *outPoint = *segment->startPoint;
    ++outPoint;
    do {
        *outPoint = *segment->endPoint;
        ++outPoint;

        segment = segment->next;
    } while (segment != lastSegment);

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-togglepointaxesforcontoursource
 * @recoil-artifact defines .text recoil:function:0x4683a0: zGeometry_Weiler::TogglePointAxesForContourSource
 * @recoil-match byte
 *
 * Purpose: Swap point axes in the active input contour buffer for the contour source.
 */
void __fastcall TogglePointAxesForContourSource(zGeometry_WeilerStatePartial* self)
{
    if (self->inputContourBBuffer.base != 0) {
        float* axis = (self->contourSource == 2) ? &((zVec3*)(self->inputContourBBuffer.base))->y
                                                 : &((zVec3*)(self->inputContourBBuffer.base))->x;
        float* z = &((zVec3*)(self->inputContourBBuffer.base))->z;
        for (int i = self->inputContourBBuffer.count; i != 0; --i) {
            const float value = *axis;
            *axis = *z;
            *z = value;
            axis += 3;
            z += 3;
        }
    } else {
        float* axis = (self->contourSource == 2) ? &((zVec3*)(self->inputContourABuffer.base))->y
                                                 : &((zVec3*)(self->inputContourABuffer.base))->x;
        float* z = &((zVec3*)(self->inputContourABuffer.base))->z;
        for (int i = self->inputContourABuffer.count; i != 0; --i) {
            const float value = *axis;
            *axis = *z;
            *z = value;
            axis += 3;
            z += 3;
        }
    }
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerContourSegment {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-updatebounds-0x468410
 * @recoil-artifact defines .text recoil:function:0x468410: zGeometry_WeilerContourSegment::UpdateBounds
 * @recoil-match byte
 *
 * Purpose: Refresh a contour segment's cached XY bounds from its endpoints.
 */
void __fastcall UpdateBounds(zGeometry_WeilerContourSegmentPartial* segment)
{

    zVec3* const start = segment->startPoint;
    zVec3* const end = segment->endPoint;

    if (start->x > end->x) {
        segment->minX = end->x;
        segment->maxX = start->x;
    } else {
        segment->minX = start->x;
        segment->maxX = end->x;
    }

    if (start->y > end->y) {
        segment->minY = end->y;
        segment->maxY = start->y;
    } else {
        segment->minY = start->y;
        segment->maxY = end->y;
    }

    segment->boundsDirty = 0;
}

} // namespace zGeometry_WeilerContourSegment

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-buildpointsidetablesforcontourpair
 * @recoil-artifact defines .text recoil:function:0x468470: zGeometry_Weiler::BuildPointSideTablesForContourPair
 * @recoil-match byte
 *
 * Purpose: Fill the contour A/B point-side tables used by Weiler contour-pair classification.
 */
void __fastcall BuildPointSideTablesForContourPair(zGeometry_WeilerStatePartial* self)
{
    float* table = self->contourAPointSideByContourBEdge;
    zVec3* testPoints = (zVec3*)(self->inputContourABuffer.base);
    unsigned int testCount = self->inputContourABuffer.count;
    zVec3* edgePoints = (zVec3*)(self->inputContourBBuffer.base);
    unsigned int edgeCount = self->inputContourBBuffer.count;
    for (int pass = 2; pass != 0; --pass) {
        zVec3* edgeStart = edgePoints;
        for (unsigned int edgeIndex = edgeCount - 1; edgeIndex > 0; --edgeIndex) {
            float* const rowStart = table;
            const zVec3* const edgeEnd = edgeStart + 1;
            zVec3* point = testPoints;
            for (unsigned int pointIndex = testCount; pointIndex > 0; --pointIndex) {
                *table++ = (edgeEnd->y - edgeStart->y) * (point->x - edgeStart->x)
                    - (point->y - edgeStart->y) * (edgeEnd->x - edgeStart->x);
                ++point;
            }
            *table++ = *rowStart;
            ++edgeStart;
        }

        float* const rowStart = table;
        zVec3* point = testPoints;
        for (unsigned int pointIndex = testCount; pointIndex > 0; --pointIndex) {
            *table++ = (edgePoints->y - edgeStart->y) * (point->x - edgeStart->x)
                - (point->y - edgeStart->y) * (edgePoints->x - edgeStart->x);
            ++point;
        }
        *table = *rowStart;

        edgePoints = testPoints;
        edgeCount = testCount;
        testPoints = (zVec3*)(self->inputContourBBuffer.base);
        testCount = self->inputContourBBuffer.count;
        table = self->contourBPointSideByContourAEdge;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-dividecontoursegmentatpoint
 * @recoil-artifact defines .text recoil:function:0x468580: zGeometry_Weiler::DivideContourSegmentAtPoint
 * @recoil-match byte
 *
 * Purpose: Split a contour segment at a crossing point while preserving contour links.
 */
int __fastcall DivideContourSegmentAtPoint(
    zGeometry_WeilerStatePartial* self,
    zVec3* xing,
    zGeometry_WeilerContourSegmentPartial* segment,
    int updateSplitLinks
)
{
    zGeometry_WeilerContourSegmentPartial* nextSegment;

    if (zGeometry_Vec3::IsNearEqualXY(segment->endPoint, xing, 0.00100000005f) != 0) {
        nextSegment = segment->next;
    } else {
        nextSegment = (zGeometry_WeilerContourSegmentPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
            &self->segmentBuffer,
            1,
            0
        ));
        if (nextSegment == 0) {
            fprintf(stderr, g_zGeometry_DivideEdgeBufferEntryFailedFmt, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x113a);
            return 0;
        }

        nextSegment->startPoint = xing;
        nextSegment->endPoint = segment->endPoint;

        segment->endPoint = xing;
        nextSegment->next = segment->next;
        nextSegment->prev = segment;
        segment->next->prev = nextSegment;
        segment->next = nextSegment;

        nextSegment->contourType = segment->contourType;
        nextSegment->contourOutput = 0;
        nextSegment->endXing = segment->endXing;
        nextSegment->boundsDirty = 1;
        segment->boundsDirty = 1;
    }

    zGeometry_WeilerXingPartial* const xingLink = (zGeometry_WeilerXingPartial*)(xing);
    if (updateSplitLinks != 0) {
        segment->endXing = xingLink;
        nextSegment->startXing = xingLink;
    } else {
        nextSegment->startXing = 0;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-createforwardsegmentpairatpoint
 * @recoil-artifact defines .text recoil:function:0x468650: zGeometry_Weiler::CreateForwardSegmentPairAtPoint
 * @recoil-match byte
 *
 * Purpose: Insert matching forward contour split segments at a shared point.
 */
int __fastcall CreateForwardSegmentPairAtPoint(
    zGeometry_WeilerStatePartial* self,
    zGeometry_WeilerContourSegmentPartial* firstSegment,
    zGeometry_WeilerContourSegmentPartial* secondSegment,
    zVec3* point,
    int firstContourTypeMask,
    int secondContourTypeMask
)
{
    zGeometry_WeilerContourSegmentPartial* segment = firstSegment;
    int segmentCount = 2;

    while (segmentCount-- != 0) {
        zGeometry_WeilerContourSegmentPartial* const newSegment
            = (zGeometry_WeilerContourSegmentPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
                &self->segmentBuffer,
                1,
                0
            ));
        if (newSegment == 0) {
            zError::ReportOld(0x200, g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x1181, g_zGeometry_BufferEntryFailedMsg);
            return 0;
        }

        newSegment->prev = segment;
        newSegment->next = segment->next;
        newSegment->contourType = segment->contourType | firstContourTypeMask;
        newSegment->startPoint = point;
        newSegment->endPoint = segment->endPoint;
        newSegment->startXing = 0;
        newSegment->endXing = segment->endXing;
        newSegment->contourOutput = 0;
        zGeometry_WeilerContourSegment::UpdateBounds(newSegment);

        segment->next->prev = newSegment;
        segment->next = newSegment;

        segment = secondSegment;
        firstContourTypeMask = secondContourTypeMask;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-outputselectedinputcontourtopolygonseta
 * @recoil-artifact defines .text recoil:function:0x468700: zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA
 * @recoil-match byte
 *
 * Purpose: Append the selected input contour into polygon set A of the caller-owned Weiler clip output.
 */
bool __fastcall OutputSelectedInputContourToPolygonSetA(zGeometry_WeilerStatePartial* self, int mode)
{
    if ((self->clipMode & 1) == 0) {
        return true;
    }

    zGeometry_WeilerBufferPartial* selectedInputContour = &self->inputContourBBuffer;
    if (mode != 3) {
        selectedInputContour = &self->inputContourABuffer;
    }

    self->outClip->polygonSetA.polygonCount = 1;

    if ((unsigned int)(self->outClip->pointList.pointCount + selectedInputContour->count) > 0x80) {
        self->outClip->pointList.points = (zVec3*)(realloc(
            self->outClip->pointList.points,
            (size_t)(self->outClip->pointList.pointCount + selectedInputContour->count) * sizeof(zVec3)
        ));
    }

    self->outClip->polygonSetA.polygons->pointCount = selectedInputContour->count;
    self->outClip->polygonSetA.polygons->pointDwordOffset = self->outClip->pointList.pointCount * 3;
    memcpy(
        &self->outClip->pointList.points[self->outClip->pointList.pointCount],
        selectedInputContour->base,
        (size_t)(selectedInputContour->count) * sizeof(zVec3)
    );

    self->outClip->pointList.pointCount += selectedInputContour->count;
    return true;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-generateoutsideresults
 * @recoil-artifact defines .text recoil:function:0x4687b0: zGeometry_Weiler::GenerateOutsideResults
 * @recoil-match byte
 *
 * Purpose: Emit an outside-result polygon span and wrapped B/A point bridge when clip mode requests outside output.
 */
bool __fastcall GenerateOutsideResults(zGeometry_WeilerStatePartial* self)
{
    const int contourAPointCount = self->inputContourABuffer.count;
    const int contourBPointCount = self->inputContourBBuffer.count;
    zVec3* const contourBPoints = (zVec3*)(self->inputContourBBuffer.base);
    zGeometry_WeilerClipOutputPartial* const outClip = self->outClip;

    if ((self->clipMode & 2) == 0) {
        return 1;
    }

    zGeometry_PolygonPointSpanPartial* const polygon
        = (zGeometry_PolygonPointSpanPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(
            &self->polygonSetBBuffer,
            1,
            (void**)(&outClip->polygonSetB.polygons)
        ));
    if (polygon == 0) {
        fprintf(
            stderr,
            g_zGeometry_GenerateOutsideResultsBufferEntryFailedFmt,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x11f7
        );
        return 0;
    }

    polygon->pointDwordOffset = outClip->pointList.pointCount * 3;
    polygon->pointCount = contourBPointCount + contourAPointCount + 2;
    ++outClip->polygonSetB.polygonCount;

    zVec3* outPoint = (zVec3*)(zGeometry_WeilerBuffer::GetAppendSpace(
        &self->pointListBuffer,
        polygon->pointCount,
        (void**)(&outClip->pointList.points)
    ));
    if (outPoint == 0) {
        fprintf(
            stderr,
            g_zGeometry_GenerateOutsideResultsBufferEntryFailedFmt,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x120f
        );
        return 0;
    }

    outClip->pointList.pointCount += polygon->pointCount;

    zVec3* selectedContourAPoint = (zVec3*)(self->inputContourABuffer.base);
    zVec3* contourAPoint = selectedContourAPoint + 1;
    int remainingPointCount = contourAPointCount - 1;
    while (remainingPointCount-- != 0) {
        if (contourAPoint->x > selectedContourAPoint->x
            || (fabs((double)(contourAPoint->x) - (double)(selectedContourAPoint->x)) < 0.0000099999997473787516
                && contourAPoint->y > selectedContourAPoint->y)) {
            selectedContourAPoint = contourAPoint;
        }

        ++contourAPoint;
    }

    zVec3* selectedContourBPoint = contourBPoints;
    zVec3* contourBPoint = contourBPoints + 1;
    remainingPointCount = contourBPointCount - 1;
    while (remainingPointCount-- != 0) {
        if (contourBPoint->x > selectedContourBPoint->x
            || (fabs((double)(contourBPoint->x) - (double)(selectedContourBPoint->x)) < 0.0000099999997473787516
                && selectedContourBPoint->y < contourBPoint->y)) {
            selectedContourBPoint = contourBPoint;
        }

        ++contourBPoint;
    }

    zGeometry_Weiler::SelectForwardStartPointInContourA(selectedContourBPoint, &selectedContourAPoint, self);

    zVec3* const contourBLastPoint = &contourBPoints[contourBPointCount - 1];
    remainingPointCount = contourBPointCount;
    while (remainingPointCount-- != 0) {
        *outPoint++ = *selectedContourBPoint;
        selectedContourBPoint = selectedContourBPoint != contourBLastPoint ? selectedContourBPoint + 1 : contourBPoints;
    }

    *outPoint++ = *selectedContourBPoint;

    zVec3* const contourAPoints = (zVec3*)(self->inputContourABuffer.base);
    remainingPointCount = contourAPointCount;
    while (remainingPointCount-- != 0) {
        *outPoint++ = *selectedContourAPoint;
        selectedContourAPoint = selectedContourAPoint != contourAPoints
            ? selectedContourAPoint - 1
            : &((zVec3*)(self->inputContourABuffer.base))[contourAPointCount - 1];
    }

    *outPoint = *selectedContourAPoint;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifypointincontourpointlistxy
 * @recoil-artifact defines .text recoil:function:0x468a10: zGeometry_Weiler::ClassifyPointInContourPointListXY
 *
 *
 * Purpose: Classify a test point as outside, on, or inside an XY contour by crossing parity.
 */
char __fastcall ClassifyPointInContourPointListXY(zVec3* point, int contourPointCount, zVec3* contourPoints)
{
    float x = contourPoints[contourPointCount - 1].x;
    // Retail compares y double-widened (fld/fld/fcompp) while x stays a float compare.
    double y = contourPoints[contourPointCount - 1].y;
    int xSide = x < point->x ? -1 : x > point->x ? 1 : 0;
    int ySide = y < point->y ? -1 : y > point->y ? 1 : 0;

    int crossingParity = 0;
    if (xSide == 0 && ySide == 0) {
        return 0;
    }

    while (contourPointCount--) {
        const float previousX = x;
        const float previousY = y;
        const int previousXSide = xSide;
        const int previousYSide = ySide;
        x = contourPoints->x;
        y = contourPoints->y;
        xSide = x < point->x ? -1 : x > point->x ? 1 : 0;
        ySide = y < point->y ? -1 : y > point->y ? 1 : 0;
        ++contourPoints;

        if (xSide == 0 && ySide == 0) {
            return 0;
        }

        if (xSide != previousXSide) {
            if ((ySide >= 0) != (previousYSide >= 0)) {
                const float xIntersection = (point->y - y) / (previousY - y) * (previousX - x) + x;
                const float intersectionSide = xIntersection < point->x ? -1 : xIntersection > point->x ? 1 : 0;

                if (intersectionSide == 0.0f) {
                    return 0;
                }

                if (intersectionSide == 1.0f) {
                    ++crossingParity;
                }
            } else if (ySide == 0 && previousYSide == 0) {
                return 0;
            }
        } else if ((ySide >= 0) != (previousYSide >= 0)) {
            if (xSide == 1) {
                ++crossingParity;
            } else if (xSide == 0) {
                return 0;
            }
        }
    }

    if (crossingParity & 1) {
        return 1;
    }

    return -1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-intersect2d
 * @recoil-artifact defines .text recoil:function:0x468c40: zGeometry_Weiler::Intersect2d
 *
 *
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp; BN x87 rendering is limited at the classifier callsite and
 * computed Y store, so assembly is source of truth. Purpose: Build the crossing record, if any, for the classified
 * intersection between two XY edges.
 */
int __fastcall Intersect2d(
    zGeometry_WeilerStatePartial* self,
    zGeometry_WeilerXingPartial** outXing,
    zVec3 edge0Start,
    zVec3 edge0End,
    zVec3 edge1Start,
    zVec3 edge1End
)
{
    zGeometry_WeilerXingPartial* createdXing = 0;
    int xingType = zGeometry_Weiler::ClassifyIntersect2d(&edge0Start, &edge0End, &edge1Start, &edge1End, self);

    {
        switch (xingType) {
        case 4:
        case 5: {
            const double edge0DeltaX = edge0End.x - edge0Start.x;
            const double edge0DeltaY = edge0End.y - edge0Start.y;
            const double edge1ReverseDeltaX = edge1Start.x - edge1End.x;
            const double edge1ReverseDeltaY = edge1Start.y - edge1End.y;
            const double divisor = edge1ReverseDeltaY * edge0DeltaX - edge1ReverseDeltaX * edge0DeltaY;

            if (divisor != 0.0) {
                createdXing
                    = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
                if (createdXing == 0) {
                    fprintf(
                        stderr,
                        g_zGeometry_Intersect2dBufferEntryFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0x1301
                    );
                    return 1;
                }

                // Retail scales by the reciprocal divisor term by term.
                const double edge0Param = (edge1Start.x - edge0Start.x) * (edge1ReverseDeltaY * (1.0 / divisor))
                    + (edge1Start.y - edge0Start.y) * (-edge1ReverseDeltaX * (1.0 / divisor));
                createdXing->point.x = (float)(edge0DeltaX * edge0Param + edge0Start.x);
                createdXing->point.y = (float)(edge0DeltaY * edge0Param + edge0Start.y);

                // Retail recomputes the float edge deltas for the z interpolation instead of reusing the doubles.
                if (edge1Start.x - edge1End.x != 0.0) {
                    createdXing->point.z = (edge1Start.x - createdXing->point.x) / (edge1Start.x - edge1End.x)
                            * (edge1End.z - edge1Start.z)
                        + edge1Start.z;
                } else {
                    createdXing->point.z = (edge1Start.y - createdXing->point.y) / (edge1Start.y - edge1End.y)
                            * (edge1End.z - edge1Start.z)
                        + edge1Start.z;
                }
            } else {
                xingType = 0;
            }

            break;
        }

        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            createdXing
                = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0x132a
                );
                return 1;
            }

            createdXing->point.x = edge1Start.x;
            createdXing->point.y = edge1Start.y;
            createdXing->point.z = edge1Start.z;
            break;

        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
            createdXing
                = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0x1340
                );
                return 1;
            }

            createdXing->point.x = edge1End.x;
            createdXing->point.y = edge1End.y;
            createdXing->point.z = edge1End.z;
            break;

        case 6:
        case 7:
            createdXing
                = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0x1351
                );
                return 1;
            }

            createdXing->point.x = edge0Start.x;
            createdXing->point.y = edge0Start.y;
            createdXing->point.z = edge0Start.z;
            break;

        case 8:
        case 9:
            createdXing
                = (zGeometry_WeilerXingPartial*)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0x1363
                );
                return 1;
            }

            createdXing->point.x = edge0End.x;
            createdXing->point.y = edge0End.y;
            createdXing->point.z = edge0End.z;
            break;

        case 0:
            break;

        default:
            break;
        }
    }

    if (createdXing != 0) {
        createdXing->xingType = xingType;
    }

    *outXing = createdXing;
    return xingType;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifyintersect2d
 * @recoil-artifact defines .text recoil:function:0x468fa0: zGeometry_Weiler::ClassifyIntersect2d
 *
 *
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp; BN x87 sign-class HLIL is limited, so assembly is source of
 * truth. Purpose: Classify two XY edges into the Weiler intersection case table, including contour-side disambiguation
 * for vertex cases.
 */
int __fastcall ClassifyIntersect2d(
    zVec3* edge0Start,
    zVec3* edge0End,
    zVec3* edge1Start,
    zVec3* edge1End,
    zGeometry_WeilerStatePartial* self
)
{
    const float edge1DeltaY = edge1End->y - edge1Start->y;
    const float edge1DeltaX = edge1End->x - edge1Start->x;

    float edge0StartSide
        = (edge0Start->x - edge1Start->x) * edge1DeltaY - (edge0Start->y - edge1Start->y) * edge1DeltaX;
    float edge0EndSide = (edge0End->x - edge1Start->x) * edge1DeltaY - (edge0End->y - edge1Start->y) * edge1DeltaX;
    if ((edge0StartSide < 0.0f && edge0EndSide < 0.0f) || (edge0StartSide > 0.0f && edge0EndSide > 0.0f)) {
        return 0;
    }

    const float edge0DeltaY = edge0End->y - edge0Start->y;
    const float edge0DeltaX = edge0End->x - edge0Start->x;
    float edge1StartSide
        = (edge1Start->x - edge0Start->x) * edge0DeltaY - (edge1Start->y - edge0Start->y) * edge0DeltaX;
    float edge1EndSide = (edge1End->x - edge0Start->x) * edge0DeltaY - (edge1End->y - edge0Start->y) * edge0DeltaX;
    if ((edge1StartSide < 0.0f && edge1EndSide < 0.0f) || (edge1StartSide > 0.0f && edge1EndSide > 0.0f)) {
        return 0;
    }

    const int zeroSideCount = (edge0StartSide == 0.0f ? 1 : 0) + (edge0EndSide == 0.0f ? 1 : 0)
        + (edge1StartSide == 0.0f ? 1 : 0) + (edge1EndSide == 0.0f ? 1 : 0);

    if (zeroSideCount == 2) {
        zVec3 probe;

        if (edge1StartSide == 0.0f) {
            probe.x = edge1Start->x + edge1DeltaX * 0.00000999999975f;
            probe.y = edge1Start->y + edge1DeltaY * 0.00000999999975f;

            if (edge1EndSide > 0.0f) {
                if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                        &probe,
                        self->inputContourABuffer.count,
                        (zVec3*)(self->inputContourABuffer.base)
                    )
                    > 0) {
                    edge0StartSide = -edge0StartSide;
                    edge0EndSide = -edge0EndSide;
                    edge1EndSide = -edge1EndSide;
                }
            } else if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                           &probe,
                           self->inputContourABuffer.count,
                           (zVec3*)(self->inputContourABuffer.base)
                       )
                < 0) {
                edge0StartSide = -edge0StartSide;
                edge0EndSide = -edge0EndSide;
                edge1EndSide = -edge1EndSide;
            }
        } else {
            // Retail computes the end probe separately in each arm (the call setup is then hoisted).
            if (edge1StartSide > 0.0f) {
                probe.x = edge1Start->x + edge1DeltaX * 0.999989986f;
                probe.y = edge1Start->y + edge1DeltaY * 0.999989986f;
                if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                        &probe,
                        self->inputContourABuffer.count,
                        (zVec3*)(self->inputContourABuffer.base)
                    )
                    > 0) {
                    edge0StartSide = -edge0StartSide;
                    edge0EndSide = -edge0EndSide;
                    edge1StartSide = -edge1StartSide;
                }
            } else {
                probe.x = edge1Start->x + edge1DeltaX * 0.999989986f;
                probe.y = edge1Start->y + edge1DeltaY * 0.999989986f;
                if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                        &probe,
                        self->inputContourABuffer.count,
                        (zVec3*)(self->inputContourABuffer.base)
                    )
                    < 0) {
                    edge0StartSide = -edge0StartSide;
                    edge0EndSide = -edge0EndSide;
                    edge1StartSide = -edge1StartSide;
                }
            }
        }
    }

    const int edge0StartClass = edge0StartSide < 0.0 ? 0 : edge0StartSide == 0.0 ? 1 : 2;
    const int edge0EndClass = edge0EndSide < 0.0 ? 0 : edge0EndSide == 0.0 ? 1 : 2;
    const int edge1StartClass = edge1StartSide < 0.0 ? 0 : edge1StartSide == 0.0 ? 1 : 2;
    const int edge1EndClass = edge1EndSide < 0.0 ? 0 : edge1EndSide == 0.0 ? 1 : 2;
    const int index = ((edge0StartClass * 3 + edge0EndClass) * 3 + edge1StartClass) * 3 + edge1EndClass;

    return kIntersect2dCaseIdBySignClass[index];
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerContourSegmentArray {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-updatebounds-0x4693a0
 * @recoil-artifact defines .text recoil:function:0x4693a0: zGeometry_WeilerContourSegmentArray::UpdateBounds
 * @recoil-match byte
 *
 * Purpose: Refresh cached XY bounds for each segment in a contour segment array.
 */
void __fastcall UpdateBounds(zGeometry_WeilerContourSegmentPartial* segments, int segmentCount)
{
    while (segmentCount--) {
        zGeometry_WeilerContourSegment::UpdateBounds(segments++);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-initfrompointlist
 * @recoil-artifact defines .text recoil:function:0x4693c0: zGeometry_WeilerContourSegmentArray::InitFromPointList
 * @recoil-match byte
 *
 * Purpose: Build a linked contour segment ring from a point list.
 */
void __fastcall
InitFromPointList(zGeometry_WeilerContourSegmentPartial* segments, zVec3* points, int pointCount, int contourType)
{

    zVec3* point = points;
    zGeometry_WeilerContourSegmentPartial* segment = segments - 1;
    int count = pointCount;
    while (count--) {
        ++segment;
        segment->prev = segment - 1;
        segment->next = segment + 1;
        segment->contourType = contourType;
        segment->startPoint = point;
        ++point;
        segment->endPoint = point;
        segment->endXing = 0;
        segment->startXing = 0;
        segment->contourOutput = 0;
    }
    segments->prev = &segments[pointCount - 1];
    segment->next = segments;
    segment->endPoint = points;
}

} // namespace zGeometry_WeilerContourSegmentArray

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-getnextcontoursegmentfortraversal
 * @recoil-artifact defines .text recoil:function:0x469430: zGeometry_Weiler::GetNextContourSegmentForTraversal
 * @recoil-match byte
 *
 * Purpose: Advance Weiler contour traversal while reversing adjacent segment links for two-node contour cases.
 */
zGeometry_WeilerContourSegmentPartial* __fastcall
GetNextContourSegmentForTraversal(zGeometry_WeilerContourSegmentPartial* segment)
{
    zGeometry_WeilerContourSegmentPartial* const next = segment->next;

    if (next->next == segment) {
        zGeometry_WeilerContourSegmentPartial* const oldPrev = next->prev;
        next->prev = segment;
        next->next = oldPrev;

        zVec3* const oldStart = next->startPoint;
        next->startPoint = next->endPoint;
        next->endPoint = oldStart;
    }

    return next;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifyadjacentedgepairagainstcontoursegment
 * @recoil-artifact defines .text recoil:function:0x469450: zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment
 *
 *
 * Purpose: Classify whether an adjacent edge pair crosses or lies to one side of a contour segment.
 */
int __fastcall ClassifyAdjacentEdgePairAgainstContourSegment(
    zGeometry_WeilerContourSegmentPartial* firstSegment,
    zGeometry_WeilerContourSegmentPartial* secondSegment,
    zGeometry_WeilerContourSegmentPartial* contourSegment
)
{
    int result = 0;
    if (firstSegment->endPoint == secondSegment->startPoint) {
        const zVec3* const contourStart = contourSegment->startPoint;
        const zVec3* const contourEnd = contourSegment->endPoint;
        const zVec3* const firstStart = firstSegment->startPoint;
        const zVec3* const secondEnd = secondSegment->endPoint;
        const float contourDeltaY = contourEnd->y - contourStart->y;
        const float contourDeltaX = contourEnd->x - contourStart->x;
        const float firstSide
            = (firstStart->x - contourStart->x) * contourDeltaY - (firstStart->y - contourStart->y) * contourDeltaX;
        const float secondSide
            = (secondEnd->x - contourStart->x) * contourDeltaY - (secondEnd->y - contourStart->y) * contourDeltaX;

        if (!((firstSide < 0.0 || secondSide < 0.0) && (firstSide > 0.0 || secondSide > 0.0))) {
            if ((secondEnd->x - firstStart->x) * (firstSegment->endPoint->y - firstStart->y)
                    - (firstSegment->endPoint->x - firstStart->x) * (secondEnd->y - firstStart->y)
                > 0.0f) {
                return 1;
            }

            return 2;
        }

        result = 7;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifyadjacentedgepairagainstadjacentedgepair
 * @recoil-artifact defines .text recoil:function:0x469560: zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair
 *
 *
 * Purpose: Classify two linked adjacent edge pairs by their endpoint wedge relationship.
 */
int __fastcall ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
    zGeometry_WeilerContourSegmentPartial* pairAFirstSegment,
    zGeometry_WeilerContourSegmentPartial* pairASecondSegment,
    zGeometry_WeilerContourSegmentPartial* pairBFirstSegment,
    zGeometry_WeilerContourSegmentPartial* pairBSecondSegment,
    zGeometry_WeilerStatePartial*
)
{
    if (pairAFirstSegment->endPoint != pairASecondSegment->startPoint
        || pairBFirstSegment->endPoint != pairBSecondSegment->startPoint) {
        return 0;
    }

    const float pairBFirstDeltaX = pairBFirstSegment->endPoint->x - pairBFirstSegment->startPoint->x;
    const float pairBFirstDeltaY = pairBFirstSegment->endPoint->y - pairBFirstSegment->startPoint->y;
    const float pairBSecondDeltaX = pairBSecondSegment->endPoint->x - pairBSecondSegment->startPoint->x;
    const float pairBSecondDeltaY = pairBSecondSegment->endPoint->y - pairBSecondSegment->startPoint->y;
    int startClass;
    if ((pairAFirstSegment->startPoint->x - pairBFirstSegment->startPoint->x) * pairBFirstDeltaY
            - (pairAFirstSegment->startPoint->y - pairBFirstSegment->startPoint->y) * pairBFirstDeltaX
        < 0.0) {
        if ((pairAFirstSegment->startPoint->x - pairBSecondSegment->startPoint->x) * pairBSecondDeltaY
                    - (pairAFirstSegment->startPoint->y - pairBSecondSegment->startPoint->y) * pairBSecondDeltaX
                < 0.0
            || (pairBSecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) * pairBFirstDeltaX
                    - (pairBSecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) * pairBFirstDeltaY
                < 0.0) {
            startClass = 1;
        } else {
            startClass = -1;
        }
    } else if ((pairAFirstSegment->startPoint->x - pairBSecondSegment->startPoint->x) * pairBSecondDeltaY
                - (pairAFirstSegment->startPoint->y - pairBSecondSegment->startPoint->y) * pairBSecondDeltaX
            < 0.0
        && (pairBSecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) * pairBFirstDeltaX
                - (pairBSecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) * pairBFirstDeltaY
            < 0.0) {
        startClass = 1;
    } else {
        startClass = -1;
    }

    int endClass;
    if ((pairASecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) * pairBFirstDeltaY
            - (pairASecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) * pairBFirstDeltaX
        < 0.0) {
        if ((pairASecondSegment->endPoint->x - pairBSecondSegment->startPoint->x) * pairBSecondDeltaY
                    - (pairASecondSegment->endPoint->y - pairBSecondSegment->startPoint->y) * pairBSecondDeltaX
                < 0.0
            || (pairBSecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) * pairBFirstDeltaX
                    - (pairBSecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) * pairBFirstDeltaY
                < 0.0) {
            endClass = 1;
        } else {
            endClass = -1;
        }
    } else if ((pairASecondSegment->endPoint->x - pairBSecondSegment->startPoint->x) * pairBSecondDeltaY
                - (pairASecondSegment->endPoint->y - pairBSecondSegment->startPoint->y) * pairBSecondDeltaX
            < 0.0
        && (pairBSecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) * pairBFirstDeltaX
                - (pairBSecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) * pairBFirstDeltaY
            < 0.0) {
        endClass = 1;
    } else {
        endClass = -1;
    }

    if (startClass == -1 && endClass == -1) {
        const float pairAFirstDeltaX = pairAFirstSegment->endPoint->x - pairAFirstSegment->startPoint->x;
        const float pairAFirstDeltaY = pairAFirstSegment->endPoint->y - pairAFirstSegment->startPoint->y;
        const float pairASecondDeltaX = pairASecondSegment->endPoint->x - pairASecondSegment->startPoint->x;
        const float pairASecondDeltaY = pairASecondSegment->endPoint->y - pairASecondSegment->startPoint->y;
        if (((pairBFirstSegment->startPoint->x - pairAFirstSegment->startPoint->x) * pairAFirstDeltaY
                - (pairBFirstSegment->startPoint->y - pairAFirstSegment->startPoint->y) * pairAFirstDeltaX)
                < 0.0
            && ((pairBFirstSegment->startPoint->x - pairASecondSegment->startPoint->x) * pairASecondDeltaY
                   - (pairBFirstSegment->startPoint->y - pairASecondSegment->startPoint->y) * pairASecondDeltaX)
                < 0.0
            && ((pairBSecondSegment->endPoint->x - pairAFirstSegment->startPoint->x) * pairAFirstDeltaY
                   - (pairBSecondSegment->endPoint->y - pairAFirstSegment->startPoint->y) * pairAFirstDeltaX)
                < 0.0
            && ((pairBSecondSegment->endPoint->x - pairASecondSegment->startPoint->x) * pairASecondDeltaY
                   - (pairBSecondSegment->endPoint->y - pairASecondSegment->startPoint->y) * pairASecondDeltaX)
                < 0.0) {
            return 6;
        }

        return 5;
    }

    if (startClass == 1 && endClass == 1) {
        const float pairAFirstDeltaX = pairAFirstSegment->endPoint->x - pairAFirstSegment->startPoint->x;
        const float pairAFirstDeltaY = pairAFirstSegment->endPoint->y - pairAFirstSegment->startPoint->y;
        const float pairASecondDeltaX = pairASecondSegment->endPoint->x - pairASecondSegment->startPoint->x;
        const float pairASecondDeltaY = pairASecondSegment->endPoint->y - pairASecondSegment->startPoint->y;
        if (((pairBFirstSegment->startPoint->x - pairAFirstSegment->startPoint->x) * pairAFirstDeltaY
                - (pairBFirstSegment->startPoint->y - pairAFirstSegment->startPoint->y) * pairAFirstDeltaX)
                < 0.0
            && ((pairBFirstSegment->startPoint->x - pairASecondSegment->startPoint->x) * pairASecondDeltaY
                   - (pairBFirstSegment->startPoint->y - pairASecondSegment->startPoint->y) * pairASecondDeltaX)
                < 0.0
            && ((pairBSecondSegment->endPoint->x - pairAFirstSegment->startPoint->x) * pairAFirstDeltaY
                   - (pairBSecondSegment->endPoint->y - pairAFirstSegment->startPoint->y) * pairAFirstDeltaX)
                < 0.0
            && ((pairBSecondSegment->endPoint->x - pairASecondSegment->startPoint->x) * pairASecondDeltaY
                   - (pairBSecondSegment->endPoint->y - pairASecondSegment->startPoint->y) * pairASecondDeltaX)
                < 0.0) {
            return 4;
        }

        return 3;
    }

    return startClass == 1 ? 9 : 8;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-recenterpointsetsifoutofrange
 * @recoil-artifact defines .text recoil:function:0x469960: zGeometry_Weiler::RecenterPointSetsIfOutOfRange
 * @recoil-match byte
 *
 * Purpose: Translate input points when their coordinates are outside the local range.
 */
void __fastcall RecenterPointSetsIfOutOfRange(zGeometry_WeilerStatePartial* self)
{
    if (self->inputContourBBuffer.base != 0) {
        zVec3* point = (zVec3*)(self->inputContourBBuffer.base);
        for (int i = self->inputContourBBuffer.count; i != 0; --i) {
            point->x -= self->pointTranslationX;
            point->y -= self->pointTranslationY;
            ++point;
        }

        return;
    }

    zVec3* point = (zVec3*)(self->inputContourABuffer.base);
    if (point->x < 65536.0f && point->x > -65536.0f && point->y < 65536.0f && point->y > -65536.0f) {
        self->pointsRecentered = false;
        return;
    }

    self->pointTranslationX = point->x;
    self->pointTranslationY = point->y;
    self->pointsRecentered = true;

    for (int i = self->inputContourABuffer.count; i != 0; --i) {
        point->x -= self->pointTranslationX;
        point->y -= self->pointTranslationY;
        ++point;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-preclassifyinputcontouraadjacentedgepairs
 * @recoil-artifact defines .text recoil:function:0x469a30: zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs
 * @recoil-match byte
 *
 * Purpose: Reset clipping scratch buffers and seed contour A's forward and reverse adjacent-edge segment rings.
 */
void __fastcall PreclassifyInputContourAAdjacentEdgePairs(zGeometry_WeilerStatePartial* self)
{
    zGeometry_WeilerContourSegmentPartial* segments = (zGeometry_WeilerContourSegmentPartial*)self->segmentBuffer.base;
    zGeometry_WeilerContourOutputPartial* contour = (zGeometry_WeilerContourOutputPartial*)self->contourBuffer.base;

    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->segmentBuffer, self->inputContourABuffer.count << 1);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->contourBuffer, 2);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->xingBuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->polygonSetABuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->polygonSetBBuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->polygonSetCBuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->pointListBuffer, 0);

    zGeometry_WeilerContourSegmentArray::InitFromPointList(
        segments,
        (zVec3*)self->inputContourABuffer.base,
        self->inputContourABuffer.count,
        1
    );
    segments->contourOutput = contour;
    contour->firstSegment = segments;
    zGeometry_WeilerContourSegmentArray::UpdateBounds(segments, self->inputContourABuffer.count);

    segments += self->inputContourABuffer.count;
    ++contour;
    zGeometry_WeilerContourSegmentArray::InitFromPointList(
        segments,
        (zVec3*)self->inputContourABuffer.base,
        self->inputContourABuffer.count,
        4
    );
    segments->contourOutput = contour;
    contour->firstSegment = segments;
    zGeometry_WeilerContourSegmentArray::UpdateBounds(segments, self->inputContourABuffer.count);
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerBuffer {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-setcountandappendptr
 * @recoil-artifact defines .text recoil:function:0x469ae0: zGeometry_WeilerBuffer::SetCountAndAppendPtr
 * @recoil-match byte
 *
 * Purpose: Reset the logical count and append pointer within the backing store.
 */
void __fastcall SetCountAndAppendPtr(zGeometry_WeilerBufferPartial* self, int count)
{
    self->count = count;
    self->appendPtr = (void*)((unsigned int)(self->base) + count * self->elementSize);
}

} // namespace zGeometry_WeilerBuffer

namespace zMath {
/**
 * Purpose: Inline-function spelling of the reviewed vector-cross island for
 * this unit's consumers (same form as zgeo_hole.cpp and zmth_main.c).
 * Retail inline-expansion evidence: the consumer captures both edge addresses
 * and the normal destination before the 65-byte cross sequence, without a
 * call; spelling and header ownership are inferred.
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x469b60.
 */
inline void Vec3Cross(const zVec3* left, const zVec3* right, zVec3* dest)
{
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    ZMTH_VECTOR_CROSS_BODY(left, right, dest);
#else
    ZMTH_VECTOR_CROSS(left, right, dest);
#endif
}
} // namespace zMath

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-restorepointtranslation
 * @recoil-artifact defines .text recoil:function:0x469af0: zGeometry_Weiler::RestorePointTranslation
 * @recoil-match byte
 *
 * Purpose: Restore the saved XY translation to caller-owned input points and generated output points.
 */
void __fastcall RestorePointTranslation(zGeometry_WeilerStatePartial* self)
{

    const float translationX = self->pointTranslationX;
    const float translationY = self->pointTranslationY;
    int count = self->inputContourBBuffer.count;
    if (count) {
        zVec3* point = (zVec3*)self->inputContourBBuffer.base;
        do {
            point->x = translationX + point->x;
            point->y = translationY + point->y;
            ++point;
        } while (--count);
    }
    zGeometry_WeilerClipOutputPartial* const outClip = self->outClip;
    int outputCount = outClip->pointList.pointCount;
    if (outputCount) {
        zVec3* point = outClip->pointList.points;
        do {
            point->x = translationX + point->x;
            point->y = translationY + point->y;
            ++point;
        } while (--outputCount);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-restoreoutputzfrominputplane
 * @recoil-artifact defines .text recoil:function:0x469b60: zGeometry_Weiler::RestoreOutputZFromInputPlane
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-match byte
 *
 * Purpose: Restore output point Z values from the input contour B plane.
 */
void __fastcall RestoreOutputZFromInputPlane(zGeometry_WeilerStatePartial* self)
{
    zVec3* const inputPoints = (zVec3*)(self->inputContourBBuffer.base);
    zVec3 edge01;
    zVec3 edge12;
    // Retail keeps the normal and offset contiguous ([ebp-0x34..-0x28]) as one plane record.
    struct {
        zVec3 normal;
        float offset;
    } plane;

    zMath::Vec3Subtract(&inputPoints[0], &inputPoints[1], &edge01);
    zMath::Vec3Subtract(&inputPoints[2], &inputPoints[1], &edge12);
    zMath::Vec3Cross(&edge01, &edge12, &plane.normal);

    if (plane.normal.z == 0.0f) {
        return;
    }

    plane.normal.x /= plane.normal.z;
    plane.normal.y /= plane.normal.z;

    plane.offset = -(plane.normal.x * inputPoints[0].x + plane.normal.y * inputPoints[0].y + inputPoints[0].z);

    zVec3* point = self->outClip->pointList.points;
    for (unsigned int i = 0; i < (unsigned int)(self->outClip->pointList.pointCount); ++i, ++point) {
        point->z = -(plane.normal.x * point->x + plane.normal.y * point->y + plane.offset);
    }
}

} // namespace zGeometry_Weiler

namespace zGeometry_Vec3 {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-isbetweenendpointsxy
 * @recoil-artifact defines .text recoil:function:0x469ca0: zGeometry_Vec3::IsBetweenEndpointsXY
 * @recoil-match byte
 *
 * Purpose: Test whether a point lies within the inclusive XY endpoint span of a segment.
 */
int __fastcall IsBetweenEndpointsXY(zVec3* testPoint, zVec3* startPoint, zVec3* endPoint)
{
    if (fabs((double)(startPoint->x) - (double)(endPoint->x)) < 0.0000099999997473787516) {
        if (startPoint->y >= endPoint->y) {
            return testPoint->y >= endPoint->y && testPoint->y <= startPoint->y;
        }

        return testPoint->y >= startPoint->y && testPoint->y <= endPoint->y;
    }

    if (startPoint->x >= endPoint->x) {
        return testPoint->x >= endPoint->x && testPoint->x <= startPoint->x;
    }

    return testPoint->x >= startPoint->x && testPoint->x <= endPoint->x;
}

} // namespace zGeometry_Vec3

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-selectforwardstartpointincontoura
 * @recoil-artifact defines .text recoil:function:0x469d60: zGeometry_Weiler::SelectForwardStartPointInContourA
 *
 *
 * Purpose: Choose the forward start point on contour A for outside-results bridge traversal.
 */
void __fastcall
SelectForwardStartPointInContourA(zVec3* point, zVec3** selectedPoint, zGeometry_WeilerStatePartial* self)
{
    int remainingPointCount = self->inputContourABuffer.count;
    zVec3* currentPoint = (zVec3*)(self->inputContourABuffer.base);
    zVec3* previousPoint = &currentPoint[remainingPointCount - 1];

    while (remainingPointCount-- != 0) {
        zVec3* const candidatePoint = *selectedPoint;
        const float edgeDeltaX = currentPoint->x - previousPoint->x;
        const float edgeDeltaY = currentPoint->y - previousPoint->y;
        const float pointCross
            = (point->x - previousPoint->x) * edgeDeltaY - (point->y - previousPoint->y) * edgeDeltaX;
        const float candidateCross
            = (candidatePoint->x - previousPoint->x) * edgeDeltaY - (candidatePoint->y - previousPoint->y) * edgeDeltaX;

        if ((pointCross > 0.0 && candidateCross < 0.0) || (pointCross < 0.0 && candidateCross > 0.0)) {
            if (currentPoint->x >= point->x) {
                previousPoint = currentPoint;
            }

            *selectedPoint = previousPoint;
            remainingPointCount = self->inputContourABuffer.count;
            currentPoint = (zVec3*)(self->inputContourABuffer.base);
            previousPoint = &currentPoint[remainingPointCount - 1];
        }
    }
}

} // namespace zGeometry_Weiler

namespace zGeometry_Vec3 {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-isnearequalxy
 * @recoil-artifact defines .text recoil:function:0x469e50: zGeometry_Vec3::IsNearEqualXY
 * @recoil-match byte
 *
 * Purpose: Compare two vectors in XY using the caller-supplied tolerance.
 */
int __fastcall IsNearEqualXY(zVec3* vecA, zVec3* vecB, float tolerance)
{

    float dx = vecA->x - vecB->x;
    float dy = vecA->y - vecB->y;
    dx = fabs(dx);
    dy = fabs(dy);
    if (dx > tolerance || dy > tolerance) {
        return 0;
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-snappointtosegmentxyifnear
 * @recoil-artifact defines .text recoil:function:0x469e90: zGeometry_Vec3::SnapPointToSegmentXYIfNear
 * @recoil-match byte
 *
 * Purpose: Snap a nearby point onto a segment in XY while preserving Z.
 */
int __fastcall SnapPointToSegmentXYIfNear(zVec3* lineStart, zVec3* lineEnd, zVec3* testPoint, float tolerance)
{
    const float dx = lineEnd->x - lineStart->x;
    const float dy = lineEnd->y - lineStart->y;
    const float testDx = testPoint->x - lineStart->x;
    const float testDy = testPoint->y - lineStart->y;

    if (fabs(dx) < tolerance) {
        if (fabs(testDx) <= tolerance) {
            const float t = testDy / dy;
            if (t > 0.0f && t < 1.0f) {
                testPoint->x = lineStart->x;
                return 1;
            }
        }
    } else if (fabs(dy) < tolerance) {
        if (fabs(testDy) <= tolerance) {
            const float t = testDx / dx;
            if (t > 0.0f && t < 1.0f) {
                testPoint->y = lineStart->y;
                return 1;
            }
        }
    } else {
        const float ty = testDy / dy;
        const float tx = testDx / dx;
        if (fabs(tx - ty) < tolerance && tx > 0.0f && ty > 0.0f && tx < 1.0f && ty < 1.0f) {
            zVec3 snapped;
            snapped.x = tx * dx + lineStart->x;
            if (fabs(snapped.x - testPoint->x) <= tolerance) {
                snapped.y = ty * dy + lineStart->y;
                if (fabs(snapped.y - testPoint->y) <= tolerance) {
                    testPoint->x = snapped.x;
                    testPoint->y = snapped.y;
                    return 1;
                }
            }
        }
    }

    return 0;
}

} // namespace zGeometry_Vec3

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-removeadjacentduplicatepointsxy
 * @recoil-artifact defines .text recoil:function:0x46a080: zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY
 * @recoil-match byte
 *
 * Purpose: Collapse adjacent duplicate XY vertices from a polygon point list.
 */
int __fastcall RemoveAdjacentDuplicatePointsXY(zVec3* vertices, int count)
{
    unsigned int index = 0;
    if (index < count) {
        unsigned int nextIndex = 1;
        zVec3* current = vertices;
        zVec3* next = vertices + 1;
        do {
            if (zGeometry_Vec3::IsNearEqualXY(current, &vertices[nextIndex % count], 0.00999999978f)) {
                const int lastIndex = count - 1;
                if (index != lastIndex) {
                    memcpy(current, next, (count - index - 1) * sizeof(zVec3));
                    --index;
                    --nextIndex;
                    --next;
                    --current;
                }
                count = lastIndex;
            }
            ++index;
            ++nextIndex;
            ++next;
            ++current;
        } while (index < count);
    }
    return count;
}

} // namespace zGeometry_Vec3Array

namespace zGeometry_Polygon {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-snappointsxyifnear
 * @recoil-artifact defines .text recoil:function:0x46a130: zGeometry_Polygon::SnapPointsXYIfNear
 * @recoil-match byte
 *
 * Purpose: Snap target polygon points to nearby source vertices or XY edges.
 */
int __fastcall SnapPointsXYIfNear(
    zVec3* polygon,
    int polyCount,
    zVec3* targetVerts,
    int targetCount,
    float vertexTolerance,
    float edgeTolerance
)
{
    int result = 0;

    for (int i = 0; i < polyCount; ++i) {
        for (int j = 0; j < targetCount; ++j) {
            if (zGeometry_Vec3::IsNearEqualXY(&polygon[i], &targetVerts[j], vertexTolerance)) {
                result = 1;
                targetVerts[j] = polygon[i];
            } else if (zGeometry_Vec3::SnapPointToSegmentXYIfNear(
                           &polygon[i],
                           &polygon[(i + 1) % polyCount],
                           &targetVerts[j],
                           edgeTolerance
                       )) {
                result = 1;
            }
        }
    }

    return result;
}

} // namespace zGeometry_Polygon

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-validatexings
 * @recoil-artifact defines .text recoil:function:0x46a1f0: zGeometry_Weiler::ValidateXings
 * @recoil-match byte
 *
 * Purpose: Walk the Weiler crossing array, report the first invalid crossing, and return the validation status.
 */
int __fastcall ValidateXings(int xingCount, zGeometry_WeilerXingPartial* xingArray, int* failedXingIndex)
{
    int isValid = 1;
    for (int xingIndex = 0; xingIndex < xingCount && isValid != 0; ++xingIndex, ++xingArray) {
        zGeometry_WeilerContourSegmentPartial* const segment0 = xingArray->segment0;
        zGeometry_WeilerContourSegmentPartial* const segment1 = xingArray->segment1;
        zGeometry_WeilerContourSegmentPartial* const segment2 = xingArray->segment2;
        zGeometry_WeilerContourSegmentPartial* const segment3 = xingArray->segment3;
        zGeometry_WeilerContourSegmentPartial* const segment4 = xingArray->segment4;
        zGeometry_WeilerContourSegmentPartial* const segment5 = xingArray->segment5;
        zGeometry_WeilerContourSegmentPartial* const segment6 = xingArray->segment6;
        zGeometry_WeilerContourSegmentPartial* const segment7 = xingArray->segment7;

        switch (xingArray->xingType) {
        case 3:
            break;
        case 4:
        case 5:
            if (segment0 == 0 || segment1 == 0 || segment2 == 0 || segment3 == 0 || segment4 == 0 || segment5 == 0
                || segment6 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        case 13:
            if (segment0 == 0 || segment2 == 0 || segment6 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        case 16:
            if (segment1 == 0 || segment3 == 0 || segment6 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        case 19:
            if (segment0 == 0 || segment2 == 0 || segment4 == 0 || segment5 == 0) {
                isValid = 0;
            }
            break;
        case 22:
            if (segment1 == 0 || segment3 == 0 || segment4 == 0 || segment5 == 0) {
                isValid = 0;
            }
            break;
        case 24:
            if (segment5 != 0) {
                if (segment0 == 0 || segment2 == 0) {
                    isValid = 0;
                }
            } else if (segment0 == 0 || segment2 == 0 || segment4 == 0 || segment6 == 0) {
                isValid = 0;
            }
            break;
        case 25:
            if (segment5 != 0) {
                if (segment1 == 0 || segment3 == 0) {
                    isValid = 0;
                }
            } else if (segment1 == 0 || segment3 == 0 || segment4 == 0 || segment6 == 0) {
                isValid = 0;
            }
            break;
        case 12:
            if (segment2 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        case 14:
            if (segment0 == 0 || segment6 == 0) {
                isValid = 0;
            }
            break;
        case 15:
            if (segment3 == 0 || segment6 == 0) {
                isValid = 0;
            }
            break;
        case 17:
            if (segment1 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        case 18:
            if (segment2 == 0 || segment4 == 0) {
                isValid = 0;
            }
            break;
        case 20:
            if (segment0 == 0 || segment5 == 0) {
                isValid = 0;
            }
            break;
        case 21:
            if (segment3 == 0 || segment5 == 0) {
                isValid = 0;
            }
            break;
        case 23:
            if (segment1 == 0 || segment4 == 0) {
                isValid = 0;
            }
            break;
        case 6:
            if (segment3 == 0 || segment4 == 0 || segment6 == 0) {
                isValid = 0;
            }
            break;
        case 7:
            if (segment2 == 0 || segment3 == 0 || segment5 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        case 8:
            if (segment0 == 0 || segment1 == 0 || segment4 == 0 || segment6 == 0) {
                isValid = 0;
            }
            break;
        case 9:
            if (segment0 == 0 || segment1 == 0 || segment5 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        case 10:
            if (segment1 != 0) {
                if (segment3 == 0 || segment4 == 0 || segment6 == 0) {
                    isValid = 0;
                }
            } else if (segment0 == 0 || segment2 == 0 || segment4 == 0 || segment6 == 0) {
                isValid = 0;
            }
            break;
        case 11:
            if (segment1 != 0) {
                if (segment3 == 0 || segment5 == 0 || segment7 == 0) {
                    isValid = 0;
                }
            } else if (segment0 == 0 || segment2 == 0 || segment5 == 0 || segment7 == 0) {
                isValid = 0;
            }
            break;
        }

        if (isValid == 0) {
            if (failedXingIndex != 0) {
                *failedXingIndex = xingIndex;
            }

            if (xingArray != 0) {
                zError::ReportOld(
                    0x100,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0x1788,
                    g_zGeometry_ValidateXingTypeFmt,
                    xingIndex,
                    xingArray->xingType
                );
            } else {
                zError::ReportOld(
                    0x100,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0x179a,
                    g_zGeometry_ValidateXingNullFmt,
                    xingIndex
                );
            }
        }
    }

    return isValid;
}

} // namespace zGeometry_Weiler

namespace zGeometry_Vec3Array {
} // namespace zGeometry_Vec3Array

namespace zGeometry_ClipPolygon {
} // namespace zGeometry_ClipPolygon

namespace zGeometry_Model {
} // namespace zGeometry_Model
