#include "zDi.h"

#include "Battlesport/player.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"

#include <math.h>
#include <malloc.h>
#include <string.h>

int g_cls_di_BreakOnFirstCandidate = 0;
int g_cls_di_StopAfterFirstHit = 0;
zVec3 g_zClass_DiFaceVertexScratch4[4] = {0};
zVec3 g_DiPickQueryPoint = {0};
zVec3 g_DiSegmentEnd = {0};
float g_DiSegmentMinX = 0.0f;
float g_DiSegmentMinY = 0.0f;
float g_DiSegmentMinZ = 0.0f;
float g_DiSegmentMaxX = 0.0f;
float g_DiSegmentMaxY = 0.0f;
float g_DiSegmentMaxZ = 0.0f;
zClass_DiSegmentBounds g_DiSegmentBounds[24] = {0};
zVec3 *g_DiPickPointArray = 0;
int g_DiPickPointCount = 0;
float g_DiPickPointQueryMaxY = 0.0f;
PlayerProbeSampleCandidateBuffer *g_DiPickCandidateBuffer = 0;
zClassDiPickCandidateEntry *g_DiPickCandidateCursor = 0;

namespace {
    const char *kClsDiSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c";
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
    const int kNodeFlagUseLocalMatrixMode3 = 0x80000;
    const int kNodeFlagClearDuringPick = 0x02000000;
    const int kObjectFlagTransformDirty = 0x01;
    const int kObjectFlagNoPickMatrixPush = 0x08;
    const int kObjectFlagUseCachedWorldMatrix = 0x20;
    const int kMaxPickCandidates = 0x20;
    const double kPickEdgeInsideEpsilon = -0.0001;
    const unsigned short kPickFaceBatchDamageMaskUvFlag = 0x0100;
    const unsigned short kPickFaceTexturedDamageMaskFlag = 0x0200;

    float AbsFloat(float value) {
        return value < 0.0f ? -value : value;
    }

    unsigned int FloatBits(float value) {
        union {
            float f;
            unsigned int u;
        } bits = {value};
        return bits.u;
    }

    float Dot3(const zVec3 *a, const zVec3 *b) {
        return a->x * b->x + a->y * b->y + a->z * b->z;
    }

    zVec3 Delta3(const zVec3 *a, const zVec3 *b) {
        zVec3 result = {a->x - b->x, a->y - b->y, a->z - b->z};
        return result;
    }

    double ProjectedEdgeCross(const zVec3 *edgeStart, const zVec3 *edgeEnd, const zVec3 *point,
                              int axis) {
        switch (axis) {
        case 0:
            return (edgeEnd->y - edgeStart->y) * (point->z - edgeStart->z) -
                   (edgeEnd->z - edgeStart->z) * (point->y - edgeStart->y);
        case 1:
            return (edgeEnd->x - edgeStart->x) * (point->z - edgeStart->z) -
                   (edgeEnd->z - edgeStart->z) * (point->x - edgeStart->x);
        default:
            return (edgeEnd->x - edgeStart->x) * (point->y - edgeStart->y) -
                   (edgeEnd->y - edgeStart->y) * (point->x - edgeStart->x);
        }
    }

    int DominantAxis(const zVec3 *normal) {
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

    float DominantAxisComponent(const zVec3 *normal, int axis) {
        if (axis == 0) {
            return normal->x;
        }
        if (axis == 1) {
            return normal->y;
        }
        return normal->z;
    }

    int ProjectedWindingSign(const zVec3 *normal, int axis) {
        const int componentIsNegative = DominantAxisComponent(normal, axis) < 0.0f ? 1 : 0;
        if (axis == 1) {
            return componentIsNegative != 0 ? 1 : -1;
        }
        return componentIsNegative != 0 ? -1 : 1;
    }

    bool PointInProjectedPolygon(const zVec3 *polygonVertices, int vertexCount,
                                 const zVec3 *point, const zVec3 *normal) {
        const int axis = DominantAxis(normal);
        const int windingSign = ProjectedWindingSign(normal, axis);

        {
        for (int edgeIndex = vertexCount - 1; edgeIndex >= 0; --edgeIndex) {
            const zVec3 *edgeStart = &polygonVertices[edgeIndex];
            const zVec3 *edgeEnd = &polygonVertices[(edgeIndex + 1) % vertexCount];
            const double edgeValue = static_cast<double>(windingSign) *
                                     ProjectedEdgeCross(edgeStart, edgeEnd, point, axis);
            if (edgeValue <= kPickEdgeInsideEpsilon) {
                return false;
            }
        }
        }

        return true;
    }

    bool BuildPickCandidateForSegmentVsPolygonCore(
        zClassDiPickCandidateEntry * candidate, const zVec3 *segmentStart, const zVec3 *segmentEnd,
        const zVec3 *polygonVertices, int vertexCount, int cullBackface,
        int *outDominantAxis) {
        zMath_Vec3_TriangleNormal(&polygonVertices[0], &polygonVertices[1], &polygonVertices[2],
                                  &candidate->surfaceNormal);

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

        return PointInProjectedPolygon(polygonVertices, vertexCount, &candidate->hitPos,
                                       &candidate->surfaceNormal);
    }

    bool BuildBatchSegmentPlaneHit(zClassDiPickCandidateEntry *candidate,
                                   const zClass_DiSegmentEndpoints *segment,
                                   const zVec3 *polygonVertices, const zVec3 *normal,
                                   int cullBackface) {
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

    void AppendBatchPolygonCandidate(zClass_NodePartial *candidateOwner,
                                     PlayerProbeSampleCandidateBuffer *buffer,
                                     const zVec3 *normal,
                                     const zModel_PickFaceEntry *faceEntry) {
        if (buffer->candidateCount >= kMaxPickCandidates) {
            return;
        }

        zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
        entry->surfaceNormal = *normal;
        entry->node = candidateOwner;
        entry->scenePayload = faceEntry->scenePayload;
        ++buffer->candidateCount;
    }

    void SolvePickCandidateUvForProjectedPlane(
        const zClassDiPickCandidateEntry *candidate, const zVec3 *polygonVertices,
        const zModel_PickFaceUvData *faceUvData, zVec2 *outUv, int dominantAxis) {
        float uGrad0;
        float uGrad1;
        float vGrad0;
        float vGrad1;

        if (dominantAxis == 0) {
            zMath_SolveLinearGradient2D(
                &uGrad0, &uGrad1, polygonVertices[0].y, polygonVertices[0].z, polygonVertices[1].y,
                polygonVertices[1].z, polygonVertices[2].y, polygonVertices[2].z,
                faceUvData->uvs[0].x, faceUvData->uvs[1].x, faceUvData->uvs[2].x);
            zMath_SolveLinearGradient2D(
                &vGrad0, &vGrad1, polygonVertices[0].y, polygonVertices[0].z, polygonVertices[1].y,
                polygonVertices[1].z, polygonVertices[2].y, polygonVertices[2].z,
                faceUvData->uvs[0].y, faceUvData->uvs[1].y, faceUvData->uvs[2].y);

            outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad0 +
                       (candidate->hitPos.z - polygonVertices[0].z) * uGrad1 + faceUvData->uvs[0].x;
            outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad0 +
                       (candidate->hitPos.z - polygonVertices[0].z) * vGrad1 + faceUvData->uvs[0].y;
            return;
        }

        if (dominantAxis == 1) {
            zMath_SolveLinearGradient2D(
                &uGrad0, &uGrad1, polygonVertices[0].x, polygonVertices[0].z, polygonVertices[1].x,
                polygonVertices[1].z, polygonVertices[2].x, polygonVertices[2].z,
                faceUvData->uvs[0].x, faceUvData->uvs[1].x, faceUvData->uvs[2].x);
            zMath_SolveLinearGradient2D(
                &vGrad0, &vGrad1, polygonVertices[0].x, polygonVertices[0].z, polygonVertices[1].x,
                polygonVertices[1].z, polygonVertices[2].x, polygonVertices[2].z,
                faceUvData->uvs[0].y, faceUvData->uvs[1].y, faceUvData->uvs[2].y);

            outUv->x = (candidate->hitPos.z - polygonVertices[0].z) * uGrad1 +
                       (candidate->hitPos.x - polygonVertices[0].x) * uGrad0 + faceUvData->uvs[0].x;
            outUv->y = (candidate->hitPos.z - polygonVertices[0].z) * vGrad1 +
                       (candidate->hitPos.x - polygonVertices[0].x) * vGrad0 + faceUvData->uvs[0].y;
            return;
        }

        zMath_SolveLinearGradient2D(
            &uGrad0, &uGrad1, polygonVertices[0].x, polygonVertices[0].y, polygonVertices[1].x,
            polygonVertices[1].y, polygonVertices[2].x, polygonVertices[2].y, faceUvData->uvs[0].x,
            faceUvData->uvs[1].x, faceUvData->uvs[2].x);
        zMath_SolveLinearGradient2D(
            &vGrad0, &vGrad1, polygonVertices[0].x, polygonVertices[0].y, polygonVertices[1].x,
            polygonVertices[1].y, polygonVertices[2].x, polygonVertices[2].y, faceUvData->uvs[0].y,
            faceUvData->uvs[1].y, faceUvData->uvs[2].y);

        outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad1 +
                   (candidate->hitPos.x - polygonVertices[0].x) * uGrad0 + faceUvData->uvs[0].x;
        outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad1 +
                   (candidate->hitPos.x - polygonVertices[0].x) * vGrad0 + faceUvData->uvs[0].y;
    }

    const zMat4x3 *CurrentMatrix() {
        return (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    }

    zVec3 TransformWorldPointToModel(const zVec3 *point) {
        const zMat4x3 *matrix = CurrentMatrix();
        const float x = point->x - matrix->posX;
        const float y = point->y - matrix->posY;
        const float z = point->z - matrix->posZ;

        zVec3 result = {x * matrix->xx + y * matrix->xy + z * matrix->xz,
                x * matrix->yx + y * matrix->yy + z * matrix->yz,
                x * matrix->zx + y * matrix->zy + z * matrix->zz};
        return result;
    }

    zVec3 TransformModelPointToWorld(const zVec3 *point) {
        const zMat4x3 *matrix = CurrentMatrix();

        zVec3 result = {
            point->x * matrix->xx + point->y * matrix->yx + point->z * matrix->zx + matrix->posX,
            point->x * matrix->xy + point->y * matrix->yy + point->z * matrix->zy + matrix->posY,
            point->x * matrix->xz + point->y * matrix->yz + point->z * matrix->zz + matrix->posZ};
        return result;
    }

    zVec3 TransformModelVectorToWorld(const zVec3 *vec) {
        const zMat4x3 *matrix = CurrentMatrix();

        zVec3 result = {vec->x * matrix->xx + vec->y * matrix->yx + vec->z * matrix->zx,
                vec->x * matrix->xy + vec->y * matrix->yy + vec->z * matrix->zy,
                vec->x * matrix->xz + vec->y * matrix->yz + vec->z * matrix->zz};
        return result;
    }

    void CopyFaceVerticesToScratch(const zVec3 *vertices, const int *vertexIndices,
                                   unsigned int vertexCount) {
        for (unsigned int i = 0; i < vertexCount; ++i) {
            g_zClass_DiFaceVertexScratch4[i] = vertices[vertexIndices[i]];
        }
    }

    zModel_PickFaceData *NodePickFaceData(zClass_NodePartial * node) {
        return (zModel_PickFaceData *)(
            static_cast<unsigned int>(node->userDataOrDiRef));
    }

    void AppendCurrentCandidateNode(zClass_NodePartial * node) {
        g_DiPickCandidateCursor->node = node;
        ++g_DiPickCandidateCursor;
        ++g_DiPickCandidateBuffer->candidateCount;
    }

    bool BreakOnFirstCandidateHit() {
        return g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0;
    }

    int NoCandidatesReturn() {
        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    float MinFloat(float a, float b) {
        return a < b ? a : b;
    }

    float MaxFloat(float a, float b) {
        return a > b ? a : b;
    }

    int RayGridStep(float delta) {
        if (delta > 0.0f) {
            return 1;
        }
        if (delta < 0.0f) {
            return -1;
        }
        return 0;
    }

    void AppendNodeFaceCandidateIfHit(zClass_NodePartial * node) {
        zModel_PickFaceData *faceData = NodePickFaceData(node);
        if (faceData != 0 &&
            zClass_cls_di::AppendPickCandidatesForFace(faceData, g_DiPickCandidateCursor,
                                                       &g_DiPickQueryPoint, &g_DiSegmentEnd) != 0) {
            AppendCurrentCandidateNode(node);
        }
    }

    void OffsetActiveRayPacket(float offsetX, float offsetZ) {
        g_DiPickQueryPoint.x += offsetX;
        g_DiPickQueryPoint.z += offsetZ;
        g_DiSegmentEnd.x += offsetX;
        g_DiSegmentEnd.z += offsetZ;
        g_DiSegmentMinX += offsetX;
        g_DiSegmentMaxX += offsetX;
        g_DiSegmentMinZ += offsetZ;
        g_DiSegmentMaxZ += offsetZ;
    }

    void OffsetCandidatesFromCell(PlayerProbeSampleCandidateBuffer * rayData,
                                  int firstCandidate, float offsetX, float offsetZ) {
        for (int i = firstCandidate; i < rayData->candidateCount; ++i) {
            rayData->entries[i].hitPos.x -= offsetX;
            rayData->entries[i].hitPos.z -= offsetZ;
        }
    }

    void ProcessWorldAreaPickCell(zWorldAreaPartial * area, int nodeCountHint) {
        for (int i = 0; i < area->childCount; ++i) {
            zClass_NodePartial *node = area->childList[i];
            const int flags = node->flags;
            if ((flags & kNodeFlagEnabledForPick) != 0 && (flags & kNodeFlagRaycastable) != 0) {
                zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(node, nodeCountHint);
            }

            if (BreakOnFirstCandidateHit()) {
                break;
            }
        }
    }

    void RecurseListBChildren(zClass_NodePartial * node, bool requireEnabledRaycastFlags) {
        {
        for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
            zClass_NodePartial *child = node->listB[childIndex];
            if (!requireEnabledRaycastFlags || ((child->flags & kNodeFlagEnabledForPick) != 0 &&
                                                (child->flags & kNodeFlagRaycastable) != 0)) {
                zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(child, node->listCountB);
            }

            if (BreakOnFirstCandidateHit()) {
                break;
            }
        }
        }
    }

    void ComputeBBoxExtents(const zBBoxCorners *corners, float *outMinX, float *outMaxX,
                            float *outMinY, float *outMaxY, float *outMinZ, float *outMaxZ) {
        const float *values = corners->values;
        *outMinX = values[0];
        *outMaxX = values[0];
        *outMinY = values[1];
        *outMaxY = values[1];
        *outMinZ = values[2];
        *outMaxZ = values[2];

        for (int i = 1; i < 8; ++i) {
            const float *corner = &values[i * 3];
            if (corner[0] < *outMinX) {
                *outMinX = corner[0];
            } else if (corner[0] > *outMaxX) {
                *outMaxX = corner[0];
            }
            if (corner[1] < *outMinY) {
                *outMinY = corner[1];
            } else if (corner[1] > *outMaxY) {
                *outMaxY = corner[1];
            }
            if (corner[2] < *outMinZ) {
                *outMinZ = corner[2];
            } else if (corner[2] > *outMaxZ) {
                *outMaxZ = corner[2];
            }
        }
    }

    void CopyBBoxCornerToScratch(const zBBoxCorners *bboxCorners, int sourceCorner,
                                 int scratchCorner) {
        const float *src = &bboxCorners->values[sourceCorner * 3];
        zVec3 *dst = &g_zClass_DiFaceVertexScratch4[scratchCorner];
        dst->x = src[0];
        dst->y = src[1];
        dst->z = src[2];
    }

    bool TestBBoxFace(zClassDiPickCandidateEntry * candidate, const zVec3 *segmentStart,
                      const zVec3 *segmentEnd, int corner0, int corner1,
                      int corner2, int corner3, const zBBoxCorners *bboxCorners) {
        CopyBBoxCornerToScratch(bboxCorners, corner0, 0);
        CopyBBoxCornerToScratch(bboxCorners, corner1, 1);
        CopyBBoxCornerToScratch(bboxCorners, corner2, 2);
        CopyBBoxCornerToScratch(bboxCorners, corner3, 3);
        return zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(
                   candidate, segmentStart, segmentEnd, g_zClass_DiFaceVertexScratch4, 4, 0) != 0;
    }

    int TestSegmentBatchBBoxFace(zClass_NodePartial *candidateOwner,
                                 PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment,
                                 zClass_DiSegmentEndpoints *segmentEndpointsByBatch,
                                 int *activeMask, int segmentCount,
                                 const zBBoxCorners *bboxCorners,
                                 zModel_PickFaceEntry *faceEntry, int corner0,
                                 int corner1, int corner2, int corner3) {
        CopyBBoxCornerToScratch(bboxCorners, corner0, 0);
        CopyBBoxCornerToScratch(bboxCorners, corner1, 1);
        CopyBBoxCornerToScratch(bboxCorners, corner2, 2);
        CopyBBoxCornerToScratch(bboxCorners, corner3, 3);
        return zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon(
            candidateOwner, outCandidateBuffersBySegment, segmentEndpointsByBatch, activeMask,
            segmentCount, g_zClass_DiFaceVertexScratch4, faceEntry);
    }

    zClass_DiSegmentEndpoints *SegmentEndpointBatchFromPickPointArray() {
        return static_cast<zClass_DiSegmentEndpoints *>(static_cast<void *>(g_DiPickPointArray));
    }

    bool SegmentBoundsOverlapBox(const zClass_DiSegmentBounds *bounds, float minX, float maxX,
                                 float minY, float maxY, float minZ, float maxZ) {
        return bounds->maxX > minX && bounds->minX < maxX && bounds->maxY > minY &&
               bounds->minY < maxY && bounds->maxZ > minZ && bounds->minZ < maxZ;
    }

    void CopySegmentActiveMask(int *dst, const int *src) {
        memcpy(dst, src, static_cast<size_t>(g_DiPickPointCount) * sizeof(int));
    }

    int GridCoordFromWorld(float value, float origin, float invCellSize) {
        return static_cast<int>(floor((value - origin) * invCellSize));
    }

    int ClampGridCoord(int coord, int count) {
        if (coord < 0) {
            return 0;
        }
        if (coord >= count) {
            return count - 1;
        }
        return coord;
    }

    void OffsetSegmentBatchXZ(float offsetX, float offsetZ) {
        zClass_DiSegmentEndpoints *segments = SegmentEndpointBatchFromPickPointArray();
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

    void SaveSegmentCandidateCounts(int *candidateCounts) {
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            candidateCounts[i] = g_DiPickCandidateBuffer[i].candidateCount;
        }
    }

    void RestoreClampedSegmentCandidatePositions(const int *firstNewCandidate,
                                                 float offsetX, float offsetZ) {
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            PlayerProbeSampleCandidateBuffer *buffer = &g_DiPickCandidateBuffer[i];
            for (int j = firstNewCandidate[i]; j < buffer->candidateCount; ++j) {
                buffer->entries[j].hitPos.x -= offsetX;
                buffer->entries[j].hitPos.z -= offsetZ;
            }
        }
    }

    void BuildSegmentBoundsFromEndpoints(const zClass_DiSegmentEndpoints *segments,
                                         zClass_DiSegmentBounds *bounds) {
        bounds->minX = segments->start.x < segments->end.x ? segments->start.x : segments->end.x;
        bounds->maxX = segments->start.x < segments->end.x ? segments->end.x : segments->start.x;
        bounds->minY = segments->start.y < segments->end.y ? segments->start.y : segments->end.y;
        bounds->maxY = segments->start.y < segments->end.y ? segments->end.y : segments->start.y;
        bounds->minZ = segments->start.z < segments->end.z ? segments->start.z : segments->end.z;
        bounds->maxZ = segments->start.z < segments->end.z ? segments->end.z : segments->start.z;
    }

    bool SegmentBoundsOverlapWorldXZ(const zClass_DiSegmentBounds *bounds,
                                     const zClass_WorldDataPartial *worldData) {
        return bounds->minX < worldData->worldMaxX && bounds->maxX >= worldData->originX &&
               bounds->minZ <= worldData->originZ && bounds->maxZ > worldData->worldMaxZ;
    }

    zDiPartial *NodeDiRef(zClass_NodePartial * node);

    void FilterCurrentSegmentRegions(zClass_NodePartial *node, int *activeMask) {
        zModel_PickFaceData *faceData =
            static_cast<zModel_PickFaceData *>(static_cast<void *>(NodeDiRef(node)));
        if (faceData != 0) {
            zClass_cls_di::FilterRegionsAgainstPolygon(
                node, faceData, SegmentEndpointBatchFromPickPointArray(), activeMask,
                g_DiPickPointCount, g_DiPickCandidateBuffer);
        }
    }

    void RecurseSegmentBatchChildren(zClass_NodePartial *node, int *activeMask,
                                     bool requirePickFlags) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if (!requirePickFlags || ((child->flags & kNodeFlagEnabledForPick) != 0 &&
                                      (child->flags & kNodeFlagRaycastable) != 0)) {
                zClass_cls_di::BuildPickCandidatesForSegmentsRecursive(child, node->listCountB,
                                                                       activeMask);
            }

            if (BreakOnFirstCandidateHit()) {
                break;
            }
        }
    }

