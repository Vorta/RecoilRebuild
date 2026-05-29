#include "zGeometry.h"

#include "GameZRecoil/zError/zError.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
const char kZGeoConvexifySourceFile[] = "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_convexify.cpp";
const int kTriangulateHoleMaxTriangles = 0x400;

zVec3 *g_zGeometry_TriangulateHole_CombinedPoints = 0;
int g_zGeometry_TriangulateHole_TriangleCount = 0;
int g_zGeometry_TriangulateHole_CombinedPointCount = 0;
zGeometry_TriangleIndexTriple
    g_zGeometry_TriangulateHole_TriangleIndices[kTriangulateHoleMaxTriangles];
zGeometry_PlaneEquationPartial g_zGeometry_TriangulateHole_CachedPlane;

float OffsetX(const float *pointDwords, const int *pointDwordOffsets, int index,
              int stride) {
    return pointDwords[pointDwordOffsets[index * stride]];
}

float OffsetY(const float *pointDwords, const int *pointDwordOffsets, int index,
              int stride) {
    return pointDwords[pointDwordOffsets[index * stride + 1]];
}

float Cross2D(float ax, float ay, float bx, float by, float cx, float cy) {
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

float PolygonArea2D(const float *pointDwords, const int *pointDwordOffsets,
                    int pointCount, int stride) {
    float area = 0.0f;
    for (int i = 0; i < pointCount; ++i) {
        const int next = (i + 1) % pointCount;
        area += OffsetX(pointDwords, pointDwordOffsets, i, stride) *
                    OffsetY(pointDwords, pointDwordOffsets, next, stride) -
                OffsetY(pointDwords, pointDwordOffsets, i, stride) *
                    OffsetX(pointDwords, pointDwordOffsets, next, stride);
    }

    return area;
}

bool PointInTriangle2D(float px, float py, float ax, float ay, float bx, float by, float cx,
                       float cy, bool ccw) {
    const float cross0 = Cross2D(ax, ay, bx, by, px, py);
    const float cross1 = Cross2D(bx, by, cx, cy, px, py);
    const float cross2 = Cross2D(cx, cy, ax, ay, px, py);

    if (ccw) {
        return cross0 >= 0.0f && cross1 >= 0.0f && cross2 >= 0.0f;
    }

    return cross0 <= 0.0f && cross1 <= 0.0f && cross2 <= 0.0f;
}

void CopyOffsetVertex(int *dest, const int *source, int stride) {
    memcpy(dest, source, (size_t)(stride) * sizeof(int));
}

const float *PointDwordBase(const zVec3 *points, int pointDwordOffset) {
    return (const float *)(points) + pointDwordOffset;
}

zVec3 *CopySpanPoints(zGeometry_ConvexPolygonSetPartial *result, zVec3 *outputPointWriteCursor,
                      const float *sourcePointDwords, int pointCount) {
    zGeometry_PolygonPointSpanPartial *polygon = &result->polygons[result->polygonCount];
    polygon->pointCount = pointCount;
    polygon->pointDwordOffset = result->totalPointCount * 3;

    memcpy(outputPointWriteCursor, sourcePointDwords,
                (size_t)(pointCount) * sizeof(zVec3));

    ++result->polygonCount;
    result->totalPointCount += pointCount;
    return outputPointWriteCursor + pointCount;
}

bool IsConvexQuadXY(const zVec3 *points) {
    int sign = 0;
    for (int i = 0; i < 4; ++i) {
        const zVec3 &a = points[i];
        const zVec3 &b = points[(i + 1) & 3];
        const zVec3 &c = points[(i + 2) & 3];
        const float cross = Cross2D(a.x, a.y, b.x, b.y, c.x, c.y);
        if (cross == 0.0f) {
            continue;
        }

        const int thisSign = cross > 0.0f ? 1 : -1;
        if (sign != 0 && sign != thisSign) {
            return false;
        }

        sign = thisSign;
    }

    return true;
}

zVec3 *AppendTriangulatedSpan(zGeometry_ConvexPolygonSetPartial *result,
                              zVec3 *outputPointWriteCursor,
                              const zGeometry_PolygonPointSpanPartial *inputPolygon,
                              const zVec3 *allPoints) {
    const float *sourcePointDwords = PointDwordBase(allPoints, inputPolygon->pointDwordOffset);
    zGeometry_TriangleDwordOffsetList *triangles =
        zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive(
            inputPolygon->pointCount, (float *)(sourcePointDwords), 0, 0);

    if (triangles == 0) {
        return outputPointWriteCursor;
    }

    const int *triangleOffsets = triangles->triangleDwordOffsets;
    {
    for (int triangle = 0; triangle < triangles->triangleCount; ++triangle) {
        zGeometry_PolygonPointSpanPartial *polygon = &result->polygons[result->polygonCount];
        polygon->pointCount = 3;
        polygon->pointDwordOffset = result->totalPointCount * 3;
        ++result->polygonCount;
        result->totalPointCount += 3;

        float *outputDwords = (float *)(outputPointWriteCursor);
        {
        for (int dwordIndex = 0; dwordIndex < 9; ++dwordIndex) {
            outputDwords[dwordIndex] =
                sourcePointDwords[triangleOffsets[triangle * 9 + dwordIndex]];
        }
        }

        outputPointWriteCursor += 3;
    }
    }

    free(triangles);
    return outputPointWriteCursor;
}

int *TrianglePayload(zGeometry_TriangleDwordOffsetList *list) {
    return list->triangleDwordOffsets;
}

void AppendTriangleOffsets(zGeometry_TriangleDwordOffsetList *list, int triangleIndex,
                           const int *polygonOffsets, int index0,
                           int index1, int index2, int stride) {
    int *out = &TrianglePayload(list)[triangleIndex * stride * 3];
    CopyOffsetVertex(out, &polygonOffsets[index0 * stride], stride);
    CopyOffsetVertex(out + stride, &polygonOffsets[index1 * stride], stride);
    CopyOffsetVertex(out + stride * 2, &polygonOffsets[index2 * stride], stride);
}

bool IsEar(const float *pointDwords, const int *pointDwordOffsets, int pointCount,
           int stride, int prev, int curr, int next, bool ccw) {
    const float ax = OffsetX(pointDwords, pointDwordOffsets, prev, stride);
    const float ay = OffsetY(pointDwords, pointDwordOffsets, prev, stride);
    const float bx = OffsetX(pointDwords, pointDwordOffsets, curr, stride);
    const float by = OffsetY(pointDwords, pointDwordOffsets, curr, stride);
    const float cx = OffsetX(pointDwords, pointDwordOffsets, next, stride);
    const float cy = OffsetY(pointDwords, pointDwordOffsets, next, stride);
    const float cross = Cross2D(ax, ay, bx, by, cx, cy);

    if (ccw) {
        if (cross <= 0.0f) {
            return false;
        }
    } else if (cross >= 0.0f) {
        return false;
    }

    for (int i = 0; i < pointCount; ++i) {
        if (i == prev || i == curr || i == next) {
            continue;
        }

        if (PointInTriangle2D(OffsetX(pointDwords, pointDwordOffsets, i, stride),
                              OffsetY(pointDwords, pointDwordOffsets, i, stride), ax, ay, bx, by,
                              cx, cy, ccw)) {
            return false;
        }
    }

    return true;
}

float EstimateMagnitudeFromSquaredLength(float squaredLength) {
    union {
        float value;
        int bits;
    } estimate;

    estimate.value = squaredLength;
    estimate.bits = (estimate.bits >> 1) + 0x1fc00000;
    return estimate.value;
}
} // namespace

