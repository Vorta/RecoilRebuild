#include "recoil/Mfc42Abi.h"
#include "zdi.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"

#include <math.h>
#include <string.h>

CZDisplayInstanceRaycastFilterRuntime g_CZDisplayInstance_RaycastFilterRuntime = { 0 };

namespace
{
    const int kNodeClassCamera = 1;
    const int kNodeClassObject3D = 5;
    const int kNodeClassLod = 6;
    const int kNodeClassSequence = 7;
    const int kNodeClassAnimate = 8;
    const int kNodeClassLight = 9;
    const int kNodeClassSound = 10;
    const int kNodeFlagEnabledForPick = 0x04;
    const int kNodeFlagRaycastable = 0x10;
    const int kNodeFlagPointCandidate = 0x20;
    const int kNodeFlagFilterRegionCandidate = 0x40;
    const int kNodeFlagCachedBoundsValid = 0x100;
    const int kNodeFlagRequiresLineOfSight = 1 << 22;
    const int kNodeFlagUseLocalMatrixMode3 = 0x80000;
    const int kNodeFlagClearDuringPick = 0x02000000;
    const int kObjectFlagTransformDirty = 0x01;
    const int kObjectFlagNoPickMatrixPush = 0x08;
    const int kObjectFlagUseCachedWorldMatrix = 0x20;
    const int kMaxPickCandidates = 0x20;
    const double kPickEdgeInsideEpsilon = -0.0001;
    const unsigned short kPickFaceBatchDamageMaskUvFlag = 0x0100;
    const unsigned short kPickFaceTexturedDamageMaskFlag = 0x0200;

    /**
     * Grid cell coordinate pair used by the grid-window walks.
     * Evidence: the retail frames of 0x445f60 and 0x446a80 keep every (col, row)
     * pair in adjacent dwords; 0x445f60 even keeps the unused home of its
     * register-held window minimum row.
     * Purpose: address one world area-grid cell by column and row.
     */
    struct GridCell {
        int col;
        int row;
    };