    zDiPartial *NodeDiRef(zClass_NodePartial * node) {
        return (zDiPartial *)(static_cast<unsigned int>(node->userDataOrDiRef));
    }

    bool NodePassesQueryVariant(zClass_NodePartial * node) {
        return (node->flags & 0x01000000) == 0 || VariantTag::CurrentAllowsId(node->nodeType) != 0;
    }

    bool NodePassesQueryFlags(zClass_NodePartial * node) {
        return (node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & 0x08) != 0;
    }

    void AppendQueryPointCandidateIfHit(zClass_NodePartial * node) {
        zDiPartial *di = NodeDiRef(node);
        if (di == 0) {
            return;
        }

        PlayerProbeSampleCandidateBuffer *buffer = g_DiPickCandidateBuffer;
        zClassDiPickCandidateEntry *outCandidate = &buffer->entries[buffer->candidateCount];
        if (zDi::BuildPickCandidateForQueryPoint(di, outCandidate, &g_DiPickQueryPoint) != 0) {
            AppendCurrentCandidateNode(node);
        }
    }

    void RecurseQueryPointChildren(zClass_NodePartial * node, int cullCount,
                                   bool requireQueryFlags) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if (!requireQueryFlags || NodePassesQueryFlags(child)) {
                zClass_cls_di::BuildPickCandidateList(child, cullCount);
            }
        }
    }

    void RecursePointBatchChildren(zClass_NodePartial * node, int depth, int *hitFlags,
                                   bool requireQueryFlags) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if (!requireQueryFlags || NodePassesQueryFlags(child)) {
                zClass_cls_di::BuildPickCandidatesForPoints(child, depth, hitFlags);
            }
        }
    }

    void TransformVerticesToSharedScratch(const zVec3 *vertices, int vertexCount) {
        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            memcpy(g_zModel_SharedVec3ScratchB, vertices,
                        static_cast<size_t>(vertexCount) * sizeof(zVec3));
            return;
        }

        for (int i = 0; i < vertexCount; ++i) {
            g_zModel_SharedVec3ScratchB[i] = TransformModelPointToWorld(&vertices[i]);
        }
    }
}

namespace zClass_cls_di {
    // Reimplements 0x443c50: zClass_cls_di::SetBreakOnFirstCandidate (GameZRecoil/zClass/cls_di.c)
    void RECOIL_FASTCALL SetBreakOnFirstCandidate(int enabled) {
        g_cls_di_BreakOnFirstCandidate = enabled;
    }

    // Reimplements 0x443c60: zClass_cls_di::SetStopAfterFirstHit (GameZRecoil/zClass/cls_di.c)
    void RECOIL_FASTCALL SetStopAfterFirstHit(int flag) {
        g_cls_di_StopAfterFirstHit = flag;
    }

    // Reimplements 0x443c70: zClass_cls_di::FindBestPickCandidateBelowPoint
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL FindBestPickCandidateBelowPoint(
        zClass_NodePartial * world, const zVec3 *position,
        PlayerProbeSampleCandidateBuffer *outResults) {
        if (BuildPickCandidateListBelowPoint(world, outResults, position->x, position->y,
                                             position->z) != 0) {
            outResults->candidateCount = 0;
            zTag4::Clear(&outResults->entries[0].variantTag);
            return;
        }

        zClassDiPickCandidateEntry *best = &outResults->entries[0];
        for (int i = 1; i < outResults->candidateCount; ++i) {
            zClassDiPickCandidateEntry *candidate = &outResults->entries[i];
            if (candidate->hitPos.y > position->y) {
                continue;
            }

            if (best->hitPos.y > position->y || candidate->hitPos.y > best->hitPos.y ||
                (candidate->hitPos.y == best->hitPos.y && best->variantTag.count == 0)) {
                best = candidate;
            }
        }

        outResults->entries[0] = *best;
        outResults->candidateCount = 1;
    }

