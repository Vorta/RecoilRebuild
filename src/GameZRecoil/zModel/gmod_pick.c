// zModel compilation unit between gmod_const.c and gmod_light.c, inferred from
// the retail object boundary [0x484960, 0x487a30): its .rdata pooled constants
// [0x4d2b08, 0x4d2b28) repeat double 0.0, float 0.0 and 1.0f that gmod_const.c
// pools separately, and the 1998-07-21 demo links it after gmod_matl.c, apart
// from gmod_const.c. Original filename unresolved; gmod_pick.c is a
// provisional name (2026-10-02).

#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"

#include <math.h>
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

namespace zMath
{
    /**
     * Purpose: Inline-function spelling of the reviewed vector-dot island for
     * this unit's consumers. VC5 binds simple variable arguments to their own
     * homes and address arguments to inline-parameter homes, which the capturing
     * ZMTH_VECTOR_DOT cannot express. Original header ownership is unrecovered.
     * Original inline helper evidence: no standalone retail function; retail
     * 0x4857f0 and 0x485d10 load the surface normal from the candidate home and
     * &delta/&scratch from an inline-parameter home (TU-resident reconstruction
     * model, as zmth_main.c).
     */
    inline float Vec3Dot(const zVec3* left, const zVec3* right)
    {
        float result;
        ZMTH_VECTOR_DOT_BOUND(result, left, right);
        return result;
    }
} // namespace zMath

/*
 * Address-backed gmod_const.c function contribution in natural retail order.
 */

namespace
{
    const int kMaxPickCandidates = 0x20;
    const unsigned short kPickFaceBatchDamageMaskUvFlag = 0x0100;
    const unsigned short kPickFaceTexturedDamageMaskFlag = 0x0200;
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
     * @recoil-match byte
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
        zVec3 edgeNormal;
        int index = vertexCount - 1;
        int current = 0;
        edgeNormal.x = polygonVertices[current].z - polygonVertices[index].z;
        edgeNormal.z = polygonVertices[index].x - polygonVertices[current].x;

