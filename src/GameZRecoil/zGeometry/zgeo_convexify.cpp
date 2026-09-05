#include "zgeo.h"

#include "GameZRecoil/zError/zerr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
/*
 * Purpose: retain the current candidate allocation. Retail proves 12-byte
 * triangle-index records at 0x53a750, but the gap to 0x53d750 does not prove
 * this original array bound. The full extent remains unresolved.
 */
const int kTriangulateHoleMaxTriangles = 0x400;

/**
 * Data evidence: BN 0x4e050c..0x4e059f is the contiguous zgeo_convexify.cpp
 * diagnostic/source literal owner linked by engine.zgeometry.polygon_convexification.
 * Purpose: Preserve the writable source/error literals used by polygon convexification diagnostics.
 */
char g_zGeometry_ConvexifyNullInputsMsg[0x2a] =
    "convexify(): One or more inputs are null\n";
char g_zGeometry_SourceFile_ZgeoConvexifyCpp[0x31] =
    "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_convexify.cpp";
char g_zGeometry_ConvexifyInvalidInputPolygonSizeFmt[0x34] =
    "convexify(): Invalid input polygon size (%d) verts.";

/**
 * Data evidence: BN 0x4e05a0 is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate3ErrorMsg, referenced at 0x46ce97.
 * Purpose: Preserve the recursive triangulation first-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate3ErrorMsg[0x22] =
    "Error in recursive triangulate 3\n";

/**
 * Data evidence: BN 0x4e05c4 is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate4ErrorMsg, referenced at 0x46ce63.
 * Purpose: Preserve the recursive triangulation second-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate4ErrorMsg[0x22] =
    "Error in recursive triangulate 4\n";

/**
 * Data evidence: BN 0x4e05e8 is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate2ErrorMsg, referenced at 0x46cd76.
 * Purpose: Preserve the recursive triangulation non-triangle first-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate2ErrorMsg[0x22] =
    "Error in recursive triangulate 2\n";

/**
 * Data evidence: BN 0x4e060c is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate1ErrorMsg, referenced at 0x46ccf5.
 * Purpose: Preserve the recursive triangulation non-triangle second-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate1ErrorMsg[0x22] =
    "Error in recursive triangulate 1\n";

/**
 * Data evidence: BN 0x4e0630 is writable char[0x2e]
 * g_zGeometry_TriangulateOnlyVertsReceivedFmt, referenced at 0x46cb6b.
 * Purpose: Preserve the recursive triangulation input-count guard diagnostic.
 */
char g_zGeometry_TriangulateOnlyVertsReceivedFmt[0x2e] =
    "Error in TRIANGULATE: only %d verts received\n";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-g-zgeometry-triangulatehole-combinedpoints
 * @recoil-artifact defines .data recoil:data:0x53a748: g_zGeometry_TriangulateHole_CombinedPoints.
 * Purpose: Hold the combined outer/inner point buffer while triangulating a hole.
 */
zVec3 *g_zGeometry_TriangulateHole_CombinedPoints = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-g-zgeometry-triangulatehole-trianglecount
 * @recoil-artifact defines .data recoil:data:0x53d750: g_zGeometry_TriangulateHole_TriangleCount.
 * Purpose: Count emitted triangulate-hole triangles in the current pass.
 */
int g_zGeometry_TriangulateHole_TriangleCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-g-zgeometry-triangulatehole-combinedpointcount
 * @recoil-artifact defines .data recoil:data:0x53d754: g_zGeometry_TriangulateHole_CombinedPointCount.
 * Purpose: Track the combined outer/inner point count for active edge traversal.
 */
int g_zGeometry_TriangulateHole_CombinedPointCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-ktriangulateholemaxtriangles
 * @recoil-artifact defines .data recoil:data:0x53a750: g_zGeometry_TriangulateHole_TriangleIndices.
 * Purpose: Store emitted triangulate-hole vertex index triples before output materialization.
 */
zGeometry_TriangleIndexTriple
    g_zGeometry_TriangulateHole_TriangleIndices[kTriangulateHoleMaxTriangles];
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-g-zgeometry-triangulatehole-cachedplane
 * @recoil-artifact defines .data recoil:data:0x53d758: g_zGeometry_TriangulateHole_CachedPlane.
 * Purpose: Cache the combined ring plane while projecting inner-ring points.
 */
zGeometry_PlaneEquationPartial g_zGeometry_TriangulateHole_CachedPlane;

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in callers 0x46ced0 and 0x46d140.
 * Purpose: Read the X component from a point-dword offset list.
 */