namespace zGeometry_Vec3Array {
// Reimplements 0x46c5b0: zGeometry_Vec3Array::ReversePoints
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ReversePoints(int pointCount, zVec3 *points) {
    zVec3 *front = &points[1];
    zVec3 *back = &points[pointCount - 1];
    const int swapCount = pointCount / 2;

    for (int i = 0; i < swapCount; ++i) {
        const zVec3 temp = *back;
        *back = *front;
        *front = temp;
        --back;
        ++front;
    }
}

// Reimplements 0x46c620: zGeometry_Vec3Array::EnsurePositiveCrossZ
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL EnsurePositiveCrossZ(int pointCount,
                                                                  zVec3 *points,
                                                                  int allowReverse) {
    const zVec3 edge0 = {points[1].x - points[0].x, points[1].y - points[0].y,
                      points[1].z - points[0].z};
    const zVec3 edge1 = {points[2].x - points[1].x, points[2].y - points[1].y,
                      points[2].z - points[1].z};
    const zVec3 cross = {edge0.y * edge1.z - edge0.z * edge1.y, edge0.z * edge1.x - edge0.x * edge1.z,
                      edge0.x * edge1.y - edge0.y * edge1.x};

    if (!(cross.z > 0.0f)) {
        if (allowReverse == 0) {
            return 0;
        }

        ReversePoints(pointCount, points);
    }

    return 1;
}

// Reimplements 0x46c3a0: zGeometry_Vec3Array::ComputeNewellPlane
// (D:\Proj\GameZRecoil\zGeometry\zgeometry.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ComputeNewellPlane(int pointCount, zVec3 *points,
                                                        zGeometry_PlaneEquationPartial *outPlane) {
    float normalX = 0.0f;
    float normalY = 0.0f;
    float normalZ = 0.0f;
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;

    for (int i = 0; i < pointCount; ++i) {
        zVec3 *const point = &points[i];
        zVec3 *const next = &points[(i + 1) % pointCount];

        normalX += (point->y - next->y) * (point->z + next->z);
        normalY += (point->z - next->z) * (point->x + next->x);
        normalZ += (point->x - next->x) * (point->y + next->y);

        sumX += point->x;
        sumY += point->y;
        sumZ += point->z;
    }

    float estimatedMagnitude = 0.0f;
    if (normalX != 0.0f || normalY != 0.0f || normalZ != 0.0f) {
        const float squaredLength = normalX * normalX + normalY * normalY + normalZ * normalZ;
        estimatedMagnitude = EstimateMagnitudeFromSquaredLength(squaredLength);
    }

    float normalScale = 0.0f;
    if (estimatedMagnitude != 0.0f) {
        normalScale = 1.0f / estimatedMagnitude;
    }

    outPlane->a = normalX * normalScale;
    outPlane->b = normalY * normalScale;
    outPlane->c = normalZ * normalScale;
    outPlane->d = -((sumX * normalX + sumY * normalY + sumZ * normalZ) /
                    ((float)(pointCount) * estimatedMagnitude));
}
} // namespace zGeometry_Vec3Array

