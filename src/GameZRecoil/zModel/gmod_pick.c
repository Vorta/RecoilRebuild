// zModel compilation unit between gmod_const.c and gmod_light.c, inferred from
// the retail object boundary [0x484960, 0x487a30): its .rdata pooled constants
// [0x4d2b08, 0x4d2b28) repeat double 0.0, float 0.0 and 1.0f that gmod_const.c
// pools separately, and the 1998-07-21 demo links it after gmod_matl.c, apart
// from gmod_const.c. Original filename unresolved; gmod_pick.c is a
// provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zTime/time.h"
#include "zdi.h"

#include "Battlesport/player.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace zMath
{
    /**
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.gmod-pick.vector-transform-point-in-place
     *
     * Purpose: Transform a point in place by the matrix's 3x3 part plus
     * translation; all reads precede the x/z/y binary32 stores.
     * Reconstruction: recurring inline-helper family (retail 0x4747ff and
     * 0x4852a9); original spelling and declaration location unproved.
     * gmod_pick.c-resident inline definition.
     * Raw assembly: the parameter-home reloads and grouped x87 schedule strongly
     * support an inferred inline-assembly helper; the documented VC5SP3 C/C++
     * candidates (and a capture-macro form) did not reproduce the consumer body.
     * Island contract: EAX/EBX hold vector/matrix from compiler-owned parameter
     * homes and are clobbered; integer flags and the x87 control word unchanged;
     * x87 entry/peak/exit depth 0/6/0 on normal completion; x87 status and
     * exceptions are not preserved. The vector must not overlap the matrix: the
     * non-target fallback interleaves result stores with later matrix reads.
     * Consumers are scoped by the raw-assembly allowlist.
     */
    inline void Vec3TransformPointInPlace(const zMat4x3* matrix, zVec3* vector)
    {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
        mov eax, vector
        mov ebx, matrix
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xx
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xy
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xz
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yx
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yy
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zx
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zy
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fxch st(2)
        fadd dword ptr [ebx]zMat4x3.posX
        fxch st(1)
        fadd dword ptr [ebx]zMat4x3.posY
        fxch st(2)
        fadd dword ptr [ebx]zMat4x3.posZ
        fxch st(1)
        fstp dword ptr [eax]zVec3.x
        fstp dword ptr [eax]zVec3.z
        fstp dword ptr [eax]zVec3.y
        }
#else
        const zVec3 source = *vector;
        vector->x = source.x * matrix->xx + source.y * matrix->yx + source.z * matrix->zx + matrix->posX;
        vector->y = source.x * matrix->xy + source.y * matrix->yy + source.z * matrix->zy + matrix->posY;
        vector->z = source.x * matrix->xz + source.y * matrix->yz + source.z * matrix->zz + matrix->posZ;
#endif
    }
} // namespace zMath

namespace zMath
{
    /**
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.gmod-pick.vector-transform-direction-in-place
     *
     * Purpose: Transform a direction in place by the matrix's 3x3 part, without
     * translation; all reads precede the z/y/x binary32 stores.
     * Reconstruction: recurring inline-helper family (retail 0x4293da, 0x473f6d,
     * 0x47460f and 0x485315); original spelling and declaration location
     * unproved. gmod_pick.c-resident inline definition.
     * Raw assembly: the parameter-home reloads and grouped x87 schedule strongly
     * support an inferred inline-assembly helper; the documented VC5SP3 C/C++
     * candidates (and a capture-macro form) did not reproduce the consumer body.
     * Island contract: EAX/EBX hold vector/matrix from compiler-owned parameter
     * homes and are clobbered; integer flags and the x87 control word unchanged;
     * x87 entry/peak/exit depth 0/6/0 on normal completion; x87 status and
     * exceptions are not preserved. The vector must not overlap the matrix: the
     * non-target fallback interleaves result stores with later matrix reads.
     * Consumers are scoped by the raw-assembly allowlist.
     */
    inline void Vec3TransformDirectionInPlace(const zMat4x3* matrix, zVec3* vector)
    {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
        mov eax, vector
        mov ebx, matrix
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xx
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xy
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xz
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yx
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yy
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zx
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zy
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fstp dword ptr [eax]zVec3.z
        fstp dword ptr [eax]zVec3.y
        fstp dword ptr [eax]zVec3.x
        }
#else
        const zVec3 source = *vector;
        vector->x = source.x * matrix->xx + source.y * matrix->yx + source.z * matrix->zx;
        vector->y = source.x * matrix->xy + source.y * matrix->yy + source.z * matrix->zy;
        vector->z = source.x * matrix->xz + source.y * matrix->yz + source.z * matrix->zz;
#endif
    }
} // namespace zMath

/*
 * Address-backed gmod_const.c function contribution in natural retail order.
 */

namespace
{
    const char* kClsDiSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c";
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
        zModel_PickFaceData* faceData = (zModel_PickFaceData*)((void*)(NodeDiRef(node)));
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
} // namespace

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildpickcandidateforquerypoint
     * @recoil-artifact defines .text recoil:function:0x484960: zDi::BuildPickCandidateForQueryPoint.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     * @recoil-match byte
     *
     * Provenance: address-backed reconstruction placed in the cls_di runtime
     * surface from current Binary Ninja behavior/global evidence.
     * Purpose: preserve the recovered pick-face helper behavior used by cls_di.
     * Contract note: when execution reaches the vertex batch, its count N
     * (self->vertCount) must satisfy 1 <= N <= 0x400. The selected source must
     * contain N initialized zVec3 elements (including when the blend/morph
     * branch prepares g_zModel_SharedVec3ScratchA), and the shared transform
     * destination must provide N writable elements. The source and destination
     * ranges must not overlap; the batch identity path uses memcpy. Current
     * matrix-stack slots must satisfy the shared batch helper's contract.
     * For every processed entry, its low-byte vertex count K must satisfy
     * 1 <= K <= 0x40. Its index array must contain K readable indices, each in
     * [0, N), and the face scratch must provide K writable zVec3 elements.
     * These are caller/data preconditions, not checks performed here.
     */
    int __fastcall BuildPickCandidateForQueryPoint(
        zDiPartial * self,
        zClassDiPickCandidateEntry * outCandidate,
        const zVec3* queryPoint
    )
    {
        if (self == 0 || self->entryCount == 0) {
            return 0;
        }

        const zVec3* vertices;
        if ((self->flags & 0x08) != 0 && self->blendScale != 0.0 && self->blendVertCount != 0) {
            zMathVec3ArrayAddScaled(
                g_zModel_SharedVec3ScratchA,
                self->verts,
                self->blendVerts,
                self->blendVertCount,
                self->blendScale
            );
            vertices = g_zModel_SharedVec3ScratchA;
        } else {
            vertices = self->verts;
        }

        ZMTH_MAT_TRANSFORM_POINT_BATCH(vertices, g_zModel_SharedVec3ScratchB, self->vertCount);

        for (int entryIndex = 0; entryIndex < self->entryCount; ++entryIndex) {
            int vertexCount = (int)(self->entries[entryIndex].flagsAndIndexCount & 0xffu);
            const int* vertexIndices = (const int*)(self->entries[entryIndex].vertexIndices);
            zVec3* faceVertex = g_CZClass_DiFaceVertexScratch4;
            const zVec3* transformed = g_zModel_SharedVec3ScratchB;
            do {
                *faceVertex++ = transformed[*vertexIndices++];
            } while (--vertexCount != 0);

            if (CZDisplayInstance::TryGetPolygonHitAtQueryXZ(
                    outCandidate,
                    g_CZClass_DiFaceVertexScratch4,
                    queryPoint->x,
                    queryPoint->z,
                    (int)(self->entries[entryIndex].flagsAndIndexCount & 0xffu)
                ) != 0
                && outCandidate->hitPos.y <= queryPoint->y) {
                memcpy(
                    &outCandidate->variantTag,
                    &self->entries[entryIndex].variantTagInitialized,
                    sizeof(outCandidate->variantTag)
                );
                outCandidate->scenePayload = self->entries[entryIndex].material;
                return 1;
            }
        }

        return 0;
    }
} // namespace zDi