float OffsetX(
    const float *pointDwords,
    const int *pointDwordOffsets,
    int index,
    int stride
) {
    return pointDwords[pointDwordOffsets[index * stride]];
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in callers 0x46ced0 and 0x46d140.
 * Purpose: Read the Y component from a point-dword offset list.
 */
float OffsetY(
    const float *pointDwords,
    const int *pointDwordOffsets,
    int index,
    int stride
) {
    return pointDwords[pointDwordOffsets[index * stride + 1]];
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in callers 0x46ced0, 0x46d140, and zGeometry XY helpers.
 * Purpose: Compute the signed two-dimensional cross product.
 */
float Cross2D(
    float ax,
    float ay,
    float bx,
    float by,
    float cx,
    float cy
) {
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Accumulate signed polygon area from offset point dwords.
 */
inline float PolygonArea2D(
    const float *pointDwords,
    const int *pointDwordOffsets,
    int pointCount,
    int stride
) {
    float area = 0.0f;
    for (int i = 0; i < pointCount; ++i) {
        const int next = (i + 1) % pointCount;
        area += OffsetX(
            pointDwords,
            pointDwordOffsets,
            i,
            stride
        ) *
                    OffsetY(
                        pointDwords,
                        pointDwordOffsets,
                        next,
                        stride
                    ) -
                OffsetY(
                    pointDwords,
                    pointDwordOffsets,
                    i,
                    stride
                ) *
                    OffsetX(
                        pointDwords,
                        pointDwordOffsets,
                        next,
                        stride
                    );
    }

    return area;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Test whether an XY point lies inside a candidate triangle.
 */
bool PointInTriangle2D(
    float px,
    float py,
    float ax,
    float ay,
    float bx,
    float by,
    float cx,
    float cy,
    bool ccw
) {
    const float cross0 = Cross2D(
        ax,
        ay,
        bx,
        by,
        px,
        py
    );
    const float cross1 = Cross2D(
        bx,
        by,
        cx,
        cy,
        px,
        py
    );
    const float cross2 = Cross2D(
        cx,
        cy,
        ax,
        ay,
        px,
        py
    );

    if (ccw) {
        return cross0 >= 0.0f && cross1 >= 0.0f && cross2 >= 0.0f;
    }

    return cross0 <= 0.0f && cross1 <= 0.0f && cross2 <= 0.0f;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Copy one point's dword-offset tuple into triangle output storage.
 */
void CopyOffsetVertex(
    int *dest,
    const int *source,
    int stride
) {
    memcpy(
        dest,
        source,
        (size_t)(stride) * sizeof(int)
    );
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in address-backed callers in this source file.
 * Purpose: Convert a point dword offset into the source float tuple base.
 */
const float *PointDwordBase(
    const zVec3 *points,
    int pointDwordOffset
) {
    return (const float *)(points) + pointDwordOffset;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in address-backed callers in this source file.
 * Purpose: Append a contiguous source point span to the convexification result.
 */
zVec3 *CopySpanPoints(
    zGeometry_ConvexPolygonSetPartial *result,
    zVec3 *outputPointWriteCursor,
    const float *sourcePointDwords,
    int pointCount
) {
    zGeometry_PolygonPointSpanPartial *polygon = &result->polygons[result->polygonCount];
    polygon->pointCount = pointCount;
    polygon->pointDwordOffset = result->totalPointCount * 3;

    memcpy(
        outputPointWriteCursor,
        sourcePointDwords,
        (size_t)(pointCount) * sizeof(zVec3)
    );

    ++result->polygonCount;
    result->totalPointCount += pointCount;
    return outputPointWriteCursor + pointCount;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46d140.
 * Purpose: Classify a four-point XY span as convex before preserving it.
 */
bool IsConvexQuadXY(
    const zVec3 *points
) {
    int sign = 0;
    for (int i = 0; i < 4; ++i) {
        const zVec3 &a = points[i];
        const zVec3 &b = points[(i + 1) & 3];
        const zVec3 &c = points[(i + 2) & 3];
        const float cross = Cross2D(
            a.x,
            a.y,
            b.x,
            b.y,
            c.x,
            c.y
        );
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

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in address-backed callers in this source file.
 * Purpose: Triangulate and append a polygon span into triangle-sized output spans.
 */
zVec3 *AppendTriangulatedSpan(
    zGeometry_ConvexPolygonSetPartial *result,
    zVec3 *outputPointWriteCursor,
    const zGeometry_PolygonPointSpanPartial *inputPolygon,
    const zVec3 *allPoints
) {
    const float *sourcePointDwords = PointDwordBase(
        allPoints,
        inputPolygon->pointDwordOffset
    );
    zGeometry_TriangleDwordOffsetList *triangles =
        zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive(
            inputPolygon->pointCount,
            (float *)(sourcePointDwords),
            0,
            0
        );

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

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in address-backed callers in this source file.
 * Purpose: Access the packed triangle dword-offset payload.
 */
int *TrianglePayload(
    zGeometry_TriangleDwordOffsetList *list
) {
    return list->triangleDwordOffsets;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Append one triangle's source point offsets into the output list.
 */
void AppendTriangleOffsets(
    zGeometry_TriangleDwordOffsetList *list,
    int triangleIndex,
    const int *polygonOffsets,
    int index0,
    int index1,
    int index2,
    int stride
) {
    int *out = &TrianglePayload(list)[triangleIndex * stride * 3];
    CopyOffsetVertex(
        out,
        &polygonOffsets[index0 * stride],
        stride
    );
    CopyOffsetVertex(
        out + stride,
        &polygonOffsets[index1 * stride],
        stride
    );
    CopyOffsetVertex(
        out + stride * 2,
        &polygonOffsets[index2 * stride],
        stride
    );
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Reject non-ears and ears containing another polygon point.
 */
bool IsEar(
    const float *pointDwords,
    const int *pointDwordOffsets,
    int pointCount,
    int stride,
    int prev,
    int curr,
    int next,
    bool ccw
) {
    const float ax = OffsetX(
        pointDwords,
        pointDwordOffsets,
        prev,
        stride
    );
    const float ay = OffsetY(
        pointDwords,
        pointDwordOffsets,
        prev,
        stride
    );
    const float bx = OffsetX(
        pointDwords,
        pointDwordOffsets,
        curr,
        stride
    );
    const float by = OffsetY(
        pointDwords,
        pointDwordOffsets,
        curr,
        stride
    );
    const float cx = OffsetX(
        pointDwords,
        pointDwordOffsets,
        next,
        stride
    );
    const float cy = OffsetY(
        pointDwords,
        pointDwordOffsets,
        next,
        stride
    );
    const float cross = Cross2D(
        ax,
        ay,
        bx,
        by,
        cx,
        cy
    );

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

        if (PointInTriangle2D(
                OffsetX(
                    pointDwords,
                    pointDwordOffsets,
                    i,
                    stride
                ),
                OffsetY(
                    pointDwords,
                    pointDwordOffsets,
                    i,
                    stride
                ),
                ax,
                ay,
                bx,
                by,
                cx,
                cy,
                ccw
            )) {
            return false;
        }
    }

    return true;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46c3a0.
 * Purpose: Produce the VC-era fast square-root estimate used for plane scale.
 */
float EstimateMagnitudeFromSquaredLength(
    float squaredLength
) {
    union {
        float value;
        int bits;
    } estimate;

    estimate.value = squaredLength;
    estimate.bits = (estimate.bits >> 1) + 0x1fc00000;
    return estimate.value;
}
} // namespace

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-tryappendbridgeedge
 * @recoil-artifact defines .text recoil:function:0x46bd50: zGeometry_TriangulateHole::TryAppendBridgeEdge
 * Purpose: Append a bridge edge when it is unique and does not cross live edges.
 */
int __fastcall TryAppendBridgeEdge(
    zGeometry_TriangulateHole_EdgeState *edgeState,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates
) {
    if (FindActiveEdgeState(
            edgeState->vertexIndex0,
            edgeState->vertexIndex1,
            edgeCount,
            edgeStates
        ) != 0) {
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
                bridgeStart,
                bridgeEnd,
                &g_zGeometry_TriangulateHole_CombinedPoints[edge->vertexIndex0],
                &g_zGeometry_TriangulateHole_CombinedPoints[edge->vertexIndex1]
            ) != 0) {
            return edgeCount;
        }
    }

    edgeStates[edgeCount] = *edgeState;
    return edgeCount + 1;
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry_Segment {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-intersectssegmentxy
 * @recoil-artifact defines .text recoil:function:0x46be20: zGeometry_Segment::IntersectsSegmentXY
 * Purpose: Test whether two XY segments intersect with both parametric coordinates in the half-open unit range.
 */
int __fastcall IntersectsSegmentXY(
    zVec3 *segmentAPoint0,
    zVec3 *segmentAPoint1,
    zVec3 *segmentBPoint0,
    zVec3 *segmentBPoint1
) {
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

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-collectactiveedgeindicesforvertex
 * @recoil-artifact defines .text recoil:function:0x46bf30: zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex
 * Purpose: Collect live edge-state indices incident to one combined-ring vertex.
 */
int __fastcall CollectActiveEdgeIndicesForVertex(
    int vertexIndex,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates,
    int *outEdgeIndices
) {
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
} // namespace zGeometry_TriangulateHole

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-findactiveedgestate
 * @recoil-artifact defines .text recoil:function:0x46bf70: zGeometry_TriangulateHole::FindActiveEdgeState
 * Purpose: Find a live edge between two combined-ring vertex indices.
 */
zGeometry_TriangulateHole_EdgeState *__fastcall FindActiveEdgeState(
    int vertexIndex0,
    int vertexIndex1,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates
) {
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
} // namespace zGeometry_TriangulateHole

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-tryemittrianglefromedgepair
 * @recoil-artifact defines .text recoil:function:0x46bfc0: zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair
 * Purpose: Emit a triangle from two incident live edges and their closing edge.
 */
void __fastcall TryEmitTriangleFromEdgePair(
    int edgeIndex0,
    int edgeIndex1,
    int vertexIndex,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState *edgeStates
) {
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
        FindActiveEdgeState(
            vertexIndex0,
            vertexIndex1,
            edgeCount,
            edgeStates
        );
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
} // namespace zGeometry_TriangulateHole

namespace zGeometry {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-triangulatepolygonwithhole
 * @recoil-artifact defines .text recoil:function:0x46c070: zGeometry::TriangulatePolygonWithHole
 * Purpose: Bridge an inner polygon ring to an outer ring and emit triangle soup.
 */
zGeometry_TriangleSoup *__fastcall TriangulatePolygonWithHole(
    int outerPointCount,
    zVec3 *outerPoints,
    int innerPointCount,
    zVec3 *innerPoints
) {
    const int combinedPointCount = outerPointCount + innerPointCount;

    g_zGeometry_TriangulateHole_TriangleCount = 0;
    g_zGeometry_TriangulateHole_CombinedPointCount = combinedPointCount;

    zGeometry_TriangulateHole_EdgeState *const edgeStates =
        (zGeometry_TriangulateHole_EdgeState *)(malloc(
            (size_t)(combinedPointCount * combinedPointCount) *
            sizeof(zGeometry_TriangulateHole_EdgeState)
        ));

    g_zGeometry_TriangulateHole_CombinedPoints =
        (zVec3 *)(malloc((size_t)(combinedPointCount) * sizeof(zVec3)));

    memcpy(
        g_zGeometry_TriangulateHole_CombinedPoints,
        outerPoints,
        (size_t)(outerPointCount) * sizeof(zVec3)
    );
    memcpy(
        &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount],
        innerPoints,
        (size_t)(innerPointCount) * sizeof(zVec3)
    );

    zGeometry_TriangulateHole::CacheCombinedPlane(
        outerPointCount,
        outerPoints
    );
    zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane(
        innerPointCount,
        &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount]
    );
    memcpy(
        innerPoints,
        &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount],
        (size_t)(innerPointCount) * sizeof(zVec3)
    );

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
                    edgeCount = zGeometry_TriangulateHole::TryAppendBridgeEdge(
                        &bridgeEdge,
                        edgeCount,
                        edgeStates
                    );
                }
            }
        }
    }

    {
        for (int vertexIndex = 0; vertexIndex < combinedPointCount; ++vertexIndex) {
            int edgeIndices[0x20];
            const int activeEdgeCount =
                zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex(
                    vertexIndex,
                    edgeCount,
                    edgeStates,
                    edgeIndices
                );

            {
                for (int edgeIndex0 = 0; edgeIndex0 < activeEdgeCount; ++edgeIndex0) {
                    for (int edgeIndex1 = 0; edgeIndex1 < activeEdgeCount; ++edgeIndex1) {
                        if (edgeIndex0 == edgeIndex1) {
                            continue;
                        }

                        zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair(
                            edgeIndices[edgeIndex0],
                            edgeIndices[edgeIndex1],
                            vertexIndex,
                            edgeCount,
                            edgeStates
                        );
                    }
                }
            }
        }
    }

    zGeometry_TriangleSoup *const result = (zGeometry_TriangleSoup *)(malloc(
        sizeof(int) + (size_t)(g_zGeometry_TriangulateHole_TriangleCount * 3) * sizeof(zVec3)
    ));
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

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-cachecombinedplane
 * @recoil-artifact defines .text recoil:function:0x46c390: zGeometry_TriangulateHole::CacheCombinedPlane
 * Purpose: Cache the plane equation used to project the inner ring.
 */
void __fastcall CacheCombinedPlane(
    int pointCount,
    zVec3 *points
) {
    zGeometry_Vec3Array::ComputeNewellPlane(
        pointCount,
        points,
        &g_zGeometry_TriangulateHole_CachedPlane
    );
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-computenewellplane
 * @recoil-artifact defines .text recoil:function:0x46c3a0: zGeometry_Vec3Array::ComputeNewellPlane
 * Purpose: Compute a normalized Newell plane equation from a point ring.
 */
void __fastcall ComputeNewellPlane(
    int pointCount,
    zVec3 *points,
    zGeometry_PlaneEquationPartial *outPlane
) {
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
        union {
            float value;
            int bits;
        } estimate;

        estimate.value = normalX * normalX + normalY * normalY + normalZ * normalZ;
        estimate.bits = (estimate.bits >> 1) + 0x1fc00000;
        estimatedMagnitude = estimate.value;
    }

    float normalScale = 0.0f;
    if (estimatedMagnitude != 0.0f) {
        normalScale = 1.0f / estimatedMagnitude;
    }

    outPlane->a = normalX * normalScale;
    outPlane->b = normalY * normalScale;
    outPlane->c = normalZ * normalScale;
    outPlane->d =
        -((sumX * normalX + sumY * normalY + sumZ * normalZ) /
            ((float)(pointCount)*estimatedMagnitude));
}
} // namespace zGeometry_Vec3Array

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-projectinnerringontocachedplane
 * @recoil-artifact defines .text recoil:function:0x46c570: zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane
 * Purpose: Project inner-ring Z values onto the cached outer-ring plane.
 */
void __fastcall ProjectInnerRingOntoCachedPlane(
    int pointCount,
    zVec3 *points
) {
    for (int i = 0; i < pointCount; ++i) {
        zVec3 *const point = &points[i];
        point->z = -(g_zGeometry_TriangulateHole_CachedPlane.a * point->x +
                       g_zGeometry_TriangulateHole_CachedPlane.b * point->y +
                       g_zGeometry_TriangulateHole_CachedPlane.d) /
                   g_zGeometry_TriangulateHole_CachedPlane.c;
    }
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-reversepoints
 * @recoil-artifact defines .text recoil:function:0x46c5b0: zGeometry_Vec3Array::ReversePoints
 * Purpose: Reverse all points after the anchor point in a polygon ring.
 */
void __fastcall ReversePoints(
    int pointCount,
    zVec3 *points
) {
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
} // namespace zGeometry_Vec3Array

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-ensurepositivecrossz
 * @recoil-artifact defines .text recoil:function:0x46c620: zGeometry_Vec3Array::EnsurePositiveCrossZ
 * Purpose: Ensure the first two polygon edges produce a positive Z cross.
 */
int __fastcall EnsurePositiveCrossZ(
    int pointCount,
    zVec3 *points,
    int allowReverse
) {
    const zVec3 edge0 = {points[1].x - points[0].x,
        points[1].y - points[0].y,
        points[1].z - points[0].z};
    const zVec3 edge1 = {points[2].x - points[1].x,
        points[2].y - points[1].y,
        points[2].z - points[1].z};
    const zVec3 cross = {edge0.y * edge1.z - edge0.z * edge1.y,
        edge0.z * edge1.x - edge0.x * edge1.z,
        edge0.x * edge1.y - edge0.y * edge1.x};

    if (!(cross.z > 0.0f)) {
        if (allowReverse == 0) {
            return 0;
        }

        ReversePoints(
            pointCount,
            points
        );
    }

    return 1;
}
} // namespace zGeometry_Vec3Array

namespace zGeometry_ConvexPolygonSet {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-destroy
 * @recoil-artifact defines .text recoil:function:0x46c720: zGeometry_ConvexPolygonSet::Destroy
 * Purpose: Release a convex polygon set and its owned point and polygon arrays.
 */
void __fastcall Destroy(
    zGeometry_ConvexPolygonSetPartial *self
) {
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

namespace zGeometry_Polygon {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-convexify
 * @recoil-artifact defines .text recoil:function:0x46c760: zGeometry_Polygon::Convexify
 * Purpose: Convert polygon spans into convex polygon output, copying already
 * convex spans and triangulating non-convex spans through the polygon splitter.
 */
zGeometry_ConvexPolygonSetPartial *__fastcall Convexify(
    zGeometry_PolygonSpanArrayPartial *polygonSet,
    int inputPointCount,
    zVec3 *points
) {
    if (inputPointCount <= 0 || points == 0) {
        fprintf(
            stderr,
            g_zGeometry_ConvexifyNullInputsMsg
        );
        return 0;
    }

    zGeometry_ConvexPolygonSetPartial *result =
        (zGeometry_ConvexPolygonSetPartial *)(malloc(sizeof(zGeometry_ConvexPolygonSetPartial)));
    result->points = (zVec3 *)(malloc((size_t)(inputPointCount * 3 - 6) * sizeof(zVec3)));
    result->polygons = (zGeometry_PolygonPointSpanPartial *)(malloc(
        (size_t)(inputPointCount - 2) * sizeof(zGeometry_PolygonPointSpanPartial)
    ));
    result->totalPointCount = 0;
    result->polygonCount = 0;

    zVec3 *outputPointWriteCursor = result->points;
    zGeometry_PolygonPointSpanPartial *inputPolygon = polygonSet->polygons;
    for (int remaining = polygonSet->polygonCount; remaining != 0; --remaining, ++inputPolygon) {
        const int polygonPointCount = inputPolygon->pointCount;
        if (polygonPointCount < 3) {
            continue;
        }

        const float *sourcePointDwords =
            (const float *)(points) + inputPolygon->pointDwordOffset;
        bool copySpan = polygonPointCount == 3;
        if (polygonPointCount == 4) {
            const zVec3 *quadPoints = (const zVec3 *)(sourcePointDwords);
            int sign = 0;
            copySpan = true;
            for (int i = 0; i < 4; ++i) {
                const zVec3 &a = quadPoints[i];
                const zVec3 &b = quadPoints[(i + 1) & 3];
                const zVec3 &c = quadPoints[(i + 2) & 3];
                const float cross =
                    (b.x - a.x) * (c.y - a.y) -
                    (b.y - a.y) * (c.x - a.x);
                if (cross == 0.0f) {
                    continue;
                }

                const int thisSign = cross > 0.0f ? 1 : -1;
                if (sign != 0 && sign != thisSign) {
                    copySpan = false;
                    break;
                }

                sign = thisSign;
            }
        }

        if (copySpan) {
            zGeometry_PolygonPointSpanPartial *polygon =
                &result->polygons[result->polygonCount];
            polygon->pointCount = polygonPointCount;
            polygon->pointDwordOffset = result->totalPointCount * 3;
            memcpy(
                outputPointWriteCursor,
                sourcePointDwords,
                (size_t)(polygonPointCount) * sizeof(zVec3)
            );
            ++result->polygonCount;
            result->totalPointCount += polygonPointCount;
            outputPointWriteCursor += polygonPointCount;
        } else if (polygonPointCount >= 4) {
            zGeometry_TriangleDwordOffsetList *triangles =
                zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive(
                    polygonPointCount,
                    (float *)(sourcePointDwords),
                    0,
                    0
                );
            if (triangles != 0) {
                const int *triangleOffsets = triangles->triangleDwordOffsets;
                for (int triangle = 0; triangle < triangles->triangleCount; ++triangle) {
                    zGeometry_PolygonPointSpanPartial *polygon =
                        &result->polygons[result->polygonCount];
                    polygon->pointCount = 3;
                    polygon->pointDwordOffset = result->totalPointCount * 3;
                    ++result->polygonCount;
                    result->totalPointCount += 3;

                    float *outputDwords = (float *)(outputPointWriteCursor);
                    for (int dwordIndex = 0; dwordIndex < 9; ++dwordIndex) {
                        outputDwords[dwordIndex] =
                            sourcePointDwords[triangleOffsets[triangle * 9 + dwordIndex]];
                    }
                    outputPointWriteCursor += 3;
                }
                free(triangles);
            }
        } else {
            zError::ReportOld(
                0x100,
                g_zGeometry_SourceFile_ZgeoConvexifyCpp,
                0x38b,
                g_zGeometry_ConvexifyInvalidInputPolygonSizeFmt,
                inputPolygon->pointCount
            );
            zGeometry_ConvexPolygonSet::Destroy(result);
            return 0;
        }
    }

    return result;
}
} // namespace zGeometry_Polygon

namespace zGeometry_Polygon {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-triangulatepointdwordoffsetsrecursive
 * @recoil-artifact defines .text recoil:function:0x46cb50: zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive
 * Purpose: Recursively split a polygon point-dword offset list and append the
 * resulting triangle offset lists.
 */
zGeometry_TriangleDwordOffsetList *__fastcall TriangulatePointDwordOffsetsRecursive(
    int pointCount,
    float *pointDwords,
    int *pointDwordOffsets,
    int pointDwordStrideMode
) {
    if (pointCount < 3) {
        fprintf(
            stderr,
            g_zGeometry_TriangulateOnlyVertsReceivedFmt,
            pointCount
        );
        return 0;
    }

    const int pointDwordStride = pointDwordStrideMode == 1 ? 2 : 3;
    int *workingOffsets = pointDwordOffsets;
    if (workingOffsets == 0) {
        workingOffsets = (int *)(malloc((size_t)(pointCount * pointDwordStride) * sizeof(int)));
        for (int i = 0; i < pointCount * pointDwordStride; ++i) {
            workingOffsets[i] = i;
        }
    }

    const int triangleCount = pointCount - 2;
    zGeometry_TriangleDwordOffsetList *result = (zGeometry_TriangleDwordOffsetList *)(malloc(
        sizeof(int) + (size_t)(triangleCount * pointDwordStride * 3) * sizeof(int)
    ));
    result->triangleCount = triangleCount;

    zGeometry_PolygonSplitDwordOffsetListPair *splitPointLists = 0;
    if (pointCount != 3) {
        const int splitPointCount = pointCount + 2;
        splitPointLists =
            (zGeometry_PolygonSplitDwordOffsetListPair *)(malloc(
                sizeof(zGeometry_PolygonSplitDwordOffsetListPair) +
                (size_t)(splitPointCount * pointDwordStride - 1) * sizeof(int)
            ));
    }

    int splitSucceeded = 1;
    if (pointCount != 3) {
        splitSucceeded = TrySplitPointDwordOffsetsAtBestDiagonal(
            pointCount,
            pointDwords,
            workingOffsets,
            splitPointLists,
            pointDwordStride
        );
    }

    if (pointCount != 3) {
    if (splitSucceeded == 0) {
        free(splitPointLists);
        free(result);

        return 0;
    }

    int *outTriangleOffsets = result->triangleDwordOffsets;
    zGeometry_TriangleDwordOffsetList *triangles = 0;
    char *oneSideErrorMessage = 0;
    int oneSideComplete = 0;
    int triangles0DwordCount = 0;
    if (splitPointLists->pointCount0 == 3) {
        triangles = TriangulatePointDwordOffsetsRecursive(
            splitPointLists->pointCount1,
            pointDwords,
            splitPointLists->pointDwordOffsets + 3 * pointDwordStride,
            pointDwordStrideMode
        );
        if (triangles == 0) {
            oneSideErrorMessage = g_zGeometry_RecursiveTriangulate1ErrorMsg;
        } else {
            memcpy(
                outTriangleOffsets,
                splitPointLists->pointDwordOffsets,
                (size_t)(3 * pointDwordStride) * sizeof(int)
            );
            memcpy(
                outTriangleOffsets + 3 * pointDwordStride,
                triangles->triangleDwordOffsets,
                (size_t)(triangles->triangleCount * 3 * pointDwordStride) * sizeof(int)
            );
            oneSideComplete = 1;
        }
    } else if (splitPointLists->pointCount1 == 3) {
        triangles = TriangulatePointDwordOffsetsRecursive(
            splitPointLists->pointCount0,
            pointDwords,
            splitPointLists->pointDwordOffsets,
            pointDwordStrideMode
        );
        if (triangles == 0) {
            oneSideErrorMessage = g_zGeometry_RecursiveTriangulate2ErrorMsg;
        } else {
            triangles0DwordCount = triangles->triangleCount * 3 * pointDwordStride;
            memcpy(
                outTriangleOffsets,
                triangles->triangleDwordOffsets,
                (size_t)(triangles0DwordCount) * sizeof(int)
            );
            memcpy(
                outTriangleOffsets + triangles0DwordCount,
                splitPointLists->pointDwordOffsets +
                    splitPointLists->pointCount0 * pointDwordStride,
                (size_t)(3 * pointDwordStride) * sizeof(int)
            );
            oneSideComplete = 1;
        }
    }

    if (oneSideErrorMessage != 0) {
        fprintf(stderr, oneSideErrorMessage);
        free(result);
        free(splitPointLists);
        return 0;
    }
    if (oneSideComplete == 0) {
        triangles = TriangulatePointDwordOffsetsRecursive(
            splitPointLists->pointCount0,
            pointDwords,
            splitPointLists->pointDwordOffsets,
            pointDwordStrideMode
        );
    }

    if (triangles != 0) {
        if (oneSideComplete == 0) {
            triangles0DwordCount = triangles->triangleCount * 3 * pointDwordStride;
            memcpy(
                outTriangleOffsets,
                triangles->triangleDwordOffsets,
                (size_t)(triangles0DwordCount) * sizeof(int)
            );
        }

        free(triangles);
        if (oneSideComplete == 0) {
            triangles = TriangulatePointDwordOffsetsRecursive(
                splitPointLists->pointCount1,
                pointDwords,
                splitPointLists->pointDwordOffsets +
                    splitPointLists->pointCount0 * pointDwordStride,
                pointDwordStrideMode
            );
            if (triangles != 0) {
                memcpy(
                    outTriangleOffsets + triangles0DwordCount,
                    triangles->triangleDwordOffsets,
                    (size_t)(triangles->triangleCount * 3 * pointDwordStride) * sizeof(int)
                );
                free(triangles);
                free(splitPointLists);
                return result;
            }

            fprintf(stderr, g_zGeometry_RecursiveTriangulate3ErrorMsg);
            free(splitPointLists);
            free(result);
            return triangles;
        }

        return result;
    }

    fprintf(stderr, g_zGeometry_RecursiveTriangulate4ErrorMsg);
    free(splitPointLists);
    free(result);
    return 0;
    }

    int *outTriangleOffsets = result->triangleDwordOffsets;
    memcpy(
        outTriangleOffsets,
        workingOffsets,
        (size_t)(pointDwordStride) * sizeof(int)
    );
    memcpy(
        outTriangleOffsets + pointDwordStride,
        workingOffsets + pointDwordStride,
        (size_t)(pointDwordStride) * sizeof(int)
    );
    memcpy(
        outTriangleOffsets + pointDwordStride * 2,
        workingOffsets + pointDwordStride * 2,
        (size_t)(pointDwordStride) * sizeof(int)
    );
    return result;
}
} // namespace zGeometry_Polygon

namespace zGeometry_Polygon {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-trysplitpointdwordoffsetsatbestdiagonal
 * @recoil-artifact defines .text recoil:function:0x46ced0: zGeometry_Polygon::TrySplitPointDwordOffsetsAtBestDiagonal
 * Purpose: Split a polygon point-dword offset list across the chosen diagonal
 * into two smaller polygon offset lists.
 */
int __fastcall TrySplitPointDwordOffsetsAtBestDiagonal(
    int pointCount,
    float *pointDwords,
    int *pointDwordOffsets,
    zGeometry_PolygonSplitDwordOffsetListPair *outSplitPointLists,
    int pointDwordStride
) {
    float polygonArea = 0.0f;
    for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
        const int nextPointIndex = (pointIndex + 1) % pointCount;
        polygonArea +=
            pointDwords[pointDwordOffsets[pointIndex * pointDwordStride]] *
                pointDwords[pointDwordOffsets[nextPointIndex * pointDwordStride + 1]] -
            pointDwords[pointDwordOffsets[pointIndex * pointDwordStride + 1]] *
                pointDwords[pointDwordOffsets[nextPointIndex * pointDwordStride]];
    }
    const bool ccw = polygonArea >= 0.0f;

    int earPrev = 0;
    int earCurr = 1;
    int earNext = 2;

    bool foundEar = false;
    {
        for (int curr = 0; curr < pointCount; ++curr) {
            const int prev = (curr + pointCount - 1) % pointCount;
            const int next = (curr + 1) % pointCount;
            const float ax = pointDwords[pointDwordOffsets[prev * pointDwordStride]];
            const float ay = pointDwords[pointDwordOffsets[prev * pointDwordStride + 1]];
            const float bx = pointDwords[pointDwordOffsets[curr * pointDwordStride]];
            const float by = pointDwords[pointDwordOffsets[curr * pointDwordStride + 1]];
            const float cx = pointDwords[pointDwordOffsets[next * pointDwordStride]];
            const float cy = pointDwords[pointDwordOffsets[next * pointDwordStride + 1]];
            const float cross =
                (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
            bool isEar = ccw ? cross > 0.0f : cross < 0.0f;

            for (int testIndex = 0; isEar && testIndex < pointCount; ++testIndex) {
                if (testIndex == prev || testIndex == curr || testIndex == next) {
                    continue;
                }

                const float px =
                    pointDwords[pointDwordOffsets[testIndex * pointDwordStride]];
                const float py =
                    pointDwords[pointDwordOffsets[testIndex * pointDwordStride + 1]];
                const float cross0 =
                    (bx - ax) * (py - ay) - (by - ay) * (px - ax);
                const float cross1 =
                    (cx - bx) * (py - by) - (cy - by) * (px - bx);
                const float cross2 =
                    (ax - cx) * (py - cy) - (ay - cy) * (px - cx);
                const bool pointInTriangle = ccw
                    ? cross0 >= 0.0f && cross1 >= 0.0f && cross2 >= 0.0f
                    : cross0 <= 0.0f && cross1 <= 0.0f && cross2 <= 0.0f;
                if (pointInTriangle) {
                    isEar = false;
                }
            }

            if (isEar) {
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
    memcpy(
        out,
        &pointDwordOffsets[earPrev * pointDwordStride],
        (size_t)(pointDwordStride) * sizeof(int)
    );
    memcpy(
        out + pointDwordStride,
        &pointDwordOffsets[earCurr * pointDwordStride],
        (size_t)(pointDwordStride) * sizeof(int)
    );
    memcpy(
        out + pointDwordStride * 2,
        &pointDwordOffsets[earNext * pointDwordStride],
        (size_t)(pointDwordStride) * sizeof(int)
    );

    out += pointDwordStride * 3;
    int index = earNext;
    while (true) {
        memcpy(
            out,
            &pointDwordOffsets[index * pointDwordStride],
            (size_t)(pointDwordStride) * sizeof(int)
        );
        out += pointDwordStride;

        if (index == earPrev) {
            break;
        }

        index = (index + 1) % pointCount;
    }

    return 1;
}
} // namespace zGeometry_Polygon