namespace zGeometry_TriangulateHole {
// Reimplements 0x46bf70:
// zGeometry_TriangulateHole::FindActiveEdgeState
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE zGeometry_TriangulateHole_EdgeState *RECOIL_FASTCALL
FindActiveEdgeState(int vertexIndex0, int vertexIndex1, int edgeCount,
                    zGeometry_TriangulateHole_EdgeState *edgeStates) {
    for (int i = 0; i < edgeCount; ++i) {
        zGeometry_TriangulateHole_EdgeState *const edge = &edgeStates[i];
        if ((edge->vertexIndex0 == vertexIndex0 && edge->vertexIndex1 == vertexIndex1) ||
            (edge->vertexIndex0 == vertexIndex1 && edge->vertexIndex1 == vertexIndex0)) {
            if (edge->remainingUseCount != 0) {
                return edge;
            }

            return 0;
        }
    }

    return 0;
}

// Reimplements 0x46bd50:
// zGeometry_TriangulateHole::TryAppendBridgeEdge
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
TryAppendBridgeEdge(zGeometry_TriangulateHole_EdgeState *edgeState, int edgeCount,
                    zGeometry_TriangulateHole_EdgeState *edgeStates) {
    if (FindActiveEdgeState(edgeState->vertexIndex0, edgeState->vertexIndex1, edgeCount,
                            edgeStates) != 0) {
        return edgeCount;
    }

    zVec3 *const bridgeStart = &g_zGeometry_TriangulateHole_CombinedPoints[edgeState->vertexIndex0];
    zVec3 *const bridgeEnd = &g_zGeometry_TriangulateHole_CombinedPoints[edgeState->vertexIndex1];

    for (int i = 0; i < edgeCount; ++i) {
        zGeometry_TriangulateHole_EdgeState *const edge = &edgeStates[i];
        if (edge->vertexIndex0 == edgeState->vertexIndex0 ||
            edge->vertexIndex1 == edgeState->vertexIndex0 ||
            edge->vertexIndex0 == edgeState->vertexIndex1 ||
            edge->vertexIndex1 == edgeState->vertexIndex1) {
            continue;
        }

        if (zGeometry_Segment::IntersectsSegmentXY(
                bridgeStart, bridgeEnd,
                &g_zGeometry_TriangulateHole_CombinedPoints[edge->vertexIndex0],
                &g_zGeometry_TriangulateHole_CombinedPoints[edge->vertexIndex1]) != 0) {
            return edgeCount;
        }
    }

    edgeStates[edgeCount] = *edgeState;
    return edgeCount + 1;
}

// Reimplements 0x46bf30:
// zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CollectActiveEdgeIndicesForVertex(
    int vertexIndex, int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates, int *outEdgeIndices) {
    int result = 0;
    for (int i = 0; i < edgeCount; ++i) {
        zGeometry_TriangulateHole_EdgeState *const edge = &edgeStates[i];
        if (edge->remainingUseCount != 0 &&
            (edge->vertexIndex0 == vertexIndex || edge->vertexIndex1 == vertexIndex)) {
            outEdgeIndices[result] = i;
            ++result;
        }
    }

    return result;
}

