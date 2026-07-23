#include "zgeo.h"

#include "GameZRecoil/zUtil/zutil.h"

#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zModel/gmod.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
const float kRandToDebugColorScale = 0.00778221991f;

/**
 * Data evidence: BN 0x4e034c..0x4e0509 is the contiguous zgeo_model.cpp
 * diagnostic literal owner linked by geometry_model_assets.zgeometry_model_initialized_data.
 * Purpose: Preserve the writable source/error literals used by model clipping diagnostics.
 */
char g_zGeometry_SourceFile_ZgeoModelCpp[0x2d] =
    "D:\\Proj\\GameZRecoil\\zGeometry\\zgeo_model.cpp";
char g_zGeometry_GeneratePolygonVertexCountFmt[0x32] =
    "Attempting to generate polygon with (%d) vertices";
char g_zGeometry_PolygonVertexBufferErrorMsg[0x30] =
    "Error getting linear buffer of polygon vertices";
char g_zGeometry_SkippingClipPolygonVertsFmt[0x29] =
    "Skipping clip of polygon with (%d) verts";
char g_zGeometry_NullAreaPartitionOrOutlineClipPatchFmt[0x4a] =
    "Null Area Partition (0x%08x) or null Outline (0x%08x) passed to ClipPatch";
char g_zGeometry_WeilerClipInSubjTraceMsg[0x32] =
    "\nWEILER_CLIP_IN_SUBJ\n\tclip.outside.num_polys = 0\n";
char g_zGeometry_AddChildPolygonVertexCountFmt[0x33] =
    "Attempting to add child polygon with (%d) vertices";
char g_zGeometry_IntersectionFoundNoPolygonsMsg[0x23] =
    "Intersection found, no polygons...";
char g_zGeometry_WeilerAlgorithmClipErrorMsg[0x26] =
    "Weiler algorithm clip error occurred.";

struct zGeometry_ClipPatchModelNodeBoundsView {
    zClass_NodePartial node;
    float boundsMinX;
    unsigned char unknown_90[0x04];
    float boundsNegMaxY;
    float boundsMaxX;
    unsigned char unknown_9c[0x04];
    float boundsNegMinY;
};

RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPatchModelNodeBoundsView,
        boundsMinX
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPatchModelNodeBoundsView,
        boundsNegMaxY
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPatchModelNodeBoundsView,
        boundsMaxX
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGeometry_ClipPatchModelNodeBoundsView,
        boundsNegMinY
    ) == 0xa0
);

/**
 * Original-source helper evidence: no standalone retail function; observed in address-backed
 * callers 0x46b550 and 0x46b030 in this source file.
 * Purpose: Recover the model draw-batch pointer stored in a clip-patch node.
 */
zModel_DrawBatchBasePartial *ModelDrawBatchFromNode(
    zGeometry_ClipPatchNodeView *node
) {
    return (zModel_DrawBatchBasePartial *)((unsigned int)(node->userDataOrDiRef));
}

/**
 * Original helper evidence: no standalone retail function is assigned;
 * observed in address-backed caller 0x46b550 in this source file.
 * Purpose: reject clip-patch model nodes whose XY bounds sit outside the
 * active clip polygon bounds with the retail one-unit margin.
 */
bool IsClipPatchNodeOutsideClipBoundsXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zGeometry_ClipPatchNodeView *node
) {
    zGeometry_ClipPatchModelNodeBoundsView *const modelBounds =
        (zGeometry_ClipPatchModelNodeBoundsView *)(node);

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

/**
 * Original-source helper evidence: no standalone retail function; observed in address-backed
 * caller 0x46b6d0 in this source file.
 * Purpose: Address a point list by the recovered dword offset stored in clipped polygon spans.
 */
zVec3 *PointAtDwordOffset(
    zVec3 *points,
    int pointDwordOffset
) {
    return (zVec3 *)((float *)(points) + pointDwordOffset);
}

/**
 * Reimplements data 0x53a73c: g_zGeometry_Model_LastRandomDebugMaterial.
 * Purpose: Remember the last randomized debug material cloned for generated model polygons.
 */
zModel_MaterialPartial *g_zGeometry_Model_LastRandomDebugMaterial = 0;
} // namespace

