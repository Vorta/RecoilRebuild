#include "zgeo.h"

#include "GameZRecoil/zError/zerr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
/**
 * Data evidence: BN 0x4e050c..0x4e059f is the contiguous zgeo_convexify.cpp
 * diagnostic/source literal owner linked by engine.zgeometry.polygon_convexification.
 * Purpose: Preserve the writable source/error literals used by polygon convexification diagnostics.
 */
char g_zGeometry_ConvexifyNullInputsMsg[0x2a] = "convexify(): One or more inputs are null\n";
char g_zGeometry_SourceFile_ZgeoConvexifyCpp[0x31] = "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_convexify.cpp";
char g_zGeometry_ConvexifyInvalidInputPolygonSizeFmt[0x34] = "convexify(): Invalid input polygon size (%d) verts.";

/**
 * Data evidence: BN 0x4e05a0 is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate3ErrorMsg, referenced at 0x46ce97.
 * Purpose: Preserve the recursive triangulation first-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate3ErrorMsg[0x22] = "Error in recursive triangulate 3\n";

/**
 * Data evidence: BN 0x4e05c4 is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate4ErrorMsg, referenced at 0x46ce63.
 * Purpose: Preserve the recursive triangulation second-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate4ErrorMsg[0x22] = "Error in recursive triangulate 4\n";

/**
 * Data evidence: BN 0x4e05e8 is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate2ErrorMsg, referenced at 0x46cd76.
 * Purpose: Preserve the recursive triangulation non-triangle first-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate2ErrorMsg[0x22] = "Error in recursive triangulate 2\n";

/**
 * Data evidence: BN 0x4e060c is writable char[0x22]
 * g_zGeometry_RecursiveTriangulate1ErrorMsg, referenced at 0x46ccf5.
 * Purpose: Preserve the recursive triangulation non-triangle second-subpolygon failure diagnostic.
 */
char g_zGeometry_RecursiveTriangulate1ErrorMsg[0x22] = "Error in recursive triangulate 1\n";

/**
 * Data evidence: BN 0x4e0630 is writable char[0x2e]
 * g_zGeometry_TriangulateOnlyVertsReceivedFmt, referenced at 0x46cb6b.
 * Purpose: Preserve the recursive triangulation input-count guard diagnostic.
 */
char g_zGeometry_TriangulateOnlyVertsReceivedFmt[0x2e] = "Error in TRIANGULATE: only %d verts received\n";

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in callers 0x46ced0 and 0x46d140.
 * Purpose: Read the X component from a point-dword offset list.
 */
