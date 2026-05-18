#include "zGeometry.h"

#include "zUtil/zutil.h"

#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zModel/zModel.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
const char kZGeoModelSourceFile[] = "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_model.cpp";
const float kRandToDebugColorScale = 0.00778221991f;

struct zGeometry_ClipPatchModelNodeBoundsView {
    zClass_NodePartial node;
    float boundsMinX;
    unsigned char unknown_90[0x04];
    float boundsNegMaxY;
    float boundsMaxX;
    unsigned char unknown_9c[0x04];
    float boundsNegMinY;
};

RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPatchModelNodeBoundsView, boundsMinX) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPatchModelNodeBoundsView, boundsNegMaxY) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPatchModelNodeBoundsView, boundsMaxX) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(zGeometry_ClipPatchModelNodeBoundsView, boundsNegMinY) == 0xa0);

zModel_DrawBatchBasePartial *ModelDrawBatchFromNode(zGeometry_ClipPatchNodeView *node) {
    return reinterpret_cast<zModel_DrawBatchBasePartial *>(
        static_cast<unsigned int>(node->userDataOrDiRef));
}

bool IsClipPatchNodeOutsideClipBoundsXY(zGeometry_ClipPolygonPartial *clipPolygon,
                                        zGeometry_ClipPatchNodeView *node) {
    zGeometry_ClipPatchModelNodeBoundsView *const modelBounds =
        reinterpret_cast<zGeometry_ClipPatchModelNodeBoundsView *>(node);

    if (modelBounds->boundsMinX > clipPolygon->bounds.maxX + 1.0f) {
        return true;
    }

    if (modelBounds->boundsMaxX < clipPolygon->bounds.minX - 1.0f) {
        return true;
    }

    if (clipPolygon->bounds.maxY - 1.0f > -modelBounds->boundsNegMaxY) {
        return true;
    }

    if (clipPolygon->bounds.minY + 1.0f < -modelBounds->boundsNegMinY) {
        return true;
    }

    return false;
}

zVec3 *PointAtDwordOffset(zVec3 *points, int pointDwordOffset) {
    return reinterpret_cast<zVec3 *>(reinterpret_cast<float *>(points) + pointDwordOffset);
}

zModel_MaterialPartial *g_zGeometry_Model_LastRandomDebugMaterial = 0;
} // namespace

