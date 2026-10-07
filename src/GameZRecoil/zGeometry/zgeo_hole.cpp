// zGeometry compilation unit for hole triangulation with its Newell-plane and
// point-ring helpers, inferred from the retail object boundary [0x46bd50,
// 0x46c720): its pooled .rdata [0x4d2670, 0x4d2680) holds 0.0f, 1.0f and
// double 0.0, while zgeo_convexify.cpp pools 0.0f and 1.0f again at [0x4d2680,
// 0x4d2690); the 1998 demos keep the same two pools. Its .bss triangulation
// state occupies [0x53a748, 0x53d768) and it owns no .data. Ending the object
// before zGeometry_ConvexPolygonSet::Destroy is a placement heuristic. Original
// filename unresolved; zgeo_hole.cpp is a provisional name (2026-10-03).

#include "zgeo.h"

#include "GameZRecoil/zMath/zmth.h"

#include <stdlib.h>
#include <string.h>

/**
 * VC5 C1 draws declarations, labels and temporaries from one translation-unit
 * ID counter, and ProjectInnerRingOntoCachedPlane (0x46c570) orders its x87 loads
 * by that counter's parity at parse time. These declarations are never referenced
 * and emit no code, data or symbols. User-authorized exception: match-proofs.md
 * "Per-TU VC5 ID-counter parity exception".
 * Purpose: keep this file's ID-counter parity after including zmth.h.
 */
extern int g_ZgeoHoleIdCounterAlignment0;

