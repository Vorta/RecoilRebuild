#pragma once

#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zMath/zMath.h"
#include "recoil/recoil_callconv.h"

typedef zClass_NodePartial zGeometry_ClipPatchNodeView;

struct zDEClient_FeatureGridCell;
struct zGeometry_PolygonPointSpanPartial;
struct zGeometry_WeilerContourOutputPartial;
struct zGeometry_WeilerContourSegmentPartial;
struct zGeometry_WeilerXingPartial;
struct zGeometry_WeilerStatePartial;

struct zGeometry_BoundsXY {
    float minX;
    float maxY;
    float maxX;
    float minY;
};

struct zGeometry_WeilerBufferPartial {
    int elementSize;
    int capacity;
    int count;
    void *base;
    void *appendPtr;
};

struct zGeometry_WeilerContourSegmentPartial {
    zGeometry_WeilerContourSegmentPartial *prev;
    zGeometry_WeilerContourSegmentPartial *next;
    int contourType;
    zVec3 *startPoint;
    zVec3 *endPoint;
    int boundsDirty;
    float minX;
    float minY;
    unsigned char unknown_20[0x04];
    float maxX;
    float maxY;
    unsigned char unknown_2c[0x04];
    zGeometry_WeilerXingPartial *startXing;
    zGeometry_WeilerXingPartial *endXing;
    zGeometry_WeilerContourOutputPartial *contourOutput;
};

struct zGeometry_WeilerXingPartial {
    zVec3 point;
    zGeometry_WeilerContourSegmentPartial *segment0;
    zGeometry_WeilerContourSegmentPartial *segment1;
    zGeometry_WeilerContourSegmentPartial *segment2;
    zGeometry_WeilerContourSegmentPartial *segment3;
    zGeometry_WeilerContourSegmentPartial *segment4;
    zGeometry_WeilerContourSegmentPartial *segment5;
    zGeometry_WeilerContourSegmentPartial *segment6;
    zGeometry_WeilerContourSegmentPartial *segment7;
    int xingType;
};

struct zGeometry_WeilerContourOutputPartial {
    int contourType;
    zGeometry_WeilerContourSegmentPartial *firstSegment;
    int pointCount;
};

struct zGeometry_PolygonPointSpanPartial {
    int pointDwordOffset;
    int pointCount;
};

struct zGeometry_PolygonSpanArrayPartial {
    int polygonCount;
    zGeometry_PolygonPointSpanPartial *polygons;
};

struct zGeometry_ConvexPolygonSetPartial {
    int polygonCount;
    zGeometry_PolygonPointSpanPartial *polygons;
    int totalPointCount;
    zVec3 *points;
};

struct zGeometry_TriangleDwordOffsetList {
    int triangleCount;
    int triangleDwordOffsets[1];
};

struct zGeometry_TriangleIndexTriple {
    int i0;
    int i1;
    int i2;
};

struct zGeometry_TriangulateHole_EdgeState {
    int vertexIndex0;
    int vertexIndex1;
    int remainingUseCount;
};

struct zGeometry_PlaneEquationPartial {
    float a;
    float b;
    float c;
    float d;
};

struct zGeometry_TriangleSoup {
    int triangleCount;
    zVec3 triangleVerts[1];
};

struct zGeometry_PolygonSplitDwordOffsetListPair {
    int pointCount0;
    int pointCount1;
    int pointDwordOffsets[1];
};

struct zGeometry_PointListPartial {
    int pointCount;
    zVec3 *points;
};

struct zGeometry_WeilerClipOutputPartial {
    zGeometry_PolygonSpanArrayPartial polygonSetA;
    zGeometry_PolygonSpanArrayPartial polygonSetB;
    zGeometry_PolygonSpanArrayPartial polygonSetC;
    zGeometry_PointListPartial pointList;
};

struct zGeometry_ClipPolygonPartial {
    zGeometry_WeilerStatePartial *weilerState;
    zVec3 *points;
    int pointCount;
    zGeometry_BoundsXY bounds;
};

