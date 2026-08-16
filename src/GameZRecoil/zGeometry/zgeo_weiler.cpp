
#include "zgeo.h"

#include "GameZRecoil/zError/zerr.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" const char g_zGeometry_WeilerIntersectBufferEntryFailedFmt[0x32] =
    "%s %d: weiler_intersect call to bufEntry failed.\n";

namespace {
/**
 * Data evidence: BN 0x4dfdd0 is int32_t[0x51], xrefed by 0x468fa0, and matches these case ids byte-for-byte.
 * Purpose: Map the four ternary edge-side sign classes to the Weiler intersection case id.
 */
const int kIntersect2dCaseIdBySignClass[0x51] = {0,
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
    0};

/**
 * Data evidence: BN 0x468f80 is uint8_t[0x18], xrefed by 0x468c40, and matches these output kinds byte-for-byte.
 * Purpose: Map each Weiler intersection case id to the output crossing construction branch.
 */
const unsigned char kIntersect2dOutputKindByXingType[0x18] =
    {0, 6, 6, 6, 1, 1, 2, 2, 3, 3, 6, 6, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5};

/**
 * Data evidence: BN 0x4dff14..0x4e0349 is the contiguous zgeo_weiler.cpp diagnostic
 * literal owner linked by geometry_model_assets.zgeometry_weiler_initialized_data.
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x27
 * @recoil-artifact defines .data recoil:data:0x4dff14: g_zGeometry_WeilerInitFailedMsg.
 * Purpose: Preserve the source-visible error/source literals used by Weiler diagnostics.
 */
const char g_zGeometry_WeilerInitFailedMsg[0x27] = "weiler_init call to weilerInit failed.";
const char g_zGeometry_SourceFile_ZgeoWeilerCpp[0x2e] =
    "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_weiler.cpp";
const char g_zGeometry_BadClipRegionForWeilerClipMsg[0x27] =
    "Bad clip region passed to Weiler Clip.";
const char g_zGeometry_WeilerGatherContoursFailedFmt[0x33] =
    "%s %d: weiler_clip call to gatherContours failed.\n";
const char g_zGeometry_WeilerBadParametersFmt[0x30] =
    "%s %d: Bad parameter(s) passed to Weiler Clip.\n";
const char g_zGeometry_WeilerInitNewContourFailedFmt[0x30] =
    "%s %d: weilerInit call to _new_contour failed.\n";
const char g_zGeometry_WeilerInitBufferEntryFailedFmt[0x2c] =
    "%s %d: weilerInit call to bufEntry failed.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x17
 * @recoil-artifact defines .data recoil:data:0x4e0054: g_zGeometry_ForwardSegmentFailedMsg.
 * Purpose: Preserve the weed-out diagnostic label for failed forward segment traversal.
 */
const char g_zGeometry_ForwardSegmentFailedMsg[0x17] = "Forward Segment Failed";
const char g_zGeometry_WeedOutCoincidentSegForwardFailedFmt[0x38] =
    "%s %d: _weed_out_coincident call to segForward failed.\n";
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
const char g_zGeometry_WeilerDivideEdgeFailedFmt[0x37] =
    "%s %d: _weiler_intersect call to _divide_edge failed.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x1a-0x4e013c
 * @recoil-artifact defines .data recoil:data:0x4e013c: g_zGeometry_WeilerIntersectErrorFmt.
 * Purpose: Preserve the old zError format used by Weiler intersection failures.
 */
const char g_zGeometry_WeilerIntersectErrorFmt[0x1a] = "weilerIntersect Error: %s";
const char g_zGeometry_NewContourBufferEntryFailedMsg[0x2a] =
    "New_contour could not obtain buffer entry";
const char g_zGeometry_MergeContoursNewContourFailedFmt[0x37] =
    "%s %d: _merge_contours failed to receive new contour.\n";
const char g_zGeometry_ContourMergeValidationFailedMsg[0x22] =
    "contourMerge:  Failed validation\n";
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
const char g_zGeometry_OutputContourBufferEntryFailedFmt[0x2f] =
    "%s %d: outputContour call to bufEntry failed.\n";
const char g_zGeometry_DivideEdgeBufferEntryFailedFmt[0x2e] =
    "%s %d: _divide_edge call to bufEntry failed.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-x10
 * @recoil-artifact defines .data recoil:data:0x4e0278: g_zGeometry_BufferEntryFailedMsg.
 * Purpose: Preserve the shared Weiler buffer-entry failure diagnostic label.
 */
const char g_zGeometry_BufferEntryFailedMsg[0x10] = "bufEntry failed";
const char g_zGeometry_GenerateOutsideResultsBufferEntryFailedFmt[0x35] =
    "%s %d: _gen._outside_rslts call to buf_entry failed\n";
const char g_zGeometry_Intersect2dBufferEntryFailedFmt[0x2e] =
    "%s %d: _intersect2d call to buf_entry failed\n";
const char g_zGeometry_ValidateXingNullFmt[0x2e] =
    "validateXing failed (xing %d) xing_p is NULL!";
const char g_zGeometry_ValidateXingTypeFmt[0x2a] =
    "validateXing failed (xing %d) (type = %d)";

struct WeilerPreclassifyContourPacket {
    zGeometry_WeilerContourOutputPartial contourA;
    zGeometry_WeilerContourOutputPartial contourB;
    zGeometry_WeilerContourOutputPartial contourC;
    zGeometry_WeilerContourOutputPartial contourD;
};

RECOIL_STATIC_ASSERT(sizeof(WeilerPreclassifyContourPacket) == 0x30);

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
 * Purpose: expose contour A's point buffer and count from an initialized
 * Weiler state.
 */
int __fastcall GetInputContourAPointList(
    zGeometry_WeilerStatePartial *self,
    zVec3 **outPoints
) {
    if (self == 0) {
        return 0;
    }

    *outPoints = (zVec3 *)(self->inputContourABuffer.base);
    return self->inputContourABuffer.count;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-init-0x464680
 * @recoil-artifact defines .text recoil:function:0x464680: zGeometry_Weiler::Init
 * Purpose: Allocate and initialize Weiler clip state from an input contour.
 */
zGeometry_WeilerStatePartial *__fastcall Init(
    zVec3 *points,
    int pointCount,
    int contourSource
) {
    if (pointCount == 0 || points == 0) {
        zError::ReportOld(
            0x200,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x20d,
            g_zGeometry_BadClipRegionForWeilerClipMsg
        );
    }

    const int dedupedPointCount =
        zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY(
            points,
            pointCount
        );

    zGeometry_WeilerStatePartial *const result =
        (zGeometry_WeilerStatePartial *)(calloc(
            1,
            sizeof(zGeometry_WeilerStatePartial)
        ));

    zGeometry_WeilerBuffer::Init(
        &result->segmentBuffer,
        0x80,
        sizeof(zGeometry_WeilerContourSegmentPartial)
    );
    zGeometry_WeilerBuffer::Init(
        &result->contourBuffer,
        0x80,
        0x0c
    );
    zGeometry_WeilerBuffer::Init(
        &result->xingBuffer,
        0x80,
        0x30
    );
    zGeometry_WeilerBuffer::Init(
        &result->inputContourABuffer,
        dedupedPointCount,
        sizeof(zVec3)
    );

    const size_t pointBytes = (size_t)(dedupedPointCount) * sizeof(zVec3);
    memcpy(
        result->inputContourABuffer.base,
        points,
        pointBytes
    );
    result->inputContourABuffer.count = dedupedPointCount;
    result->inputContourBBuffer.count = 0;
    result->inputContourBBuffer.base = 0;

    if (contourSource != 0) {
        zGeometry_Weiler::TogglePointAxesForContourSource(result);
    }

    result->contourSource = contourSource;
    zGeometry_Weiler::RecenterPointSetsIfOutOfRange(result);

    if (zGeometry_Weiler::InitInputContourPair(
        result,
        points,
        dedupedPointCount,
        1
    ) == 0) {
        zError::ReportOld(
            0x200,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x24c,
            g_zGeometry_WeilerInitFailedMsg
        );
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
 * Purpose: Replace the clip polygon's Weiler state from a point list while preserving the old contour source.
 */
int __fastcall ResetWeilerStateFromContourPoints(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zVec3 *points,
    int pointCount
) {
    if (points == 0 || pointCount == 0) {
        return 0;
    }

    zGeometry_WeilerStatePartial *const oldState = clipPolygon->weilerState;
    zGeometry_WeilerStatePartial *const newState =
        zGeometry_Weiler::Init(
            points,
            pointCount,
            oldState->contourSource
        );
    zGeometry_Weiler::DestroyState(oldState);
    clipPolygon->weilerState = newState;
    return 1;
}

} // namespace zGeometry_ClipPolygon

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-destroystate
 * @recoil-artifact defines .text recoil:function:0x4647d0: zGeometry_Weiler::DestroyState
 * Purpose: Release Weiler clip state buffers and state storage.
 */
void __fastcall DestroyState(
    zGeometry_WeilerStatePartial *self
) {
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
 * Purpose: Initialize clip output state, handle preclassified contour relationships, dispatch the Weiler clipping pipeline, and restore caller-visible output state.
 */
int __fastcall ClipPointList(
    zGeometry_WeilerStatePartial *self,
    int clipMode,
    zVec3 *points,
    int pointCount,
    zGeometry_WeilerClipOutputPartial *outClip
) {
    if (self == 0 || self->inputContourABuffer.count == 0 || pointCount == 0 || points == 0 ||
        outClip == 0) {
        fprintf(
            stderr,
            g_zGeometry_WeilerBadParametersFmt,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x2a0
        );
        return 0;
    }

    self->inputContourBBuffer.base = points;
    self->inputContourBBuffer.count = pointCount;
    self->clipMode = clipMode;

    if (self->contourSource != 0) {
        zGeometry_Weiler::TogglePointAxesForContourSource(self);
    }

    if (self->pointsRecentered) {
        zGeometry_Weiler::RecenterPointSetsIfOutOfRange(self);
    }

    zGeometry_WeilerBuffer::Init(
        &self->polygonSetABuffer,
        0x80,
        sizeof(zGeometry_PolygonPointSpanPartial)
    );
    zGeometry_WeilerBuffer::Init(
        &self->polygonSetBBuffer,
        0x80,
        sizeof(zGeometry_PolygonPointSpanPartial)
    );
    zGeometry_WeilerBuffer::Init(
        &self->polygonSetCBuffer,
        0x80,
        sizeof(zGeometry_PolygonPointSpanPartial)
    );
    zGeometry_WeilerBuffer::Init(
        &self->pointListBuffer,
        0x80,
        sizeof(zVec3)
    );

    self->outClip = outClip;
    outClip->polygonSetA.polygonCount = 0;
    outClip->polygonSetA.polygons =
        (zGeometry_PolygonPointSpanPartial *)(self->polygonSetABuffer.base);
    outClip->polygonSetB.polygonCount = 0;
    outClip->polygonSetB.polygons =
        (zGeometry_PolygonPointSpanPartial *)(self->polygonSetBBuffer.base);
    outClip->polygonSetC.polygonCount = 0;
    outClip->polygonSetC.polygons =
        (zGeometry_PolygonPointSpanPartial *)(self->polygonSetCBuffer.base);
    outClip->pointList.pointCount = 0;
    outClip->pointList.points = (zVec3 *)(self->pointListBuffer.base);

    const int preclassifiedMode = zGeometry_Weiler::ClassifyInputContourPairBounds(self);
    if (preclassifiedMode == 1) {
        return 1;
    }

    if (preclassifiedMode == 4) {
        if (zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(
            self,
            3
        ) != 0) {
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
    if (zGeometry_Weiler::InitInputContourPair(
            self,
            (zVec3 *)(self->inputContourBBuffer.base),
            pointCount,
            2
        ) == 0) {
        zGeometry_WeilerClipOutput::Destroy(outClip);
        if (self->pointsRecentered) {
            zGeometry_Weiler::RestorePointTranslation(self);
        }

        if (self->contourSource != 0) {
            zGeometry_Weiler::TogglePointAxesForContourSource(self);
        }

        return 0;
    }

    zGeometry_Weiler::BuildPointSideTablesForContourPair(self);
    if (zGeometry_Weiler::PreclassifyInputContourPair(self) == 0) {
        zGeometry_WeilerClipOutput::Destroy(outClip);
        if (self->pointsRecentered) {
            zGeometry_Weiler::RestorePointTranslation(self);
        }

        if (self->contourSource != 0) {
            zGeometry_Weiler::TogglePointAxesForContourSource(self);
        }

        return 0;
    }

    int clipResult = zGeometry_Weiler::ClassifyContainedContour(self);
    if (clipResult == 1) {
        zGeometry_WeilerClipOutput::Destroy(outClip);
        if (self->pointsRecentered) {
            zGeometry_Weiler::RestorePointTranslation(self);
        }

        if (self->contourSource != 0) {
            zGeometry_Weiler::TogglePointAxesForContourSource(self);
        }

        return 0;
    }

    if (clipResult != 0) {
        if (zGeometry_Weiler::MergeContours(self) == 0) {
            zGeometry_WeilerClipOutput::Destroy(outClip);
            if (self->pointsRecentered) {
                zGeometry_Weiler::RestorePointTranslation(self);
            }

            if (self->contourSource != 0) {
                zGeometry_Weiler::TogglePointAxesForContourSource(self);
            }

            return 0;
        }

        zGeometry_Weiler::NewContour(self);

        if (!self->allContoursSingleSided || preclassifiedMode == 2) {
            if (zGeometry_Weiler::OutputContoursForClipMode(self) == 0) {
                fprintf(
                    stderr,
                    g_zGeometry_WeilerGatherContoursFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp,
                    0x3b2
                );
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

    int outputMode = 1;
    if (preclassifiedMode == 2) {
        if (zGeometry_Weiler::GenerateOutsideResults(self) == 0 ||
            zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(
                self,
                4
            ) == 0) {
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
        if (zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(self, preclassifiedMode) ==
            0) {
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
 * Purpose: Free and clear the point list and three polygon-set buffers owned by a clip output.
 */
void __fastcall Destroy(
    zGeometry_WeilerClipOutputPartial *self
) {
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
 * Purpose: Allocate forward and reverse contour segment rings for an input contour.
 */
int __fastcall InitInputContourPair(
    zGeometry_WeilerStatePartial *self,
    zVec3 *points,
    int pointCount,
    int contourType
) {
    zGeometry_WeilerContourSegmentPartial *segments = (zGeometry_WeilerContourSegmentPartial
            *)(zGeometry_WeilerBuffer::GetAppendSpace(
                &self->segmentBuffer,
                pointCount * 2,
                0
            ));
    if (segments == 0) {
        fprintf(
            stderr,
            g_zGeometry_WeilerInitBufferEntryFailedFmt,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x455
        );
        return 0;
    }

    zGeometry_WeilerContourSegmentArray::InitFromPointList(
        segments,
        points,
        pointCount,
        contourType
    );
    segments->contourOutput = 0;
    if (zGeometry_Weiler::EnsureContourOutput(
        self,
        segments
    ) == 0) {
        fprintf(
            stderr,
            g_zGeometry_WeilerInitNewContourFailedFmt,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x468
        );
        return 0;
    }

    zGeometry_WeilerContourSegmentArray::UpdateBounds(
        segments,
        pointCount
    );

    zGeometry_WeilerContourSegmentPartial *const reverseSegments = &segments[pointCount];
    zGeometry_WeilerContourSegmentArray::InitFromPointList(
        reverseSegments,
        points,
        pointCount,
        4
    );
    reverseSegments->contourOutput = 0;
    if (zGeometry_Weiler::EnsureContourOutput(
        self,
        reverseSegments
    ) == 0) {
        fprintf(
            stderr,
            g_zGeometry_WeilerInitNewContourFailedFmt,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0x485
        );
        return 0;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifyinputcontourpairbounds
 * @recoil-artifact defines .text recoil:function:0x464c90: zGeometry_Weiler::ClassifyInputContourPairBounds
 * Purpose: Preclassify the two input contours by comparing their XY bounding boxes.
 */
int __fastcall ClassifyInputContourPairBounds(
    zGeometry_WeilerStatePartial *self
) {
    const int inputPointCountA = self->inputContourABuffer.count;
    const int inputPointCountB = self->inputContourBBuffer.count;
    zVec3 *const inputPointsA = (zVec3 *)(self->inputContourABuffer.base);
    zVec3 *const inputPointsB = (zVec3 *)(self->inputContourBBuffer.base);

    WeilerPointBoundsXY boundsA;
    WeilerPointBoundsXY boundsB;
    boundsB.minX = inputPointsB[0].x;
    boundsB.maxX = inputPointsB[0].x;
    boundsB.minY = inputPointsB[0].y;
    boundsB.maxY = inputPointsB[0].y;

    for (int inputPointIndexB = 1; inputPointIndexB < inputPointCountB;
         ++inputPointIndexB) {
        zVec3 *const inputPointB = &inputPointsB[inputPointIndexB];
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

    for (int inputPointIndexA = 1; inputPointIndexA < inputPointCountA;
         ++inputPointIndexA) {
        zVec3 *const inputPointA = &inputPointsA[inputPointIndexA];
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

    if (boundsB.minX >= boundsA.maxX || boundsB.maxX <= boundsA.minX ||
        boundsB.minY >= boundsA.maxY || boundsB.maxY <= boundsA.minY) {
        return 1;
    }

    if (boundsB.minX <= boundsA.minX && boundsB.maxX >= boundsA.maxX &&
        boundsB.minY <= boundsA.minY && boundsB.maxY >= boundsA.maxY) {
        return zGeometry_Weiler::OutputPreclassifiedContourPairResult(
            inputPointCountA,
            inputPointsA,
            inputPointCountB,
            inputPointsB,
            2
        );
    }

    if (boundsB.minX >= boundsA.minX && boundsB.maxX <= boundsA.maxX &&
        boundsB.minY >= boundsA.minY && boundsB.maxY <= boundsA.maxY) {
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
 * Purpose: Resolve a preclassified containment result by rejecting unmatched outside points.
 */
int __fastcall OutputPreclassifiedContourPairResult(
    int contourAPointCount,
    zVec3 *contourAPoints,
    int contourBPointCount,
    zVec3 *contourBPoints,
    int resultCode
) {
    if (resultCode == 2 && contourAPointCount == contourBPointCount) {
        bool allPointsMatched = true;
        zVec3 *contourBPoint = contourBPoints;

        for (int i = contourBPointCount; i != 0; --i) {
            bool pointMatched = false;
            zVec3 *contourAPoint = contourAPoints;

            for (int j = contourAPointCount; j != 0; --j) {
                if (fabs((double)(contourAPoint->x) - (double)(contourBPoint->x)) <=
                        0.0010000000474974513 &&
                    fabs((double)(contourAPoint->y) - (double)(contourBPoint->y)) <=
                        0.0010000000474974513) {
                    pointMatched = true;
                    break;
                }

                ++contourAPoint;
            }

            if (!pointMatched) {
                allPointsMatched = false;
                break;
            }

            ++contourBPoint;
        }

        if (allPointsMatched) {
            return 4;
        }
    }

    zVec3 *contourAPoint = contourAPoints;
    for (int i = contourAPointCount; i != 0; --i) {
        if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                contourAPoint,
                contourBPointCount,
                contourBPoints
            ) < 0) {
            return 0;
        }

        ++contourAPoint;
    }

    return resultCode;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-preclassifyinputcontourpair
 * @recoil-artifact defines .text recoil:function:0x464f70: zGeometry_Weiler::PreclassifyInputContourPair
 * Purpose: Preclassify overlapping input contours by splitting coincident segments and merging contour type flags.
 */
int __fastcall PreclassifyInputContourPair(
    zGeometry_WeilerStatePartial *self
) {
    WeilerPreclassifyContourPacket *const contourPacket =
        (WeilerPreclassifyContourPacket *)(self->contourBuffer.base);

    zGeometry_WeilerContourSegmentPartial *contourA = contourPacket->contourA.firstSegment;
    zGeometry_WeilerContourSegmentPartial *contourB = contourPacket->contourB.firstSegment;
    zGeometry_WeilerContourSegmentPartial *const contourC = contourPacket->contourC.firstSegment;
    zGeometry_WeilerContourSegmentPartial *const contourD = contourPacket->contourD.firstSegment;

    float *orientationTableB = self->contourBPointSideByContourAEdge;
    const int contourAPointCount = self->inputContourABuffer.count;
    const int contourBPointCount = self->inputContourBBuffer.count;
    FILE *const coincidentSegmentErrorOutput = stderr;

    if (contourAPointCount <= 0) {
        return 1;
    }

    {
        for (int contourAIndex = 0; contourAIndex < contourAPointCount; ++contourAIndex) {
            float *orientationTableA = &self->contourAPointSideByContourBEdge[contourAIndex];
            zVec3 *contourAStart = contourA->startPoint;
            zVec3 *contourAEnd = contourA->endPoint;

            zGeometry_WeilerContourSegmentPartial *contourCWalker = contourC;
            zGeometry_WeilerContourSegmentPartial *contourDWalker = contourD;

            {
                for (int contourBIndex = 0; contourBIndex < contourBPointCount; ++contourBIndex) {
                    zVec3 *const contourCStart = contourCWalker->startPoint;
                    zVec3 *const contourCEnd = contourCWalker->endPoint;

                    if (fabs((double)(orientationTableA[0])) <
                            0.0000099999997473787516 &&
                        fabs((double)(orientationTableA[1])) <
                            0.0000099999997473787516 &&
                        fabs((double)(orientationTableB[0])) <
                            0.0000099999997473787516 &&
                        fabs((double)(orientationTableB[1])) <
                            0.0000099999997473787516) {
                        const int overlapCase = (((
                                                      (zGeometry_Vec3::IsBetweenEndpointsXY(
                                                           contourAStart,
                                                           contourCStart,
                                                           contourCEnd
                                                       ) * 2) |
                                                      zGeometry_Vec3::IsBetweenEndpointsXY(
                                                          contourAEnd,
                                                          contourCStart,
                                                          contourCEnd
                                                      )
                                                  ) << 1) |
                                                    zGeometry_Vec3::IsBetweenEndpointsXY(
                                                        contourCStart,
                                                        contourAStart,
                                                        contourAEnd
                                                    ))
                                                    << 1 |
                                                zGeometry_Vec3::IsBetweenEndpointsXY(
                                                    contourCEnd,
                                                    contourAStart,
                                                    contourAEnd
                                                );
                        switch (overlapCase - 3) {
                        case 0:
                            if (contourCEnd->x != contourCStart->x &&
                                    contourAEnd->x != contourAStart->x
                                    ? (contourCEnd->x < contourCStart->x) ==
                                          (contourAEnd->x < contourAStart->x)
                                    : (contourCEnd->y < contourCStart->y) ==
                                          (contourAEnd->y < contourAStart->y)) {
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourA,
                                        contourB,
                                        contourCEnd,
                                        0,
                                        0
                                    ) == 0) {
                                    zError::ReportOld(
                                        0x100,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x568,
                                        g_zGeometry_WeedOutErrorFmt,
                                        g_zGeometry_WeilerCase_BCompletelyInsideA
                                    );
                                    return 0;
                                }

                                contourAEnd = contourCStart;
                                contourB->endPoint = contourCStart;
                                contourA->endPoint = contourCStart;
                                contourCWalker->contourType |= contourA->contourType;
                                contourDWalker->contourType |= contourB->contourType;
                            } else {
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourA,
                                        contourB,
                                        contourCStart,
                                        0,
                                        0
                                    ) == 0) {
                                    zError::ReportOld(
                                        0x100,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x572,
                                        g_zGeometry_WeedOutErrorFmt,
                                        g_zGeometry_WeilerCase_BCompletelyInsideA
                                    );
                                    return 0;
                                }

                                contourAEnd = contourCEnd;
                                contourB->endPoint = contourCEnd;
                                contourA->endPoint = contourCEnd;
                                contourCWalker->contourType |= contourB->contourType;
                                contourDWalker->contourType |= contourA->contourType;
                            }

                            zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                            break;

                        case 2:
                            if (!(fabs((double)(contourAEnd->x) -
                                       (double)(contourCEnd->x)) <=
                                      0.0010000000474974513 &&
                                  fabs((double)(contourAEnd->y) -
                                       (double)(contourCEnd->y)) <=
                                      0.0010000000474974513)) {
                                zVec3 *const oldContourAEnd = contourAEnd;
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourA,
                                        contourB,
                                        contourCEnd,
                                        contourDWalker->contourType,
                                        contourCWalker->contourType
                                    ) == 0) {
                                    fprintf(
                                        coincidentSegmentErrorOutput,
                                        g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x593
                                    );
                                    return 0;
                                }

                                contourAEnd = contourCEnd;
                                contourB->endPoint = contourCEnd;
                                contourA->endPoint = contourCEnd;
                                contourDWalker->endPoint = oldContourAEnd;
                                contourCWalker->endPoint = oldContourAEnd;
                                zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                                zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                            }
                            break;

                        case 3:

                            if (!(fabs((double)(contourAEnd->x) -
                                       (double)(contourCStart->x)) <=
                                      0.0010000000474974513 &&
                                  fabs((double)(contourAEnd->y) -
                                       (double)(contourCStart->y)) <=
                                      0.0010000000474974513)) {
                                zVec3 *const oldContourAEnd = contourAEnd;
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourCWalker,
                                        contourDWalker,
                                        contourAEnd,
                                        0,
                                        0
                                    ) == 0) {
                                    fprintf(
                                        stderr,
                                        g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x5b6
                                    );
                                    return 0;
                                }

                                contourDWalker->endPoint = oldContourAEnd;
                                contourCWalker->endPoint = oldContourAEnd;
                                contourAEnd = contourCStart;
                                contourB->endPoint = contourCStart;
                                contourA->endPoint = contourCStart;
                                contourCWalker->contourType |= contourA->contourType;
                                contourDWalker->contourType |= contourB->contourType;
                                zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                                zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                            }
                            break;

                        case 4:
                            if (contourCEnd->x != contourCStart->x &&
                                    contourAEnd->x != contourAStart->x
                                    ? (contourCEnd->x < contourCStart->x) ==
                                          (contourAEnd->x < contourAStart->x)
                                    : (contourCEnd->y < contourCStart->y) ==
                                          (contourAEnd->y < contourAStart->y)) {
                                contourAEnd = contourCEnd;
                                contourB->endPoint = contourCEnd;
                                contourA->endPoint = contourCEnd;
                                contourCWalker->contourType |= contourB->contourType;
                                contourDWalker->contourType |= contourA->contourType;
                            } else {
                                contourAEnd = contourCStart;
                                contourB->endPoint = contourCStart;
                                contourA->endPoint = contourCStart;
                                contourCWalker->contourType |= contourA->contourType;
                                contourDWalker->contourType |= contourB->contourType;
                            }

                            zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                            break;

                        case 6:
                            if (!(fabs((double)(contourAStart->x) -
                                       (double)(contourCEnd->x)) <=
                                      0.0010000000474974513 &&
                                  fabs((double)(contourAStart->y) -
                                       (double)(contourCEnd->y)) <=
                                      0.0010000000474974513)) {
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourA,
                                        contourB,
                                        contourCEnd,
                                        0,
                                        0
                                    ) == 0) {
                                    fprintf(
                                        stderr,
                                        g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x5f0
                                    );
                                    return 0;
                                }

                                contourAEnd = contourCEnd;
                                contourB->endPoint = contourCEnd;
                                contourA->endPoint = contourCEnd;
                                contourDWalker->endPoint = contourAStart;
                                contourCWalker->endPoint = contourAStart;
                                contourA->contourType |= contourCWalker->contourType;
                                contourB->contourType |= contourDWalker->contourType;
                                zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                                zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                            }
                            break;

                        case 7:
                            if (!(fabs((double)(contourAStart->x) -
                                       (double)(contourCStart->x)) <=
                                      0.0010000000474974513 &&
                                  fabs((double)(contourAStart->y) -
                                       (double)(contourCStart->y)) <=
                                      0.0010000000474974513)) {
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourA,
                                        contourB,
                                        contourCStart,
                                        0,
                                        0
                                    ) == 0) {
                                    fprintf(
                                        stderr,
                                        g_zGeometry_WeedOutCoincidentSegForwardFailedFmt,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x610
                                    );
                                    return 0;
                                }

                                contourB->endPoint = contourCStart;
                                contourA->endPoint = contourCStart;
                                contourDWalker->startPoint = contourAStart;
                                contourCWalker->startPoint = contourAStart;
                                contourAStart = contourCStart;
                                contourA->contourType |= contourDWalker->contourType;
                                contourB->contourType |= contourCWalker->contourType;
                                zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                                zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                            }
                            break;

                        case 8:
                            if (contourCEnd->x != contourCStart->x &&
                                    contourAEnd->x != contourAStart->x
                                    ? (contourCEnd->x < contourCStart->x) ==
                                          (contourAEnd->x < contourAStart->x)
                                    : (contourCEnd->y < contourCStart->y) ==
                                          (contourAEnd->y < contourAStart->y)) {
                                contourAStart = contourCEnd;
                                contourB->startPoint = contourCEnd;
                                contourA->startPoint = contourCEnd;
                                contourCWalker->contourType |= contourA->contourType;
                                contourDWalker->contourType |= contourB->contourType;
                            } else {
                                contourAStart = contourCStart;
                                contourB->startPoint = contourCStart;
                                contourA->startPoint = contourCStart;
                                contourCWalker->contourType |= contourB->contourType;
                                contourDWalker->contourType |= contourA->contourType;
                            }

                            zGeometry_WeilerContourSegment::UpdateBounds(contourA);
                            break;

                        case 9:
                            if (contourCEnd->x != contourCStart->x &&
                                    contourAEnd->x != contourAStart->x
                                    ? (contourCEnd->x < contourCStart->x) ==
                                          (contourAEnd->x < contourAStart->x)
                                    : (contourCEnd->y < contourCStart->y) ==
                                          (contourAEnd->y < contourAStart->y)) {
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourCWalker,
                                        contourDWalker,
                                        contourAEnd,
                                        0,
                                        0
                                    ) == 0) {
                                    zError::ReportOld(
                                        0x100,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x64a,
                                        g_zGeometry_WeedOutErrorFmt,
                                        g_zGeometry_ForwardSegmentFailedMsg
                                    );
                                    return 0;
                                }

                                contourDWalker->endPoint = contourAStart;
                                contourCWalker->endPoint = contourAStart;
                            } else {
                                if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                        self,
                                        contourCWalker,
                                        contourDWalker,
                                        contourAStart,
                                        0,
                                        0
                                    ) == 0) {
                                    zError::ReportOld(
                                        0x100,
                                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                        0x650,
                                        g_zGeometry_WeedOutErrorFmt,
                                        g_zGeometry_ForwardSegmentFailedMsg
                                    );
                                    return 0;
                                }

                                contourDWalker->endPoint = contourAEnd;
                                contourCWalker->endPoint = contourAEnd;
                            }

                            zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                            contourA->contourType |= contourCWalker->contourType;
                            contourB->contourType |= contourDWalker->contourType;
                            break;

                        case 10:
                            if (contourCEnd->x != contourCStart->x &&
                                    contourAEnd->x != contourAStart->x
                                    ? (contourCEnd->x < contourCStart->x) ==
                                          (contourAEnd->x < contourAStart->x)
                                    : (contourCEnd->y < contourCStart->y) ==
                                          (contourAEnd->y < contourAStart->y)) {
                                contourDWalker->endPoint = contourAStart;
                                contourCWalker->endPoint = contourAStart;
                                contourA->contourType |= contourCWalker->contourType;
                                contourB->contourType |= contourDWalker->contourType;
                            } else {
                                contourDWalker->endPoint = contourAEnd;
                                contourCWalker->endPoint = contourAEnd;
                                contourA->contourType |= contourDWalker->contourType;
                                contourB->contourType |= contourCWalker->contourType;
                            }

                            zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                            break;

                        case 11:
                            if (contourCEnd->x != contourCStart->x &&
                                    contourAEnd->x != contourAStart->x
                                    ? (contourCEnd->x < contourCStart->x) ==
                                          (contourAEnd->x < contourAStart->x)
                                    : (contourCEnd->y < contourCStart->y) ==
                                          (contourAEnd->y < contourAStart->y)) {
                                contourDWalker->startPoint = contourAEnd;
                                contourCWalker->startPoint = contourAEnd;
                                contourA->contourType |= contourCWalker->contourType;
                                contourB->contourType |= contourDWalker->contourType;
                            } else {
                                contourDWalker->startPoint = contourAStart;
                                contourCWalker->startPoint = contourAStart;
                                contourA->contourType |= contourDWalker->contourType;
                                contourB->contourType |= contourCWalker->contourType;
                            }

                            zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                            break;

                        case 12:
                            if (contourCEnd->x != contourCStart->x &&
                                    contourAEnd->x != contourAStart->x
                                    ? (contourCEnd->x < contourCStart->x) ==
                                          (contourAEnd->x < contourAStart->x)
                                    : (contourCEnd->y < contourCStart->y) ==
                                          (contourAEnd->y < contourAStart->y)) {
                                contourA->contourType |= contourCWalker->contourType;
                                contourB->contourType |= contourDWalker->contourType;
                            } else {
                                contourB->contourType |= contourCWalker->contourType;
                                contourA->contourType |= contourDWalker->contourType;
                            }

                            if (contourCWalker == contourC) {
                                contourPacket->contourC.firstSegment = contourCWalker->next;
                                contourPacket->contourD.firstSegment = contourDWalker->next;
                                contourCWalker->next->contourOutput = &contourPacket->contourC;
                                contourDWalker->next->contourOutput = &contourPacket->contourD;
                            }

                            contourCWalker->prev->next = contourCWalker->next;
                            contourCWalker->next->prev = contourCWalker->prev;
                            contourDWalker->prev->next = contourDWalker->next;
                            contourDWalker->next->prev = contourDWalker->prev;
                            break;

                        default:
                            break;
                        }
                    }

                    contourCWalker = contourCWalker + 1;
                    contourDWalker = contourDWalker + 1;
                    orientationTableA += contourAPointCount + 1;
                    ++orientationTableB;
                }
            }

            ++orientationTableB;
            contourA = contourA + 1;
            contourB = contourB + 1;
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifycontainedcontour
 * @recoil-artifact defines .text recoil:function:0x465ac0: zGeometry_Weiler::ClassifyContainedContour
 * Purpose: Classify contained contour pairs by intersecting segment rings, splitting at crossings, and repairing crossing back-references.
 */
int __fastcall ClassifyContainedContour(
    zGeometry_WeilerStatePartial *self
) {
    WeilerPreclassifyContourPacket *const contourPacket =
        (WeilerPreclassifyContourPacket *)(self->contourBuffer.base);

    zGeometry_WeilerContourSegmentPartial *const contourOutput0Start =
        contourPacket->contourA.firstSegment;
    zGeometry_WeilerContourSegmentPartial *const contourOutput1Start =
        contourPacket->contourB.firstSegment;
    zGeometry_WeilerContourSegmentPartial *const contourOutput2Start =
        contourPacket->contourC.firstSegment;
    zGeometry_WeilerContourSegmentPartial *const contourOutput3Start =
        contourPacket->contourD.firstSegment;

    int aggregateIntersectResult = 0;

    zGeometry_WeilerContourSegmentPartial *contourOutput0Segment = contourOutput0Start;
    zGeometry_WeilerContourSegmentPartial *contourOutput1Segment = contourOutput1Start;

    do {
        zGeometry_WeilerContourSegmentPartial *contourOutput2Segment = contourOutput2Start;
        zGeometry_WeilerContourSegmentPartial *contourOutput3Segment = contourOutput3Start;

        do {
            if (contourOutput0Segment->boundsDirty != 0) {
                zGeometry_WeilerContourSegment::UpdateBounds(contourOutput0Segment);
            }

            if (contourOutput2Segment->boundsDirty != 0) {
                zGeometry_WeilerContourSegment::UpdateBounds(contourOutput2Segment);
            }

            if (contourOutput0Segment->minX <= contourOutput2Segment->maxX &&
                contourOutput0Segment->maxX >= contourOutput2Segment->minX &&
                contourOutput0Segment->minY <= contourOutput2Segment->maxY &&
                contourOutput0Segment->maxY >= contourOutput2Segment->minY &&
                !((contourOutput2Segment->startXing != 0 &&
                       (contourOutput2Segment->startXing == contourOutput0Segment->startXing ||
                           contourOutput2Segment->startXing == contourOutput0Segment->endXing ||
                           contourOutput2Segment->startXing == contourOutput1Segment->startXing ||
                           contourOutput2Segment->startXing == contourOutput1Segment->endXing)) ||
                    (contourOutput2Segment->endXing != 0 &&
                        (contourOutput2Segment->endXing == contourOutput0Segment->startXing ||
                            contourOutput2Segment->endXing == contourOutput0Segment->endXing ||
                            contourOutput2Segment->endXing == contourOutput1Segment->startXing ||
                            contourOutput2Segment->endXing == contourOutput1Segment->endXing)) ||
                    (contourOutput3Segment->startXing != 0 &&
                        (contourOutput3Segment->startXing == contourOutput0Segment->startXing ||
                            contourOutput3Segment->startXing == contourOutput0Segment->endXing ||
                            contourOutput3Segment->startXing == contourOutput1Segment->startXing ||
                            contourOutput3Segment->startXing == contourOutput1Segment->endXing)) ||
                    (contourOutput3Segment->endXing != 0 &&
                        (contourOutput3Segment->endXing == contourOutput0Segment->startXing ||
                            contourOutput3Segment->endXing == contourOutput0Segment->endXing ||
                            contourOutput3Segment->endXing == contourOutput1Segment->startXing ||
                            contourOutput3Segment->endXing == contourOutput1Segment->endXing)))) {
                zGeometry_WeilerXingPartial *intersectXing = 0;
                const int intersectResult = zGeometry_Weiler::Intersect2d(
                    self,
                    &intersectXing,
                    *contourOutput0Segment->startPoint,
                    *contourOutput0Segment->endPoint,
                    *contourOutput2Segment->startPoint,
                    *contourOutput2Segment->endPoint
                );

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
                    aggregateIntersectResult |= intersectResult;
                }

                if (intersectXing != 0) {
                    if (intersectResult == 3) {
                        const int overlapCase = (((
                                                      (zGeometry_Vec3::IsBetweenEndpointsXY(
                                                           contourOutput0Segment->startPoint,
                                                           contourOutput2Segment->startPoint,
                                                           contourOutput2Segment->endPoint
                                                       ) * 2) |
                                                      zGeometry_Vec3::IsBetweenEndpointsXY(
                                                          contourOutput0Segment->endPoint,
                                                          contourOutput2Segment->startPoint,
                                                          contourOutput2Segment->endPoint
                                                      )
                                                  ) << 1) |
                                                    zGeometry_Vec3::IsBetweenEndpointsXY(
                                                        contourOutput2Segment->startPoint,
                                                        contourOutput0Segment->startPoint,
                                                        contourOutput0Segment->endPoint
                                                    ))
                                                    << 1 |
                                                zGeometry_Vec3::IsBetweenEndpointsXY(
                                                    contourOutput2Segment->endPoint,
                                                    contourOutput0Segment->startPoint,
                                                    contourOutput0Segment->endPoint
                                                );

                        switch (overlapCase - 5) {
                        case 0:
                            intersectXing = (zGeometry_WeilerXingPartial
                                    *)(zGeometry_WeilerBuffer::GetAppendSpace(
                                        &self->xingBuffer,
                                        1,
                                        0
                                    ));
                            if (intersectXing != 0) {
                                intersectXing->xingType = 0x17;
                            }
                            break;

                        case 1:
                            intersectXing = (zGeometry_WeilerXingPartial
                                    *)(zGeometry_WeilerBuffer::GetAppendSpace(
                                        &self->xingBuffer,
                                        1,
                                        0
                                    ));
                            if (intersectXing != 0) {
                                intersectXing->xingType = 0x11;
                            }
                            break;

                        case 4:
                            intersectXing = (zGeometry_WeilerXingPartial
                                    *)(zGeometry_WeilerBuffer::GetAppendSpace(
                                        &self->xingBuffer,
                                        1,
                                        0
                                    ));
                            if (intersectXing != 0) {
                                intersectXing->xingType = 0x15;
                            }
                            break;

                        case 5:
                            intersectXing = (zGeometry_WeilerXingPartial
                                    *)(zGeometry_WeilerBuffer::GetAppendSpace(
                                        &self->xingBuffer,
                                        1,
                                        0
                                    ));
                            if (intersectXing != 0) {
                                intersectXing->xingType = 0xf;
                            }
                            break;

                        default:
                            break;
                        }

                        if (intersectXing == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_Intersect2dBufferEntryFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x78a
                            );
                            return 1;
                        }
                    }

                    intersectXing->segment6 = 0;
                    intersectXing->segment7 = 0;
                    intersectXing->segment4 = 0;
                    intersectXing->segment5 = 0;
                    intersectXing->segment2 = 0;
                    intersectXing->segment3 = 0;
                    intersectXing->segment0 = 0;
                    intersectXing->segment1 = 0;

                    if (intersectResult == 4 || intersectResult == 5) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput0Segment,
                                1
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput1Segment,
                                1
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput2Segment,
                                1
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput3Segment,
                                1
                            ) == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x7ea
                            );
                            return 1;
                        }
                    }

                    if (intersectResult == 0xd) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput0Segment,
                                1
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput1Segment,
                                0
                            ) == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x7fc
                            );
                            return 1;
                        }

                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput2Segment->prev,
                                contourOutput2Segment,
                                contourOutput1Segment
                            );
                        if (edgeClass == 0) {
                            intersectXing->xingType = 0xd;
                        }
                    }

                    if (intersectResult == 0x10) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput0Segment,
                                0
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput1Segment,
                                1
                            ) == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x829
                            );
                            return 1;
                        }

                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput2Segment->prev,
                                contourOutput2Segment,
                                contourOutput1Segment
                            );
                        if (edgeClass == 0) {
                            intersectXing->xingType = 0x10;
                        }
                    }

                    if (intersectResult == 0x13) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput0Segment,
                                1
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput1Segment,
                                0
                            ) == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x85e
                            );
                            return 1;
                        }

                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput2Segment,
                                contourOutput2Segment->next,
                                contourOutput0Segment
                            );
                        if (edgeClass == 0) {
                            intersectXing->xingType = 0x13;
                        }
                    }

                    if (intersectResult == 0x16) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput0Segment,
                                0
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput1Segment,
                                1
                            ) == 0) {
                            fprintf(
                                stderr,
                                g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                                0x893
                            );
                            return 1;
                        }

                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput2Segment,
                                contourOutput2Segment->next,
                                contourOutput1Segment
                            );
                        if (edgeClass == 0) {
                            intersectXing->xingType = 0x16;
                        }
                    }

                    if (intersectResult == 6) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput2Segment,
                                1
                            ) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(
                                self,
                                &intersectXing->point,
                                contourOutput3Segment,
                                0
                            ) == 0) {
                            fprintf(stderr, g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x8c9);
                            return 1;
                        }
                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput0Segment->prev,
                                contourOutput0Segment,
                                contourOutput2Segment
                            );
                        if (edgeClass == 0) intersectXing->xingType = 6;
                    }

                    if (intersectResult == 7) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self,
                                &intersectXing->point, contourOutput2Segment, 0) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(self,
                                &intersectXing->point, contourOutput3Segment, 1) == 0) {
                            fprintf(stderr, g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x904);
                            return 1;
                        }
                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput0Segment->prev, contourOutput0Segment,
                                contourOutput3Segment
                            );
                        if (edgeClass == 0) intersectXing->xingType = 7;
                    }

                    if (intersectResult == 8) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self,
                                &intersectXing->point, contourOutput2Segment, 1) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(self,
                                &intersectXing->point, contourOutput3Segment, 0) == 0) {
                            fprintf(stderr, g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x939);
                            return 1;
                        }
                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput0Segment,
                                contourOutput0Segment->next,
                                contourOutput2Segment
                            );
                        if (edgeClass == 0) intersectXing->xingType = 8;
                    }

                    if (intersectResult == 9) {
                        if (zGeometry_Weiler::DivideContourSegmentAtPoint(self,
                                &intersectXing->point, contourOutput2Segment, 0) == 0 ||
                            zGeometry_Weiler::DivideContourSegmentAtPoint(self,
                                &intersectXing->point, contourOutput3Segment, 1) == 0) {
                            fprintf(stderr, g_zGeometry_WeilerDivideEdgeFailedFmt,
                                g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x974);
                            return 1;
                        }
                        const int edgeClass =
                            zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
                                contourOutput0Segment, contourOutput0Segment->next,
                                contourOutput3Segment
                            );
                        if (edgeClass == 0) intersectXing->xingType = 9;
                    }

                    if (intersectResult == 0xc) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment->prev, contourOutput2Segment,
                            contourOutput0Segment->prev, contourOutput0Segment, self);
                        if (edgeClass == 0) intersectXing->xingType = 0xc;
                    }
                    if (intersectResult == 0xf) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment->prev, contourOutput2Segment,
                            contourOutput1Segment->prev, contourOutput1Segment, self);
                        if (edgeClass == 0) intersectXing->xingType = 0xf;
                    }
                    if (intersectResult == 0xe) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment->prev, contourOutput2Segment,
                            contourOutput1Segment, contourOutput1Segment->next, self);
                        if (edgeClass == 0) intersectXing->xingType = 0xe;
                    }
                    if (intersectResult == 0x11) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment->prev, contourOutput2Segment,
                            contourOutput1Segment, contourOutput1Segment->next, self);
                        if (edgeClass == 0) intersectXing->xingType = 0x11;
                    }
                    if (intersectResult == 0x12) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment, contourOutput2Segment->next,
                            contourOutput1Segment->prev, contourOutput1Segment, self);
                        if (edgeClass == 0) intersectXing->xingType = 0x12;
                    }
                    if (intersectResult == 0x15) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment, contourOutput2Segment->next,
                            contourOutput1Segment->prev, contourOutput1Segment, self);
                        if (edgeClass == 0) intersectXing->xingType = 0x15;
                    }
                    if (intersectResult == 0x14) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment, contourOutput2Segment->next,
                            contourOutput1Segment, contourOutput1Segment->next, self);
                        if (edgeClass == 0) intersectXing->xingType = 0x14;
                    }
                    if (intersectResult == 0x17) {
                        const int edgeClass = zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
                            contourOutput2Segment, contourOutput2Segment->next,
                            contourOutput1Segment, contourOutput1Segment->next, self);
                        if (edgeClass == 0) intersectXing->xingType = 0x17;
                    }

                    contourOutput2Segment = contourOutput2Segment->next;
                    contourOutput3Segment = contourOutput3Segment->next;
                }
            }

            contourOutput2Segment = contourOutput2Segment->next;
            contourOutput3Segment = contourOutput3Segment->next;
        } while (contourOutput2Segment != contourOutput2Start);

        contourOutput0Segment = contourOutput0Segment->next;
        contourOutput1Segment = contourOutput1Segment->next;
    } while (contourOutput0Segment != contourOutput0Start);

    if (self->xingBuffer.count != 0) {
        zGeometry_WeilerContourSegmentPartial *contourASegment = contourOutput0Start;
        zGeometry_WeilerContourSegmentPartial *contourBSegment = contourOutput1Start;

        do {
            if (contourASegment->startXing != 0) {
                contourASegment->startXing->segment2 = contourASegment;
            }

            if (contourASegment->endXing != 0) {
                contourASegment->endXing->segment0 = contourASegment;
            }

            if (contourBSegment->startXing != 0) {
                contourBSegment->startXing->segment3 = contourBSegment;
            }

            if (contourBSegment->endXing != 0) {
                contourBSegment->endXing->segment1 = contourBSegment;
            }

            contourASegment = contourASegment->next;
            contourBSegment = contourBSegment->next;
        } while (contourASegment != contourOutput0Start);

        zGeometry_WeilerContourSegmentPartial *contourCSegment = contourOutput2Start;
        zGeometry_WeilerContourSegmentPartial *contourDSegment = contourOutput3Start;

        do {
            if (contourCSegment->startXing != 0) {
                contourCSegment->startXing->segment6 = contourCSegment;
            }

            if (contourCSegment->endXing != 0) {
                contourCSegment->endXing->segment4 = contourCSegment;
            }

            if (contourDSegment->startXing != 0) {
                contourDSegment->startXing->segment7 = contourDSegment;
            }

            if (contourDSegment->endXing != 0) {
                contourDSegment->endXing->segment5 = contourDSegment;
            }

            contourCSegment = contourCSegment->next;
            contourDSegment = contourDSegment->next;
        } while (contourCSegment != contourOutput2Start);
    }

    return aggregateIntersectResult;
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerBuffer {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-init-0x467600
 * @recoil-artifact defines .text recoil:function:0x467600: zGeometry_WeilerBuffer::Init
 * Purpose: Allocate zero-filled Weiler buffer storage and initialize append state.
 */
void __fastcall Init(
    zGeometry_WeilerBufferPartial *self,
    int initialCapacity,
    int elementSize
) {
    void *const base = calloc(
        initialCapacity,
        elementSize
    );
    self->capacity = initialCapacity;
    self->base = base;
    self->elementSize = elementSize;
    self->count = 0;
    self->appendPtr = base;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-destroy-0x467630
 * @recoil-artifact defines .text recoil:function:0x467630: zGeometry_WeilerBuffer::Destroy
 * Purpose: Release backing storage and clear buffer bookkeeping.
 */
void __fastcall Destroy(
    zGeometry_WeilerBufferPartial *self
) {
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
 * Purpose: Reserve contiguous append slots, growing backing storage when needed.
 */
void *__fastcall GetAppendSpace(
    zGeometry_WeilerBufferPartial *self,
    int appendCount,
    void **outBase
) {
    const int newCount = self->count + appendCount;
    if ((unsigned int)(newCount) >= (unsigned int)(self->capacity)) {
        self->capacity += appendCount + 0x10;
        void *const base = realloc(
            self->base,
            self->capacity * self->elementSize
        );
        self->base = base;
        self->appendPtr = (void *)((unsigned int)(base) + self->elementSize * self->count);

        if (outBase != 0) {
            *outBase = base;
        }
    }

    void *const result = self->appendPtr;
    self->count = newCount;
    self->appendPtr = (void *)((unsigned int)(self->appendPtr) + appendCount * self->elementSize);
    return result;
}

} // namespace zGeometry_WeilerBuffer

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-ensurecontouroutput
 * @recoil-artifact defines .text recoil:function:0x4676c0: zGeometry_Weiler::EnsureContourOutput
 * Purpose: Ensure a contour segment has an attached contour output record.
 */
int __fastcall EnsureContourOutput(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerContourSegmentPartial *segment
) {
    if (segment->contourOutput == 0) {
        zGeometry_WeilerContourOutputPartial *const contourOutput =
            (zGeometry_WeilerContourOutputPartial
                    *)(zGeometry_WeilerBuffer::GetAppendSpace(
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
 * Purpose: Merge classified Weiler contour segments into contour output chains.
 */
int __fastcall MergeContours(
    zGeometry_WeilerStatePartial *self
) {
    zGeometry_WeilerXingPartial *const xingBase =
        (zGeometry_WeilerXingPartial *)(self->xingBuffer.base);
    if (zGeometry_Weiler::ValidateXings(
        self->xingBuffer.count,
        xingBase,
        0
    ) == 0) {
        zError::ReportOld(
            0x100,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0xc9a,
            g_zGeometry_ContourMergeValidationFailedMsg
        );
        return 0;
    }

    zGeometry_WeilerXingPartial *xing = xingBase;
    for (unsigned int xingIndex = 0; xingIndex < (unsigned int)(self->xingBuffer.count);
        ++xingIndex, ++xing) {
        zGeometry_WeilerContourSegmentPartial *const segment0 = xing->segment0;
        zGeometry_WeilerContourSegmentPartial *const segment1 = xing->segment1;
        zGeometry_WeilerContourSegmentPartial *const segment2 = xing->segment2;
        zGeometry_WeilerContourSegmentPartial *const segment3 = xing->segment3;
        zGeometry_WeilerContourSegmentPartial *const segment4 = xing->segment4;
        zGeometry_WeilerContourSegmentPartial *const segment5 = xing->segment5;
        zGeometry_WeilerContourSegmentPartial *const segment6 = xing->segment6;
        zGeometry_WeilerContourSegmentPartial *const segment7 = xing->segment7;

        switch (xing->xingType - 3) {
        case 1:
            segment0->next = segment5;
            segment1->next = segment7;
            segment2->prev = segment4;
            segment3->prev = segment6;
            segment4->next = segment2;
            segment5->next = segment0;
            segment6->prev = segment3;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xcc5);
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

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xce0);
                return 0;
            }
            break;

        case 3:
            segment2->prev = segment4;
            segment3->prev = segment6;
            segment4->next = segment2;
            segment6->prev = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe28);
                return 0;
            }
            break;

        case 4:
            segment2->prev = segment7;
            segment3->prev = segment5;
            segment5->next = segment3;
            segment7->prev = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment5) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe3d);
                return 0;
            }
            break;

        case 5:
            segment0->next = segment6;
            segment1->next = segment4;
            segment4->next = segment1;
            segment6->prev = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe52);
                return 0;
            }
            break;

        case 6:
            segment0->next = segment5;
            segment1->next = segment7;
            segment5->next = segment0;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe67);
                return 0;
            }
            break;

        case 7:
            if (segment1 != 0) {
                segment1->next = segment4;
                segment3->prev = segment6;
                segment4->next = segment1;
                segment6->prev = segment3;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment3) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe7d);
                    return 0;
                }
            } else {
                segment0->next = segment6;
                segment2->prev = segment4;
                segment4->next = segment2;
                segment6->prev = segment0;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe8d);
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

                if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment3) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xea4);
                    return 0;
                }
            } else {
                segment0->next = segment7;
                segment7->prev = segment0;
                segment2->prev = segment5;
                segment5->next = segment2;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xeb4);
                    return 0;
                }
            }
            break;

        case 9:
            segment2->prev = segment7;
            segment7->prev = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd95);
                return 0;
            }
            break;

        case 10:
            segment0->next = segment6;
            segment2->prev = segment7;
            segment6->prev = segment0;
            segment7->prev = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xcf5);
                return 0;
            }
            break;

        case 11:
            segment0->next = segment6;
            segment6->prev = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xda7);
                return 0;
            }
            break;

        case 12:
            segment3->prev = segment6;
            segment6->prev = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xdb9);
                return 0;
            }
            break;

        case 13:
            segment1->next = segment7;
            segment3->prev = segment6;
            segment6->prev = segment3;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd0a);
                return 0;
            }
            break;

        case 14:
            segment1->next = segment7;
            segment7->prev = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment1) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xdcb);
                return 0;
            }
            break;

        case 15:
            segment2->prev = segment4;
            segment4->next = segment2;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xddd);
                return 0;
            }
            break;

        case 16:
            segment0->next = segment5;
            segment2->prev = segment4;
            segment4->next = segment2;
            segment5->next = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment2) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd1f);
                return 0;
            }
            break;

        case 17:
            segment0->next = segment5;
            segment5->next = segment0;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment0) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xdef);
                return 0;
            }
            break;

        case 18:
            segment3->prev = segment5;
            segment5->next = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment3) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe01);
                return 0;
            }
            break;

        case 19:
            segment1->next = segment4;
            segment3->prev = segment5;
            segment4->next = segment1;
            segment5->next = segment3;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0 ||
                zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd34);
                return 0;
            }
            break;

        case 20:
            segment1->next = segment4;
            segment4->next = segment1;

            if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0) {
                fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xe13);
                return 0;
            }
            break;

        case 21:
            if (segment5 != 0) {
                segment0->next = segment5;
                segment2->prev = segment7;
                segment5->next = segment0;
                segment7->prev = segment2;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment5) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd4a);
                    return 0;
                }
            } else {
                segment0->next = segment6;
                segment2->prev = segment4;
                segment4->next = segment2;
                segment6->prev = segment0;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd5a);
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

                if (zGeometry_Weiler::EnsureContourOutput(self, segment5) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment7) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd71);
                    return 0;
                }
            } else {
                segment4->next = segment3;
                segment3->prev = segment4;
                segment6->prev = segment1;
                segment1->next = segment6;

                if (zGeometry_Weiler::EnsureContourOutput(self, segment4) == 0 ||
                    zGeometry_Weiler::EnsureContourOutput(self, segment6) == 0) {
                    fprintf(stderr, g_zGeometry_MergeContoursNewContourFailedFmt,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp, 0xd82);
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
 * Purpose: Rebuild contour output type and point counts, clear stale segment output ownership, and track all-single-sided state.
 */
void __fastcall NewContour(
    zGeometry_WeilerStatePartial *self
) {
    int contourCount = self->contourBuffer.count;
    zGeometry_WeilerContourOutputPartial *contour =
        (zGeometry_WeilerContourOutputPartial *)(self->contourBuffer.base);

    self->allContoursSingleSided = true;
    if (contourCount == 0) {
        return;
    }

    while (contourCount != 0) {
        zGeometry_WeilerContourSegmentPartial *const firstSegment = contour->firstSegment;
        if (firstSegment != 0) {
            contour->contourType = firstSegment->contourType;
            int primarySide = firstSegment->contourType & 3;

            if (firstSegment->contourType == 6) {
                zGeometry_WeilerContourSegmentPartial *const oldPrev = firstSegment->prev;
                firstSegment->prev = firstSegment->next;
                firstSegment->next = oldPrev;
            }

            contour->pointCount = 1;
            zGeometry_WeilerContourSegmentPartial *segment =
                zGeometry_Weiler::GetNextContourSegmentForTraversal(firstSegment);

            while (segment != firstSegment) {
                zGeometry_WeilerContourSegmentPartial *nextBase;
                if (primarySide == 0 && (segment->contourType & 3) != 0) {
                    primarySide = 1;
                    contour->contourType |= segment->contourType;
                    contour->pointCount = 1;

                    zGeometry_WeilerContourSegmentPartial *const oldPrev = firstSegment->prev;
                    firstSegment->prev = firstSegment->next;
                    firstSegment->next = oldPrev;

                    zVec3 *const oldStart = firstSegment->startPoint;
                    firstSegment->startPoint = firstSegment->endPoint;
                    firstSegment->endPoint = oldStart;
                    nextBase = firstSegment;
                } else {
                    ++contour->pointCount;
                    contour->contourType |= segment->contourType;

                    zGeometry_WeilerContourOutputPartial *const oldOutput = segment->contourOutput;
                    if (oldOutput != 0) {
                        oldOutput->firstSegment = 0;
                    }
                    segment->contourOutput = 0;
                    nextBase = segment;
                }

                segment = zGeometry_Weiler::GetNextContourSegmentForTraversal(nextBase);
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
 * Purpose: Route contour outputs to polygon sets A, B, and C according to clip mode bits and contour type.
 */
int __fastcall OutputContoursForClipMode(
    zGeometry_WeilerStatePartial *self
) {
    int contourCount = self->contourBuffer.count;
    zGeometry_WeilerContourOutputPartial *contour =
        (zGeometry_WeilerContourOutputPartial *)(self->contourBuffer.base);

    if (contourCount == 0) {
        return 1;
    }

    while (contourCount != 0) {
        if (contour->firstSegment != 0) {
            if ((self->clipMode & 1) != 0 && contour->contourType == 3 &&
                zGeometry_Weiler::OutputContourToPolygonSet(
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
                if ((contourType == 6 || contourType == 2) &&
                    zGeometry_Weiler::OutputContourToPolygonSet(
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
                if ((contourType == 1 || contourType == 5) &&
                    zGeometry_Weiler::OutputContourToPolygonSet(
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

        ++contour;
        --contourCount;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-outputcontourtopolygonset
 * @recoil-artifact defines .text recoil:function:0x4682c0: zGeometry_Weiler::OutputContourToPolygonSet
 * Purpose: Append a polygon span and copy contour segment points into the output point list.
 */
int __fastcall OutputContourToPolygonSet(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerContourOutputPartial *contour,
    zGeometry_WeilerBufferPartial *polygonBuffer,
    zGeometry_PolygonSpanArrayPartial *polygonSet
) {
    zGeometry_WeilerContourSegmentPartial *segment = contour->firstSegment;
    zGeometry_WeilerContourSegmentPartial *const lastSegment = segment->prev;
    zGeometry_WeilerClipOutputPartial *const outClip = self->outClip;

    zGeometry_PolygonPointSpanPartial *const polygon =
        (zGeometry_PolygonPointSpanPartial *)(zGeometry_WeilerBuffer::GetAppendSpace(
            polygonBuffer,
            1,
            (void **)(&polygonSet->polygons)
        ));
    if (polygon == 0) {
        return 0;
    }

    polygon->pointDwordOffset = outClip->pointList.pointCount * 3;
    polygon->pointCount = contour->pointCount;
    ++polygonSet->polygonCount;

    zVec3 *outPoint = (zVec3 *)(zGeometry_WeilerBuffer::GetAppendSpace(
        &self->pointListBuffer,
        contour->pointCount,
        (void **)(&outClip->pointList.points)
    ));
    if (outPoint == 0) {
        fprintf(
            stderr,
            g_zGeometry_OutputContourBufferEntryFailedFmt,
            g_zGeometry_SourceFile_ZgeoWeilerCpp,
            0xfb9
        );
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
 * Purpose: Swap point axes in the active input contour buffer for the contour source.
 */
void __fastcall TogglePointAxesForContourSource(
    zGeometry_WeilerStatePartial *self
) {
    zGeometry_WeilerBufferPartial *buffer = &self->inputContourABuffer;
    if (self->inputContourBBuffer.base != 0) {
        buffer = &self->inputContourBBuffer;
    }

    zVec3 *point = (zVec3 *)(buffer->base);
    for (int i = 0; i < buffer->count; ++i) {
        if (self->contourSource == 2) {
            const float y = point[i].y;
            point[i].y = point[i].z;
            point[i].z = y;
        } else {
            const float x = point[i].x;
            point[i].x = point[i].z;
            point[i].z = x;
        }
    }
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerContourSegment {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-updatebounds-0x468410
 * @recoil-artifact defines .text recoil:function:0x468410: zGeometry_WeilerContourSegment::UpdateBounds
 * Purpose: Refresh a contour segment's cached XY bounds from its endpoints.
 */
void __fastcall UpdateBounds(
    zGeometry_WeilerContourSegmentPartial *segment
) {

    zVec3 *const start = segment->startPoint;
    zVec3 *const end = segment->endPoint;

    if (start->x <= end->x) {
        segment->minX = start->x;
        segment->maxX = end->x;
    } else {
        segment->minX = end->x;
        segment->maxX = start->x;
    }

    if (start->y <= end->y) {
        segment->minY = start->y;
        segment->maxY = end->y;
    } else {
        segment->minY = end->y;
        segment->maxY = start->y;
    }

    segment->boundsDirty = 0;
}

} // namespace zGeometry_WeilerContourSegment

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-buildpointsidetablesforcontourpair
 * @recoil-artifact defines .text recoil:function:0x468470: zGeometry_Weiler::BuildPointSideTablesForContourPair
 * Purpose: Fill the contour A/B point-side tables used by Weiler contour-pair classification.
 */
void __fastcall BuildPointSideTablesForContourPair(
    zGeometry_WeilerStatePartial *self
) {
    zVec3 *edgePoints = (zVec3 *)(self->inputContourBBuffer.base);
    zVec3 *testPoints = (zVec3 *)(self->inputContourABuffer.base);
    float *table = self->contourAPointSideByContourBEdge;
    for (int edgeIndex = 0; edgeIndex < self->inputContourBBuffer.count; ++edgeIndex) {
        zVec3 *const edgeStart = &edgePoints[edgeIndex];
        zVec3 *const edgeEnd = &edgePoints[(edgeIndex + 1) % self->inputContourBBuffer.count];
        float *const rowStart2 = table;
        for (int pointIndex = 0; pointIndex < self->inputContourABuffer.count; ++pointIndex) {
            zVec3 *const point = &testPoints[pointIndex];
            const float edgeDx = edgeEnd->x - edgeStart->x;
            const float edgeDy = edgeEnd->y - edgeStart->y;
            *table++ = edgeDy * (point->x - edgeStart->x) -
                (point->y - edgeStart->y) * edgeDx;
        }
        *table++ = *rowStart2;
    }

    edgePoints = (zVec3 *)(self->inputContourABuffer.base);
    testPoints = (zVec3 *)(self->inputContourBBuffer.base);
    table = self->contourBPointSideByContourAEdge;
    for (int edgeIndex2 = 0; edgeIndex2 < self->inputContourABuffer.count; ++edgeIndex2) {
        zVec3 *const edgeStart2 = &edgePoints[edgeIndex2];
        zVec3 *const edgeEnd2 = &edgePoints[(edgeIndex2 + 1) % self->inputContourABuffer.count];
        float *const rowStart = table;
        for (int pointIndex2 = 0; pointIndex2 < self->inputContourBBuffer.count; ++pointIndex2) {
            zVec3 *const point2 = &testPoints[pointIndex2];
            const float edgeDx2 = edgeEnd2->x - edgeStart2->x;
            const float edgeDy2 = edgeEnd2->y - edgeStart2->y;
            *table++ = edgeDy2 * (point2->x - edgeStart2->x) -
                (point2->y - edgeStart2->y) * edgeDx2;
        }
        *table++ = *rowStart;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-dividecontoursegmentatpoint
 * @recoil-artifact defines .text recoil:function:0x468580: zGeometry_Weiler::DivideContourSegmentAtPoint
 * Purpose: Split a contour segment at a crossing point while preserving contour links.
 */
int __fastcall DivideContourSegmentAtPoint(
    zGeometry_WeilerStatePartial *self,
    zVec3 *xing,
    zGeometry_WeilerContourSegmentPartial *segment,
    int updateSplitLinks
) {
    zGeometry_WeilerContourSegmentPartial *nextSegment;


    if (zGeometry_Vec3::IsNearEqualXY(
        segment->endPoint,
        xing,
        0.00100000005f
    ) != 0) {
        nextSegment = segment->next;
    } else {
        nextSegment = (zGeometry_WeilerContourSegmentPartial
                *)(zGeometry_WeilerBuffer::GetAppendSpace(
                    &self->segmentBuffer,
                    1,
                    0
                ));
        if (nextSegment == 0) {
            fprintf(
                stderr,
                g_zGeometry_DivideEdgeBufferEntryFailedFmt,
                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                0x113a
            );
            return 0;
        }

        nextSegment->startPoint = xing;
        nextSegment->endPoint = segment->endPoint;

        zGeometry_WeilerContourSegmentPartial *const oldNext = segment->next;
        segment->endPoint = xing;
        nextSegment->next = oldNext;
        nextSegment->prev = segment;
        oldNext->prev = nextSegment;
        segment->next = nextSegment;

        nextSegment->contourType = segment->contourType;
        nextSegment->contourOutput = 0;
        nextSegment->endXing = segment->endXing;
        nextSegment->boundsDirty = 1;
        segment->boundsDirty = 1;
    }

    zGeometry_WeilerXingPartial *const xingLink = (zGeometry_WeilerXingPartial *)(xing);
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
 * Purpose: Insert matching forward contour split segments at a shared point.
 */
int __fastcall CreateForwardSegmentPairAtPoint(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment,
    zVec3 *point,
    int firstContourTypeMask,
    int secondContourTypeMask
) {
    zGeometry_WeilerContourSegmentPartial *segment = firstSegment;
    int contourTypeMask = firstContourTypeMask;

    for (int i = 0; i < 2; ++i) {
        zGeometry_WeilerContourSegmentPartial *const newSegment =
            (zGeometry_WeilerContourSegmentPartial
                    *)(zGeometry_WeilerBuffer::GetAppendSpace(
                        &self->segmentBuffer,
                        1,
                        0
                    ));
        if (newSegment == 0) {
            zError::ReportOld(
                0x200,
                g_zGeometry_SourceFile_ZgeoWeilerCpp,
                0x1181,
                g_zGeometry_BufferEntryFailedMsg
            );
            return 0;
        }

        newSegment->prev = segment;
        newSegment->next = segment->next;
        newSegment->contourType = segment->contourType | contourTypeMask;
        newSegment->startPoint = point;
        newSegment->endPoint = segment->endPoint;
        newSegment->startXing = 0;
        newSegment->endXing = segment->endXing;
        newSegment->contourOutput = 0;
        zGeometry_WeilerContourSegment::UpdateBounds(newSegment);

        segment->next->prev = newSegment;
        segment->next = newSegment;

        segment = secondSegment;
        contourTypeMask = secondContourTypeMask;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-outputselectedinputcontourtopolygonseta
 * @recoil-artifact defines .text recoil:function:0x468700: zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA
 * Purpose: Append the selected input contour into polygon set A of the caller-owned Weiler clip output.
 */
int __fastcall OutputSelectedInputContourToPolygonSetA(
    zGeometry_WeilerStatePartial *self,
    int mode
) {
    if ((self->clipMode & 1) == 0) {
        return 1;
    }

    zGeometry_WeilerBufferPartial *selectedInputContour = &self->inputContourBBuffer;
    if (mode != 3) {
        selectedInputContour = &self->inputContourABuffer;
    }

    zGeometry_WeilerClipOutputPartial *const outClip = self->outClip;
    outClip->polygonSetA.polygonCount = 1;

    const int oldPointCount = outClip->pointList.pointCount;
    const int selectedPointCount = selectedInputContour->count;
    const int totalPointCount = oldPointCount + selectedPointCount;

    if ((unsigned int)(totalPointCount) > 0x80) {
        outClip->pointList.points = (zVec3 *)(realloc(
            outClip->pointList.points,
            (size_t)(totalPointCount) * sizeof(zVec3)
        ));
    }

    outClip->polygonSetA.polygons->pointCount = selectedPointCount;
    outClip->polygonSetA.polygons->pointDwordOffset = oldPointCount * 3;

    const size_t pointBytes = (size_t)(selectedPointCount) * sizeof(zVec3);
    memcpy(
        &outClip->pointList.points[oldPointCount],
        selectedInputContour->base,
        pointBytes
    );

    outClip->pointList.pointCount = oldPointCount + selectedPointCount;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-generateoutsideresults
 * @recoil-artifact defines .text recoil:function:0x4687b0: zGeometry_Weiler::GenerateOutsideResults
 * Purpose: Emit an outside-result polygon span and wrapped B/A point bridge when clip mode requests outside output.
 */
int __fastcall GenerateOutsideResults(
    zGeometry_WeilerStatePartial *self
) {
    const int contourAPointCount = self->inputContourABuffer.count;
    zVec3 *const contourBPoints = (zVec3 *)(self->inputContourBBuffer.base);
    zGeometry_WeilerClipOutputPartial *const outClip = self->outClip;
    const int contourBPointCount = self->inputContourBBuffer.count;

    if ((self->clipMode & 2) == 0) {
        return 1;
    }

    zGeometry_PolygonPointSpanPartial *const polygon =
        (zGeometry_PolygonPointSpanPartial *)(zGeometry_WeilerBuffer::GetAppendSpace(
            &self->polygonSetBBuffer,
            1,
            (void **)(&outClip->polygonSetB.polygons)
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

    zVec3 *outPoint = (zVec3 *)(zGeometry_WeilerBuffer::GetAppendSpace(
        &self->pointListBuffer,
        polygon->pointCount,
        (void **)(&outClip->pointList.points)
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

    zVec3 *const contourAPoints = (zVec3 *)(self->inputContourABuffer.base);
    zVec3 *selectedContourAPoint = contourAPoints;
    zVec3 *contourAPoint = &contourAPoints[1];
    for (int i = contourAPointCount - 1; i > 0; --i) {
        if (contourAPoint->x > selectedContourAPoint->x ||
            (fabs((double)(contourAPoint->x) - (double)(selectedContourAPoint->x)) <
                 0.0000099999997473787516 &&
             contourAPoint->y > selectedContourAPoint->y)) {
            selectedContourAPoint = contourAPoint;
        }

        ++contourAPoint;
    }

    zVec3 *selectedContourBPoint = contourBPoints;
    zVec3 *contourBPoint = &contourBPoints[1];
    for (int i_2005 = contourBPointCount - 1; i_2005 > 0; --i_2005) {
        if (contourBPoint->x > selectedContourBPoint->x ||
            (fabs((double)(contourBPoint->x) - (double)(selectedContourBPoint->x)) <
                 0.0000099999997473787516 &&
             contourBPoint->y > selectedContourBPoint->y)) {
            selectedContourBPoint = contourBPoint;
        }

        ++contourBPoint;
    }

    zGeometry_Weiler::SelectForwardStartPointInContourA(
        selectedContourBPoint,
        &selectedContourAPoint,
        self
    );

    zVec3 *const contourBLastPoint = &contourBPoints[contourBPointCount - 1];
    contourBPoint = selectedContourBPoint;
    for (int i_2018 = contourBPointCount; i_2018 > 0; --i_2018) {
        *outPoint++ = *contourBPoint;
        contourBPoint = contourBPoint == contourBLastPoint ? contourBPoints : contourBPoint + 1;
    }

    *outPoint++ = *contourBPoint;

    zVec3 *contourAWritePoint = selectedContourAPoint;
    for (int i_2026 = contourAPointCount; i_2026 > 0; --i_2026) {
        *outPoint++ = *contourAWritePoint;
        contourAWritePoint = contourAWritePoint == contourAPoints
                                 ? &contourAPoints[contourAPointCount - 1]
                                 : contourAWritePoint - 1;
    }

    *outPoint++ = *contourAWritePoint;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifypointincontourpointlistxy
 * @recoil-artifact defines .text recoil:function:0x468a10: zGeometry_Weiler::ClassifyPointInContourPointListXY
 * Purpose: Classify a test point as outside, on, or inside an XY contour by crossing parity.
 */
int __fastcall ClassifyPointInContourPointListXY(
    zVec3 *point,
    int contourPointCount,
    zVec3 *contourPoints
) {
    if (contourPointCount <= 0) {
        return -1;
    }

    zVec3 *previous = &contourPoints[contourPointCount - 1];
    int previousXSide = previous->x < point->x ? -1 : previous->x > point->x ? 1 : 0;
    int previousYSide = previous->y < point->y ? -1 : previous->y > point->y ? 1 : 0;

    if (previousXSide == 0 && previousYSide == 0) {
        return 0;
    }

    int crossingParity = 0;

    for (int i = 0; i < contourPointCount; ++i) {
        zVec3 *const current = &contourPoints[i];
        const int currentXSide = current->x < point->x ? -1 : current->x > point->x ? 1 : 0;
        const int currentYSide = current->y < point->y ? -1 : current->y > point->y ? 1 : 0;

        if (currentXSide == 0 && currentYSide == 0) {
            return 0;
        }

        if (currentXSide != previousXSide) {
            const int currentYNonNegative = currentYSide >= 0;
            const int previousYNonNegative = previousYSide >= 0;

            if (currentYNonNegative != previousYNonNegative) {
                const float xIntersection = (point->y - current->y) / (previous->y - current->y) *
                                                (previous->x - current->x) +
                                            current->x;
                const int intersectionSide =
                    xIntersection < point->x ? -1 : xIntersection > point->x ? 1 : 0;

                if (intersectionSide == 0) {
                    return 0;
                }

                if (intersectionSide == 1) {
                    ++crossingParity;
                }
            } else if (currentYSide == 0 && previousYSide == 0) {
                return 0;
            }
        } else if ((currentYSide >= 0) != (previousYSide >= 0)) {
            if (currentXSide == 1) {
                ++crossingParity;
            } else if (currentXSide == 0) {
                return 0;
            }
        }

        previous = current;
        previousXSide = currentXSide;
        previousYSide = currentYSide;
    }

    return (crossingParity & 1) != 0 ? 1 : -1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-intersect2d
 * @recoil-artifact defines .text recoil:function:0x468c40: zGeometry_Weiler::Intersect2d
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp; BN x87 rendering is limited at the classifier callsite and computed Y store, so assembly is source of truth.
 * Purpose: Build the crossing record, if any, for the classified intersection between two XY edges.
 */
int __fastcall Intersect2d(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerXingPartial **outXing,
    zVec3 edge0Start,
    zVec3 edge0End,
    zVec3 edge1Start,
    zVec3 edge1End
) {
    int xingType =
        zGeometry_Weiler::ClassifyIntersect2d(
            &edge0Start,
            &edge0End,
            &edge1Start,
            &edge1End,
            self
        );

    zGeometry_WeilerXingPartial *createdXing = 0;
    if ((unsigned int)(xingType) <= 0x17) {
        switch (kIntersect2dOutputKindByXingType[xingType]) {
        case 1: {
            const double edge1ReverseDeltaX = (double)(edge1Start.x) - (double)(edge1End.x);
            const double edge1ReverseDeltaY = (double)(edge1Start.y) - (double)(edge1End.y);
            const double edge0DeltaX = (double)(edge0End.x) - (double)(edge0Start.x);
            const double edge0DeltaY = (double)(edge0End.y) - (double)(edge0Start.y);
            const double divisor =
                edge1ReverseDeltaY * edge0DeltaX - edge1ReverseDeltaX * edge0DeltaY;

            if (divisor == 0.0) {
                xingType = 0;
                break;
            }

            createdXing = (zGeometry_WeilerXingPartial
                *)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            const double edge0Param =
                (((double)(edge1Start.x) - (double)(edge0Start.x)) * edge1ReverseDeltaY +
                    ((double)(edge1Start.y) - (double)(edge0Start.y)) * -edge1ReverseDeltaX) /
                divisor;
            createdXing->point.x = (float)(edge0DeltaX * edge0Param + edge0Start.x);
            createdXing->point.y = (float)(edge0DeltaY * edge0Param + edge0Start.y);

            if (edge1ReverseDeltaX != 0.0) {
                createdXing->point.z =
                    (float)((((double)(edge1Start.x) - (double)(createdXing->point.x)) /
                                edge1ReverseDeltaX) *
                                ((double)(edge1End.z) - (double)(edge1Start.z)) +
                            (double)(edge1Start.z));
            } else {
                createdXing->point.z =
                    (float)((((double)(edge1Start.y) - (double)(createdXing->point.y)) /
                                ((double)(edge1Start.y) - (double)(edge1End.y))) *
                                ((double)(edge1End.z) - (double)(edge1Start.z)) +
                            (double)(edge1Start.z));
            }

            break;
        }

        case 2:
            createdXing = (zGeometry_WeilerXingPartial
                *)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(stderr, g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x1351);
                return 1;
            }

            createdXing->point = edge0Start;
            break;

        case 3:
            createdXing = (zGeometry_WeilerXingPartial
                *)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(stderr, g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x1363);
                return 1;
            }

            createdXing->point = edge0End;
            break;

        case 4:
            createdXing = (zGeometry_WeilerXingPartial
                *)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(stderr, g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x132a);
                return 1;
            }

            createdXing->point = edge1Start;
            break;

        case 5:
            createdXing = (zGeometry_WeilerXingPartial
                *)(zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
            if (createdXing == 0) {
                fprintf(stderr, g_zGeometry_Intersect2dBufferEntryFailedFmt,
                    g_zGeometry_SourceFile_ZgeoWeilerCpp, 0x1340);
                return 1;
            }

            createdXing->point = edge1End;
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
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp; BN x87 sign-class HLIL is limited, so assembly is source of truth.
 * Purpose: Classify two XY edges into the Weiler intersection case table, including contour-side disambiguation for vertex cases.
 */
int __fastcall ClassifyIntersect2d(
    zVec3 *edge0Start,
    zVec3 *edge0End,
    zVec3 *edge1Start,
    zVec3 *edge1End,
    zGeometry_WeilerStatePartial *self
) {
    const float edge1DeltaY = edge1End->y - edge1Start->y;
    const float edge1DeltaX = edge1End->x - edge1Start->x;

    float edge0StartSide =
        (edge0Start->x - edge1Start->x) * edge1DeltaY -
        (edge0Start->y - edge1Start->y) * edge1DeltaX;
    float edge0EndSide =
        (edge0End->x - edge1Start->x) * edge1DeltaY -
        (edge0End->y - edge1Start->y) * edge1DeltaX;
    if ((edge0StartSide < 0.0f && edge0EndSide < 0.0f) ||
        (edge0StartSide > 0.0f && edge0EndSide > 0.0f)) {
        return 0;
    }

    const float edge0DeltaX = edge0End->x - edge0Start->x;
    const float edge0DeltaY = edge0End->y - edge0Start->y;
    float edge1StartSide =
        (edge1Start->x - edge0Start->x) * edge0DeltaY -
        (edge1Start->y - edge0Start->y) * edge0DeltaX;
    float edge1EndSide =
        (edge1End->x - edge0Start->x) * edge0DeltaY -
        (edge1End->y - edge0Start->y) * edge0DeltaX;
    if ((edge1StartSide < 0.0f && edge1EndSide < 0.0f) ||
        (edge1StartSide > 0.0f && edge1EndSide > 0.0f)) {
        return 0;
    }

    const int zeroSideCount = (edge0StartSide == 0.0f ? 1 : 0) + (edge0EndSide == 0.0f ? 1 : 0) +
                              (edge1StartSide == 0.0f ? 1 : 0) + (edge1EndSide == 0.0f ? 1 : 0);

    if (zeroSideCount == 2) {
        zVec3 probe;
        probe.z = 0.0f;

        if (edge1StartSide == 0.0f) {
            probe.x = edge1Start->x + edge1DeltaX * 0.00000999999975f;
            probe.y = edge1Start->y + edge1DeltaY * 0.00000999999975f;

            if (edge1EndSide > 0.0f) {
                if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                        &probe,
                        self->inputContourABuffer.count,
                        (zVec3 *)(self->inputContourABuffer.base)
                    ) > 0) {
                    edge0StartSide = -edge0StartSide;
                    edge0EndSide = -edge0EndSide;
                    edge1EndSide = -edge1EndSide;
                }
            } else if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                           &probe,
                           self->inputContourABuffer.count,
                           (zVec3 *)(self->inputContourABuffer.base)
                       ) < 0) {
                edge0StartSide = -edge0StartSide;
                edge0EndSide = -edge0EndSide;
                edge1EndSide = -edge1EndSide;
            }
        } else {
            probe.x = edge1Start->x + edge1DeltaX * 0.999989986f;
            probe.y = edge1Start->y + edge1DeltaY * 0.999989986f;

            if (edge1StartSide > 0.0f) {
                if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                        &probe,
                        self->inputContourABuffer.count,
                        (zVec3 *)(self->inputContourABuffer.base)
                    ) > 0) {
                    edge0StartSide = -edge0StartSide;
                    edge0EndSide = -edge0EndSide;
                    edge1StartSide = -edge1StartSide;
                }
            } else if (zGeometry_Weiler::ClassifyPointInContourPointListXY(
                           &probe,
                           self->inputContourABuffer.count,
                           (zVec3 *)(self->inputContourABuffer.base)
                       ) < 0) {
                edge0StartSide = -edge0StartSide;
                edge0EndSide = -edge0EndSide;
                edge1StartSide = -edge1StartSide;
            }
        }
    }

    const int edge0StartClass = edge0StartSide < 0.0f ? 0 : edge0StartSide == 0.0f ? 1 : 2;
    const int edge0EndClass = edge0EndSide < 0.0f ? 0 : edge0EndSide == 0.0f ? 1 : 2;
    const int edge1StartClass = edge1StartSide < 0.0f ? 0 : edge1StartSide == 0.0f ? 1 : 2;
    const int edge1EndClass = edge1EndSide < 0.0f ? 0 : edge1EndSide == 0.0f ? 1 : 2;
    const int index = ((edge0StartClass * 3 + edge0EndClass) * 3 + edge1StartClass) * 3 +
                      edge1EndClass;

    return kIntersect2dCaseIdBySignClass[index];
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerContourSegmentArray {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-updatebounds-0x4693a0
 * @recoil-artifact defines .text recoil:function:0x4693a0: zGeometry_WeilerContourSegmentArray::UpdateBounds
 * Purpose: Refresh cached XY bounds for each segment in a contour segment array.
 */
void __fastcall UpdateBounds(
    zGeometry_WeilerContourSegmentPartial *segments,
    int segmentCount
) {
    for (int i = 0; i < segmentCount; ++i) {
        zGeometry_WeilerContourSegment::UpdateBounds(&segments[i]);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-initfrompointlist
 * @recoil-artifact defines .text recoil:function:0x4693c0: zGeometry_WeilerContourSegmentArray::InitFromPointList
 * Purpose: Build a linked contour segment ring from a point list.
 */
void __fastcall InitFromPointList(
    zGeometry_WeilerContourSegmentPartial *segments,
    zVec3 *points,
    int pointCount,
    int contourType
) {
    zVec3 *point = points;
    for (int i = 0; i < pointCount; ++i) {
        zGeometry_WeilerContourSegmentPartial *const segment = &segments[i];
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

    zGeometry_WeilerContourSegmentPartial *const lastSegment = &segments[pointCount - 1];
    segments->prev = lastSegment;
    lastSegment->next = segments;
    lastSegment->endPoint = points;
}

} // namespace zGeometry_WeilerContourSegmentArray

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-getnextcontoursegmentfortraversal
 * @recoil-artifact defines .text recoil:function:0x469430: zGeometry_Weiler::GetNextContourSegmentForTraversal
 * Purpose: Advance Weiler contour traversal while reversing adjacent segment links for two-node contour cases.
 */
zGeometry_WeilerContourSegmentPartial *__fastcall GetNextContourSegmentForTraversal(
    zGeometry_WeilerContourSegmentPartial *segment
) {
    zGeometry_WeilerContourSegmentPartial *const next = segment->next;

    if (next->next == segment) {
        zGeometry_WeilerContourSegmentPartial *const oldPrev = next->prev;
        next->prev = segment;
        next->next = oldPrev;

        zVec3 *const oldStart = next->startPoint;
        next->startPoint = next->endPoint;
        next->endPoint = oldStart;
    }

    return next;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifyadjacentedgepairagainstcontoursegment
 * @recoil-artifact defines .text recoil:function:0x469450: zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment
 * Purpose: Classify whether an adjacent edge pair crosses or lies to one side of a contour segment.
 */
int __fastcall ClassifyAdjacentEdgePairAgainstContourSegment(
    zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment,
    zGeometry_WeilerContourSegmentPartial *contourSegment
) {
    if (firstSegment->endPoint != secondSegment->startPoint) {
        return 0;
    }

    const float contourDeltaX = contourSegment->endPoint->x - contourSegment->startPoint->x;
    const float contourDeltaY = contourSegment->endPoint->y - contourSegment->startPoint->y;
    const float firstSide =
        (firstSegment->startPoint->x - contourSegment->startPoint->x) * contourDeltaY -
        (firstSegment->startPoint->y - contourSegment->startPoint->y) * contourDeltaX;
    const float secondSide =
        (secondSegment->endPoint->x - contourSegment->startPoint->x) * contourDeltaY -
        (secondSegment->endPoint->y - contourSegment->startPoint->y) * contourDeltaX;

    if ((firstSide < 0.0f && secondSide > 0.0f) || (firstSide > 0.0f && secondSide < 0.0f)) {
        return 7;
    }

    const float firstDeltaX = firstSegment->endPoint->x - firstSegment->startPoint->x;
    const float firstDeltaY = firstSegment->endPoint->y - firstSegment->startPoint->y;
    return ((secondSegment->endPoint->x - firstSegment->startPoint->x) * firstDeltaY -
            (secondSegment->endPoint->y - firstSegment->startPoint->y) * firstDeltaX) > 0.0f
               ? 1
               : 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-classifyadjacentedgepairagainstadjacentedgepair
 * @recoil-artifact defines .text recoil:function:0x469560: zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair
 * Purpose: Classify two linked adjacent edge pairs by their endpoint wedge relationship.
 */
int __fastcall ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
    zGeometry_WeilerContourSegmentPartial *pairAFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairASecondSegment,
    zGeometry_WeilerContourSegmentPartial *pairBFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairBSecondSegment,
    zGeometry_WeilerStatePartial *
) {
    if (pairAFirstSegment->endPoint != pairASecondSegment->startPoint ||
        pairBFirstSegment->endPoint != pairBSecondSegment->startPoint) {
        return 0;
    }

    const float pairBFirstDeltaX =
        pairBFirstSegment->endPoint->x - pairBFirstSegment->startPoint->x;
    const float pairBFirstDeltaY =
        pairBFirstSegment->endPoint->y - pairBFirstSegment->startPoint->y;
    const float pairBSecondDeltaX =
        pairBSecondSegment->endPoint->x - pairBSecondSegment->startPoint->x;
    const float pairBSecondDeltaY =
        pairBSecondSegment->endPoint->y - pairBSecondSegment->startPoint->y;
    const bool startFirstSideNegative =
        ((pairAFirstSegment->startPoint->x - pairBFirstSegment->startPoint->x) *
                pairBFirstDeltaY -
            (pairAFirstSegment->startPoint->y - pairBFirstSegment->startPoint->y) *
                pairBFirstDeltaX) <
        0.0f;
    const bool startSecondSideNegative =
        ((pairAFirstSegment->startPoint->x - pairBSecondSegment->startPoint->x) *
                pairBSecondDeltaY -
            (pairAFirstSegment->startPoint->y - pairBSecondSegment->startPoint->y) *
                pairBSecondDeltaX) <
        0.0f;
    int startClass = -1;
    if ((startFirstSideNegative && startSecondSideNegative) ||
        (startFirstSideNegative != startSecondSideNegative &&
            ((pairBSecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) *
                    pairBFirstDeltaY -
                (pairBSecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) *
                    pairBFirstDeltaX) >
                0.0f)) {
        startClass = 1;
    }

    const bool endFirstSideNegative =
        ((pairASecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) *
                pairBFirstDeltaY -
            (pairASecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) *
                pairBFirstDeltaX) <
        0.0f;
    const bool endSecondSideNegative =
        ((pairASecondSegment->endPoint->x - pairBSecondSegment->startPoint->x) *
                pairBSecondDeltaY -
            (pairASecondSegment->endPoint->y - pairBSecondSegment->startPoint->y) *
                pairBSecondDeltaX) <
        0.0f;
    int endClass = -1;
    if ((endFirstSideNegative && endSecondSideNegative) ||
        (endFirstSideNegative != endSecondSideNegative &&
            ((pairBSecondSegment->endPoint->x - pairBFirstSegment->startPoint->x) *
                    pairBFirstDeltaY -
                (pairBSecondSegment->endPoint->y - pairBFirstSegment->startPoint->y) *
                    pairBFirstDeltaX) >
                0.0f)) {
        endClass = 1;
    }

    if (startClass == -1 && endClass == -1) {
        const float pairAFirstDeltaX =
            pairAFirstSegment->endPoint->x - pairAFirstSegment->startPoint->x;
        const float pairAFirstDeltaY =
            pairAFirstSegment->endPoint->y - pairAFirstSegment->startPoint->y;
        const float pairASecondDeltaX =
            pairASecondSegment->endPoint->x - pairASecondSegment->startPoint->x;
        const float pairASecondDeltaY =
            pairASecondSegment->endPoint->y - pairASecondSegment->startPoint->y;
        const bool allNegative =
            ((pairBFirstSegment->startPoint->x - pairAFirstSegment->startPoint->x) *
                    pairAFirstDeltaY -
                (pairBFirstSegment->startPoint->y - pairAFirstSegment->startPoint->y) *
                    pairAFirstDeltaX) <
                0.0f &&
            ((pairBFirstSegment->startPoint->x - pairASecondSegment->startPoint->x) *
                    pairASecondDeltaY -
                (pairBFirstSegment->startPoint->y - pairASecondSegment->startPoint->y) *
                    pairASecondDeltaX) <
                0.0f &&
            ((pairBSecondSegment->endPoint->x - pairAFirstSegment->startPoint->x) *
                    pairAFirstDeltaY -
                (pairBSecondSegment->endPoint->y - pairAFirstSegment->startPoint->y) *
                    pairAFirstDeltaX) <
                0.0f &&
            ((pairBSecondSegment->endPoint->x - pairASecondSegment->startPoint->x) *
                    pairASecondDeltaY -
                (pairBSecondSegment->endPoint->y - pairASecondSegment->startPoint->y) *
                    pairASecondDeltaX) <
                0.0f;
        return allNegative ? 6 : 5;
    }

    if (startClass == 1 && endClass == 1) {
        const float pairAFirstDeltaX =
            pairAFirstSegment->endPoint->x - pairAFirstSegment->startPoint->x;
        const float pairAFirstDeltaY =
            pairAFirstSegment->endPoint->y - pairAFirstSegment->startPoint->y;
        const float pairASecondDeltaX =
            pairASecondSegment->endPoint->x - pairASecondSegment->startPoint->x;
        const float pairASecondDeltaY =
            pairASecondSegment->endPoint->y - pairASecondSegment->startPoint->y;
        const bool allNegative =
            ((pairBFirstSegment->startPoint->x - pairAFirstSegment->startPoint->x) *
                    pairAFirstDeltaY -
                (pairBFirstSegment->startPoint->y - pairAFirstSegment->startPoint->y) *
                    pairAFirstDeltaX) <
                0.0f &&
            ((pairBFirstSegment->startPoint->x - pairASecondSegment->startPoint->x) *
                    pairASecondDeltaY -
                (pairBFirstSegment->startPoint->y - pairASecondSegment->startPoint->y) *
                    pairASecondDeltaX) <
                0.0f &&
            ((pairBSecondSegment->endPoint->x - pairAFirstSegment->startPoint->x) *
                    pairAFirstDeltaY -
                (pairBSecondSegment->endPoint->y - pairAFirstSegment->startPoint->y) *
                    pairAFirstDeltaX) <
                0.0f &&
            ((pairBSecondSegment->endPoint->x - pairASecondSegment->startPoint->x) *
                    pairASecondDeltaY -
                (pairBSecondSegment->endPoint->y - pairASecondSegment->startPoint->y) *
                    pairASecondDeltaX) <
                0.0f;
        return allNegative ? 4 : 3;
    }

    return startClass == -1 ? 8 : 9;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-recenterpointsetsifoutofrange
 * @recoil-artifact defines .text recoil:function:0x469960: zGeometry_Weiler::RecenterPointSetsIfOutOfRange
 * Purpose: Translate input points when their coordinates are outside the local range.
 */
void __fastcall RecenterPointSetsIfOutOfRange(
    zGeometry_WeilerStatePartial *self
) {
    if (self->inputContourBBuffer.base != 0) {
        if (self->inputContourBBuffer.count != 0) {
            zVec3 *point = (zVec3 *)(self->inputContourBBuffer.base);
            for (int i = 0; i < self->inputContourBBuffer.count; ++i) {
                point[i].x -= self->pointTranslationX;
                point[i].y -= self->pointTranslationY;
            }
        }

        return;
    }

    zVec3 *const firstPoint = (zVec3 *)(self->inputContourABuffer.base);
    if (firstPoint->x < 65536.0f && firstPoint->x > -65536.0f && firstPoint->y < 65536.0f &&
        firstPoint->y > -65536.0f) {
        self->pointsRecentered = false;
        return;
    }

    self->pointTranslationX = firstPoint->x;
    self->pointTranslationY = firstPoint->y;
    self->pointsRecentered = true;

    if (self->inputContourABuffer.count != 0) {
        zVec3 *point = (zVec3 *)(self->inputContourABuffer.base);
        for (int i = 0; i < self->inputContourABuffer.count; ++i) {
            point[i].x -= self->pointTranslationX;
            point[i].y -= self->pointTranslationY;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-preclassifyinputcontouraadjacentedgepairs
 * @recoil-artifact defines .text recoil:function:0x469a30: zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs
 * Purpose: Reset clipping scratch buffers and seed contour A's forward and reverse adjacent-edge segment rings.
 */
void __fastcall PreclassifyInputContourAAdjacentEdgePairs(
    zGeometry_WeilerStatePartial *self
) {
    const int pointCount = self->inputContourABuffer.count;
    zGeometry_WeilerContourSegmentPartial *const segmentBase =
        (zGeometry_WeilerContourSegmentPartial *)(self->segmentBuffer.base);
    zGeometry_WeilerContourOutputPartial *const contourBase =
        (zGeometry_WeilerContourOutputPartial *)(self->contourBuffer.base);
    zVec3 *const points = (zVec3 *)(self->inputContourABuffer.base);

    zGeometry_WeilerBuffer::SetCountAndAppendPtr(
        &self->segmentBuffer,
        pointCount << 1
    );
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(
        &self->contourBuffer,
        2
    );
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(
        &self->xingBuffer,
        0
    );
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(
        &self->polygonSetABuffer,
        0
    );
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(
        &self->polygonSetBBuffer,
        0
    );
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(
        &self->polygonSetCBuffer,
        0
    );
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(
        &self->pointListBuffer,
        0
    );

    zGeometry_WeilerContourSegmentArray::InitFromPointList(
        segmentBase,
        points,
        pointCount,
        1
    );
    segmentBase->contourOutput = contourBase;
    contourBase[0].firstSegment = segmentBase;
    zGeometry_WeilerContourSegmentArray::UpdateBounds(
        segmentBase,
        pointCount
    );

    zGeometry_WeilerContourSegmentPartial *const reverseSegments = &segmentBase[pointCount];
    zGeometry_WeilerContourSegmentArray::InitFromPointList(
        reverseSegments,
        points,
        pointCount,
        4
    );
    reverseSegments->contourOutput = &contourBase[1];
    contourBase[1].firstSegment = reverseSegments;
    zGeometry_WeilerContourSegmentArray::UpdateBounds(
        reverseSegments,
        pointCount
    );
}

} // namespace zGeometry_Weiler

namespace zGeometry_WeilerBuffer {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-setcountandappendptr
 * @recoil-artifact defines .text recoil:function:0x469ae0: zGeometry_WeilerBuffer::SetCountAndAppendPtr
 * Purpose: Reset the logical count and append pointer within the backing store.
 */
void __fastcall SetCountAndAppendPtr(
    zGeometry_WeilerBufferPartial *self,
    int count
) {
    self->count = count;
    self->appendPtr = (void *)((unsigned int)(self->base) + count * self->elementSize);
}

} // namespace zGeometry_WeilerBuffer

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-restorepointtranslation
 * @recoil-artifact defines .text recoil:function:0x469af0: zGeometry_Weiler::RestorePointTranslation
 * Purpose: Restore the saved XY translation to caller-owned input points and generated output points.
 */
void __fastcall RestorePointTranslation(
    zGeometry_WeilerStatePartial *self
) {
    const float translationX = self->pointTranslationX;
    const float translationY = self->pointTranslationY;

    zVec3 *point = (zVec3 *)(self->inputContourBBuffer.base);
    for (int i = self->inputContourBBuffer.count; i != 0; --i) {
        point->x += translationX;
        point->y += translationY;
        ++point;
    }

    zGeometry_WeilerClipOutputPartial *const outClip = self->outClip;
    point = outClip->pointList.points;
    for (int i_2790 = outClip->pointList.pointCount; i_2790 != 0; --i_2790) {
        point->x += translationX;
        point->y += translationY;
        ++point;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-restoreoutputzfrominputplane
 * @recoil-artifact defines .text recoil:function:0x469b60: zGeometry_Weiler::RestoreOutputZFromInputPlane
 * Purpose: Restore output point Z values from the input contour B plane.
 */
void __fastcall RestoreOutputZFromInputPlane(
    zGeometry_WeilerStatePartial *self
) {
    zVec3 *const inputPoints = (zVec3 *)(self->inputContourBBuffer.base);

    const zVec3 edge01 = {
        inputPoints[0].x - inputPoints[1].x,
        inputPoints[0].y - inputPoints[1].y,
        inputPoints[0].z - inputPoints[1].z,
    };
    const zVec3 edge12 = {
        inputPoints[2].x - inputPoints[1].x,
        inputPoints[2].y - inputPoints[1].y,
        inputPoints[2].z - inputPoints[1].z,
    };

    zVec3 planeNormal;
    planeNormal.x = edge01.y * edge12.z - edge01.z * edge12.y;
    planeNormal.y = edge01.z * edge12.x - edge01.x * edge12.z;
    planeNormal.z = edge01.x * edge12.y - edge01.y * edge12.x;

    if (planeNormal.z == 0.0f) {
        return;
    }

    planeNormal.x /= planeNormal.z;
    planeNormal.y /= planeNormal.z;

    const float planeOffset =
        -(planeNormal.x * inputPoints[0].x + planeNormal.y * inputPoints[0].y + inputPoints[0].z);

    zGeometry_WeilerClipOutputPartial *const outClip = self->outClip;
    zVec3 *point = outClip->pointList.points;
    for (unsigned int i = 0; i < (unsigned int)(outClip->pointList.pointCount); ++i) {
        point->z = -(planeNormal.x * point->x + planeNormal.y * point->y + planeOffset);
        ++point;
    }
}

} // namespace zGeometry_Weiler

namespace zGeometry_Vec3 {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-isbetweenendpointsxy
 * @recoil-artifact defines .text recoil:function:0x469ca0: zGeometry_Vec3::IsBetweenEndpointsXY
 * Purpose: Test whether a point lies within the inclusive XY endpoint span of a segment.
 */
int __fastcall IsBetweenEndpointsXY(
    zVec3 *testPoint,
    zVec3 *startPoint,
    zVec3 *endPoint
) {
    if (fabs((double)(startPoint->x) - (double)(endPoint->x)) < 0.0000099999997473787516) {
        if (startPoint->y < endPoint->y) {
            return testPoint->y >= startPoint->y && testPoint->y <= endPoint->y;
        }

        return testPoint->y >= endPoint->y && testPoint->y <= startPoint->y;
    }

    if (startPoint->x < endPoint->x) {
        return testPoint->x >= startPoint->x && testPoint->x <= endPoint->x;
    }

    return testPoint->x >= endPoint->x && testPoint->x <= startPoint->x;
}

} // namespace zGeometry_Vec3

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-selectforwardstartpointincontoura
 * @recoil-artifact defines .text recoil:function:0x469d60: zGeometry_Weiler::SelectForwardStartPointInContourA
 * Purpose: Choose the forward start point on contour A for outside-results bridge traversal.
 */
void __fastcall SelectForwardStartPointInContourA(
    zVec3 *point,
    zVec3 **selectedPoint,
    zGeometry_WeilerStatePartial *self
) {
    const int pointCount = self->inputContourABuffer.count;
    zVec3 *const points = (zVec3 *)(self->inputContourABuffer.base);
    if (pointCount == 0) {
        return;
    }

    zVec3 *currentPoint = points;
    zVec3 *previousPoint = &points[pointCount - 1];
    int remainingPointCount = pointCount - 1;

    while (true) {
        zVec3 *const candidatePoint = *selectedPoint;
        const float edgeDeltaX = currentPoint->x - previousPoint->x;
        const float edgeDeltaY = currentPoint->y - previousPoint->y;
        const float pointCross =
            (point->x - previousPoint->x) * edgeDeltaY -
            (point->y - previousPoint->y) * edgeDeltaX;
        const float candidateCross =
            (candidatePoint->x - previousPoint->x) * edgeDeltaY -
            (candidatePoint->y - previousPoint->y) * edgeDeltaX;

        if ((pointCross > 0.0f && candidateCross < 0.0f) ||
            (pointCross < 0.0f && candidateCross > 0.0f)) {
            if (currentPoint->x >= point->x) {
                previousPoint = currentPoint;
            }

            *selectedPoint = previousPoint;
            remainingPointCount = pointCount;
            currentPoint = points;
            previousPoint = &points[pointCount - 1];
        }

        const int i = remainingPointCount;
        --remainingPointCount;
        if (i == 0) {
            break;
        }
    }
}

} // namespace zGeometry_Weiler

namespace zGeometry_Vec3 {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-isnearequalxy
 * @recoil-artifact defines .text recoil:function:0x469e50: zGeometry_Vec3::IsNearEqualXY
 * Purpose: Compare two vectors in XY using the caller-supplied tolerance.
 */
int __fastcall IsNearEqualXY(
    zVec3 *vecA,
    zVec3 *vecB,
    float tolerance
) {
    if (fabs(vecA->x - vecB->x) <= tolerance && fabs(vecA->y - vecB->y) <= tolerance) {
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-snappointtosegmentxyifnear
 * @recoil-artifact defines .text recoil:function:0x469e90: zGeometry_Vec3::SnapPointToSegmentXYIfNear
 * Purpose: Snap a nearby point onto a segment in XY while preserving Z.
 */
int __fastcall SnapPointToSegmentXYIfNear(
    zVec3 *lineStart,
    zVec3 *lineEnd,
    zVec3 *testPoint,
    float tolerance
) {
    const float dx = lineEnd->x - lineStart->x;
    const float dy = lineEnd->y - lineStart->y;
    const float testDx = testPoint->x - lineStart->x;
    const float testDy = testPoint->y - lineStart->y;

    if (fabs(dx) <= tolerance) {
        if (fabs(testDx) <= tolerance) {
            const float t = testDy / dy;
            if (t > 0.0f && t < 1.0f) {
                testPoint->x = lineStart->x;
                return 1;
            }
        }
    } else if (fabs(dy) <= tolerance) {
        if (fabs(testDy) <= tolerance) {
            const float t = testDx / dx;
            if (t > 0.0f && t < 1.0f) {
                testPoint->y = lineStart->y;
                return 1;
            }
        }
    } else {
        const float tx = testDx / dx;
        const float ty = testDy / dy;
        if (fabs(tx - ty) <= tolerance && tx > 0.0f && ty > 0.0f && tx < 1.0f && ty < 1.0f) {
            const float snappedX = tx * dx + lineStart->x;
            if (fabs(snappedX - testPoint->x) <= tolerance) {
                const float snappedY = ty * dy + lineStart->y;
                if (fabs(snappedY - testPoint->y) <= tolerance) {
                    testPoint->y = snappedY;
                    testPoint->x = snappedX;
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
 * Purpose: Collapse adjacent duplicate XY vertices from a polygon point list.
 */
int __fastcall RemoveAdjacentDuplicatePointsXY(
    zVec3 *vertices,
    int count
) {
    int result = count;
    int index = 0;

    if (result == 0) {
        return result;
    }

    int nextIndex = 1;
    zVec3 *current = vertices;

    while ((unsigned int)(index) < (unsigned int)(result)) {
        if (zGeometry_Vec3::IsNearEqualXY(
            current,
            &vertices[nextIndex % result],
            0.00999999978f
        )) {
            const int lastIndex = result - 1;
            if (index == lastIndex) {
                result = lastIndex;
            } else {
                const int bytesToMove = (result - index - 1) * (int)(sizeof(zVec3));
                memcpy(
                    current,
                    current + 1,
                    bytesToMove
                );
                --index;
                --nextIndex;
                --current;
                --result;
            }
        }

        ++index;
        ++nextIndex;
        ++current;
    }

    return result;
}

} // namespace zGeometry_Vec3Array

namespace zGeometry_Polygon {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-snappointsxyifnear
 * @recoil-artifact defines .text recoil:function:0x46a130: zGeometry_Polygon::SnapPointsXYIfNear
 * Purpose: Snap target polygon points to nearby source vertices or XY edges.
 */
int __fastcall SnapPointsXYIfNear(
    zVec3 *polygon,
    int polyCount,
    zVec3 *targetVerts,
    int targetCount,
    float vertexTolerance,
    float edgeTolerance
) {
    int result = 0;

    for (int i = 0; i < polyCount; ++i) {
        if (targetCount <= 0) {
            continue;
        }

        zVec3 *target = targetVerts;
        zVec3 *const polygonVertex = &polygon[i];
        for (int j = 0; j < targetCount; ++j) {
            if (zGeometry_Vec3::IsNearEqualXY(
                polygonVertex,
                target,
                vertexTolerance
            )) {
                result = 1;
                target->x = polygonVertex->x;
                target->y = polygonVertex->y;
                target->z = polygonVertex->z;
            } else {
                const int nextIndex = (i + 1) % polyCount;
                if (zGeometry_Vec3::SnapPointToSegmentXYIfNear(
                        polygonVertex,
                        &polygon[nextIndex],
                        target,
                        edgeTolerance
                    )) {
                    result = 1;
                }
            }

            ++target;
        }
    }

    return result;
}

} // namespace zGeometry_Polygon

namespace zGeometry_Weiler {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-validatexings
 * @recoil-artifact defines .text recoil:function:0x46a1f0: zGeometry_Weiler::ValidateXings
 * Purpose: Walk the Weiler crossing array, report the first invalid crossing, and return the validation status.
 */
int __fastcall ValidateXings(
    int xingCount,
    zGeometry_WeilerXingPartial *xingArray,
    int *failedXingIndex
) {
    int isValid = 1;
    zGeometry_WeilerXingPartial *xing = xingArray;

    {
        for (int xingIndex = 0; xingIndex < xingCount; ++xingIndex) {
            if (xing == 0) {
                isValid = 0;
            } else {
                const bool hasSegment0 = xing->segment0 != 0;
                const bool hasSegment1 = xing->segment1 != 0;
                const bool hasSegment2 = xing->segment2 != 0;
                const bool hasSegment3 = xing->segment3 != 0;
                const bool hasSegment4 = xing->segment4 != 0;
                const bool hasSegment5 = xing->segment5 != 0;
                const bool hasSegment6 = xing->segment6 != 0;
                const bool hasSegment7 = xing->segment7 != 0;
                const int xingType = xing->xingType;

                if (xingType == 4 || xingType == 5) {
                    isValid =
                        hasSegment0 && hasSegment1 && hasSegment2 && hasSegment3 &&
                                hasSegment4 && hasSegment5 && hasSegment6 && hasSegment7
                            ? 1
                            : 0;
                } else if (xingType == 6) {
                    isValid = hasSegment3 && hasSegment4 && hasSegment6 ? 1 : 0;
                } else if (xingType == 7) {
                    isValid = hasSegment2 && hasSegment3 && hasSegment5 && hasSegment7 ? 1 : 0;
                } else if (xingType == 8) {
                    isValid = hasSegment0 && hasSegment1 && hasSegment4 && hasSegment6 ? 1 : 0;
                } else if (xingType == 9) {
                    isValid = hasSegment0 && hasSegment1 && hasSegment5 && hasSegment7 ? 1 : 0;
                } else if (xingType == 10) {
                    isValid =
                        hasSegment4 && hasSegment6 &&
                                ((hasSegment1 && hasSegment3) ||
                                    (!hasSegment1 && hasSegment0 && hasSegment2))
                            ? 1
                            : 0;
                } else if (xingType == 11) {
                    isValid =
                        hasSegment5 && hasSegment7 &&
                                ((hasSegment1 && hasSegment3) ||
                                    (!hasSegment1 && hasSegment0 && hasSegment2))
                            ? 1
                            : 0;
                } else if (xingType == 12) {
                    isValid = hasSegment2 && hasSegment7 ? 1 : 0;
                } else if (xingType == 13) {
                    isValid = hasSegment0 && hasSegment2 && hasSegment6 && hasSegment7 ? 1 : 0;
                } else if (xingType == 14) {
                    isValid = hasSegment0 && hasSegment6 ? 1 : 0;
                } else if (xingType == 15) {
                    isValid = hasSegment3 && hasSegment6 ? 1 : 0;
                } else if (xingType == 16) {
                    isValid = hasSegment1 && hasSegment3 && hasSegment6 && hasSegment7 ? 1 : 0;
                } else if (xingType == 17) {
                    isValid = hasSegment1 && hasSegment7 ? 1 : 0;
                } else if (xingType == 18) {
                    isValid = hasSegment2 && hasSegment4 ? 1 : 0;
                } else if (xingType == 19) {
                    isValid = hasSegment0 && hasSegment2 && hasSegment4 && hasSegment5 ? 1 : 0;
                } else if (xingType == 20) {
                    isValid = hasSegment0 && hasSegment5 ? 1 : 0;
                } else if (xingType == 21) {
                    isValid = hasSegment3 && hasSegment5 ? 1 : 0;
                } else if (xingType == 22) {
                    isValid = hasSegment1 && hasSegment3 && hasSegment4 && hasSegment5 ? 1 : 0;
                } else if (xingType == 23) {
                    isValid = hasSegment1 && hasSegment4 ? 1 : 0;
                } else if (xingType == 24) {
                    isValid =
                        hasSegment0 && hasSegment2 &&
                                (hasSegment5 || (hasSegment4 && hasSegment6))
                            ? 1
                            : 0;
                } else if (xingType == 25) {
                    isValid =
                        hasSegment1 && hasSegment3 &&
                                (hasSegment5 || (hasSegment4 && hasSegment6))
                            ? 1
                            : 0;
                } else {
                    isValid = 1;
                }
            }
            if (isValid == 0) {
                if (failedXingIndex != 0) {
                    *failedXingIndex = xingIndex;
                }

                if (xing != 0) {
                    zError::ReportOld(
                        0x100,
                        g_zGeometry_SourceFile_ZgeoWeilerCpp,
                        0x1788,
                        g_zGeometry_ValidateXingTypeFmt,
                        xingIndex,
                        xing->xingType
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

                break;
            }

            ++xing;
        }
    }

    return isValid;
}

} // namespace zGeometry_Weiler

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-rotateneg90aroundx
 * @recoil-artifact defines .text recoil:function:0x46a5e0: zGeometry_Vec3Array::RotateNeg90AroundX
 * Purpose: Rotate an array of vectors negative ninety degrees around X.
 */
void __fastcall RotateNeg90AroundX(
    int pointCount,
    zVec3 *points
) {
    if (pointCount == 0) {
        return;
    }

    for (int i = 0; i < pointCount; ++i) {
        const float z = points[i].z;
        points[i].z = -points[i].y;
        points[i].y = z;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-rotatepos90aroundx
 * @recoil-artifact defines .text recoil:function:0x46a600: zGeometry_Vec3Array::RotatePos90AroundX
 * Purpose: Rotate an array of vectors positive ninety degrees around X.
 */
void __fastcall RotatePos90AroundX(
    int pointCount,
    zVec3 *points
) {
    if (pointCount == 0) {
        return;
    }

    for (int i = 0; i < pointCount; ++i) {
        const float y = points[i].y;
        points[i].y = -points[i].z;
        points[i].z = y;
    }
}

} // namespace zGeometry_Vec3Array

namespace zGeometry_Bounds2D {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-weiler-overlapswithunitmargin
 * @recoil-artifact defines .text recoil:function:0x46a620: zGeometry_Bounds2D::OverlapsWithUnitMargin
 * Purpose: Test XY bounds overlap with the retail one-unit margin.
 */
int __fastcall OverlapsWithUnitMargin(
    zGeometry_BoundsXY *boundsA,
    zGeometry_BoundsXY *boundsB
) {
    if (boundsB->maxX + 1.0f < boundsA->minX) {
        return 0;
    }

    if (boundsA->maxX < boundsB->minX - 1.0f) {
        return 0;
    }

    if (boundsA->minY > boundsB->maxY + 1.0f) {
        return 0;
    }

    if (boundsA->maxY < boundsB->minY - 1.0f) {
        return 0;
    }

    return 1;
}

} // namespace zGeometry_Bounds2D

namespace zGeometry_Vec3Array {
} // namespace zGeometry_Vec3Array

namespace zGeometry_ClipPolygon {
} // namespace zGeometry_ClipPolygon

namespace zGeometry_Model {
} // namespace zGeometry_Model