// Reimplements 0x46bfc0:
// zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL TryEmitTriangleFromEdgePair(
    int edgeIndex0, int edgeIndex1, int vertexIndex,
    int edgeCount, zGeometry_TriangulateHole_EdgeState *edgeStates) {
    zGeometry_TriangulateHole_EdgeState *const edge0 = &edgeStates[edgeIndex0];
    zGeometry_TriangulateHole_EdgeState *const edge1 = &edgeStates[edgeIndex1];

    if (edge0->remainingUseCount == 0 || edge1->remainingUseCount == 0) {
        return;
    }

    int vertexIndex0 = edge0->vertexIndex0;
    if (vertexIndex0 == vertexIndex) {
        vertexIndex0 = edge0->vertexIndex1;
    }

    int vertexIndex1 = edge1->vertexIndex0;
    if (vertexIndex1 == vertexIndex) {
        vertexIndex1 = edge1->vertexIndex1;
    }

    if (vertexIndex0 + vertexIndex1 + vertexIndex == 3) {
        return;
    }

    zGeometry_TriangulateHole_EdgeState *const closingEdge =
        FindActiveEdgeState(vertexIndex0, vertexIndex1, edgeCount, edgeStates);
    if (closingEdge == 0) {
        return;
    }

    zGeometry_TriangleIndexTriple *const triangle =
        &g_zGeometry_TriangulateHole_TriangleIndices[g_zGeometry_TriangulateHole_TriangleCount];
    triangle->i0 = vertexIndex0;
    triangle->i1 = vertexIndex1;
    triangle->i2 = vertexIndex;
    ++g_zGeometry_TriangulateHole_TriangleCount;

    --edge0->remainingUseCount;
    --edge1->remainingUseCount;
    --closingEdge->remainingUseCount;
}

// Reimplements 0x46c390:
// zGeometry_TriangulateHole::CacheCombinedPlane
// (D:\Proj\GameZRecoil\zGeometry\zgeometry.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL CacheCombinedPlane(int pointCount, zVec3 *points) {
    zGeometry_Vec3Array::ComputeNewellPlane(pointCount, points,
                                            &g_zGeometry_TriangulateHole_CachedPlane);
}