struct zGeometry_WeilerStatePartial {
    int clipMode;
    int contourSource;
    zGeometry_WeilerClipOutputPartial *outClip;
    zGeometry_WeilerBufferPartial inputContourABuffer;
    zGeometry_WeilerBufferPartial inputContourBBuffer;
    zGeometry_WeilerBufferPartial segmentBuffer;
    zGeometry_WeilerBufferPartial contourBuffer;
    zGeometry_WeilerBufferPartial xingBuffer;
    zGeometry_WeilerBufferPartial polygonSetABuffer;
    zGeometry_WeilerBufferPartial polygonSetBBuffer;
    zGeometry_WeilerBufferPartial polygonSetCBuffer;
    zGeometry_WeilerBufferPartial pointListBuffer;
    float contourAPointSideByContourBEdge[0x500];
    float contourBPointSideByContourAEdge[0x500];
    float pointTranslationX;
    float pointTranslationY;
    bool pointsRecentered;
    bool allContoursSingleSided;
};

struct zGeometry_ClipPatchNodeDiPair {
    zGeometry_ClipPatchNodeView *node;
    zDiPartial *di;
};

struct zGeometry_ClipPatchPartitionOutput;

struct zGeometry_ClipPatchOutputPartial {
    int pointCount;
    zVec3 *points;
    int partitionCount;
    zGeometry_ClipPatchPartitionOutput *partitions;
};

struct zGeometry_ClipPatchPartitionOutput {
    int nodeDiPairCount;
    zGeometry_ClipPatchNodeDiPair *nodeDiPairs;
    zDEClient_FeatureGridCell *featureGridCell;
};

struct zModel_MaterialPartial;

struct zModel_PolygonUvBasis {
    zClipUV uv0;
    zClipUV uv1;
    zClipUV uv2;
};

struct zModel_PolygonPartial {
    unsigned int vertexCountAndFlags;
    unsigned int drawFlags;
    int *vertexIndices;
    unsigned char unknown_0c[0x04];
    zModel_PolygonUvBasis *uvBasis;
    zModel_MaterialPartial *material;
    int userTag;
};

struct zModel_DrawBatchBasePartial {
    unsigned char unknown_00[0x0c];
    int faceCount;
    unsigned char unknown_10[0x20];
    zModel_PolygonPartial *faceList;
    zVec3 *verts;
};

RECOIL_STATIC_ASSERT(sizeof(zGeometry_BoundsXY) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerBufferPartial, base) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerBufferPartial) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, contourType) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, startPoint) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, endPoint) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, boundsDirty) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, minX) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, minY) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, maxX) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, maxY) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, startXing) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, endXing) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerContourSegmentPartial, contourOutput) == 0x38);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerContourSegmentPartial) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerXingPartial, point) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerXingPartial, segment0) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerXingPartial, segment7) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerXingPartial, xingType) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerXingPartial) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerContourOutputPartial) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_PolygonPointSpanPartial) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_PolygonSpanArrayPartial) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ConvexPolygonSetPartial, polygons) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ConvexPolygonSetPartial, totalPointCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ConvexPolygonSetPartial, points) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ConvexPolygonSetPartial) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_TriangleDwordOffsetList, triangleDwordOffsets) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_TriangleIndexTriple) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_TriangulateHole_EdgeState) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_PlaneEquationPartial) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_TriangleSoup, triangleVerts) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_PolygonSplitDwordOffsetListPair, pointCount1) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_PolygonSplitDwordOffsetListPair, pointDwordOffsets) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerClipOutputPartial, polygonSetB) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerClipOutputPartial, polygonSetC) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerClipOutputPartial, pointList) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerClipOutputPartial) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPolygonPartial, points) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPolygonPartial, pointCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPolygonPartial, bounds) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPolygonPartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, clipMode) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, contourSource) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, outClip) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, inputContourABuffer) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, inputContourBBuffer) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, segmentBuffer) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, contourBuffer) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, xingBuffer) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, polygonSetABuffer) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, polygonSetBBuffer) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, polygonSetCBuffer) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, pointListBuffer) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, contourAPointSideByContourBEdge) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, contourBPointSideByContourAEdge) == 0x14c0);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, pointTranslationX) == 0x28c0);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, pointTranslationY) == 0x28c4);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, pointsRecentered) == 0x28c8);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_WeilerStatePartial, allContoursSingleSided) == 0x28c9);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerStatePartial) == 0x28cc);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPatchNodeDiPair) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPatchPartitionOutput, nodeDiPairs) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPatchPartitionOutput, featureGridCell) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPatchPartitionOutput) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPatchOutputPartial) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_PolygonPartial, vertexIndices) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_PolygonPartial, uvBasis) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_PolygonPartial, material) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_PolygonPartial, userTag) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zModel_PolygonPartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zModel_DrawBatchBasePartial, faceCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_DrawBatchBasePartial, faceList) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zModel_DrawBatchBasePartial, verts) == 0x34);