namespace zGeometry_Bounds2D {
// Reimplements 0x46a620: zGeometry_Bounds2D::OverlapsWithUnitMargin
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL OverlapsWithUnitMargin(zGeometry_BoundsXY *boundsA,
                                                                    zGeometry_BoundsXY *boundsB) {
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

namespace zGeometry_Model {

// Reimplements 0x46b6d0: zGeometry_Model::ProcessClipPatchNode
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ProcessClipPatchNode(zGeometry_ClipPolygonPartial *clipPolygon, zModel_DrawBatchBasePartial *model,
                     zDiPartial **outDi) {
    if (model == 0 || clipPolygon == 0) {
        return 1;
    }

    zDiPartial *di = zModel_DiPool::AllocFromFreeList();
    if (di == 0) {
        return 0;
    }

    zGeometry_WeilerClipOutputPartial clipOutput;
    memset(&clipOutput, 0, sizeof(clipOutput));
    zUtil::StoreInt32(&di->mode, 0);

    zModel_PolygonPartial *polygon = model->faceList;
    zVec3 *polygonPointsBuffer = 0;
    int clipPolygonDirty = 0;
    int clipTouched = 0;

    for (int polygonIndex = 0; polygonIndex < model->faceCount;
         ++polygonIndex, ++polygon) {
        const int pointCount =
            static_cast<int>(polygon->vertexCountAndFlags & 0xff);
        if (pointCount < 3) {
            zError::ReportOld(0x400, kZGeoModelSourceFile, 0x4d7,
                              "Skipping clip of polygon with (%d) verts", pointCount);
            continue;
        }

        polygonPointsBuffer =
            zGeometry_Model::GetLinearBufferOfPolygonVertices(model, polygon, polygonPointsBuffer);
        if (polygonPointsBuffer == 0) {
            zError::ReportOld(0x400, kZGeoModelSourceFile, 0x4df,
                              "Error getting linear buffer of polygon vertices");
            continue;
        }

        zGeometry_Vec3Array::RotatePos90AroundX(pointCount, polygonPointsBuffer);

        zGeometry_BoundsXY bounds;
        zGeometry_Vec3Array::ComputeBoundsXY(&bounds, polygonPointsBuffer, pointCount);

        int clipResult = 1;
        if (zGeometry_Bounds2D::OverlapsWithUnitMargin(&bounds, &clipPolygon->bounds) != 0) {
            if (clipPolygonDirty != 0) {
                zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints(
                    clipPolygon, clipPolygon->points, clipPolygon->pointCount);
                clipPolygonDirty = 0;
            }

            clipResult = zGeometry_Weiler::ClipPointList(
                clipPolygon->weilerState, 3, polygonPointsBuffer, pointCount, &clipOutput);
        }

        switch (clipResult) {
        case 0:
            if (polygonPointsBuffer != 0) {
                free(polygonPointsBuffer);
            }
            zModel_DiPool::FreeIfUnreferenced(di);
            return 0;

        case 1:
            zGeometry_Model::AddIndexedPolygonToDi(di, model, polygon);
            break;

        case 2: {
            zGeometry_PolygonPointSpanPartial *const upsertPolygon =
                clipOutput.polygonSetA.polygons;
            clipTouched = 1;
            if (zGeometry_ClipPolygon::UpsertPointListXY(
                    clipPolygon, upsertPolygon->pointCount,
                    PointAtDwordOffset(clipOutput.pointList.points,
                                       upsertPolygon->pointDwordOffset)) != 0) {
                clipPolygonDirty = 1;
            }

            zGeometry_ConvexPolygonSetPartial *const convexSet = zGeometry_Polygon::Convexify(
                &clipOutput.polygonSetB, clipOutput.pointList.pointCount,
                clipOutput.pointList.points);
            if (convexSet != 0) {
                zGeometry_Vec3Array::RotateNeg90AroundX(convexSet->totalPointCount,
                                                        convexSet->points);

                zGeometry_PolygonPointSpanPartial *convexPolygon = convexSet->polygons;
                for (int convexIndex = 0; convexIndex < convexSet->polygonCount;
                     ++convexIndex, ++convexPolygon) {
                    if (convexPolygon->pointCount >= 3) {
                        zGeometry_Model::AddPointListPolygonToDi(
                            di, convexPolygon->pointCount,
                            PointAtDwordOffset(convexSet->points, convexPolygon->pointDwordOffset),
                            model, polygon);
                    }
                }

                zGeometry_ConvexPolygonSet::Destroy(convexSet);
            }
        } break;

        case 3:
            clipTouched = 1;
            break;

        case 4: {
            if (clipOutput.polygonSetB.polygonCount == 0) {
                zError::ReportOld(0x100, kZGeoModelSourceFile, 0x548,
                                  "\nWEILER_CLIP_IN_SUBJ\n\tclip.outside.num_polys = 0\n");
                if (polygonPointsBuffer != 0) {
                    free(polygonPointsBuffer);
                }
                zModel_DiPool::FreeIfUnreferenced(di);
                return 0;
            }

            zVec3 *inputContourPoints = 0;
            const int inputContourPointCount = zGeometry_Weiler::GetInputContourAPointList(
                clipPolygon->weilerState, &inputContourPoints);
            clipTouched = 1;

            zGeometry_TriangleSoup *triangleSoup = zGeometry::TriangulatePolygonWithHole(
                pointCount, polygonPointsBuffer, inputContourPointCount, inputContourPoints);

            if (zGeometry_ClipPolygon::UpsertPointListXY(clipPolygon, inputContourPointCount,
                                                         inputContourPoints) != 0) {
                clipPolygonDirty = 1;
            }

            if (triangleSoup->triangleCount < pointCount + inputContourPointCount) {
                if (polygonPointsBuffer != 0) {
                    free(polygonPointsBuffer);
                }
                zModel_DiPool::FreeIfUnreferenced(di);
                free(triangleSoup);
                return 0;
            }

            zVec3 *trianglePoints = triangleSoup->triangleVerts;
            for (int triangleIndex = 0; triangleIndex < triangleSoup->triangleCount;
                 ++triangleIndex) {
                zGeometry_Vec3Array::EnsurePositiveCrossZ(3, trianglePoints, 1);
                zGeometry_Vec3Array::RotateNeg90AroundX(3, trianglePoints);
                zGeometry_Model::AddPointListPolygonToDi(di, 3, trianglePoints, model, polygon);
                trianglePoints += 3;
            }

            free(triangleSoup);
        } break;

        default:
            break;
        }

        zGeometry_WeilerClipOutput::Destroy(&clipOutput);
    }

    if (clipTouched == 0) {
        zModel_DiPool::FreeIfUnreferenced(di);
        di = 0;
    }

    *outDi = di;

    if (polygonPointsBuffer != 0) {
        free(polygonPointsBuffer);
    }

    return 1;
}

// Reimplements 0x46b1f0: zGeometry_Model::ClipPatch
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPatch(int pointCount, zVec3 *points, zDEClient_FeatureGridCell *featureGridCell,
          zGeometry_ClipPatchOutputPartial *outClipPatchOutput) {
    if (featureGridCell == 0 || points == 0) {
        zError::ReportOld(
            0x100, kZGeoModelSourceFile, 0x3af,
            "Null Area Partition (0x%08x) or null Outline (0x%08x) passed to ClipPatch",
            featureGridCell, points);
        return -1;
    }

    zGeometry_ClipPolygonPartial *const clipPolygon =
        zGeometry_ClipPolygon::CreateFromPointList(pointCount, points);
    if (clipPolygon == 0) {
        return -1;
    }

    const int oldPartitionCount = outClipPatchOutput->partitionCount;
    outClipPatchOutput->partitions = static_cast<zGeometry_ClipPatchPartitionOutput *>(realloc(
        outClipPatchOutput->partitions, static_cast<size_t>(oldPartitionCount + 1) *
                                            sizeof(zGeometry_ClipPatchPartitionOutput)));
    ++outClipPatchOutput->partitionCount;

    zGeometry_ClipPatchPartitionOutput *const partitionOutput =
        &outClipPatchOutput->partitions[oldPartitionCount];
    partitionOutput->featureGridCell = featureGridCell;

    const int featureGridNodeCount = featureGridCell->nodeCount;
    partitionOutput->nodeDiPairCount = featureGridNodeCount;
    partitionOutput->nodeDiPairs = static_cast<zGeometry_ClipPatchNodeDiPair *>(calloc(
        static_cast<size_t>(featureGridNodeCount), sizeof(zGeometry_ClipPatchNodeDiPair)));

    zClass_NodePartial *const cameraNode = zDEClient::GetCameraNode();
    const int candidateCapacity = cameraNode->listCountB + featureGridNodeCount;
    zGeometry_ClipPatchNodeView **insideNodes =
        static_cast<zGeometry_ClipPatchNodeView **>(malloc(
            static_cast<size_t>(candidateCapacity) * sizeof(zGeometry_ClipPatchNodeView *)));
    zGeometry_ClipPatchNodeView **clipNodes =
        static_cast<zGeometry_ClipPatchNodeView **>(malloc(
            static_cast<size_t>(candidateCapacity) * sizeof(zGeometry_ClipPatchNodeView *)));

    int insideNodeCount = 0;
    int clipNodeCount = 0;

    {
    for (int nodeIndex = 0; nodeIndex < cameraNode->listCountB; ++nodeIndex) {
        zGeometry_ClipPatchNodeView *const node = cameraNode->listB[nodeIndex];
        if ((node->flags & 0x04) == 0) {
            continue;
        }

        if ((node->flags & 0x20000) != 0) {
            insideNodes[insideNodeCount] = node;
            ++insideNodeCount;
        }

        if ((node->flags & 0x10000) != 0) {
            clipNodes[clipNodeCount] = node;
            ++clipNodeCount;
        }
    }
    }

    {
    for (int nodeIndex = 0; nodeIndex < featureGridNodeCount; ++nodeIndex) {
        zGeometry_ClipPatchNodeView *const node = featureGridCell->nodes[nodeIndex];
        if ((node->flags & 0x04) == 0) {
            continue;
        }

        if ((node->flags & 0x20000) != 0) {
            insideNodes[insideNodeCount] = node;
            ++insideNodeCount;
        }

        if ((node->flags & 0x10000) != 0) {
            clipNodes[clipNodeCount] = node;
            ++clipNodeCount;
        }
    }
    }

    {
    for (int nodeIndex = 0; nodeIndex < clipNodeCount; ++nodeIndex) {
        if (zGeometry_ClipPolygon::SnapPointsNearNodeModelXY(clipPolygon, clipNodes[nodeIndex]) !=
            0) {
            zGeometry_Vec3Array::ComputeBoundsXY(&clipPolygon->bounds, clipPolygon->points,
                                                 clipPolygon->pointCount);
        }
    }
    }

    clipPolygon->weilerState =
        zGeometry_Weiler::Init(clipPolygon->points, clipPolygon->pointCount, 0);

    int result = 1;
    zGeometry_ClipPatchNodeDiPair *nodeDiPairWriteCursor = partitionOutput->nodeDiPairs;

    {
    for (int nodeIndex = 0; nodeIndex < insideNodeCount && result != 0; ++nodeIndex) {
        result = zGeometry_ClipPolygon::ProcessNodePolygonSetXY(clipPolygon, insideNodes[nodeIndex],
                                                                &nodeDiPairWriteCursor->di);
    }
    }

    int nodeDiPairCount = 0;
    if (result != 0) {
        nodeDiPairWriteCursor = partitionOutput->nodeDiPairs;
        {
        for (int nodeIndex = 0; nodeIndex < clipNodeCount && result != 0; ++nodeIndex) {
            nodeDiPairWriteCursor->node = clipNodes[nodeIndex];
            result = zGeometry_ClipPolygon::ProcessNodePolygonSetXY(
                clipPolygon, clipNodes[nodeIndex], &nodeDiPairWriteCursor->di);

            if (nodeDiPairWriteCursor->di != 0) {
                ++nodeDiPairCount;
                ++nodeDiPairWriteCursor;
            }
        }
        }
    }

    int returnValue = nodeDiPairCount;
    if (nodeDiPairCount != 0 && result != 0) {
        if (nodeDiPairCount != featureGridNodeCount) {
            partitionOutput->nodeDiPairCount = nodeDiPairCount;
            partitionOutput->nodeDiPairs =
                static_cast<zGeometry_ClipPatchNodeDiPair *>(realloc(
                    partitionOutput->nodeDiPairs, static_cast<size_t>(nodeDiPairCount) *
                                                      sizeof(zGeometry_ClipPatchNodeDiPair)));
        }

        zGeometry_ClipPolygon::CopyPointsOutRotatedBack(
            clipPolygon, &outClipPatchOutput->pointCount, &outClipPatchOutput->points);
    } else {
        --outClipPatchOutput->partitionCount;
        if (partitionOutput->nodeDiPairs != 0) {
            free(partitionOutput->nodeDiPairs);
        }

        if (outClipPatchOutput->partitionCount == 0) {
            free(outClipPatchOutput->partitions);
            outClipPatchOutput->partitions = 0;
        } else {
            outClipPatchOutput->partitions = static_cast<zGeometry_ClipPatchPartitionOutput *>(
                realloc(outClipPatchOutput->partitions,
                             static_cast<size_t>(outClipPatchOutput->partitionCount) *
                                 sizeof(zGeometry_ClipPatchPartitionOutput)));
        }

        returnValue = 0;
    }

    zGeometry_ClipPolygon::FinalizeAndDestroy(clipPolygon);

    if (insideNodes != 0) {
        free(insideNodes);
    }

    if (clipNodes != 0) {
        free(clipNodes);
    }

    return returnValue;
}
} // namespace zGeometry_Model

namespace zGeometry_ClipPolygon {
// Reimplements 0x46b550:
// zGeometry_ClipPolygon::ProcessNodePolygonSetXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ProcessNodePolygonSetXY(zGeometry_ClipPolygonPartial *clipPolygon,
                        zGeometry_ClipPatchNodeView *node, zDiPartial **outDi) {
    if (clipPolygon == 0 || node == 0) {
        return 1;
    }

    zModel_DrawBatchBasePartial *const model = ModelDrawBatchFromNode(node);
    if (model == 0) {
        return 1;
    }

    const int flags = node->flags;
    if ((flags & 0x200) != 0 && IsClipPatchNodeOutsideClipBoundsXY(clipPolygon, node)) {
        return 1;
    }

    if ((flags & 0x20000) != 0) {
        *outDi = 0;
        return zGeometry_Model::IsFullyInsideClipPolygonXY(clipPolygon, model);
    }

    if ((flags & 0x10000) != 0) {
        return zGeometry_Model::ProcessClipPatchNode(clipPolygon, model, outDi);
    }

    return 1;
}
} // namespace zGeometry_ClipPolygon

namespace zGeometry_Vec3 {
// Reimplements 0x469e50: zGeometry_Vec3::IsNearEqualXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IsNearEqualXY(zVec3 *vecA, zVec3 *vecB,
                                                           float tolerance) {
    if (fabs(vecA->x - vecB->x) <= tolerance && fabs(vecA->y - vecB->y) <= tolerance) {
        return 1;
    }

    return 0;
}

// Reimplements 0x469e90: zGeometry_Vec3::SnapPointToSegmentXYIfNear
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SnapPointToSegmentXYIfNear(zVec3 *lineStart,
                                                                        zVec3 *lineEnd,
                                                                        zVec3 *testPoint,
                                                                        float tolerance) {
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

namespace zGeometry_Polygon {
// Reimplements 0x46a8e0:
// zGeometry_Polygon::SolveUvAxisCoefficientsXZ
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SolveUvAxisCoefficientsXZ(zVec3 *point0, zVec3 *point1,
                                                               zVec3 *point2, float value0,
                                                               float value1, float value2,
                                                               zVec2 *outCoefficients) {
    const float x01 = point0->x - point1->x;
    const float z01 = point0->z - point1->z;
    const float x21 = point2->x - point1->x;
    const float z21 = point2->z - point1->z;
    const float determinant = z21 * x01 - x21 * z01;

    if (determinant == 0.0f) {
        outCoefficients->x = 0.0f;
        outCoefficients->y = 0.0f;
        return;
    }

    const float value01 = value0 - value1;
    const float value21 = value2 - value1;
    const float invDeterminant = 1.0f / determinant;
    outCoefficients->x = (z21 * value01 - value21 * z01) * invDeterminant;
    outCoefficients->y = (value21 * x01 - x21 * value01) * invDeterminant;
}

// Reimplements 0x46a130: zGeometry_Polygon::SnapPointsXYIfNear
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
SnapPointsXYIfNear(zVec3 *polygon, int polyCount, zVec3 *targetVerts,
                   int targetCount, float vertexTolerance, float edgeTolerance) {
    int result = 0;

    for (int i = 0; i < polyCount; ++i) {
        if (targetCount <= 0) {
            continue;
        }

        zVec3 *target = targetVerts;
        zVec3 *const polygonVertex = &polygon[i];
        for (int j = 0; j < targetCount; ++j) {
            if (zGeometry_Vec3::IsNearEqualXY(polygonVertex, target, vertexTolerance)) {
                result = 1;
                target->x = polygonVertex->x;
                target->y = polygonVertex->y;
                target->z = polygonVertex->z;
            } else {
                const int nextIndex = (i + 1) % polyCount;
                if (zGeometry_Vec3::SnapPointToSegmentXYIfNear(polygonVertex, &polygon[nextIndex],
                                                               target, edgeTolerance)) {
                    result = 1;
                }
            }

            ++target;
        }
    }

    return result;
}
} // namespace zGeometry_Polygon

namespace zGeometry_Vec3Array {
// Reimplements 0x46a5e0: zGeometry_Vec3Array::RotateNeg90AroundX
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RotateNeg90AroundX(int pointCount, zVec3 *points) {
    if (pointCount == 0) {
        return;
    }

    for (int i = 0; i < pointCount; ++i) {
        const float z = points[i].z;
        points[i].z = -points[i].y;
        points[i].y = z;
    }
}
} // namespace zGeometry_Vec3Array

namespace zGeometry_ClipPolygon {
// Reimplements 0x46b030:
// zGeometry_ClipPolygon::SnapPointsNearNodeModelXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SnapPointsNearNodeModelXY(
    zGeometry_ClipPolygonPartial *clipPolygon, zGeometry_ClipPatchNodeView *node) {
    zVec3 *linearPoints = 0;
    int result = 0;

    if (clipPolygon == 0 || node == 0) {
        return 0;
    }

    zModel_DrawBatchBasePartial *polygonSet = ModelDrawBatchFromNode(node);
    if (polygonSet == 0) {
        return result;
    }

    if ((node->flags & 0x200) != 0) {
        zGeometry_ClipPatchModelNodeBoundsView *modelBounds =
            reinterpret_cast<zGeometry_ClipPatchModelNodeBoundsView *>(node);

        if (modelBounds->boundsMinX > clipPolygon->bounds.maxX + 1.0f) {
            return 0;
        }

        if (modelBounds->boundsMaxX < clipPolygon->bounds.minX - 1.0f) {
            return 0;
        }

        if (clipPolygon->bounds.maxY - 1.0f > -modelBounds->boundsNegMaxY) {
            return 0;
        }

        if (clipPolygon->bounds.minY + 1.0f < -modelBounds->boundsNegMinY) {
            return 0;
        }
    }

    zModel_PolygonPartial *face = polygonSet->faceList;
    for (int i = 0; i < polygonSet->faceCount; ++i) {
        const unsigned int vertexCount = face->vertexCountAndFlags & 0xff;
        if (vertexCount < 3) {
            zError::ReportOld(0x400, kZGeoModelSourceFile, 0x36b,
                              "Skipping clip of polygon with (%d) verts", vertexCount);
        } else {
            linearPoints =
                zGeometry_Model::GetLinearBufferOfPolygonVertices(polygonSet, face, linearPoints);
            if (linearPoints == 0) {
                zError::ReportOld(0x400, kZGeoModelSourceFile, 0x375,
                                  "Error getting linear buffer of polygon vertices");
            } else {
                zGeometry_Vec3Array::RotatePos90AroundX(vertexCount, linearPoints);

                zGeometry_BoundsXY bounds;
                zGeometry_Vec3Array::ComputeBoundsXY(&bounds, linearPoints, vertexCount);

                if (zGeometry_Bounds2D::OverlapsWithUnitMargin(&bounds, &clipPolygon->bounds)) {
                    if (zGeometry_Polygon::SnapPointsXYIfNear(
                            linearPoints, vertexCount, clipPolygon->points, clipPolygon->pointCount,
                            0.100000001f, 0.100000001f)) {
                        result = 1;
                    }
                }
            }
        }

        ++face;
    }

    if (linearPoints != 0) {
        free(linearPoints);
    }

    return result;
}

// Reimplements 0x46aa40: zGeometry_ClipPolygon::CreateFromPointList
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE zGeometry_ClipPolygonPartial *RECOIL_FASTCALL
CreateFromPointList(int pointCount, zVec3 *points) {
    zGeometry_ClipPolygonPartial *result = static_cast<zGeometry_ClipPolygonPartial *>(
        malloc(sizeof(zGeometry_ClipPolygonPartial)));
    memset(result, 0, sizeof(zGeometry_ClipPolygonPartial));

    const size_t pointBytes = static_cast<size_t>(pointCount) * sizeof(zVec3);
    result->points = static_cast<zVec3 *>(malloc(pointBytes));
    memcpy(result->points, points, pointBytes);

    zGeometry_Vec3Array::RotatePos90AroundX(pointCount, result->points);
    result->pointCount = pointCount;
    zGeometry_Vec3Array::ComputeBoundsXY(&result->bounds, result->points, pointCount);

    return result;
}

// Reimplements 0x46aab0:
// zGeometry_ClipPolygon::CopyPointsOutRotatedBack
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CopyPointsOutRotatedBack(
    zGeometry_ClipPolygonPartial *clipPolygon, int *outPointCount, zVec3 **outPoints) {
    *outPointCount = clipPolygon->pointCount;

    const size_t pointBytes =
        static_cast<size_t>(clipPolygon->pointCount) * sizeof(zVec3);
    *outPoints = static_cast<zVec3 *>(realloc(*outPoints, pointBytes));
    memcpy(*outPoints, clipPolygon->points, pointBytes);

    zGeometry_Vec3Array::RotateNeg90AroundX(*outPointCount, *outPoints);
    return 0;
}
} // namespace zGeometry_ClipPolygon

namespace zGeometry_Model {
// Reimplements 0x46a690:
// zGeometry_Model::FindOrCreateRandomDebugMaterial
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_CDECL FindOrCreateRandomDebugMaterial() {
    zModel_MaterialPartial material;
    zModel_Material::ResetDefaults(&material);

    const float green = static_cast<float>(rand()) * kRandToDebugColorScale;
    const float red = static_cast<float>(rand()) * kRandToDebugColorScale;
    const float blue = static_cast<float>(rand()) * kRandToDebugColorScale;

    material.colorRgb.red = red;
    material.colorRgb.green = green;
    material.colorRgb.blue = blue;
    material.packedColor =
        static_cast<unsigned short>(((static_cast<int>(red) & 0x1f) << 11) |
                                   ((static_cast<int>(green) & 0x3f) << 5) |
                                   (static_cast<int>(blue) & 0x1f));

    g_zGeometry_Model_LastRandomDebugMaterial = zModel_Material::FindOrClone(&material);
    return g_zGeometry_Model_LastRandomDebugMaterial;
}

// Reimplements 0x46a770: zGeometry_Model::AddPolygonToDi
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AddPolygonToDi(zDiPartial *di, int pointCount,
                                                            zVec3 *points,
                                                            zModel_MaterialPartial *material,
                                                            zClipUV *uvPairs) {
    zTag4Partial localUserTag;
    zTag4::Clear(&localUserTag);

    if (pointCount < 3) {
        zError::ReportOld(0x800, kZGeoModelSourceFile, 0x9f,
                          "Attempting to generate polygon with (%d) vertices", pointCount);
        return -1;
    }

    if (uvPairs == 0 && material == 0) {
        material = FindOrCreateRandomDebugMaterial();
    }

    return zDi::AddPolygon(di, pointCount, points, uvPairs, 0, 0, 0, material, 0,
                           0, reinterpret_cast<const int *>(&localUserTag));
}

// Reimplements 0x46a7f0: zGeometry_Model::BuildPolygonUvList
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE zClipUV *RECOIL_FASTCALL BuildPolygonUvList(int pointCount, zVec3 *points,
                                                            zModel_DrawBatchBasePartial *model,
                                                            zModel_PolygonPartial *polygon) {
    const int *vertexIndices = polygon->vertexIndices;
    zVec3 *const verts = model->verts;
    zVec3 *const point0 = &verts[vertexIndices[0]];
    zVec3 *const point1 = &verts[vertexIndices[1]];
    zVec3 *const point2 = &verts[vertexIndices[2]];
    zModel_PolygonUvBasis *const uvBasis = polygon->uvBasis;

    zVec2 uCoefficients;
    zGeometry_Polygon::SolveUvAxisCoefficientsXZ(point0, point1, point2, uvBasis->uv0.u,
                                                 uvBasis->uv1.u, uvBasis->uv2.u, &uCoefficients);

    zVec2 vCoefficients;
    zGeometry_Polygon::SolveUvAxisCoefficientsXZ(point0, point1, point2, uvBasis->uv0.v,
                                                 uvBasis->uv1.v, uvBasis->uv2.v, &vCoefficients);

    zClipUV *const result =
        static_cast<zClipUV *>(malloc(static_cast<size_t>(pointCount) * sizeof(zClipUV)));
    for (int i = 0; i < pointCount; ++i) {
        const float deltaX = points[i].x - point1->x;
        const float deltaZ = points[i].z - point1->z;
        result[i].u = deltaX * uCoefficients.x + deltaZ * uCoefficients.y + uvBasis->uv1.u;
        result[i].v = deltaX * vCoefficients.x + deltaZ * vCoefficients.y + uvBasis->uv1.v;
    }

    return result;
}

// Reimplements 0x46ba90: zGeometry_Model::AddPointListPolygonToDi
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
AddPointListPolygonToDi(zDiPartial *di, int pointCount, zVec3 *points,
                        zModel_DrawBatchBasePartial *model, zModel_PolygonPartial *polygon) {
    if (pointCount < 3) {
        zError::ReportOld(0x800, kZGeoModelSourceFile, 0x111,
                          "Attempting to add child polygon with (%d) vertices", pointCount);
        return -1;
    }

    zClipUV *uvPairs = 0;
    zModel_MaterialPartial *material = 0;
    if (polygon->uvBasis != 0) {
        uvPairs = BuildPolygonUvList(pointCount, points, model, polygon);
        material = polygon->material;
    } else {
        material = FindOrCreateRandomDebugMaterial();
    }

    const int result = zDi::AddPolygon(
        di, pointCount, points, uvPairs, 0, 0, 0, material, polygon->drawFlags,
        static_cast<int>((polygon->vertexCountAndFlags >> 8) & 1), &polygon->userTag);

    if (uvPairs != 0) {
        free(uvPairs);
    }

    return result;
}

// Reimplements 0x46bb30: zGeometry_Model::AddIndexedPolygonToDi
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AddIndexedPolygonToDi(
    zDiPartial *di, zModel_DrawBatchBasePartial *model, zModel_PolygonPartial *polygon) {
    zVec3 *polygonPointsBuffer = GetLinearBufferOfPolygonVertices(model, polygon, 0);
    const unsigned int vertexCountAndFlags = polygon->vertexCountAndFlags;
    const int result = zDi::AddPolygon(
        di, static_cast<int>(vertexCountAndFlags & 0xff), polygonPointsBuffer,
        reinterpret_cast<zClipUV *>(polygon->uvBasis), 0, 0, 0, polygon->material,
        polygon->drawFlags, static_cast<int>((vertexCountAndFlags >> 8) & 1),
        &polygon->userTag);

    if (polygonPointsBuffer != 0) {
        free(polygonPointsBuffer);
    }

    return result;
}

// Reimplements 0x46bb90:
// zGeometry_Model::IsFullyInsideClipPolygonXY
// (D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IsFullyInsideClipPolygonXY(
    zGeometry_ClipPolygonPartial *clipPolygon, zModel_DrawBatchBasePartial *model) {
    zVec3 *polygonPointsBuffer = 0;

    if (model == 0 || clipPolygon == 0) {
        return 0;
    }

    zGeometry_WeilerClipOutputPartial clipOutput;
    memset(&clipOutput, 0, sizeof(clipOutput));

    zModel_PolygonPartial *face = model->faceList;
    {
    for (int polygonIndex = 0; polygonIndex < model->faceCount; ++polygonIndex, ++face) {
        const int pointCount = static_cast<int>(face->vertexCountAndFlags & 0xff);
        if (pointCount < 3) {
            zError::ReportOld(0x400, kZGeoModelSourceFile, 0x5ce,
                              "Skipping clip of polygon with (%d) verts", pointCount);
            continue;
        }

        polygonPointsBuffer =
            zGeometry_Model::GetLinearBufferOfPolygonVertices(model, face, polygonPointsBuffer);
        if (polygonPointsBuffer == 0) {
            zError::ReportOld(0x400, kZGeoModelSourceFile, 0x5d5,
                              "Error getting linear buffer of polygon vertices");
            continue;
        }

        zGeometry_Vec3Array::RotatePos90AroundX(pointCount, polygonPointsBuffer);

        zGeometry_BoundsXY bounds;
        zGeometry_Vec3Array::ComputeBoundsXY(&bounds, polygonPointsBuffer, pointCount);

        int clipResult = 1;
        if (zGeometry_Bounds2D::OverlapsWithUnitMargin(&bounds, &clipPolygon->bounds) != 0) {
            clipResult = zGeometry_Weiler::ClipPointList(
                clipPolygon->weilerState, 4, polygonPointsBuffer, pointCount, &clipOutput);
        }

        switch (clipResult) {
        case 0:
            zError::ReportOld(0x200, kZGeoModelSourceFile, 0x5ed,
                              "Weiler algorithm clip error occurred.");
            if (polygonPointsBuffer != 0) {
                free(polygonPointsBuffer);
            }
            return 0;

        case 1:
            zGeometry_WeilerClipOutput::Destroy(&clipOutput);
            break;

        case 2:
            if (clipOutput.polygonSetC.polygonCount == 0) {
                zError::ReportOld(0x200, kZGeoModelSourceFile, 0x5f7,
                                  "Intersection found, no polygons...");
            }
            if (polygonPointsBuffer != 0) {
                free(polygonPointsBuffer);
            }
            return 0;

        case 3:
        case 4:
            if (polygonPointsBuffer != 0) {
                free(polygonPointsBuffer);
            }
            return 0;

        default:
            zGeometry_WeilerClipOutput::Destroy(&clipOutput);
            break;
        }
    }
    }

    if (polygonPointsBuffer != 0) {
        free(polygonPointsBuffer);
    }

    return 1;
}
} // namespace zGeometry_Model