namespace zModelConst
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addfacetoplayerprobesamplebuckets
     * @recoil-artifact defines .text recoil:function:0x484b70: zModelConst::AddFaceToPlayerProbeSampleBuckets.
     *
     *
     * Provenance: address-backed reconstruction placed in the cls_di runtime
     * surface from current Binary Ninja behavior/global evidence.
     * Purpose: preserve the recovered pick-face helper behavior used by cls_di.
     */
    void __fastcall AddFaceToPlayerProbeSampleBuckets(
        CZNodePartial * node,
        PlayerProbeSampleCandidateBuffer * outputBuckets,
        const zVec3* samplePoints,
        const int* sampleMaskSeeds,
        int samplePointCount,
        float maxProjectedY,
        const zVec3* polygonVertices,
        const zModel_PickFaceEntry* faceEntry
    )
    {
        int anyActive = 1;
        int slopesPending = 1;
        float invNormalY;
        zVec3 normal;
        zMathVec3TriangleNormal(&polygonVertices[0], &polygonVertices[1], &polygonVertices[2], &normal);
        if (normal.y <= 0.0f) {
            return;
        }

        int activeFlags[24];
        for (int i = samplePointCount - 1; i >= 0; --i) {
            activeFlags[i] = sampleMaskSeeds[i];
        }

        zVec3 edgeNormal;
        int edgeStart = 0;
        for (int edgeEnd = (int)(faceEntry->vertexCount) - 1; edgeEnd >= 0 && anyActive != 0; --edgeEnd) {
            edgeNormal.x = polygonVertices[edgeStart].z - polygonVertices[edgeEnd].z;
            edgeNormal.z = polygonVertices[edgeEnd].x - polygonVertices[edgeStart].x;

            anyActive = 0;
            for (int sampleIndex = 0; sampleIndex < samplePointCount; ++sampleIndex) {
                if (activeFlags[sampleIndex] != 0) {
                    activeFlags[sampleIndex] = (samplePoints[sampleIndex].x - polygonVertices[edgeEnd].x) * edgeNormal.x
                                + (samplePoints[sampleIndex].z - polygonVertices[edgeEnd].z) * edgeNormal.z
                            > -0.0001
                        ? 1
                        : 0;
                    if (activeFlags[sampleIndex] != 0) {
                        anyActive = 1;
                    }
                }
            }

            edgeStart = edgeEnd;
        }

        if (anyActive == 0 || samplePointCount <= 0) {
            return;
        }

        float xSlope;
        float zSlope;
        for (int sampleIndex = 0; sampleIndex < samplePointCount; ++sampleIndex) {
            if (activeFlags[sampleIndex] != 0) {
                if (outputBuckets[sampleIndex].candidateCount < 0x20) {
                    outputBuckets[sampleIndex].entries[outputBuckets[sampleIndex].candidateCount].surfaceNormal
                        = normal;
                    if (slopesPending != 0) {
                        // Retail derives the plane slopes lazily from the first accepted sample.
                        invNormalY = 1.0f / normal.y;
                        slopesPending = 0;
                        xSlope = -normal.x * invNormalY;
                        zSlope = -normal.z * invNormalY;
                    }

                    outputBuckets[sampleIndex].entries[outputBuckets[sampleIndex].candidateCount].hitPos.y
                        = (samplePoints[sampleIndex].z - polygonVertices[0].z) * zSlope
                        + (samplePoints[sampleIndex].x - polygonVertices[0].x) * xSlope + polygonVertices[0].y;
                    if (outputBuckets[sampleIndex].entries[outputBuckets[sampleIndex].candidateCount].hitPos.y
                        <= maxProjectedY) {
                        outputBuckets[sampleIndex].entries[outputBuckets[sampleIndex].candidateCount].node = node;
                        outputBuckets[sampleIndex].entries[outputBuckets[sampleIndex].candidateCount].variantTag
                            = faceEntry->variantTag;
                        outputBuckets[sampleIndex].entries[outputBuckets[sampleIndex].candidateCount].scenePayload
                            = faceEntry->scenePayload;
                        ++outputBuckets[sampleIndex].candidateCount;
                    }
                }
            }
        }
    }
} // namespace zModelConst