namespace zGeometry_Vec3Array {
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveAdjacentDuplicatePointsXY(zVec3 *vertices,
                                                                             int count);
RECOIL_NOINLINE void RECOIL_FASTCALL RotateNeg90AroundX(int pointCount, zVec3 *points);
RECOIL_NOINLINE void RECOIL_FASTCALL RotatePos90AroundX(int pointCount, zVec3 *points);
RECOIL_NOINLINE void RECOIL_FASTCALL ComputeBoundsXY(zGeometry_BoundsXY *outBounds, zVec3 *points,
                                                     int pointCount);
RECOIL_NOINLINE void RECOIL_FASTCALL ReversePoints(int pointCount, zVec3 *points);
RECOIL_NOINLINE int RECOIL_FASTCALL EnsurePositiveCrossZ(int pointCount,
                                                                  zVec3 *points,
                                                                  int allowReverse);
RECOIL_NOINLINE void RECOIL_FASTCALL ComputeNewellPlane(int pointCount, zVec3 *points,
                                                        zGeometry_PlaneEquationPartial *outPlane);
} // namespace zGeometry_Vec3Array

namespace zGeometry_Segment {
RECOIL_NOINLINE int RECOIL_FASTCALL IntersectsSegmentXY(zVec3 *segmentAPoint0,
                                                                 zVec3 *segmentAPoint1,
                                                                 zVec3 *segmentBPoint0,
                                                                 zVec3 *segmentBPoint1);
}

namespace zGeometry_TriangulateHole {
RECOIL_NOINLINE int RECOIL_FASTCALL
TryAppendBridgeEdge(zGeometry_TriangulateHole_EdgeState *edgeState, int edgeCount,
                    zGeometry_TriangulateHole_EdgeState *edgeStates);
RECOIL_NOINLINE int RECOIL_FASTCALL CollectActiveEdgeIndicesForVertex(
    int vertexIndex, int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates, int *outEdgeIndices);
RECOIL_NOINLINE void RECOIL_FASTCALL TryEmitTriangleFromEdgePair(
    int edgeIndex0, int edgeIndex1, int vertexIndex,
    int edgeCount, zGeometry_TriangulateHole_EdgeState *edgeStates);
RECOIL_NOINLINE zGeometry_TriangulateHole_EdgeState *RECOIL_FASTCALL
FindActiveEdgeState(int vertexIndex0, int vertexIndex1, int edgeCount,
                    zGeometry_TriangulateHole_EdgeState *edgeStates);
RECOIL_NOINLINE void RECOIL_FASTCALL CacheCombinedPlane(int pointCount, zVec3 *points);
RECOIL_NOINLINE void RECOIL_FASTCALL ProjectInnerRingOntoCachedPlane(int pointCount,
                                                                     zVec3 *points);
} // namespace zGeometry_TriangulateHole

namespace zGeometry {
RECOIL_NOINLINE zGeometry_TriangleSoup *RECOIL_FASTCALL
TriangulatePolygonWithHole(int outerPointCount, zVec3 *outerPoints,
                           int innerPointCount, zVec3 *innerPoints);
}

namespace zGeometry_Bounds2D {
RECOIL_NOINLINE int RECOIL_FASTCALL OverlapsWithUnitMargin(zGeometry_BoundsXY *boundsA,
                                                                    zGeometry_BoundsXY *boundsB);
}

