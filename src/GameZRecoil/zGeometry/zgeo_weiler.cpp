#include "zGeometry.h"

#include "GameZRecoil/zError/zError.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
const char kZGeoWeilerSourceFile[] = "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_weiler.cpp";

const int kIntersect2dCaseIdBySignClass[0x51] = {
    0, 0,  0, 0, 2, 0,  0, 0, 0, 0, 2,  2, 14, 2, 2,  8, 23, 2, 0, 2, 2, 13, 2, 2, 5, 22, 0,
    0, 18, 6, 2, 2, 15, 2, 2, 0, 0, 2,  2, 2,  3, 2,  2, 2,  0, 0, 2, 2, 12, 2, 2, 7, 21, 0,
    0, 19, 4, 2, 2, 16, 2, 2, 0, 2, 20, 9, 2,  2, 17, 2, 2,  0, 0, 0, 0, 0,  2, 0, 0, 0,  0};

const unsigned char kIntersect2dOutputKindByXingType[0x18] = {0, 6, 6, 6, 1, 1, 2, 2, 3, 3, 6, 6,
                                                             4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5};

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

int ClassifyFloatAgainstPivot(float value, float pivot) {
    if (value < pivot) {
        return -1;
    }

    if (value > pivot) {
        return 1;
    }

    return 0;
}

void ComputePointBoundsXY(zVec3 *points, int pointCount, WeilerPointBoundsXY *outBounds) {
    outBounds->minX = points[0].x;
    outBounds->maxX = points[0].x;
    outBounds->minY = points[0].y;
    outBounds->maxY = points[0].y;

    for (int i = 1; i < pointCount; ++i) {
        zVec3 *const point = &points[i];
        if (point->x < outBounds->minX) {
            outBounds->minX = point->x;
        }

        if (point->x > outBounds->maxX) {
            outBounds->maxX = point->x;
        }

        if (point->y < outBounds->minY) {
            outBounds->minY = point->y;
        }

        if (point->y > outBounds->maxY) {
            outBounds->maxY = point->y;
        }
    }
}

void BuildPointSideTable(zVec3 *edgePoints, int edgePointCount, zVec3 *testPoints,
                         int testPointCount, float *table) {
    {
    for (int edgeIndex = 0; edgeIndex < edgePointCount; ++edgeIndex) {
        zVec3 *const edgeStart = &edgePoints[edgeIndex];
        zVec3 *const edgeEnd = &edgePoints[(edgeIndex + 1) % edgePointCount];
        float *const rowStart = table;

        {
        for (int pointIndex = 0; pointIndex < testPointCount; ++pointIndex) {
            zVec3 *const point = &testPoints[pointIndex];
            const float edgeDx = edgeEnd->x - edgeStart->x;
            const float edgeDy = edgeEnd->y - edgeStart->y;
            *table = edgeDy * (point->x - edgeStart->x) - (point->y - edgeStart->y) * edgeDx;
            ++table;
        }
        }

        *table = *rowStart;
        ++table;
    }
    }
}