namespace CZDisplayInstance
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.picktestmeshatqueryxz
     * @recoil-artifact defines .text recoil:function:0x484e00: CZDisplayInstance::PickTestMeshAtQueryXZ.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     * Contract note: when execution reaches the vertex batch, its count N
     * (faceData->vertexCount) must satisfy 1 <= N <= 0x400. The selected source must
     * contain N initialized zVec3 elements (including when the blend/morph
     * branch prepares g_zModel_SharedVec3ScratchA), and the shared transform
     * destination must provide N writable elements. The source and destination
     * ranges must not overlap; the batch identity path uses memcpy. Current
     * matrix-stack slots must satisfy the shared batch helper's contract.
     * For every processed face, its low-byte vertex count K must satisfy
     * 1 <= K <= 0x40. Its index array must contain K readable indices, each in
     * [0, N), and the face scratch must provide K writable zVec3 elements.
     * These are caller/data preconditions, not checks performed here.
     */
    void __fastcall PickTestMeshAtQueryXZ(
        CZNodePartial * node,
        zModel_PickFaceData * faceData,
        const zVec3* samplePoints,
        const int* sampleMaskSeeds,
        int samplePointCount,
        float maxProjectedY,
        PlayerProbeSampleCandidateBuffer* outputBuckets
    )
    {
        if (faceData == 0 || faceData->faceCount == 0) {
            return;
        }

        const zVec3* vertices;
        if ((faceData->flags & 0x08) != 0 && faceData->morphWeight != 0.0 && faceData->morphVertexCount != 0) {
            zMathVec3ArrayAddScaled(
                g_zModel_SharedVec3ScratchA,
                faceData->baseVertices,
                faceData->morphVertices,
                faceData->morphVertexCount,
                faceData->morphWeight
            );
            vertices = g_zModel_SharedVec3ScratchA;
        } else {
            vertices = faceData->baseVertices;
        }

        ZMTH_MAT_TRANSFORM_POINT_BATCH(vertices, g_zModel_SharedVec3ScratchB, faceData->vertexCount);

        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            int vertexCount = (int)(faceData->faces[faceIndex].vertexCount);
            const int* vertexIndices = faceData->faces[faceIndex].vertexIndices;
            zVec3* faceVertex = g_CZClass_DiFaceVertexScratch4;
            const zVec3* transformed = g_zModel_SharedVec3ScratchB;
            do {
                *faceVertex++ = transformed[*vertexIndices++];
            } while (--vertexCount != 0);

            zModelConst::AddFaceToPlayerProbeSampleBuckets(
                node,
                outputBuckets,
                samplePoints,
                sampleMaskSeeds,
                samplePointCount,
                maxProjectedY,
                g_CZClass_DiFaceVertexScratch4,
                &faceData->faces[faceIndex]
            );
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.appendpickcandidatesforface
     * @recoil-artifact defines .text recoil:function:0x484fc0: CZDisplayInstance::AppendPickCandidatesForFace.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.gmod-pick.vector-transform-point-in-place
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.gmod-pick.vector-transform-direction-in-place
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     * Contract note: the bottom-tested face-vertex copy requires a positive vertex
     * count bounded by the shared scratch capacity.
     */
    int __fastcall AppendPickCandidatesForFace(
        const zModel_PickFaceData* faceData,
        zClassDiPickCandidateEntry* candidate,
        const zVec3* segmentStart,
        const zVec3* segmentEnd
    )
    {
        if (faceData == 0 || faceData->faceCount == 0) {
            return 0;
        }

        const zVec3* vertices;
        if ((faceData->flags & 8) != 0 && faceData->morphWeight != 0.0 && faceData->morphVertexCount != 0) {
            zMathVec3ArrayAddScaled(
                g_zModel_SharedVec3ScratchA,
                faceData->baseVertices,
                faceData->morphVertices,
                faceData->morphVertexCount,
                faceData->morphWeight
            );
            vertices = g_zModel_SharedVec3ScratchA;
        } else {
            vertices = faceData->baseVertices;
        }

        zVec3 segmentStartLocal;
        zVec3 segmentEndLocal;
        zVec2 outUv;
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            zMath::Vec3Subtract(
                segmentStart,
                (const zVec3*)(&((const zMat4x3*)(*zMath::g_currentMatrixPtrSlot))->posX),
                &segmentStartLocal
            );
            ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE((const zMat4x3*)(*zMath::g_currentMatrixPtrSlot), &segmentStartLocal);
            zMath::Vec3Subtract(
                segmentEnd,
                (const zVec3*)(&((const zMat4x3*)(*zMath::g_currentMatrixPtrSlot))->posX),
                &segmentEndLocal
            );
            ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE((const zMat4x3*)(*zMath::g_currentMatrixPtrSlot), &segmentEndLocal);
        } else {
            segmentStartLocal = *segmentStart;
            segmentEndLocal = *segmentEnd;
        }

        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            int vertexCount = (int)(faceData->faces[faceIndex].vertexCount);
            const int* vertexIndices = faceData->faces[faceIndex].vertexIndices;
            zVec3* faceVertex = g_CZClass_DiFaceVertexScratch4;
            do {
                *faceVertex++ = vertices[*vertexIndices++];
            } while (--vertexCount != 0);

            int hit;
            if ((faceData->faces[faceIndex].scenePayload->flags & kPickFaceTexturedDamageMaskFlag) != 0) {
                hit = BuildPickCandidateForSegmentVsPolygonWithUv(
                    candidate,
                    &segmentStartLocal,
                    &segmentEndLocal,
                    g_CZClass_DiFaceVertexScratch4,
                    faceData->faces[faceIndex].faceUvData,
                    &outUv,
                    (int)(faceData->faces[faceIndex].vertexCount),
                    (int)(faceData->faces[faceIndex].doubleSided)
                );
            } else {
                hit = BuildPickCandidateForSegmentVsPolygon(
                    candidate,
                    &segmentStartLocal,
                    &segmentEndLocal,
                    g_CZClass_DiFaceVertexScratch4,
                    (int)(faceData->faces[faceIndex].vertexCount),
                    (int)(faceData->faces[faceIndex].doubleSided)
                );
            }

            if (hit != 0) {
                candidate->scenePayload = faceData->faces[faceIndex].scenePayload;
                if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
                    zMath::Vec3TransformPointInPlace(
                        (const zMat4x3*)(*zMath::g_currentMatrixPtrSlot),
                        &candidate->hitPos
                    );
                    zMath::Vec3TransformDirectionInPlace(
                        (const zMat4x3*)(*zMath::g_currentMatrixPtrSlot),
                        &candidate->surfaceNormal
                    );
                }

                return 1;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildpickcandidatesforsegmentvsbboxfaces
     * @recoil-artifact defines .text recoil:function:0x485380: CZDisplayInstance::BuildPickCandidatesForSegmentVsBBoxFaces.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentVsBBoxFaces(
        const zBBoxCorners* bboxCorners,
        zClassDiPickCandidateEntry* candidate,
        const zVec3* segmentStart,
        const zVec3* segmentEnd
    )
    {
        candidate->scenePayload = 0;

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[0];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[4];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[7];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[3];
        if (CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon(
                candidate,
                segmentStart,
                segmentEnd,
                g_CZClass_DiFaceVertexScratch4,
                4,
                0
            )) {
            return 1;
        }

        // Each face only rewrites the scratch slots that differ from the previous face.
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[1];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[5];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[4];
        if (CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon(
                candidate,
                segmentStart,
                segmentEnd,
                g_CZClass_DiFaceVertexScratch4,
                4,
                0
            )) {
            return 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[5];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[2];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[6];
        if (CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon(
                candidate,
                segmentStart,
                segmentEnd,
                g_CZClass_DiFaceVertexScratch4,
                4,
                0
            )) {
            return 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[7];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[6];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[3];
        if (CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon(
                candidate,
                segmentStart,
                segmentEnd,
                g_CZClass_DiFaceVertexScratch4,
                4,
                0
            )) {
            return 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[0];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[3];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[1];
        if (CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon(
                candidate,
                segmentStart,
                segmentEnd,
                g_CZClass_DiFaceVertexScratch4,
                4,
                0
            )) {
            return 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[4];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[5];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[6];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[7];
        if (CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon(
                candidate,
                segmentStart,
                segmentEnd,
                g_CZClass_DiFaceVertexScratch4,
                4,
                0
            )) {
            return 1;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.trygetpolygonhitatqueryxz
     * @recoil-artifact defines .text recoil:function:0x4856d0: CZDisplayInstance::TryGetPolygonHitAtQueryXZ.
     *
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall TryGetPolygonHitAtQueryXZ(
        zClassDiPickCandidateEntry * candidate,
        const zVec3* polygonVertices,
        float queryX,
        float queryZ,
        int vertexCount
    )
    {
        int index = vertexCount - 1;
        const zVec3* previous = &polygonVertices[index];
        if ((queryX - previous->x) * (polygonVertices[0].z - previous->z)
                + (queryZ - previous->z) * (previous->x - polygonVertices[0].x)
            <= -0.0001) {
            return 0;
        }

        while (--index >= 0) {
            if ((queryX - polygonVertices[index].x) * (polygonVertices[index + 1].z - polygonVertices[index].z)
                    + (queryZ - polygonVertices[index].z) * (polygonVertices[index].x - polygonVertices[index + 1].x)
                <= -0.0001) {
                return 0;
            }
        }

        zMathVec3TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &candidate->surfaceNormal
        );

        if (candidate->surfaceNormal.y == 0.0) {
            candidate->hitPos.y = polygonVertices[0].y;
            return 1;
        }

        // Retail rounds through 1/normal.y slopes (as in AddFaceToPlayerProbeSampleBuckets), not one divide.
        const float invNormalY = 1.0f / candidate->surfaceNormal.y;
        const float xSlope = -candidate->surfaceNormal.x * invNormalY;
        const float zSlope = -candidate->surfaceNormal.z * invNormalY;
        candidate->hitPos.y = (queryZ - polygonVertices[0].z) * zSlope + (queryX - polygonVertices[0].x) * xSlope
            + polygonVertices[0].y;
        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildpickcandidateforsegmentvspolygon
     * @recoil-artifact defines .text recoil:function:0x4857f0: CZDisplayInstance::BuildPickCandidateForSegmentVsPolygon.
     *
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidateForSegmentVsPolygon(
        zClassDiPickCandidateEntry * candidate,
        const zVec3* segmentStart,
        const zVec3* segmentEnd,
        const zVec3* polygonVertices,
        int vertexCount,
        int cullBackface
    )
    {
        union {
            float f;
            unsigned int u;
        } startBits, endBits;
        zVec3 delta;

        zMathVec3TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &candidate->surfaceNormal
        );

        float endSide;
        zMath::Vec3Subtract(segmentEnd, polygonVertices, &delta);
        ZMTH_VECTOR_DOT(endSide, &delta, &candidate->surfaceNormal);
        endBits.f = endSide;
        if (cullBackface == 0 && endSide >= 0.0) {
            return 0;
        }

        float startSide;
        zMath::Vec3Subtract(segmentStart, polygonVertices, &delta);
        ZMTH_VECTOR_DOT(startSide, &delta, &candidate->surfaceNormal);
        startBits.f = startSide;
        if (((startBits.u ^ endBits.u) & 0x80000000u) == 0) {
            return 0;
        }

        const float t = startSide / (startSide - endSide);
        zMath::Vec3Subtract(segmentEnd, segmentStart, &delta);
        delta.x = t * delta.x;
        delta.y = t * delta.y;
        delta.z = t * delta.z;
        zMath::Vec3Add(segmentStart, &delta, &candidate->hitPos);

        int axis = 0;
        float maxAbs = (float)fabs(candidate->surfaceNormal.x);
        const float absY = (float)fabs(candidate->surfaceNormal.y);
        if (absY > maxAbs) {
            maxAbs = absY;
            axis = 1;
        }
        if ((float)fabs(candidate->surfaceNormal.z) > maxAbs) {
            axis = 2;
        }
        if (((float*)(&candidate->surfaceNormal))[axis] < 0.0f) {
            axis += 3;
        }

        int index = vertexCount - 1;
        int current;
        switch (axis) {
        case 1:
            if ((polygonVertices[index].x - polygonVertices[0].x) * (candidate->hitPos.z - polygonVertices[index].z)
                    + (polygonVertices[0].z - polygonVertices[index].z)
                        * (candidate->hitPos.x - polygonVertices[index].x)
                <= -0.0001) {
                return 0;
            }

            while (1) {
                current = index;
                --index;
                if (index < 0) {
                    break;
                }

                if ((polygonVertices[index].x - polygonVertices[current].x)
                            * (candidate->hitPos.z - polygonVertices[index].z)
                        + (polygonVertices[current].z - polygonVertices[index].z)
                            * (candidate->hitPos.x - polygonVertices[index].x)
                    <= -0.0001) {
                    return 0;
                }
            }
            break;
        case 0:
            if ((polygonVertices[index].z - polygonVertices[0].z) * (candidate->hitPos.y - polygonVertices[index].y)
                    + (polygonVertices[0].y - polygonVertices[index].y)
                        * (candidate->hitPos.z - polygonVertices[index].z)
                <= -0.0001) {
                return 0;
            }

            while (1) {
                current = index;
                --index;
                if (index < 0) {
                    break;
                }

                if ((polygonVertices[index].z - polygonVertices[current].z)
                            * (candidate->hitPos.y - polygonVertices[index].y)
                        + (polygonVertices[current].y - polygonVertices[index].y)
                            * (candidate->hitPos.z - polygonVertices[index].z)
                    <= -0.0001) {
                    return 0;
                }
            }
            break;
        case 2:
            if ((polygonVertices[index].y - polygonVertices[0].y) * (candidate->hitPos.x - polygonVertices[index].x)
                    + (polygonVertices[0].x - polygonVertices[index].x)
                        * (candidate->hitPos.y - polygonVertices[index].y)
                <= -0.0001) {
                return 0;
            }

            while (1) {
                current = index;
                --index;
                if (index < 0) {
                    break;
                }

                if ((polygonVertices[index].y - polygonVertices[current].y)
                            * (candidate->hitPos.x - polygonVertices[index].x)
                        + (polygonVertices[current].x - polygonVertices[index].x)
                            * (candidate->hitPos.y - polygonVertices[index].y)
                    <= -0.0001) {
                    return 0;
                }
            }
            break;
        case 4:
            if ((polygonVertices[0].x - polygonVertices[index].x) * (candidate->hitPos.z - polygonVertices[index].z)
                    + (polygonVertices[index].z - polygonVertices[0].z)
                        * (candidate->hitPos.x - polygonVertices[index].x)
                <= -0.0001) {
                return 0;
            }

            while (1) {
                current = index;
                --index;
                if (index < 0) {
                    break;
                }

                if ((polygonVertices[current].x - polygonVertices[index].x)
                            * (candidate->hitPos.z - polygonVertices[index].z)
                        + (polygonVertices[index].z - polygonVertices[current].z)
                            * (candidate->hitPos.x - polygonVertices[index].x)
                    <= -0.0001) {
                    return 0;
                }
            }
            break;
        case 3:
            if ((polygonVertices[0].z - polygonVertices[index].z) * (candidate->hitPos.y - polygonVertices[index].y)
                    + (polygonVertices[index].y - polygonVertices[0].y)
                        * (candidate->hitPos.z - polygonVertices[index].z)
                <= -0.0001) {
                return 0;
            }

            while (1) {
                current = index;
                --index;
                if (index < 0) {
                    break;
                }

                if ((polygonVertices[current].z - polygonVertices[index].z)
                            * (candidate->hitPos.y - polygonVertices[index].y)
                        + (polygonVertices[index].y - polygonVertices[current].y)
                            * (candidate->hitPos.z - polygonVertices[index].z)
                    <= -0.0001) {
                    return 0;
                }
            }
            break;
        case 5:
            if ((polygonVertices[0].y - polygonVertices[index].y) * (candidate->hitPos.x - polygonVertices[index].x)
                    + (polygonVertices[index].x - polygonVertices[0].x)
                        * (candidate->hitPos.y - polygonVertices[index].y)
                <= -0.0001) {
                return 0;
            }

            while (1) {
                current = index;
                --index;
                if (index < 0) {
                    break;
                }

                if ((polygonVertices[current].y - polygonVertices[index].y)
                            * (candidate->hitPos.x - polygonVertices[index].x)
                        + (polygonVertices[index].x - polygonVertices[current].x)
                            * (candidate->hitPos.y - polygonVertices[index].y)
                    <= -0.0001) {
                    return 0;
                }
            }
            break;
        }

        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildpickcandidateforsegmentvspolygonwithuv
     * @recoil-artifact defines .text recoil:function:0x485d10: CZDisplayInstance::BuildPickCandidateForSegmentVsPolygonWithUv.
     *
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidateForSegmentVsPolygonWithUv(
        zClassDiPickCandidateEntry * candidate,
        const zVec3* segmentStart,
        const zVec3* segmentEnd,
        const zVec3* polygonVertices,
        const zModel_PickFaceUvData* faceUvData,
        zVec2* outUv,
        int vertexCount,
        int cullBackface
    )
    {
        float maxAbs;
        int inside = 1;
        int dominantAxis;
        float startSide;
        int nextIndex;
        zVec2 vGrad;
        zVec3 scratch;
        float endSide;
        int edgeIndex;
        float absZ;
        zVec2 uGrad;
        float t;

        zMathVec3TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &candidate->surfaceNormal
        );

        zMath::Vec3Subtract(segmentEnd, polygonVertices, &scratch);
        ZMTH_VECTOR_DOT(endSide, &scratch, &candidate->surfaceNormal);
        if (cullBackface != 0 || endSide < 0.0) {
            zMath::Vec3Subtract(segmentStart, polygonVertices, &scratch);
            ZMTH_VECTOR_DOT(startSide, &scratch, &candidate->surfaceNormal);
            if ((*(int*)&endSide ^ *(int*)&startSide) & 0x80000000) {
                t = startSide / (startSide - endSide);
                zMath::Vec3Subtract(segmentEnd, segmentStart, &scratch);
                scratch.x = t * scratch.x;
                scratch.y = t * scratch.y;
                scratch.z = t * scratch.z;
                zMath::Vec3Add(segmentStart, &scratch, &candidate->hitPos);

                // endSide is dead here; retail reuses its stack home for |normal.y|.
                dominantAxis = 0;
                maxAbs = (float)fabs(candidate->surfaceNormal.x);
                endSide = (float)fabs(candidate->surfaceNormal.y);
                if (endSide > maxAbs) {
                    maxAbs = endSide;
                    dominantAxis = 1;
                }
                absZ = (float)fabs(candidate->surfaceNormal.z);
                if (absZ > maxAbs) {
                    dominantAxis = 2;
                }

                edgeIndex = vertexCount - 1;
                switch (dominantAxis) {
                case 0:
                    for (nextIndex = 0; edgeIndex >= 0 && inside; nextIndex = edgeIndex--) {
                        if (candidate->surfaceNormal.x < 0.0) {
                            scratch.z = polygonVertices[edgeIndex].y - polygonVertices[nextIndex].y;
                            scratch.y = polygonVertices[nextIndex].z - polygonVertices[edgeIndex].z;
                        } else {
                            scratch.z = polygonVertices[nextIndex].y - polygonVertices[edgeIndex].y;
                            scratch.y = polygonVertices[edgeIndex].z - polygonVertices[nextIndex].z;
                        }
                        inside = (candidate->hitPos.y - polygonVertices[edgeIndex].y) * scratch.y
                                + (candidate->hitPos.z - polygonVertices[edgeIndex].z) * scratch.z
                            > -0.0001;
                    }
                    if (!inside) {
                        return 0;
                    }
                    zMathSolveLinearGradient2D(
                        &uGrad.x,
                        &uGrad.y,
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
                        &vGrad.x,
                        &vGrad.y,
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
                    outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad.x
                        + (candidate->hitPos.z - polygonVertices[0].z) * uGrad.y + faceUvData->uvs[0].x;
                    outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad.x
                        + (candidate->hitPos.z - polygonVertices[0].z) * vGrad.y + faceUvData->uvs[0].y;
                    break;
                case 1:
                    for (nextIndex = 0; edgeIndex >= 0 && inside; nextIndex = edgeIndex--) {
                        if (candidate->surfaceNormal.y > 0.0) {
                            scratch.z = polygonVertices[edgeIndex].x - polygonVertices[nextIndex].x;
                            scratch.x = polygonVertices[nextIndex].z - polygonVertices[edgeIndex].z;
                        } else {
                            scratch.z = polygonVertices[nextIndex].x - polygonVertices[edgeIndex].x;
                            scratch.x = polygonVertices[edgeIndex].z - polygonVertices[nextIndex].z;
                        }
                        inside = (candidate->hitPos.x - polygonVertices[edgeIndex].x) * scratch.x
                                + (candidate->hitPos.z - polygonVertices[edgeIndex].z) * scratch.z
                            > -0.0001;
                    }
                    if (!inside) {
                        return 0;
                    }
                    zMathSolveLinearGradient2D(
                        &uGrad.x,
                        &uGrad.y,
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
                        &vGrad.x,
                        &vGrad.y,
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
                    outUv->x = (candidate->hitPos.z - polygonVertices[0].z) * uGrad.y
                        + (candidate->hitPos.x - polygonVertices[0].x) * uGrad.x + faceUvData->uvs[0].x;
                    outUv->y = (candidate->hitPos.z - polygonVertices[0].z) * vGrad.y
                        + (candidate->hitPos.x - polygonVertices[0].x) * vGrad.x + faceUvData->uvs[0].y;
                    break;
                case 2:
                    for (nextIndex = 0; edgeIndex >= 0 && inside; nextIndex = edgeIndex--) {
                        if (candidate->surfaceNormal.z > 0.0) {
                            scratch.x = polygonVertices[edgeIndex].y - polygonVertices[nextIndex].y;
                            scratch.y = polygonVertices[nextIndex].x - polygonVertices[edgeIndex].x;
                        } else {
                            scratch.x = polygonVertices[nextIndex].y - polygonVertices[edgeIndex].y;
                            scratch.y = polygonVertices[edgeIndex].x - polygonVertices[nextIndex].x;
                        }
                        inside = (candidate->hitPos.x - polygonVertices[edgeIndex].x) * scratch.x
                                + (candidate->hitPos.y - polygonVertices[edgeIndex].y) * scratch.y
                            > -0.0001;
                    }
                    if (!inside) {
                        return 0;
                    }
                    zMathSolveLinearGradient2D(
                        &uGrad.x,
                        &uGrad.y,
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
                        &vGrad.x,
                        &vGrad.y,
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
                    outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad.y
                        + (candidate->hitPos.x - polygonVertices[0].x) * uGrad.x + faceUvData->uvs[0].x;
                    outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad.y
                        + (candidate->hitPos.x - polygonVertices[0].x) * vGrad.x + faceUvData->uvs[0].y;
                    break;
                default:
                    return 0;
                }

                OptCatalogSetDamageMaskUv(outUv->x, outUv->y);
                return 1;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildpickcandidatesforsegmentbatchvspolygon
     * @recoil-artifact defines .text recoil:function:0x486290: CZDisplayInstance::BuildPickCandidatesForSegmentBatchVsPolygon.
     *
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentBatchVsPolygon(
        CZNodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        CZDisplayInstanceSegmentEndpoints * segmentEndpointsByBatch,
        int* activeMask,
        int segmentCount,
        zVec3* polygonVertices,
        zModel_PickFaceEntry* faceEntry
    )
    {
        union {
            float f;
            unsigned int u;
        } startBits, endBits;
        zVec3 scratch;
        int segmentIndex;
        int edgeIndex;
        CZDisplayInstanceSegmentEndpoints* segment;
        int dominantAxis;
        int localActive[24];
        float absZ;
        float startSide;
        float maxAbs;
        const zVec3* segmentEnd;
        float t;
        float endSide;
        int anyActive;
        int nextIndex;
        zVec3 normal;
        zVec3* hitPos;

        for (segmentIndex = segmentCount - 1; segmentIndex >= 0; --segmentIndex) {
            localActive[segmentIndex] = activeMask[segmentIndex];
        }

        zMathVec3TriangleNormal(&polygonVertices[0], &polygonVertices[1], &polygonVertices[2], &normal);

        anyActive = 0;
        segment = segmentEndpointsByBatch;
        for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex, ++segment) {
            segmentEnd = &segment->end;
            if (localActive[segmentIndex] != 0) {
                zMath::Vec3Subtract(segmentEnd, polygonVertices, &scratch);
                ZMTH_VECTOR_DOT(endSide, &scratch, &normal);
                endBits.f = endSide;
                if (faceEntry->doubleSided == 0 && endSide >= 0.0) {
                    localActive[segmentIndex] = 0;
                } else {
                    zMath::Vec3Subtract(&segment->start, polygonVertices, &scratch);
                    ZMTH_VECTOR_DOT(startSide, &scratch, &normal);
                    startBits.f = startSide;
                    if (((endBits.u ^ startBits.u) & 0x80000000u) == 0) {
                        localActive[segmentIndex] = 0;
                    } else {
                        anyActive = 1;
                        t = startSide / (startSide - endSide);
                        zMath::Vec3Subtract(segmentEnd, &segment->start, &scratch);
                        scratch.x = t * scratch.x;
                        scratch.y = t * scratch.y;
                        scratch.z = t * scratch.z;
                        zMath::Vec3Add(
                            &segment->start,
                            &scratch,
                            &outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .hitPos
                        );
                    }
                }
            }
        }

        if (anyActive == 0) {
            return 0;
        }

        // endSide is dead here; retail reuses its stack home for |normal.y|.
        dominantAxis = 0;
        maxAbs = (float)fabs(normal.x);
        endSide = (float)fabs(normal.y);
        if (endSide > maxAbs) {
            maxAbs = endSide;
            dominantAxis = 1;
        }
        absZ = (float)fabs(normal.z);
        if (absZ > maxAbs) {
            dominantAxis = 2;
        }

        anyActive = 1;
        edgeIndex = (int)(faceEntry->vertexCount) - 1;
        switch (dominantAxis) {
        case 0:
            for (nextIndex = 0; edgeIndex >= 0 && anyActive; nextIndex = edgeIndex--) {
                if (normal.x < 0.0) {
                    scratch.z = polygonVertices[edgeIndex].y - polygonVertices[nextIndex].y;
                    scratch.y = polygonVertices[nextIndex].z - polygonVertices[edgeIndex].z;
                } else {
                    scratch.z = polygonVertices[nextIndex].y - polygonVertices[edgeIndex].y;
                    scratch.y = polygonVertices[edgeIndex].z - polygonVertices[nextIndex].z;
                }
                anyActive = 0;
                for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                    if (localActive[segmentIndex] != 0) {
                        localActive[segmentIndex]
                            = (outCandidateBuffersBySegment[segmentIndex]
                                      .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                      .hitPos.y
                                  - polygonVertices[edgeIndex].y)
                                    * scratch.y
                                + (outCandidateBuffersBySegment[segmentIndex]
                                          .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                          .hitPos.z
                                      - polygonVertices[edgeIndex].z)
                                    * scratch.z
                            > -0.0001;
                        if (localActive[segmentIndex] != 0) {
                            anyActive = 1;
                        }
                    }
                }
            }
            if (anyActive != 0) {
                for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                    if (localActive[segmentIndex] != 0
                        && outCandidateBuffersBySegment[segmentIndex].candidateCount < kMaxPickCandidates) {
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .surfaceNormal = normal;
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .node = candidateOwner;
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .scenePayload = faceEntry->scenePayload;
                        ++outCandidateBuffersBySegment[segmentIndex].candidateCount;
                    }
                }
            }
            return anyActive;
        case 1:
            for (nextIndex = 0; edgeIndex >= 0 && anyActive; nextIndex = edgeIndex--) {
                if (normal.y > 0.0) {
                    scratch.z = polygonVertices[edgeIndex].x - polygonVertices[nextIndex].x;
                    scratch.x = polygonVertices[nextIndex].z - polygonVertices[edgeIndex].z;
                } else {
                    scratch.z = polygonVertices[nextIndex].x - polygonVertices[edgeIndex].x;
                    scratch.x = polygonVertices[edgeIndex].z - polygonVertices[nextIndex].z;
                }
                anyActive = 0;
                for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                    if (localActive[segmentIndex] != 0) {
                        localActive[segmentIndex]
                            = (outCandidateBuffersBySegment[segmentIndex]
                                      .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                      .hitPos.x
                                  - polygonVertices[edgeIndex].x)
                                    * scratch.x
                                + (outCandidateBuffersBySegment[segmentIndex]
                                          .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                          .hitPos.z
                                      - polygonVertices[edgeIndex].z)
                                    * scratch.z
                            > -0.0001;
                        if (localActive[segmentIndex] != 0) {
                            anyActive = 1;
                        }
                    }
                }
            }
            if (anyActive != 0) {
                for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                    if (localActive[segmentIndex] != 0
                        && outCandidateBuffersBySegment[segmentIndex].candidateCount < kMaxPickCandidates) {
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .surfaceNormal = normal;
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .node = candidateOwner;
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .scenePayload = faceEntry->scenePayload;
                        ++outCandidateBuffersBySegment[segmentIndex].candidateCount;
                    }
                }
            }
            return anyActive;
        case 2:
            for (nextIndex = 0; edgeIndex >= 0 && anyActive; nextIndex = edgeIndex--) {
                if (normal.z > 0.0) {
                    scratch.x = polygonVertices[edgeIndex].y - polygonVertices[nextIndex].y;
                    scratch.y = polygonVertices[nextIndex].x - polygonVertices[edgeIndex].x;
                } else {
                    scratch.x = polygonVertices[nextIndex].y - polygonVertices[edgeIndex].y;
                    scratch.y = polygonVertices[edgeIndex].x - polygonVertices[nextIndex].x;
                }
                anyActive = 0;
                for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                    if (localActive[segmentIndex] != 0) {
                        localActive[segmentIndex]
                            = (outCandidateBuffersBySegment[segmentIndex]
                                      .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                      .hitPos.x
                                  - polygonVertices[edgeIndex].x)
                                    * scratch.x
                                + (outCandidateBuffersBySegment[segmentIndex]
                                          .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                          .hitPos.y
                                      - polygonVertices[edgeIndex].y)
                                    * scratch.y
                            > -0.0001;
                        if (localActive[segmentIndex] != 0) {
                            anyActive = 1;
                        }
                    }
                }
            }
            if (anyActive != 0) {
                for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                    if (localActive[segmentIndex] != 0
                        && outCandidateBuffersBySegment[segmentIndex].candidateCount < kMaxPickCandidates) {
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .surfaceNormal = normal;
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .node = candidateOwner;
                        outCandidateBuffersBySegment[segmentIndex]
                            .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                            .scenePayload = faceEntry->scenePayload;
                        ++outCandidateBuffersBySegment[segmentIndex].candidateCount;
                    }
                }
            }
            return anyActive;
        default:
            return 0;
        }
    }

    /**
     * Function modeled here:
     * CZDisplayInstance::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence for the expanded raycast/filter runtime slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
        CZNodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        CZDisplayInstanceSegmentEndpoints * segmentEndpointsByBatch,
        int* activeMask,
        int segmentCount,
        zVec3* polygonVertices,
        zModel_PickFaceUvData* faceUvData,
        zVec2* scratchUv,
        zModel_PickFaceEntry* faceEntry
    )
    {
        union {
            float f;
            unsigned int u;
        } startBits, endBits;
        float absZ;
        int dominantAxis;
        float endSide;
        float startSide;
        const zVec3* segmentEnd;
        int anyActive;
        zVec2 uGrad;
        CZDisplayInstanceSegmentEndpoints* segment;
        int edgeIndex;
        int nextIndex;
        int segmentIndex;
        float maxAbs;
        zVec3 scratch;
        zVec3 normal;
        float t;
        zVec2 vGrad;
        int localActive[24];
        zVec3* hitPos;

        for (segmentIndex = segmentCount - 1; segmentIndex >= 0; --segmentIndex) {
            localActive[segmentIndex] = activeMask[segmentIndex];
        }

        zMathVec3TriangleNormal(&polygonVertices[0], &polygonVertices[1], &polygonVertices[2], &normal);

        anyActive = 0;
        segment = segmentEndpointsByBatch;
        for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex, ++segment) {
            segmentEnd = &segment->end;
            if (localActive[segmentIndex] != 0) {
                zMath::Vec3Subtract(segmentEnd, polygonVertices, &scratch);
                ZMTH_VECTOR_DOT(endSide, &scratch, &normal);
                endBits.f = endSide;
                if (faceEntry->doubleSided == 0 && endSide >= 0.0) {
                    localActive[segmentIndex] = 0;
                } else {
                    zMath::Vec3Subtract(&segment->start, polygonVertices, &scratch);
                    ZMTH_VECTOR_DOT(startSide, &scratch, &normal);
                    startBits.f = startSide;
                    if (((endBits.u ^ startBits.u) & 0x80000000u) == 0) {
                        localActive[segmentIndex] = 0;
                    } else {
                        anyActive = 1;
                        t = startSide / (startSide - endSide);
                        zMath::Vec3Subtract(segmentEnd, &segment->start, &scratch);
                        scratch.x = t * scratch.x;
                        scratch.y = t * scratch.y;
                        scratch.z = t * scratch.z;
                        zMath::Vec3Add(
                            &segment->start,
                            &scratch,
                            &outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .hitPos
                        );
                    }
                }
            }
        }

        if (anyActive != 0) {
            // endSide is dead here; retail reuses its stack home for |normal.y|.
            dominantAxis = 0;
            maxAbs = (float)fabs(normal.x);
            endSide = (float)fabs(normal.y);
            if (endSide > maxAbs) {
                maxAbs = endSide;
                dominantAxis = 1;
            }
            absZ = (float)fabs(normal.z);
            if (absZ > maxAbs) {
                dominantAxis = 2;
            }

            anyActive = 1;
            edgeIndex = (int)(faceEntry->vertexCount) - 1;
            switch (dominantAxis) {
            case 0:
                for (nextIndex = 0; edgeIndex >= 0 && anyActive; nextIndex = edgeIndex--) {
                    if (normal.x < 0.0) {
                        scratch.z = polygonVertices[edgeIndex].y - polygonVertices[nextIndex].y;
                        scratch.y = polygonVertices[nextIndex].z - polygonVertices[edgeIndex].z;
                    } else {
                        scratch.z = polygonVertices[nextIndex].y - polygonVertices[edgeIndex].y;
                        scratch.y = polygonVertices[edgeIndex].z - polygonVertices[nextIndex].z;
                    }
                    anyActive = 0;
                    for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                        if (localActive[segmentIndex] != 0) {
                            localActive[segmentIndex]
                                = (outCandidateBuffersBySegment[segmentIndex]
                                          .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                          .hitPos.y
                                      - polygonVertices[edgeIndex].y)
                                        * scratch.y
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.z
                                          - polygonVertices[edgeIndex].z)
                                        * scratch.z
                                > -0.0001;
                            if (localActive[segmentIndex] != 0) {
                                anyActive = 1;
                            }
                        }
                    }
                }
                if (anyActive != 0) {
                    if (OptCatalogIsDamageMaskEnabled() != 0) {
                        zMathSolveLinearGradient2D(
                            &uGrad.x,
                            &uGrad.y,
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
                            &vGrad.x,
                            &vGrad.y,
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
                    }
                    for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                        if (localActive[segmentIndex] != 0
                            && outCandidateBuffersBySegment[segmentIndex].candidateCount < kMaxPickCandidates) {
                            if (OptCatalogIsDamageMaskEnabled() != 0) {
                                scratchUv->x
                                    = (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.y
                                          - polygonVertices[0].y)
                                        * uGrad.x
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.z
                                          - polygonVertices[0].z)
                                        * uGrad.y
                                    + faceUvData->uvs[0].x;
                                scratchUv->y
                                    = (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.y
                                          - polygonVertices[0].y)
                                        * vGrad.x
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.z
                                          - polygonVertices[0].z)
                                        * vGrad.y
                                    + faceUvData->uvs[0].y;
                                OptCatalogSetDamageMaskUv(scratchUv->x, scratchUv->y);
                            }
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .surfaceNormal = normal;
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .node = candidateOwner;
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .scenePayload = faceEntry->scenePayload;
                            ++outCandidateBuffersBySegment[segmentIndex].candidateCount;
                        }
                    }
                }
                break;
            case 1:
                for (nextIndex = 0; edgeIndex >= 0 && anyActive; nextIndex = edgeIndex--) {
                    if (normal.y > 0.0) {
                        scratch.z = polygonVertices[edgeIndex].x - polygonVertices[nextIndex].x;
                        scratch.x = polygonVertices[nextIndex].z - polygonVertices[edgeIndex].z;
                    } else {
                        scratch.z = polygonVertices[nextIndex].x - polygonVertices[edgeIndex].x;
                        scratch.x = polygonVertices[edgeIndex].z - polygonVertices[nextIndex].z;
                    }
                    anyActive = 0;
                    for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                        if (localActive[segmentIndex] != 0) {
                            localActive[segmentIndex]
                                = (outCandidateBuffersBySegment[segmentIndex]
                                          .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                          .hitPos.x
                                      - polygonVertices[edgeIndex].x)
                                        * scratch.x
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.z
                                          - polygonVertices[edgeIndex].z)
                                        * scratch.z
                                > -0.0001;
                            if (localActive[segmentIndex] != 0) {
                                anyActive = 1;
                            }
                        }
                    }
                }
                if (anyActive != 0) {
                    if (OptCatalogIsDamageMaskEnabled() != 0) {
                        zMathSolveLinearGradient2D(
                            &uGrad.x,
                            &uGrad.y,
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
                            &vGrad.x,
                            &vGrad.y,
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
                    }
                    for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                        if (localActive[segmentIndex] != 0
                            && outCandidateBuffersBySegment[segmentIndex].candidateCount < kMaxPickCandidates) {
                            if (OptCatalogIsDamageMaskEnabled() != 0) {
                                scratchUv->x
                                    = (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.z
                                          - polygonVertices[0].z)
                                        * uGrad.y
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.x
                                          - polygonVertices[0].x)
                                        * uGrad.x
                                    + faceUvData->uvs[0].x;
                                scratchUv->y
                                    = (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.z
                                          - polygonVertices[0].z)
                                        * vGrad.y
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.x
                                          - polygonVertices[0].x)
                                        * vGrad.x
                                    + faceUvData->uvs[0].y;
                                OptCatalogSetDamageMaskUv(scratchUv->x, scratchUv->y);
                            }
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .surfaceNormal = normal;
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .node = candidateOwner;
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .scenePayload = faceEntry->scenePayload;
                            ++outCandidateBuffersBySegment[segmentIndex].candidateCount;
                        }
                    }
                }
                break;
            case 2:
                for (nextIndex = 0; edgeIndex >= 0 && anyActive; nextIndex = edgeIndex--) {
                    if (normal.z > 0.0) {
                        scratch.x = polygonVertices[edgeIndex].y - polygonVertices[nextIndex].y;
                        scratch.y = polygonVertices[nextIndex].x - polygonVertices[edgeIndex].x;
                    } else {
                        scratch.x = polygonVertices[nextIndex].y - polygonVertices[edgeIndex].y;
                        scratch.y = polygonVertices[edgeIndex].x - polygonVertices[nextIndex].x;
                    }
                    anyActive = 0;
                    for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                        if (localActive[segmentIndex] != 0) {
                            localActive[segmentIndex]
                                = (outCandidateBuffersBySegment[segmentIndex]
                                          .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                          .hitPos.x
                                      - polygonVertices[edgeIndex].x)
                                        * scratch.x
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.y
                                          - polygonVertices[edgeIndex].y)
                                        * scratch.y
                                > -0.0001;
                            if (localActive[segmentIndex] != 0) {
                                anyActive = 1;
                            }
                        }
                    }
                }
                if (anyActive != 0) {
                    if (OptCatalogIsDamageMaskEnabled() != 0) {
                        zMathSolveLinearGradient2D(
                            &uGrad.x,
                            &uGrad.y,
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
                            &vGrad.x,
                            &vGrad.y,
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
                    }
                    for (segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
                        if (localActive[segmentIndex] != 0
                            && outCandidateBuffersBySegment[segmentIndex].candidateCount < kMaxPickCandidates) {
                            if (OptCatalogIsDamageMaskEnabled() != 0) {
                                scratchUv->x
                                    = (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.y
                                          - polygonVertices[0].y)
                                        * uGrad.y
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.x
                                          - polygonVertices[0].x)
                                        * uGrad.x
                                    + faceUvData->uvs[0].x;
                                scratchUv->y
                                    = (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.y
                                          - polygonVertices[0].y)
                                        * vGrad.y
                                    + (outCandidateBuffersBySegment[segmentIndex]
                                              .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                              .hitPos.x
                                          - polygonVertices[0].x)
                                        * vGrad.x
                                    + faceUvData->uvs[0].y;
                                OptCatalogSetDamageMaskUv(scratchUv->x, scratchUv->y);
                            }
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .surfaceNormal = normal;
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .node = candidateOwner;
                            outCandidateBuffersBySegment[segmentIndex]
                                .entries[outCandidateBuffersBySegment[segmentIndex].candidateCount]
                                .scenePayload = faceEntry->scenePayload;
                            ++outCandidateBuffersBySegment[segmentIndex].candidateCount;
                        }
                    }
                }
                break;
            }
        }

        return anyActive;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.filterregionsagainstpolygon
     * @recoil-artifact defines .text recoil:function:0x487350: CZDisplayInstance::FilterRegionsAgainstPolygon.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     * Contract note: when execution reaches the vertex batch, its count N
     * (faceData->vertexCount) must satisfy 1 <= N <= 0x400. The selected source must
     * contain N initialized zVec3 elements (including when the blend/morph
     * branch prepares g_zModel_SharedVec3ScratchA), and the shared transform
     * destination must provide N writable elements. The source and destination
     * ranges must not overlap; the batch identity path uses memcpy. Current
     * matrix-stack slots must satisfy the shared batch helper's contract.
     * For every processed face, its low-byte vertex count K must satisfy
     * 1 <= K <= 0x40. Its index array must contain K readable indices, each in
     * [0, N), and the face scratch must provide K writable zVec3 elements.
     * These are caller/data preconditions, not checks performed here.
     */
    void __fastcall FilterRegionsAgainstPolygon(
        CZNodePartial * candidateOwner,
        zModel_PickFaceData * faceData,
        CZDisplayInstanceSegmentEndpoints * segmentEndpointsByBatch,
        int* activeMask,
        int segmentCount,
        PlayerProbeSampleCandidateBuffer* outCandidateBuffersBySegment
    )
    {
        if (faceData == 0 || faceData->faceCount == 0) {
            return;
        }

        const zVec3* vertices;
        if ((faceData->flags & 0x08) != 0 && faceData->morphWeight != 0.0 && faceData->morphVertexCount != 0) {
            zMathVec3ArrayAddScaled(
                g_zModel_SharedVec3ScratchA,
                faceData->baseVertices,
                faceData->morphVertices,
                faceData->morphVertexCount,
                faceData->morphWeight
            );
            vertices = g_zModel_SharedVec3ScratchA;
        } else {
            vertices = faceData->baseVertices;
        }

        ZMTH_MAT_TRANSFORM_POINT_BATCH(vertices, g_zModel_SharedVec3ScratchB, faceData->vertexCount);

        zVec2 scratchUv;
        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            int vertexCount = (int)(faceData->faces[faceIndex].vertexCount);
            const int* vertexIndices = faceData->faces[faceIndex].vertexIndices;
            zVec3* faceVertex = g_CZClass_DiFaceVertexScratch4;
            const zVec3* transformed = g_zModel_SharedVec3ScratchB;
            do {
                *faceVertex++ = transformed[*vertexIndices++];
            } while (--vertexCount != 0);

            if ((faceData->faces[faceIndex].scenePayload->flags & kPickFaceBatchDamageMaskUvFlag) != 0) {
                BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
                    candidateOwner,
                    outCandidateBuffersBySegment,
                    segmentEndpointsByBatch,
                    activeMask,
                    segmentCount,
                    g_CZClass_DiFaceVertexScratch4,
                    faceData->faces[faceIndex].faceUvData,
                    &scratchUv,
                    &faceData->faces[faceIndex]
                );
            } else {
                BuildPickCandidatesForSegmentBatchVsPolygon(
                    candidateOwner,
                    outCandidateBuffersBySegment,
                    segmentEndpointsByBatch,
                    activeMask,
                    segmentCount,
                    g_CZClass_DiFaceVertexScratch4,
                    &faceData->faces[faceIndex]
                );
            }
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.filterregionsagainstpolygonwithdamagemaskuv
     * @recoil-artifact defines .text recoil:function:0x487540: CZDisplayInstance::FilterRegionsAgainstPolygonWithDamageMaskUv.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsAgainstPolygonWithDamageMaskUv(
        CZNodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        CZDisplayInstanceSegmentEndpoints * segmentEndpointsByBatch,
        int* activeMask,
        int segmentCount,
        const zBBoxCorners* bboxCorners
    )
    {
        zModel_PickFaceEntry faceEntry;
        faceEntry.scenePayload = 0;
        faceEntry.doubleSided = 0;
        faceEntry.vertexCount = 4;

        int result = 0;
        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[0];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[4];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[7];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[3];
        if (BuildPickCandidatesForSegmentBatchVsPolygon(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                g_CZClass_DiFaceVertexScratch4,
                &faceEntry
            )
            != 0) {
            result = 1;
        }

        // Each face only rewrites the scratch slots that differ from the previous face.
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[1];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[5];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[4];
        if (BuildPickCandidatesForSegmentBatchVsPolygon(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                g_CZClass_DiFaceVertexScratch4,
                &faceEntry
            )
            != 0) {
            result = 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[1];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[2];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[6];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[5];
        if (BuildPickCandidatesForSegmentBatchVsPolygon(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                g_CZClass_DiFaceVertexScratch4,
                &faceEntry
            )
            != 0) {
            result = 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[2];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[3];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[7];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[6];
        if (BuildPickCandidatesForSegmentBatchVsPolygon(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                g_CZClass_DiFaceVertexScratch4,
                &faceEntry
            )
            != 0) {
            result = 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[0];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[2];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[1];
        if (BuildPickCandidatesForSegmentBatchVsPolygon(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                g_CZClass_DiFaceVertexScratch4,
                &faceEntry
            )
            != 0) {
            result = 1;
        }

        g_CZClass_DiFaceVertexScratch4[0] = bboxCorners->corners[4];
        g_CZClass_DiFaceVertexScratch4[1] = bboxCorners->corners[5];
        g_CZClass_DiFaceVertexScratch4[2] = bboxCorners->corners[6];
        g_CZClass_DiFaceVertexScratch4[3] = bboxCorners->corners[7];
        if (BuildPickCandidatesForSegmentBatchVsPolygon(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                g_CZClass_DiFaceVertexScratch4,
                &faceEntry
            )
            != 0) {
            result = 1;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.filterregionsagainstmeshfaces
     * @recoil-artifact defines .text recoil:function:0x487900: CZDisplayInstance::FilterRegionsAgainstMeshFaces.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsAgainstMeshFaces(zVec3 * meshVertices, int faceCount)
    {
        g_zModel_PointInPolygonVertexCount = 0;
        if (faceCount > 0x40) {
            return 0;
        }

        int nextIndex = 0;
        for (int vertexIndex = faceCount - 1; vertexIndex >= 0; --vertexIndex) {
            g_zModel_PointInPolygonVertices[vertexIndex] = meshVertices[vertexIndex];
            g_zModel_PointInPolygonEdgeNormals[vertexIndex].z = meshVertices[vertexIndex].x - meshVertices[nextIndex].x;
            g_zModel_PointInPolygonEdgeNormals[vertexIndex].x = meshVertices[nextIndex].z - meshVertices[vertexIndex].z;
            g_zModel_PointInPolygonEdgeNormals[vertexIndex].y = 0.0f;
            zMath::Vec3Normalize(&g_zModel_PointInPolygonEdgeNormals[vertexIndex]);
            nextIndex = vertexIndex;
        }

        g_zModel_PointInPolygonVertexCount = faceCount;
        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.filterregionsagainsthexahedronfaces
     * @recoil-artifact defines .text recoil:function:0x4879c0: CZDisplayInstance::FilterRegionsAgainstHexahedronFaces.
     * @recoil-match byte
     *
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsAgainstHexahedronFaces(zVec3 * center, float radius)
    {
        zVec3* vertex = g_zModel_PointInPolygonVertices;
        zVec3* edgeNormal = g_zModel_PointInPolygonEdgeNormals;

        for (int vertexIndex = 0; vertexIndex < g_zModel_PointInPolygonVertexCount;
            ++vertexIndex, ++vertex, ++edgeNormal) {
            const float distance = (center->x - vertex->x) * edgeNormal->x + (center->z - vertex->z) * edgeNormal->z;
            if (distance < radius) {
                return 0;
            }
        }

        return 1;
    }
} // namespace CZDisplayInstance