namespace {
/*
 * Purpose: retain the current candidate allocation. Retail proves 12-byte
 * triangle-index records at 0x53a750, but the gap to 0x53d750 does not prove
 * this original array bound. The full extent remains unresolved.
 */
const int kTriangulateHoleMaxTriangles = 0x400;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-g-zgeometry-triangulatehole-combinedpoints
 * @recoil-artifact defines .data recoil:data:0x53a748: g_zGeometry_TriangulateHole_CombinedPoints.
 * Purpose: Hold the combined outer/inner point buffer while triangulating a hole.
 */
zVec3* g_zGeometry_TriangulateHole_CombinedPoints = 0;
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
zGeometry_TriangleIndexTriple g_zGeometry_TriangulateHole_TriangleIndices[kTriangulateHoleMaxTriangles];
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-g-zgeometry-triangulatehole-cachedplane
 * @recoil-artifact defines .data recoil:data:0x53d758: g_zGeometry_TriangulateHole_CachedPlane.
 * Purpose: Cache the combined ring plane while projecting inner-ring points.
 */
zGeometry_PlaneEquationPartial g_zGeometry_TriangulateHole_CachedPlane;

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46c3a0.
 * Purpose: Produce the VC-era fast square-root estimate used for plane scale.
 * Placement: defined after the triangulation state; this counter-neutral
 * arrangement keeps VC5's x87 operand order in ProjectInnerRingOntoCachedPlane
 * (0x46c570) after the zModel module-state declarations left the shared
 * zClass/zVideo headers.
 */
float EstimateMagnitudeFromSquaredLength(float squaredLength)
{
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
 * @recoil-match byte
 *
 * Purpose: Append a bridge edge when it is unique and does not cross live edges.
 */
int __fastcall TryAppendBridgeEdge(
    zGeometry_TriangulateHole_EdgeState* edgeState,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState* edgeStates
)
{
    zVec3* const bridgeStart = &g_zGeometry_TriangulateHole_CombinedPoints[edgeState->vertexIndex0];
    zVec3* const bridgeEnd = &g_zGeometry_TriangulateHole_CombinedPoints[edgeState->vertexIndex1];

    if (FindActiveEdgeState(edgeState->vertexIndex0, edgeState->vertexIndex1, edgeCount, edgeStates) != 0) {
        return edgeCount;
    }

    // Retail advances the edge cursor on both the skip path and the tested path.
    zGeometry_TriangulateHole_EdgeState* edge = edgeStates;
    for (int i = 0; i < edgeCount; ++i) {
        if (edge->vertexIndex0 == edgeState->vertexIndex0 || edge->vertexIndex1 == edgeState->vertexIndex0
            || edge->vertexIndex0 == edgeState->vertexIndex1 || edge->vertexIndex1 == edgeState->vertexIndex1) {
            ++edge;
            continue;
        }

        if (zGeometry_Segment::IntersectsSegmentXY(
                bridgeStart,
                bridgeEnd,
                &g_zGeometry_TriangulateHole_CombinedPoints[edge->vertexIndex0],
                &g_zGeometry_TriangulateHole_CombinedPoints[edge->vertexIndex1]
            )
            != 0) {
            return edgeCount;
        }

        ++edge;
    }

    edgeStates[edgeCount] = *edgeState;
    return edgeCount + 1;
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry_Segment {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-intersectssegmentxy
 * @recoil-artifact defines .text recoil:function:0x46be20: zGeometry_Segment::IntersectsSegmentXY
 * @recoil-match byte
 *
 * Purpose: Test whether two XY segments intersect with both parametric coordinates strictly inside the unit range.
 */
int __fastcall
IntersectsSegmentXY(zVec3* segmentAPoint0, zVec3* segmentAPoint1, zVec3* segmentBPoint0, zVec3* segmentBPoint1)
{
    const float segmentAX = segmentAPoint1->x - segmentAPoint0->x;
    const float segmentBX = segmentBPoint0->x - segmentBPoint1->x;
    const float segmentAY = segmentAPoint1->y - segmentAPoint0->y;
    const float segmentBY = segmentBPoint0->y - segmentBPoint1->y;
    const float determinant = segmentAX * segmentBY - segmentAY * segmentBX;

    if (determinant == 0.0f) {
        return 0;
    }

    const float originDeltaX = segmentBPoint0->x - segmentAPoint0->x;
    const float originDeltaY = segmentBPoint0->y - segmentAPoint0->y;
    const float segmentAParameter = (originDeltaX * segmentBY - originDeltaY * segmentBX) / determinant;
    const float segmentBParameter = (originDeltaY * segmentAX - originDeltaX * segmentAY) / determinant;

    if (segmentAParameter > 0.0f && segmentBParameter > 0.0f && segmentAParameter < 1.0f && segmentBParameter < 1.0f) {
        return 1;
    }

    return 0;
}
} // namespace zGeometry_Segment

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-collectactiveedgeindicesforvertex
 * @recoil-artifact defines .text recoil:function:0x46bf30: zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex
 * @recoil-match byte
 *
 * Purpose: Collect live edge-state indices incident to one combined-ring vertex.
 */
int __fastcall CollectActiveEdgeIndicesForVertex(
    int vertexIndex,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState* edgeStates,
    int* outEdgeIndices
)
{

    int* output = outEdgeIndices;
    int result = 0;
    for (int i = 0; i < edgeCount; ++i) {
        zGeometry_TriangulateHole_EdgeState* const edge = &edgeStates[i];
        if (edge->remainingUseCount != 0 && (edge->vertexIndex0 == vertexIndex || edge->vertexIndex1 == vertexIndex)) {
            *output++ = i;
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
 * @recoil-match byte
 *
 * Purpose: Find a live edge between two combined-ring vertex indices.
 */
zGeometry_TriangulateHole_EdgeState* __fastcall
FindActiveEdgeState(int vertexIndex0, int vertexIndex1, int edgeCount, zGeometry_TriangulateHole_EdgeState* edgeStates)
{

    zGeometry_TriangulateHole_EdgeState* edge = edgeStates;
    for (int i = 0; i < edgeCount; ++i, ++edge) {
        if (edge->remainingUseCount == 0) {
            if ((edge->vertexIndex0 == vertexIndex0 && edge->vertexIndex1 == vertexIndex1)
                || (edge->vertexIndex1 == vertexIndex0 && edge->vertexIndex0 == vertexIndex1)) {
                return 0;
            }
        } else {
            if ((edge->vertexIndex0 == vertexIndex0 && edge->vertexIndex1 == vertexIndex1)
                || (edge->vertexIndex1 == vertexIndex0 && edge->vertexIndex0 == vertexIndex1)) {
                return edge;
            }
        }
    }
    return 0;
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-tryemittrianglefromedgepair
 * @recoil-artifact defines .text recoil:function:0x46bfc0: zGeometry_TriangulateHole::TryEmitTriangleFromEdgePair
 * @recoil-match byte
 *
 * Purpose: Emit a triangle from two incident live edges and their closing edge.
 */
int __fastcall TryEmitTriangleFromEdgePair(
    int edgeIndex0,
    int edgeIndex1,
    int vertexIndex,
    int edgeCount,
    zGeometry_TriangulateHole_EdgeState* edgeStates
)
{
    zGeometry_TriangleIndexTriple* const triangle
        = &g_zGeometry_TriangulateHole_TriangleIndices[g_zGeometry_TriangulateHole_TriangleCount];
    zGeometry_TriangulateHole_EdgeState* const edge0 = &edgeStates[edgeIndex0];
    zGeometry_TriangulateHole_EdgeState* const edge1 = &edgeStates[edgeIndex1];

    if (edge0->remainingUseCount == 0 || edge1->remainingUseCount == 0) {
        return 0;
    }

    int vertexIndex0;
    if (edge0->vertexIndex0 == vertexIndex) {
        vertexIndex0 = edge0->vertexIndex1;
    } else {
        vertexIndex0 = edge0->vertexIndex0;
    }

    int vertexIndex1;
    if (edge1->vertexIndex0 == vertexIndex) {
        vertexIndex1 = edge1->vertexIndex1;
    } else {
        vertexIndex1 = edge1->vertexIndex0;
    }

    if (vertexIndex0 + vertexIndex1 + vertexIndex == 3) {
        return 0;
    }

    zGeometry_TriangulateHole_EdgeState* const closingEdge
        = FindActiveEdgeState(vertexIndex0, vertexIndex1, edgeCount, edgeStates);
    if (closingEdge == 0) {
        return 0;
    }

    triangle->i0 = vertexIndex0;
    triangle->i1 = vertexIndex1;
    triangle->i2 = vertexIndex;
    ++g_zGeometry_TriangulateHole_TriangleCount;

    --edge0->remainingUseCount;
    --edge1->remainingUseCount;
    --closingEdge->remainingUseCount;
    return 0;
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-triangulatepolygonwithhole
 * @recoil-artifact defines .text recoil:function:0x46c070: zGeometry::TriangulatePolygonWithHole
 *
 *
 * Purpose: Bridge an inner polygon ring to an outer ring and emit triangle soup.
 */
zGeometry_TriangleSoup* __fastcall
TriangulatePolygonWithHole(int outerPointCount, zVec3* outerPoints, int innerPointCount, zVec3* innerPoints)
{
    // One ring edge per combined point; the same count seeds the edge list below.
    int edgeCount = outerPointCount + innerPointCount;

    g_zGeometry_TriangulateHole_TriangleCount = 0;
    g_zGeometry_TriangulateHole_CombinedPointCount = edgeCount;

    zGeometry_TriangulateHole_EdgeState* const edgeStates = (zGeometry_TriangulateHole_EdgeState*)(malloc(
        (size_t)(edgeCount * edgeCount) * sizeof(zGeometry_TriangulateHole_EdgeState)
    ));

    g_zGeometry_TriangulateHole_CombinedPoints
        = (zVec3*)(malloc((size_t)(g_zGeometry_TriangulateHole_CombinedPointCount) * sizeof(zVec3)));

    memcpy(g_zGeometry_TriangulateHole_CombinedPoints, outerPoints, (size_t)(outerPointCount) * sizeof(zVec3));
    memcpy(
        &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount],
        innerPoints,
        (size_t)(innerPointCount) * sizeof(zVec3)
    );

    zGeometry_TriangulateHole::CacheCombinedPlane(outerPointCount, outerPoints);
    zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane(
        innerPointCount,
        &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount]
    );
    memcpy(
        innerPoints,
        &g_zGeometry_TriangulateHole_CombinedPoints[outerPointCount],
        (size_t)(innerPointCount) * sizeof(zVec3)
    );

    zGeometry_TriangulateHole_EdgeState* edge = edgeStates;
    edge->vertexIndex0 = 0;
    edge->vertexIndex1 = outerPointCount - 1;
    edge->remainingUseCount = 1;
    ++edge;

    {
        for (int outerIndex = 1; outerIndex < outerPointCount; ++outerIndex) {
            edge->vertexIndex0 = outerIndex - 1;
            edge->vertexIndex1 = outerIndex;
            edge->remainingUseCount = 1;
            ++edge;
        }
    }

    edge->vertexIndex0 = outerPointCount;
    edge->vertexIndex1 = edgeCount - 1;
    edge->remainingUseCount = 1;
    ++edge;

    {
        for (int innerIndex = 1; innerIndex < innerPointCount; ++innerIndex) {
            edge->vertexIndex0 = outerPointCount + innerIndex - 1;
            edge->vertexIndex1 = outerPointCount + innerIndex;
            edge->remainingUseCount = 1;
            ++edge;
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
                    edgeCount = zGeometry_TriangulateHole::TryAppendBridgeEdge(&bridgeEdge, edgeCount, edgeStates);
                }
            }
        }
    }

    {
        for (int vertexIndex = 0; vertexIndex < g_zGeometry_TriangulateHole_CombinedPointCount; ++vertexIndex) {
            int edgeIndices[0x20];
            const int activeEdgeCount = zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex(
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

    zGeometry_TriangleSoup* const result = (zGeometry_TriangleSoup*)(malloc(
        sizeof(int) + (size_t)(g_zGeometry_TriangulateHole_TriangleCount * 3) * sizeof(zVec3)
    ));
    result->triangleCount = g_zGeometry_TriangulateHole_TriangleCount;

    zVec3* outPoint = result->triangleVerts;
    for (int triangleIndex = 0; triangleIndex < g_zGeometry_TriangulateHole_TriangleCount; ++triangleIndex) {
        const zGeometry_TriangleIndexTriple* const triangle
            = &g_zGeometry_TriangulateHole_TriangleIndices[triangleIndex];
        *outPoint++ = g_zGeometry_TriangulateHole_CombinedPoints[triangle->i0];
        *outPoint++ = g_zGeometry_TriangulateHole_CombinedPoints[triangle->i1];
        *outPoint++ = g_zGeometry_TriangulateHole_CombinedPoints[triangle->i2];
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
 * @recoil-match byte
 *
 * Purpose: Cache the plane equation used to project the inner ring.
 */
void __fastcall CacheCombinedPlane(int pointCount, zVec3* points)
{
    zGeometry_Vec3Array::ComputeNewellPlane(pointCount, points, &g_zGeometry_TriangulateHole_CachedPlane);
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-computenewellplane
 * @recoil-artifact defines .text recoil:function:0x46c3a0: zGeometry_Vec3Array::ComputeNewellPlane
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zgeometry.compute-newell-plane.fast-sqrt-estimate recoil:function:0x46c3a0
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zgeometry.compute-newell-plane.fast-sqrt-estimate
 * @recoil-match byte
 *
 * Raw assembly: the reviewed full-XYZ dot island for the plane offset and one
 * in-body 13-byte fast-sqrt estimate island at retail [0x46c4b5,0x46c4c2);
 * zgeo_hole.cpp builds /Ob0, so an inline helper cannot expand.
 *
 * Purpose: Compute a normalized Newell plane equation from a point ring.
 */
void __fastcall ComputeNewellPlane(int pointCount, zVec3* points, zGeometry_PlaneEquationPartial* outPlane)
{
    zVec3 normal;
    zVec3 sum;
    normal.z = 0.0f;
    normal.y = 0.0f;
    normal.x = 0.0f;
    sum.z = 0.0f;
    sum.y = 0.0f;
    sum.x = 0.0f;

    for (int i = 0; i < pointCount; ++i) {
        zVec3* const point = &points[i];
        zVec3* const next = &points[(i + 1) % pointCount];

        normal.x += (point->y - next->y) * (point->z + next->z);
        normal.y += (point->z - next->z) * (point->x + next->x);
        normal.z += (point->x - next->x) * (point->y + next->y);

        sum.x += point->x;
        sum.y += point->y;
        sum.z += point->z;
    }

    float magnitude;
    if (normal.x == 0.0 && normal.y == 0.0 && normal.z == 0.0) {
        magnitude = 0.0f;
    } else {
        float lengthSq = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
        float estimate;
        // Raw-assembly fast square-root estimate: retail transforms the named lengthSq
        // bits through EAX ((bits >> 1) + 0x1fc00000) into the named estimate local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
            mov eax, lengthSq
            sar eax, 1
            add eax, 01fc00000h
            mov estimate, eax
        }
#else
        {
            int estimateBits;
            memcpy(&estimateBits, &lengthSq, sizeof estimateBits);
            estimateBits = (estimateBits >> 1) + 0x1fc00000;
            memcpy(&estimate, &estimateBits, sizeof estimate);
        }
#endif
        magnitude = estimate;
    }

    float normalScale;
    if (magnitude != 0.0) {
        normalScale = 1.0f / magnitude;
    } else {
        normalScale = 0.0f;
    }

    outPlane->a = normal.x * normalScale;
    outPlane->b = normal.y * normalScale;
    outPlane->c = normal.z * normalScale;

    float planeDot;
    ZMTH_VECTOR_DOT(planeDot, &sum, &normal);
    // A double-typed numerator temporary reproduces retail's numerator-first
    // x87 evaluation order under VC5; the original source type is not unique.
    const double planeOffset = planeDot;
    outPlane->d = -(planeOffset / ((float)pointCount * magnitude));
}
} // namespace zGeometry_Vec3Array

namespace zGeometry_TriangulateHole {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-projectinnerringontocachedplane
 * @recoil-artifact defines .text recoil:function:0x46c570: zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane
 * @recoil-match byte
 *
 * Purpose: Project inner-ring Z values onto the cached outer-ring plane.
 */
void __fastcall ProjectInnerRingOntoCachedPlane(int pointCount, zVec3* points)
{
    for (int i = 0; i < pointCount; ++i) {
        zVec3* const point = &points[i];
        point->z
            = -(g_zGeometry_TriangulateHole_CachedPlane.a * point->x
                  + g_zGeometry_TriangulateHole_CachedPlane.b * point->y + g_zGeometry_TriangulateHole_CachedPlane.d)
            / g_zGeometry_TriangulateHole_CachedPlane.c;
    }
}
} // namespace zGeometry_TriangulateHole

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-reversepoints
 * @recoil-artifact defines .text recoil:function:0x46c5b0: zGeometry_Vec3Array::ReversePoints
 * @recoil-match byte
 *
 * Purpose: Reverse all points after the anchor point in a polygon ring.
 */
void __fastcall ReversePoints(int pointCount, zVec3* points)
{

    int swapCount = pointCount / 2;
    zVec3* front = &points[1];
    zVec3* back = &points[pointCount - 1];
    while (swapCount--) {
        const zVec3 temp = *back;
        *back-- = *front;
        *front++ = temp;
    }
}
} // namespace zGeometry_Vec3Array

namespace zMath {
/**
 * Purpose: Inline-function spelling of the reviewed vector-cross island for
 * this unit's consumers. VC5 binds simple variable arguments to their own
 * homes and address arguments to inline-parameter homes, which the capturing
 * ZMTH_VECTOR_CROSS cannot express. Original header ownership is unrecovered.
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x46c620's cross-product island bound to the caller's homes.
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

namespace zGeometry_Vec3Array {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-ensurepositivecrossz
 * @recoil-artifact defines .text recoil:function:0x46c620: zGeometry_Vec3Array::EnsurePositiveCrossZ
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-match byte
 *
 * Purpose: Ensure the first two polygon edges produce a positive Z cross.
 */
int __fastcall EnsurePositiveCrossZ(int pointCount, zVec3* points, int allowReverse)
{
    zVec3 edge0;
    zVec3 edge1;
    zVec3 cross;
    zMath::Vec3Subtract(&points[1], &points[0], &edge0);
    zMath::Vec3Subtract(&points[2], &points[1], &edge1);
    zMath::Vec3Cross(&edge0, &edge1, &cross);

    if (!(cross.z > 0.0f)) {
        if (allowReverse == 0) {
            return 0;
        }

        ReversePoints(pointCount, points);
    }

    return 1;
}
} // namespace zGeometry_Vec3Array