// Reimplements 0x46c570:
// zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane
// (D:\Proj\GameZRecoil\zGeometry\zgeometry.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ProjectInnerRingOntoCachedPlane(int pointCount,
                                                                     zVec3 *points) {
    for (int i = 0; i < pointCount; ++i) {
        zVec3 *const point = &points[i];
        point->z = -(g_zGeometry_TriangulateHole_CachedPlane.a * point->x +
                     g_zGeometry_TriangulateHole_CachedPlane.b * point->y +
                     g_zGeometry_TriangulateHole_CachedPlane.d) /
                   g_zGeometry_TriangulateHole_CachedPlane.c;
    }
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry {
// Reimplements 0x46c070: zGeometry::TriangulatePolygonWithHole
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE zGeometry_TriangleSoup *RECOIL_FASTCALL
TriangulatePolygonWithHole(int outerPointCount, zVec3 *outerPoints,
                           int innerPointCount, zVec3 *innerPoints) {
    const int combinedPointCount = outerPointCount + innerPointCount;

    g_zGeometry_TriangulateHole_TriangleCount = 0;
    g_zGeometry_TriangulateHole_CombinedPointCount = combinedPointCount;

    zGeometry_TriangulateHole_EdgeState *const edgeStates =
        (zGeometry_TriangulateHole_EdgeState *)(
            malloc((size_t)(combinedPointCount * combinedPointCount) *
                        sizeof(zGeometry_TriangulateHole_EdgeState)));

    g_zGeometry_TriangulateHole_CombinedPoints = (zVec3 *)(
        malloc((size_t)(combinedPointCount) * sizeof(zVec3)));

    memcpy(g_zGeometry_TriangulateHole_CombinedPoints, outerPoints,
                (size_t)(outerPointCount) * sizeof(zVec3));
    memcpy(&g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount], innerPoints,
                (size_t)(innerPointCount) * sizeof(zVec3));

    zGeometry_TriangulateHole::CacheCombinedPlane(outerPointCount, outerPoints);
    zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane(
        innerPointCount, &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount]);
    memcpy(innerPoints, &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount],
                (size_t)(innerPointCount) * sizeof(zVec3));

    int edgeCount = 0;
    edgeStates[edgeCount].vertexIndex0 = 0;
    edgeStates[edgeCount].vertexIndex1 = outerPointCount - 1;
    edgeStates[edgeCount].remainingUseCount = 1;
    ++edgeCount;

    {
    for (int outerIndex = 1; outerIndex < outerPointCount; ++outerIndex) {
        edgeStates[edgeCount].vertexIndex0 = outerIndex - 1;
        edgeStates[edgeCount].vertexIndex1 = outerIndex;
        edgeStates[edgeCount].remainingUseCount = 1;
        ++edgeCount;
    }
    }

    edgeStates[edgeCount].vertexIndex0 = outerPointCount;
    edgeStates[edgeCount].vertexIndex1 = combinedPointCount - 1;
    edgeStates[edgeCount].remainingUseCount = 1;
    ++edgeCount;

    {
    for (int innerIndex = 1; innerIndex < innerPointCount; ++innerIndex) {
        edgeStates[edgeCount].vertexIndex0 = outerPointCount + innerIndex - 1;
        edgeStates[edgeCount].vertexIndex1 = outerPointCount + innerIndex;
        edgeStates[edgeCount].remainingUseCount = 1;
        ++edgeCount;
    }
    }

    zGeometry_TriangulateHole_EdgeState bridgeEdge;
    bridgeEdge.remainingUseCount = 2;
    {
    for (int outerIndex = 0; outerIndex < outerPointCount; ++outerIndex) {
        bridgeEdge.vertexIndex0 = outerIndex;

        {
        for (int innerIndex = 0; innerIndex < innerPointCount; ++innerIndex) {
            bridgeEdge.vertexIndex1 = outerPointCount + innerIndex;
            edgeCount =
                zGeometry_TriangulateHole::TryAppendBridgeEdge(&bridgeEdge, edgeCount, edgeStates);
        }
        }
    }
    }

    {
    for (int vertexIndex = 0; vertexIndex < combinedPointCount; ++vertexIndex) {
        int edgeIndices[0x20];
        const int activeEdgeCount =
            zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex(vertexIndex, edgeCount,
                                                                         edgeStates, edgeIndices);

        {
        for (int edgeIndex0 = 0; edgeIndex0 < activeEdgeCount; ++edgeIndex0) {
            for (int edgeIndex1 = 0; edgeIndex1 < activeEdgeCount; ++edgeIndex1) {
                if (edgeIndex0 == edgeIndex1) {
                    continue;
                }

                zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair(
                    edgeIndices[edgeIndex0], edgeIndices[edgeIndex1], vertexIndex, edgeCount,
                    edgeStates);
            }
        }
        }
    }
    }

    zGeometry_TriangleSoup *const result = (zGeometry_TriangleSoup *)(malloc(
        sizeof(int) +
        (size_t)(g_zGeometry_TriangulateHole_TriangleCount * 3) * sizeof(zVec3)));
    result->triangleCount = g_zGeometry_TriangulateHole_TriangleCount;

    zVec3 *outPoint = result->triangleVerts;
    for (int triangleIndex = 0; triangleIndex < g_zGeometry_TriangulateHole_TriangleCount;
         ++triangleIndex) {
        const zGeometry_TriangleIndexTriple *const triangle =
            &g_zGeometry_TriangulateHole_TriangleIndices[triangleIndex];
        outPoint[0] = g_zGeometry_TriangulateHole_CombinedPoints[triangle->i0];
        outPoint[1] = g_zGeometry_TriangulateHole_CombinedPoints[triangle->i1];
        outPoint[2] = g_zGeometry_TriangulateHole_CombinedPoints[triangle->i2];
        outPoint += 3;
    }

    free(edgeStates);
    free(g_zGeometry_TriangulateHole_CombinedPoints);

    return result;
}
} // namespace zGeometry

