#ifndef RECOIL_GAMEZRECOIL_ZGEOMETRY_ZGEOMETRY_H
#define RECOIL_GAMEZRECOIL_ZGEOMETRY_ZGEOMETRY_H

#pragma once

#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zMath/zMathDecls.h"
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
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerBufferPartial,
        base
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerBufferPartial) == 0x14);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        contourType
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        startPoint
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        endPoint
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        boundsDirty
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        minX
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        minY
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        maxX
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        maxY
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        startXing
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        endXing
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerContourSegmentPartial,
        contourOutput
    ) == 0x38
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerContourSegmentPartial) == 0x3c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerXingPartial,
        point
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerXingPartial,
        segment0
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerXingPartial,
        segment7
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerXingPartial,
        xingType
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerXingPartial) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerContourOutputPartial) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_PolygonPointSpanPartial) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_PolygonSpanArrayPartial) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ConvexPolygonSetPartial,
        polygons
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ConvexPolygonSetPartial,
        totalPointCount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ConvexPolygonSetPartial,
        points
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ConvexPolygonSetPartial) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_TriangleDwordOffsetList,
        triangleDwordOffsets
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_TriangleIndexTriple) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_TriangulateHole_EdgeState) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_PlaneEquationPartial) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_TriangleSoup,
        triangleVerts
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_PolygonSplitDwordOffsetListPair,
        pointCount1
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_PolygonSplitDwordOffsetListPair,
        pointDwordOffsets
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerClipOutputPartial,
        polygonSetB
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerClipOutputPartial,
        polygonSetC
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerClipOutputPartial,
        pointList
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerClipOutputPartial) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPolygonPartial,
        points
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPolygonPartial,
        pointCount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPolygonPartial,
        bounds
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPolygonPartial) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        clipMode
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        contourSource
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        outClip
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        inputContourABuffer
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        inputContourBBuffer
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        segmentBuffer
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        contourBuffer
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        xingBuffer
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        polygonSetABuffer
    ) == 0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        polygonSetBBuffer
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        polygonSetCBuffer
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        pointListBuffer
    ) == 0xac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        contourAPointSideByContourBEdge
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        contourBPointSideByContourAEdge
    ) == 0x14c0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        pointTranslationX
    ) == 0x28c0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        pointTranslationY
    ) == 0x28c4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        pointsRecentered
    ) == 0x28c8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_WeilerStatePartial,
        allContoursSingleSided
    ) == 0x28c9
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_WeilerStatePartial) == 0x28cc);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPatchNodeDiPair) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPatchPartitionOutput,
        nodeDiPairs
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPatchPartitionOutput,
        featureGridCell
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPatchPartitionOutput) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zGeometry_ClipPatchOutputPartial) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PolygonPartial,
        vertexIndices
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PolygonPartial,
        uvBasis
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PolygonPartial,
        material
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PolygonPartial,
        userTag
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zModel_PolygonPartial) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_DrawBatchBasePartial,
        faceCount
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_DrawBatchBasePartial,
        faceList
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_DrawBatchBasePartial,
        verts
    ) == 0x34
);

namespace zGeometry_Vec3Array {
int __fastcall RemoveAdjacentDuplicatePointsXY(
    zVec3 *vertices,
    int count
);
void __fastcall RotateNeg90AroundX(
    int pointCount,
    zVec3 *points
);
void __fastcall RotatePos90AroundX(
    int pointCount,
    zVec3 *points
);
void __fastcall ComputeBoundsXY(
    zGeometry_BoundsXY *outBounds,
    zVec3 *points,
    int pointCount
);
void __fastcall ReversePoints(
    int pointCount,
    zVec3 *points
);
int __fastcall EnsurePositiveCrossZ(
    int pointCount,
    zVec3 *points,
    int allowReverse
);
void __fastcall ComputeNewellPlane(
    int pointCount,
    zVec3 *points,
    zGeometry_PlaneEquationPartial *outPlane
);
} // namespace zGeometry_Vec3Array