namespace zGeometry_Vec3 {
RECOIL_NOINLINE int RECOIL_FASTCALL IsNearEqualXY(zVec3 *vecA, zVec3 *vecB,
                                                           float tolerance);
RECOIL_NOINLINE int RECOIL_FASTCALL SnapPointToSegmentXYIfNear(zVec3 *lineStart,
                                                                        zVec3 *lineEnd,
                                                                        zVec3 *testPoint,
                                                                        float tolerance);
RECOIL_NOINLINE int RECOIL_FASTCALL IsBetweenEndpointsXY(zVec3 *testPoint,
                                                                  zVec3 *startPoint,
                                                                  zVec3 *endPoint);
} // namespace zGeometry_Vec3

namespace zGeometry_Polygon {
RECOIL_NOINLINE void RECOIL_FASTCALL SolveUvAxisCoefficientsXZ(zVec3 *point0, zVec3 *point1,
                                                               zVec3 *point2, float value0,
                                                               float value1, float value2,
                                                               zVec2 *outCoefficients);
RECOIL_NOINLINE int RECOIL_FASTCALL TrySplitPointDwordOffsetsAtBestDiagonal(
    int pointCount, float *pointDwords, int *pointDwordOffsets,
    zGeometry_PolygonSplitDwordOffsetListPair *outSplitPointLists, int pointDwordStride);
RECOIL_NOINLINE zGeometry_TriangleDwordOffsetList *RECOIL_FASTCALL
TriangulatePointDwordOffsetsRecursive(int pointCount, float *pointDwords,
                                      int *pointDwordOffsets,
                                      int pointDwordStrideMode);
RECOIL_NOINLINE zGeometry_ConvexPolygonSetPartial *RECOIL_FASTCALL Convexify(
    zGeometry_PolygonSpanArrayPartial *polygonSet, int inputPointCount, zVec3 *points);
RECOIL_NOINLINE int RECOIL_FASTCALL
SnapPointsXYIfNear(zVec3 *polygon, int polyCount, zVec3 *targetVerts,
                   int targetCount, float vertexTolerance, float edgeTolerance);
} // namespace zGeometry_Polygon

namespace zGeometry_ConvexPolygonSet {
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_ConvexPolygonSetPartial *self);
}

namespace zGeometry_Model {
RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_CDECL FindOrCreateRandomDebugMaterial();
RECOIL_NOINLINE int RECOIL_FASTCALL AddPolygonToDi(zDiPartial *di, int pointCount,
                                                            zVec3 *points,
                                                            zModel_MaterialPartial *material,
                                                            zClipUV *uvPairs);
RECOIL_NOINLINE zClipUV *RECOIL_FASTCALL BuildPolygonUvList(int pointCount, zVec3 *points,
                                                            zModel_DrawBatchBasePartial *model,
                                                            zModel_PolygonPartial *polygon);
RECOIL_NOINLINE int RECOIL_FASTCALL
AddPointListPolygonToDi(zDiPartial *di, int pointCount, zVec3 *points,
                        zModel_DrawBatchBasePartial *model, zModel_PolygonPartial *polygon);
RECOIL_NOINLINE int RECOIL_FASTCALL AddIndexedPolygonToDi(
    zDiPartial *di, zModel_DrawBatchBasePartial *model, zModel_PolygonPartial *polygon);
RECOIL_NOINLINE zVec3 *RECOIL_FASTCALL GetLinearBufferOfPolygonVertices(
    zModel_DrawBatchBasePartial *model, zModel_PolygonPartial *polygon, zVec3 *points);
RECOIL_NOINLINE int RECOIL_FASTCALL IsFullyInsideClipPolygonXY(
    zGeometry_ClipPolygonPartial *clipPolygon, zModel_DrawBatchBasePartial *model);
RECOIL_NOINLINE int RECOIL_FASTCALL
ProcessClipPatchNode(zGeometry_ClipPolygonPartial *clipPolygon, zModel_DrawBatchBasePartial *model,
                     zDiPartial **outDi);
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPatch(int pointCount, zVec3 *points, zDEClient_FeatureGridCell *featureGridCell,
          zGeometry_ClipPatchOutputPartial *outClipPatchOutput);
} // namespace zGeometry_Model