    /**
     * Original static helper observed in cls_di polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: absolute float component for dominant-axis selection.
     */
    float AbsFloat(float value)
    {
        return value < 0.0f ? -value : value;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: read float sign bits for side-test rejection.
     */
    unsigned int FloatBits(float value)
    {
        union {
            float f;
            unsigned int u;
        } bits = { value };
        return bits.u;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: three-component dot product for plane side tests.
     */
    float Dot3(const zVec3* a, const zVec3* b)
    {
        return a->x * b->x + a->y * b->y + a->z * b->z;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build the segment endpoint delta used by plane tests.
     */
    zVec3 Delta3(const zVec3* a, const zVec3* b)
    {
        zVec3 result = { a->x - b->x, a->y - b->y, a->z - b->z };
        return result;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: projected 2D edge cross product for winding tests.
     */
    double ProjectedEdgeCross(const zVec3* edgeStart, const zVec3* edgeEnd, const zVec3* point, int axis)
    {
        switch (axis) {
        case 0:
            return (edgeEnd->y - edgeStart->y) * (point->z - edgeStart->z)
                - (edgeEnd->z - edgeStart->z) * (point->y - edgeStart->y);
        case 1:
            return (edgeEnd->x - edgeStart->x) * (point->z - edgeStart->z)
                - (edgeEnd->z - edgeStart->z) * (point->x - edgeStart->x);
        default:
            return (edgeEnd->x - edgeStart->x) * (point->y - edgeStart->y)
                - (edgeEnd->y - edgeStart->y) * (point->x - edgeStart->x);
        }
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the dominant normal component for 2D projection.
     */
    int DominantAxis(const zVec3* normal)
    {
        int axis = 0;
        float maxAbs = AbsFloat(normal->x);
        const float absY = AbsFloat(normal->y);
        if (absY > maxAbs) {
            maxAbs = absY;
            axis = 1;
        }
        const float absZ = AbsFloat(normal->z);
        if (absZ > maxAbs) {
            axis = 2;
        }
        return axis;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: read the normal component selected by DominantAxis.
     */
    float DominantAxisComponent(const zVec3* normal, int axis)
    {
        if (axis == 0) {
            return normal->x;
        }
        if (axis == 1) {
            return normal->y;
        }
        return normal->z;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: orient projected polygon winding against the dominant axis.
     */
    int ProjectedWindingSign(const zVec3* normal, int axis)
    {
        const int componentIsNegative = DominantAxisComponent(normal, axis) < 0.0f ? 1 : 0;
        if (axis == 1) {
            return componentIsNegative != 0 ? 1 : -1;
        }
        return componentIsNegative != 0 ? -1 : 1;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test whether the projected segment-plane hit lies inside the polygon.
     */
    bool PointInProjectedPolygon(const zVec3* polygonVertices, int vertexCount, const zVec3* point, const zVec3* normal)
    {
        const int axis = DominantAxis(normal);
        const int windingSign = ProjectedWindingSign(normal, axis);

        {
            for (int edgeIndex = vertexCount - 1; edgeIndex >= 0; --edgeIndex) {
                const zVec3* edgeStart = &polygonVertices[edgeIndex];
                const zVec3* edgeEnd = &polygonVertices[(edgeIndex + 1) % vertexCount];
                const double edgeValue = (double)(windingSign)*ProjectedEdgeCross(edgeStart, edgeEnd, point, axis);
                if (edgeValue <= kPickEdgeInsideEpsilon) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build a pick candidate from one segment-plane intersection and projected polygon test.
     */
    bool BuildPickCandidateForSegmentVsPolygonCore(
        zClassDiPickCandidateEntry * candidate,
        const zVec3* segmentStart,
        const zVec3* segmentEnd,
        const zVec3* polygonVertices,
        int vertexCount,
        int cullBackface,
        int* outDominantAxis
    )
    {
        zMathVec3TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &candidate->surfaceNormal
        );

        const zVec3 endDelta = Delta3(segmentEnd, &polygonVertices[0]);
        const float endSide = Dot3(&endDelta, &candidate->surfaceNormal);
        if (cullBackface == 0 && endSide >= 0.0f) {
            return false;
        }

        const zVec3 startDelta = Delta3(segmentStart, &polygonVertices[0]);
        const float startSide = Dot3(&startDelta, &candidate->surfaceNormal);
        if (((FloatBits(startSide) ^ FloatBits(endSide)) & 0x80000000u) == 0) {
            return false;
        }

        const float t = startSide / (startSide - endSide);
        const zVec3 segmentDelta = Delta3(segmentEnd, segmentStart);
        candidate->hitPos.x = segmentStart->x + t * segmentDelta.x;
        candidate->hitPos.y = segmentStart->y + t * segmentDelta.y;
        candidate->hitPos.z = segmentStart->z + t * segmentDelta.z;

        const int dominantAxis = DominantAxis(&candidate->surfaceNormal);
        if (outDominantAxis != 0) {
            *outDominantAxis = dominantAxis;
        }

        return PointInProjectedPolygon(polygonVertices, vertexCount, &candidate->hitPos, &candidate->surfaceNormal);
    }

    /**
     * Original static helper observed in cls_di segment-batch polygon callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: compute a batched segment-plane hit point before polygon inclusion tests.
     */
    bool BuildBatchSegmentPlaneHit(
        zClassDiPickCandidateEntry * candidate,
        const CZDisplayInstanceSegmentEndpoints* segment,
        const zVec3* polygonVertices,
        const zVec3* normal,
        int cullBackface
    )
    {
        const zVec3 endDelta = Delta3(&segment->end, &polygonVertices[0]);
        const float endSide = Dot3(&endDelta, normal);
        if (cullBackface == 0 && endSide >= 0.0f) {
            return false;
        }

        const zVec3 startDelta = Delta3(&segment->start, &polygonVertices[0]);
        const float startSide = Dot3(&startDelta, normal);
        if (((FloatBits(startSide) ^ FloatBits(endSide)) & 0x80000000u) == 0) {
            return false;
        }

        const float t = startSide / (startSide - endSide);
        const zVec3 segmentDelta = Delta3(&segment->end, &segment->start);
        candidate->hitPos.x = segment->start.x + t * segmentDelta.x;
        candidate->hitPos.y = segment->start.y + t * segmentDelta.y;
        candidate->hitPos.z = segment->start.z + t * segmentDelta.z;
        return true;
    }

    /**
     * Original static helper observed in cls_di segment-batch polygon callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the current polygon hit to the player-probe candidate buffer.
     */
    void AppendBatchPolygonCandidate(
        CZNodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * buffer,
        const zVec3* normal,
        const zModel_PickFaceEntry* faceEntry
    )
    {
        if (buffer->candidateCount >= kMaxPickCandidates) {
            return;
        }

        zClassDiPickCandidateEntry* entry = &buffer->entries[buffer->candidateCount];
        entry->surfaceNormal = *normal;
        entry->node = candidateOwner;
        entry->scenePayload = faceEntry->scenePayload;
        ++buffer->candidateCount;
    }

    /**
     * Original static helper observed in cls_di damage-mask polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: solve hit UV coordinates in the dominant projected plane.
     */
    void SolvePickCandidateUvForProjectedPlane(
        const zClassDiPickCandidateEntry* candidate,
        const zVec3* polygonVertices,
        const zModel_PickFaceUvData* faceUvData,
        zVec2* outUv,
        int dominantAxis
    )
    {
        float uGrad0;
        float uGrad1;
        float vGrad0;
        float vGrad1;

        if (dominantAxis == 0) {
            zMathSolveLinearGradient2D(
                &uGrad0,
                &uGrad1,
                polygonVertices[0].y,
                polygonVertices[0].z,
                polygonVertices[1].y,
                polygonVertices[1].z,
                polygonVertices[2].y,
                polygonVertices[2].z,
                faceUvData->uvs[0].x,
                faceUvData->uvs[1].x,
                faceUvData->uvs[2].x
            );
            zMathSolveLinearGradient2D(
                &vGrad0,
                &vGrad1,
                polygonVertices[0].y,
                polygonVertices[0].z,
                polygonVertices[1].y,
                polygonVertices[1].z,
                polygonVertices[2].y,
                polygonVertices[2].z,
                faceUvData->uvs[0].y,
                faceUvData->uvs[1].y,
                faceUvData->uvs[2].y
            );

            outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad0
                + (candidate->hitPos.z - polygonVertices[0].z) * uGrad1 + faceUvData->uvs[0].x;
            outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad0
                + (candidate->hitPos.z - polygonVertices[0].z) * vGrad1 + faceUvData->uvs[0].y;
            return;
        }

        if (dominantAxis == 1) {
            zMathSolveLinearGradient2D(
                &uGrad0,
                &uGrad1,
                polygonVertices[0].x,
                polygonVertices[0].z,
                polygonVertices[1].x,
                polygonVertices[1].z,
                polygonVertices[2].x,
                polygonVertices[2].z,
                faceUvData->uvs[0].x,
                faceUvData->uvs[1].x,
                faceUvData->uvs[2].x
            );
            zMathSolveLinearGradient2D(
                &vGrad0,
                &vGrad1,
                polygonVertices[0].x,
                polygonVertices[0].z,
                polygonVertices[1].x,
                polygonVertices[1].z,
                polygonVertices[2].x,
                polygonVertices[2].z,
                faceUvData->uvs[0].y,
                faceUvData->uvs[1].y,
                faceUvData->uvs[2].y
            );

            outUv->x = (candidate->hitPos.z - polygonVertices[0].z) * uGrad1
                + (candidate->hitPos.x - polygonVertices[0].x) * uGrad0 + faceUvData->uvs[0].x;
            outUv->y = (candidate->hitPos.z - polygonVertices[0].z) * vGrad1
                + (candidate->hitPos.x - polygonVertices[0].x) * vGrad0 + faceUvData->uvs[0].y;
            return;
        }

        zMathSolveLinearGradient2D(
            &uGrad0,
            &uGrad1,
            polygonVertices[0].x,
            polygonVertices[0].y,
            polygonVertices[1].x,
            polygonVertices[1].y,
            polygonVertices[2].x,
            polygonVertices[2].y,
            faceUvData->uvs[0].x,
            faceUvData->uvs[1].x,
            faceUvData->uvs[2].x
        );
        zMathSolveLinearGradient2D(
            &vGrad0,
            &vGrad1,
            polygonVertices[0].x,
            polygonVertices[0].y,
            polygonVertices[1].x,
            polygonVertices[1].y,
            polygonVertices[2].x,
            polygonVertices[2].y,
            faceUvData->uvs[0].y,
            faceUvData->uvs[1].y,
            faceUvData->uvs[2].y
        );

        outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad1
            + (candidate->hitPos.x - polygonVertices[0].x) * uGrad0 + faceUvData->uvs[0].x;
        outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad1
            + (candidate->hitPos.x - polygonVertices[0].x) * vGrad0 + faceUvData->uvs[0].y;
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed transformed pick callers including 0x484e00,
     * 0x4857f0, and 0x485d10 through the active matrix slot access pattern.
     * Purpose: return the current model matrix used by cls_di point and normal
     * transforms.
     */
    const zMat4x3* CurrentMatrix()
    {
        return (const zMat4x3*)(*zMath::g_currentMatrixPtrSlot);
    }

    /**
     * Original static helper observed in cls_di transformed face-pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: transform a world-space query point into the active model matrix space.
     */
    zVec3 TransformWorldPointToModel(const zVec3* point)
    {
        const zMat4x3* matrix = CurrentMatrix();
        const float x = point->x - matrix->posX;
        const float y = point->y - matrix->posY;
        const float z = point->z - matrix->posZ;

        zVec3 result = { x * matrix->xx + y * matrix->xy + z * matrix->xz,
            x * matrix->yx + y * matrix->yy + z * matrix->yz,
            x * matrix->zx + y * matrix->zy + z * matrix->zz };
        return result;
    }

    /**
     * Original static helper observed in cls_di transformed face-pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: transform a model-space hit point into world space.
     */
    zVec3 TransformModelPointToWorld(const zVec3* point)
    {
        const zMat4x3* matrix = CurrentMatrix();

        zVec3 result = { point->x * matrix->xx + point->y * matrix->yx + point->z * matrix->zx + matrix->posX,
            point->x * matrix->xy + point->y * matrix->yy + point->z * matrix->zy + matrix->posY,
            point->x * matrix->xz + point->y * matrix->yz + point->z * matrix->zz + matrix->posZ };
        return result;
    }

    /**
     * Original static helper observed in cls_di transformed face-pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: rotate a model-space surface normal into world space.
     */
    zVec3 TransformModelVectorToWorld(const zVec3* vec)
    {
        const zMat4x3* matrix = CurrentMatrix();

        zVec3 result = { vec->x * matrix->xx + vec->y * matrix->yx + vec->z * matrix->zx,
            vec->x * matrix->xy + vec->y * matrix->yy + vec->z * matrix->zy,
            vec->x * matrix->xz + vec->y * matrix->yz + vec->z * matrix->zz };
        return result;
    }

    /**
     * Original static helper observed in cls_di mesh and polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: gather indexed face vertices into the four-entry DI scratch buffer.
     */
    void CopyFaceVerticesToScratch(const zVec3* vertices, const int* vertexIndices, unsigned int vertexCount)
    {
        for (unsigned int i = 0; i < vertexCount; ++i) {
            g_CZClass_DiFaceVertexScratch4[i] = vertices[vertexIndices[i]];
        }
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed segment and point pick callers including 0x445650,
     * 0x445b20, and 0x445c20 as the typed mesh face payload access.
     * Purpose: view the node DI payload as polygon/mesh pick face data.
     */
    zModel_PickFaceData* NodePickFaceData(CZNodePartial * node)
    {
        return (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
    }

    /**
     * Original static helper observed in cls_di pick traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the current node to the active pick-candidate cursor.
     */
    void AppendCurrentCandidateNode(CZNodePartial * node)
    {
        g_DiPickCandidateCursor->node = node;
        ++g_DiPickCandidateCursor;
        ++g_DiPickCandidateBuffer->candidateCount;
    }

    /**
     * Original static helper observed in cls_di pick traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test whether traversal should stop after the first accepted candidate.
     */
    bool BreakOnFirstCandidateHit()
    {
        return g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0;
    }

    /**
     * Original static helper observed in cls_di pick traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: convert the active candidate count into the original no-hit return value.
     */
    int NoCandidatesReturn()
    {
        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * Original static helper observed in cls_di segment bounds callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the smaller floating-point segment bound.
     */
    float MinFloat(float a, float b)
    {
        return a < b ? a : b;
    }

    /**
     * Original static helper observed in cls_di segment bounds callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the larger floating-point segment bound.
     */
    float MaxFloat(float a, float b)
    {
        return a > b ? a : b;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: expand a bounding box into the local eight-corner scratch order.
     */
    void CopyBBoxToCornersLocal(const zBBox3f* bbox, zBBoxCorners* outCorners)
    {
        const float minX = bbox->min.x;
        const float minY = bbox->min.y;
        const float minZ = bbox->min.z;
        const float maxX = bbox->max.x;
        const float maxY = bbox->max.y;
        const float maxZ = bbox->max.z;

        zVec3* vertices = outCorners->corners;
        vertices[0].x = minX;
        vertices[0].y = minY;
        vertices[0].z = maxZ;
        vertices[1].x = maxX;
        vertices[1].y = minY;
        vertices[1].z = maxZ;
        vertices[2].x = maxX;
        vertices[2].y = minY;
        vertices[2].z = minZ;
        vertices[3].x = minX;
        vertices[3].y = minY;
        vertices[3].z = minZ;
        vertices[4].x = minX;
        vertices[4].y = maxY;
        vertices[4].z = maxZ;
        vertices[5].x = maxX;
        vertices[5].y = maxY;
        vertices[5].z = maxZ;
        vertices[6].x = maxX;
        vertices[6].y = maxY;
        vertices[6].z = minZ;
        vertices[7].x = minX;
        vertices[7].y = maxY;
        vertices[7].z = minZ;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: apply the optional node-name prefix filter for region hits.
     */
    int FilterRegionNodeNameAllowed(CZNodePartial * node)
    {
        const char* prefix = g_CZDisplayInstance_FilterRegions_NodeNamePrefix;
        if (prefix == 0) {
            return 1;
        }

        return strncmp(node->name, prefix, strlen(prefix)) == 0 ? 1 : 0;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: compute squared clearance outside the active filter sphere.
     */
    float FilterRegionClearanceDistanceSq(const zVec3* boundsCenter, float boundsRadius)
    {
        if (g_CZDisplayInstance_FilterRegions_EnableClearanceCheck == 0) {
            return 0.0f;
        }

        float clearance = zMath::Vec3DeltaLength(g_CZDisplayInstance_FilterRegions_Center, boundsCenter) - boundsRadius;
        if (clearance < 0.0f) {
            return 0.0f;
        }

        return clearance * clearance;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: reject nodes whose bounds center is hidden by the active world raycast.
     */
    int FilterRegionLineOfSightBlocked(CZNodePartial * node, const zVec3* boundsCenter)
    {
        CZNodePartial* world = g_CZDisplayInstance_FilterRegions_LineOfSightWorld;
        if (world == 0 || (node->flags & kNodeFlagRequiresLineOfSight) == 0) {
            return 0;
        }

        PlayerProbeSampleCandidateBuffer rayData = { 0 };
        CZDisplayInstance::SetBreakOnFirstCandidate(1);
        CZDisplayInstance::SetStopAfterFirstHit(0x40000);
        CZClass::gwNodeSetRaycastable(node, 0);
        zVec3* center = g_CZDisplayInstance_FilterRegions_Center;
        const int result = CZDisplayInstance::RaycastFindClosest(
            world,
            center->x,
            center->y,
            center->z,
            boundsCenter->x,
            boundsCenter->y,
            boundsCenter->z,
            &rayData
        );
        CZClass::gwNodeSetRaycastable(node, 1);
        CZDisplayInstance::SetBreakOnFirstCandidate(0);

        return result == 0 && rayData.candidateCount != 0 ? 1 : 0;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append one filter-region hit entry to the active raycast hit list.
     */
    void AppendFilterRegionHit(CZNodePartial * node, const zVec3* hitPos, float distanceSq)
    {
        OptCatalogRaycastHitList* hitList = g_CZDisplayInstance_FilterRegions_OutHitList;
        OptCatalogRaycastHitEntry* entry = &hitList->hits[hitList->hitCount];
        entry->hitNode = node;
        entry->pos = *hitPos;
        entry->surfaceRef = 0;
        entry->distance = distanceSq;
        ++hitList->hitCount;
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the signed grid step direction for a ray delta.
     */
    int RayGridStep(float delta)
    {
        if (delta > 0.0f) {
            return 1;
        }
        if (delta < 0.0f) {
            return -1;
        }
        return 0;
    }

    /**
     * Original static helper observed in cls_di segment child traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the node when its pick-face data produces a segment hit.
     */
    void AppendNodeFaceCandidateIfHit(CZNodePartial * node)
    {
        zModel_PickFaceData* faceData = NodePickFaceData(node);
        if (faceData != 0
            && CZDisplayInstance::AppendPickCandidatesForFace(
                   faceData,
                   g_DiPickCandidateCursor,
                   &g_DiPickQueryPoint,
                   &g_DiSegmentEnd
               ) != 0) {
            AppendCurrentCandidateNode(node);
        }
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: shift the active ray packet into or out of a world-area cell.
     */
    void OffsetActiveRayPacket(float offsetX, float offsetZ)
    {
        g_DiPickQueryPoint.x += offsetX;
        g_DiPickQueryPoint.z += offsetZ;
        g_DiSegmentEnd.x += offsetX;
        g_DiSegmentEnd.z += offsetZ;
        g_DiSegmentMinX += offsetX;
        g_DiSegmentMaxX += offsetX;
        g_DiSegmentMinZ += offsetZ;
        g_DiSegmentMaxZ += offsetZ;
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: restore candidate hit positions from local cell space to world space.
     */
    void OffsetCandidatesFromCell(
        PlayerProbeSampleCandidateBuffer * rayData,
        int firstCandidate,
        float offsetX,
        float offsetZ
    )
    {
        for (int i = firstCandidate; i < rayData->candidateCount; ++i) {
            rayData->entries[i].hitPos.x -= offsetX;
            rayData->entries[i].hitPos.z -= offsetZ;
        }
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: scan one world-area cell for raycastable segment children.
     */
    void ProcessWorldAreaPickCell(zWorldAreaPartial * area, int nodeCountHint)
    {
        for (int i = 0; i < area->childCount; ++i) {
            CZNodePartial* node = area->childList[i];
            const int flags = node->flags;
            if ((flags & kNodeFlagEnabledForPick) != 0 && (flags & kNodeFlagRaycastable) != 0) {
                CZDisplayInstance::BuildPickCandidatesForSegmentChildFallback(node, nodeCountHint);
            }

            if (BreakOnFirstCandidateHit()) {
                break;
            }
        }
    }

    /**
     * Original static helper observed in cls_di segment child traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children with optional enabled/raycastable filtering.
     */
    void RecurseListBChildren(CZNodePartial * node, bool requireEnabledRaycastFlags)
    {
        {
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                CZNodePartial* child = node->listB[childIndex];
                if (!requireEnabledRaycastFlags
                    || ((child->flags & kNodeFlagEnabledForPick) != 0 && (child->flags & kNodeFlagRaycastable) != 0)) {
                    CZDisplayInstance::BuildPickCandidatesForSegmentChildFallback(child, node->listCountB);
                }

                if (BreakOnFirstCandidateHit()) {
                    break;
                }
            }
        }
    }

    /**
     * Original static helper observed in cls_di bbox pick/filter callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: compute min/max extents from the eight transformed bbox corners.
     */
    void ComputeBBoxExtents(
        const zBBoxCorners* corners,
        float* outMinX,
        float* outMaxX,
        float* outMinY,
        float* outMaxY,
        float* outMinZ,
        float* outMaxZ
    )
    {
        const zVec3* vertices = corners->corners;
        *outMinX = vertices[0].x;
        *outMaxX = vertices[0].x;
        *outMinY = vertices[0].y;
        *outMaxY = vertices[0].y;
        *outMinZ = vertices[0].z;
        *outMaxZ = vertices[0].z;

        for (int i = 1; i < 8; ++i) {
            const zVec3* corner = &vertices[i];
            if (corner->x < *outMinX) {
                *outMinX = corner->x;
            } else if (corner->x > *outMaxX) {
                *outMaxX = corner->x;
            }
            if (corner->y < *outMinY) {
                *outMinY = corner->y;
            } else if (corner->y > *outMaxY) {
                *outMaxY = corner->y;
            }
            if (corner->z < *outMinZ) {
                *outMinZ = corner->z;
            } else if (corner->z > *outMaxZ) {
                *outMaxZ = corner->z;
            }
        }
    }

    /**
     * Original static helper observed in cls_di bbox face-test callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: copy one bbox corner into the DI four-vertex scratch polygon.
     */
    void CopyBBoxCornerToScratch(const zBBoxCorners* bboxCorners, int sourceCorner, int scratchCorner)
    {
        const zVec3* src = &bboxCorners->corners[sourceCorner];
        zVec3* dst = &g_CZClass_DiFaceVertexScratch4[scratchCorner];
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }

    /**
     * Original static helper observed in cls_di single-segment bbox callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build and test one bbox face polygon against the active segment.
     */
    bool TestBBoxFace(
        zClassDiPickCandidateEntry * candidate,
        const zVec3* segmentStart,
        const zVec3* segmentEnd,
        int corner0,
        int corner1,
        int corner2,
        int corner3,
        const zBBoxCorners* bboxCorners
    )
    {
        CopyBBoxCornerToScratch(bboxCorners, corner0, 0);
        CopyBBoxCornerToScratch(bboxCorners, corner1, 1);
        CopyBBoxCornerToScratch(bboxCorners, corner2, 2);
        CopyBBoxCornerToScratch(bboxCorners, corner3, 3);
        return CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon(
                   candidate,
                   segmentStart,
                   segmentEnd,
                   g_CZClass_DiFaceVertexScratch4,
                   4,
                   0
               )
            != 0;
    }

    /**
     * Original static helper observed in cls_di segment-batch bbox callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build and test one bbox face polygon against the active segment batch.
     */
    int TestSegmentBatchBBoxFace(
        CZNodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        CZDisplayInstanceSegmentEndpoints * segmentEndpointsByBatch,
        int* activeMask,
        int segmentCount,
        const zBBoxCorners* bboxCorners,
        zModel_PickFaceEntry* faceEntry,
        int corner0,
        int corner1,
        int corner2,
        int corner3
    )
    {
        CopyBBoxCornerToScratch(bboxCorners, corner0, 0);
        CopyBBoxCornerToScratch(bboxCorners, corner1, 1);
        CopyBBoxCornerToScratch(bboxCorners, corner2, 2);
        CopyBBoxCornerToScratch(bboxCorners, corner3, 3);
        return CZDisplayInstance::BuildPickCandidatesForSegmentBatchVsPolygon(
            candidateOwner,
            outCandidateBuffersBySegment,
            segmentEndpointsByBatch,
            activeMask,
            segmentCount,
            g_CZClass_DiFaceVertexScratch4,
            faceEntry
        );
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed segment-batch callers including 0x4476f0 and
     * 0x486290 as the packed segment endpoint view of the active point array.
     * Purpose: reinterpret the active pick point array as segment endpoint
     * pairs for batched segment traversal.
     */
    CZDisplayInstanceSegmentEndpoints* SegmentEndpointBatchFromPickPointArray()
    {
        return (CZDisplayInstanceSegmentEndpoints*)((void*)(g_DiPickPointArray));
    }

    /**
     * Original static helper observed in cls_di segment-bbox callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: reject segment bounds that do not overlap a candidate box.
     */
    bool SegmentBoundsOverlapBox(
        const CZDisplayInstanceSegmentBounds* bounds,
        float minX,
        float maxX,
        float minY,
        float maxY,
        float minZ,
        float maxZ
    )
    {
        return bounds->maxX > minX && bounds->minX < maxX && bounds->maxY > minY && bounds->minY < maxY
            && bounds->maxZ > minZ && bounds->minZ < maxZ;
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: copy the per-segment active mask for recursive filtering.
     */
    void CopySegmentActiveMask(int* dst, const int* src)
    {
        memcpy(dst, src, (size_t)(g_DiPickPointCount) * sizeof(int));
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: convert a world coordinate to a grid cell coordinate.
     */
    int GridCoordFromWorld(float value, float origin, float invCellSize)
    {
        return (int)(floor((value - origin) * invCellSize));
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: clamp a grid coordinate into the world-area grid range.
     */
    int ClampGridCoord(int coord, int count)
    {
        if (coord < 0) {
            return 0;
        }
        if (coord >= count) {
            return count - 1;
        }
        return coord;
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: shift all active segment endpoints and bounds in the XZ plane.
     */
    void OffsetSegmentBatchXZ(float offsetX, float offsetZ)
    {
        CZDisplayInstanceSegmentEndpoints* segments = SegmentEndpointBatchFromPickPointArray();
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            segments[i].start.x += offsetX;
            segments[i].start.z += offsetZ;
            segments[i].end.x += offsetX;
            segments[i].end.z += offsetZ;

            g_DiSegmentBounds[i].minX += offsetX;
            g_DiSegmentBounds[i].maxX += offsetX;
            g_DiSegmentBounds[i].minZ += offsetZ;
            g_DiSegmentBounds[i].maxZ += offsetZ;
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: snapshot candidate counts before processing a clamped grid cell.
     */
    void SaveSegmentCandidateCounts(int* candidateCounts)
    {
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            candidateCounts[i] = g_DiPickCandidateBuffer[i].candidateCount;
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: restore new candidate positions from clamped cell space to world space.
     */
    void RestoreClampedSegmentCandidatePositions(const int* firstNewCandidate, float offsetX, float offsetZ)
    {
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            PlayerProbeSampleCandidateBuffer* buffer = &g_DiPickCandidateBuffer[i];
            for (int j = firstNewCandidate[i]; j < buffer->candidateCount; ++j) {
                buffer->entries[j].hitPos.x -= offsetX;
                buffer->entries[j].hitPos.z -= offsetZ;
            }
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build axis-aligned segment bounds from start and end points.
     */
    void BuildSegmentBoundsFromEndpoints(
        const CZDisplayInstanceSegmentEndpoints* segments,
        CZDisplayInstanceSegmentBounds* bounds
    )
    {
        bounds->minX = segments->start.x < segments->end.x ? segments->start.x : segments->end.x;
        bounds->maxX = segments->start.x < segments->end.x ? segments->end.x : segments->start.x;
        bounds->minY = segments->start.y < segments->end.y ? segments->start.y : segments->end.y;
        bounds->maxY = segments->start.y < segments->end.y ? segments->end.y : segments->start.y;
        bounds->minZ = segments->start.z < segments->end.z ? segments->start.z : segments->end.z;
        bounds->maxZ = segments->start.z < segments->end.z ? segments->end.z : segments->start.z;
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test whether segment bounds overlap the world grid in XZ.
     */
    bool SegmentBoundsOverlapWorldXZ(const CZDisplayInstanceSegmentBounds* bounds, const CZWorldDataPartial* worldData)
    {
        return bounds->minX < worldData->worldMaxX && bounds->maxX >= worldData->originX
            && bounds->minZ <= worldData->originZ && bounds->maxZ > worldData->worldMaxZ;
    }

    zDiPartial* NodeDiRef(CZNodePartial * node);

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: filter the current node's pick faces against the active segment batch.
     */
    void FilterCurrentSegmentRegions(CZNodePartial * node, int* activeMask)
    {
        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0) {
            CZDisplayInstance::FilterRegionsAgainstPolygon(
                node,
                faceData,
                SegmentEndpointBatchFromPickPointArray(),
                activeMask,
                g_DiPickPointCount,
                g_DiPickCandidateBuffer
            );
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children for the active segment batch.
     */
    void RecurseSegmentBatchChildren(CZNodePartial * node, int* activeMask, bool requirePickFlags)
    {
        for (int i = 0; i < node->listCountB; ++i) {
            CZNodePartial* child = node->listB[i];
            if (!requirePickFlags
                || ((child->flags & kNodeFlagEnabledForPick) != 0 && (child->flags & kNodeFlagRaycastable) != 0)) {
                CZDisplayInstance::BuildPickCandidatesForSegmentsRecursive(child, node->listCountB, activeMask);
            }

            if (BreakOnFirstCandidateHit()) {
                break;
            }
        }
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed cls_di point and segment traversal callers including
     * 0x443f80, 0x444890, 0x444c50, 0x444d10, and 0x446440.
     * Purpose: view the node payload pointer as a zDi record for point and
     * segment pick tests.
     */
    zDiPartial* NodeDiRef(CZNodePartial * node)
    {
        return (zDiPartial*)((unsigned int)(node->userDataOrDiRef));
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: apply the optional variant id gate for point-query nodes.
     */
    bool NodePassesQueryVariant(CZNodePartial * node)
    {
        return (node->flags & 0x01000000) == 0 || VariantTag::CurrentAllowsId(node->nodeType) != 0;
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test the node flags required for point-query candidates.
     */
    bool NodePassesQueryFlags(CZNodePartial * node)
    {
        return (node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & 0x08) != 0;
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the node when its DI payload accepts the active query point.
     */
    void AppendQueryPointCandidateIfHit(CZNodePartial * node)
    {
        zDiPartial* di = NodeDiRef(node);
        if (di == 0) {
            return;
        }

        PlayerProbeSampleCandidateBuffer* buffer = g_DiPickCandidateBuffer;
        zClassDiPickCandidateEntry* outCandidate = &buffer->entries[buffer->candidateCount];
        if (zDi::BuildPickCandidateForQueryPoint(di, outCandidate, &g_DiPickQueryPoint) != 0) {
            AppendCurrentCandidateNode(node);
        }
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children for the active single point query.
     */
    void RecurseQueryPointChildren(CZNodePartial * node, int cullCount, bool requireQueryFlags)
    {
        for (int i = 0; i < node->listCountB; ++i) {
            CZNodePartial* child = node->listB[i];
            if (!requireQueryFlags || NodePassesQueryFlags(child)) {
                CZDisplayInstance::BuildPickCandidateList(child, cullCount);
            }
        }
    }

    /**
     * Original static helper observed in cls_di point-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children for the active point batch query.
     */
    void RecursePointBatchChildren(CZNodePartial * node, int depth, int* hitFlags, bool requireQueryFlags)
    {
        for (int i = 0; i < node->listCountB; ++i) {
            CZNodePartial* child = node->listB[i];
            if (!requireQueryFlags || NodePassesQueryFlags(child)) {
                CZDisplayInstance::BuildPickCandidatesForPoints(child, depth, hitFlags);
            }
        }
    }

    /**
     * Original static helper observed in cls_di transformed mesh and polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: copy or transform model vertices into the shared world-space scratch buffer.
     */
    void TransformVerticesToSharedScratch(const zVec3* vertices, int vertexCount)
    {
        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            memcpy(g_zModel_SharedVec3ScratchB, vertices, (size_t)(vertexCount) * sizeof(zVec3));
            return;
        }

        for (int i = 0; i < vertexCount; ++i) {
            g_zModel_SharedVec3ScratchB[i] = TransformModelPointToWorld(&vertices[i]);
        }
    }
}

namespace CZDisplayInstance
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.setbreakonfirstcandidate
     * @recoil-artifact defines .text recoil:function:0x443c50: CZDisplayInstance::SetBreakOnFirstCandidate.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    void __fastcall SetBreakOnFirstCandidate(int enabled)
    {
        g_cls_di_BreakOnFirstCandidate = enabled;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.setstopafterfirsthit
     * @recoil-artifact defines .text recoil:function:0x443c60: CZDisplayInstance::SetStopAfterFirstHit.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    void __fastcall SetStopAfterFirstHit(int flag)
    {
        g_cls_di_StopAfterFirstHit = flag;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.findbestpickcandidatebelowpoint
     * @recoil-artifact defines .text recoil:function:0x443c70: CZDisplayInstance::FindBestPickCandidateBelowPoint.
     *
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FindBestPickCandidateBelowPoint(
        CZNodePartial * world,
        const zVec3* position,
        PlayerProbeSampleCandidateBuffer* outResults
    )
    {
        if (BuildPickCandidateListBelowPoint(world, position->x, position->y, position->z, outResults) == 0) {
            zClassDiPickCandidateEntry* candidate = &outResults->entries[0];
            zClassDiPickCandidateEntry* best = candidate;
            while (--outResults->candidateCount != 0) {
                ++candidate;
                if (candidate->hitPos.y > position->y) {
                    continue;
                }

                if (best->hitPos.y > position->y || candidate->hitPos.y > best->hitPos.y
                    || (candidate->hitPos.y == best->hitPos.y && best->variantTag.count == 0)) {
                    best = candidate;
                }
            }

            outResults->entries[0] = *best;
            outResults->candidateCount = 1;
            return 0;
        }

        outResults->candidateCount = 0;
        zTag4::Clear(&outResults->entries[0].variantTag);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatelistbelowpoint
     * @recoil-artifact defines .text recoil:function:0x443d20: CZDisplayInstance::BuildPickCandidateListBelowPoint.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidateListBelowPoint(
        CZNodePartial * world,
        float x,
        float maxY,
        float z,
        PlayerProbeSampleCandidateBuffer* outResults
    )
    {
        if (g_CZTypeList_Buckets[7].head != 0) {
            CZTypeList::UpdateQueuedTrees();
        }

        g_DiPickQueryPoint.x = x;
        g_DiPickQueryPoint.y = maxY;
        g_DiPickQueryPoint.z = z;
        g_DiPickCandidateBuffer = outResults;
        g_DiPickCandidateCursor = outResults->entries;
        outResults->candidateCount = 0;

        CZWorldDataPartial* worldData = (CZWorldDataPartial*)(world->classData);

        zMat4x3 slotBuffer;
        zMath::MatStackPushPtr((float*)(&slotBuffer));
        zMath::MatLoadIdentity();

        int visitGridCell = 1;
        const int gridCol = (int)(floor((x - worldData->originX) * worldData->areaInvSizeX));
        const int gridRow = (int)(floor((z - worldData->originZ) * worldData->areaInvSizeZ));

        int usedClampedCell;
        int cellCol;
        int cellRow;
        float offsetX;
        float offsetZ;
        if (gridCol >= 0 && gridCol < worldData->areaGridColCount && gridRow >= 0
            && gridRow < worldData->areaGridRowCount) {
            usedClampedCell = 0;
        } else if (worldData->clampQueriesToBounds == 0) {
            visitGridCell = 0;
        } else {
            usedClampedCell = 1;
            cellCol = gridCol;
            cellRow = gridRow;
            if (gridCol > worldData->areaGridColCount - 1) {
                cellCol = worldData->areaGridColCount - 1;
            } else if (gridCol < 0) {
                cellCol = 0;
            }

            if (gridRow > worldData->areaGridRowCount - 1) {
                cellRow = worldData->areaGridRowCount - 1;
            } else if (gridRow < 0) {
                cellRow = 0;
            }

            offsetX = (float)(cellCol - gridCol) * worldData->areaCellSizeX;
            offsetZ = (float)(cellRow - gridRow) * worldData->areaCellSizeZ;
        }

        if (visitGridCell != 0) {
            zWorldAreaPartial* area;
            if (usedClampedCell != 0) {
                g_DiPickQueryPoint.x += offsetX;
                g_DiPickQueryPoint.z += offsetZ;
                area = &worldData->areaGridRows[cellRow][cellCol];
            } else {
                area = &worldData->areaGridRows[gridRow][gridCol];
            }

            for (int i = 0; i < area->childCount; ++i) {
                CZNodePartial* node = area->childList[i];
                if ((node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & 0x08) != 0
                    && ((node->flags & 0x01000000) == 0 || VariantTag::CurrentAllowsId(node->nodeType) != 0)) {
                    BuildPickCandidateList(node, area->childCount + 1);
                }
            }

            if (usedClampedCell != 0) {
                g_DiPickQueryPoint.x -= offsetX;
                g_DiPickQueryPoint.z -= offsetZ;
            }
        }

        if (world->listCountB > 0) {
            for (int i = 0; i < world->listCountB; ++i) {
                CZNodePartial* node = world->listB[i];
                if ((node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & 0x08) != 0
                    && ((node->flags & 0x01000000) == 0 || VariantTag::CurrentAllowsId(node->nodeType) != 0)) {
                    BuildPickCandidateList(node, world->listCountB + 1);
                }
            }
        }

        zMath::MatStackPopPtr();
        return outResults->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatelist
     * @recoil-artifact defines .text recoil:function:0x443f80: CZDisplayInstance::BuildPickCandidateList.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidateList(CZNodePartial * node, int cullCount)
    {
        if ((node->flags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((node->flags & 0x08) == 0) {
            return 1;
        }
        if ((node->flags & 0x01000000) != 0 && VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        node->flags &= ~kNodeFlagClearDuringPick;
        const int nodeFlags = node->flags;
        if (g_DiPickCandidateBuffer->candidateCount >= kMaxPickCandidates) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0x26b,
                "Database intersections array is full"
            );
            return 1;
        }

        int result; // Unset for sound nodes (retail returns the stale slot).
        switch (node->classId) {
        case kNodeClassObject3D: {
            if (cullCount > 1) {
                const int outside = IsPickQueryPointOutsideViewBBoxXZ(node);
                if (outside != 0) {
                    return outside;
                }
            }

            CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(node->classData);
            int pushedMatrix;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) != 0) {
                    if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                        zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                        zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 1);
                        if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                            objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                        }
                    } else {
                        zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                    }
                } else {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 1);
                }
            } else {
                pushedMatrix = 0;
            }

            zDiPartial* di = (zDiPartial*)((unsigned int)(node->userDataOrDiRef));
            if (di != 0) {
                PlayerProbeSampleCandidateBuffer* buffer = g_DiPickCandidateBuffer;
                zClassDiPickCandidateEntry* outCandidate = &buffer->entries[buffer->candidateCount];
                if (zDi::BuildPickCandidateForQueryPoint(di, outCandidate, &g_DiPickQueryPoint) != 0) {
                    g_DiPickCandidateCursor->node = node;
                    ++g_DiPickCandidateCursor;
                    ++g_DiPickCandidateBuffer->candidateCount;
                }
            }

            if (node->listCountB > 0) {
                for (int i = 0; i < node->listCountB; ++i) {
                    CZNodePartial* child = node->listB[i];
                    if ((child->flags & kNodeFlagEnabledForPick) != 0 && (child->flags & 0x08) != 0) {
                        BuildPickCandidateList(child, node->listCountB);
                    }
                }
            }

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }

            return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
        }

        case kNodeClassLod: {
            CZLodDataPartial* lodData = (CZLodDataPartial*)(node->classData);
            if (lodData->nearRangeSq > 5.0) {
                return 1;
            }

            if (cullCount > 1) {
                const int outside = IsPickQueryPointOutsideViewBBoxXZ(node);
                if (outside != 0) {
                    return outside;
                }
            }

            for (int i = 0; i < node->listCountB; ++i) {
                BuildPickCandidateList(node->listB[i], node->listCountB);
            }
            return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
        }

        case kNodeClassCamera: {
            zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
            CZCameraDataPartial* cameraData = (CZCameraDataPartial*)(node->classData);

            int pushedMatrix;
            if ((nodeFlags & kNodeFlagEnabledForPick) != 0) {
                pushedMatrix = 1;
                zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
                // Retail 0x4441c1 passes camera data +0x20 in ECX and +0x14 in EDX.
                zMath::MatApplyLocalTRS(&cameraData->localRotation, &cameraData->localPosition, &unitScale);
            } else {
                pushedMatrix = 0;
            }

            zDiPartial* di = (zDiPartial*)((unsigned int)(node->userDataOrDiRef));
            if (di != 0) {
                PlayerProbeSampleCandidateBuffer* buffer = g_DiPickCandidateBuffer;
                zClassDiPickCandidateEntry* outCandidate = &buffer->entries[buffer->candidateCount];
                if (zDi::BuildPickCandidateForQueryPoint(di, outCandidate, &g_DiPickQueryPoint) != 0) {
                    g_DiPickCandidateCursor->node = node;
                    ++g_DiPickCandidateCursor;
                    ++g_DiPickCandidateBuffer->candidateCount;
                }
            }

            if (node->listCountB > 0) {
                for (int i = 0; i < node->listCountB; ++i) {
                    CZNodePartial* child = node->listB[i];
                    if ((child->flags & kNodeFlagEnabledForPick) != 0 && (child->flags & 0x08) != 0) {
                        BuildPickCandidateList(child, node->listCountB);
                    }
                }
            }

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }

            return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
        }

        case kNodeClassSequence: {
            CZSequenceDataPartial* sequenceData = (CZSequenceDataPartial*)(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            if (cullCount > 1) {
                const int outside = IsPickQueryPointOutsideViewBBoxXZ(node);
                if (outside != 0) {
                    return outside;
                }
            }

            const int childResult
                = BuildPickCandidateList(sequenceData->entries[sequenceData->currentIndex].node, node->listCountB);
            return childResult;
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesRecursive(node, cullCount);

        case kNodeClassLight:
            return BuildPickCandidatesForLight(node, cullCount);

        case kNodeClassSound:
            break;

        default:
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0x295,
                "Unrecognized node class type:  node = %s class_type = %d",
                node,
                node->classId
            );
            result = 3;
            break;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesrecursive
     * @recoil-artifact defines .text recoil:function:0x444310: CZDisplayInstance::BuildPickCandidatesRecursive.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesRecursive(CZNodePartial * node, int cullCount)
    {
        zDiPartial* di = (zDiPartial*)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            if (zDi::BuildPickCandidateForQueryPoint(
                    di,
                    &g_DiPickCandidateBuffer->entries[g_DiPickCandidateBuffer->candidateCount],
                    &g_DiPickQueryPoint
                )
                != 0) {
                g_DiPickCandidateCursor->node = node;
                ++g_DiPickCandidateCursor;
                ++g_DiPickCandidateBuffer->candidateCount;
            }
        }

        CZAnimateDataPartial* animateData = (CZAnimateDataPartial*)(node->classData);
        int pushedMatrix;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3*)(animateData->animatedTransform), 1);
        } else {
            pushedMatrix = 0;
        }

        if (cullCount > 1) {
            const int result = IsPickQueryPointOutsideViewBBoxXZ(node);
            if (result != 0) {
                if (pushedMatrix != 0) {
                    zMath::MatStackPopPtr();
                }
                return result;
            }
        }

        if (node->listCountB > 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                BuildPickCandidateList(node->listB[i], node->listCountB);
            }
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforlight
     * @recoil-artifact defines .text recoil:function:0x4443e0: CZDisplayInstance::BuildPickCandidatesForLight.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForLight(CZNodePartial * node, int cullCount)
    {
        if (cullCount > 1) {
            const int result = IsPickQueryPointOutsideViewBBoxXZ(node);
            if (result != 0) {
                return result;
            }
        }

        CZLightDataPartial* lightData = (CZLightDataPartial*)(node->classData);
        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y, lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        zDiPartial* di = (zDiPartial*)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            if (zDi::BuildPickCandidateForQueryPoint(
                    di,
                    &g_DiPickCandidateBuffer->entries[g_DiPickCandidateBuffer->candidateCount],
                    &g_DiPickQueryPoint
                )
                != 0) {
                g_DiPickCandidateCursor->node = node;
                ++g_DiPickCandidateCursor;
                ++g_DiPickCandidateBuffer->candidateCount;
            }
        }

        if (node->listCountB > 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                BuildPickCandidateList(node->listB[i], node->listCountB);
            }
        }

        zMath::MatStackPopPtr();
        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforpointbatch
     * @recoil-artifact defines .text recoil:function:0x4444b0: CZDisplayInstance::BuildPickCandidatesForPointBatch.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForPointBatch(
        CZNodePartial * world,
        zVec3 * pointArray,
        int pointCount,
        float queryMaxY,
        PlayerProbeSampleCandidateBuffer* outCandidateBuffersByPoint
    )
    {
        if (pointCount > 24) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0x495,
                "More test pnts than space for: %d",
                pointCount
            );
            pointCount = 24;
        }

        if (g_CZTypeList_Buckets[7].head != 0) {
            CZTypeList::UpdateQueuedTrees();
        }

        CZWorldDataPartial* worldData = (CZWorldDataPartial*)(world->classData);
        int pointActive[24];
        zWorldAreaPartial* gridCellForPoint[24];
        zWorldAreaPartial* uniqueGridCells[24];
        int pointWasClamped[24];
        float clampOffsetXZ[24][2];

        zVec3* point = pointArray;
        for (int i = 0; i < pointCount; ++i, ++point) {
            pointActive[i] = 1;
            pointWasClamped[i] = 0;
            outCandidateBuffersByPoint[i].candidateCount = 0;

            const int gridCol = (int)(floor((point->x - worldData->originX) * worldData->areaInvSizeX));
            const int gridRow = (int)(floor((point->z - worldData->originZ) * worldData->areaInvSizeZ));
            if (gridCol >= 0 && gridCol < worldData->areaGridColCount && gridRow >= 0
                && gridRow < worldData->areaGridRowCount) {
                gridCellForPoint[i] = &worldData->areaGridRows[gridRow][gridCol];
                continue;
            }

            if (worldData->clampQueriesToBounds == 0) {
                pointActive[i] = 0;
                pointWasClamped[i] = 0;
                continue;
            }

            int clampedCol = gridCol;
            int clampedRow = gridRow;
            if (gridCol > worldData->areaGridColCount - 1) {
                clampedCol = worldData->areaGridColCount - 1;
            } else if (gridCol < 0) {
                clampedCol = 0;
            }

            if (gridRow > worldData->areaGridRowCount - 1) {
                clampedRow = worldData->areaGridRowCount - 1;
            } else if (gridRow < 0) {
                clampedRow = 0;
            }

            pointWasClamped[i] = 1;
            gridCellForPoint[i] = &worldData->areaGridRows[clampedRow][clampedCol];
            clampOffsetXZ[i][0] = (float)(clampedCol - gridCol) * worldData->areaCellSizeX;
            clampOffsetXZ[i][1] = (float)(clampedRow - gridRow) * worldData->areaCellSizeZ;
            point->x += clampOffsetXZ[i][0];
            point->z += clampOffsetXZ[i][1];
        }

        int uniqueGridCellCount = 0;
        for (int uniquePointIndex = 0; uniquePointIndex < pointCount; ++uniquePointIndex) {
            if (pointActive[uniquePointIndex] != 0) {
                int isNewCell = 1;
                for (int j = 0; j < uniqueGridCellCount; ++j) {
                    if (gridCellForPoint[uniquePointIndex] == uniqueGridCells[j]) {
                        isNewCell = 0;
                    }
                }

                if (isNewCell != 0) {
                    uniqueGridCells[uniqueGridCellCount++] = gridCellForPoint[uniquePointIndex];
                }
            }
        }

        zMat4x3 slotBuffer;
        zMath::MatStackPushPtr((float*)(&slotBuffer));
        zMath::MatLoadIdentity();

        g_DiPickCandidateBuffer = outCandidateBuffersByPoint;
        g_DiPickCandidateCursor = outCandidateBuffersByPoint->entries;
        g_DiPickPointQueryMaxY = queryMaxY;
        g_DiPickPointArray = pointArray;
        g_DiPickPointCount = pointCount;

        for (int cellIndex = 0; cellIndex < uniqueGridCellCount; ++cellIndex) {
            zWorldAreaPartial* cell = uniqueGridCells[cellIndex];
            int hitFlags[24];
            for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                if (pointActive[pointIndex] != 0 && gridCellForPoint[pointIndex] == cell) {
                    hitFlags[pointIndex] = 1;
                } else {
                    hitFlags[pointIndex] = 0;
                }
            }

            for (int childIndex = 0; childIndex < cell->childCount; ++childIndex) {
                CZNodePartial* node = cell->childList[childIndex];
                if ((node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & 0x08) != 0) {
                    BuildPickCandidatesForPoints(node, cell->childCount + 1, hitFlags);
                }
            }
        }

        for (int restoreIndex = 0; restoreIndex < pointCount; ++restoreIndex) {
            if (pointWasClamped[restoreIndex] != 0) {
                pointArray[restoreIndex].x -= clampOffsetXZ[restoreIndex][0];
                pointArray[restoreIndex].z -= clampOffsetXZ[restoreIndex][1];
            }
        }

        if (world->listCountB > 0) {
            for (int worldNodeIndex = 0; worldNodeIndex < world->listCountB; ++worldNodeIndex) {
                CZNodePartial* node = world->listB[worldNodeIndex];
                const int nodeFlags = node->flags;
                if ((nodeFlags & kNodeFlagEnabledForPick) != 0 && (nodeFlags & 0x08) != 0
                    && ((nodeFlags & 0x01000000) == 0 || VariantTag::CurrentAllowsId(node->nodeType) != 0)) {
                    BuildPickCandidatesForPoints(node, world->listCountB + 1, pointActive);
                }
            }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforpoints
     * @recoil-artifact defines .text recoil:function:0x444890: CZDisplayInstance::BuildPickCandidatesForPoints.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForPoints(CZNodePartial * node, int depth, int* hitFlags)
    {
        if ((node->flags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((node->flags & 0x08) == 0) {
            return 1;
        }
        if ((node->flags & 0x01000000) != 0 && VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        const int classId = node->classId;
        node->flags &= ~kNodeFlagClearDuringPick;
        const int nodeFlags = node->flags;

        int sampleMask[24];
        int result; // Unset for sound nodes (retail returns the stale slot).
        switch (classId) {
        case kNodeClassObject3D: {
            memcpy(sampleMask, hitFlags, (size_t)(g_DiPickPointCount) * sizeof(int));
            if (depth > 1) {
                result = PickTestBBox2D(node, sampleMask);
                if (result != 0) {
                    break;
                }
            }

            CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(node->classData);
            int pushedMatrix;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) != 0) {
                    if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                        zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                        zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 1);
                        if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                            objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                        }
                    } else {
                        zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                    }
                } else {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 1);
                }
            } else {
                pushedMatrix = 0;
            }

            zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
            if (faceData != 0) {
                PickTestMeshAtQueryXZ(
                    node,
                    faceData,
                    g_DiPickPointArray,
                    sampleMask,
                    g_DiPickPointCount,
                    g_DiPickPointQueryMaxY,
                    g_DiPickCandidateBuffer
                );
            }

            for (int i = 0; i < node->listCountB; ++i) {
                CZNodePartial* child = node->listB[i];
                const int childFlags = child->flags;
                if ((childFlags & kNodeFlagEnabledForPick) != 0 && (childFlags & 0x08) != 0) {
                    BuildPickCandidatesForPoints(child, node->listCountB, sampleMask);
                }
            }

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassCamera: {
            memcpy(sampleMask, hitFlags, (size_t)(g_DiPickPointCount) * sizeof(int));

            zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
            CZCameraDataPartial* cameraData = (CZCameraDataPartial*)(node->classData);
            int pushedMatrix;
            if ((nodeFlags & kNodeFlagEnabledForPick) != 0) {
                pushedMatrix = 1;
                zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
                // Retail 0x444a52 uses the same camera transform as rendering.
                zMath::MatApplyLocalTRS(&cameraData->localRotation, &cameraData->localPosition, &unitScale);
            } else {
                pushedMatrix = 0;
            }

            zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
            if (faceData != 0) {
                PickTestMeshAtQueryXZ(
                    node,
                    faceData,
                    g_DiPickPointArray,
                    sampleMask,
                    g_DiPickPointCount,
                    g_DiPickPointQueryMaxY,
                    g_DiPickCandidateBuffer
                );
            }

            for (int i = 0; i < node->listCountB; ++i) {
                CZNodePartial* child = node->listB[i];
                const int childFlags = child->flags;
                if ((childFlags & kNodeFlagEnabledForPick) != 0 && (childFlags & 0x08) != 0) {
                    BuildPickCandidatesForPoints(child, node->listCountB, sampleMask);
                }
            }

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassLod: {
            CZLodDataPartial* lodData = (CZLodDataPartial*)(node->classData);
            if (lodData->nearRangeSq > 5.0) {
                return 1;
            }

            memcpy(sampleMask, hitFlags, (size_t)(g_DiPickPointCount) * sizeof(int));
            if (depth > 1) {
                result = PickTestBBox2D(node, sampleMask);
                if (result != 0) {
                    break;
                }
            }

            for (int i = 0; i < node->listCountB; ++i) {
                BuildPickCandidatesForPoints(node->listB[i], node->listCountB, sampleMask);
            }
            return 0;
        }

        case kNodeClassSequence: {
            CZSequenceDataPartial* sequenceData = (CZSequenceDataPartial*)(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            memcpy(sampleMask, hitFlags, (size_t)(g_DiPickPointCount) * sizeof(int));
            if (depth > 1) {
                result = PickTestBBox2D(node, sampleMask);
                if (result != 0) {
                    break;
                }
            }

            return BuildPickCandidatesForPoints(
                sequenceData->entries[sequenceData->currentIndex].node,
                node->listCountB,
                sampleMask
            );
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesForPointsRecursive(node, depth, hitFlags);

        case kNodeClassLight:
            return BuildPickCandidatesForPointsForLight(node, depth, hitFlags);

        case kNodeClassSound:
            break;

        default:
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0x587,
                "Unrecognized node class type:  node = %s class_type = %d",
                node,
                classId
            );
            result = 3;
            break;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforpointsrecursive
     * @recoil-artifact defines .text recoil:function:0x444c50: CZDisplayInstance::BuildPickCandidatesForPointsRecursive.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForPointsRecursive(CZNodePartial * node, int depth, int* hitFlags)
    {
        int sampleMask[24];
        memcpy(sampleMask, hitFlags, (size_t)(g_DiPickPointCount) * sizeof(int));

        if (depth > 1) {
            const int bboxResult = PickTestBBox2D(node, sampleMask);
            if (bboxResult != 0) {
                return bboxResult;
            }
        }

        CZAnimateDataPartial* animateData = (CZAnimateDataPartial*)(node->classData);
        int pushedMatrix;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3*)(animateData->animatedTransform), 1);
        } else {
            pushedMatrix = 0;
        }

        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0) {
            PickTestMeshAtQueryXZ(
                node,
                faceData,
                g_DiPickPointArray,
                sampleMask,
                g_DiPickPointCount,
                g_DiPickPointQueryMaxY,
                g_DiPickCandidateBuffer
            );
        }

        for (int i = 0; i < node->listCountB; ++i) {
            BuildPickCandidatesForPoints(node->listB[i], node->listCountB, sampleMask);
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforpointsforlight
     * @recoil-artifact defines .text recoil:function:0x444d10: CZDisplayInstance::BuildPickCandidatesForPointsForLight.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForPointsForLight(CZNodePartial * node, int depth, int* hitFlags)
    {
        int sampleMask[24];
        memcpy(sampleMask, hitFlags, (size_t)(g_DiPickPointCount) * sizeof(int));

        if (depth > 1) {
            const int bboxResult = PickTestBBox2D(node, sampleMask);
            if (bboxResult != 0) {
                return bboxResult;
            }
        }

        CZLightDataPartial* lightData = (CZLightDataPartial*)(node->classData);
        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y, lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0) {
            PickTestMeshAtQueryXZ(
                node,
                faceData,
                g_DiPickPointArray,
                sampleMask,
                g_DiPickPointCount,
                g_DiPickPointQueryMaxY,
                g_DiPickCandidateBuffer
            );
        }

        for (int i = 0; i < node->listCountB; ++i) {
            BuildPickCandidatesForPoints(node->listB[i], node->listCountB, sampleMask);
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.raycastselectclosesthitbetweenpoints
     * @recoil-artifact defines .text recoil:function:0x444de0: CZDisplayInstance::RaycastSelectClosestHitBetweenPoints.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall RaycastSelectClosestHitBetweenPoints(
        CZNodePartial * world,
        const zVec3* startPoint,
        const zVec3* endPoint,
        PlayerProbeSampleCandidateBuffer* rayData
    )
    {
        if (RaycastFindClosest(
                world,
                startPoint->x,
                startPoint->y,
                startPoint->z,
                endPoint->x,
                endPoint->y,
                endPoint->z,
                rayData
            )
            != 0) {
            return 1;
        }

        if (rayData->candidateCount > 1) {
            const zClassDiPickCandidateEntry* candidate = &rayData->entries[0];
            float closestDistance = zMath::Vec3DeltaLengthSq(startPoint, &candidate->hitPos);
            int bestCandidateIndex = 0;
            int candidateIndex = 0;

            --rayData->candidateCount;
            do {
                ++candidate;
                ++candidateIndex;

                const float candidateDistance = zMath::Vec3DeltaLengthSq(startPoint, &candidate->hitPos);
                if (candidateDistance < closestDistance) {
                    closestDistance = candidateDistance;
                    bestCandidateIndex = candidateIndex;
                }
            } while (--rayData->candidateCount != 0);

            rayData->candidateCount = bestCandidateIndex;
        } else {
            rayData->candidateCount = 0;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.raycastfindclosest
     * @recoil-artifact defines .text recoil:function:0x444e90: CZDisplayInstance::RaycastFindClosest.
     *
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall RaycastFindClosest(
        CZNodePartial * world,
        float startX,
        float startY,
        float startZ,
        float endX,
        float endY,
        float endZ,
        PlayerProbeSampleCandidateBuffer* rayData
    )
    {
        rayData->candidateCount = 0;

        if (world == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c", 0x7d1, "Null node pointer.");
            return 5;
        }

        if (world->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c", 0x7d2, "Null class data pointer");
            return 5;
        }

        if (g_CZTypeList_Buckets[7].head != 0) {
            CZTypeList::UpdateQueuedTrees();
        }

        CZWorldDataPartial* worldData = (CZWorldDataPartial*)(world->classData);
        if (startX < endX) {
            g_DiSegmentMinX = startX;
            g_DiSegmentMaxX = endX;
        } else {
            g_DiSegmentMinX = endX;
            g_DiSegmentMaxX = startX;
        }
        if (startY < endY) {
            g_DiSegmentMinY = startY;
            g_DiSegmentMaxY = endY;
        } else {
            g_DiSegmentMinY = endY;
            g_DiSegmentMaxY = startY;
        }
        if (startZ < endZ) {
            g_DiSegmentMinZ = startZ;
            g_DiSegmentMaxZ = endZ;
        } else {
            g_DiSegmentMinZ = endZ;
            g_DiSegmentMaxZ = startZ;
        }

        g_DiPickQueryPoint.x = startX;
        g_DiPickQueryPoint.y = startY;
        g_DiPickQueryPoint.z = startZ;
        g_DiSegmentEnd.x = endX;
        g_DiSegmentEnd.y = endY;
        g_DiSegmentEnd.z = endZ;
        g_DiPickCandidateBuffer = rayData;
        g_DiPickCandidateCursor = rayData->entries;

        zMat4x3 slotBuffer;
        zMath::MatStackPushPtr((float*)(&slotBuffer));
        zMath::MatLoadIdentity();

        if (worldData->clampQueriesToBounds != 0
            || (g_DiSegmentMinX < worldData->worldMaxX && g_DiSegmentMaxX >= worldData->originX
                && g_DiSegmentMinZ <= worldData->originZ && g_DiSegmentMaxZ > worldData->worldMaxZ)) {
            worldData = (CZWorldDataPartial*)(world->classData);
            int gridCol = (int)(floor((g_DiPickQueryPoint.x - worldData->originX) * worldData->areaInvSizeX));
            int gridRow = (int)(floor((g_DiPickQueryPoint.z - worldData->originZ) * worldData->areaInvSizeZ));
            zVec3 delta;
            delta.x = g_DiSegmentEnd.x - g_DiPickQueryPoint.x;
            delta.z = g_DiSegmentEnd.z - g_DiPickQueryPoint.z;
            int gridColStep;
            int gridRowStep;
            float invDeltaX;
            float invDeltaZ;
            if (delta.x == 0.0f) {
                gridColStep = 0;
            } else {
                invDeltaX = 1.0f / delta.x;
                int deltaBitsX;
                int scaleBitsX;
                memcpy(&deltaBitsX, &delta.x, sizeof(int));
                memcpy(&scaleBitsX, &worldData->areaInvSizeX, sizeof(int));
                gridColStep = ((deltaBitsX ^ scaleBitsX) & 0x80000000) != 0 ? -1 : 1;
            }
            if (delta.z == 0.0f) {
                gridRowStep = 0;
            } else {
                invDeltaZ = 1.0f / delta.z;
                int deltaBitsZ;
                int scaleBitsZ;
                memcpy(&deltaBitsZ, &delta.z, sizeof(int));
                memcpy(&scaleBitsZ, &worldData->areaInvSizeZ, sizeof(int));
                gridRowStep = ((deltaBitsZ ^ scaleBitsZ) & 0x80000000) != 0 ? -1 : 1;
            }

            float tToNextGridColBoundary = 2.0f;
            float tToNextGridRowBoundary = 2.0f;
            for (;;) {
                int visitCell = 1;
                int usedClampedCell;
                int candidateCountBeforeCell;
                int cellCol;
                int cellRow;
                float offsetX;
                float offsetZ;
                if (gridCol >= 0 && gridCol < worldData->areaGridColCount && gridRow >= 0
                    && gridRow < worldData->areaGridRowCount) {
                    usedClampedCell = 0;
                } else if (worldData->clampQueriesToBounds == 0) {
                    visitCell = 0;
                } else {
                    usedClampedCell = 1;
                    candidateCountBeforeCell = g_DiPickCandidateBuffer->candidateCount;
                    cellCol = gridCol;
                    cellRow = gridRow;
                    if (gridCol > worldData->areaGridColCount - 1) {
                        cellCol = worldData->areaGridColCount - 1;
                    } else if (gridCol < 0) {
                        cellCol = 0;
                    }

                    if (gridRow > worldData->areaGridRowCount - 1) {
                        cellRow = worldData->areaGridRowCount - 1;
                    } else if (gridRow < 0) {
                        cellRow = 0;
                    }

                    offsetX = (float)(cellCol - gridCol) * worldData->areaCellSizeX;
                    offsetZ = (float)(cellRow - gridRow) * worldData->areaCellSizeZ;
                }

                if (visitCell != 0) {
                    zWorldAreaPartial* area;
                    if (usedClampedCell != 0) {
                        g_DiPickQueryPoint.x += offsetX;
                        g_DiPickQueryPoint.z += offsetZ;
                        g_DiSegmentEnd.x += offsetX;
                        g_DiSegmentEnd.z += offsetZ;
                        g_DiSegmentMinX += offsetX;
                        g_DiSegmentMinZ += offsetZ;
                        g_DiSegmentMaxX += offsetX;
                        g_DiSegmentMaxZ += offsetZ;
                        area = &worldData->areaGridRows[cellRow][cellCol];
                    } else {
                        area = &worldData->areaGridRows[gridRow][gridCol];
                    }

                    for (int i = 0; i < area->childCount; ++i) {
                        CZNodePartial* node = area->childList[i];
                        if ((node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & kNodeFlagRaycastable) != 0) {
                            BuildPickCandidatesForSegmentChildFallback(node, area->childCount + 1);
                            if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                                // Retail leaves the walk straight from the child loop (no second hit test),
                                // keeping the clamped-cell offsets applied.
                                zMath::MatStackPopPtr();
                                g_cls_di_StopAfterFirstHit = 0;
                                return rayData->candidateCount <= 0 ? 1 : 0;
                            }
                        }
                    }

                    if (usedClampedCell != 0) {
                        g_DiPickQueryPoint.x -= offsetX;
                        g_DiPickQueryPoint.z -= offsetZ;
                        g_DiSegmentEnd.x -= offsetX;
                        g_DiSegmentEnd.z -= offsetZ;
                        g_DiSegmentMinX -= offsetX;
                        g_DiSegmentMinZ -= offsetZ;
                        g_DiSegmentMaxX -= offsetX;
                        g_DiSegmentMaxZ -= offsetZ;
                        for (int i = candidateCountBeforeCell; i < g_DiPickCandidateBuffer->candidateCount; ++i) {
                            g_DiPickCandidateBuffer->entries[i].hitPos.x -= offsetX;
                            g_DiPickCandidateBuffer->entries[i].hitPos.z -= offsetZ;
                        }
                    }
                }

                if (gridColStep != 0) {
                    int boundaryCol = gridCol;
                    if (gridColStep == 1) {
                        boundaryCol = gridCol + 1;
                    }
                    tToNextGridColBoundary
                        = ((float)(boundaryCol)*worldData->areaCellSizeX + worldData->originX - g_DiPickQueryPoint.x)
                        * invDeltaX;
                }

                if (gridRowStep != 0) {
                    int boundaryRow = gridRow;
                    if (gridRowStep == 1) {
                        boundaryRow = gridRow + 1;
                    }
                    tToNextGridRowBoundary
                        = ((float)(boundaryRow)*worldData->areaCellSizeZ + worldData->originZ - g_DiPickQueryPoint.z)
                        * invDeltaZ;
                }

                if (tToNextGridColBoundary > 1.0f && tToNextGridRowBoundary > 1.0f) {
                    break;
                }

                if (tToNextGridColBoundary <= tToNextGridRowBoundary) {
                    gridCol += gridColStep;
                }
                if (tToNextGridRowBoundary <= tToNextGridColBoundary) {
                    gridRow += gridRowStep;
                }
            }
        }

        if ((g_cls_di_BreakOnFirstCandidate == 0 || rayData->candidateCount <= 0) && world->listCountB > 0) {
            BuildPickCandidatesForSegment(world);
        }

        zMath::MatStackPopPtr();
        g_cls_di_StopAfterFirstHit = 0;

        return rayData->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegment
     * @recoil-artifact defines .text recoil:function:0x4455f0: CZDisplayInstance::BuildPickCandidatesForSegment.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegment(CZNodePartial * self)
    {
        int result = self->listCountB;
        {
            for (int childIndex = 0; childIndex < result; ++childIndex) {
                CZNodePartial* child = self->listB[childIndex];
                const int childFlags = child->flags;
                if ((childFlags & kNodeFlagEnabledForPick) != 0 && (childFlags & kNodeFlagRaycastable) != 0
                    && VariantTag::CurrentAllowsId(child->nodeType) != 0) {
                    BuildPickCandidatesForSegmentChildFallback(child, self->listCountB + 1);
                    result = g_cls_di_BreakOnFirstCandidate;
                    if (result != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                        break;
                    }
                }

                result = self->listCountB;
            }
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentchildfallback
     * @recoil-artifact defines .text recoil:function:0x445650: CZDisplayInstance::BuildPickCandidatesForSegmentChildFallback.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentChildFallback(CZNodePartial * node, int nodeCountHint)
    {
        if ((node->flags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((node->flags & kNodeFlagRaycastable) == 0) {
            return 1;
        }
        if ((g_cls_di_StopAfterFirstHit & node->flags) != 0) {
            return 1;
        }
        if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
            return 0;
        }
        if (VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        node->flags &= ~kNodeFlagClearDuringPick;
        const int nodeFlags = node->flags;
        if (g_DiPickCandidateBuffer->candidateCount >= kMaxPickCandidates) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0x94c,
                "Database intersections array is full"
            );
            return 1;
        }

        int result; // Unset for sound nodes (retail returns the stale slot).
        switch (node->classId) {
        case kNodeClassObject3D: {
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FilterPointsBBox(node, (void*)((unsigned int)(nodeFlags)));
                if (result != 0) {
                    return result;
                }

                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    g_DiPickCandidateCursor->node = node;
                    ++g_DiPickCandidateCursor;
                    ++g_DiPickCandidateBuffer->candidateCount;
                    return 0;
                }
            }

            CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(node->classData);
            int pushedMatrix;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) != 0) {
                    if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                        zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                        zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 1);
                        if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                            objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                        }
                    } else {
                        zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                    }
                } else {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 3);
                }
            } else {
                pushedMatrix = 0;
            }

            zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
            if (faceData != 0
                && AppendPickCandidatesForFace(faceData, g_DiPickCandidateCursor, &g_DiPickQueryPoint, &g_DiSegmentEnd)
                    != 0) {
                g_DiPickCandidateCursor->node = node;
                ++g_DiPickCandidateCursor;
                ++g_DiPickCandidateBuffer->candidateCount;
            }
            if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
                && node->listCountB > 0) {
                for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                    CZNodePartial* child = node->listB[childIndex];
                    if ((child->flags & kNodeFlagEnabledForPick) != 0 && (child->flags & kNodeFlagRaycastable) != 0) {
                        BuildPickCandidatesForSegmentChildFallback(child, node->listCountB);
                        if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                            break;
                        }
                    }
                }
            }

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }

            return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
        }

        case kNodeClassLod: {
            CZLodDataPartial* lodData = (CZLodDataPartial*)(node->classData);
            if (lodData->nearRangeSq > 5.0) {
                return 1;
            }

            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FilterPointsBBox(node, (void*)((unsigned int)(nodeFlags)));
                if (result != 0) {
                    return result;
                }

                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    g_DiPickCandidateCursor->node = node;
                    ++g_DiPickCandidateCursor;
                    ++g_DiPickCandidateBuffer->candidateCount;
                    return 0;
                }
            }

            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                BuildPickCandidatesForSegmentChildFallback(node->listB[childIndex], node->listCountB);

                if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                    break;
                }
            }
            return g_DiPickCandidateBuffer->candidateCount == 0 ? 1 : 0;
        }

        case kNodeClassSequence: {
            CZSequenceDataPartial* sequenceData = (CZSequenceDataPartial*)(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FilterPointsBBox(node, (void*)((unsigned int)(nodeFlags)));
                if (result != 0) {
                    return result;
                }

                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    g_DiPickCandidateCursor->node = node;
                    ++g_DiPickCandidateCursor;
                    ++g_DiPickCandidateBuffer->candidateCount;
                    return 0;
                }
            }

            const int childResult = BuildPickCandidatesForSegmentChildFallback(
                sequenceData->entries[sequenceData->currentIndex].node,
                node->listCountB
            );
            return childResult;
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesForSegmentRecursive(node, nodeCountHint);

        case kNodeClassCamera:
            return BuildPickCandidatesForSegmentForCamera(node, nodeCountHint);

        case kNodeClassLight:
            return BuildPickCandidatesForSegmentForLight(node, nodeCountHint);

        case kNodeClassSound:
            break;

        default:
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0x97a,
                "Unrecognized node class type:  node = %s class_type = %d",
                node,
                node->classId
            );
            result = 3;
            break;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentrecursive
     * @recoil-artifact defines .text recoil:function:0x445a00: CZDisplayInstance::BuildPickCandidatesForSegmentRecursive.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentRecursive(CZNodePartial * node, int depth)
    {
        if (depth > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FilterPointsBBox(node, (void*)((unsigned int)(depth)));
            if (result != 0) {
                return result;
            }

            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                g_DiPickCandidateCursor->node = node;
                ++g_DiPickCandidateCursor;
                ++g_DiPickCandidateBuffer->candidateCount;
                return 0;
            }
        }

        CZAnimateDataPartial* animateData = (CZAnimateDataPartial*)(node->classData);
        int pushedMatrix;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3*)(animateData->animatedTransform), 1);
        } else {
            pushedMatrix = 0;
        }

        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0
            && AppendPickCandidatesForFace(faceData, g_DiPickCandidateCursor, &g_DiPickQueryPoint, &g_DiSegmentEnd)
                != 0) {
            g_DiPickCandidateCursor->node = node;
            ++g_DiPickCandidateCursor;
            ++g_DiPickCandidateBuffer->candidateCount;
        }
        if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
            && node->listCountB > 0) {
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                BuildPickCandidatesForSegmentChildFallback(node->listB[childIndex], node->listCountB);
                if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                    break;
                }
            }
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentforcamera
     * @recoil-artifact defines .text recoil:function:0x445b20: CZDisplayInstance::BuildPickCandidatesForSegmentForCamera.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentForCamera(CZNodePartial * node, int /*depth*/)
    {
        zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
        CZCameraDataPartial* cameraData = (CZCameraDataPartial*)(node->classData);

        int pushedMatrix;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
            zMath::MatApplyLocalTRS(&cameraData->localRotation, &cameraData->localPosition, &unitScale);
        } else {
            pushedMatrix = 0;
        }

        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0
            && AppendPickCandidatesForFace(faceData, g_DiPickCandidateCursor, &g_DiPickQueryPoint, &g_DiSegmentEnd)
                != 0) {
            g_DiPickCandidateCursor->node = node;
            ++g_DiPickCandidateCursor;
            ++g_DiPickCandidateBuffer->candidateCount;
        }
        if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
            && node->listCountB > 0) {
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                BuildPickCandidatesForSegmentChildFallback(node->listB[childIndex], node->listCountB);
                if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                    break;
                }
            }
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentforlight
     * @recoil-artifact defines .text recoil:function:0x445c20: CZDisplayInstance::BuildPickCandidatesForSegmentForLight.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentForLight(CZNodePartial * node, int depth)
    {
        CZLightDataPartial* lightData = (CZLightDataPartial*)(node->classData);

        if (depth > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FilterPointsBBox(node, (void*)((unsigned int)(depth)));
            if (result != 0) {
                return result;
            }

            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                g_DiPickCandidateCursor->node = node;
                ++g_DiPickCandidateCursor;
                ++g_DiPickCandidateBuffer->candidateCount;
                return 0;
            }
        }

        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y, lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0
            && AppendPickCandidatesForFace(faceData, g_DiPickCandidateCursor, &g_DiPickQueryPoint, &g_DiSegmentEnd)
                != 0) {
            g_DiPickCandidateCursor->node = node;
            ++g_DiPickCandidateCursor;
            ++g_DiPickCandidateBuffer->candidateCount;
        }
        if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
            && node->listCountB > 0) {
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                BuildPickCandidatesForSegmentChildFallback(node->listB[childIndex], node->listCountB);
                if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                    break;
                }
            }
        }

        zMath::MatStackPopPtr();
        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildprobehitbatchesforsegments
     * @recoil-artifact defines .text recoil:function:0x445d40: CZDisplayInstance::BuildProbeHitBatchesForSegments.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    void __fastcall BuildProbeHitBatchesForSegments(
        CZNodePartial * world,
        CZDisplayInstanceSegmentEndpoints * segmentEndpoints,
        int endpointCount,
        PlayerProbeSampleCandidateBuffer* hitBatches
    )
    {
        if (endpointCount > 24) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0xba9,
                "More test pnts than space for: %d",
                endpointCount
            );
            endpointCount = 24;
        }

        if (g_CZTypeList_Buckets[7].head != 0) {
            CZTypeList::UpdateQueuedTrees();
        }

        const int segmentCount = endpointCount >> 1;
        int segmentActive[12];
        for (int activeIndex = 0; activeIndex < segmentCount; ++activeIndex) {
            segmentActive[activeIndex] = 1;
            hitBatches[activeIndex].candidateCount = 0;
        }

        CZWorldDataPartial* worldData = (CZWorldDataPartial*)(world->classData);
        int anyActive = 0;
        const CZDisplayInstanceSegmentEndpoints* segment = segmentEndpoints;
        for (int boundsIndex = 0; boundsIndex < segmentCount; ++boundsIndex, ++segment) {
            const zVec3* start = &segment->start;
            const zVec3* end = &segment->end;
            if (start->x < end->x) {
                g_DiSegmentBounds[boundsIndex].minX = start->x;
                g_DiSegmentBounds[boundsIndex].maxX = end->x;
            } else {
                g_DiSegmentBounds[boundsIndex].minX = end->x;
                g_DiSegmentBounds[boundsIndex].maxX = start->x;
            }
            if (start->y < end->y) {
                g_DiSegmentBounds[boundsIndex].minY = start->y;
                g_DiSegmentBounds[boundsIndex].maxY = end->y;
            } else {
                g_DiSegmentBounds[boundsIndex].minY = end->y;
                g_DiSegmentBounds[boundsIndex].maxY = start->y;
            }
            if (start->z < end->z) {
                g_DiSegmentBounds[boundsIndex].minZ = start->z;
                g_DiSegmentBounds[boundsIndex].maxZ = end->z;
            } else {
                g_DiSegmentBounds[boundsIndex].minZ = end->z;
                g_DiSegmentBounds[boundsIndex].maxZ = start->z;
            }

            // Retail tests the first segment's bounds (g_DiSegmentBounds[0]) for every segment.
            if (worldData->clampQueriesToBounds == 0
                && !(
                    g_DiSegmentBounds[0].minX < worldData->worldMaxX && g_DiSegmentBounds[0].maxX >= worldData->originX
                    && g_DiSegmentBounds[0].minZ <= worldData->originZ
                    && g_DiSegmentBounds[0].maxZ > worldData->worldMaxZ
                )) {
                segmentActive[boundsIndex] = 0;
            } else {
                anyActive = 1;
            }
        }

        if (anyActive != 0) {
            zMat4x3 slotBuffer;
            zMath::MatStackPushPtr((float*)(&slotBuffer));
            zMath::MatLoadIdentity();

            g_DiPickCandidateBuffer = hitBatches;
            g_DiPickPointArray = &segmentEndpoints[0].start;
            g_DiPickPointCount = segmentCount;

            BuildPickCandidatesForSegmentsInGridWindow(world, segmentActive);
            if (world->listCountB > 0) {
                for (int worldNodeIndex = 0; worldNodeIndex < world->listCountB; ++worldNodeIndex) {
                    CZNodePartial* node = world->listB[worldNodeIndex];
                    if ((node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & kNodeFlagRaycastable) != 0
                        && VariantTag::CurrentAllowsId(node->nodeType) != 0) {
                        BuildPickCandidatesForSegmentsRecursive(node, world->listCountB + 1, segmentActive);
                        if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                            break;
                        }
                    }
                }
            }

            zMath::MatStackPopPtr();
        }

        g_cls_di_StopAfterFirstHit = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentsingridwindow
     * @recoil-artifact defines .text recoil:function:0x445f60: CZDisplayInstance::BuildPickCandidatesForSegmentsInGridWindow.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    void __fastcall BuildPickCandidatesForSegmentsInGridWindow(CZNodePartial * world, int* activeMask)
    {
        CZWorldDataPartial* worldData = (CZWorldDataPartial*)(world->classData);
        GridCell windowMin;
        GridCell windowMax;
        {
            // Grid cells of each segment's (minX, maxZ) and (maxX, minZ) corners; the grid rows run opposite to Z.
            GridCell minCell[12];
            GridCell maxCell[12];
            for (int i = 0; i < g_DiPickPointCount; ++i) {
                minCell[i].col
                    = (int)(floor((g_DiSegmentBounds[i].minX - worldData->originX) * worldData->areaInvSizeX));
                minCell[i].row
                    = (int)(floor((g_DiSegmentBounds[i].maxZ - worldData->originZ) * worldData->areaInvSizeZ));
                maxCell[i].col
                    = (int)(floor((g_DiSegmentBounds[i].maxX - worldData->originX) * worldData->areaInvSizeX));
                maxCell[i].row
                    = (int)(floor((g_DiSegmentBounds[i].minZ - worldData->originZ) * worldData->areaInvSizeZ));
            }

            windowMin.col = minCell[0].col;
            windowMin.row = minCell[0].row;
            windowMax.col = maxCell[0].col;
            windowMax.row = maxCell[0].row;
            for (int windowIndex = 1; windowIndex < g_DiPickPointCount; ++windowIndex) {
                if (minCell[windowIndex].col < windowMin.col) {
                    windowMin.col = minCell[windowIndex].col;
                }
                if (minCell[windowIndex].row < windowMin.row) {
                    windowMin.row = minCell[windowIndex].row;
                }
                if (maxCell[windowIndex].col > windowMax.col) {
                    windowMax.col = maxCell[windowIndex].col;
                }
                if (maxCell[windowIndex].row > windowMax.row) {
                    windowMax.row = maxCell[windowIndex].row;
                }
            }
        }

        GridCell cell;
        for (cell.row = windowMin.row; cell.row <= windowMax.row; ++cell.row) {
            for (cell.col = windowMin.col; cell.col <= windowMax.col; ++cell.col) {
                int visitCell = 1;
                int usedClampedCell;
                int firstNewCandidate[12];
                int cellCol;
                int cellRow;
                float offsetX;
                float offsetZ;
                if (cell.col >= 0 && cell.col < worldData->areaGridColCount && cell.row >= 0
                    && cell.row < worldData->areaGridRowCount) {
                    usedClampedCell = 0;
                } else if (worldData->clampQueriesToBounds == 0) {
                    visitCell = 0;
                } else {
                    usedClampedCell = 1;
                    for (int segmentIndex = 0; segmentIndex < g_DiPickPointCount; ++segmentIndex) {
                        firstNewCandidate[segmentIndex] = g_DiPickCandidateBuffer[segmentIndex].candidateCount;
                    }

                    cellCol = cell.col;
                    cellRow = cell.row;
                    if (cell.col > worldData->areaGridColCount - 1) {
                        cellCol = worldData->areaGridColCount - 1;
                    } else if (cell.col < 0) {
                        cellCol = 0;
                    }

                    if (cell.row > worldData->areaGridRowCount - 1) {
                        cellRow = worldData->areaGridRowCount - 1;
                    } else if (cell.row < 0) {
                        cellRow = 0;
                    }

                    offsetX = (float)(cellCol - cell.col) * worldData->areaCellSizeX;
                    offsetZ = (float)(cellRow - cell.row) * worldData->areaCellSizeZ;
                }

                if (visitCell != 0) {
                    zWorldAreaPartial* area;
                    if (usedClampedCell != 0) {
                        // Endpoint pairs indexed as i + i and i + i + 1: retail gives the end-point x its
                        // own induction variable (0x4461da 'mov esi,0xc'); i * 2 + 1 folds it into the shared one.
                        for (int segmentIndex = 0; segmentIndex < g_DiPickPointCount; ++segmentIndex) {
                            g_DiPickPointArray[segmentIndex + segmentIndex].x += offsetX;
                            g_DiPickPointArray[segmentIndex + segmentIndex].z += offsetZ;
                            g_DiPickPointArray[segmentIndex + segmentIndex + 1].x += offsetX;
                            g_DiPickPointArray[segmentIndex + segmentIndex + 1].z += offsetZ;
                            g_DiSegmentBounds[segmentIndex].minX += offsetX;
                            g_DiSegmentBounds[segmentIndex].minZ += offsetZ;
                            g_DiSegmentBounds[segmentIndex].maxX += offsetX;
                            g_DiSegmentBounds[segmentIndex].maxZ += offsetZ;
                        }
                        area = &worldData->areaGridRows[cellRow][cellCol];
                    } else {
                        area = &worldData->areaGridRows[cell.row][cell.col];
                    }

                    for (int childIndex = 0; childIndex < area->childCount; ++childIndex) {
                        CZNodePartial* child = area->childList[childIndex];
                        if ((child->flags & kNodeFlagEnabledForPick) != 0
                            && (child->flags & kNodeFlagRaycastable) != 0) {
                            BuildPickCandidatesForSegmentsRecursive(child, area->childCount + 1, activeMask);
                            if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                                break;
                            }
                        }
                    }

                    if (usedClampedCell != 0) {
                        for (int segmentIndex = 0; segmentIndex < g_DiPickPointCount; ++segmentIndex) {
                            g_DiPickPointArray[segmentIndex + segmentIndex].x -= offsetX;
                            g_DiPickPointArray[segmentIndex + segmentIndex].z -= offsetZ;
                            g_DiPickPointArray[segmentIndex + segmentIndex + 1].x -= offsetX;
                            g_DiPickPointArray[segmentIndex + segmentIndex + 1].z -= offsetZ;
                            g_DiSegmentBounds[segmentIndex].minX -= offsetX;
                            g_DiSegmentBounds[segmentIndex].minZ -= offsetZ;
                            g_DiSegmentBounds[segmentIndex].maxX -= offsetX;
                            g_DiSegmentBounds[segmentIndex].maxZ -= offsetZ;
                        }
                        for (int bufferIndex = 0; bufferIndex < g_DiPickPointCount; ++bufferIndex) {
                            for (int candidateIndex = firstNewCandidate[bufferIndex];
                                candidateIndex < g_DiPickCandidateBuffer[bufferIndex].candidateCount;
                                ++candidateIndex) {
                                g_DiPickCandidateBuffer[bufferIndex].entries[candidateIndex].hitPos.x -= offsetX;
                                g_DiPickCandidateBuffer[bufferIndex].entries[candidateIndex].hitPos.z -= offsetZ;
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentsrecursive
     * @recoil-artifact defines .text recoil:function:0x446440: CZDisplayInstance::BuildPickCandidatesForSegmentsRecursive.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentsRecursive(CZNodePartial * node, int nodeCountHint, int* activeMask)
    {
        if ((node->flags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((node->flags & kNodeFlagRaycastable) == 0) {
            return 1;
        }
        if ((g_cls_di_StopAfterFirstHit & node->flags) != 0) {
            return 1;
        }
        if (VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        node->flags &= ~kNodeFlagClearDuringPick;
        const int nodeFlags = node->flags;

        int result; // Unset for sound nodes (retail returns the stale slot).
        switch (node->classId) {
        case kNodeClassObject3D: {
            int localActive[12];
            memcpy(localActive, activeMask, (size_t)(g_DiPickPointCount) * sizeof(int));
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FrustumTestAndPick(node, localActive);
                if (result != 0) {
                    return result;
                }
                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    return 0;
                }
            }

            CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(node->classData);
            int pushedMatrix;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) != 0) {
                    if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                        zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                        zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 1);
                        if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                            objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                        }
                    } else {
                        zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                    }
                } else {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), 3);
                }
            } else {
                pushedMatrix = 0;
            }

            zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
            if (faceData != 0) {
                CZDisplayInstance::FilterRegionsAgainstPolygon(
                    node,
                    faceData,
                    (CZDisplayInstanceSegmentEndpoints*)((void*)(g_DiPickPointArray)),
                    localActive,
                    g_DiPickPointCount,
                    g_DiPickCandidateBuffer
                );
            }
            if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
                && node->listCountB > 0) {
                for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                    CZNodePartial* child = node->listB[childIndex];
                    if ((child->flags & kNodeFlagEnabledForPick) != 0 && (child->flags & kNodeFlagRaycastable) != 0) {
                        BuildPickCandidatesForSegmentsRecursive(child, node->listCountB, localActive);
                        if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                            break;
                        }
                    }
                }
            }
            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassLod: {
            CZLodDataPartial* lodData = (CZLodDataPartial*)(node->classData);
            if (lodData->nearRangeSq > 5.0) {
                return 1;
            }

            int localActive[12];
            memcpy(localActive, activeMask, (size_t)(g_DiPickPointCount) * sizeof(int));
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FrustumTestAndPick(node, localActive);
                if (result != 0) {
                    return result;
                }
                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    return 0;
                }
            }

            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                BuildPickCandidatesForSegmentsRecursive(node->listB[childIndex], node->listCountB, localActive);
                if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                    break;
                }
            }
            return 0;
        }

        case kNodeClassSequence: {
            CZSequenceDataPartial* sequenceData = (CZSequenceDataPartial*)(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            int localActive[12];
            memcpy(localActive, activeMask, (size_t)(g_DiPickPointCount) * sizeof(int));
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FrustumTestAndPick(node, localActive);
                if (result != 0) {
                    return result;
                }
                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    return 0;
                }
            }

            const int childResult = BuildPickCandidatesForSegmentsRecursive(
                sequenceData->entries[sequenceData->currentIndex].node,
                node->listCountB,
                localActive
            );
            return childResult;
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesForSegmentsForAnimate(node, nodeCountHint, activeMask);

        case kNodeClassCamera: {
            zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
            CZCameraDataPartial* cameraData = (CZCameraDataPartial*)(node->classData);
            int pushedMatrix;
            if ((nodeFlags & kNodeFlagEnabledForPick) != 0) {
                pushedMatrix = 1;
                zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
                zMath::MatApplyLocalTRS(&cameraData->localRotation, &cameraData->localPosition, &unitScale);
            } else {
                pushedMatrix = 0;
            }

            zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
            if (faceData != 0) {
                CZDisplayInstance::FilterRegionsAgainstPolygon(
                    node,
                    faceData,
                    (CZDisplayInstanceSegmentEndpoints*)((void*)(g_DiPickPointArray)),
                    activeMask,
                    g_DiPickPointCount,
                    g_DiPickCandidateBuffer
                );
            }
            if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
                && node->listCountB > 0) {
                for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                    BuildPickCandidatesForSegmentsRecursive(node->listB[childIndex], node->listCountB, activeMask);
                    if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                        break;
                    }
                }
            }
            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassLight:
            return BuildPickCandidatesForSegmentsForLight(node, nodeCountHint, activeMask);

        case kNodeClassSound:
            break;

        default:
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0xd41,
                "Unrecognized node class type:  node = %s class_type = %d",
                node,
                node->classId
            );
            result = 3;
            break;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentsforanimate
     * @recoil-artifact defines .text recoil:function:0x446880: CZDisplayInstance::BuildPickCandidatesForSegmentsForAnimate.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentsForAnimate(CZNodePartial * node, int nodeCountHint, int* activeMask)
    {
        int localActive[12];
        memcpy(localActive, activeMask, (size_t)(g_DiPickPointCount) * sizeof(int));

        if (nodeCountHint > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FrustumTestAndPick(node, localActive);
            if (result != 0) {
                return result;
            }
            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                return 0;
            }
        }

        int pushedMatrix;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            CZAnimateDataPartial* const animateData = (CZAnimateDataPartial*)(node->classData);
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3*)(animateData->animatedTransform), 1);
        } else {
            pushedMatrix = 0;
        }

        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0) {
            CZDisplayInstance::FilterRegionsAgainstPolygon(
                node,
                faceData,
                (CZDisplayInstanceSegmentEndpoints*)((void*)(g_DiPickPointArray)),
                localActive,
                g_DiPickPointCount,
                g_DiPickCandidateBuffer
            );
        }
        if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
            && node->listCountB > 0) {
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                BuildPickCandidatesForSegmentsRecursive(node->listB[childIndex], node->listCountB, localActive);
                if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                    break;
                }
            }
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.buildpickcandidatesforsegmentsforlight
     * @recoil-artifact defines .text recoil:function:0x446970: CZDisplayInstance::BuildPickCandidatesForSegmentsForLight.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentsForLight(CZNodePartial * node, int nodeCountHint, int* activeMask)
    {
        int localActive[12];
        memcpy(localActive, activeMask, (size_t)(g_DiPickPointCount) * sizeof(int));

        if (nodeCountHint > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FrustumTestAndPick(node, localActive);
            if (result != 0) {
                return result;
            }
            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                return 0;
            }
        }

        CZLightDataPartial* lightData = (CZLightDataPartial*)(node->classData);
        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y, lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((unsigned int)(node->userDataOrDiRef));
        if (faceData != 0) {
            CZDisplayInstance::FilterRegionsAgainstPolygon(
                node,
                faceData,
                (CZDisplayInstanceSegmentEndpoints*)((void*)(g_DiPickPointArray)),
                localActive,
                g_DiPickPointCount,
                g_DiPickCandidateBuffer
            );
        }
        if ((g_cls_di_BreakOnFirstCandidate == 0 || g_DiPickCandidateBuffer->candidateCount <= 0)
            && node->listCountB > 0) {
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                CZNodePartial* child = node->listB[childIndex];
                if ((child->flags & kNodeFlagEnabledForPick) != 0 && (child->flags & kNodeFlagRaycastable) != 0) {
                    BuildPickCandidatesForSegmentsRecursive(child, node->listCountB, localActive);
                    if (g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0) {
                        break;
                    }
                }
            }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.filterregionsagainstsphere
     * @recoil-artifact defines .text recoil:function:0x446a80: CZDisplayInstance::FilterRegionsAgainstSphere.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsAgainstSphere(
        CZNodePartial * world,
        zVec3 * center,
        const char* nodeNamePrefix,
        float radius,
        int enableDistanceCull,
        int requireLineOfSight,
        OptCatalogRaycastHitList* outHitList
    )
    {
        if (world == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c", 0xf8a, "Null node pointer.");
            return 5;
        }

        if (world->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c", 0xf8b, "Null class data pointer");
            return 5;
        }

        if (g_CZTypeList_Buckets[7].head != 0) {
            CZTypeList::UpdateQueuedTrees();
        }

        outHitList->hitCount = 0;
        CZWorldDataPartial* worldData = (CZWorldDataPartial*)(world->classData);

        // XZ bounds of the sphere: the retail frame keeps this box, with its unwritten y and FPU-consumed homes.
        zBBox3f sphereBounds;
        sphereBounds.min.x = center->x - radius;
        sphereBounds.min.z = center->z - radius;
        sphereBounds.max.x = center->x + radius;
        sphereBounds.max.z = center->z + radius;

        GridCell minCell;
        int result = CZWorld::WorldToGridCoordsClamped(
            world,
            sphereBounds.min.x,
            sphereBounds.max.z,
            &minCell.col,
            &minCell.row
        );
        if (result != 0) {
            return result;
        }

        GridCell maxCell;
        result = CZWorld::WorldToGridCoordsClamped(
            world,
            sphereBounds.max.x,
            sphereBounds.min.z,
            &maxCell.col,
            &maxCell.row
        );
        if (result != 0) {
            return result;
        }

        g_CZDisplayInstance_FilterRegions_NodeNamePrefix = nodeNamePrefix;
        g_CZDisplayInstance_FilterRegions_Center = center;
        g_CZDisplayInstance_FilterRegions_RadiusSq = radius * radius;
        g_CZDisplayInstance_FilterRegions_EnableClearanceCheck = enableDistanceCull;
        g_CZDisplayInstance_FilterRegions_LineOfSightWorld = requireLineOfSight != 0 ? world : 0;
        g_CZDisplayInstance_FilterRegions_OutHitList = outHitList;

        GridCell cell;
        for (cell.row = minCell.row; cell.row <= maxCell.row; ++cell.row) {
            zWorldAreaPartial* area = &worldData->areaGridRows[cell.row][minCell.col];
            for (cell.col = minCell.col; cell.col <= maxCell.col; ++cell.col, ++area) {
                for (int childIndex = 0; childIndex < area->childCount; ++childIndex) {
                    CZNodePartial* node = area->childList[childIndex];
                    if (g_CZDisplayInstance_FilterRegions_OutHitList->hitCount >= kMaxPickCandidates) {
                        zError::ReportOld(
                            0x200,
                            "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                            0xff3,
                            "Database intersections array is full"
                        );
                        continue;
                    }

                    int nodeFlags = node->flags;
                    if ((nodeFlags & kNodeFlagEnabledForPick) == 0) {
                        continue;
                    }
                    if ((nodeFlags & kNodeFlagFilterRegionCandidate) == 0) {
                        continue;
                    }
                    if (VariantTag::CurrentAllowsId(node->nodeType) == 0) {
                        continue;
                    }
                    const char* prefix = g_CZDisplayInstance_FilterRegions_NodeNamePrefix;
                    if (prefix != 0 && strncmp(node->name, prefix, strlen(prefix)) != 0) {
                        continue;
                    }

                    nodeFlags = node->flags;
                    if ((nodeFlags & kNodeFlagFilterRegionCandidate) == 0) {
                        for (int nestedIndex = 0; nestedIndex < node->listCountB; ++nestedIndex) {
                            FilterRegionsTryAppendNode(node->listB[nestedIndex]);
                        }
                        continue;
                    }
                    if ((nodeFlags & kNodeFlagCachedBoundsValid) == 0) {
                        continue;
                    }

                    zBBox3f bbox;
                    CZClass::gwNodeGetBBox(node, &bbox);
                    zBBoxCorners corners;
                    CZBBox::ExpandToCorners(&bbox, &corners);

                    zMat4x3 slotBuffer;
                    zMath::MatStackPushPtr((float*)(&slotBuffer));
                    zMath::MatLoadIdentity();
                    result = CZNode::gwNodeBuildNodeToAncestorMatrix(node, 1);
                    if (result != 0) {
                        zMath::MatStackPopPtr();
                        continue;
                    }
                    zMath::MatTransformPointBatchInPlace(corners.corners, 8);
                    zMath::MatStackPopPtr();

                    zVec3 boundsCenter;
                    float boundsRadius;
                    CZBBox::CornersToBoundingSphere(&corners, &boundsCenter, &boundsRadius);
                    float distanceSq;
                    if (g_CZDisplayInstance_FilterRegions_EnableClearanceCheck != 0) {
                        const float clearance
                            = zMath::Vec3DeltaLength(g_CZDisplayInstance_FilterRegions_Center, &boundsCenter)
                            - boundsRadius;
                        if (clearance < 0.0f) {
                            distanceSq = 0.0f;
                        } else {
                            distanceSq = clearance * clearance;
                        }
                        if (distanceSq > g_CZDisplayInstance_FilterRegions_RadiusSq) {
                            continue;
                        }
                    } else {
                        distanceSq = 0.0f;
                    }

                    if (g_CZDisplayInstance_FilterRegions_LineOfSightWorld != 0
                        && (node->flags & kNodeFlagRequiresLineOfSight) != 0) {
                        PlayerProbeSampleCandidateBuffer rayData;
                        CZDisplayInstance::SetBreakOnFirstCandidate(1);
                        CZDisplayInstance::SetStopAfterFirstHit(0x40000);
                        CZClass::gwNodeSetRaycastable(node, 0);
                        const int rayResult = CZDisplayInstance::RaycastFindClosest(
                            g_CZDisplayInstance_FilterRegions_LineOfSightWorld,
                            g_CZDisplayInstance_FilterRegions_Center->x,
                            g_CZDisplayInstance_FilterRegions_Center->y,
                            g_CZDisplayInstance_FilterRegions_Center->z,
                            boundsCenter.x,
                            boundsCenter.y,
                            boundsCenter.z,
                            &rayData
                        );
                        CZClass::gwNodeSetRaycastable(node, 1);
                        CZDisplayInstance::SetBreakOnFirstCandidate(0);
                        if (rayResult == 0 && rayData.candidateCount != 0) {
                            continue;
                        }
                    }

                    OptCatalogRaycastHitEntry* entry
                        = &g_CZDisplayInstance_FilterRegions_OutHitList
                               ->hits[g_CZDisplayInstance_FilterRegions_OutHitList->hitCount];
                    entry->hitNode = node;
                    entry->pos = boundsCenter;
                    entry->surfaceRef = 0;
                    entry->distance = distanceSq;
                    ++g_CZDisplayInstance_FilterRegions_OutHitList->hitCount;
                }
            }
        }

        for (int childIndex = 0; childIndex < world->listCountB; ++childIndex) {
            FilterRegionsTryAppendNode(world->listB[childIndex]);
        }

        return outHitList->hitCount <= 0 ? 1 : 0;
    }
}