        // Retail rotates this loop: the closing-edge test is a separate guard (0x4856d0..0x485712), and the
        // walked edges keep edgeNormal.x on the x87 stack while edgeNormal.z lives in its frame slot.
        for (;;) {
            if ((queryX - polygonVertices[index].x) * edgeNormal.x + (queryZ - polygonVertices[index].z) * edgeNormal.z
                <= -0.0001) {
                return 0;
            }

            current = index;
            index--;
            if (index < 0) {
                break;
            }

            edgeNormal.x = polygonVertices[current].z - polygonVertices[index].z;
            edgeNormal.z = polygonVertices[index].x - polygonVertices[current].x;
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
        endSide = zMath::Vec3Dot(&delta, &candidate->surfaceNormal);
        endBits.f = endSide;
        if (cullBackface == 0 && endSide >= 0.0) {
            return 0;
        }

        float startSide;
        zMath::Vec3Subtract(segmentStart, polygonVertices, &delta);
        startSide = zMath::Vec3Dot(&delta, &candidate->surfaceNormal);
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
        int current = 0;
        // Retail rotates each edge loop like TryGetPolygonHitAtQueryXZ: the first edge test is peeled and the
        // two edge-normal components stay x87 register variables (fmul st(n), popped after the compare).
        zVec3 edgeNormal;
        switch (axis) {
        case 1:
            edgeNormal.z = polygonVertices[index].x - polygonVertices[current].x;
            edgeNormal.x = polygonVertices[current].z - polygonVertices[index].z;
            for (;;) {
                if ((candidate->hitPos.z - polygonVertices[index].z) * edgeNormal.z
                        + (candidate->hitPos.x - polygonVertices[index].x) * edgeNormal.x
                    <= -0.0001) {
                    return 0;
                }

                current = index;
                index--;
                if (index < 0) {
                    break;
                }

                edgeNormal.z = polygonVertices[index].x - polygonVertices[current].x;
                edgeNormal.x = polygonVertices[current].z - polygonVertices[index].z;
            }
            break;
        case 0:
            edgeNormal.z = polygonVertices[current].y - polygonVertices[index].y;
            edgeNormal.y = polygonVertices[index].z - polygonVertices[current].z;
            for (;;) {
                if ((candidate->hitPos.y - polygonVertices[index].y) * edgeNormal.y
                        + (candidate->hitPos.z - polygonVertices[index].z) * edgeNormal.z
                    <= -0.0001) {
                    return 0;
                }

                current = index;
                index--;
                if (index < 0) {
                    break;
                }

                edgeNormal.z = polygonVertices[current].y - polygonVertices[index].y;
                edgeNormal.y = polygonVertices[index].z - polygonVertices[current].z;
            }
            break;
        case 2:
            edgeNormal.x = polygonVertices[index].y - polygonVertices[current].y;
            edgeNormal.y = polygonVertices[current].x - polygonVertices[index].x;
            for (;;) {
                if ((candidate->hitPos.x - polygonVertices[index].x) * edgeNormal.x
                        + (candidate->hitPos.y - polygonVertices[index].y) * edgeNormal.y
                    <= -0.0001) {
                    return 0;
                }

                current = index;
                index--;
                if (index < 0) {
                    break;
                }

                edgeNormal.x = polygonVertices[index].y - polygonVertices[current].y;
                edgeNormal.y = polygonVertices[current].x - polygonVertices[index].x;
            }
            break;
        case 4:
            edgeNormal.z = polygonVertices[current].x - polygonVertices[index].x;
            edgeNormal.x = polygonVertices[index].z - polygonVertices[current].z;
            for (;;) {
                if ((candidate->hitPos.x - polygonVertices[index].x) * edgeNormal.x
                        + (candidate->hitPos.z - polygonVertices[index].z) * edgeNormal.z
                    <= -0.0001) {
                    return 0;
                }

                current = index;
                index--;
                if (index < 0) {
                    break;
                }

                edgeNormal.z = polygonVertices[current].x - polygonVertices[index].x;
                edgeNormal.x = polygonVertices[index].z - polygonVertices[current].z;
            }
            break;
        case 3:
            edgeNormal.z = polygonVertices[index].y - polygonVertices[current].y;
            edgeNormal.y = polygonVertices[current].z - polygonVertices[index].z;
            for (;;) {
                if ((candidate->hitPos.y - polygonVertices[index].y) * edgeNormal.y
                        + (candidate->hitPos.z - polygonVertices[index].z) * edgeNormal.z
                    <= -0.0001) {
                    return 0;
                }

                current = index;
                index--;
                if (index < 0) {
                    break;
                }

                edgeNormal.z = polygonVertices[index].y - polygonVertices[current].y;
                edgeNormal.y = polygonVertices[current].z - polygonVertices[index].z;
            }
            break;
        case 5:
            edgeNormal.x = polygonVertices[current].y - polygonVertices[index].y;
            edgeNormal.y = polygonVertices[index].x - polygonVertices[current].x;
            for (;;) {
                if ((candidate->hitPos.x - polygonVertices[index].x) * edgeNormal.x
                        + (candidate->hitPos.y - polygonVertices[index].y) * edgeNormal.y
                    <= -0.0001) {
                    return 0;
                }

                current = index;
                index--;
                if (index < 0) {
                    break;
                }

                edgeNormal.x = polygonVertices[current].y - polygonVertices[index].y;
                edgeNormal.y = polygonVertices[index].x - polygonVertices[current].x;
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
        endSide = zMath::Vec3Dot(&scratch, &candidate->surfaceNormal);
        if (cullBackface != 0 || endSide < 0.0) {
            zMath::Vec3Subtract(segmentStart, polygonVertices, &scratch);
            startSide = zMath::Vec3Dot(&scratch, &candidate->surfaceNormal);
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
     * Returns nothing: the only caller (FilterRegionsAgainstPolygon) discards the
     * result, and retail leaves EAX unset on the commit paths.
     */
    void __fastcall BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
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