void TogglePointAxes(zGeometry_WeilerBufferPartial *buffer, int contourSource) {
    zVec3 *point = static_cast<zVec3 *>(buffer->base);
    if (buffer->count == 0) {
        return;
    }

    for (int i = 0; i < buffer->count; ++i) {
        if (contourSource == 2) {
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

void RecenterPoints(zGeometry_WeilerBufferPartial *buffer, float translationX, float translationY) {
    zVec3 *point = static_cast<zVec3 *>(buffer->base);
    for (int i = 0; i < buffer->count; ++i) {
        point[i].x -= translationX;
        point[i].y -= translationY;
    }
}

float EdgeSideXY(zVec3 *point, zVec3 *start, zVec3 *end) {
    const float edgeDeltaX = end->x - start->x;
    const float edgeDeltaY = end->y - start->y;
    return (point->x - start->x) * edgeDeltaY - (point->y - start->y) * edgeDeltaX;
}

int SignClassForIntersectTable(float value) {
    if (value < 0.0f) {
        return 0;
    }

    if (value == 0.0f) {
        return 1;
    }

    return 2;
}

bool HasStrictSameSign(float first, float second) {
    return (first < 0.0f && second < 0.0f) || (first > 0.0f && second > 0.0f);
}

zGeometry_WeilerXingPartial *AllocWeilerXing(zGeometry_WeilerStatePartial *self,
                                             int errorLine) {
    zGeometry_WeilerXingPartial *const xing = static_cast<zGeometry_WeilerXingPartial *>(
        zGeometry_WeilerBuffer::GetAppendSpace(&self->xingBuffer, 1, 0));
    if (xing == 0) {
        fprintf(stderr, "%s %d: _intersect2d call to buf_entry failed\n",
                     kZGeoWeilerSourceFile, errorLine);
    }

    return xing;
}

int ValidateWeilerXingSegmentSet(zGeometry_WeilerXingPartial *xing) {
    if (xing == 0) {
        return 0;
    }

    const bool hasSegment0 = xing->segment0 != 0;
    const bool hasSegment1 = xing->segment1 != 0;
    const bool hasSegment2 = xing->segment2 != 0;
    const bool hasSegment3 = xing->segment3 != 0;
    const bool hasSegment4 = xing->segment4 != 0;
    const bool hasSegment5 = xing->segment5 != 0;
    const bool hasSegment6 = xing->segment6 != 0;
    const bool hasSegment7 = xing->segment7 != 0;

    switch (xing->xingType) {
    case 3:
        return 1;

    case 4:
    case 5:
        return hasSegment0 && hasSegment1 && hasSegment2 && hasSegment3 && hasSegment4 &&
                       hasSegment5 && hasSegment6 && hasSegment7
                   ? 1
                   : 0;

    case 6:
        return hasSegment3 && hasSegment4 && hasSegment6 ? 1 : 0;

    case 7:
        return hasSegment2 && hasSegment3 && hasSegment5 && hasSegment7 ? 1 : 0;

    case 8:
        return hasSegment0 && hasSegment1 && hasSegment4 && hasSegment6 ? 1 : 0;

    case 9:
        return hasSegment0 && hasSegment1 && hasSegment5 && hasSegment7 ? 1 : 0;

    case 10:
        return hasSegment4 && hasSegment6 &&
                       ((hasSegment1 && hasSegment3) ||
                        (!hasSegment1 && hasSegment0 && hasSegment2))
                   ? 1
                   : 0;

    case 11:
        return hasSegment5 && hasSegment7 &&
                       ((hasSegment1 && hasSegment3) ||
                        (!hasSegment1 && hasSegment0 && hasSegment2))
                   ? 1
                   : 0;

    case 12:
        return hasSegment2 && hasSegment7 ? 1 : 0;

    case 13:
        return hasSegment0 && hasSegment2 && hasSegment6 && hasSegment7 ? 1 : 0;

    case 14:
        return hasSegment0 && hasSegment6 ? 1 : 0;

    case 15:
        return hasSegment3 && hasSegment6 ? 1 : 0;

    case 16:
        return hasSegment1 && hasSegment3 && hasSegment6 && hasSegment7 ? 1 : 0;

    case 17:
        return hasSegment1 && hasSegment7 ? 1 : 0;

    case 18:
        return hasSegment2 && hasSegment4 ? 1 : 0;

    case 19:
        return hasSegment0 && hasSegment2 && hasSegment4 && hasSegment5 ? 1 : 0;

    case 20:
        return hasSegment0 && hasSegment5 ? 1 : 0;

    case 21:
        return hasSegment3 && hasSegment5 ? 1 : 0;

    case 22:
        return hasSegment1 && hasSegment3 && hasSegment4 && hasSegment5 ? 1 : 0;

    case 23:
        return hasSegment1 && hasSegment4 ? 1 : 0;

    case 24:
        return hasSegment0 && hasSegment2 && (hasSegment5 || (hasSegment4 && hasSegment6)) ? 1 : 0;

    case 25:
        return hasSegment1 && hasSegment3 && (hasSegment5 || (hasSegment4 && hasSegment6)) ? 1 : 0;

    default:
        return 1;
    }
}

void ReportMergeContourOutputFailure(int errorLine) {
    fprintf(stderr, "%s %d: _merge_contours failed to receive new contour.\n",
                 kZGeoWeilerSourceFile, errorLine);
}

int EnsureMergedContourOutput(zGeometry_WeilerStatePartial *self,
                                       zGeometry_WeilerContourSegmentPartial *segment,
                                       int errorLine) {
    if (zGeometry_Weiler::EnsureContourOutput(self, segment) == 0) {
        ReportMergeContourOutputFailure(errorLine);
        return 0;
    }

    return 1;
}

int EnsureMergedContourOutputs(zGeometry_WeilerStatePartial *self,
                                        zGeometry_WeilerContourSegmentPartial *firstSegment,
                                        zGeometry_WeilerContourSegmentPartial *secondSegment,
                                        int errorLine) {
    if (zGeometry_Weiler::EnsureContourOutput(self, firstSegment) == 0 ||
        zGeometry_Weiler::EnsureContourOutput(self, secondSegment) == 0) {
        ReportMergeContourOutputFailure(errorLine);
        return 0;
    }

    return 1;
}

bool IsNearZeroForCoincidentWeedOut(float value) {
    return fabs(static_cast<double>(value)) < 0.0000099999997473787516;
}

bool IsNearPointXY(zVec3 *first, zVec3 *second) {
    return fabs(static_cast<double>(first->x) - static_cast<double>(second->x)) <=
               0.0010000000474974513 &&
           fabs(static_cast<double>(first->y) - static_cast<double>(second->y)) <=
               0.0010000000474974513;
}

bool HaveSameDirectionXY(zVec3 *firstStart, zVec3 *firstEnd, zVec3 *secondStart, zVec3 *secondEnd) {
    const float firstDeltaX = firstEnd->x - firstStart->x;
    const float secondDeltaX = secondEnd->x - secondStart->x;

    if (firstDeltaX != 0.0f && secondDeltaX != 0.0f) {
        return (firstDeltaX < 0.0f) == (secondDeltaX < 0.0f);
    }

    return (firstEnd->y < firstStart->y) == (secondEnd->y < secondStart->y);
}

void ReportWeedOutInsideAFailure(int errorLine) {
    zError::ReportOld(0x100, kZGeoWeilerSourceFile, errorLine, "WeedOut Error: %s",
                      "B_COMPLETELY_INSIDE_A");
}

void ReportWeedOutForwardSegmentFailure(int errorLine) {
    zError::ReportOld(0x100, kZGeoWeilerSourceFile, errorLine, "WeedOut Error: %s",
                      "Forward Segment Failed");
}

void ReportWeedOutSegForwardFailure(int errorLine) {
    fprintf(stderr, "%s %d: _weed_out_coincident call to segForward failed.\n",
                 kZGeoWeilerSourceFile, errorLine);
}

void UnlinkWeilerContourSegmentPair(WeilerPreclassifyContourPacket *contourPacket,
                                    zGeometry_WeilerContourSegmentPartial *originalContourC,
                                    zGeometry_WeilerContourSegmentPartial *contourCWalker,
                                    zGeometry_WeilerContourSegmentPartial *contourDWalker) {
    if (contourCWalker == originalContourC) {
        contourPacket->contourC.firstSegment = contourCWalker->next;
        contourPacket->contourD.firstSegment = contourDWalker->next;
        contourCWalker->next->contourOutput = &contourPacket->contourC;
        contourDWalker->next->contourOutput = &contourPacket->contourD;
    }

    contourCWalker->prev->next = contourCWalker->next;
    contourCWalker->next->prev = contourCWalker->prev;
    contourDWalker->prev->next = contourDWalker->next;
    contourDWalker->next->prev = contourDWalker->prev;
}

void ClearWeilerXingSegments(zGeometry_WeilerXingPartial *xing) {
    if (xing == 0) {
        return;
    }

    xing->segment6 = 0;
    xing->segment7 = 0;
    xing->segment4 = 0;
    xing->segment5 = 0;
    xing->segment2 = 0;
    xing->segment3 = 0;
    xing->segment0 = 0;
    xing->segment1 = 0;
}

bool SegmentBoundsOverlapXY(zGeometry_WeilerContourSegmentPartial *first,
                            zGeometry_WeilerContourSegmentPartial *second) {
    if (first->boundsDirty != 0) {
        zGeometry_WeilerContourSegment::UpdateBounds(first);
    }

    if (second->boundsDirty != 0) {
        zGeometry_WeilerContourSegment::UpdateBounds(second);
    }

    return first->minX <= second->maxX && first->maxX >= second->minX &&
           first->minY <= second->maxY && first->maxY >= second->minY;
}

bool XingTouchesContourPair(zGeometry_WeilerXingPartial *xing,
                            zGeometry_WeilerContourSegmentPartial *firstSegment,
                            zGeometry_WeilerContourSegmentPartial *secondSegment) {
    return xing == firstSegment->startXing || xing == firstSegment->endXing ||
           xing == secondSegment->startXing || xing == secondSegment->endXing;
}

bool HasExistingXingWithContourPair(zGeometry_WeilerContourSegmentPartial *contourCSegment,
                                    zGeometry_WeilerContourSegmentPartial *contourDSegment,
                                    zGeometry_WeilerContourSegmentPartial *contourASegment,
                                    zGeometry_WeilerContourSegmentPartial *contourBSegment) {
    return (contourCSegment->startXing != 0 &&
            XingTouchesContourPair(contourCSegment->startXing, contourASegment, contourBSegment)) ||
           (contourCSegment->endXing != 0 &&
            XingTouchesContourPair(contourCSegment->endXing, contourASegment, contourBSegment)) ||
           (contourDSegment->startXing != 0 &&
            XingTouchesContourPair(contourDSegment->startXing, contourASegment, contourBSegment)) ||
           (contourDSegment->endXing != 0 &&
            XingTouchesContourPair(contourDSegment->endXing, contourASegment, contourBSegment));
}

void ReportWeilerIntersectError(int errorLine) {
    zError::ReportOld(0x100, kZGeoWeilerSourceFile, errorLine, "weilerIntersect Error: %s", "");
}

void ReportWeilerIntersectDivideFailure(int errorLine) {
    fprintf(stderr, "%s %d: _weiler_intersect call to _divide_edge failed.\n",
                 kZGeoWeilerSourceFile, errorLine);
}

int DivideContainedContourSegment(zGeometry_WeilerStatePartial *self,
                                           zGeometry_WeilerXingPartial *xing,
                                           zGeometry_WeilerContourSegmentPartial *segment,
                                           int updateSplitLinks, int errorLine) {
    if (zGeometry_Weiler::DivideContourSegmentAtPoint(self, &xing->point, segment,
                                                      updateSplitLinks) == 0) {
        ReportWeilerIntersectDivideFailure(errorLine);
        return 0;
    }

    return 1;
}

int DivideContainedContourPairAtXing(
    zGeometry_WeilerStatePartial *self, zGeometry_WeilerXingPartial *xing,
    zGeometry_WeilerContourSegmentPartial *contourASegment,
    zGeometry_WeilerContourSegmentPartial *contourBSegment,
    zGeometry_WeilerContourSegmentPartial *contourCSegment,
    zGeometry_WeilerContourSegmentPartial *contourDSegment, int errorLine) {
    return DivideContainedContourSegment(self, xing, contourASegment, 1, errorLine) &&
                   DivideContainedContourSegment(self, xing, contourBSegment, 1, errorLine) &&
                   DivideContainedContourSegment(self, xing, contourCSegment, 1, errorLine) &&
                   DivideContainedContourSegment(self, xing, contourDSegment, 1, errorLine)
               ? 1
               : 0;
}

void PropagateXingBackReferencesForContourPair(
    zGeometry_WeilerContourSegmentPartial *contourAStart,
    zGeometry_WeilerContourSegmentPartial *contourBStart,
    zGeometry_WeilerContourSegmentPartial *contourCStart,
    zGeometry_WeilerContourSegmentPartial *contourDStart) {
    zGeometry_WeilerContourSegmentPartial *contourASegment = contourAStart;
    zGeometry_WeilerContourSegmentPartial *contourBSegment = contourBStart;

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
    } while (contourASegment != contourAStart);

    zGeometry_WeilerContourSegmentPartial *contourCSegment = contourCStart;
    zGeometry_WeilerContourSegmentPartial *contourDSegment = contourDStart;

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
    } while (contourCSegment != contourCStart);
}

bool SelectMaxXThenMaxY(zVec3 *candidate, zVec3 *current) {
    if (candidate->x > current->x) {
        return true;
    }

    return fabs(static_cast<double>(candidate->x) - static_cast<double>(current->x)) <
               0.0000099999997473787516 &&
           candidate->y > current->y;
}

void CopyPointAndAdvance(zVec3 *&outPoint, zVec3 *point) {
    *outPoint = *point;
    ++outPoint;
}

int
ClassifyPointAgainstAdjacentEdgePair(zVec3 *point,
                                     zGeometry_WeilerContourSegmentPartial *firstSegment,
                                     zGeometry_WeilerContourSegmentPartial *secondSegment) {
    const bool firstSideNegative =
        EdgeSideXY(point, firstSegment->startPoint, firstSegment->endPoint) < 0.0f;
    const bool secondSideNegative =
        EdgeSideXY(point, secondSegment->startPoint, secondSegment->endPoint) < 0.0f;

    if (firstSideNegative && secondSideNegative) {
        return 1;
    }

    if (firstSideNegative != secondSideNegative &&
        EdgeSideXY(secondSegment->endPoint, firstSegment->startPoint, firstSegment->endPoint) >
            0.0f) {
        return 1;
    }

    return -1;
}

bool ArePointsStrictlyNegativeToAdjacentEdgePair(
    zVec3 *firstPoint, zVec3 *secondPoint, zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment) {
    return EdgeSideXY(firstPoint, firstSegment->startPoint, firstSegment->endPoint) < 0.0f &&
           EdgeSideXY(firstPoint, secondSegment->startPoint, secondSegment->endPoint) < 0.0f &&
           EdgeSideXY(secondPoint, firstSegment->startPoint, firstSegment->endPoint) < 0.0f &&
           EdgeSideXY(secondPoint, secondSegment->startPoint, secondSegment->endPoint) < 0.0f;
}
} // namespace

namespace zGeometry_Segment {
// Reimplements 0x46be20: zGeometry_Segment::IntersectsSegmentXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IntersectsSegmentXY(zVec3 *segmentAPoint0,
                                                                 zVec3 *segmentAPoint1,
                                                                 zVec3 *segmentBPoint0,
                                                                 zVec3 *segmentBPoint1) {
    const float segmentAX = segmentAPoint1->x - segmentAPoint0->x;
    const float segmentAY = segmentAPoint1->y - segmentAPoint0->y;
    const float segmentBX = segmentBPoint1->x - segmentBPoint0->x;
    const float segmentBY = segmentBPoint1->y - segmentBPoint0->y;
    const float determinant = segmentAX * segmentBY - segmentAY * segmentBX;

    if (determinant == 0.0f) {
        return 0;
    }

    const float originDeltaX = segmentBPoint0->x - segmentAPoint0->x;
    const float originDeltaY = segmentBPoint0->y - segmentAPoint0->y;
    const float segmentAParameter =
        (originDeltaX * segmentBY - originDeltaY * segmentBX) / determinant;
    const float segmentBParameter =
        (originDeltaX * segmentAY - originDeltaY * segmentAX) / determinant;

    if (segmentAParameter >= 0.0f && segmentBParameter >= 0.0f && segmentAParameter < 1.0f &&
        segmentBParameter < 1.0f) {
        return 1;
    }

    return 0;
}
} // namespace zGeometry_Segment

namespace zGeometry_Model {
// Reimplements 0x46b650:
// zGeometry_Model::GetLinearBufferOfPolygonVertices
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE zVec3 *RECOIL_FASTCALL GetLinearBufferOfPolygonVertices(
    zModel_DrawBatchBasePartial *model, zModel_PolygonPartial *polygon, zVec3 *points) {
    const unsigned int vertexCount = polygon->vertexCountAndFlags & 0xff;
    zVec3 *result = static_cast<zVec3 *>(realloc(points, vertexCount * sizeof(zVec3)));

    for (unsigned int i = 0; i < vertexCount; ++i) {
        const int vertexIndex = polygon->vertexIndices[i];
        result[i] = model->verts[vertexIndex];
    }

    return result;
}
} // namespace zGeometry_Model