namespace zGeometry_Model {
/**
 * Reimplements 0x46a690: zGeometry_Model::FindOrCreateRandomDebugMaterial
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Create or reuse a randomized debug material and remember the last result.
 */
zModel_MaterialPartial *FindOrCreateRandomDebugMaterial() {
    zModel_MaterialPartial material;
    zModel_Material::ResetDefaults(&material);

    const float green = (float)(rand()) * kRandToDebugColorScale;
    const float red = (float)(rand()) * kRandToDebugColorScale;
    const float blue = (float)(rand()) * kRandToDebugColorScale;

    material.colorRgb.red = red;
    material.colorRgb.green = green;
    material.colorRgb.blue = blue;
    material.packedColor = (unsigned short)((((int)(red) & 0x1f) << 11) |
                                            (((int)(green) & 0x3f) << 5) | ((int)(blue) & 0x1f));

    g_zGeometry_Model_LastRandomDebugMaterial = zModel_Material::FindOrClone(&material);
    return g_zGeometry_Model_LastRandomDebugMaterial;
}

/**
 * Reimplements 0x46a770: zGeometry_Model::AddPolygonToDi
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: validate a generated polygon, choose a debug material when needed,
 * and forward the point list to the DI polygon sink.
 */
int __fastcall AddPolygonToDi(
    zDiPartial *di,
    int pointCount,
    zVec3 *points,
    zModel_MaterialPartial *material,
    zClipUV *uvPairs
) {
    zTag4Partial localUserTag;
    zTag4::Clear(&localUserTag);

    if (pointCount < 3) {
        zError::ReportOld(
            0x800,
            g_zGeometry_SourceFile_ZgeoModelCpp,
            0x9f,
            g_zGeometry_GeneratePolygonVertexCountFmt,
            pointCount
        );
        return -1;
    }

    if (uvPairs == 0 && material == 0) {
        material = FindOrCreateRandomDebugMaterial();
    }

    return zDi::AddPolygon(
        di,
        pointCount,
        points,
        uvPairs,
        0,
        0,
        0,
        material,
        0,
        0,
        (const int *)(&localUserTag)
    );
}

/**
 * Reimplements 0x46a7f0: zGeometry_Model::BuildPolygonUvList
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Allocate and fill clipped polygon UVs from the source polygon UV basis.
 */
zClipUV *__fastcall BuildPolygonUvList(
    int pointCount,
    zVec3 *points,
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon
) {
    const int *vertexIndices = polygon->vertexIndices;
    zVec3 *const verts = model->verts;
    zVec3 *const point0 = &verts[vertexIndices[0]];
    zVec3 *const point1 = &verts[vertexIndices[1]];
    zVec3 *const point2 = &verts[vertexIndices[2]];
    zModel_PolygonUvBasis *const uvBasis = polygon->uvBasis;

    zVec2 uCoefficients;
    zGeometry_Polygon::SolveUvAxisCoefficientsXZ(
        point0,
        point1,
        point2,
        uvBasis->uv0.u,
        uvBasis->uv1.u,
        uvBasis->uv2.u,
        &uCoefficients
    );

    zVec2 vCoefficients;
    zGeometry_Polygon::SolveUvAxisCoefficientsXZ(
        point0,
        point1,
        point2,
        uvBasis->uv0.v,
        uvBasis->uv1.v,
        uvBasis->uv2.v,
        &vCoefficients
    );

    zClipUV *const result = (zClipUV *)(malloc((size_t)(pointCount) * sizeof(zClipUV)));
    for (int i = 0; i < pointCount; ++i) {
        const float deltaX = points[i].x - point1->x;
        const float deltaZ = points[i].z - point1->z;
        result[i].u = deltaX * uCoefficients.x + deltaZ * uCoefficients.y + uvBasis->uv1.u;
        result[i].v = deltaX * vCoefficients.x + deltaZ * vCoefficients.y + uvBasis->uv1.v;
    }

    return result;
}

} // namespace zGeometry_Model