    // Reimplements 0x443d20: zClass_cls_di::BuildPickCandidateListBelowPoint
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateListBelowPoint(
        zClass_NodePartial * world, PlayerProbeSampleCandidateBuffer * outResults, float x,
        float maxY, float z) {
        if (zClass_TypeList::Head(0) != 0) {
            zClass_TypeList::UpdateQueuedTrees();
        }

        g_DiPickQueryPoint.x = x;
        g_DiPickQueryPoint.y = maxY;
        g_DiPickQueryPoint.z = z;
        g_DiPickCandidateBuffer = outResults;
        g_DiPickCandidateCursor = outResults->entries;
        outResults->candidateCount = 0;

        zClass_WorldDataPartial *worldData =
            static_cast<zClass_WorldDataPartial *>(world->classData);

        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)(&slotBuffer));
        zMath::MatLoadIdentity();

        const int gridCol = static_cast<int>(
            floor((x - worldData->originX) * worldData->areaInvSizeX));
        const int gridRow = static_cast<int>(
            floor((z - worldData->originZ) * worldData->areaInvSizeZ));

        bool visitGridCell = true;
        bool usedClampedCell = false;
        int cellCol = gridCol;
        int cellRow = gridRow;
        float offsetX = 0.0f;
        float offsetZ = 0.0f;

        const bool insideGrid = gridCol >= 0 && gridCol < worldData->areaGridColCount &&
                                gridRow >= 0 && gridRow < worldData->areaGridRowCount;
        if (!insideGrid) {
            if (worldData->clampQueriesToBounds == 0) {
                visitGridCell = false;
            } else {
                usedClampedCell = true;
                if (cellCol < 0) {
                    cellCol = 0;
                } else if (cellCol >= worldData->areaGridColCount) {
                    cellCol = worldData->areaGridColCount - 1;
                }

                if (cellRow < 0) {
                    cellRow = 0;
                } else if (cellRow >= worldData->areaGridRowCount) {
                    cellRow = worldData->areaGridRowCount - 1;
                }

                offsetX = static_cast<float>(cellCol - gridCol) * worldData->areaCellSizeX;
                offsetZ = static_cast<float>(cellRow - gridRow) * worldData->areaCellSizeZ;
            }
        }

        if (visitGridCell) {
            if (usedClampedCell) {
                g_DiPickQueryPoint.x += offsetX;
                g_DiPickQueryPoint.z += offsetZ;
            }

            zWorldAreaPartial *area = &worldData->areaGridRows[cellRow][cellCol];
            for (int i = 0; i < area->childCount; ++i) {
                zClass_NodePartial *node = area->childList[i];
                if (NodePassesQueryFlags(node) && NodePassesQueryVariant(node)) {
                    BuildPickCandidateList(node, area->childCount + 1);
                }
            }

            if (usedClampedCell) {
                g_DiPickQueryPoint.x -= offsetX;
                g_DiPickQueryPoint.z -= offsetZ;
            }
        }

        for (int i = 0; i < world->listCountB; ++i) {
            zClass_NodePartial *node = world->listB[i];
            if (NodePassesQueryFlags(node) && NodePassesQueryVariant(node)) {
                BuildPickCandidateList(node, world->listCountB + 1);
            }
        }

        zMath::MatStackPopPtr();
        return outResults->candidateCount <= 0 ? 1 : 0;
    }

    // Reimplements 0x42ba50: zClass_cls_di::SnapProbePointYToBestCandidate
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SnapProbePointYToBestCandidate(zVec3 *point) {
        PlayerProbeSampleCandidateBuffer candidateBuffer;
        const int result =
            BuildPickCandidateListBelowPoint(g_Player_RuntimeDiScene, &candidateBuffer, point->x,
                                             500.0f, point->z);
        int selectedImpactSlot;
        int bestCandidateIndex;
        float taggedHeight;
        point->y = Player::SelectProbeSampleHeightFromCandidates(
            &candidateBuffer, &bestCandidateIndex, point->y, 2.0f, 0, &selectedImpactSlot,
            &taggedHeight);
        return result;
    }

    // Reimplements 0x444890: zClass_cls_di::BuildPickCandidatesForPoints
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForPoints(
        zClass_NodePartial * node, int depth, int *hitFlags) {
        int nodeFlags = node->flags;
        if ((nodeFlags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((nodeFlags & 0x08) == 0) {
            return 1;
        }
        if ((nodeFlags & 0x01000000) != 0 && VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        const int classId = node->classId;
        nodeFlags &= ~kNodeFlagClearDuringPick;
        node->flags = nodeFlags;

        int sampleMask[24];
        switch (classId) {
        case kNodeClassCamera: {
            memcpy(sampleMask, hitFlags,
                   static_cast<size_t>(g_DiPickPointCount) * sizeof(int));

            zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            zClass_CameraDataPartial *cameraData =
                static_cast<zClass_CameraDataPartial *>(node->classData);
            int pushedMatrix = 0;
            if ((nodeFlags & kNodeFlagEnabledForPick) != 0) {
                pushedMatrix = 1;
                zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
                zMath::MatApplyLocalTRS(&cameraData->targetOrEuler, &cameraData->posOffset,
                                        &unitScale);
            }

            zModel_PickFaceData *faceData =
                static_cast<zModel_PickFaceData *>(static_cast<void *>(NodeDiRef(node)));
            if (faceData != 0) {
                PickTestMeshAtQueryXZ(node, faceData, g_DiPickPointArray, sampleMask,
                                      g_DiPickPointCount, g_DiPickPointQueryMaxY,
                                      g_DiPickCandidateBuffer);
            }

            RecursePointBatchChildren(node, node->listCountB, sampleMask, true);

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassObject3D: {
            memcpy(sampleMask, hitFlags,
                   static_cast<size_t>(g_DiPickPointCount) * sizeof(int));
            if (depth > 1) {
                const int bboxResult = PickTestBBox2D(node, sampleMask);
                if (bboxResult != 0) {
                    return bboxResult;
                }
            }

            zClass_Object3DDataPartial *objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            int pushedMatrix = 0;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) == 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix), 1);
                } else if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix), 1);
                    if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                        objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                    }
                } else {
                    zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                }
            }

            zModel_PickFaceData *faceData =
                static_cast<zModel_PickFaceData *>(static_cast<void *>(NodeDiRef(node)));
            if (faceData != 0) {
                PickTestMeshAtQueryXZ(node, faceData, g_DiPickPointArray, sampleMask,
                                      g_DiPickPointCount, g_DiPickPointQueryMaxY,
                                      g_DiPickCandidateBuffer);
            }

            RecursePointBatchChildren(node, node->listCountB, sampleMask, true);

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassLod: {
            zClass_LodDataPartial *lodData = static_cast<zClass_LodDataPartial *>(node->classData);
            if (lodData->nearRangeSq > 5.0f) {
                return 1;
            }

            memcpy(sampleMask, hitFlags,
                   static_cast<size_t>(g_DiPickPointCount) * sizeof(int));
            if (depth > 1) {
                const int bboxResult = PickTestBBox2D(node, sampleMask);
                if (bboxResult != 0) {
                    return bboxResult;
                }
            }

            RecursePointBatchChildren(node, node->listCountB, sampleMask, false);
            return 0;
        }

        case kNodeClassSequence: {
            zClass_SequenceDataPartial *sequenceData =
                static_cast<zClass_SequenceDataPartial *>(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            memcpy(sampleMask, hitFlags,
                   static_cast<size_t>(g_DiPickPointCount) * sizeof(int));
            if (depth > 1) {
                const int bboxResult = PickTestBBox2D(node, sampleMask);
                if (bboxResult != 0) {
                    return bboxResult;
                }
            }

            return BuildPickCandidatesForPoints(sequenceData->entries[sequenceData->currentIndex].node,
                                                node->listCountB, sampleMask);
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesForPointsRecursive(node, depth, hitFlags);

        case kNodeClassLight:
            return BuildPickCandidatesForPointsForLight(node, depth, hitFlags);

        case kNodeClassSound:
            return 1;

        default:
            zError::ReportOld(0x200, kClsDiSourceFile, 0x587,
                              "Unrecognized node class type:  node = %s class_type = %d", node,
                              classId);
            return 3;
        }
    }

    // Reimplements 0x444c50: zClass_cls_di::BuildPickCandidatesForPointsRecursive
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForPointsRecursive(
        zClass_NodePartial * node, int depth, int *hitFlags) {
        int sampleMask[24];
        memcpy(sampleMask, hitFlags,
               static_cast<size_t>(g_DiPickPointCount) * sizeof(int));

        if (depth > 1) {
            const int bboxResult = PickTestBBox2D(node, sampleMask);
            if (bboxResult != 0) {
                return bboxResult;
            }
        }

        zClass_AnimateDataPartial *animateData =
            static_cast<zClass_AnimateDataPartial *>(node->classData);
        int pushedMatrix = 0;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3 *)(animateData->animatedTransform), 1);
        }

        zModel_PickFaceData *faceData =
            static_cast<zModel_PickFaceData *>(static_cast<void *>(NodeDiRef(node)));
        if (faceData != 0) {
            PickTestMeshAtQueryXZ(node, faceData, g_DiPickPointArray, sampleMask,
                                  g_DiPickPointCount, g_DiPickPointQueryMaxY,
                                  g_DiPickCandidateBuffer);
        }

        RecursePointBatchChildren(node, node->listCountB, sampleMask, false);

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return 0;
    }

    // Reimplements 0x444d10: zClass_cls_di::BuildPickCandidatesForPointsForLight
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForPointsForLight(
        zClass_NodePartial * node, int depth, int *hitFlags) {
        int sampleMask[24];
        memcpy(sampleMask, hitFlags,
               static_cast<size_t>(g_DiPickPointCount) * sizeof(int));

        if (depth > 1) {
            const int bboxResult = PickTestBBox2D(node, sampleMask);
            if (bboxResult != 0) {
                return bboxResult;
            }
        }

        zClass_LightDataPartial *lightData =
            static_cast<zClass_LightDataPartial *>(node->classData);
        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y,
                            lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        zModel_PickFaceData *faceData =
            static_cast<zModel_PickFaceData *>(static_cast<void *>(NodeDiRef(node)));
        if (faceData != 0) {
            PickTestMeshAtQueryXZ(node, faceData, g_DiPickPointArray, sampleMask,
                                  g_DiPickPointCount, g_DiPickPointQueryMaxY,
                                  g_DiPickCandidateBuffer);
        }

        RecursePointBatchChildren(node, node->listCountB, sampleMask, false);

        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x4444b0: zClass_cls_di::BuildPickCandidatesForPointBatch
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForPointBatch(
        zClass_NodePartial * world, zVec3 * pointArray, int pointCount, float queryMaxY,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersByPoint) {
        if (pointCount > 24) {
            zError::ReportOld(0x200, kClsDiSourceFile, 0x495,
                              "More test pnts than space for: %d", pointCount);
            pointCount = 24;
        }

        if (zClass_TypeList::Head(0) != 0) {
            zClass_TypeList::UpdateQueuedTrees();
        }

        zClass_WorldDataPartial *worldData =
            static_cast<zClass_WorldDataPartial *>(world->classData);
        zWorldAreaPartial *gridCellForPoint[24] = {0};
        zWorldAreaPartial *uniqueGridCells[24] = {0};
        int pointActive[24] = {0};
        int pointWasClamped[24] = {0};
        float clampOffsetX[24] = {0.0f};
        float clampOffsetZ[24] = {0.0f};

        for (int i = 0; i < pointCount; ++i) {
            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersByPoint[i];
            buffer->candidateCount = 0;

            const int gridCol = static_cast<int>(
                floor((pointArray[i].x - worldData->originX) * worldData->areaInvSizeX));
            const int gridRow = static_cast<int>(
                floor((pointArray[i].z - worldData->originZ) * worldData->areaInvSizeZ));

            if (gridCol >= 0 && gridCol < worldData->areaGridColCount &&
                gridRow >= 0 && gridRow < worldData->areaGridRowCount) {
                pointActive[i] = 1;
                gridCellForPoint[i] = &worldData->areaGridRows[gridRow][gridCol];
                continue;
            }

            if (worldData->clampQueriesToBounds == 0) {
                continue;
            }

            int clampedCol = gridCol;
            if (clampedCol < 0) {
                clampedCol = 0;
            } else if (clampedCol >= worldData->areaGridColCount) {
                clampedCol = worldData->areaGridColCount - 1;
            }

            int clampedRow = gridRow;
            if (clampedRow < 0) {
                clampedRow = 0;
            } else if (clampedRow >= worldData->areaGridRowCount) {
                clampedRow = worldData->areaGridRowCount - 1;
            }

            pointActive[i] = 1;
            pointWasClamped[i] = 1;
            gridCellForPoint[i] = &worldData->areaGridRows[clampedRow][clampedCol];
            clampOffsetX[i] = static_cast<float>(clampedCol - gridCol) * worldData->areaCellSizeX;
            clampOffsetZ[i] = static_cast<float>(clampedRow - gridRow) * worldData->areaCellSizeZ;
            pointArray[i].x += clampOffsetX[i];
            pointArray[i].z += clampOffsetZ[i];
        }

        int uniqueGridCellCount = 0;
        for (int i = 0; i < pointCount; ++i) {
            zWorldAreaPartial *cell = gridCellForPoint[i];
            if (cell == 0) {
                continue;
            }

            int alreadyAdded = 0;
            for (int j = 0; j < uniqueGridCellCount; ++j) {
                if (uniqueGridCells[j] == cell) {
                    alreadyAdded = 1;
                    break;
                }
            }

            if (alreadyAdded == 0) {
                uniqueGridCells[uniqueGridCellCount++] = cell;
            }
        }

        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)(&slotBuffer));
        zMath::MatLoadIdentity();

        g_DiPickCandidateBuffer = outCandidateBuffersByPoint;
        g_DiPickCandidateCursor = outCandidateBuffersByPoint->entries;
        g_DiPickPointQueryMaxY = queryMaxY;
        g_DiPickPointArray = pointArray;
        g_DiPickPointCount = pointCount;

        for (int cellIndex = 0; cellIndex < uniqueGridCellCount; ++cellIndex) {
            zWorldAreaPartial *cell = uniqueGridCells[cellIndex];
            int hitFlags[24] = {0};
            for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                hitFlags[pointIndex] =
                    pointActive[pointIndex] != 0 && gridCellForPoint[pointIndex] == cell ? 1 : 0;
            }

            for (int childIndex = 0; childIndex < cell->childCount; ++childIndex) {
                zClass_NodePartial *node = cell->childList[childIndex];
                if (NodePassesQueryFlags(node)) {
                    BuildPickCandidatesForPoints(node, cell->childCount + 1, hitFlags);
                }
            }
        }

        for (int i = 0; i < pointCount; ++i) {
            if (pointWasClamped[i] != 0) {
                pointArray[i].x -= clampOffsetX[i];
                pointArray[i].z -= clampOffsetZ[i];
            }
        }

        for (int i = 0; i < world->listCountB; ++i) {
            zClass_NodePartial *node = world->listB[i];
            if (NodePassesQueryFlags(node) && NodePassesQueryVariant(node)) {
                BuildPickCandidatesForPoints(node, world->listCountB + 1, pointActive);
            }
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x443f80: zClass_cls_di::BuildPickCandidateList (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateList(zClass_NodePartial * node,
                                                                        int cullCount) {
        int nodeFlags = node->flags;
        if ((nodeFlags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((nodeFlags & 0x08) == 0) {
            return 1;
        }
        if ((nodeFlags & 0x01000000) != 0 && VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        nodeFlags &= ~kNodeFlagClearDuringPick;
        node->flags = nodeFlags;
        if (g_DiPickCandidateBuffer->candidateCount >= kMaxPickCandidates) {
            zError::ReportOld(0x200, kClsDiSourceFile, 0x26b,
                              "Database intersections array is full");
            return 1;
        }

        switch (node->classId) {
        case kNodeClassCamera: {
            zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            zClass_CameraDataPartial *cameraData =
                static_cast<zClass_CameraDataPartial *>(node->classData);

            int pushedMatrix = 0;
            if ((nodeFlags & kNodeFlagEnabledForPick) != 0) {
                pushedMatrix = 1;
                zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
                zMath::MatApplyLocalTRS(&cameraData->targetOrEuler, &cameraData->posOffset,
                                        &unitScale);
            }

            AppendQueryPointCandidateIfHit(node);
            RecurseQueryPointChildren(node, node->listCountB, true);

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }

            return NoCandidatesReturn();
        }

        case kNodeClassObject3D: {
            if (cullCount > 1 && IsPickQueryPointOutsideViewBBoxXZ(node) != 0) {
                return 1;
            }

            zClass_Object3DDataPartial *objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            int pushedMatrix = 0;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) == 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix),
                                       1);
                } else if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix),
                                       1);
                    if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                        objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                    }
                } else {
                    zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                }
            }

            AppendQueryPointCandidateIfHit(node);
            RecurseQueryPointChildren(node, node->listCountB, true);

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }

            return NoCandidatesReturn();
        }

        case kNodeClassLod: {
            zClass_LodDataPartial *lodData = static_cast<zClass_LodDataPartial *>(node->classData);
            if (lodData->nearRangeSq > 5.0f) {
                return 1;
            }

            if (cullCount > 1 && IsPickQueryPointOutsideViewBBoxXZ(node) != 0) {
                return 1;
            }

            RecurseQueryPointChildren(node, node->listCountB, false);
            return NoCandidatesReturn();
        }

        case kNodeClassSequence: {
            zClass_SequenceDataPartial *sequenceData =
                static_cast<zClass_SequenceDataPartial *>(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            if (cullCount > 1 && IsPickQueryPointOutsideViewBBoxXZ(node) != 0) {
                return 1;
            }

            return BuildPickCandidateList(sequenceData->entries[sequenceData->currentIndex].node,
                                          node->listCountB);
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesRecursive(node, cullCount);

        case kNodeClassLight:
            return BuildPickCandidatesForLight(node, cullCount);

        case kNodeClassSound:
            return 1;

        default:
            zError::ReportOld(0x200, kClsDiSourceFile, 0x295,
                              "Unrecognized node class type:  node = %s class_type = %d", node,
                              node->classId);
            return 3;
        }
    }

    // Reimplements 0x444310: zClass_cls_di::BuildPickCandidatesRecursive
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesRecursive(
        zClass_NodePartial * node, int cullCount) {
        AppendQueryPointCandidateIfHit(node);

        zClass_AnimateDataPartial *animateData =
            static_cast<zClass_AnimateDataPartial *>(node->classData);
        int pushedMatrix = 0;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3 *)(animateData->animatedTransform),
                               1);
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

        RecurseQueryPointChildren(node, node->listCountB, false);

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return NoCandidatesReturn();
    }

    // Reimplements 0x4443e0: zClass_cls_di::BuildPickCandidatesForLight
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForLight(
        zClass_NodePartial * node, int cullCount) {
        if (cullCount > 1) {
            const int result = IsPickQueryPointOutsideViewBBoxXZ(node);
            if (result != 0) {
                return result;
            }
        }

        zClass_LightDataPartial *lightData =
            static_cast<zClass_LightDataPartial *>(node->classData);
        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y,
                            lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        AppendQueryPointCandidateIfHit(node);
        RecurseQueryPointChildren(node, node->listCountB, false);

        zMath::MatStackPopPtr();
        return NoCandidatesReturn();
    }

    // Reimplements 0x4472c0: zClass_cls_di::IsPickQueryPointOutsideViewBBoxXZ
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL IsPickQueryPointOutsideViewBBoxXZ(
        zClass_NodePartial * node) {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners = {0};
        zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);

        float minX;
        float maxX;
        float minY;
        float maxY;
        float minZ;
        float maxZ;
        ComputeBBoxExtents(&corners, &minX, &maxX, &minY, &maxY, &minZ, &maxZ);

        return g_DiPickQueryPoint.x >= minX && g_DiPickQueryPoint.x <= maxX &&
                       g_DiPickQueryPoint.z >= minZ && g_DiPickQueryPoint.z <= maxZ
                   ? 0
                   : 1;
    }

    // Reimplements 0x4473e0: zClass_cls_di::PickTestBBox2D
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL PickTestBBox2D(zClass_NodePartial * node,
                                                                int *hitFlags) {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners = {0};
        zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);

        float minX;
        float maxX;
        float minY;
        float maxY;
        float minZ;
        float maxZ;
        ComputeBBoxExtents(&corners, &minX, &maxX, &minY, &maxY, &minZ, &maxZ);

        int result = 1;
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            if (hitFlags[i] != 0) {
                const zVec3 *point = &g_DiPickPointArray[i];
                if (point->x >= minX && point->x <= maxX &&
                    point->z >= minZ && point->z <= maxZ) {
                    result = 0;
                } else {
                    hitFlags[i] = 0;
                }
            }
        }

        return result;
    }

    // Reimplements 0x447540: zClass_cls_di::FilterPointsBBox (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL FilterPointsBBox(zClass_NodePartial * node,
                                                                  void * /*pointData*/) {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners = {0};
        zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);

        float minX;
        float maxX;
        float minY;
        float maxY;
        float minZ;
        float maxZ;
        ComputeBBoxExtents(&corners, &minX, &maxX, &minY, &maxY, &minZ, &maxZ);

        if (g_DiSegmentMaxX <= minX || g_DiSegmentMinX >= maxX || g_DiSegmentMaxY <= minY ||
            g_DiSegmentMinY >= maxY || g_DiSegmentMaxZ <= minZ || g_DiSegmentMinZ >= maxZ) {
            return 1;
        }

        if ((node->flags & 0x20) != 0 &&
            BuildPickCandidatesForSegmentVsBBoxFaces(&corners, g_DiPickCandidateCursor,
                                                     &g_DiPickQueryPoint, &g_DiSegmentEnd) == 0) {
            return 1;
        }

        return 0;
    }

    // Reimplements 0x4476f0: zClass_cls_di::FrustumTestAndPick
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL FrustumTestAndPick(zClass_NodePartial * node,
                                                           int * activeMask) {
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        zBBoxCorners corners = {0};
        zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);

        float minX;
        float maxX;
        float minY;
        float maxY;
        float minZ;
        float maxZ;
        ComputeBBoxExtents(&corners, &minX, &maxX, &minY, &maxY, &minZ, &maxZ);

        int anyActive = 0;
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            if (activeMask[i] == 0) {
                continue;
            }

            if (SegmentBoundsOverlapBox(&g_DiSegmentBounds[i], minX, maxX, minY, maxY, minZ,
                                        maxZ)) {
                anyActive = 1;
            } else {
                activeMask[i] = 0;
            }
        }

        if (anyActive != 0 && (node->flags & kNodeFlagPointCandidate) != 0) {
            const int bboxHit = FilterRegionsAgainstPolygonWithDamageMaskUv(
                node, g_DiPickCandidateBuffer, SegmentEndpointBatchFromPickPointArray(),
                activeMask, g_DiPickPointCount, &corners);
            return bboxHit == 0 ? 1 : 0;
        }

        return anyActive == 0 ? 1 : 0;
    }

    // Reimplements 0x485380: zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentVsBBoxFaces(
        const zBBoxCorners *bboxCorners, zClassDiPickCandidateEntry *candidate,
        const zVec3 *segmentStart, const zVec3 *segmentEnd) {
        candidate->scenePayload = 0;

        if (TestBBoxFace(candidate, segmentStart, segmentEnd, 0, 4, 7, 3, bboxCorners) ||
            TestBBoxFace(candidate, segmentStart, segmentEnd, 0, 1, 5, 4, bboxCorners) ||
            TestBBoxFace(candidate, segmentStart, segmentEnd, 5, 1, 2, 6, bboxCorners) ||
            TestBBoxFace(candidate, segmentStart, segmentEnd, 7, 6, 2, 3, bboxCorners) ||
            TestBBoxFace(candidate, segmentStart, segmentEnd, 0, 3, 2, 1, bboxCorners) ||
            TestBBoxFace(candidate, segmentStart, segmentEnd, 4, 5, 6, 7, bboxCorners)) {
            return 1;
        }

        return 0;
    }

    // Reimplements 0x487540: zClass_cls_di::FilterRegionsAgainstPolygonWithDamageMaskUv
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstPolygonWithDamageMaskUv(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch, int * activeMask, int segmentCount,
        const zBBoxCorners * bboxCorners) {
        zModel_PickFaceEntry faceEntry;
        memset(&faceEntry, 0, sizeof(faceEntry));
        faceEntry.flagsAndVertexCount = 4;

        int result = 0;
        if (TestSegmentBatchBBoxFace(candidateOwner, outCandidateBuffersBySegment,
                                     segmentEndpointsByBatch, activeMask, segmentCount,
                                     bboxCorners, &faceEntry, 0, 4, 7, 3) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(candidateOwner, outCandidateBuffersBySegment,
                                     segmentEndpointsByBatch, activeMask, segmentCount,
                                     bboxCorners, &faceEntry, 0, 1, 5, 4) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(candidateOwner, outCandidateBuffersBySegment,
                                     segmentEndpointsByBatch, activeMask, segmentCount,
                                     bboxCorners, &faceEntry, 1, 2, 6, 5) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(candidateOwner, outCandidateBuffersBySegment,
                                     segmentEndpointsByBatch, activeMask, segmentCount,
                                     bboxCorners, &faceEntry, 2, 3, 7, 6) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(candidateOwner, outCandidateBuffersBySegment,
                                     segmentEndpointsByBatch, activeMask, segmentCount,
                                     bboxCorners, &faceEntry, 0, 3, 2, 1) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(candidateOwner, outCandidateBuffersBySegment,
                                     segmentEndpointsByBatch, activeMask, segmentCount,
                                     bboxCorners, &faceEntry, 4, 5, 6, 7) != 0) {
            result = 1;
        }

        return result;
    }

    // Reimplements 0x487350: zClass_cls_di::FilterRegionsAgainstPolygon
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL FilterRegionsAgainstPolygon(
        zClass_NodePartial * candidateOwner, zModel_PickFaceData * faceData,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch, int * activeMask, int segmentCount,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment) {
        if (faceData == 0 || faceData->faceCount == 0) {
            return;
        }

        const zVec3 *vertices = faceData->baseVertices;
        if ((faceData->flags & 0x08) != 0 && faceData->morphWeight != 0.0f &&
            faceData->morphVertexCount != 0) {
            zMath_Vec3Array_AddScaled(g_zModel_SharedVec3ScratchA, faceData->baseVertices,
                                      faceData->morphVertices, faceData->morphVertexCount,
                                      faceData->morphWeight);
            vertices = g_zModel_SharedVec3ScratchA;
        }

        TransformVerticesToSharedScratch(vertices, faceData->vertexCount);

        zVec2 scratchUv = {0.0f, 0.0f};
        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            zModel_PickFaceEntry *face = &faceData->faces[faceIndex];
            const unsigned int vertexCount = face->flagsAndVertexCount & 0xffu;
            CopyFaceVerticesToScratch(g_zModel_SharedVec3ScratchB, face->vertexIndices,
                                      vertexCount);

            if ((face->scenePayload->flags & kPickFaceBatchDamageMaskUvFlag) != 0) {
                BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
                    candidateOwner, outCandidateBuffersBySegment, segmentEndpointsByBatch,
                    activeMask, segmentCount, g_zClass_DiFaceVertexScratch4, face->faceUvData,
                    &scratchUv, face);
            } else {
                BuildPickCandidatesForSegmentBatchVsPolygon(
                    candidateOwner, outCandidateBuffersBySegment, segmentEndpointsByBatch,
                    activeMask, segmentCount, g_zClass_DiFaceVertexScratch4, face);
            }
        }
    }

    // Reimplements 0x486290: zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentBatchVsPolygon(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch, int * activeMask, int segmentCount,
        zVec3 * polygonVertices, zModel_PickFaceEntry * faceEntry) {
        int *localActive =
            segmentCount > 0 ? static_cast<int *>(_alloca(sizeof(int) * segmentCount)) : 0;
        for (int i = 0; i < segmentCount; ++i) {
            localActive[i] = activeMask[i];
        }

        zVec3 normal;
        zMath_Vec3_TriangleNormal(&polygonVertices[0], &polygonVertices[1], &polygonVertices[2],
                                  &normal);

        const int cullBackface =
            static_cast<int>((faceEntry->flagsAndVertexCount >> 8) & 1u);
        int anyActive = 0;
        for (int i = 0; i < segmentCount; ++i) {
            if (localActive[i] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[i];
            if (buffer->candidateCount >= kMaxPickCandidates ||
                !BuildBatchSegmentPlaneHit(&buffer->entries[buffer->candidateCount],
                                           &segmentEndpointsByBatch[i], polygonVertices, &normal,
                                           cullBackface)) {
                localActive[i] = 0;
                continue;
            }

            anyActive = 1;
        }

        if (anyActive == 0) {
            return 0;
        }

        const int vertexCount = static_cast<int>(faceEntry->flagsAndVertexCount & 0xffu);
        for (int i = 0; i < segmentCount; ++i) {
            if (localActive[i] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[i];
            const zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
            localActive[i] =
                PointInProjectedPolygon(polygonVertices, vertexCount, &entry->hitPos, &normal)
                    ? 1
                    : 0;
        }

        anyActive = 0;
        for (int i = 0; i < segmentCount; ++i) {
            if (localActive[i] != 0) {
                anyActive = 1;
                AppendBatchPolygonCandidate(candidateOwner, &outCandidateBuffersBySegment[i],
                                            &normal, faceEntry);
            }
        }

        return anyActive;
    }

    // Reimplements 0x4869a0:
    // zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch, int * activeMask, int segmentCount,
        zVec3 * polygonVertices, zModel_PickFaceUvData * faceUvData, zVec2 * scratchUv,
        zModel_PickFaceEntry * faceEntry) {
        int *localActive =
            segmentCount > 0 ? static_cast<int *>(_alloca(sizeof(int) * segmentCount)) : 0;
        for (int i = 0; i < segmentCount; ++i) {
            localActive[i] = activeMask[i];
        }

        zVec3 normal;
        zMath_Vec3_TriangleNormal(&polygonVertices[0], &polygonVertices[1], &polygonVertices[2],
                                  &normal);

        const int cullBackface =
            static_cast<int>((faceEntry->flagsAndVertexCount >> 8) & 1u);
        int anyActive = 0;
        for (int i = 0; i < segmentCount; ++i) {
            if (localActive[i] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[i];
            if (buffer->candidateCount >= kMaxPickCandidates ||
                !BuildBatchSegmentPlaneHit(&buffer->entries[buffer->candidateCount],
                                           &segmentEndpointsByBatch[i], polygonVertices, &normal,
                                           cullBackface)) {
                localActive[i] = 0;
                continue;
            }

            anyActive = 1;
        }

        if (anyActive == 0) {
            return 0;
        }

        const int vertexCount = static_cast<int>(faceEntry->flagsAndVertexCount & 0xffu);
        for (int i = 0; i < segmentCount; ++i) {
            if (localActive[i] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[i];
            const zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
            localActive[i] =
                PointInProjectedPolygon(polygonVertices, vertexCount, &entry->hitPos, &normal)
                    ? 1
                    : 0;
        }

        const int damageMaskEnabled = OptCatalog_IsDamageMaskEnabled();
        const int dominantAxis = DominantAxis(&normal);
        anyActive = 0;
        for (int i = 0; i < segmentCount; ++i) {
            if (localActive[i] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[i];
            if (buffer->candidateCount >= kMaxPickCandidates) {
                continue;
            }

            if (damageMaskEnabled != 0) {
                const zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
                zClassDiPickCandidateEntry uvCandidate = {};
                uvCandidate.hitPos = entry->hitPos;
                uvCandidate.surfaceNormal = normal;
                SolvePickCandidateUvForProjectedPlane(&uvCandidate, polygonVertices, faceUvData,
                                                      scratchUv, dominantAxis);
                OptCatalog_SetDamageMaskUv(scratchUv->x, scratchUv->y);
            }

            anyActive = 1;
            AppendBatchPolygonCandidate(candidateOwner, buffer, &normal, faceEntry);
        }

        return anyActive;
    }

    // Reimplements 0x4856d0: zClass_cls_di::TryGetPolygonHitAtQueryXZ (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL TryGetPolygonHitAtQueryXZ(
        zClassDiPickCandidateEntry * candidate, const zVec3 *polygonVertices, float queryX,
        float queryZ, int vertexCount) {
        {
        for (int currentIndex = 0; currentIndex < vertexCount; ++currentIndex) {
            const int previousIndex =
                currentIndex == 0 ? vertexCount - 1 : currentIndex - 1;
            const zVec3 *previous = &polygonVertices[previousIndex];
            const zVec3 *current = &polygonVertices[currentIndex];
            const float edge = (queryX - previous->x) * (current->z - previous->z) +
                               (queryZ - previous->z) * (previous->x - current->x);
            if (edge <= -0.0001f) {
                return 0;
            }
        }
        }

        zMath_Vec3_TriangleNormal(&polygonVertices[0], &polygonVertices[1], &polygonVertices[2],
                                  &candidate->surfaceNormal);

        if (candidate->surfaceNormal.y == 0.0f) {
            candidate->hitPos.y = polygonVertices[0].y;
            return 1;
        }

        candidate->hitPos.y =
            polygonVertices[0].y - ((queryX - polygonVertices[0].x) * candidate->surfaceNormal.x +
                                    (queryZ - polygonVertices[0].z) * candidate->surfaceNormal.z) /
                                       candidate->surfaceNormal.y;
        return 1;
    }

    // Reimplements 0x4857f0: zClass_cls_di::BuildPickCandidateForSegmentVsPolygon
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateForSegmentVsPolygon(
        zClassDiPickCandidateEntry * candidate, const zVec3 *segmentStart, const zVec3 *segmentEnd,
        const zVec3 *polygonVertices, int vertexCount, int cullBackface) {
        return BuildPickCandidateForSegmentVsPolygonCore(candidate, segmentStart, segmentEnd,
                                                         polygonVertices, vertexCount, cullBackface,
                                                         0)
                   ? 1
                   : 0;
    }

    // Reimplements 0x485d10: zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateForSegmentVsPolygonWithUv(
        zClassDiPickCandidateEntry * candidate, const zVec3 *segmentStart, const zVec3 *segmentEnd,
        const zVec3 *polygonVertices, const zModel_PickFaceUvData *faceUvData, zVec2 *outUv,
        int vertexCount, int cullBackface) {
        int dominantAxis = 0;
        if (!BuildPickCandidateForSegmentVsPolygonCore(candidate, segmentStart, segmentEnd,
                                                       polygonVertices, vertexCount, cullBackface,
                                                       &dominantAxis)) {
            return 0;
        }

        SolvePickCandidateUvForProjectedPlane(candidate, polygonVertices, faceUvData, outUv,
                                              dominantAxis);
        OptCatalog_SetDamageMaskUv(outUv->x, outUv->y);
        return 1;
    }

    // Reimplements 0x484fc0: zClass_cls_di::AppendPickCandidatesForFace
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL AppendPickCandidatesForFace(
        const zModel_PickFaceData *faceData, zClassDiPickCandidateEntry *candidate,
        const zVec3 *segmentStart, const zVec3 *segmentEnd) {
        if (faceData == 0 || faceData->faceCount == 0) {
            return 0;
        }

        const zVec3 *vertices = faceData->baseVertices;
        if ((faceData->flags & 8) != 0 && faceData->morphWeight != 0.0f &&
            faceData->morphVertexCount != 0) {
            zMath_Vec3Array_AddScaled(g_zModel_SharedVec3ScratchA, faceData->baseVertices,
                                      faceData->morphVertices, faceData->morphVertexCount,
                                      faceData->morphWeight);
            vertices = g_zModel_SharedVec3ScratchA;
        }

        zVec3 queryPoint = {0};
        zVec3 localSegmentEnd = {0};
        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            queryPoint = *segmentStart;
            localSegmentEnd = *segmentEnd;
        } else {
            queryPoint = TransformWorldPointToModel(segmentStart);
            localSegmentEnd = TransformWorldPointToModel(segmentEnd);
        }

        {
        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            const zModel_PickFaceEntry *face = &faceData->faces[faceIndex];
            const unsigned int flagsAndVertexCount = face->flagsAndVertexCount;
            const unsigned int vertexCount = flagsAndVertexCount & 0xffu;
            CopyFaceVerticesToScratch(vertices, face->vertexIndices, vertexCount);

            const int cullBackface =
                static_cast<int>((flagsAndVertexCount >> 8) & 1u);
            int hit = 0;
            if ((face->scenePayload->flags & kPickFaceTexturedDamageMaskFlag) != 0) {
                zVec2 outUv = {0};
                hit = BuildPickCandidateForSegmentVsPolygonWithUv(
                    candidate, &queryPoint, &localSegmentEnd, g_zClass_DiFaceVertexScratch4,
                    face->faceUvData, &outUv, static_cast<int>(vertexCount), cullBackface);
            } else {
                hit = BuildPickCandidateForSegmentVsPolygon(
                    candidate, &queryPoint, &localSegmentEnd, g_zClass_DiFaceVertexScratch4,
                    static_cast<int>(vertexCount), cullBackface);
            }

            if (hit == 0) {
                continue;
            }

            candidate->scenePayload = face->scenePayload;
            if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
                candidate->hitPos = TransformModelPointToWorld(&candidate->hitPos);
                candidate->surfaceNormal = TransformModelVectorToWorld(&candidate->surfaceNormal);
            }

            return 1;
        }
        }

        return 0;
    }
}