float OffsetX(const float* pointDwords, const int* pointDwordOffsets, int index, int stride)
{
    return pointDwords[pointDwordOffsets[index * stride]];
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in callers 0x46ced0 and 0x46d140.
 * Purpose: Read the Y component from a point-dword offset list.
 */
float OffsetY(const float* pointDwords, const int* pointDwordOffsets, int index, int stride)
{
    return pointDwords[pointDwordOffsets[index * stride + 1]];
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in callers 0x46ced0, 0x46d140, and zGeometry XY helpers.
 * Purpose: Compute the signed two-dimensional cross product.
 */
float Cross2D(float ax, float ay, float bx, float by, float cx, float cy)
{
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Accumulate signed polygon area from offset point dwords.
 */
inline float PolygonArea2D(const float* pointDwords, const int* pointDwordOffsets, int pointCount, int stride)
{
    float area = 0.0f;
    for (int i = 0; i < pointCount; ++i) {
        const int next = (i + 1) % pointCount;
        area += OffsetX(pointDwords, pointDwordOffsets, i, stride)
                * OffsetY(pointDwords, pointDwordOffsets, next, stride)
            - OffsetY(pointDwords, pointDwordOffsets, i, stride)
                * OffsetX(pointDwords, pointDwordOffsets, next, stride);
    }

    return area;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Copy one point's dword-offset tuple into triangle output storage.
 */
void CopyOffsetVertex(int* dest, const int* source, int stride)
{
    memcpy(dest, source, (size_t)(stride) * sizeof(int));
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in address-backed callers in this source file.
 * Purpose: Convert a point dword offset into the source float tuple base.
 */
const float* PointDwordBase(const zVec3* points, int pointDwordOffset)
{
    return (const float*)(points) + pointDwordOffset;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in address-backed callers in this source file.
 * Purpose: Append a contiguous source point span to the convexification result.
 */
zVec3* CopySpanPoints(
    zGeometry_ConvexPolygonSetPartial* result,
    zVec3* outputPointWriteCursor,
    const float* sourcePointDwords,
    int pointCount
)
{
    zGeometry_PolygonPointSpanPartial* polygon = &result->polygons[result->polygonCount];
    polygon->pointCount = pointCount;
    polygon->pointDwordOffset = result->totalPointCount * 3;

    memcpy(outputPointWriteCursor, sourcePointDwords, (size_t)(pointCount) * sizeof(zVec3));

    ++result->polygonCount;
    result->totalPointCount += pointCount;
    return outputPointWriteCursor + pointCount;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in address-backed callers in this source file.
 * Purpose: Triangulate and append a polygon span into triangle-sized output spans.
 */
zVec3* AppendTriangulatedSpan(
    zGeometry_ConvexPolygonSetPartial* result,
    zVec3* outputPointWriteCursor,
    const zGeometry_PolygonPointSpanPartial* inputPolygon,
    const zVec3* allPoints
)
{
    const float* sourcePointDwords = PointDwordBase(allPoints, inputPolygon->pointDwordOffset);
    zGeometry_TriangleDwordOffsetList* triangles = zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive(
        inputPolygon->pointCount,
        (float*)(sourcePointDwords),
        0,
        0
    );

    if (triangles == 0) {
        return outputPointWriteCursor;
    }

    const int* triangleOffsets = triangles->triangleDwordOffsets;
    {
        for (int triangle = 0; triangle < triangles->triangleCount; ++triangle) {
            zGeometry_PolygonPointSpanPartial* polygon = &result->polygons[result->polygonCount];
            polygon->pointCount = 3;
            polygon->pointDwordOffset = result->totalPointCount * 3;
            ++result->polygonCount;
            result->totalPointCount += 3;

            float* outputDwords = (float*)(outputPointWriteCursor);
            {
                for (int dwordIndex = 0; dwordIndex < 9; ++dwordIndex) {
                    outputDwords[dwordIndex] = sourcePointDwords[triangleOffsets[triangle * 9 + dwordIndex]];
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
int* TrianglePayload(zGeometry_TriangleDwordOffsetList* list)
{
    return list->triangleDwordOffsets;
}

/**
 * Original-source helper evidence: no standalone retail function is present.
 * Observed in caller 0x46ced0.
 * Purpose: Append one triangle's source point offsets into the output list.
 */
void AppendTriangleOffsets(
    zGeometry_TriangleDwordOffsetList* list,
    int triangleIndex,
    const int* polygonOffsets,
    int index0,
    int index1,
    int index2,
    int stride
)
{
    int* out = &TrianglePayload(list)[triangleIndex * stride * 3];
    CopyOffsetVertex(out, &polygonOffsets[index0 * stride], stride);
    CopyOffsetVertex(out + stride, &polygonOffsets[index1 * stride], stride);
    CopyOffsetVertex(out + stride * 2, &polygonOffsets[index2 * stride], stride);
}
} // namespace

namespace zGeometry_ConvexPolygonSet {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zgeometry-zgeo-convexify-destroy
 * @recoil-artifact defines .text recoil:function:0x46c720: zGeometry_ConvexPolygonSet::Destroy
 * @recoil-match byte
 *
 * Purpose: Release a convex polygon set and its owned point and polygon arrays.
 */
void __fastcall Destroy(zGeometry_ConvexPolygonSetPartial* self)
{
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
 * @recoil-artifact defines .text recoil:function:0x46c760: zGeometry_Polygon::convexify
 * @recoil-match byte
 *
 * Purpose: Convert polygon spans into convex polygon output, copying already
 * convex spans and triangulating non-convex spans through the polygon splitter.
 */
zGeometry_ConvexPolygonSetPartial* __fastcall
convexify(zGeometry_PolygonSpanArrayPartial* polygonSet, int inputPointCount, zVec3* points)
{
    if (inputPointCount <= 0 || points == 0) {
        fprintf(stderr, g_zGeometry_ConvexifyNullInputsMsg);
        return 0;
    }

    zGeometry_ConvexPolygonSetPartial* result
        = (zGeometry_ConvexPolygonSetPartial*)(malloc(sizeof(zGeometry_ConvexPolygonSetPartial)));
    result->points = (zVec3*)(malloc((size_t)(inputPointCount * 3 - 6) * sizeof(zVec3)));
    result->polygons = (zGeometry_PolygonPointSpanPartial*)(malloc(
        (size_t)(inputPointCount - 2) * sizeof(zGeometry_PolygonPointSpanPartial)
    ));
    result->totalPointCount = 0;
    result->polygonCount = 0;

    float* outputDwords = (float*)(result->points);
    zGeometry_PolygonPointSpanPartial* outputPolygon = result->polygons - 1;
    zGeometry_PolygonPointSpanPartial* inputPolygon = polygonSet->polygons;
    for (int remaining = polygonSet->polygonCount; remaining != 0; --remaining, ++inputPolygon) {
        const unsigned int pointCount = inputPolygon->pointCount;
        if (pointCount < 3) {
            continue;
        }

        if (pointCount == 3) {
            ++outputPolygon;
            outputPolygon->pointCount = 3;
            outputPolygon->pointDwordOffset = result->totalPointCount * 3;
            ++result->polygonCount;
            result->totalPointCount += 3;
            memcpy(outputDwords, (float*)(points) + inputPolygon->pointDwordOffset, 3 * sizeof(zVec3));
            outputDwords += 9;
        } else if (pointCount == 4) {
            zVec3* a = (zVec3*)((float*)(points) + inputPolygon->pointDwordOffset);
            zVec3* b = a + 1;
            zVec3* c = b + 1;
            int splitVertex = -1;
            for (int i = 0; i < 4; ++i) {
                if (i == 2) {
                    c = (zVec3*)((float*)(points) + inputPolygon->pointDwordOffset);
                } else if (i == 3) {
                    b = (zVec3*)((float*)(points) + inputPolygon->pointDwordOffset);
                }

                if ((c->x - a->x) * (b->y - a->y) - (b->x - a->x) * (c->y - a->y) > 0.0f) {
                    splitVertex = i + 1;
                    break;
                }

                ++a;
                ++b;
                ++c;
            }

            splitVertex %= 4;
            if (splitVertex < 0) {
                ++outputPolygon;
                outputPolygon->pointCount = 4;
                outputPolygon->pointDwordOffset = result->totalPointCount * 3;
                ++result->polygonCount;
                result->totalPointCount += 4;
                memcpy(outputDwords, (float*)(points) + inputPolygon->pointDwordOffset, 4 * sizeof(zVec3));
                outputDwords += 12;
            } else {
                result->polygonCount += 2;
                if ((splitVertex & 1) != 0) {
                    ++outputPolygon;
                    outputPolygon->pointCount = 3;
                    outputPolygon->pointDwordOffset = result->totalPointCount * 3;
                    result->totalPointCount += 3;
                    memcpy(outputDwords, (float*)(points) + inputPolygon->pointDwordOffset, 2 * sizeof(zVec3));
                    memcpy(outputDwords + 6, (float*)(points) + inputPolygon->pointDwordOffset + 9, sizeof(zVec3));
                    outputDwords += 9;
                    ++outputPolygon;
                    outputPolygon->pointCount = 3;
                    outputPolygon->pointDwordOffset = result->totalPointCount * 3;
                    result->totalPointCount += 3;
                    memcpy(outputDwords, (float*)(points) + inputPolygon->pointDwordOffset + 3, 3 * sizeof(zVec3));
                    outputDwords += 9;
                } else {
                    ++outputPolygon;
                    outputPolygon->pointCount = 3;
                    outputPolygon->pointDwordOffset = result->totalPointCount * 3;
                    result->totalPointCount += 3;
                    memcpy(outputDwords, (float*)(points) + inputPolygon->pointDwordOffset, 3 * sizeof(zVec3));
                    outputDwords += 9;
                    ++outputPolygon;
                    outputPolygon->pointCount = 3;
                    outputPolygon->pointDwordOffset = result->totalPointCount * 3;
                    result->totalPointCount += 3;
                    memcpy(outputDwords, (float*)(points) + inputPolygon->pointDwordOffset, sizeof(zVec3));
                    memcpy(outputDwords + 3, (float*)(points) + inputPolygon->pointDwordOffset + 6, 2 * sizeof(zVec3));
                    outputDwords += 9;
                }
            }
        } else if (pointCount > 4) {
            zGeometry_TriangleDwordOffsetList* const triangles
                = zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive(
                    pointCount,
                    (float*)(points) + inputPolygon->pointDwordOffset,
                    0,
                    0
                );
            result->polygonCount += triangles->triangleCount;
            const int* triangleOffset = triangles->triangleDwordOffsets;
            for (int triangle = triangles->triangleCount; triangle != 0; --triangle) {
                ++outputPolygon;
                outputPolygon->pointCount = 3;
                outputPolygon->pointDwordOffset = result->totalPointCount * 3;
                result->totalPointCount += 3;
                for (int dwordIndex = 9; dwordIndex != 0; --dwordIndex) {
                    *outputDwords++ = ((float*)(points))[*triangleOffset++ + inputPolygon->pointDwordOffset];
                }
            }

            free(triangles);
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
 *
 *
 * Purpose: Recursively split a polygon point-dword offset list and append the
 * resulting triangle offset lists.
 */
zGeometry_TriangleDwordOffsetList* __fastcall TriangulatePointDwordOffsetsRecursive(
    int pointCount,
    float* pointDwords,
    int* pointDwordOffsets,
    int pointDwordStrideMode
)
{
    if (pointCount < 3) {
        fprintf(stderr, g_zGeometry_TriangulateOnlyVertsReceivedFmt, pointCount);
        return 0;
    }

    const int pointDwordStride = pointDwordStrideMode == 1 ? 2 : 3;
    int* workingOffsets = pointDwordOffsets;
    if (workingOffsets == 0) {
        workingOffsets = (int*)(malloc((size_t)(pointCount * pointDwordStride) * sizeof(int)));
        for (int i = 0; i < pointCount * pointDwordStride; ++i) {
            workingOffsets[i] = i;
        }
    }

    const int triangleCount = pointCount - 2;
    zGeometry_TriangleDwordOffsetList* result = (zGeometry_TriangleDwordOffsetList*)(malloc(
        sizeof(int) + (size_t)(triangleCount * pointDwordStride * 3) * sizeof(int)
    ));
    result->triangleCount = triangleCount;

    zGeometry_PolygonSplitDwordOffsetListPair* splitPointLists = 0;
    if (pointCount != 3) {
        const int splitPointCount = pointCount + 2;
        splitPointLists = (zGeometry_PolygonSplitDwordOffsetListPair*)(malloc(
            sizeof(zGeometry_PolygonSplitDwordOffsetListPair)
            + (size_t)(splitPointCount * pointDwordStride - 1) * sizeof(int)
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

        int* outTriangleOffsets = result->triangleDwordOffsets;
        zGeometry_TriangleDwordOffsetList* triangles = 0;
        char* oneSideErrorMessage = 0;
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
                    splitPointLists->pointDwordOffsets + splitPointLists->pointCount0 * pointDwordStride,
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
                    splitPointLists->pointDwordOffsets + splitPointLists->pointCount0 * pointDwordStride,
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

    int* outTriangleOffsets = result->triangleDwordOffsets;
    memcpy(outTriangleOffsets, workingOffsets, (size_t)(pointDwordStride) * sizeof(int));
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
 *
 *
 * Purpose: Split a polygon point-dword offset list across the chosen diagonal
 * into two smaller polygon offset lists.
 */
int __fastcall TrySplitPointDwordOffsetsAtBestDiagonal(
    int pointCount,
    float* pointDwords,
    int* pointDwordOffsets,
    zGeometry_PolygonSplitDwordOffsetListPair* outSplitPointLists,
    int pointDwordStride
)
{
    int* minPoint = pointDwordOffsets;
    float minX = pointDwords[pointDwordOffsets[0]];
    int* point = pointDwordOffsets + pointDwordStride;
    int i;
    for (i = 1; i < pointCount; ++i) {
        if (pointDwords[point[0]] < minX
            || (pointDwords[point[0]] == minX && pointDwords[minPoint[1]] > pointDwords[point[1]])) {
            minX = pointDwords[point[0]];
            minPoint = point;
        }
        point += pointDwordStride;
    }

    int* prevPoint;
    if (minPoint == pointDwordOffsets) {
        prevPoint = minPoint + (pointCount - 1) * pointDwordStride;
    } else {
        prevPoint = minPoint - pointDwordStride;
    }

    int* nextPoint;
    if (minPoint == pointDwordOffsets + (pointCount - 1) * pointDwordStride) {
        nextPoint = pointDwordOffsets;
    } else {
        nextPoint = minPoint + pointDwordStride;
    }

    float minY;
    float maxY;
    if (pointDwords[prevPoint[1]] > pointDwords[nextPoint[1]]) {
        minY = pointDwords[nextPoint[1]];
        maxY = pointDwords[prevPoint[1]];
    } else {
        minY = pointDwords[prevPoint[1]];
        maxY = pointDwords[nextPoint[1]];
    }
    if (maxY < pointDwords[minPoint[1]]) {
        maxY = pointDwords[minPoint[1]];
    } else if (minY > pointDwords[minPoint[1]]) {
        minY = pointDwords[minPoint[1]];
    }

    int* farPoint;
    if (pointDwords[prevPoint[0]] > pointDwords[nextPoint[0]]) {
        farPoint = prevPoint;
    } else {
        farPoint = nextPoint;
    }

    bool found = false;
    float bestDistanceSq = 10000000.0f;
    int* bestPoint;
    point = pointDwordOffsets;
    for (i = 0; i < pointCount; ++i) {
        if (point != prevPoint && point != minPoint && point != nextPoint && pointDwords[point[1]] <= maxY
            && pointDwords[point[1]] >= minY && pointDwords[point[0]] < pointDwords[farPoint[0]]) {
            const int pIndex = point[0];
            const int mIndex = minPoint[0];
            const int nIndex = nextPoint[0];
            const int vIndex = prevPoint[0];
            const float mdx = pointDwords[mIndex] - pointDwords[pIndex];
            const float ndx = pointDwords[nIndex] - pointDwords[pIndex];
            const float ndy = pointDwords[nIndex + 1] - pointDwords[pIndex + 1];
            const float mdy = pointDwords[mIndex + 1] - pointDwords[pIndex + 1];
            const float vdx = pointDwords[vIndex] - pointDwords[pIndex];
            const float vdy = pointDwords[vIndex + 1] - pointDwords[pIndex + 1];
            if (ndy * mdx > ndx * mdy && vdy * ndx >= vdx * ndy && vdx * mdy > vdy * mdx) {
                const float dy = pointDwords[minPoint[1]] - pointDwords[point[1]];
                const float distanceSq = mdx * mdx + dy * dy;
                if (distanceSq < bestDistanceSq) {
                    bestDistanceSq = distanceSq;
                    bestPoint = point;
                    found = true;
                }
            }
        }
        point += pointDwordStride;
    }

    int* const end = pointDwordOffsets + pointCount * pointDwordStride;
    if (!found) {
        minPoint = prevPoint;
        if (farPoint == minPoint) {
            minPoint = nextPoint;
        }
    } else {
        farPoint = bestPoint;
    }

    int* out = outSplitPointLists->pointDwordOffsets;
    if (farPoint > minPoint) {
        outSplitPointLists->pointCount0 = (farPoint - minPoint) / pointDwordStride + 1;
        outSplitPointLists->pointCount1 = pointCount - outSplitPointLists->pointCount0 + 2;
        memcpy(out, minPoint, pointDwordStride * outSplitPointLists->pointCount0 * sizeof(int));
        out += pointDwordStride * outSplitPointLists->pointCount0;
        memcpy(out, farPoint, (end - farPoint) * sizeof(int));
        out += end - farPoint;
        memcpy(out, pointDwordOffsets, (minPoint - pointDwordOffsets + pointDwordStride) * sizeof(int));
        return 1;
    }

    outSplitPointLists->pointCount0 = (minPoint - farPoint) / pointDwordStride + 1;
    outSplitPointLists->pointCount1 = pointCount - outSplitPointLists->pointCount0 + 2;
    memcpy(out, farPoint, pointDwordStride * outSplitPointLists->pointCount0 * sizeof(int));
    out += pointDwordStride * outSplitPointLists->pointCount0;
    memcpy(out, minPoint, (end - minPoint) * sizeof(int));
    out += end - minPoint;
    memcpy(out, pointDwordOffsets, (farPoint - pointDwordOffsets + pointDwordStride) * sizeof(int));
    return 1;
}
} // namespace zGeometry_Polygon