namespace zGeometry_Polygon {
/**
 * Reimplements 0x46a8e0: zGeometry_Polygon::SolveUvAxisCoefficientsXZ
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Solve XZ-plane linear coefficients for one polygon UV axis.
 */
void __fastcall SolveUvAxisCoefficientsXZ(
    zVec3 *point0,
    zVec3 *point1,
    zVec3 *point2,
    float value0,
    float value1,
    float value2,
    zVec2 *outCoefficients
) {
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

} // namespace zGeometry_Polygon

namespace zGeometry_Vec3Array {
/**
 * Reimplements 0x46a9c0: zGeometry_Vec3Array::ComputeBoundsXY
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Compute XY min/max bounds for a point array.
 */
void __fastcall ComputeBoundsXY(
    zGeometry_BoundsXY *outBounds,
    zVec3 *points,
    int pointCount
) {
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

namespace zGeometry_ClipPolygon {
/**
 * Reimplements 0x46aa40: zGeometry_ClipPolygon::CreateFromPointList
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Allocate a clip polygon, rotate source points, and initialize bounds/state.
 */
zGeometry_ClipPolygonPartial *__fastcall CreateFromPointList(
    int pointCount,
    zVec3 *points
) {
    zGeometry_ClipPolygonPartial *result =
        (zGeometry_ClipPolygonPartial *)(malloc(sizeof(zGeometry_ClipPolygonPartial)));
    memset(
        result,
        0,
        sizeof(zGeometry_ClipPolygonPartial)
    );

    const size_t pointBytes = (size_t)(pointCount) * sizeof(zVec3);
    result->points = (zVec3 *)(malloc(pointBytes));
    memcpy(
        result->points,
        points,
        pointBytes
    );

    zGeometry_Vec3Array::RotatePos90AroundX(
        pointCount,
        result->points
    );
    result->pointCount = pointCount;
    zGeometry_Vec3Array::ComputeBoundsXY(
        &result->bounds,
        result->points,
        pointCount
    );

    return result;
}

/**
 * Reimplements 0x46aab0: zGeometry_ClipPolygon::CopyPointsOutRotatedBack
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Copy clip polygon points to caller storage and restore model-space rotation.
 */
int __fastcall CopyPointsOutRotatedBack(
    zGeometry_ClipPolygonPartial *clipPolygon,
    int *outPointCount,
    zVec3 **outPoints
) {
    *outPointCount = clipPolygon->pointCount;

    const size_t pointBytes = (size_t)(clipPolygon->pointCount) * sizeof(zVec3);
    *outPoints = (zVec3 *)(realloc(
        *outPoints,
        pointBytes
    ));
    memcpy(
        *outPoints,
        clipPolygon->points,
        pointBytes
    );

    zGeometry_Vec3Array::RotateNeg90AroundX(
        *outPointCount,
        *outPoints
    );
    return 0;
}

/**
 * Reimplements 0x46ab10: zGeometry_ClipPolygon::FinalizeAndDestroy
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Release clip polygon point storage and associated Weiler state.
 */
void __fastcall FinalizeAndDestroy(
    zGeometry_ClipPolygonPartial *clipPolygon
) {
    if (clipPolygon->points != 0) {
        free(clipPolygon->points);
    }

    zGeometry_Weiler::DestroyState(clipPolygon->weilerState);
    free(clipPolygon);
}

/**
 * Reimplements 0x46ab40: zGeometry_ClipPolygon::FindPointIndexXY
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Find the first clip-polygon point whose XY coordinates match the candidate point within tolerance.
 */
int __fastcall FindPointIndexXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zVec3 *point
) {
    for (int i = 0; i < clipPolygon->pointCount; ++i) {
        if (zGeometry_Vec3::IsNearEqualXY(
            &clipPolygon->points[i],
            point,
            0.00999999978f
        )) {
            return i;
        }
    }

    return -1;
}

/**
 * Reimplements 0x46ab90: zGeometry_ClipPolygon::UpsertPointListXY
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Update matching clip-polygon points and insert candidate points that lie on clip-polygon edges.
 */
int __fastcall UpsertPointListXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    int pointCount,
    zVec3 *points
) {
    int result = 0;
    if (pointCount <= 0) {
        return result;
    }

    zVec3 *point = points;
    {
        for (int remaining = pointCount; remaining != 0; --remaining) {
            const int existingIndex = zGeometry_ClipPolygon::FindPointIndexXY(
                clipPolygon,
                point
            );
            if (existingIndex != -1) {
                clipPolygon->points[existingIndex] = *point;
                result = 1;
            } else {
                const int edgeIndex =
                    zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex(
                        clipPolygon,
                        point
                    );
                if (edgeIndex != -1) {
                    const int oldPointCount = clipPolygon->pointCount;
                    clipPolygon->points = (zVec3 *)(realloc(
                        clipPolygon->points,
                        (oldPointCount + 1) * sizeof(zVec3)
                    ));

                    if (edgeIndex != oldPointCount - 1) {
                        memmove(
                            &clipPolygon->points[edgeIndex + 2],
                            &clipPolygon->points[edgeIndex + 1],
                            (oldPointCount - edgeIndex - 1) * sizeof(zVec3)
                        );
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

/**
 * Reimplements 0x46ac80: zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Find the clip-polygon edge that contains a candidate point in XY.
 */
int __fastcall FindPointInsertionEdgeXYIndex(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zVec3 *point
) {
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
            if (fabs(tX - tY) < tolerance && tY > 0.0f && tY < 1.0f && tX > 0.0f && tX < 1.0f &&
                fabs(tX * edgeDx + current->x - point->x) <= tolerance &&
                fabs(tY * edgeDy + current->y - point->y) <= tolerance) {
                return i;
            }
        }

        current = next;
    }

    return -1;
}

} // namespace zGeometry_ClipPolygon

namespace zGeometry_ClipPatchOutput {
/**
 * Reimplements 0x46ae40: zGeometry_ClipPatchOutput::ApplyNodeDiPairs
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 *
 * Purpose: publish generated display instances to their clip-patch nodes,
 * release replaced display instances, and clear consumed node/DI pairs.
 */
int __fastcall ApplyNodeDiPairs(
    zGeometry_ClipPatchOutputPartial *self
) {
    {
        for (int partitionIndex = 0; partitionIndex < self->partitionCount; ++partitionIndex) {
            zGeometry_ClipPatchPartitionOutput *const partition = &self->partitions[partitionIndex];
            for (int i = 0; i < partition->nodeDiPairCount; ++i) {
                zGeometry_ClipPatchNodeDiPair *const pair = &partition->nodeDiPairs[i];

                unsigned int oldDisplayInstanceValue = 0;
                zClass_Class::gwNodeGetUserData(
                    pair->node,
                    &oldDisplayInstanceValue
                );
                zClass_Class::gwNodeSetDisplayInstance(
                    pair->node,
                    pair->di
                );

                if (oldDisplayInstanceValue != 0) {
                    zModel_DiPool::FreeIfUnreferenced(
                        (zDiPartial *)((unsigned int)(oldDisplayInstanceValue))
                    );
                }
            }

            ++partition->featureGridCell->featureCount;

            if (partition->nodeDiPairs != 0) {
                free(partition->nodeDiPairs);
                partition->nodeDiPairs = 0;
                partition->nodeDiPairCount = 0;
            }
        }
    }

    return 0;
}

/**
 * Reimplements 0x46af00: zGeometry_ClipPatchOutput::Create
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 *
 * Purpose: allocate an empty clip-patch output record for crater and quicksand
 * feature tessellation.
 */
zGeometry_ClipPatchOutputPartial *Create() {
    zGeometry_ClipPatchOutputPartial *result =
        (zGeometry_ClipPatchOutputPartial *)(malloc(sizeof(zGeometry_ClipPatchOutputPartial)));
    result->pointCount = 0;
    result->points = 0;
    result->partitionCount = 0;
    result->partitions = 0;
    return result;
}

/**
 * Reimplements 0x46af20: zGeometry_ClipPatchOutput::Destroy
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 *
 * Purpose: free the partition buffer owned by a clip-patch output record and
 * release the record itself.
 */
void __fastcall Destroy(
    zGeometry_ClipPatchOutputPartial *self
) {
    if (self->partitions != 0) {
        free(self->partitions);
    }

    free(self);
}

} // namespace zGeometry_ClipPatchOutput

namespace zDEClient {
/**
 * Reimplements 0x46af40:
 * zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 *
 * Purpose: allocate a feature child node and display instance for a clipped
 * partition while preserving the original node type and parent linkage.
 */
zDiPartial *__fastcall CreateFeatureNodeAndDiFromClipPatchPartition(
    zGeometry_ClipPatchPartitionOutput *partitionOutput,
    zClass_NodePartial *parentNode,
    zClass_NodePartial **outNode
) {
    if (partitionOutput == 0) {
        return 0;
    }

    zClass_NodePartial *child = zClass_Object3D::gwObject3DInit();
    if (child == 0) {
        if (outNode != 0) {
            *outNode = child;
        }

        return 0;
    }

    if (outNode != 0) {
        *outNode = child;
    }

    zClass_Class::gwNodeSetNodeType(
        child,
        0xff
    );

    for (int i = 0; i < partitionOutput->nodeDiPairCount; ++i) {
        zGeometry_ClipPatchNodeView *const node = partitionOutput->nodeDiPairs[i].node;
        if ((node->flags & 0x10000) == 0) {
            continue;
        }

        int nodeType;
        zClass_Class::gwNodeGetNodeType(
            node,
            &nodeType
        );
        if (nodeType != 0xff) {
            zClass_Class::gwNodeSetNodeType(
                child,
                nodeType
            );
            break;
        }
    }

    zClass_Class::gwNodeSetFlag17(
        child,
        1
    );

    zDiPartial *const displayInstance = zModel_DiPool::AllocFromFreeList();
    if (displayInstance == 0) {
        if (outNode != 0) {
            *outNode = 0;
        }

        zClass_Object3D::DeleteNode(child);
        return 0;
    }

    zClass_Class::AddChild(
        parentNode,
        child
    );
    zClass_Class::gwNodeSetDisplayInstance(
        child,
        displayInstance
    );
    return displayInstance;
}

} // namespace zDEClient

namespace zGeometry_ClipPolygon {
/**
 * Reimplements 0x46b030: zGeometry_ClipPolygon::SnapPointsNearNodeModelXY
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Snap clip polygon points to nearby model polygon edges in XY space.
 */
int __fastcall SnapPointsNearNodeModelXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zGeometry_ClipPatchNodeView *node
) {
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
            (zGeometry_ClipPatchModelNodeBoundsView *)(node);

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
            zError::ReportOld(
                0x400,
                g_zGeometry_SourceFile_ZgeoModelCpp,
                0x36b,
                g_zGeometry_SkippingClipPolygonVertsFmt,
                vertexCount
            );
        } else {
            linearPoints =
                zGeometry_Model::GetLinearBufferOfPolygonVertices(
                    polygonSet,
                    face,
                    linearPoints
                );
            if (linearPoints == 0) {
                zError::ReportOld(
                    0x400,
                    g_zGeometry_SourceFile_ZgeoModelCpp,
                    0x375,
                    g_zGeometry_PolygonVertexBufferErrorMsg
                );
            } else {
                zGeometry_Vec3Array::RotatePos90AroundX(
                    vertexCount,
                    linearPoints
                );

                zGeometry_BoundsXY bounds;
                zGeometry_Vec3Array::ComputeBoundsXY(
                    &bounds,
                    linearPoints,
                    vertexCount
                );

                if (zGeometry_Bounds2D::OverlapsWithUnitMargin(
                    &bounds,
                    &clipPolygon->bounds
                )) {
                    if (zGeometry_Polygon::SnapPointsXYIfNear(
                            linearPoints,
                            vertexCount,
                            clipPolygon->points,
                            clipPolygon->pointCount,
                            0.100000001f,
                            0.100000001f
                        )) {
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

} // namespace zGeometry_ClipPolygon

namespace zGeometry_Model {
/**
 * Reimplements 0x46b1f0: zGeometry_Model::ClipPatch
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Clip an outline against visible feature-grid nodes and build patch output.
 */
int __fastcall ClipPatch(
    int pointCount,
    zVec3 *points,
    zDEClient_FeatureGridCell *featureGridCell,
    zGeometry_ClipPatchOutputPartial *outClipPatchOutput
) {
    if (featureGridCell == 0 || points == 0) {
        zError::ReportOld(
            0x100,
            g_zGeometry_SourceFile_ZgeoModelCpp,
            0x3af,
            g_zGeometry_NullAreaPartitionOrOutlineClipPatchFmt,
            featureGridCell,
            points
        );
        return -1;
    }

    zGeometry_ClipPolygonPartial *const clipPolygon =
        zGeometry_ClipPolygon::CreateFromPointList(
            pointCount,
            points
        );
    if (clipPolygon == 0) {
        return -1;
    }

    const int oldPartitionCount = outClipPatchOutput->partitionCount;
    outClipPatchOutput->partitions = (zGeometry_ClipPatchPartitionOutput *)(realloc(
        outClipPatchOutput->partitions,
        (size_t)(oldPartitionCount + 1) * sizeof(zGeometry_ClipPatchPartitionOutput)
    ));
    ++outClipPatchOutput->partitionCount;

    zGeometry_ClipPatchPartitionOutput *const partitionOutput =
        &outClipPatchOutput->partitions[oldPartitionCount];
    partitionOutput->featureGridCell = featureGridCell;

    const int featureGridNodeCount = featureGridCell->nodeCount;
    partitionOutput->nodeDiPairCount = featureGridNodeCount;
    partitionOutput->nodeDiPairs = (zGeometry_ClipPatchNodeDiPair *)(calloc(
        (size_t)(featureGridNodeCount),
        sizeof(zGeometry_ClipPatchNodeDiPair)
    ));

    zClass_NodePartial *const cameraNode = zDEClient::GetCameraNode();
    const int candidateCapacity = cameraNode->listCountB + featureGridNodeCount;
    zGeometry_ClipPatchNodeView **insideNodes = (zGeometry_ClipPatchNodeView **)(malloc(
        (size_t)(candidateCapacity) * sizeof(zGeometry_ClipPatchNodeView *)
    ));
    zGeometry_ClipPatchNodeView **clipNodes = (zGeometry_ClipPatchNodeView **)(malloc(
        (size_t)(candidateCapacity) * sizeof(zGeometry_ClipPatchNodeView *)
    ));

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
            if (zGeometry_ClipPolygon::SnapPointsNearNodeModelXY(
                    clipPolygon,
                    clipNodes[nodeIndex]
                ) != 0) {
                zGeometry_Vec3Array::ComputeBoundsXY(
                    &clipPolygon->bounds,
                    clipPolygon->points,
                    clipPolygon->pointCount
                );
            }
        }
    }

    clipPolygon->weilerState =
        zGeometry_Weiler::Init(
            clipPolygon->points,
            clipPolygon->pointCount,
            0
        );

    int result = 1;
    zGeometry_ClipPatchNodeDiPair *nodeDiPairWriteCursor = partitionOutput->nodeDiPairs;

    {
        for (int nodeIndex = 0; nodeIndex < insideNodeCount && result != 0; ++nodeIndex) {
            result = zGeometry_ClipPolygon::ProcessNodePolygonSetXY(
                clipPolygon,
                insideNodes[nodeIndex],
                &nodeDiPairWriteCursor->di
            );
        }
    }

    int nodeDiPairCount = 0;
    if (result != 0) {
        nodeDiPairWriteCursor = partitionOutput->nodeDiPairs;
        {
            for (int nodeIndex = 0; nodeIndex < clipNodeCount && result != 0; ++nodeIndex) {
                nodeDiPairWriteCursor->node = clipNodes[nodeIndex];
                result = zGeometry_ClipPolygon::ProcessNodePolygonSetXY(
                    clipPolygon,
                    clipNodes[nodeIndex],
                    &nodeDiPairWriteCursor->di
                );

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
            partitionOutput->nodeDiPairs = (zGeometry_ClipPatchNodeDiPair *)(realloc(
                partitionOutput->nodeDiPairs,
                (size_t)(nodeDiPairCount) * sizeof(zGeometry_ClipPatchNodeDiPair)
            ));
        }

        zGeometry_ClipPolygon::CopyPointsOutRotatedBack(
            clipPolygon,
            &outClipPatchOutput->pointCount,
            &outClipPatchOutput->points
        );
    } else {
        --outClipPatchOutput->partitionCount;
        if (partitionOutput->nodeDiPairs != 0) {
            free(partitionOutput->nodeDiPairs);
        }

        if (outClipPatchOutput->partitionCount == 0) {
            free(outClipPatchOutput->partitions);
            outClipPatchOutput->partitions = 0;
        } else {
            outClipPatchOutput->partitions = (zGeometry_ClipPatchPartitionOutput *)(realloc(
                outClipPatchOutput->partitions,
                (size_t)(outClipPatchOutput->partitionCount) *
                    sizeof(zGeometry_ClipPatchPartitionOutput)
            ));
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
/**
 * Reimplements 0x46b550: zGeometry_ClipPolygon::ProcessNodePolygonSetXY
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Process a node polygon set against the clip polygon in XY space.
 */
int __fastcall ProcessNodePolygonSetXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zGeometry_ClipPatchNodeView *node,
    zDiPartial **outDi
) {
    if (clipPolygon == 0 || node == 0) {
        return 1;
    }

    zModel_DrawBatchBasePartial *const model = ModelDrawBatchFromNode(node);
    if (model == 0) {
        return 1;
    }

    const int flags = node->flags;
    if ((flags & 0x200) != 0 && IsClipPatchNodeOutsideClipBoundsXY(
        clipPolygon,
        node
    )) {
        return 1;
    }

    if ((flags & 0x20000) != 0) {
        *outDi = 0;
        return zGeometry_Model::IsFullyInsideClipPolygonXY(
            clipPolygon,
            model
        );
    }

    if ((flags & 0x10000) != 0) {
        return zGeometry_Model::ProcessClipPatchNode(
            clipPolygon,
            model,
            outDi
        );
    }

    return 1;
}

} // namespace zGeometry_ClipPolygon

namespace zGeometry_Model {
/**
 * Reimplements 0x46b650: zGeometry_Model::GetLinearBufferOfPolygonVertices
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Expand a model polygon's indexed vertices into a linear point buffer.
 */
zVec3 *__fastcall GetLinearBufferOfPolygonVertices(
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon,
    zVec3 *points
) {
    const unsigned int vertexCount = polygon->vertexCountAndFlags & 0xff;
    zVec3 *result = (zVec3 *)(realloc(
        points,
        vertexCount * sizeof(zVec3)
    ));

    for (unsigned int i = 0; i < vertexCount; ++i) {
        const int vertexIndex = polygon->vertexIndices[i];
        result[i] = model->verts[vertexIndex];
    }

    return result;
}

/**
 * Reimplements 0x46b6d0: zGeometry_Model::ProcessClipPatchNode
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Clip one model node against the active patch polygon and return DI output.
 */
int __fastcall ProcessClipPatchNode(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zModel_DrawBatchBasePartial *model,
    zDiPartial **outDi
) {
    if (model == 0 || clipPolygon == 0) {
        return 1;
    }

    zDiPartial *di = zModel_DiPool::AllocFromFreeList();
    if (di == 0) {
        return 0;
    }

    zGeometry_WeilerClipOutputPartial clipOutput;
    memset(
        &clipOutput,
        0,
        sizeof(clipOutput)
    );
    zUtil::StoreInt32(
        &di->mode,
        0
    );

    zModel_PolygonPartial *polygon = model->faceList;
    zVec3 *polygonPointsBuffer = 0;
    int clipPolygonDirty = 0;
    int clipTouched = 0;

    for (int polygonIndex = 0; polygonIndex < model->faceCount; ++polygonIndex, ++polygon) {
        const int pointCount = (int)(polygon->vertexCountAndFlags & 0xff);
        if (pointCount < 3) {
            zError::ReportOld(
                0x400,
                g_zGeometry_SourceFile_ZgeoModelCpp,
                0x4d7,
                g_zGeometry_SkippingClipPolygonVertsFmt,
                pointCount
            );
            continue;
        }

        polygonPointsBuffer =
            zGeometry_Model::GetLinearBufferOfPolygonVertices(
                model,
                polygon,
                polygonPointsBuffer
            );
        if (polygonPointsBuffer == 0) {
            zError::ReportOld(
                0x400,
                g_zGeometry_SourceFile_ZgeoModelCpp,
                0x4df,
                g_zGeometry_PolygonVertexBufferErrorMsg
            );
            continue;
        }

        zGeometry_Vec3Array::RotatePos90AroundX(
            pointCount,
            polygonPointsBuffer
        );

        zGeometry_BoundsXY bounds;
        zGeometry_Vec3Array::ComputeBoundsXY(
            &bounds,
            polygonPointsBuffer,
            pointCount
        );

        int clipResult = 1;
        if (zGeometry_Bounds2D::OverlapsWithUnitMargin(
            &bounds,
            &clipPolygon->bounds
        ) != 0) {
            if (clipPolygonDirty != 0) {
                zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints(
                    clipPolygon,
                    clipPolygon->points,
                    clipPolygon->pointCount
                );
                clipPolygonDirty = 0;
            }

            clipResult = zGeometry_Weiler::ClipPointList(
                clipPolygon->weilerState,
                3,
                polygonPointsBuffer,
                pointCount,
                &clipOutput
            );
        }

        switch (clipResult) {
        case 0:
            if (polygonPointsBuffer != 0) {
                free(polygonPointsBuffer);
            }
            zModel_DiPool::FreeIfUnreferenced(di);
            return 0;

        case 1:
            zGeometry_Model::AddIndexedPolygonToDi(
                di,
                model,
                polygon
            );
            break;

        case 2: {
            zGeometry_PolygonPointSpanPartial *const upsertPolygon =
                clipOutput.polygonSetA.polygons;
            clipTouched = 1;
            if (zGeometry_ClipPolygon::UpsertPointListXY(
                    clipPolygon,
                    upsertPolygon->pointCount,
                    PointAtDwordOffset(clipOutput.pointList.points, upsertPolygon->pointDwordOffset)
                ) != 0) {
                clipPolygonDirty = 1;
            }

            zGeometry_ConvexPolygonSetPartial *const convexSet = zGeometry_Polygon::Convexify(
                &clipOutput.polygonSetB,
                clipOutput.pointList.pointCount,
                clipOutput.pointList.points
            );
            if (convexSet != 0) {
                zGeometry_Vec3Array::RotateNeg90AroundX(
                    convexSet->totalPointCount,
                    convexSet->points
                );

                zGeometry_PolygonPointSpanPartial *convexPolygon = convexSet->polygons;
                for (int convexIndex = 0; convexIndex < convexSet->polygonCount;
                    ++convexIndex, ++convexPolygon) {
                    if (convexPolygon->pointCount >= 3) {
                        zGeometry_Model::AddPointListPolygonToDi(
                            di,
                            convexPolygon->pointCount,
                            PointAtDwordOffset(
                                convexSet->points,
                                convexPolygon->pointDwordOffset
                            ),
                            model,
                            polygon
                        );
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
                zError::ReportOld(
                    0x100,
                    g_zGeometry_SourceFile_ZgeoModelCpp,
                    0x548,
                    g_zGeometry_WeilerClipInSubjTraceMsg
                );
                if (polygonPointsBuffer != 0) {
                    free(polygonPointsBuffer);
                }
                zModel_DiPool::FreeIfUnreferenced(di);
                return 0;
            }

            zVec3 *inputContourPoints = 0;
            const int inputContourPointCount = zGeometry_Weiler::GetInputContourAPointList(
                clipPolygon->weilerState,
                &inputContourPoints
            );
            clipTouched = 1;

            zGeometry_TriangleSoup *triangleSoup = zGeometry::TriangulatePolygonWithHole(
                pointCount,
                polygonPointsBuffer,
                inputContourPointCount,
                inputContourPoints
            );

            if (zGeometry_ClipPolygon::UpsertPointListXY(
                    clipPolygon,
                    inputContourPointCount,
                    inputContourPoints
                ) != 0) {
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
                zGeometry_Vec3Array::EnsurePositiveCrossZ(
                    3,
                    trianglePoints,
                    1
                );
                zGeometry_Vec3Array::RotateNeg90AroundX(
                    3,
                    trianglePoints
                );
                zGeometry_Model::AddPointListPolygonToDi(
                    di,
                    3,
                    trianglePoints,
                    model,
                    polygon
                );
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

/**
 * Reimplements 0x46ba90: zGeometry_Model::AddPointListPolygonToDi
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: add a clipped child polygon to a DI, rebuilding UVs from the source
 * model polygon when UV basis data is present.
 */
int __fastcall AddPointListPolygonToDi(
    zDiPartial *di,
    int pointCount,
    zVec3 *points,
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon
) {
    if (pointCount < 3) {
        zError::ReportOld(
            0x800,
            g_zGeometry_SourceFile_ZgeoModelCpp,
            0x111,
            g_zGeometry_AddChildPolygonVertexCountFmt,
            pointCount
        );
        return -1;
    }

    zClipUV *uvPairs = 0;
    zModel_MaterialPartial *material = 0;
    if (polygon->uvBasis != 0) {
        uvPairs = BuildPolygonUvList(
            pointCount,
            points,
            model,
            polygon
        );
        material = polygon->material;
    } else {
        material = FindOrCreateRandomDebugMaterial();
    }

    const int result = zDi::AddPolygon(
        di,
        pointCount,
        points,
        uvPairs,
        0,
        0,
        0,
        material,
        polygon->drawFlags,
        (int)((polygon->vertexCountAndFlags >> 8) & 1),
        &polygon->userTag
    );

    if (uvPairs != 0) {
        free(uvPairs);
    }

    return result;
}

/**
 * Reimplements 0x46bb30: zGeometry_Model::AddIndexedPolygonToDi
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: expand an indexed model polygon into a temporary point list and
 * submit it to the DI polygon sink with its source material, UVs, and tag.
 */
int __fastcall AddIndexedPolygonToDi(
    zDiPartial *di,
    zModel_DrawBatchBasePartial *model,
    zModel_PolygonPartial *polygon
) {
    zVec3 *polygonPointsBuffer = GetLinearBufferOfPolygonVertices(
        model,
        polygon,
        0
    );
    const unsigned int vertexCountAndFlags = polygon->vertexCountAndFlags;
    const int result = zDi::AddPolygon(
        di,
        (int)(vertexCountAndFlags & 0xff),
        polygonPointsBuffer,
        (zClipUV *)(polygon->uvBasis),
        0,
        0,
        0,
        polygon->material,
        polygon->drawFlags,
        (int)((vertexCountAndFlags >> 8) & 1),
        &polygon->userTag
    );

    if (polygonPointsBuffer != 0) {
        free(polygonPointsBuffer);
    }

    return result;
}

/**
 * Reimplements 0x46bb90: zGeometry_Model::IsFullyInsideClipPolygonXY
 * Source: D:\Proj\GameZRecoil\zGeometry\zgeo_model.cpp
 * Purpose: Test whether every model polygon lies fully inside the clip polygon.
 */
int __fastcall IsFullyInsideClipPolygonXY(
    zGeometry_ClipPolygonPartial *clipPolygon,
    zModel_DrawBatchBasePartial *model
) {
    zVec3 *polygonPointsBuffer = 0;

    if (model == 0 || clipPolygon == 0) {
        return 0;
    }

    zGeometry_WeilerClipOutputPartial clipOutput;
    memset(
        &clipOutput,
        0,
        sizeof(clipOutput)
    );

    zModel_PolygonPartial *face = model->faceList;
    {
        for (int polygonIndex = 0; polygonIndex < model->faceCount; ++polygonIndex, ++face) {
            const int pointCount = (int)(face->vertexCountAndFlags & 0xff);
            if (pointCount < 3) {
                zError::ReportOld(
                    0x400,
                    g_zGeometry_SourceFile_ZgeoModelCpp,
                    0x5ce,
                    g_zGeometry_SkippingClipPolygonVertsFmt,
                    pointCount
                );
                continue;
            }

            polygonPointsBuffer =
                zGeometry_Model::GetLinearBufferOfPolygonVertices(
                    model,
                    face,
                    polygonPointsBuffer
                );
            if (polygonPointsBuffer == 0) {
                zError::ReportOld(
                    0x400,
                    g_zGeometry_SourceFile_ZgeoModelCpp,
                    0x5d5,
                    g_zGeometry_PolygonVertexBufferErrorMsg
                );
                continue;
            }

            zGeometry_Vec3Array::RotatePos90AroundX(
                pointCount,
                polygonPointsBuffer
            );

            zGeometry_BoundsXY bounds;
            zGeometry_Vec3Array::ComputeBoundsXY(
                &bounds,
                polygonPointsBuffer,
                pointCount
            );

            int clipResult = 1;
            if (zGeometry_Bounds2D::OverlapsWithUnitMargin(
                &bounds,
                &clipPolygon->bounds
            ) != 0) {
                clipResult = zGeometry_Weiler::ClipPointList(
                    clipPolygon->weilerState,
                    4,
                    polygonPointsBuffer,
                    pointCount,
                    &clipOutput
                );
            }

            switch (clipResult) {
            case 0:
                zError::ReportOld(
                    0x200,
                    g_zGeometry_SourceFile_ZgeoModelCpp,
                    0x5ed,
                    g_zGeometry_WeilerAlgorithmClipErrorMsg
                );
                if (polygonPointsBuffer != 0) {
                    free(polygonPointsBuffer);
                }
                return 0;

            case 1:
                zGeometry_WeilerClipOutput::Destroy(&clipOutput);
                break;

            case 2:
                if (clipOutput.polygonSetC.polygonCount == 0) {
                    zError::ReportOld(
                        0x200,
                        g_zGeometry_SourceFile_ZgeoModelCpp,
                        0x5f7,
                        g_zGeometry_IntersectionFoundNoPolygonsMsg
                    );
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


namespace zGeometry_Model {

} // namespace zGeometry_Model

namespace zGeometry_ClipPolygon {
} // namespace zGeometry_ClipPolygon

namespace zGeometry_Polygon {
} // namespace zGeometry_Polygon

namespace zGeometry_ClipPolygon {
} // namespace zGeometry_ClipPolygon

namespace zGeometry_Model {
} // namespace zGeometry_Model