namespace CZBBox
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.expandtocorners
     * @recoil-artifact defines .text recoil:function:0x446ed0: CZBBox::ExpandToCorners.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    void __fastcall ExpandToCorners(const zBBox3f* bbox, zBBoxCorners* outCorners)
    {
        zVec3* vertices = outCorners->corners;
        vertices[0].x = bbox->min.x;
        vertices[0].y = bbox->min.y;
        vertices[0].z = bbox->max.z;
        vertices[1].x = bbox->max.x;
        vertices[1].y = bbox->min.y;
        vertices[1].z = bbox->max.z;
        vertices[2].x = bbox->max.x;
        vertices[2].y = bbox->min.y;
        vertices[2].z = bbox->min.z;
        vertices[3].x = bbox->min.x;
        vertices[3].y = bbox->min.y;
        vertices[3].z = bbox->min.z;
        vertices[4].x = bbox->min.x;
        vertices[4].y = bbox->max.y;
        vertices[4].z = bbox->max.z;
        vertices[5].x = bbox->max.x;
        vertices[5].y = bbox->max.y;
        vertices[5].z = bbox->max.z;
        vertices[6].x = bbox->max.x;
        vertices[6].y = bbox->max.y;
        vertices[6].z = bbox->min.z;
        vertices[7].x = bbox->min.x;
        vertices[7].y = bbox->max.y;
        vertices[7].z = bbox->min.z;
    }
}