namespace zGeometry_Segment {
int __fastcall IntersectsSegmentXY(
    zVec3 *segmentAPoint0,
    zVec3 *segmentAPoint1,
    zVec3 *segmentBPoint0,
    zVec3 *segmentBPoint1
);
}

namespace zGeometry_TriangulateHole {
int __fastcall TryAppendBridgeEdge(
    zGeometry_TriangulateHole_EdgeState *edgeState,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates
);
int __fastcall CollectActiveEdgeIndicesForVertex(
    int vertexIndex,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates,
    int *outEdgeIndices
);
void __fastcall TryEmitTriangleFromEdgePair(
    int edgeIndex0,
    int edgeIndex1,
    int vertexIndex,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates
);
zGeometry_TriangulateHole_EdgeState *__fastcall FindActiveEdgeState(
    int vertexIndex0,
    int vertexIndex1,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates
);
void __fastcall CacheCombinedPlane(
    int pointCount,
    zVec3 *points
);
void __fastcall ProjectInnerRingOntoCachedPlane(
    int pointCount,
    zVec3 *points
);
} // namespace zGeometry_TriangulateHole

namespace zGeometry {
zGeometry_TriangleSoup *__fastcall TriangulatePolygonWithHole(
    int outerPointCount,
    zVec3 *outerPoints,
    int innerPointCount,
    zVec3 *innerPoints
);
}

namespace zGeometry_Bounds2D {
int __fastcall OverlapsWithUnitMargin(
    zGeometry_BoundsXY *boundsA,
    zGeometry_BoundsXY *boundsB
);
}

namespace zGeometry_Vec3 {
int __fastcall IsNearEqualXY(
    zVec3 *vecA,
    zVec3 *vecB,
    float tolerance
);
int __fastcall SnapPointToSegmentXYIfNear(
    zVec3 *lineStart,
    zVec3 *lineEnd,
    zVec3 *testPoint,
    float tolerance
);
int __fastcall IsBetweenEndpointsXY(
    zVec3 *testPoint,
    zVec3 *startPoint,
    zVec3 *endPoint
);
} // namespace zGeometry_Vec3

namespace zGeometry_Polygon {
void __fastcall SolveUvAxisCoefficientsXZ(
    zVec3 *point0,
    zVec3 *point1,
    zVec3 *point2,
    float value0,
    float value1,
    float value2,
    zVec2 *outCoefficients
);
int __fastcall TrySplitPointDwordOffsetsAtBestDiagonal(
    int pointCount,
    float *pointDwords,
    int *pointDwordOffsets,
    zGeometry_PolygonSplitDwordOffsetListPair *outSplitPointLists,
    int pointDwordStride
);
zGeometry_TriangleDwordOffsetList *__fastcall
TriangulatePointDwordOffsetsRecursive(
    int pointCount,
    float *pointDwords,
    int *pointDwordOffsets,
    int pointDwordStrideMode
);
zGeometry_ConvexPolygonSetPartial *__fastcall Convexify(
    zGeometry_PolygonSpanArrayPartial *polygonSet,
    int inputPointCount,
    zVec3 *points
);
int __fastcall SnapPointsXYIfNear(
    zVec3 *polygon,
    int polyCount,
    zVec3 *targetVerts,
    int targetCount,
    float vertexTolerance,
    float edgeTolerance
);
} // namespace zGeometry_Polygon

namespace zGeometry_ConvexPolygonSet {
void __fastcall Destroy(zGeometry_ConvexPolygonSetPartial *self);
}

namespace zGeometry_Model {
zModel_MaterialPartial *FindOrCreateRandomDebugMaterial();
int __fastcall AddPolygonToDi(
    zDiPartial *di,
    int pointCount,
    zVec3 *points,
    zModel_MaterialPartial *material,
    zClipUV *uvPairs
);
zClipUV *__fastcall BuildPolygonUvList(
    int pointCount,
    zVec3 *points,
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon
);
int __fastcall AddPointListPolygonToDi(
    zDiPartial *di,
    int pointCount,
    zVec3 *points,
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon
);
int __fastcall AddIndexedPolygonToDi(
    zDiPartial *di,
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon
);
zVec3 *__fastcall GetLinearBufferOfPolygonVertices(
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon,
    zVec3 *points
);
int __fastcall IsFullyInsideClipPolygonXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zModel_DrawBatchBasePartial *model
);
int __fastcall ProcessClipPatchNode(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zModel_DrawBatchBasePartial *model,
    zDiPartial **outDi
);
int __fastcall ClipPatch(
    int pointCount,
    zVec3 *points,
    zDEClient_FeatureGridCell *featureGridCell,
    zGeometry_ClipPatchOutputPartial *outClipPatchOutput
);
} // namespace zGeometry_Model