namespace zGeometry_Polygon {
// Reimplements 0x46ced0:
// zGeometry_Polygon::TrySplitPointDwordOffsetsAtBestDiagonal
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL TrySplitPointDwordOffsetsAtBestDiagonal(
    int pointCount, float *pointDwords, int *pointDwordOffsets,
    zGeometry_PolygonSplitDwordOffsetListPair *outSplitPointLists, int pointDwordStride) {
    const bool ccw =
        PolygonArea2D(pointDwords, pointDwordOffsets, pointCount, pointDwordStride) >= 0.0f;

    int earPrev = 0;
    int earCurr = 1;
    int earNext = 2;

    bool foundEar = false;
    {
    for (int curr = 0; curr < pointCount; ++curr) {
        const int prev = (curr + pointCount - 1) % pointCount;
        const int next = (curr + 1) % pointCount;
        if (IsEar(pointDwords, pointDwordOffsets, pointCount, pointDwordStride, prev, curr, next,
                  ccw)) {
            earPrev = prev;
            earCurr = curr;
            earNext = next;
            foundEar = true;
            break;
        }
    }
    }

    if (!foundEar) {
        earPrev = 0;
        earCurr = 1;
        earNext = 2;
    }

    outSplitPointLists->pointCount0 = 3;
    outSplitPointLists->pointCount1 = pointCount - 1;

    int *out = outSplitPointLists->pointDwordOffsets;
    CopyOffsetVertex(out, &pointDwordOffsets[earPrev * pointDwordStride], pointDwordStride);
    CopyOffsetVertex(out + pointDwordStride, &pointDwordOffsets[earCurr * pointDwordStride],
                     pointDwordStride);
    CopyOffsetVertex(out + pointDwordStride * 2, &pointDwordOffsets[earNext * pointDwordStride],
                     pointDwordStride);

    out += pointDwordStride * 3;
    int index = earNext;
    while (true) {
        CopyOffsetVertex(out, &pointDwordOffsets[index * pointDwordStride], pointDwordStride);
        out += pointDwordStride;

        if (index == earPrev) {
            break;
        }

        index = (index + 1) % pointCount;
    }

    return 1;
}

// Reimplements 0x46cb50:
// zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE zGeometry_TriangleDwordOffsetList *RECOIL_FASTCALL
TriangulatePointDwordOffsetsRecursive(int pointCount, float *pointDwords,
                                      int *pointDwordOffsets,
                                      int pointDwordStrideMode) {
    if (pointCount < 3) {
        fprintf(stderr, "Error in TRIANGULATE: only %d verts received\n", pointCount);
        return 0;
    }

    const int pointDwordStride = pointDwordStrideMode == 1 ? 2 : 3;
    int *workingOffsets = pointDwordOffsets;
    if (workingOffsets == 0) {
        workingOffsets = (int *)(malloc(
            (size_t)(pointCount * pointDwordStride) * sizeof(int)));
        for (int i = 0; i < pointCount * pointDwordStride; ++i) {
            workingOffsets[i] = i;
        }
    }

    const int triangleCount = pointCount - 2;
    zGeometry_TriangleDwordOffsetList *result =
        (zGeometry_TriangleDwordOffsetList *)(malloc(
            sizeof(int) +
            (size_t)(triangleCount * pointDwordStride * 3) * sizeof(int)));
    result->triangleCount = triangleCount;

    const bool ccw =
        PolygonArea2D(pointDwords, workingOffsets, pointCount, pointDwordStride) >= 0.0f;

    int *indices = (int *)(
        malloc((size_t)(pointCount) * sizeof(int)));
    for (int i = 0; i < pointCount; ++i) {
        indices[i] = i;
    }

    int remaining = pointCount;
    int outTriangle = 0;
    while (remaining > 3) {
        bool emitted = false;
        for (int i = 0; i < remaining; ++i) {
            const int prev = indices[(i + remaining - 1) % remaining];
            const int curr = indices[i];
            const int next = indices[(i + 1) % remaining];

            if (!IsEar(pointDwords, workingOffsets, pointCount, pointDwordStride, prev, curr, next,
                       ccw)) {
                continue;
            }

            AppendTriangleOffsets(result, outTriangle, workingOffsets, prev, curr, next,
                                  pointDwordStride);
            ++outTriangle;

            for (int j = i; j < remaining - 1; ++j) {
                indices[j] = indices[j + 1];
            }
            --remaining;
            emitted = true;
            break;
        }

        if (!emitted) {
            AppendTriangleOffsets(result, outTriangle, workingOffsets, indices[0], indices[1],
                                  indices[2], pointDwordStride);
            ++outTriangle;
            for (int j = 1; j < remaining - 1; ++j) {
                indices[j] = indices[j + 1];
            }
            --remaining;
        }
    }

    AppendTriangleOffsets(result, outTriangle, workingOffsets, indices[0], indices[1], indices[2],
                          pointDwordStride);

    free(indices);
    if (pointDwordOffsets == 0) {
        free(workingOffsets);
    }

    return result;
}

// Reimplements 0x46c760: zGeometry_Polygon::Convexify
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE zGeometry_ConvexPolygonSetPartial *RECOIL_FASTCALL Convexify(
    zGeometry_PolygonSpanArrayPartial *polygonSet, int inputPointCount, zVec3 *points) {
    if (inputPointCount <= 0 || points == 0) {
        fprintf(stderr, "convexify(): One or more inputs are null\n");
        return 0;
    }

    zGeometry_ConvexPolygonSetPartial *result = (zGeometry_ConvexPolygonSetPartial *)(
        malloc(sizeof(zGeometry_ConvexPolygonSetPartial)));
    result->points = (zVec3 *)(
        malloc((size_t)(inputPointCount * 3 - 6) * sizeof(zVec3)));
    result->polygons = (zGeometry_PolygonPointSpanPartial *)(malloc(
        (size_t)(inputPointCount - 2) * sizeof(zGeometry_PolygonPointSpanPartial)));
    result->totalPointCount = 0;
    result->polygonCount = 0;

    zVec3 *outputPointWriteCursor = result->points;
    zGeometry_PolygonPointSpanPartial *inputPolygon = polygonSet->polygons;
    for (int remaining = polygonSet->polygonCount; remaining != 0;
         --remaining, ++inputPolygon) {
        const int polygonPointCount = inputPolygon->pointCount;
        if (polygonPointCount < 3) {
            continue;
        }

        const float *sourcePointDwords = PointDwordBase(points, inputPolygon->pointDwordOffset);
        if (polygonPointCount == 3) {
            outputPointWriteCursor =
                CopySpanPoints(result, outputPointWriteCursor, sourcePointDwords, 3);
        } else if (polygonPointCount == 4) {
            const zVec3 *quadPoints = (const zVec3 *)(sourcePointDwords);
            if (IsConvexQuadXY(quadPoints)) {
                outputPointWriteCursor =
                    CopySpanPoints(result, outputPointWriteCursor, sourcePointDwords, 4);
            } else {
                outputPointWriteCursor =
                    AppendTriangulatedSpan(result, outputPointWriteCursor, inputPolygon, points);
            }
        } else if (polygonPointCount > 4) {
            outputPointWriteCursor =
                AppendTriangulatedSpan(result, outputPointWriteCursor, inputPolygon, points);
        } else {
            zError::ReportOld(0x100, kZGeoConvexifySourceFile, 0x38b,
                              "convexify(): Invalid input polygon size (%d) verts.",
                              inputPolygon->pointCount);
            zGeometry_ConvexPolygonSet::Destroy(result);
            return 0;
        }
    }

    return result;
}
} // namespace zGeometry_Polygon

namespace zGeometry_ConvexPolygonSet {
// Reimplements 0x46c720: zGeometry_ConvexPolygonSet::Destroy
// (D:\Proj\GameZRecoil\zGeometry\zgeo_convexify.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_ConvexPolygonSetPartial *self) {
    if (self == 0) {
        return;
    }

    if (self->polygons != 0) {
        free(self->polygons);
    }

    if (self->points != 0) {
        free(self->points);
    }

    free(self);
}
} // namespace zGeometry_ConvexPolygonSet