namespace CZDisplayInstance
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.filterregions-tryappendnode
     * @recoil-artifact defines .text recoil:function:0x446f60: CZDisplayInstance::FilterRegionsTryAppendNode.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsTryAppendNode(CZNodePartial * node)
    {
        if (g_CZDisplayInstance_FilterRegions_OutHitList->hitCount >= kMaxPickCandidates) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c",
                0xff3,
                "Database intersections array is full"
            );
            return 1;
        }

        int nodeFlags = node->flags;
        if ((nodeFlags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((nodeFlags & kNodeFlagFilterRegionCandidate) == 0) {
            return 1;
        }
        if (VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }
        const char* prefix = g_CZDisplayInstance_FilterRegions_NodeNamePrefix;
        if (prefix != 0 && strncmp(node->name, prefix, strlen(prefix)) != 0) {
            return 1;
        }

        nodeFlags = node->flags;
        if ((nodeFlags & kNodeFlagFilterRegionCandidate) == 0) {
            int result = 1;
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                if (FilterRegionsTryAppendNode(node->listB[childIndex]) == 0) {
                    result = 0;
                }
            }
            return result;
        }

        if ((nodeFlags & kNodeFlagCachedBoundsValid) == 0) {
            return 1;
        }

        zBBox3f bbox;
        CZClass::gwNodeGetBBox(node, &bbox);

        zBBoxCorners corners;
        zVec3* vertices = corners.corners;
        vertices[0].x = bbox.min.x;
        vertices[0].y = bbox.min.y;
        vertices[0].z = bbox.max.z;
        vertices[1].x = bbox.max.x;
        vertices[1].y = bbox.min.y;
        vertices[1].z = bbox.max.z;
        vertices[2].x = bbox.max.x;
        vertices[2].y = bbox.min.y;
        vertices[2].z = bbox.min.z;
        vertices[3].x = bbox.min.x;
        vertices[3].y = bbox.min.y;
        vertices[3].z = bbox.min.z;
        vertices[4].x = bbox.min.x;
        vertices[4].y = bbox.max.y;
        vertices[4].z = bbox.max.z;
        vertices[5].x = bbox.max.x;
        vertices[5].y = bbox.max.y;
        vertices[5].z = bbox.max.z;
        vertices[6].x = bbox.max.x;
        vertices[6].y = bbox.max.y;
        vertices[6].z = bbox.min.z;
        vertices[7].x = bbox.min.x;
        vertices[7].y = bbox.max.y;
        vertices[7].z = bbox.min.z;

        zMat4x3 slotBuffer;
        zMath::MatStackPushPtr((float*)(&slotBuffer));
        zMath::MatLoadIdentity();
        const int matrixResult = CZNode::gwNodeBuildNodeToAncestorMatrix(node, 1);
        if (matrixResult != 0) {
            zMath::MatStackPopPtr();
            return matrixResult;
        }

        zMath::MatTransformPointBatchInPlace(corners.corners, 8);
        zMath::MatStackPopPtr();

        zVec3 boundsCenter;
        float boundsRadius;
        CZBBox::CornersToBoundingSphere(&corners, &boundsCenter, &boundsRadius);

        float distanceSq;
        if (g_CZDisplayInstance_FilterRegions_EnableClearanceCheck != 0) {
            const float clearance
                = zMath::Vec3DeltaLength(g_CZDisplayInstance_FilterRegions_Center, &boundsCenter) - boundsRadius;
            if (clearance < 0.0f) {
                distanceSq = 0.0f;
            } else {
                distanceSq = clearance * clearance;
            }
            if (distanceSq > g_CZDisplayInstance_FilterRegions_RadiusSq) {
                return 1;
            }
        } else {
            distanceSq = 0.0f;
        }

        if (g_CZDisplayInstance_FilterRegions_LineOfSightWorld != 0
            && (node->flags & kNodeFlagRequiresLineOfSight) != 0) {
            PlayerProbeSampleCandidateBuffer rayData;
            CZDisplayInstance::SetBreakOnFirstCandidate(1);
            CZDisplayInstance::SetStopAfterFirstHit(0x40000);
            CZClass::gwNodeSetRaycastable(node, 0);
            const int rayResult = CZDisplayInstance::RaycastFindClosest(
                g_CZDisplayInstance_FilterRegions_LineOfSightWorld,
                g_CZDisplayInstance_FilterRegions_Center->x,
                g_CZDisplayInstance_FilterRegions_Center->y,
                g_CZDisplayInstance_FilterRegions_Center->z,
                boundsCenter.x,
                boundsCenter.y,
                boundsCenter.z,
                &rayData
            );
            CZClass::gwNodeSetRaycastable(node, 1);
            CZDisplayInstance::SetBreakOnFirstCandidate(0);
            if (rayResult == 0 && rayData.candidateCount != 0) {
                return 1;
            }
        }

        OptCatalogRaycastHitEntry* entry = &g_CZDisplayInstance_FilterRegions_OutHitList
                                                ->hits[g_CZDisplayInstance_FilterRegions_OutHitList->hitCount];
        entry->hitNode = node;
        entry->pos = boundsCenter;
        entry->surfaceRef = 0;
        entry->distance = distanceSq;
        ++g_CZDisplayInstance_FilterRegions_OutHitList->hitCount;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.ispickquerypointoutsideviewbboxxz
     * @recoil-artifact defines .text recoil:function:0x4472c0: CZDisplayInstance::IsPickQueryPointOutsideViewBBoxXZ.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall IsPickQueryPointOutsideViewBBoxXZ(CZNodePartial * node)
    {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners;
        CZClass::gwNodeGetViewBBoxCorners(node, &corners);

        zBBox3f bounds;
        const zVec3* vertices = corners.corners;
        bounds.max.x = bounds.min.x = vertices[0].x;
        bounds.max.y = bounds.min.y = vertices[0].y;
        bounds.max.z = bounds.min.z = vertices[0].z;
        for (int bboxCornerIndex = 1; bboxCornerIndex < 8; ++bboxCornerIndex) {
            const zVec3* corner = &vertices[bboxCornerIndex];
            if (corner->x < bounds.min.x) {
                bounds.min.x = corner->x;
            } else if (corner->x > bounds.max.x) {
                bounds.max.x = corner->x;
            }
            if (corner->y < bounds.min.y) {
                bounds.min.y = corner->y;
            } else if (corner->y > bounds.max.y) {
                bounds.max.y = corner->y;
            }
            if (corner->z < bounds.min.z) {
                bounds.min.z = corner->z;
            } else if (corner->z > bounds.max.z) {
                bounds.max.z = corner->z;
            }
        }

        return g_DiPickQueryPoint.x >= bounds.min.x && g_DiPickQueryPoint.x <= bounds.max.x
                && g_DiPickQueryPoint.z >= bounds.min.z && g_DiPickQueryPoint.z <= bounds.max.z
            ? 0
            : 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.picktestbbox2d
     * @recoil-artifact defines .text recoil:function:0x4473e0: CZDisplayInstance::PickTestBBox2D.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall PickTestBBox2D(CZNodePartial * node, int* hitFlags)
    {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners;
        CZClass::gwNodeGetViewBBoxCorners(node, &corners);

        zBBox3f bounds;
        const zVec3* vertices = corners.corners;
        bounds.max.x = bounds.min.x = vertices[0].x;
        bounds.max.y = bounds.min.y = vertices[0].y;
        bounds.max.z = bounds.min.z = vertices[0].z;
        for (int bboxCornerIndex = 1; bboxCornerIndex < 8; ++bboxCornerIndex) {
            const zVec3* corner = &vertices[bboxCornerIndex];
            if (corner->x < bounds.min.x) {
                bounds.min.x = corner->x;
            } else if (corner->x > bounds.max.x) {
                bounds.max.x = corner->x;
            }
            if (corner->y < bounds.min.y) {
                bounds.min.y = corner->y;
            } else if (corner->y > bounds.max.y) {
                bounds.max.y = corner->y;
            }
            if (corner->z < bounds.min.z) {
                bounds.min.z = corner->z;
            } else if (corner->z > bounds.max.z) {
                bounds.max.z = corner->z;
            }
        }

        int result = 1;
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            if (hitFlags[i] != 0) {
                const zVec3* point = &g_DiPickPointArray[i];
                if (point->x >= bounds.min.x && point->x <= bounds.max.x && point->z >= bounds.min.z
                    && point->z <= bounds.max.z) {
                    result = 0;
                } else {
                    hitFlags[i] = 0;
                }
            }
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.filterpointsbbox
     * @recoil-artifact defines .text recoil:function:0x447540: CZDisplayInstance::FilterPointsBBox.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterPointsBBox(CZNodePartial * node, void* /*pointData*/)
    {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners;
        CZClass::gwNodeGetViewBBoxCorners(node, &corners);

        zBBox3f bounds;
        const zVec3* vertices = corners.corners;
        bounds.max.x = bounds.min.x = vertices[0].x;
        bounds.max.y = bounds.min.y = vertices[0].y;
        bounds.max.z = bounds.min.z = vertices[0].z;
        for (int bboxCornerIndex = 1; bboxCornerIndex < 8; ++bboxCornerIndex) {
            const zVec3* corner = &vertices[bboxCornerIndex];
            if (corner->x < bounds.min.x) {
                bounds.min.x = corner->x;
            } else if (corner->x > bounds.max.x) {
                bounds.max.x = corner->x;
            }
            if (corner->y < bounds.min.y) {
                bounds.min.y = corner->y;
            } else if (corner->y > bounds.max.y) {
                bounds.max.y = corner->y;
            }
            if (corner->z < bounds.min.z) {
                bounds.min.z = corner->z;
            } else if (corner->z > bounds.max.z) {
                bounds.max.z = corner->z;
            }
        }

        if (g_DiSegmentMaxX <= bounds.min.x) {
            return 1;
        }
        if (g_DiSegmentMinX >= bounds.max.x) {
            return 1;
        }
        if (g_DiSegmentMaxY <= bounds.min.y) {
            return 1;
        }
        if (g_DiSegmentMinY >= bounds.max.y) {
            return 1;
        }
        if (g_DiSegmentMaxZ <= bounds.min.z) {
            return 1;
        }
        if (g_DiSegmentMinZ >= bounds.max.z) {
            return 1;
        }

        if ((node->flags & 0x20) != 0
            && BuildPickCandidatesForSegmentVsBBoxFaces(
                   &corners,
                   g_DiPickCandidateCursor,
                   &g_DiPickQueryPoint,
                   &g_DiSegmentEnd
               ) == 0) {
            return 1;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-di.frustumtestandpick
     * @recoil-artifact defines .text recoil:function:0x4476f0: CZDisplayInstance::FrustumTestAndPick.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FrustumTestAndPick(CZNodePartial * node, int* activeMask)
    {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners;
        CZClass::gwNodeGetViewBBoxCorners(node, &corners);

        zBBox3f bounds;
        const zVec3* vertices = corners.corners;
        bounds.max.x = bounds.min.x = vertices[0].x;
        bounds.max.y = bounds.min.y = vertices[0].y;
        bounds.max.z = bounds.min.z = vertices[0].z;
        for (int bboxCornerIndex = 1; bboxCornerIndex < 8; ++bboxCornerIndex) {
            const zVec3* corner = &vertices[bboxCornerIndex];
            if (corner->x < bounds.min.x) {
                bounds.min.x = corner->x;
            } else if (corner->x > bounds.max.x) {
                bounds.max.x = corner->x;
            }
            if (corner->y < bounds.min.y) {
                bounds.min.y = corner->y;
            } else if (corner->y > bounds.max.y) {
                bounds.max.y = corner->y;
            }
            if (corner->z < bounds.min.z) {
                bounds.min.z = corner->z;
            } else if (corner->z > bounds.max.z) {
                bounds.max.z = corner->z;
            }
        }

        int anyActive = 0;
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            if (activeMask[i] == 0) {
                continue;
            }

            if (g_DiSegmentBounds[i].maxX <= bounds.min.x) {
                activeMask[i] = 0;
            } else if (g_DiSegmentBounds[i].minX >= bounds.max.x) {
                activeMask[i] = 0;
            } else if (g_DiSegmentBounds[i].maxY <= bounds.min.y) {
                activeMask[i] = 0;
            } else if (g_DiSegmentBounds[i].minY >= bounds.max.y) {
                activeMask[i] = 0;
            } else if (g_DiSegmentBounds[i].maxZ <= bounds.min.z) {
                activeMask[i] = 0;
            } else if (g_DiSegmentBounds[i].minZ >= bounds.max.z) {
                activeMask[i] = 0;
            } else {
                anyActive = 1;
            }
        }

        if (anyActive != 0 && (node->flags & kNodeFlagPointCandidate) != 0) {
            anyActive = FilterRegionsAgainstPolygonWithDamageMaskUv(
                node,
                g_DiPickCandidateBuffer,
                (CZDisplayInstanceSegmentEndpoints*)((void*)(g_DiPickPointArray)),
                activeMask,
                g_DiPickPointCount,
                &corners
            );
        }

        return anyActive == 0 ? 1 : 0;
    }
}

namespace CZDisplayInstance
{

    /*
     * Provenance-only routing note: CZDisplayInstance::BuildPickCandidatesForSegmentVsBBoxFaces.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::FilterRegionsAgainstPolygonWithDamageMaskUv.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::FilterRegionsAgainstPolygon.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::BuildPickCandidatesForSegmentBatchVsPolygon.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::TryGetPolygonHitAtQueryXZ.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::BuildPickCandidateForSegmentVsPolygonWithUv.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::AppendPickCandidatesForFace.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */
}

namespace zDi
{
    /*
     * Provenance-only routing note: zDi::BuildPickCandidateForQueryPoint.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */
}

namespace zModelConst
{
    /*
     * Provenance-only routing note: zModelConst::AddFaceToPlayerProbeSampleBuckets.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */
}

namespace CZDisplayInstance
{

    /*
     * Provenance-only routing note: CZDisplayInstance::PickTestMeshAtQueryXZ.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::FilterRegionsAgainstMeshFaces.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */

    /*
     * Provenance-only routing note: CZDisplayInstance::FilterRegionsAgainstHexahedronFaces.
     * The definition now lives in the literal-backed gmod_const.c contribution.
     */
}
/*
 * Provenance-only routing marker: this definition compiles through the
 * literal-backed Battlesport/player.cpp contribution.
 */