namespace zDi {
    // Reimplements 0x484960: zDi::BuildPickCandidateForQueryPoint (GameZRecoil/zModel/zmodel.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateForQueryPoint(
        zDiPartial * self, zClassDiPickCandidateEntry * outCandidate, const zVec3 *queryPoint) {
        if (self == 0 || self->entryCount == 0) {
            return 0;
        }

        const zVec3 *vertices = self->verts;
        if ((self->flags & 0x08) != 0 && self->blendScale != 0.0f && self->blendVertCount != 0) {
            zMath_Vec3Array_AddScaled(g_zModel_SharedVec3ScratchA, self->verts, self->blendVerts,
                                      self->blendVertCount, self->blendScale);
            vertices = g_zModel_SharedVec3ScratchA;
        }

        TransformVerticesToSharedScratch(vertices, self->vertCount);

        {
        for (int entryIndex = 0; entryIndex < self->entryCount; ++entryIndex) {
            zDiEntryPartial *entry = &self->entries[entryIndex];
            const int vertexCount =
                static_cast<int>(entry->flagsAndIndexCount & 0xffu);
            const int *vertexIndices =
                static_cast<const int *>(entry->vertexIndices);
            CopyFaceVerticesToScratch(g_zModel_SharedVec3ScratchB, vertexIndices,
                                      static_cast<unsigned int>(vertexCount));

            if (zClass_cls_di::TryGetPolygonHitAtQueryXZ(
                    outCandidate, g_zClass_DiFaceVertexScratch4, queryPoint->x, queryPoint->z,
                    vertexCount) != 0 &&
                outCandidate->hitPos.y <= queryPoint->y) {
                memcpy(&outCandidate->variantTag, &entry->variantTagInitialized,
                            sizeof(outCandidate->variantTag));
                outCandidate->scenePayload = entry->material;
                return 1;
            }
        }
        }

        return 0;
    }
}