namespace zGeometry_WeilerBuffer {
void __fastcall Init(
    zGeometry_WeilerBufferPartial *self,
    int initialCapacity,
    int elementSize
);
void *__fastcall GetAppendSpace(
    zGeometry_WeilerBufferPartial *self,
    int appendCount,
    void **outBase
);
void __fastcall SetCountAndAppendPtr(
    zGeometry_WeilerBufferPartial *self,
    int count
);
void __fastcall Destroy(zGeometry_WeilerBufferPartial *self);
} // namespace zGeometry_WeilerBuffer

namespace zGeometry_Weiler {
int __fastcall GetInputContourAPointList(
    zGeometry_WeilerStatePartial *self,
    zVec3 **outPoints
);
zGeometry_WeilerStatePartial *__fastcall Init(
    zVec3 *points,
    int pointCount,
    int contourSource
);
int __fastcall InitInputContourPair(
    zGeometry_WeilerStatePartial *self,
    zVec3 *points,
    int pointCount,
    int contourType
);
int __fastcall ClipPointList(
    zGeometry_WeilerStatePartial *self,
    int clipMode,
    zVec3 *points,
    int pointCount,
    zGeometry_WeilerClipOutputPartial *outClip
);
int __fastcall EnsureContourOutput(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerContourSegmentPartial *segment
);
int __fastcall MergeContours(zGeometry_WeilerStatePartial *self);
int __fastcall DivideContourSegmentAtPoint(
    zGeometry_WeilerStatePartial *self,
    zVec3 *xing,
    zGeometry_WeilerContourSegmentPartial *segment,
    int updateSplitLinks
);
int __fastcall CreateForwardSegmentPairAtPoint(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment,
    zVec3 *point,
    int firstContourTypeMask,
    int secondContourTypeMask
);
zGeometry_WeilerContourSegmentPartial *__fastcall
GetNextContourSegmentForTraversal(zGeometry_WeilerContourSegmentPartial *segment);
void __fastcall NewContour(zGeometry_WeilerStatePartial *self);
int __fastcall OutputContourToPolygonSet(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerContourOutputPartial *contour,
    zGeometry_WeilerBufferPartial *polygonBuffer,
    zGeometry_PolygonSpanArrayPartial *polygonSet
);
int __fastcall OutputContoursForClipMode(zGeometry_WeilerStatePartial *self);
void __fastcall SelectForwardStartPointInContourA(
    zVec3 *point,
    zVec3 **selectedPoint,
    zGeometry_WeilerStatePartial *self
);
int __fastcall GenerateOutsideResults(zGeometry_WeilerStatePartial *self);
int __fastcall ClassifyInputContourPairBounds(
    zGeometry_WeilerStatePartial *self
);
int __fastcall OutputPreclassifiedContourPairResult(
    int contourAPointCount,
    zVec3 *contourAPoints,
    int contourBPointCount,
    zVec3 *contourBPoints,
    int resultCode
);
int __fastcall OutputSelectedInputContourToPolygonSetA(
    zGeometry_WeilerStatePartial *self,
    int mode
);
void __fastcall PreclassifyInputContourAAdjacentEdgePairs(
    zGeometry_WeilerStatePartial *self
);
void __fastcall BuildPointSideTablesForContourPair(
    zGeometry_WeilerStatePartial *self
);
int __fastcall PreclassifyInputContourPair(zGeometry_WeilerStatePartial *self);
int __fastcall ClassifyContainedContour(zGeometry_WeilerStatePartial *self);
int __fastcall ClassifyIntersect2d(
    zVec3 *edge0Start,
    zVec3 *edge0End,
    zVec3 *edge1Start,
    zVec3 *edge1End,
    zGeometry_WeilerStatePartial *self
);
int __fastcall Intersect2d(
    zGeometry_WeilerStatePartial *self,
    zGeometry_WeilerXingPartial **outXing,
    zVec3 edge0Start,
    zVec3 edge0End,
    zVec3 edge1Start,
    zVec3 edge1End
);
int __fastcall ClassifyAdjacentEdgePairAgainstContourSegment(
    zGeometry_WeilerContourSegmentPartial *firstSegment,
    zGeometry_WeilerContourSegmentPartial *secondSegment,
    zGeometry_WeilerContourSegmentPartial *contourSegment
);
int __fastcall ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
    zGeometry_WeilerContourSegmentPartial *pairAFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairASecondSegment,
    zGeometry_WeilerContourSegmentPartial *pairBFirstSegment,
    zGeometry_WeilerContourSegmentPartial *pairBSecondSegment,
    zGeometry_WeilerStatePartial *self
);
int __fastcall ValidateXings(
    int xingCount,
    zGeometry_WeilerXingPartial *xingArray,
    int *failedXingIndex
);
int __fastcall ClassifyPointInContourPointListXY(
    zVec3 *point,
    int contourPointCount,
    zVec3 *contourPoints
);
void __fastcall TogglePointAxesForContourSource(
    zGeometry_WeilerStatePartial *self
);
void __fastcall RecenterPointSetsIfOutOfRange(
    zGeometry_WeilerStatePartial *self
);
void __fastcall RestorePointTranslation(zGeometry_WeilerStatePartial *self);
void __fastcall RestoreOutputZFromInputPlane(
    zGeometry_WeilerStatePartial *self
);
void __fastcall DestroyState(zGeometry_WeilerStatePartial *self);
} // namespace zGeometry_Weiler