namespace zGeometry_Vec3 {
// Reimplements 0x469ca0: zGeometry_Vec3::IsBetweenEndpointsXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IsBetweenEndpointsXY(zVec3 *testPoint,
                                                                  zVec3 *startPoint,
                                                                  zVec3 *endPoint) {
    if (fabs(static_cast<double>(startPoint->x) - static_cast<double>(endPoint->x)) <
        0.0000099999997473787516) {
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

namespace zGeometry_Vec3Array {
// Reimplements 0x46a080:
// zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveAdjacentDuplicatePointsXY(zVec3 *vertices,
                                                                             int count) {
    int result = count;
    int index = 0;

    if (result == 0) {
        return result;
    }

    int nextIndex = 1;
    zVec3 *current = vertices;

    while (static_cast<unsigned int>(index) < static_cast<unsigned int>(result)) {
        if (zGeometry_Vec3::IsNearEqualXY(current, &vertices[nextIndex % result], 0.00999999978f)) {
            const int lastIndex = result - 1;
            if (index == lastIndex) {
                result = lastIndex;
            } else {
                const int bytesToMove =
                    (result - index - 1) * static_cast<int>(sizeof(zVec3));
                memcpy(current, current + 1, bytesToMove);
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

// Reimplements 0x46a600: zGeometry_Vec3Array::RotatePos90AroundX
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RotatePos90AroundX(int pointCount, zVec3 *points) {
    if (pointCount == 0) {
        return;
    }

    for (int i = 0; i < pointCount; ++i) {
        const float y = points[i].y;
        points[i].y = -points[i].z;
        points[i].z = y;
    }
}

// Reimplements 0x46a9c0: zGeometry_Vec3Array::ComputeBoundsXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ComputeBoundsXY(zGeometry_BoundsXY *outBounds, zVec3 *points,
                                                     int pointCount) {
    outBounds->minX = points[0].x;
    outBounds->maxX = points[0].x;
    outBounds->maxY = points[0].y;
    outBounds->minY = points[0].y;

    for (int i = 1; i < pointCount; ++i) {
        zVec3 *const point = &points[i];
        if (point->x < outBounds->minX) {
            outBounds->minX = point->x;
        }

        if (point->x > outBounds->maxX) {
            outBounds->maxX = point->x;
        }

        if (point->y > outBounds->maxY) {
            outBounds->maxY = point->y;
        }

        if (point->y < outBounds->minY) {
            outBounds->minY = point->y;
        }
    }
}
} // namespace zGeometry_Vec3Array

namespace zGeometry_WeilerBuffer {
// Reimplements 0x467600: zGeometry_WeilerBuffer::Init
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL Init(zGeometry_WeilerBufferPartial *self,
                                          int initialCapacity, int elementSize) {
    void *const base = calloc(initialCapacity, elementSize);
    self->capacity = initialCapacity;
    self->base = base;
    self->elementSize = elementSize;
    self->count = 0;
    self->appendPtr = base;
}

// Reimplements 0x467660: zGeometry_WeilerBuffer::GetAppendSpace
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void *RECOIL_FASTCALL GetAppendSpace(zGeometry_WeilerBufferPartial *self,
                                                     int appendCount, void **outBase) {
    const int newCount = self->count + appendCount;
    if (static_cast<unsigned int>(newCount) >= static_cast<unsigned int>(self->capacity)) {
        self->capacity += appendCount + 0x10;
        void *const base = realloc(self->base, self->capacity * self->elementSize);
        self->base = base;
        self->appendPtr = reinterpret_cast<void *>(reinterpret_cast<unsigned int>(base) +
                                                   self->elementSize * self->count);

        if (outBase != 0) {
            *outBase = base;
        }
    }

    void *const result = self->appendPtr;
    self->count = newCount;
    self->appendPtr = reinterpret_cast<void *>(reinterpret_cast<unsigned int>(self->appendPtr) +
                                               appendCount * self->elementSize);
    return result;
}

// Reimplements 0x469ae0: zGeometry_WeilerBuffer::SetCountAndAppendPtr
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetCountAndAppendPtr(zGeometry_WeilerBufferPartial *self,
                                                          int count) {
    self->count = count;
    self->appendPtr = reinterpret_cast<void *>(reinterpret_cast<unsigned int>(self->base) +
                                               count * self->elementSize);
}

// Reimplements 0x467630: zGeometry_WeilerBuffer::Destroy
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_WeilerBufferPartial *self) {
    if (self->base != 0) {
        free(self->base);
        self->capacity = 0;
        self->count = 0;
        self->elementSize = 0;
        self->base = 0;
        self->appendPtr = 0;
    }
}
} // namespace zGeometry_WeilerBuffer

namespace zGeometry_WeilerContourSegment {
// Reimplements 0x468410:
// zGeometry_WeilerContourSegment::UpdateBounds
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateBounds(zGeometry_WeilerContourSegmentPartial *segment) {
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

namespace zGeometry_WeilerContourSegmentArray {
// Reimplements 0x4693a0:
// zGeometry_WeilerContourSegmentArray::UpdateBounds
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateBounds(zGeometry_WeilerContourSegmentPartial *segments,
                                                  int segmentCount) {
    for (int i = 0; i < segmentCount; ++i) {
        zGeometry_WeilerContourSegment::UpdateBounds(&segments[i]);
    }
}

// Reimplements 0x4693c0:
// zGeometry_WeilerContourSegmentArray::InitFromPointList
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
InitFromPointList(zGeometry_WeilerContourSegmentPartial *segments, zVec3 *points,
                  int pointCount, int contourType) {
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
// Reimplements 0x464670: zGeometry_Weiler::GetInputContourAPointList
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
GetInputContourAPointList(zGeometry_WeilerStatePartial *self, zVec3 **outPoints) {
    if (self == 0) {
        return 0;
    }

    *outPoints = static_cast<zVec3 *>(self->inputContourABuffer.base);
    return self->inputContourABuffer.count;
}

// Reimplements 0x464680: zGeometry_Weiler::Init
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE zGeometry_WeilerStatePartial *RECOIL_FASTCALL Init(zVec3 *points,
                                                                   int pointCount,
                                                                   int contourSource) {
    if (pointCount == 0 || points == 0) {
        zError::ReportOld(0x200, kZGeoWeilerSourceFile, 0x20d,
                          "Bad clip region passed to Weiler Clip.");
    }

    const int dedupedPointCount =
        zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY(points, pointCount);

    zGeometry_WeilerStatePartial *const result = static_cast<zGeometry_WeilerStatePartial *>(
        calloc(1, sizeof(zGeometry_WeilerStatePartial)));

    zGeometry_WeilerBuffer::Init(&result->segmentBuffer, 0x80,
                                 sizeof(zGeometry_WeilerContourSegmentPartial));
    zGeometry_WeilerBuffer::Init(&result->contourBuffer, 0x80, 0x0c);
    zGeometry_WeilerBuffer::Init(&result->xingBuffer, 0x80, 0x30);
    zGeometry_WeilerBuffer::Init(&result->inputContourABuffer, dedupedPointCount, sizeof(zVec3));

    const size_t pointBytes = static_cast<size_t>(dedupedPointCount) * sizeof(zVec3);
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
        zError::ReportOld(0x200, kZGeoWeilerSourceFile, 0x24c,
                          "weiler_init call to weilerInit failed.");
        zGeometry_Weiler::DestroyState(result);
        return 0;
    }

    return result;
}

// Reimplements 0x464b90: zGeometry_Weiler::InitInputContourPair
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
InitInputContourPair(zGeometry_WeilerStatePartial *self, zVec3 *points, int pointCount,
                     int contourType) {
    zGeometry_WeilerContourSegmentPartial *segments =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(
            zGeometry_WeilerBuffer::GetAppendSpace(&self->segmentBuffer, pointCount * 2, 0));
    if (segments == 0) {
        fprintf(stderr, "%s %d: weilerInit call to bufEntry failed.\n", kZGeoWeilerSourceFile,
                     0x455);
        return 0;
    }

    zGeometry_WeilerContourSegmentArray::InitFromPointList(segments, points, pointCount,
                                                           contourType);
    segments->contourOutput = 0;
    if (zGeometry_Weiler::EnsureContourOutput(self, segments) == 0) {
        fprintf(stderr, "%s %d: weilerInit call to _new_contour failed.\n",
                     kZGeoWeilerSourceFile, 0x468);
        return 0;
    }

    zGeometry_WeilerContourSegmentArray::UpdateBounds(segments, pointCount);

    zGeometry_WeilerContourSegmentPartial *const reverseSegments = &segments[pointCount];
    zGeometry_WeilerContourSegmentArray::InitFromPointList(reverseSegments, points, pointCount, 4);
    reverseSegments->contourOutput = 0;
    if (zGeometry_Weiler::EnsureContourOutput(self, reverseSegments) == 0) {
        fprintf(stderr, "%s %d: weilerInit call to _new_contour failed.\n",
                     kZGeoWeilerSourceFile, 0x485);
        return 0;
    }

    return 1;
}

// Reimplements 0x464810: zGeometry_Weiler::ClipPointList
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPointList(zGeometry_WeilerStatePartial *self, int clipMode, zVec3 *points,
              int pointCount, zGeometry_WeilerClipOutputPartial *outClip) {
    if (self == 0 || self->inputContourABuffer.count == 0 || pointCount == 0 ||
        points == 0 || outClip == 0) {
        fprintf(stderr, "%s %d: Bad parameter(s) passed to Weiler Clip.\n",
                     kZGeoWeilerSourceFile, 0x2a0);
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

    zGeometry_WeilerBuffer::Init(&self->polygonSetABuffer, 0x80,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&self->polygonSetBBuffer, 0x80,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&self->polygonSetCBuffer, 0x80,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&self->pointListBuffer, 0x80, sizeof(zVec3));

    self->outClip = outClip;
    outClip->polygonSetA.polygonCount = 0;
    outClip->polygonSetA.polygons =
        static_cast<zGeometry_PolygonPointSpanPartial *>(self->polygonSetABuffer.base);
    outClip->polygonSetB.polygonCount = 0;
    outClip->polygonSetB.polygons =
        static_cast<zGeometry_PolygonPointSpanPartial *>(self->polygonSetBBuffer.base);
    outClip->polygonSetC.polygonCount = 0;
    outClip->polygonSetC.polygons =
        static_cast<zGeometry_PolygonPointSpanPartial *>(self->polygonSetCBuffer.base);
    outClip->pointList.pointCount = 0;
    outClip->pointList.points = static_cast<zVec3 *>(self->pointListBuffer.base);

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
    if (zGeometry_Weiler::InitInputContourPair(
            self, static_cast<zVec3 *>(self->inputContourBBuffer.base), pointCount, 2) == 0) {
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
                fprintf(stderr, "%s %d: weiler_clip call to gatherContours failed.\n",
                             kZGeoWeilerSourceFile, 0x3b2);
                zGeometry_WeilerClipOutput::Destroy(outClip);
                if (self->pointsRecentered) {
                    zGeometry_Weiler::RestorePointTranslation(self);
                }

                if (self->contourSource != 0) {
                    zGeometry_Weiler::TogglePointAxesForContourSource(self);
                }

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
            zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(self, 4) == 0) {
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

// Reimplements 0x4676c0: zGeometry_Weiler::EnsureContourOutput
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL EnsureContourOutput(
    zGeometry_WeilerStatePartial *self, zGeometry_WeilerContourSegmentPartial *segment) {
    if (segment->contourOutput == 0) {
        zGeometry_WeilerContourOutputPartial *const contourOutput =
            static_cast<zGeometry_WeilerContourOutputPartial *>(
                zGeometry_WeilerBuffer::GetAppendSpace(&self->contourBuffer, 1, 0));

        if (contourOutput == 0) {
            zError::ReportOld(0x200, kZGeoWeilerSourceFile, 0xc6f,
                              "New_contour could not obtain buffer entry");
            return 0;
        }

        contourOutput->firstSegment = segment;
        contourOutput->contourType = segment->contourType;
        segment->contourOutput = contourOutput;
    }

    return 1;
}

// Reimplements 0x467710: zGeometry_Weiler::MergeContours
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL MergeContours(zGeometry_WeilerStatePartial *self) {
    zGeometry_WeilerXingPartial *const xingBase =
        static_cast<zGeometry_WeilerXingPartial *>(self->xingBuffer.base);
    if (zGeometry_Weiler::ValidateXings(self->xingBuffer.count, xingBase, 0) == 0) {
        zError::ReportOld(0x100, kZGeoWeilerSourceFile, 0xc9a,
                          "contourMerge:  Failed validation\n");
        return 0;
    }

    zGeometry_WeilerXingPartial *xing = xingBase;
    for (unsigned int xingIndex = 0;
         xingIndex < static_cast<unsigned int>(self->xingBuffer.count); ++xingIndex, ++xing) {
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

            if (EnsureMergedContourOutputs(self, segment4, segment6, 0xcc5) == 0) {
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

            if (EnsureMergedContourOutputs(self, segment4, segment6, 0xce0) == 0) {
                return 0;
            }
            break;

        case 3:
            segment2->prev = segment4;
            segment3->prev = segment6;
            segment4->next = segment2;
            segment6->prev = segment3;

            if (EnsureMergedContourOutputs(self, segment4, segment6, 0xe28) == 0) {
                return 0;
            }
            break;

        case 4:
            segment2->prev = segment7;
            segment3->prev = segment5;
            segment5->next = segment3;
            segment7->prev = segment2;

            if (EnsureMergedContourOutputs(self, segment5, segment6, 0xe3d) == 0) {
                return 0;
            }
            break;

        case 5:
            segment0->next = segment6;
            segment1->next = segment4;
            segment4->next = segment1;
            segment6->prev = segment0;

            if (EnsureMergedContourOutputs(self, segment4, segment6, 0xe52) == 0) {
                return 0;
            }
            break;

        case 6:
            segment0->next = segment5;
            segment1->next = segment7;
            segment5->next = segment0;
            segment7->prev = segment1;

            if (EnsureMergedContourOutputs(self, segment0, segment7, 0xe67) == 0) {
                return 0;
            }
            break;

        case 7:
            if (segment1 != 0) {
                segment1->next = segment4;
                segment3->prev = segment6;
                segment4->next = segment1;
                segment6->prev = segment3;

                if (EnsureMergedContourOutputs(self, segment1, segment3, 0xe7d) == 0) {
                    return 0;
                }
            } else {
                segment0->next = segment6;
                segment2->prev = segment4;
                segment4->next = segment2;
                segment6->prev = segment0;

                if (EnsureMergedContourOutputs(self, segment0, segment2, 0xe8d) == 0) {
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

                if (EnsureMergedContourOutputs(self, segment1, segment3, 0xea4) == 0) {
                    return 0;
                }
            } else {
                segment0->next = segment7;
                segment7->prev = segment0;
                segment2->prev = segment5;
                segment5->next = segment2;

                if (EnsureMergedContourOutputs(self, segment0, segment2, 0xeb4) == 0) {
                    return 0;
                }
            }
            break;

        case 9:
            segment2->prev = segment7;
            segment7->prev = segment2;

            if (EnsureMergedContourOutput(self, segment2, 0xd95) == 0) {
                return 0;
            }
            break;

        case 10:
            segment0->next = segment6;
            segment2->prev = segment7;
            segment6->prev = segment0;
            segment7->prev = segment2;

            if (EnsureMergedContourOutputs(self, segment0, segment2, 0xcf5) == 0) {
                return 0;
            }
            break;

        case 11:
            segment0->next = segment6;
            segment6->prev = segment0;

            if (EnsureMergedContourOutput(self, segment0, 0xda7) == 0) {
                return 0;
            }
            break;

        case 12:
            segment3->prev = segment6;
            segment6->prev = segment3;

            if (EnsureMergedContourOutput(self, segment6, 0xdb9) == 0) {
                return 0;
            }
            break;

        case 13:
            segment1->next = segment7;
            segment3->prev = segment6;
            segment6->prev = segment3;
            segment7->prev = segment1;

            if (EnsureMergedContourOutputs(self, segment1, segment6, 0xd0a) == 0) {
                return 0;
            }
            break;

        case 14:
            segment1->next = segment7;
            segment7->prev = segment1;

            if (EnsureMergedContourOutput(self, segment1, 0xdcb) == 0) {
                return 0;
            }
            break;

        case 15:
            segment2->prev = segment4;
            segment4->next = segment2;

            if (EnsureMergedContourOutput(self, segment2, 0xddd) == 0) {
                return 0;
            }
            break;

        case 16:
            segment0->next = segment5;
            segment2->prev = segment4;
            segment4->next = segment2;
            segment5->next = segment0;

            if (EnsureMergedContourOutputs(self, segment0, segment2, 0xd1f) == 0) {
                return 0;
            }
            break;

        case 17:
            segment0->next = segment5;
            segment5->next = segment0;

            if (EnsureMergedContourOutput(self, segment0, 0xdef) == 0) {
                return 0;
            }
            break;

        case 18:
            segment3->prev = segment5;
            segment5->next = segment3;

            if (EnsureMergedContourOutput(self, segment3, 0xe01) == 0) {
                return 0;
            }
            break;

        case 19:
            segment1->next = segment4;
            segment3->prev = segment5;
            segment4->next = segment1;
            segment5->next = segment3;

            if (EnsureMergedContourOutputs(self, segment4, segment7, 0xd34) == 0) {
                return 0;
            }
            break;

        case 20:
            segment1->next = segment4;
            segment4->next = segment1;

            if (EnsureMergedContourOutput(self, segment4, 0xe13) == 0) {
                return 0;
            }
            break;

        case 21:
            if (segment5 != 0) {
                segment0->next = segment5;
                segment2->prev = segment7;
                segment5->next = segment0;
                segment7->prev = segment2;

                if (EnsureMergedContourOutputs(self, segment5, segment7, 0xd4a) == 0) {
                    return 0;
                }
            } else {
                segment0->next = segment6;
                segment2->prev = segment4;
                segment4->next = segment2;
                segment6->prev = segment0;

                if (EnsureMergedContourOutputs(self, segment4, segment6, 0xd5a) == 0) {
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

                if (EnsureMergedContourOutputs(self, segment5, segment7, 0xd71) == 0) {
                    return 0;
                }
            } else {
                segment4->next = segment3;
                segment3->prev = segment4;
                segment6->prev = segment1;
                segment1->next = segment6;

                if (EnsureMergedContourOutputs(self, segment4, segment6, 0xd82) == 0) {
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

// Reimplements 0x468580:
// zGeometry_Weiler::DivideContourSegmentAtPoint
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL DivideContourSegmentAtPoint(
    zGeometry_WeilerStatePartial *self, zVec3 *xing, zGeometry_WeilerContourSegmentPartial *segment,
    int updateSplitLinks) {
    zGeometry_WeilerContourSegmentPartial *nextSegment;

    if (zGeometry_Vec3::IsNearEqualXY(segment->endPoint, xing, 0.00100000005f) != 0) {
        nextSegment = segment->next;
    } else {
        nextSegment = static_cast<zGeometry_WeilerContourSegmentPartial *>(
            zGeometry_WeilerBuffer::GetAppendSpace(&self->segmentBuffer, 1, 0));
        if (nextSegment == 0) {
            fprintf(stderr, "%s %d: _divide_edge call to bufEntry failed.\n",
                         kZGeoWeilerSourceFile, 0x113a);
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

    zGeometry_WeilerXingPartial *const xingLink =
        reinterpret_cast<zGeometry_WeilerXingPartial *>(xing);
    if (updateSplitLinks != 0) {
        segment->endXing = xingLink;
        nextSegment->startXing = xingLink;
    } else {
        nextSegment->startXing = 0;
    }

    return 1;
}

// Reimplements 0x468650:
// zGeometry_Weiler::CreateForwardSegmentPairAtPoint
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CreateForwardSegmentPairAtPoint(
    zGeometry_WeilerStatePartial *self, zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment, zVec3 *point,
    int firstContourTypeMask, int secondContourTypeMask) {
    zGeometry_WeilerContourSegmentPartial *segment = firstSegment;
    int contourTypeMask = firstContourTypeMask;

    for (int i = 0; i < 2; ++i) {
        zGeometry_WeilerContourSegmentPartial *const newSegment =
            static_cast<zGeometry_WeilerContourSegmentPartial *>(
                zGeometry_WeilerBuffer::GetAppendSpace(&self->segmentBuffer, 1, 0));
        if (newSegment == 0) {
            zError::ReportOld(0x200, kZGeoWeilerSourceFile, 0x1181, "bufEntry failed");
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

// Reimplements 0x469430:
// zGeometry_Weiler::GetNextContourSegmentForTraversal
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE zGeometry_WeilerContourSegmentPartial *RECOIL_FASTCALL
GetNextContourSegmentForTraversal(zGeometry_WeilerContourSegmentPartial *segment) {
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

// Reimplements 0x4680b0: zGeometry_Weiler::NewContour
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL NewContour(zGeometry_WeilerStatePartial *self) {
    int contourCount = self->contourBuffer.count;
    zGeometry_WeilerContourOutputPartial *contour =
        static_cast<zGeometry_WeilerContourOutputPartial *>(self->contourBuffer.base);

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

// Reimplements 0x468a10:
// zGeometry_Weiler::ClassifyPointInContourPointListXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ClassifyPointInContourPointListXY(
    zVec3 *point, int contourPointCount, zVec3 *contourPoints) {
    if (contourPointCount <= 0) {
        return -1;
    }

    zVec3 *previous = &contourPoints[contourPointCount - 1];
    int previousXSide = ClassifyFloatAgainstPivot(previous->x, point->x);
    int previousYSide = ClassifyFloatAgainstPivot(previous->y, point->y);

    if (previousXSide == 0 && previousYSide == 0) {
        return 0;
    }

    int crossingParity = 0;

    for (int i = 0; i < contourPointCount; ++i) {
        zVec3 *const current = &contourPoints[i];
        const int currentXSide = ClassifyFloatAgainstPivot(current->x, point->x);
        const int currentYSide = ClassifyFloatAgainstPivot(current->y, point->y);

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
                    ClassifyFloatAgainstPivot(xIntersection, point->x);

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

// Reimplements 0x464ea0:
// zGeometry_Weiler::OutputPreclassifiedContourPairResult
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL OutputPreclassifiedContourPairResult(
    int contourAPointCount, zVec3 *contourAPoints, int contourBPointCount,
    zVec3 *contourBPoints, int resultCode) {
    if (resultCode == 2 && contourAPointCount == contourBPointCount) {
        bool allPointsMatched = true;
        zVec3 *contourBPoint = contourBPoints;

        for (int i = contourBPointCount; i != 0; --i) {
            bool pointMatched = false;
            zVec3 *contourAPoint = contourAPoints;

            for (int j = contourAPointCount; j != 0; --j) {
                if (fabs(static_cast<double>(contourAPoint->x) -
                              static_cast<double>(contourBPoint->x)) <= 0.0010000000474974513 &&
                    fabs(static_cast<double>(contourAPoint->y) -
                              static_cast<double>(contourBPoint->y)) <= 0.0010000000474974513) {
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
        if (zGeometry_Weiler::ClassifyPointInContourPointListXY(contourAPoint, contourBPointCount,
                                                                contourBPoints) < 0) {
            return 0;
        }

        ++contourAPoint;
    }

    return resultCode;
}

// Reimplements 0x464c90:
// zGeometry_Weiler::ClassifyInputContourPairBounds
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ClassifyInputContourPairBounds(zGeometry_WeilerStatePartial *self) {
    const int inputPointCountA = self->inputContourABuffer.count;
    const int inputPointCountB = self->inputContourBBuffer.count;
    zVec3 *const inputPointsA = static_cast<zVec3 *>(self->inputContourABuffer.base);
    zVec3 *const inputPointsB = static_cast<zVec3 *>(self->inputContourBBuffer.base);

    WeilerPointBoundsXY boundsA;
    WeilerPointBoundsXY boundsB;
    ComputePointBoundsXY(inputPointsB, inputPointCountB, &boundsB);
    ComputePointBoundsXY(inputPointsA, inputPointCountA, &boundsA);

    if (boundsB.minX >= boundsA.maxX || boundsB.maxX <= boundsA.minX ||
        boundsB.minY >= boundsA.maxY || boundsB.maxY <= boundsA.minY) {
        return 1;
    }

    if (boundsB.minX <= boundsA.minX && boundsB.maxX >= boundsA.maxX &&
        boundsB.minY <= boundsA.minY && boundsB.maxY >= boundsA.maxY) {
        return zGeometry_Weiler::OutputPreclassifiedContourPairResult(
            inputPointCountA, inputPointsA, inputPointCountB, inputPointsB, 2);
    }

    if (boundsB.minX >= boundsA.minX && boundsB.maxX <= boundsA.maxX &&
        boundsB.minY >= boundsA.minY && boundsB.maxY <= boundsA.maxY) {
        return zGeometry_Weiler::OutputPreclassifiedContourPairResult(
            inputPointCountB, inputPointsB, inputPointCountA, inputPointsA, 3);
    }

    return 0;
}

// Reimplements 0x468700:
// zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
OutputSelectedInputContourToPolygonSetA(zGeometry_WeilerStatePartial *self, int mode) {
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

    if (static_cast<unsigned int>(totalPointCount) > 0x80) {
        outClip->pointList.points = static_cast<zVec3 *>(realloc(
            outClip->pointList.points, static_cast<size_t>(totalPointCount) * sizeof(zVec3)));
    }

    outClip->polygonSetA.polygons->pointCount = selectedPointCount;
    outClip->polygonSetA.polygons->pointDwordOffset = oldPointCount * 3;

    const size_t pointBytes = static_cast<size_t>(selectedPointCount) * sizeof(zVec3);
    memcpy(&outClip->pointList.points[oldPointCount], selectedInputContour->base, pointBytes);

    outClip->pointList.pointCount = oldPointCount + selectedPointCount;
    return 1;
}

// Reimplements 0x4682c0: zGeometry_Weiler::OutputContourToPolygonSet
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL OutputContourToPolygonSet(
    zGeometry_WeilerStatePartial *self, zGeometry_WeilerContourOutputPartial *contour,
    zGeometry_WeilerBufferPartial *polygonBuffer, zGeometry_PolygonSpanArrayPartial *polygonSet) {
    zGeometry_WeilerContourSegmentPartial *segment = contour->firstSegment;
    zGeometry_WeilerContourSegmentPartial *const lastSegment = segment->prev;
    zGeometry_WeilerClipOutputPartial *const outClip = self->outClip;

    zGeometry_PolygonPointSpanPartial *const polygon =
        static_cast<zGeometry_PolygonPointSpanPartial *>(zGeometry_WeilerBuffer::GetAppendSpace(
            polygonBuffer, 1, reinterpret_cast<void **>(&polygonSet->polygons)));
    if (polygon == 0) {
        return 0;
    }

    polygon->pointDwordOffset = outClip->pointList.pointCount * 3;
    polygon->pointCount = contour->pointCount;
    ++polygonSet->polygonCount;

    zVec3 *outPoint = static_cast<zVec3 *>(zGeometry_WeilerBuffer::GetAppendSpace(
        &self->pointListBuffer, contour->pointCount,
        reinterpret_cast<void **>(&outClip->pointList.points)));
    if (outPoint == 0) {
        fprintf(stderr, "%s %d: outputContour call to bufEntry failed.\n",
                     kZGeoWeilerSourceFile, 0xfb9);
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

// Reimplements 0x4681a0: zGeometry_Weiler::OutputContoursForClipMode
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
OutputContoursForClipMode(zGeometry_WeilerStatePartial *self) {
    int contourCount = self->contourBuffer.count;
    zGeometry_WeilerContourOutputPartial *contour =
        static_cast<zGeometry_WeilerContourOutputPartial *>(self->contourBuffer.base);

    if (contourCount == 0) {
        return 1;
    }

    while (contourCount != 0) {
        if (contour->firstSegment != 0) {
            if ((self->clipMode & 1) != 0 && contour->contourType == 3 &&
                zGeometry_Weiler::OutputContourToPolygonSet(self, contour, &self->polygonSetABuffer,
                                                            &self->outClip->polygonSetA) == 0) {
                zError::ReportOld(0x200, kZGeoWeilerSourceFile, 0xf5e, "Failed to output contours");
                return 0;
            }

            if ((self->clipMode & 2) != 0) {
                const int contourType = contour->contourType;
                if ((contourType == 6 || contourType == 2) &&
                    zGeometry_Weiler::OutputContourToPolygonSet(self, contour,
                                                                &self->polygonSetBBuffer,
                                                                &self->outClip->polygonSetB) == 0) {
                    zError::ReportOld(0x200, kZGeoWeilerSourceFile, 0xf71,
                                      "Failed to output contours");
                    return 0;
                }
            }

            if ((self->clipMode & 4) != 0) {
                const int contourType = contour->contourType;
                if ((contourType == 1 || contourType == 5) &&
                    zGeometry_Weiler::OutputContourToPolygonSet(self, contour,
                                                                &self->polygonSetCBuffer,
                                                                &self->outClip->polygonSetC) == 0) {
                    zError::ReportOld(0x100, kZGeoWeilerSourceFile, 0xf7d,
                                      "Found to output contours");
                    return 0;
                }
            }
        }

        ++contour;
        --contourCount;
    }

    return 1;
}

// Reimplements 0x469d60:
// zGeometry_Weiler::SelectForwardStartPointInContourA
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SelectForwardStartPointInContourA(
    zVec3 *point, zVec3 **selectedPoint, zGeometry_WeilerStatePartial *self) {
    const int pointCount = self->inputContourABuffer.count;
    zVec3 *const points = static_cast<zVec3 *>(self->inputContourABuffer.base);
    if (pointCount == 0) {
        return;
    }

    zVec3 *currentPoint = points;
    zVec3 *previousPoint = &points[pointCount - 1];
    int remainingPointCount = pointCount - 1;

    while (true) {
        zVec3 *const candidatePoint = *selectedPoint;
        const float pointCross = EdgeSideXY(point, previousPoint, currentPoint);
        const float candidateCross = EdgeSideXY(candidatePoint, previousPoint, currentPoint);

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

// Reimplements 0x4687b0: zGeometry_Weiler::GenerateOutsideResults
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
GenerateOutsideResults(zGeometry_WeilerStatePartial *self) {
    const int contourAPointCount = self->inputContourABuffer.count;
    zVec3 *const contourBPoints = static_cast<zVec3 *>(self->inputContourBBuffer.base);
    zGeometry_WeilerClipOutputPartial *const outClip = self->outClip;
    const int contourBPointCount = self->inputContourBBuffer.count;

    if ((self->clipMode & 2) == 0) {
        return 1;
    }

    zGeometry_PolygonPointSpanPartial *const polygon =
        static_cast<zGeometry_PolygonPointSpanPartial *>(zGeometry_WeilerBuffer::GetAppendSpace(
            &self->polygonSetBBuffer, 1,
            reinterpret_cast<void **>(&outClip->polygonSetB.polygons)));
    if (polygon == 0) {
        fprintf(stderr, "%s %d: _gen._outside_rslts call to buf_entry failed\n",
                     kZGeoWeilerSourceFile, 0x11f7);
        return 0;
    }

    polygon->pointDwordOffset = outClip->pointList.pointCount * 3;
    polygon->pointCount = contourBPointCount + contourAPointCount + 2;
    ++outClip->polygonSetB.polygonCount;

    zVec3 *outPoint = static_cast<zVec3 *>(zGeometry_WeilerBuffer::GetAppendSpace(
        &self->pointListBuffer, polygon->pointCount,
        reinterpret_cast<void **>(&outClip->pointList.points)));
    if (outPoint == 0) {
        fprintf(stderr, "%s %d: _gen._outside_rslts call to buf_entry failed\n",
                     kZGeoWeilerSourceFile, 0x120f);
        return 0;
    }

    outClip->pointList.pointCount += polygon->pointCount;

    zVec3 *const contourAPoints = static_cast<zVec3 *>(self->inputContourABuffer.base);
    zVec3 *selectedContourAPoint = contourAPoints;
    zVec3 *contourAPoint = &contourAPoints[1];
    for (int i = contourAPointCount - 1; i > 0; --i) {
        if (SelectMaxXThenMaxY(contourAPoint, selectedContourAPoint)) {
            selectedContourAPoint = contourAPoint;
        }

        ++contourAPoint;
    }

    zVec3 *selectedContourBPoint = contourBPoints;
    zVec3 *contourBPoint = &contourBPoints[1];
    for (int i_2005 = contourBPointCount - 1; i_2005 > 0; --i_2005) {
        if (SelectMaxXThenMaxY(contourBPoint, selectedContourBPoint)) {
            selectedContourBPoint = contourBPoint;
        }

        ++contourBPoint;
    }

    zGeometry_Weiler::SelectForwardStartPointInContourA(selectedContourBPoint,
                                                        &selectedContourAPoint, self);

    zVec3 *const contourBLastPoint = &contourBPoints[contourBPointCount - 1];
    contourBPoint = selectedContourBPoint;
    for (int i_2018 = contourBPointCount; i_2018 > 0; --i_2018) {
        CopyPointAndAdvance(outPoint, contourBPoint);
        contourBPoint = contourBPoint == contourBLastPoint ? contourBPoints : contourBPoint + 1;
    }

    CopyPointAndAdvance(outPoint, contourBPoint);

    zVec3 *contourAWritePoint = selectedContourAPoint;
    for (int i_2026 = contourAPointCount; i_2026 > 0; --i_2026) {
        CopyPointAndAdvance(outPoint, contourAWritePoint);
        contourAWritePoint = contourAWritePoint == contourAPoints
                                 ? &contourAPoints[contourAPointCount - 1]
                                 : contourAWritePoint - 1;
    }

    CopyPointAndAdvance(outPoint, contourAWritePoint);
    return 1;
}

// Reimplements 0x469a30:
// zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
PreclassifyInputContourAAdjacentEdgePairs(zGeometry_WeilerStatePartial *self) {
    const int pointCount = self->inputContourABuffer.count;
    zGeometry_WeilerContourSegmentPartial *const segmentBase =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(self->segmentBuffer.base);
    zGeometry_WeilerContourOutputPartial *const contourBase =
        static_cast<zGeometry_WeilerContourOutputPartial *>(self->contourBuffer.base);
    zVec3 *const points = static_cast<zVec3 *>(self->inputContourABuffer.base);

    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->segmentBuffer, pointCount << 1);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->contourBuffer, 2);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->xingBuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->polygonSetABuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->polygonSetBBuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->polygonSetCBuffer, 0);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&self->pointListBuffer, 0);

    zGeometry_WeilerContourSegmentArray::InitFromPointList(segmentBase, points, pointCount, 1);
    segmentBase->contourOutput = contourBase;
    contourBase[0].firstSegment = segmentBase;
    zGeometry_WeilerContourSegmentArray::UpdateBounds(segmentBase, pointCount);

    zGeometry_WeilerContourSegmentPartial *const reverseSegments = &segmentBase[pointCount];
    zGeometry_WeilerContourSegmentArray::InitFromPointList(reverseSegments, points, pointCount, 4);
    reverseSegments->contourOutput = &contourBase[1];
    contourBase[1].firstSegment = reverseSegments;
    zGeometry_WeilerContourSegmentArray::UpdateBounds(reverseSegments, pointCount);
}

// Reimplements 0x468470:
// zGeometry_Weiler::BuildPointSideTablesForContourPair
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
BuildPointSideTablesForContourPair(zGeometry_WeilerStatePartial *self) {
    BuildPointSideTable(static_cast<zVec3 *>(self->inputContourBBuffer.base),
                        self->inputContourBBuffer.count,
                        static_cast<zVec3 *>(self->inputContourABuffer.base),
                        self->inputContourABuffer.count, self->contourAPointSideByContourBEdge);

    BuildPointSideTable(static_cast<zVec3 *>(self->inputContourABuffer.base),
                        self->inputContourABuffer.count,
                        static_cast<zVec3 *>(self->inputContourBBuffer.base),
                        self->inputContourBBuffer.count, self->contourBPointSideByContourAEdge);
}

// Reimplements 0x464f70:
// zGeometry_Weiler::PreclassifyInputContourPair
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
PreclassifyInputContourPair(zGeometry_WeilerStatePartial *self) {
    WeilerPreclassifyContourPacket *const contourPacket =
        static_cast<WeilerPreclassifyContourPacket *>(self->contourBuffer.base);

    zGeometry_WeilerContourSegmentPartial *contourA = contourPacket->contourA.firstSegment;
    zGeometry_WeilerContourSegmentPartial *contourB = contourPacket->contourB.firstSegment;
    zGeometry_WeilerContourSegmentPartial *const contourC = contourPacket->contourC.firstSegment;
    zGeometry_WeilerContourSegmentPartial *const contourD = contourPacket->contourD.firstSegment;

    float *orientationTableB = self->contourBPointSideByContourAEdge;
    const int contourAPointCount = self->inputContourABuffer.count;
    const int contourBPointCount = self->inputContourBBuffer.count;

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

            if (IsNearZeroForCoincidentWeedOut(orientationTableA[0]) &&
                IsNearZeroForCoincidentWeedOut(orientationTableA[1]) &&
                IsNearZeroForCoincidentWeedOut(orientationTableB[0]) &&
                IsNearZeroForCoincidentWeedOut(orientationTableB[1])) {
                const int overlapCase =
                    ((((zGeometry_Vec3::IsBetweenEndpointsXY(contourAStart, contourCStart,
                                                             contourCEnd) *
                        2) |
                       zGeometry_Vec3::IsBetweenEndpointsXY(contourAEnd, contourCStart,
                                                            contourCEnd))
                      << 1) |
                     zGeometry_Vec3::IsBetweenEndpointsXY(contourCStart, contourAStart,
                                                          contourAEnd))
                        << 1 |
                    zGeometry_Vec3::IsBetweenEndpointsXY(contourCEnd, contourAStart, contourAEnd);
                const bool sameDirection =
                    HaveSameDirectionXY(contourAStart, contourAEnd, contourCStart, contourCEnd);

                switch (overlapCase - 3) {
                case 0:
                    if (sameDirection) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourA, contourB, contourCEnd, 0, 0) == 0) {
                            ReportWeedOutInsideAFailure(0x568);
                            return 0;
                        }

                        contourAEnd = contourCStart;
                        contourB->endPoint = contourCStart;
                        contourA->endPoint = contourCStart;
                        contourCWalker->contourType |= contourA->contourType;
                        contourDWalker->contourType |= contourB->contourType;
                    } else {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourA, contourB, contourCStart, 0, 0) == 0) {
                            ReportWeedOutInsideAFailure(0x572);
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
                    if (!IsNearPointXY(contourAEnd, contourCEnd)) {
                        zVec3 *const oldContourAEnd = contourAEnd;
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourA, contourB, contourCEnd, contourDWalker->contourType,
                                contourCWalker->contourType) == 0) {
                            ReportWeedOutSegForwardFailure(0x593);
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
                    if (!IsNearPointXY(contourAEnd, contourCStart)) {
                        zVec3 *const oldContourAEnd = contourAEnd;
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourCWalker, contourDWalker, contourAEnd, 0, 0) == 0) {
                            ReportWeedOutSegForwardFailure(0x5b6);
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
                    if (sameDirection) {
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
                    if (!IsNearPointXY(contourAStart, contourCEnd)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourA, contourB, contourCEnd, 0, 0) == 0) {
                            ReportWeedOutSegForwardFailure(0x5f0);
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
                    if (!IsNearPointXY(contourAStart, contourCStart)) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourA, contourB, contourCStart, 0, 0) == 0) {
                            ReportWeedOutSegForwardFailure(0x610);
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
                    if (sameDirection) {
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
                    if (sameDirection) {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourCWalker, contourDWalker, contourAEnd, 0, 0) == 0) {
                            ReportWeedOutForwardSegmentFailure(0x64a);
                            return 0;
                        }

                        contourDWalker->endPoint = contourAStart;
                        contourCWalker->endPoint = contourAStart;
                    } else {
                        if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(
                                self, contourCWalker, contourDWalker, contourAStart, 0, 0) == 0) {
                            ReportWeedOutForwardSegmentFailure(0x650);
                            return 0;
                        }

                        contourDWalker->endPoint = contourAEnd;
                        contourCWalker->endPoint = contourAEnd;
                    }

                    contourA->contourType |= contourCWalker->contourType;
                    contourB->contourType |= contourDWalker->contourType;
                    zGeometry_WeilerContourSegment::UpdateBounds(contourCWalker);
                    break;

                case 10:
                    if (sameDirection) {
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
                    if (sameDirection) {
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
                    if (sameDirection) {
                        contourA->contourType |= contourCWalker->contourType;
                        contourB->contourType |= contourDWalker->contourType;
                    } else {
                        contourB->contourType |= contourCWalker->contourType;
                        contourA->contourType |= contourDWalker->contourType;
                    }

                    UnlinkWeilerContourSegmentPair(contourPacket, contourC, contourCWalker,
                                                   contourDWalker);
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

// Reimplements 0x465ac0:
// zGeometry_Weiler::ClassifyContainedContour
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ClassifyContainedContour(zGeometry_WeilerStatePartial *self) {
    WeilerPreclassifyContourPacket *const contourPacket =
        static_cast<WeilerPreclassifyContourPacket *>(self->contourBuffer.base);

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
            if (SegmentBoundsOverlapXY(contourOutput0Segment, contourOutput2Segment) &&
                !HasExistingXingWithContourPair(contourOutput2Segment, contourOutput3Segment,
                                                contourOutput0Segment, contourOutput1Segment)) {
                zGeometry_WeilerXingPartial *intersectXing = 0;
                const int intersectResult = zGeometry_Weiler::Intersect2d(
                    self, &intersectXing, *contourOutput0Segment->startPoint,
                    *contourOutput0Segment->endPoint, *contourOutput2Segment->startPoint,
                    *contourOutput2Segment->endPoint);

                if (intersectResult == 1) {
                    ReportWeilerIntersectError(0x735);
                    return 1;
                }

                if (intersectResult != 0) {
                    aggregateIntersectResult |= intersectResult;
                }

                if (intersectXing != 0) {
                    ClearWeilerXingSegments(intersectXing);

                    if (DivideContainedContourPairAtXing(
                            self, intersectXing, contourOutput0Segment, contourOutput1Segment,
                            contourOutput2Segment, contourOutput3Segment, 0x7ea) == 0) {
                        return 1;
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
        PropagateXingBackReferencesForContourPair(contourOutput0Start, contourOutput1Start,
                                                  contourOutput2Start, contourOutput3Start);
    }

    return aggregateIntersectResult;
}

// Reimplements 0x468fa0: zGeometry_Weiler::ClassifyIntersect2d
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ClassifyIntersect2d(zVec3 *edge0Start, zVec3 *edge0End, zVec3 *edge1Start, zVec3 *edge1End,
                    zGeometry_WeilerStatePartial *self) {
    const float edge1DeltaY = edge1End->y - edge1Start->y;
    const float edge1DeltaX = edge1End->x - edge1Start->x;

    float edge0StartSide = EdgeSideXY(edge0Start, edge1Start, edge1End);
    float edge0EndSide = EdgeSideXY(edge0End, edge1Start, edge1End);
    if (HasStrictSameSign(edge0StartSide, edge0EndSide)) {
        return 0;
    }

    float edge1StartSide = EdgeSideXY(edge1Start, edge0Start, edge0End);
    float edge1EndSide = EdgeSideXY(edge1End, edge0Start, edge0End);
    if (HasStrictSameSign(edge1StartSide, edge1EndSide)) {
        return 0;
    }

    const int zeroSideCount =
        (edge0StartSide == 0.0f ? 1 : 0) + (edge0EndSide == 0.0f ? 1 : 0) +
        (edge1StartSide == 0.0f ? 1 : 0) + (edge1EndSide == 0.0f ? 1 : 0);

    if (zeroSideCount == 2) {
        zVec3 probe;
        probe.z = 0.0f;

        if (edge1StartSide == 0.0f) {
            probe.x = edge1Start->x + edge1DeltaX * 0.00000999999975f;
            probe.y = edge1Start->y + edge1DeltaY * 0.00000999999975f;

            const int probeClass = zGeometry_Weiler::ClassifyPointInContourPointListXY(
                &probe, self->inputContourABuffer.count,
                static_cast<zVec3 *>(self->inputContourABuffer.base));

            if ((edge1EndSide > 0.0f && probeClass > 0) ||
                (edge1EndSide <= 0.0f && probeClass < 0)) {
                edge0StartSide = -edge0StartSide;
                edge0EndSide = -edge0EndSide;
                edge1EndSide = -edge1EndSide;
            }
        } else {
            probe.x = edge1Start->x + edge1DeltaX * 0.999989986f;
            probe.y = edge1Start->y + edge1DeltaY * 0.999989986f;

            const int probeClass = zGeometry_Weiler::ClassifyPointInContourPointListXY(
                &probe, self->inputContourABuffer.count,
                static_cast<zVec3 *>(self->inputContourABuffer.base));

            if ((edge1StartSide > 0.0f && probeClass > 0) ||
                (edge1StartSide <= 0.0f && probeClass < 0)) {
                edge0StartSide = -edge0StartSide;
                edge0EndSide = -edge0EndSide;
                edge1StartSide = -edge1StartSide;
            }
        }
    }

    const int index = ((SignClassForIntersectTable(edge0StartSide) * 3 +
                                 SignClassForIntersectTable(edge0EndSide)) *
                                    3 +
                                SignClassForIntersectTable(edge1StartSide)) *
                                   3 +
                               SignClassForIntersectTable(edge1EndSide);

    return kIntersect2dCaseIdBySignClass[index];
}

// Reimplements 0x468c40: zGeometry_Weiler::Intersect2d
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL Intersect2d(zGeometry_WeilerStatePartial *self,
                                                         zGeometry_WeilerXingPartial **outXing,
                                                         zVec3 edge0Start, zVec3 edge0End,
                                                         zVec3 edge1Start, zVec3 edge1End) {
    int xingType =
        zGeometry_Weiler::ClassifyIntersect2d(&edge0Start, &edge0End, &edge1Start, &edge1End, self);

    zGeometry_WeilerXingPartial *createdXing = 0;
    if (static_cast<unsigned int>(xingType) <= 0x17) {
        switch (kIntersect2dOutputKindByXingType[xingType]) {
        case 1: {
            const double edge1ReverseDeltaX =
                static_cast<double>(edge1Start.x) - static_cast<double>(edge1End.x);
            const double edge1ReverseDeltaY =
                static_cast<double>(edge1Start.y) - static_cast<double>(edge1End.y);
            const double edge0DeltaX =
                static_cast<double>(edge0End.x) - static_cast<double>(edge0Start.x);
            const double edge0DeltaY =
                static_cast<double>(edge0End.y) - static_cast<double>(edge0Start.y);
            const double divisor =
                edge1ReverseDeltaY * edge0DeltaX - edge1ReverseDeltaX * edge0DeltaY;

            if (divisor == 0.0) {
                xingType = 0;
                break;
            }

            createdXing = AllocWeilerXing(self, 0x1301);
            if (createdXing == 0) {
                return 1;
            }

            const double edge0Param =
                ((static_cast<double>(edge1Start.x) - static_cast<double>(edge0Start.x)) *
                     edge1ReverseDeltaY +
                 (static_cast<double>(edge1Start.y) - static_cast<double>(edge0Start.y)) *
                     -edge1ReverseDeltaX) /
                divisor;
            createdXing->point.x = static_cast<float>(edge0DeltaX * edge0Param + edge0Start.x);
            createdXing->point.y = static_cast<float>(edge0DeltaY * edge0Param + edge0Start.y);

            if (edge1ReverseDeltaX != 0.0) {
                createdXing->point.z = static_cast<float>(
                    ((static_cast<double>(edge1Start.x) -
                      static_cast<double>(createdXing->point.x)) /
                     edge1ReverseDeltaX) *
                        (static_cast<double>(edge1End.z) - static_cast<double>(edge1Start.z)) +
                    static_cast<double>(edge1Start.z));
            } else {
                createdXing->point.z = static_cast<float>(
                    ((static_cast<double>(edge1Start.y) -
                      static_cast<double>(createdXing->point.y)) /
                     (static_cast<double>(edge1Start.y) - static_cast<double>(edge1End.y))) *
                        (static_cast<double>(edge1End.z) - static_cast<double>(edge1Start.z)) +
                    static_cast<double>(edge1Start.z));
            }

            break;
        }

        case 2:
            createdXing = AllocWeilerXing(self, 0x1351);
            if (createdXing == 0) {
                return 1;
            }

            createdXing->point = edge0Start;
            break;

        case 3:
            createdXing = AllocWeilerXing(self, 0x1363);
            if (createdXing == 0) {
                return 1;
            }

            createdXing->point = edge0End;
            break;

        case 4:
            createdXing = AllocWeilerXing(self, 0x132a);
            if (createdXing == 0) {
                return 1;
            }

            createdXing->point = edge1Start;
            break;

        case 5:
            createdXing = AllocWeilerXing(self, 0x1340);
            if (createdXing == 0) {
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

// Reimplements 0x469450:
// zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ClassifyAdjacentEdgePairAgainstContourSegment(
    zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment,
    zGeometry_WeilerContourSegmentPartial *contourSegment) {
    if (firstSegment->endPoint != secondSegment->startPoint) {
        return 0;
    }

    const float firstSide =
        EdgeSideXY(firstSegment->startPoint, contourSegment->startPoint, contourSegment->endPoint);
    const float secondSide =
        EdgeSideXY(secondSegment->endPoint, contourSegment->startPoint, contourSegment->endPoint);

    if ((firstSide < 0.0f && secondSide > 0.0f) || (firstSide > 0.0f && secondSide < 0.0f)) {
        return 7;
    }

    return EdgeSideXY(secondSegment->endPoint, firstSegment->startPoint, firstSegment->endPoint) >
                   0.0f
               ? 1
               : 2;
}

// Reimplements 0x469560:
// zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
    zGeometry_WeilerContourSegmentPartial *pairAFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairASecondSegment,
    zGeometry_WeilerContourSegmentPartial *pairBFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairBSecondSegment, zGeometry_WeilerStatePartial *) {
    if (pairAFirstSegment->endPoint != pairASecondSegment->startPoint ||
        pairBFirstSegment->endPoint != pairBSecondSegment->startPoint) {
        return 0;
    }

    const int startClass = ClassifyPointAgainstAdjacentEdgePair(
        pairAFirstSegment->startPoint, pairBFirstSegment, pairBSecondSegment);
    const int endClass = ClassifyPointAgainstAdjacentEdgePair(
        pairASecondSegment->endPoint, pairBFirstSegment, pairBSecondSegment);

    if (startClass == -1 && endClass == -1) {
        return ArePointsStrictlyNegativeToAdjacentEdgePair(pairBFirstSegment->startPoint,
                                                           pairBSecondSegment->endPoint,
                                                           pairAFirstSegment, pairASecondSegment)
                   ? 6
                   : 5;
    }

    if (startClass == 1 && endClass == 1) {
        return ArePointsStrictlyNegativeToAdjacentEdgePair(pairBFirstSegment->startPoint,
                                                           pairBSecondSegment->endPoint,
                                                           pairAFirstSegment, pairASecondSegment)
                   ? 4
                   : 3;
    }

    return startClass == -1 ? 8 : 9;
}

// Reimplements 0x46a1f0: zGeometry_Weiler::ValidateXings
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ValidateXings(int xingCount,
                                                           zGeometry_WeilerXingPartial *xingArray,
                                                           int *failedXingIndex) {
    int isValid = 1;
    zGeometry_WeilerXingPartial *xing = xingArray;

    {
    for (int xingIndex = 0; xingIndex < xingCount; ++xingIndex) {
        isValid = ValidateWeilerXingSegmentSet(xing);
        if (isValid == 0) {
            if (failedXingIndex != 0) {
                *failedXingIndex = xingIndex;
            }

            if (xing != 0) {
                zError::ReportOld(0x100, kZGeoWeilerSourceFile, 0x1788,
                                  "validateXing failed (xing %d) (type = %d)", xingIndex,
                                  xing->xingType);
            } else {
                zError::ReportOld(0x100, kZGeoWeilerSourceFile, 0x179a,
                                  "validateXing failed (xing %d) xing_p is NULL!", xingIndex);
            }

            break;
        }

        ++xing;
    }
    }

    return isValid;
}

// Reimplements 0x4683a0:
// zGeometry_Weiler::TogglePointAxesForContourSource
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
TogglePointAxesForContourSource(zGeometry_WeilerStatePartial *self) {
    if (self->inputContourBBuffer.base != 0) {
        TogglePointAxes(&self->inputContourBBuffer, self->contourSource);
        return;
    }

    TogglePointAxes(&self->inputContourABuffer, self->contourSource);
}

// Reimplements 0x469960:
// zGeometry_Weiler::RecenterPointSetsIfOutOfRange
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RecenterPointSetsIfOutOfRange(zGeometry_WeilerStatePartial *self) {
    if (self->inputContourBBuffer.base != 0) {
        if (self->inputContourBBuffer.count != 0) {
            RecenterPoints(&self->inputContourBBuffer, self->pointTranslationX,
                           self->pointTranslationY);
        }

        return;
    }

    zVec3 *const firstPoint = static_cast<zVec3 *>(self->inputContourABuffer.base);
    if (firstPoint->x < 65536.0f && firstPoint->x > -65536.0f && firstPoint->y < 65536.0f &&
        firstPoint->y > -65536.0f) {
        self->pointsRecentered = false;
        return;
    }

    self->pointTranslationX = firstPoint->x;
    self->pointTranslationY = firstPoint->y;
    self->pointsRecentered = true;

    if (self->inputContourABuffer.count != 0) {
        RecenterPoints(&self->inputContourABuffer, self->pointTranslationX,
                       self->pointTranslationY);
    }
}

// Reimplements 0x469af0: zGeometry_Weiler::RestorePointTranslation
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RestorePointTranslation(zGeometry_WeilerStatePartial *self) {
    const float translationX = self->pointTranslationX;
    const float translationY = self->pointTranslationY;

    zVec3 *point = static_cast<zVec3 *>(self->inputContourBBuffer.base);
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

// Reimplements 0x469b60:
// zGeometry_Weiler::RestoreOutputZFromInputPlane
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
RestoreOutputZFromInputPlane(zGeometry_WeilerStatePartial *self) {
    zVec3 *const inputPoints = static_cast<zVec3 *>(self->inputContourBBuffer.base);

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
    for (unsigned int i = 0; i < static_cast<unsigned int>(outClip->pointList.pointCount); ++i) {
        point->z = -(planeNormal.x * point->x + planeNormal.y * point->y + planeOffset);
        ++point;
    }
}

// Reimplements 0x4647d0: zGeometry_Weiler::DestroyState
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL DestroyState(zGeometry_WeilerStatePartial *self) {
    if (self == 0) {
        return;
    }

    zGeometry_WeilerBuffer::Destroy(&self->segmentBuffer);
    zGeometry_WeilerBuffer::Destroy(&self->contourBuffer);
    zGeometry_WeilerBuffer::Destroy(&self->xingBuffer);
    zGeometry_WeilerBuffer::Destroy(&self->inputContourABuffer);
    free(self);
}
} // namespace zGeometry_Weiler

namespace zGeometry_WeilerClipOutput {
// Reimplements 0x464b30: zGeometry_WeilerClipOutput::Destroy
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_WeilerClipOutputPartial *self) {
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

namespace zGeometry_ClipPolygon {
// Reimplements 0x46ab40: zGeometry_ClipPolygon::FindPointIndexXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
FindPointIndexXY(zGeometry_ClipPolygonPartial *clipPolygon, zVec3 *point) {
    for (int i = 0; i < clipPolygon->pointCount; ++i) {
        if (zGeometry_Vec3::IsNearEqualXY(&clipPolygon->points[i], point, 0.00999999978f)) {
            return i;
        }
    }

    return -1;
}

// Reimplements 0x46ac80:
// zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
FindPointInsertionEdgeXYIndex(zGeometry_ClipPolygonPartial *clipPolygon, zVec3 *point) {
    const float tolerance = 0.00999999978f;
    zVec3 *current = clipPolygon->points;
    const int pointCount = clipPolygon->pointCount;

    for (int i = 0; i < pointCount; ++i) {
        zVec3 *const next = &clipPolygon->points[(i + 1) % pointCount];
        const float edgeDx = next->x - current->x;
        const float edgeDy = next->y - current->y;

        if (fabs(edgeDx) < tolerance) {
            if (fabs(current->x - point->x) <= tolerance) {
                const float t = (point->y - current->y) / edgeDy;
                if (t > 0.0f && t < 1.0f) {
                    return i;
                }
            }
        } else if (fabs(edgeDy) < tolerance) {
            if (fabs(point->y - current->y) <= tolerance) {
                const float t = (point->x - current->x) / edgeDx;
                if (t > 0.0f && t < 1.0f) {
                    return i;
                }
            }
        } else {
            const float tX = (point->x - current->x) / edgeDx;
            const float tY = (point->y - current->y) / edgeDy;
            if (fabs(tX - tY) < tolerance && tY > 0.0f && tY < 1.0f && tX > 0.0f &&
                tX < 1.0f && fabs(tX * edgeDx + current->x - point->x) <= tolerance &&
                fabs(tY * edgeDy + current->y - point->y) <= tolerance) {
                return i;
            }
        }

        current = next;
    }

    return -1;
}

// Reimplements 0x46ab90: zGeometry_ClipPolygon::UpsertPointListXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL UpsertPointListXY(
    zGeometry_ClipPolygonPartial *clipPolygon, int pointCount, zVec3 *points) {
    int result = 0;
    if (pointCount <= 0) {
        return result;
    }

    zVec3 *point = points;
    {
    for (int remaining = pointCount; remaining != 0; --remaining) {
        const int existingIndex =
            zGeometry_ClipPolygon::FindPointIndexXY(clipPolygon, point);
        if (existingIndex != -1) {
            clipPolygon->points[existingIndex] = *point;
            result = 1;
        } else {
            const int edgeIndex =
                zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex(clipPolygon, point);
            if (edgeIndex != -1) {
                const int oldPointCount = clipPolygon->pointCount;
                clipPolygon->points = static_cast<zVec3 *>(
                    realloc(clipPolygon->points, (oldPointCount + 1) * sizeof(zVec3)));

                if (edgeIndex != oldPointCount - 1) {
                    memmove(&clipPolygon->points[edgeIndex + 2],
                                 &clipPolygon->points[edgeIndex + 1],
                                 (oldPointCount - edgeIndex - 1) * sizeof(zVec3));
                }

                clipPolygon->points[edgeIndex + 1] = *point;
                clipPolygon->pointCount = oldPointCount + 1;
                result = 1;
            }
        }

        ++point;
    }
    }

    return result;
}

// Reimplements 0x464790:
// zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ResetWeilerStateFromContourPoints(
    zGeometry_ClipPolygonPartial *clipPolygon, zVec3 *points, int pointCount) {
    if (points == 0 || pointCount == 0) {
        return 0;
    }

    zGeometry_WeilerStatePartial *const oldState = clipPolygon->weilerState;
    zGeometry_WeilerStatePartial *const newState =
        zGeometry_Weiler::Init(points, pointCount, oldState->contourSource);
    zGeometry_Weiler::DestroyState(oldState);
    clipPolygon->weilerState = newState;
    return 1;
}

// Reimplements 0x46ab10: zGeometry_ClipPolygon::FinalizeAndDestroy
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL FinalizeAndDestroy(zGeometry_ClipPolygonPartial *clipPolygon) {
    if (clipPolygon->points != 0) {
        free(clipPolygon->points);
    }

    zGeometry_Weiler::DestroyState(clipPolygon->weilerState);
    free(clipPolygon);
}
} // namespace zGeometry_ClipPolygon