namespace zModelConst {
    // Reimplements 0x484b70: zModelConst::AddFaceToPlayerProbeSampleBuckets
    // (GameZRecoil/zModel/zmodel.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL AddFaceToPlayerProbeSampleBuckets(
        zClass_NodePartial * node, PlayerProbeSampleCandidateBuffer *outputBuckets,
        const zVec3 *samplePoints, const int *sampleMaskSeeds, int samplePointCount,
        float maxProjectedY, const zVec3 *polygonVertices,
        const zModel_PickFaceEntry *faceEntry) {
        zVec3 normal;
        zMath_Vec3_TriangleNormal(&polygonVertices[0], &polygonVertices[1],
                                  &polygonVertices[2], &normal);
        if (normal.y <= 0.0f) {
            return;
        }

        int activeFlags[0x20];
        for (int i = 0; i < samplePointCount; ++i) {
            activeFlags[i] = sampleMaskSeeds[i];
        }

        int anyActive = 1;
        const int vertexCount = static_cast<int>(faceEntry->flagsAndVertexCount & 0xffu);
        for (int edgeEnd = vertexCount - 1; edgeEnd >= 0 && anyActive != 0; --edgeEnd) {
            const int edgeStart = edgeEnd == vertexCount - 1 ? 0 : edgeEnd + 1;
            const zVec3 *start = &polygonVertices[edgeStart];
            const zVec3 *end = &polygonVertices[edgeEnd];
            const float dx = end->x - start->x;
            const float dz = start->z - end->z;

            anyActive = 0;
            for (int sampleIndex = 0; sampleIndex < samplePointCount; ++sampleIndex) {
                if (activeFlags[sampleIndex] != 0) {
                    const zVec3 *point = &samplePoints[sampleIndex];
                    const float edgeTest =
                        (point->x - end->x) * dz + (point->z - end->z) * dx;
                    activeFlags[sampleIndex] = edgeTest > -0.0001f ? 1 : 0;
                    if (activeFlags[sampleIndex] != 0) {
                        anyActive = 1;
                    }
                }
            }
        }

        if (anyActive == 0 || samplePointCount <= 0) {
            return;
        }

        const float invNormalY = 1.0f / normal.y;
        const float xSlope = -normal.x * invNormalY;
        const float zSlope = -normal.z * invNormalY;
        for (int sampleIndex = 0; sampleIndex < samplePointCount; ++sampleIndex) {
            if (activeFlags[sampleIndex] != 0) {
                PlayerProbeSampleCandidateBuffer *bucket = &outputBuckets[sampleIndex];
                if (bucket->candidateCount < 0x20) {
                    zClassDiPickCandidateEntry *entry = &bucket->entries[bucket->candidateCount];
                    entry->surfaceNormal = normal;
                    entry->hitPos.y =
                        (samplePoints[sampleIndex].z - polygonVertices[0].z) * zSlope +
                        (samplePoints[sampleIndex].x - polygonVertices[0].x) * xSlope +
                        polygonVertices[0].y;
                    if (entry->hitPos.y <= maxProjectedY) {
                        entry->node = node;
                        entry->variantTag = faceEntry->variantTag;
                        entry->scenePayload = faceEntry->scenePayload;
                        ++bucket->candidateCount;
                    }
                }
            }
        }
    }
}