namespace zGeometry_WeilerClipOutput {
void __fastcall Destroy(zGeometry_WeilerClipOutputPartial *self);
}

namespace zGeometry_WeilerContourSegment {
void __fastcall UpdateBounds(zGeometry_WeilerContourSegmentPartial *segment);
}

namespace zGeometry_WeilerContourSegmentArray {
void __fastcall UpdateBounds(
    zGeometry_WeilerContourSegmentPartial *segments,
    int segmentCount
);
void __fastcall InitFromPointList(
    zGeometry_WeilerContourSegmentPartial *segments,
    zVec3 *points,
    int pointCount,
    int contourType
);
} // namespace zGeometry_WeilerContourSegmentArray

namespace zGeometry_ClipPolygon {
int __fastcall FindPointIndexXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zVec3 *point
);
int __fastcall FindPointInsertionEdgeXYIndex(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zVec3 *point
);
int __fastcall UpsertPointListXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    int pointCount,
    zVec3 *points
);
int __fastcall ResetWeilerStateFromContourPoints(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zVec3 *points,
    int pointCount
);
zGeometry_ClipPolygonPartial *__fastcall CreateFromPointList(
    int pointCount,
    zVec3 *points
);
int __fastcall SnapPointsNearNodeModelXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zGeometry_ClipPatchNodeView *node
);
int __fastcall ProcessNodePolygonSetXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zGeometry_ClipPatchNodeView *node,
    zDiPartial **outDi
);
int __fastcall CopyPointsOutRotatedBack(
    zGeometry_ClipPolygonPartial *clipPolygon,
    int *outPointCount,
    zVec3 **outPoints
);
void __fastcall FinalizeAndDestroy(zGeometry_ClipPolygonPartial *clipPolygon);
} // namespace zGeometry_ClipPolygon

namespace zGeometry_ClipPatchOutput {
zGeometry_ClipPatchOutputPartial *Create();
void __fastcall Destroy(zGeometry_ClipPatchOutputPartial *self);
int __fastcall ApplyNodeDiPairs(zGeometry_ClipPatchOutputPartial *self);
} // namespace zGeometry_ClipPatchOutput

#endif