namespace zGeometry_WeilerBuffer {
RECOIL_NOINLINE void RECOIL_FASTCALL Init(zGeometry_WeilerBufferPartial *self,
                                          int initialCapacity, int elementSize);
RECOIL_NOINLINE void *RECOIL_FASTCALL GetAppendSpace(zGeometry_WeilerBufferPartial *self,
                                                     int appendCount, void **outBase);
RECOIL_NOINLINE void RECOIL_FASTCALL SetCountAndAppendPtr(zGeometry_WeilerBufferPartial *self,
                                                          int count);
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_WeilerBufferPartial *self);
} // namespace zGeometry_WeilerBuffer

namespace zGeometry_Weiler {
RECOIL_NOINLINE int RECOIL_FASTCALL
GetInputContourAPointList(zGeometry_WeilerStatePartial *self, zVec3 **outPoints);
RECOIL_NOINLINE zGeometry_WeilerStatePartial *RECOIL_FASTCALL Init(zVec3 *points,
                                                                   int pointCount,
                                                                   int contourSource);
RECOIL_NOINLINE int RECOIL_FASTCALL
InitInputContourPair(zGeometry_WeilerStatePartial *self, zVec3 *points, int pointCount,
                     int contourType);
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPointList(zGeometry_WeilerStatePartial *self, int clipMode, zVec3 *points,
              int pointCount, zGeometry_WeilerClipOutputPartial *outClip);
RECOIL_NOINLINE int RECOIL_FASTCALL EnsureContourOutput(
    zGeometry_WeilerStatePartial *self, zGeometry_WeilerContourSegmentPartial *segment);
RECOIL_NOINLINE int RECOIL_FASTCALL MergeContours(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL DivideContourSegmentAtPoint(
    zGeometry_WeilerStatePartial *self, zVec3 *xing, zGeometry_WeilerContourSegmentPartial *segment,
    int updateSplitLinks);
RECOIL_NOINLINE int RECOIL_FASTCALL CreateForwardSegmentPairAtPoint(
    zGeometry_WeilerStatePartial *self, zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment, zVec3 *point,
    int firstContourTypeMask, int secondContourTypeMask);
RECOIL_NOINLINE zGeometry_WeilerContourSegmentPartial *RECOIL_FASTCALL
GetNextContourSegmentForTraversal(zGeometry_WeilerContourSegmentPartial *segment);
RECOIL_NOINLINE void RECOIL_FASTCALL NewContour(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL OutputContourToPolygonSet(
    zGeometry_WeilerStatePartial *self, zGeometry_WeilerContourOutputPartial *contour,
    zGeometry_WeilerBufferPartial *polygonBuffer, zGeometry_PolygonSpanArrayPartial *polygonSet);
RECOIL_NOINLINE int RECOIL_FASTCALL
OutputContoursForClipMode(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL SelectForwardStartPointInContourA(
    zVec3 *point, zVec3 **selectedPoint, zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL
GenerateOutsideResults(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL
ClassifyInputContourPairBounds(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL OutputPreclassifiedContourPairResult(
    int contourAPointCount, zVec3 *contourAPoints, int contourBPointCount,
    zVec3 *contourBPoints, int resultCode);
RECOIL_NOINLINE int RECOIL_FASTCALL
OutputSelectedInputContourToPolygonSetA(zGeometry_WeilerStatePartial *self, int mode);
RECOIL_NOINLINE void RECOIL_FASTCALL
PreclassifyInputContourAAdjacentEdgePairs(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL
BuildPointSideTablesForContourPair(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL
PreclassifyInputContourPair(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL
ClassifyContainedContour(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL
ClassifyIntersect2d(zVec3 *edge0Start, zVec3 *edge0End, zVec3 *edge1Start, zVec3 *edge1End,
                    zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL Intersect2d(zGeometry_WeilerStatePartial *self,
                                                         zGeometry_WeilerXingPartial **outXing,
                                                         zVec3 edge0Start, zVec3 edge0End,
                                                         zVec3 edge1Start, zVec3 edge1End);
RECOIL_NOINLINE int RECOIL_FASTCALL ClassifyAdjacentEdgePairAgainstContourSegment(
    zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment,
    zGeometry_WeilerContourSegmentPartial *contourSegment);
RECOIL_NOINLINE int RECOIL_FASTCALL ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
    zGeometry_WeilerContourSegmentPartial *pairAFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairASecondSegment,
    zGeometry_WeilerContourSegmentPartial *pairBFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairBSecondSegment, zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL ValidateXings(int xingCount,
                                                           zGeometry_WeilerXingPartial *xingArray,
                                                           int *failedXingIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL ClassifyPointInContourPointListXY(
    zVec3 *point, int contourPointCount, zVec3 *contourPoints);
RECOIL_NOINLINE void RECOIL_FASTCALL
TogglePointAxesForContourSource(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL
RecenterPointSetsIfOutOfRange(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL RestorePointTranslation(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL
RestoreOutputZFromInputPlane(zGeometry_WeilerStatePartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL DestroyState(zGeometry_WeilerStatePartial *self);
} // namespace zGeometry_Weiler

namespace zGeometry_WeilerClipOutput {
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_WeilerClipOutputPartial *self);
}

namespace zGeometry_WeilerContourSegment {
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateBounds(zGeometry_WeilerContourSegmentPartial *segment);
}

namespace zGeometry_WeilerContourSegmentArray {
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateBounds(zGeometry_WeilerContourSegmentPartial *segments,
                                                  int segmentCount);
RECOIL_NOINLINE void RECOIL_FASTCALL
InitFromPointList(zGeometry_WeilerContourSegmentPartial *segments, zVec3 *points,
                  int pointCount, int contourType);
} // namespace zGeometry_WeilerContourSegmentArray

namespace zGeometry_ClipPolygon {
RECOIL_NOINLINE int RECOIL_FASTCALL
FindPointIndexXY(zGeometry_ClipPolygonPartial *clipPolygon, zVec3 *point);
RECOIL_NOINLINE int RECOIL_FASTCALL
FindPointInsertionEdgeXYIndex(zGeometry_ClipPolygonPartial *clipPolygon, zVec3 *point);
RECOIL_NOINLINE int RECOIL_FASTCALL UpsertPointListXY(
    zGeometry_ClipPolygonPartial *clipPolygon, int pointCount, zVec3 *points);
RECOIL_NOINLINE int RECOIL_FASTCALL ResetWeilerStateFromContourPoints(
    zGeometry_ClipPolygonPartial *clipPolygon, zVec3 *points, int pointCount);
RECOIL_NOINLINE zGeometry_ClipPolygonPartial *RECOIL_FASTCALL
CreateFromPointList(int pointCount, zVec3 *points);
RECOIL_NOINLINE int RECOIL_FASTCALL SnapPointsNearNodeModelXY(
    zGeometry_ClipPolygonPartial *clipPolygon, zGeometry_ClipPatchNodeView *node);
RECOIL_NOINLINE int RECOIL_FASTCALL
ProcessNodePolygonSetXY(zGeometry_ClipPolygonPartial *clipPolygon,
                        zGeometry_ClipPatchNodeView *node, zDiPartial **outDi);
RECOIL_NOINLINE int RECOIL_FASTCALL CopyPointsOutRotatedBack(
    zGeometry_ClipPolygonPartial *clipPolygon, int *outPointCount, zVec3 **outPoints);
RECOIL_NOINLINE void RECOIL_FASTCALL FinalizeAndDestroy(zGeometry_ClipPolygonPartial *clipPolygon);
} // namespace zGeometry_ClipPolygon

namespace zGeometry_ClipPatchOutput {
RECOIL_NOINLINE zGeometry_ClipPatchOutputPartial *RECOIL_CDECL Create();
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_ClipPatchOutputPartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyNodeDiPairs(zGeometry_ClipPatchOutputPartial *self);
} // namespace zGeometry_ClipPatchOutput