namespace zClass_cls_di {

    // Reimplements 0x484e00: zClass_cls_di::PickTestMeshAtQueryXZ
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL PickTestMeshAtQueryXZ(
        zClass_NodePartial * node, zModel_PickFaceData *faceData, const zVec3 *samplePoints,
        const int *sampleMaskSeeds, int samplePointCount, float maxProjectedY,
        PlayerProbeSampleCandidateBuffer *outputBuckets) {
        if (faceData == 0 || faceData->faceCount == 0) {
            return;
        }

        const zVec3 *vertices = faceData->baseVertices;
        if ((faceData->flags & 0x08) != 0 && faceData->morphWeight != 0.0f &&
            faceData->morphVertexCount != 0) {
            zMath_Vec3Array_AddScaled(g_zModel_SharedVec3ScratchA, faceData->baseVertices,
                                      faceData->morphVertices, faceData->morphVertexCount,
                                      faceData->morphWeight);
            vertices = g_zModel_SharedVec3ScratchA;
        }

        TransformVerticesToSharedScratch(vertices, faceData->vertexCount);

        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            const zModel_PickFaceEntry *face = &faceData->faces[faceIndex];
            const int vertexCount = static_cast<int>(face->flagsAndVertexCount & 0xffu);
            CopyFaceVerticesToScratch(g_zModel_SharedVec3ScratchB, face->vertexIndices,
                                      static_cast<unsigned int>(vertexCount));
            zModelConst::AddFaceToPlayerProbeSampleBuckets(
                node, outputBuckets, samplePoints, sampleMaskSeeds, samplePointCount,
                maxProjectedY, g_zClass_DiFaceVertexScratch4, face);
        }
    }

    // Reimplements 0x487900: zClass_cls_di::FilterRegionsAgainstMeshFaces
    // (D:\Proj\GameZRecoil\zClass\cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstMeshFaces(
        zVec3 *meshVertices, int faceCount) {
        g_zModel_PointInPolygonVertexCount = 0;
        if (faceCount > 0x40) {
            return 0;
        }

        if (faceCount > 0) {
            {
            for (int vertexIndex = faceCount - 1; vertexIndex >= 0; --vertexIndex) {
                const int nextIndex =
                    vertexIndex == faceCount - 1 ? 0 : vertexIndex + 1;
                g_zModel_PointInPolygonVertices[vertexIndex] = meshVertices[vertexIndex];

                zVec3 *edgeNormal = &g_zModel_PointInPolygonEdgeNormals[vertexIndex];
                edgeNormal->x = meshVertices[nextIndex].z - meshVertices[vertexIndex].z;
                edgeNormal->y = 0.0f;
                edgeNormal->z = meshVertices[vertexIndex].x - meshVertices[nextIndex].x;
                zMath::Vec3Normalize(edgeNormal);
            }
            }
        }

        g_zModel_PointInPolygonVertexCount = faceCount;
        return 1;
    }

    // Reimplements 0x4879c0: zClass_cls_di::FilterRegionsAgainstHexahedronFaces
    // (D:\Proj\GameZRecoil\zClass\cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstHexahedronFaces(
        zVec3 *center, float radius) {
        zVec3 *vertex = g_zModel_PointInPolygonVertices;
        zVec3 *edgeNormal = g_zModel_PointInPolygonEdgeNormals;

        for (int vertexIndex = 0; vertexIndex < g_zModel_PointInPolygonVertexCount;
             ++vertexIndex) {
            const float distance = (center->x - vertex->x) * edgeNormal->x +
                                   (center->z - vertex->z) * edgeNormal->z;
            if (distance < radius) {
                return 0;
            }

            ++vertex;
            ++edgeNormal;
        }

        return 1;
    }

    // Reimplements 0x4455f0: zClass_cls_di::BuildPickCandidatesForSegment
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegment(zClass_NodePartial *
                                                                               self) {
        int result = self->listCountB;
        {
        for (int childIndex = 0; childIndex < result; ++childIndex) {
            zClass_NodePartial *child = self->listB[childIndex];
            const int childFlags = child->flags;
            if ((childFlags & kNodeFlagEnabledForPick) != 0 &&
                (childFlags & kNodeFlagRaycastable) != 0 &&
                VariantTag::CurrentAllowsId(child->nodeType) != 0) {
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

    // Reimplements 0x444de0: zClass_cls_di::RaycastSelectClosestHitBetweenPoints
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RaycastSelectClosestHitBetweenPoints(
        zClass_NodePartial * world, const zVec3 *startPoint, const zVec3 *endPoint,
        PlayerProbeSampleCandidateBuffer *rayData) {
        if (RaycastFindClosest(world, rayData, startPoint->x, startPoint->y, startPoint->z,
                               endPoint->x, endPoint->y, endPoint->z) != 0) {
            return 1;
        }

        if (rayData->candidateCount <= 1) {
            rayData->candidateCount = 0;
            return 0;
        }

        const zClassDiPickCandidateEntry *candidate = &rayData->entries[0];
        float closestDistance = zMath::Vec3DeltaLengthSq(startPoint, &candidate->hitPos);
        int bestCandidateIndex = 0;
        int candidateIndex = 0;

        rayData->candidateCount -= 1;
        do {
            ++candidate;
            ++candidateIndex;

            const float candidateDistance =
                zMath::Vec3DeltaLengthSq(startPoint, &candidate->hitPos);
            if (!(candidateDistance >= closestDistance)) {
                closestDistance = candidateDistance;
                bestCandidateIndex = candidateIndex;
            }

            const int remainingCandidateCount = rayData->candidateCount;
            rayData->candidateCount = remainingCandidateCount - 1;
            if (remainingCandidateCount == 1) {
                break;
            }
        } while (true);

        rayData->candidateCount = bestCandidateIndex;
        return 0;
    }

    // Reimplements 0x444e90: zClass_cls_di::RaycastFindClosest (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RaycastFindClosest(
        zClass_NodePartial * world, PlayerProbeSampleCandidateBuffer * rayData, float startX,
        float startY, float startZ, float endX, float endY, float endZ) {
        rayData->candidateCount = 0;

        if (world == 0) {
            zError::ReportOld(0x400, kClsDiSourceFile, 0x7d1, "Null node pointer.");
            return 5;
        }

        if (world->classData == 0) {
            zError::ReportOld(0x400, kClsDiSourceFile, 0x7d2, "Null class data pointer");
            return 5;
        }

        if (zClass_TypeList::Head(0) != 0) {
            zClass_TypeList::UpdateQueuedTrees();
        }

        zClass_WorldDataPartial *worldData =
            static_cast<zClass_WorldDataPartial *>(world->classData);

        g_DiSegmentMinX = MinFloat(startX, endX);
        g_DiSegmentMaxX = MaxFloat(startX, endX);
        g_DiSegmentMinY = MinFloat(startY, endY);
        g_DiSegmentMaxY = MaxFloat(startY, endY);
        g_DiSegmentMinZ = MinFloat(startZ, endZ);
        g_DiSegmentMaxZ = MaxFloat(startZ, endZ);

        g_DiPickQueryPoint = zVec3_Make(startX, startY, startZ);
        g_DiSegmentEnd = zVec3_Make(endX, endY, endZ);
        g_DiPickCandidateBuffer = rayData;
        g_DiPickCandidateCursor = rayData->entries;

        zMat4x3 slotBuffer = {0};
        zMath::MatStackPushPtr((float *)(&slotBuffer));
        zMath::MatLoadIdentity();

        const bool segmentOverlapsWorld =
            g_DiSegmentMaxX >= worldData->originX && g_DiSegmentMinX <= worldData->worldMaxX &&
            g_DiSegmentMaxZ >= worldData->originZ && g_DiSegmentMinZ <= worldData->worldMaxZ;

        if ((worldData->clampQueriesToBounds != 0 || segmentOverlapsWorld) &&
            worldData->areaGridRows != 0 && worldData->areaGridColCount > 0 &&
            worldData->areaGridRowCount > 0) {
            int gridCol = static_cast<int>(
                floor((g_DiPickQueryPoint.x - worldData->originX) * worldData->areaInvSizeX));
            int gridRow = static_cast<int>(
                floor((g_DiPickQueryPoint.z - worldData->originZ) * worldData->areaInvSizeZ));

            const float deltaX = g_DiSegmentEnd.x - g_DiPickQueryPoint.x;
            const float deltaZ = g_DiSegmentEnd.z - g_DiPickQueryPoint.z;
            const int gridColStep = RayGridStep(deltaX);
            const int gridRowStep = RayGridStep(deltaZ);
            const float invDeltaX = gridColStep != 0 ? 1.0f / deltaX : 0.0f;
            const float invDeltaZ = gridRowStep != 0 ? 1.0f / deltaZ : 0.0f;

            while (true) {
                const bool insideGrid = gridCol >= 0 && gridCol < worldData->areaGridColCount &&
                                        gridRow >= 0 && gridRow < worldData->areaGridRowCount;

                if (insideGrid || worldData->clampQueriesToBounds != 0) {
                    int cellCol = gridCol;
                    int cellRow = gridRow;
                    bool queryWasClamped = false;
                    if (!insideGrid) {
                        queryWasClamped = true;
                        if (cellCol < 0) {
                            cellCol = 0;
                        } else if (cellCol >= worldData->areaGridColCount) {
                            cellCol = worldData->areaGridColCount - 1;
                        }

                        if (cellRow < 0) {
                            cellRow = 0;
                        } else if (cellRow >= worldData->areaGridRowCount) {
                            cellRow = worldData->areaGridRowCount - 1;
                        }
                    }

                    const float offsetX =
                        static_cast<float>(cellCol - gridCol) * worldData->areaCellSizeX;
                    const float offsetZ =
                        static_cast<float>(cellRow - gridRow) * worldData->areaCellSizeZ;
                    const int candidateCountBeforeCell = rayData->candidateCount;

                    if (queryWasClamped) {
                        OffsetActiveRayPacket(offsetX, offsetZ);
                    }

                    zWorldAreaPartial *area = &worldData->areaGridRows[cellRow][cellCol];
                    ProcessWorldAreaPickCell(area, area->childCount + 1);

                    if (queryWasClamped) {
                        OffsetActiveRayPacket(-offsetX, -offsetZ);
                        OffsetCandidatesFromCell(rayData, candidateCountBeforeCell, offsetX,
                                                 offsetZ);
                    }

                    if (BreakOnFirstCandidateHit()) {
                        break;
                    }
                } else if (gridColStep == 0 && gridRowStep == 0) {
                    break;
                }

                float tToNextGridColBoundary = 2.0f;
                if (gridColStep != 0) {
                    const int nextGridCol = gridColStep == 1 ? gridCol + 1 : gridCol;
                    tToNextGridColBoundary =
                        (static_cast<float>(nextGridCol) * worldData->areaCellSizeX +
                         worldData->originX - g_DiPickQueryPoint.x) *
                        invDeltaX;
                }

                float tToNextGridRowBoundary = 2.0f;
                if (gridRowStep != 0) {
                    const int nextGridRow = gridRowStep == 1 ? gridRow + 1 : gridRow;
                    tToNextGridRowBoundary =
                        (static_cast<float>(nextGridRow) * worldData->areaCellSizeZ +
                         worldData->originZ - g_DiPickQueryPoint.z) *
                        invDeltaZ;
                }

                if (tToNextGridColBoundary > 1.0f && tToNextGridRowBoundary > 1.0f) {
                    break;
                }

                if (tToNextGridColBoundary <= tToNextGridRowBoundary && gridColStep != 0) {
                    gridCol += gridColStep;
                }
                if (tToNextGridRowBoundary <= tToNextGridColBoundary && gridRowStep != 0) {
                    gridRow += gridRowStep;
                }
            }
        }

        if ((g_cls_di_BreakOnFirstCandidate == 0 || rayData->candidateCount <= 0) &&
            world->listCountB > 0) {
            BuildPickCandidatesForSegment(world);
        }

        zMath::MatStackPopPtr();
        g_cls_di_StopAfterFirstHit = 0;

        return rayData->candidateCount <= 0 ? 1 : 0;
    }

    // Reimplements 0x445a00: zClass_cls_di::BuildPickCandidatesForSegmentRecursive
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentRecursive(
        zClass_NodePartial * node, int depth) {
        if (depth > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FilterPointsBBox(
                node, (void *)(static_cast<unsigned int>(depth)));
            if (result != 0) {
                return result;
            }

            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                AppendCurrentCandidateNode(node);
                return 0;
            }
        }

        zClass_AnimateDataPartial *animateData =
            static_cast<zClass_AnimateDataPartial *>(node->classData);
        int pushedMatrix = 0;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3 *)(animateData->animatedTransform),
                               1);
        }

        AppendNodeFaceCandidateIfHit(node);
        if (!BreakOnFirstCandidateHit()) {
            RecurseListBChildren(node, false);
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return NoCandidatesReturn();
    }

    // Reimplements 0x445b20: zClass_cls_di::BuildPickCandidatesForSegmentForCamera
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentForCamera(
        zClass_NodePartial * node, int /*depth*/) {
        zVec3 unitScale = {1.0f, 1.0f, 1.0f};
        zClass_CameraDataPartial *cameraData =
            static_cast<zClass_CameraDataPartial *>(node->classData);

        int pushedMatrix = 0;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
            zMath::MatApplyLocalTRS(&cameraData->posOffset, &cameraData->targetOrEuler, &unitScale);
        }

        AppendNodeFaceCandidateIfHit(node);
        if (!BreakOnFirstCandidateHit()) {
            RecurseListBChildren(node, false);
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }

        return NoCandidatesReturn();
    }

    // Reimplements 0x445c20: zClass_cls_di::BuildPickCandidatesForSegmentForLight
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentForLight(
        zClass_NodePartial * node, int depth) {
        zClass_LightDataPartial *lightData =
            static_cast<zClass_LightDataPartial *>(node->classData);

        if (depth > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FilterPointsBBox(
                node, (void *)(static_cast<unsigned int>(depth)));
            if (result != 0) {
                return result;
            }

            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                AppendCurrentCandidateNode(node);
                return 0;
            }
        }

        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y,
                            lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        AppendNodeFaceCandidateIfHit(node);
        if (!BreakOnFirstCandidateHit()) {
            RecurseListBChildren(node, false);
        }

        zMath::MatStackPopPtr();
        return NoCandidatesReturn();
    }

    // Reimplements 0x446880: zClass_cls_di::BuildPickCandidatesForSegmentsForAnimate
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentsForAnimate(
        zClass_NodePartial * node, int nodeCountHint, int * activeMask) {
        int localActive[24];
        CopySegmentActiveMask(localActive, activeMask);

        if (nodeCountHint > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FrustumTestAndPick(node, localActive);
            if (result != 0) {
                return result;
            }
            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                return 0;
            }
        }

        zClass_AnimateDataPartial *animateData =
            static_cast<zClass_AnimateDataPartial *>(node->classData);
        int pushedMatrix = 0;
        if ((node->flags & kNodeFlagEnabledForPick) != 0) {
            pushedMatrix = 1;
            zMath::MatStackPushAndCloneParent(animateData->savedParentMatrix);
            zMath::MatMultiply((const zMat4x3 *)(animateData->animatedTransform), 1);
        }

        FilterCurrentSegmentRegions(node, localActive);
        if (!BreakOnFirstCandidateHit()) {
            RecurseSegmentBatchChildren(node, localActive, false);
        }

        if (pushedMatrix != 0) {
            zMath::MatStackPopPtr();
        }
        return 0;
    }

    // Reimplements 0x446970: zClass_cls_di::BuildPickCandidatesForSegmentsForLight
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentsForLight(
        zClass_NodePartial * node, int nodeCountHint, int * activeMask) {
        int localActive[24];
        CopySegmentActiveMask(localActive, activeMask);

        if (nodeCountHint > 1 || (node->flags & kNodeFlagPointCandidate) != 0) {
            const int result = FrustumTestAndPick(node, localActive);
            if (result != 0) {
                return result;
            }
            if ((node->flags & kNodeFlagPointCandidate) != 0) {
                return 0;
            }
        }

        zClass_LightDataPartial *lightData =
            static_cast<zClass_LightDataPartial *>(node->classData);
        zMath::MatStackPushAndCloneParent(lightData->savedParentMatrix);
        zMath::MatTranslate(lightData->localPosition.x, lightData->localPosition.y,
                            lightData->localPosition.z);
        zMath::MatRotateY(lightData->localRotation.y);
        zMath::MatRotateX(lightData->localRotation.x);
        zMath::MatRotateZ(lightData->localRotation.z);

        FilterCurrentSegmentRegions(node, localActive);
        if (!BreakOnFirstCandidateHit()) {
            RecurseSegmentBatchChildren(node, localActive, true);
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x445d40: zClass_cls_di::BuildProbeHitBatchesForSegments
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL BuildProbeHitBatchesForSegments(
        zClass_NodePartial * world, zClass_DiSegmentEndpoints * segmentEndpoints,
        int endpointCount, PlayerProbeSampleCandidateBuffer * hitBatches) {
        if (endpointCount > 24) {
            zError::ReportOld(0x200, kClsDiSourceFile, 0xba9,
                              "More test pnts than space for: %d", endpointCount);
            endpointCount = 24;
        }

        if (zClass_TypeList::Head(0) != 0) {
            zClass_TypeList::UpdateQueuedTrees();
        }

        const int segmentCount = endpointCount >> 1;
        int segmentActive[24] = {0};
        for (int i = 0; i < segmentCount; ++i) {
            segmentActive[i] = 1;
            hitBatches[i].candidateCount = 0;
        }

        zClass_WorldDataPartial *worldData =
            static_cast<zClass_WorldDataPartial *>(world->classData);
        int anyActive = 0;
        for (int i = 0; i < segmentCount; ++i) {
            BuildSegmentBoundsFromEndpoints(&segmentEndpoints[i], &g_DiSegmentBounds[i]);
            if (worldData->clampQueriesToBounds == 0 &&
                !SegmentBoundsOverlapWorldXZ(&g_DiSegmentBounds[i], worldData)) {
                segmentActive[i] = 0;
            } else {
                anyActive = 1;
            }
        }

        if (anyActive != 0) {
            zMat4x3 slotBuffer = {0};
            zMath::MatStackPushPtr((float *)(&slotBuffer));
            zMath::MatLoadIdentity();

            g_DiPickCandidateBuffer = hitBatches;
            g_DiPickPointArray = &segmentEndpoints[0].start;
            g_DiPickPointCount = segmentCount;

            BuildPickCandidatesForSegmentsInGridWindow(world, segmentActive);
            for (int i = 0; i < world->listCountB; ++i) {
                zClass_NodePartial *node = world->listB[i];
                if ((node->flags & kNodeFlagEnabledForPick) != 0 &&
                    (node->flags & kNodeFlagRaycastable) != 0 &&
                    VariantTag::CurrentAllowsId(node->nodeType) != 0) {
                    BuildPickCandidatesForSegmentsRecursive(node, world->listCountB + 1,
                                                            segmentActive);
                }

                if (BreakOnFirstCandidateHit()) {
                    break;
                }
            }

            zMath::MatStackPopPtr();
        }

        g_cls_di_StopAfterFirstHit = 0;
    }

    // Reimplements 0x445f60: zClass_cls_di::BuildPickCandidatesForSegmentsInGridWindow
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL BuildPickCandidatesForSegmentsInGridWindow(
        zClass_NodePartial * world, int * activeMask) {
        const int segmentCount = g_DiPickPointCount;
        if (segmentCount <= 0) {
            return;
        }

        zClass_WorldDataPartial *worldData =
            static_cast<zClass_WorldDataPartial *>(world->classData);

        int segmentMinCol[24] = {0};
        int segmentMaxCol[24] = {0};
        int segmentMinRow[24] = {0};
        int segmentMaxRow[24] = {0};

        for (int i = 0; i < segmentCount; ++i) {
            const zClass_DiSegmentBounds *bounds = &g_DiSegmentBounds[i];
            segmentMinCol[i] =
                GridCoordFromWorld(bounds->minX, worldData->originX, worldData->areaInvSizeX);
            segmentMaxCol[i] =
                GridCoordFromWorld(bounds->maxX, worldData->originX, worldData->areaInvSizeX);
            segmentMinRow[i] =
                GridCoordFromWorld(bounds->minZ, worldData->originZ, worldData->areaInvSizeZ);
            segmentMaxRow[i] =
                GridCoordFromWorld(bounds->maxZ, worldData->originZ, worldData->areaInvSizeZ);
        }

        int windowMinCol = segmentMinCol[0];
        int windowMaxCol = segmentMaxCol[0];
        int windowMinRow = segmentMinRow[0];
        int windowMaxRow = segmentMaxRow[0];
        for (int i = 1; i < segmentCount; ++i) {
            if (segmentMinCol[i] < windowMinCol) {
                windowMinCol = segmentMinCol[i];
            }
            if (segmentMaxCol[i] > windowMaxCol) {
                windowMaxCol = segmentMaxCol[i];
            }
            if (segmentMinRow[i] < windowMinRow) {
                windowMinRow = segmentMinRow[i];
            }
            if (segmentMaxRow[i] > windowMaxRow) {
                windowMaxRow = segmentMaxRow[i];
            }
        }

        for (int row = windowMinRow; row <= windowMaxRow; ++row) {
            for (int col = windowMinCol; col <= windowMaxCol; ++col) {
                int cellCol = col;
                int cellRow = row;
                int usedClampedCell = 0;
                int firstNewCandidate[24] = {0};
                float offsetX = 0.0f;
                float offsetZ = 0.0f;

                const int insideGrid = col >= 0 && col < worldData->areaGridColCount &&
                                       row >= 0 && row < worldData->areaGridRowCount;
                if (insideGrid == 0) {
                    if (worldData->clampQueriesToBounds == 0) {
                        continue;
                    }

                    usedClampedCell = 1;
                    SaveSegmentCandidateCounts(firstNewCandidate);
                    cellCol = ClampGridCoord(cellCol, worldData->areaGridColCount);
                    cellRow = ClampGridCoord(cellRow, worldData->areaGridRowCount);
                    offsetX = static_cast<float>(cellCol - col) * worldData->areaCellSizeX;
                    offsetZ = static_cast<float>(cellRow - row) * worldData->areaCellSizeZ;
                    OffsetSegmentBatchXZ(offsetX, offsetZ);
                }

                zWorldAreaPartial *area = &worldData->areaGridRows[cellRow][cellCol];
                for (int childIndex = 0; childIndex < area->childCount; ++childIndex) {
                    zClass_NodePartial *child = area->childList[childIndex];
                    if ((child->flags & kNodeFlagEnabledForPick) != 0 &&
                        (child->flags & kNodeFlagRaycastable) != 0) {
                        BuildPickCandidatesForSegmentsRecursive(child, area->childCount + 1,
                                                                activeMask);
                    }

                    if (BreakOnFirstCandidateHit()) {
                        break;
                    }
                }

                if (usedClampedCell != 0) {
                    OffsetSegmentBatchXZ(-offsetX, -offsetZ);
                    RestoreClampedSegmentCandidatePositions(firstNewCandidate, offsetX, offsetZ);
                }
            }
        }
    }

    // Reimplements 0x446440: zClass_cls_di::BuildPickCandidatesForSegmentsRecursive
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentsRecursive(
        zClass_NodePartial * node, int nodeCountHint, int * activeMask) {
        int nodeFlags = node->flags;
        if ((nodeFlags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((nodeFlags & kNodeFlagRaycastable) == 0) {
            return 1;
        }
        if ((g_cls_di_StopAfterFirstHit & nodeFlags) != 0) {
            return 1;
        }
        if (VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        nodeFlags &= ~kNodeFlagClearDuringPick;
        node->flags = nodeFlags;

        switch (node->classId) {
        case kNodeClassCamera: {
            zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            zClass_CameraDataPartial *cameraData =
                static_cast<zClass_CameraDataPartial *>(node->classData);
            int pushedMatrix = 0;
            if ((nodeFlags & kNodeFlagEnabledForPick) != 0) {
                pushedMatrix = 1;
                zMath::MatStackPushAndCloneParent(cameraData->worldTransform);
                zMath::MatApplyLocalTRS(&cameraData->posOffset, &cameraData->targetOrEuler,
                                        &unitScale);
            }

            FilterCurrentSegmentRegions(node, activeMask);
            if (!BreakOnFirstCandidateHit()) {
                RecurseSegmentBatchChildren(node, activeMask, false);
            }
            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassObject3D: {
            int localActive[24];
            CopySegmentActiveMask(localActive, activeMask);
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FrustumTestAndPick(node, localActive);
                if (result != 0) {
                    return result;
                }
                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    return 0;
                }
            }

            zClass_Object3DDataPartial *objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            int pushedMatrix = 0;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) == 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix), 3);
                } else if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix), 1);
                    if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                        objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                    }
                } else {
                    zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                }
            }

            FilterCurrentSegmentRegions(node, localActive);
            if (!BreakOnFirstCandidateHit()) {
                RecurseSegmentBatchChildren(node, localActive, true);
            }
            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }
            return 0;
        }

        case kNodeClassLod: {
            zClass_LodDataPartial *lodData = static_cast<zClass_LodDataPartial *>(node->classData);
            if (lodData->nearRangeSq > 5.0f) {
                return 1;
            }

            int localActive[24];
            CopySegmentActiveMask(localActive, activeMask);
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FrustumTestAndPick(node, localActive);
                if (result != 0) {
                    return result;
                }
                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    return 0;
                }
            }

            RecurseSegmentBatchChildren(node, localActive, false);
            return 0;
        }

        case kNodeClassSequence: {
            zClass_SequenceDataPartial *sequenceData =
                static_cast<zClass_SequenceDataPartial *>(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            int localActive[24];
            CopySegmentActiveMask(localActive, activeMask);
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FrustumTestAndPick(node, localActive);
                if (result != 0) {
                    return result;
                }
                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    return 0;
                }
            }

            return BuildPickCandidatesForSegmentsRecursive(
                sequenceData->entries[sequenceData->currentIndex].node, node->listCountB,
                localActive);
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesForSegmentsForAnimate(node, nodeCountHint, activeMask);

        case kNodeClassLight:
            return BuildPickCandidatesForSegmentsForLight(node, nodeCountHint, activeMask);

        case kNodeClassSound:
            return static_cast<int>((unsigned int)(activeMask));

        default:
            zError::ReportOld(0x200, kClsDiSourceFile, 0xd41,
                              "Unrecognized node class type:  node = %s class_type = %d", node,
                              node->classId);
            return 3;
        }
    }

    // Reimplements 0x445650: zClass_cls_di::BuildPickCandidatesForSegmentChildFallback
    // (GameZRecoil/zClass/cls_di.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentChildFallback(
        zClass_NodePartial * node, int nodeCountHint) {
        int nodeFlags = node->flags;
        if ((nodeFlags & kNodeFlagEnabledForPick) == 0) {
            return 1;
        }
        if ((nodeFlags & kNodeFlagRaycastable) == 0) {
            return 1;
        }
        if ((g_cls_di_StopAfterFirstHit & nodeFlags) != 0) {
            return 1;
        }
        if (BreakOnFirstCandidateHit()) {
            return 0;
        }
        if (VariantTag::CurrentAllowsId(node->nodeType) == 0) {
            return 1;
        }

        nodeFlags &= ~kNodeFlagClearDuringPick;
        node->flags = nodeFlags;
        if (g_DiPickCandidateBuffer->candidateCount >= kMaxPickCandidates) {
            zError::ReportOld(0x200, kClsDiSourceFile, 0x94c,
                              "Database intersections array is full");
            return 1;
        }

        switch (node->classId) {
        case kNodeClassCamera:
            return BuildPickCandidatesForSegmentForCamera(node, nodeCountHint);

        case kNodeClassObject3D: {
            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FilterPointsBBox(
                    node, (void *)(static_cast<unsigned int>(nodeFlags)));
                if (result != 0) {
                    return result;
                }

                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    AppendCurrentCandidateNode(node);
                    return 0;
                }
            }

            zClass_Object3DDataPartial *objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            int pushedMatrix = 0;
            if ((objectData->flags & kObjectFlagNoPickMatrixPush) == 0) {
                pushedMatrix = 1;
                if ((node->flags & kNodeFlagUseLocalMatrixMode3) == 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix),
                                       3);
                } else if ((objectData->flags & kObjectFlagUseCachedWorldMatrix) != 0) {
                    zMath::MatStackPushAndCloneParent(objectData->cachedWorldMatrix);
                    zMath::MatMultiply((const zMat4x3 *)(objectData->localMatrix),
                                       1);
                    if ((objectData->flags & kObjectFlagTransformDirty) == 0) {
                        objectData->flags &= ~kObjectFlagUseCachedWorldMatrix;
                    }
                } else {
                    zMath::MatStackPushPtr(objectData->cachedWorldMatrix);
                }
            }

            AppendNodeFaceCandidateIfHit(node);
            if (!BreakOnFirstCandidateHit()) {
                RecurseListBChildren(node, true);
            }

            if (pushedMatrix != 0) {
                zMath::MatStackPopPtr();
            }

            return NoCandidatesReturn();
        }

        case kNodeClassLod: {
            zClass_LodDataPartial *lodData = static_cast<zClass_LodDataPartial *>(node->classData);
            if (lodData->nearRangeSq > 5.0f) {
                return 1;
            }

            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FilterPointsBBox(
                    node, (void *)(static_cast<unsigned int>(nodeFlags)));
                if (result != 0) {
                    return result;
                }

                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    AppendCurrentCandidateNode(node);
                    return 0;
                }
            }

            RecurseListBChildren(node, false);
            return g_DiPickCandidateBuffer->candidateCount == 0 ? 1 : 0;
        }

        case kNodeClassSequence: {
            zClass_SequenceDataPartial *sequenceData =
                static_cast<zClass_SequenceDataPartial *>(node->classData);
            if (sequenceData->isActive == 0) {
                return 1;
            }

            if (nodeCountHint > 1 || (nodeFlags & kNodeFlagPointCandidate) != 0) {
                const int result = FilterPointsBBox(
                    node, (void *)(static_cast<unsigned int>(nodeFlags)));
                if (result != 0) {
                    return result;
                }

                if ((node->flags & kNodeFlagPointCandidate) != 0) {
                    AppendCurrentCandidateNode(node);
                    return 0;
                }
            }

            return BuildPickCandidatesForSegmentChildFallback(
                sequenceData->entries[sequenceData->currentIndex].node, node->listCountB);
        }

        case kNodeClassAnimate:
            return BuildPickCandidatesForSegmentRecursive(node, nodeCountHint);

        case kNodeClassLight:
            return BuildPickCandidatesForSegmentForLight(node, nodeCountHint);

        case kNodeClassSound:
            return static_cast<int>((unsigned int)(node));

        default:
            zError::ReportOld(0x200, kClsDiSourceFile, 0x97a,
                              "Unrecognized node class type:  node = %s class_type = %d", node,
                              node->classId);
            return 3;
        }
    }
}
