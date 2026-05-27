#include "GameZRecoil/zModel/zModel.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClipAlt.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGeometry/zGeometry.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_zModel_VertexShadingEnabled = 0;

namespace {
const double kVisibleContributionThreshold = 1.0 / 255.0;

zVec3 TransformPointByCurrentMatrix(const zVec3 *point) {
    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    zVec3 out = {0};
    out.x = point->x * matrix->xx + point->y * matrix->yx + point->z * matrix->zx + matrix->posX;
    out.y = point->x * matrix->xy + point->y * matrix->yy + point->z * matrix->zy + matrix->posY;
    out.z = point->x * matrix->xz + point->y * matrix->yz + point->z * matrix->zz + matrix->posZ;
    return out;
}

bool ModelGraphicsFlagBit0Enabled() {
    const int *graphicsFlags =
        static_cast<const int *>(g_zModel_GraphicsFlagsOption);
    return graphicsFlags != 0 && ((*graphicsFlags & 1) != 0);
}

int TextureWrapExtent(const zModel_TextureScrollInfoPartial *textureInfo, bool uAxis) {
    const int baseExtent = g_zVideo_ActiveRendererPath != 0 ? 0x80 : 0x800;
    const unsigned char shift = uAxis ? textureInfo->wrapShiftU : textureInfo->wrapShiftV;
    return static_cast<int>(static_cast<unsigned int>(baseExtent) >> shift);
}

int ComputeWrapCorrection(float minValue, float maxValue, int wrapExtent) {
    const int minFloor = static_cast<int>(floor(minValue));
    const int maxCeil = static_cast<int>(ceil(maxValue));
    if (minFloor <= -wrapExtent) {
        return wrapExtent - maxCeil;
    }
    if (maxCeil >= wrapExtent) {
        return -(minFloor + wrapExtent);
    }
    return 0;
}

void ApplyUvCorrection(zModel_Uv *uvs, int uvCount, int correctionU,
                       int correctionV) {
    if (correctionU == 0 && correctionV == 0) {
        return;
    }

    for (int i = 0; i < uvCount; ++i) {
        uvs[i].u += static_cast<float>(correctionU);
        uvs[i].v += static_cast<float>(correctionV);
    }
}

bool ProjectedPointInClipBounds(const zProjectedPoint &point) {
    return !(point.x < g_zVideo_ProjectClipLeft) && !(point.y < g_zVideo_ProjectClipTop) &&
           !(point.x > g_zVideo_ProjectClipRight) && !(point.y > g_zVideo_ProjectClipBottom);
}

typedef void (RECOIL_FASTCALL *DrawPointColor16Proc)(zProjectedPoint *point,
                                                     unsigned int packedColor16,
                                                     int pointCount);
typedef void (RECOIL_FASTCALL *SubmitPolyFlatColor16Proc)(zVideo_XyzVertex *vertices,
                                                          unsigned int packedColor16, int alpha,
                                                          int renderParam, int vertexCount,
                                                          int queueMode);
typedef void (RECOIL_FASTCALL *SubmitPolyColorAttrProc)(
    zVideo_XyzVertex *vertices, unsigned int packedColor16, zVideo_ColorRgbFloat *baseColor,
    float *attr1, float *attr0, float *attr2, int alpha, int vertexCount,
    unsigned int renderParam, int queueMode);
typedef void (RECOIL_FASTCALL *SubmitPolyRenderClassProc)(
    zVideo_XyzVertex *vertices, zVideo_TexCoord *texCoords, int vertexCount,
    zVideo_RenderClass *renderClass, unsigned int renderParam, float alpha, int queueMode);
typedef void (RECOIL_FASTCALL *SubmitPolygonProc)(
    zVideo_XyzVertex *vertices, zVideo_TexCoord *uvPairs, float *attr1, float *attr0,
    float *attr2, int vertexCount, zVideo_RenderClass *renderClass, unsigned int renderParam,
    float alpha, int queueMode);
typedef void (RECOIL_FASTCALL *SubmitPolygonLitProc)(
    zVideo_XyzVertex *vertices, zVideo_TexCoord *uvPairs, float *attr1, float *attr0,
    float *attr2, int vertexCount, zVideo_RenderClass *renderClass, unsigned int renderParam,
    float alpha, int queueMode);

zDiPartial *NodeDisplayInstance(zClass_NodePartial *node) {
    return node != 0 ? (zDiPartial *)(node->userDataOrDiRef) : 0;
}

void PrepareTransformedVertices(zDiPartial *di) {
    if (di->verts == 0 || di->vertCount <= 0) {
        return;
    }

    if ((di->flags & 8) != 0 && di->blendVerts != 0 && di->blendVertCount > 0 &&
        di->blendScale != 0.0f) {
        zMath_Vec3Array_AddScaled(g_zModel_TransformedVerts, di->verts, di->blendVerts,
                                  di->blendVertCount, di->blendScale);
        if (di->vertCount > di->blendVertCount) {
            memcpy(&g_zModel_TransformedVerts[di->blendVertCount], &di->verts[di->blendVertCount],
                   static_cast<size_t>(di->vertCount - di->blendVertCount) * sizeof(zVec3));
        }
    } else {
        memcpy(g_zModel_TransformedVerts, di->verts, static_cast<size_t>(di->vertCount) * sizeof(zVec3));
    }

    zMath::MatTransformPointBatchInPlace(g_zModel_TransformedVerts, di->vertCount);
}

void PrepareTransformedNormals(zDiPartial *di) {
    if (g_zModel_VertexShadingEnabled == 0 || di->normals == 0 || di->normalCount <= 0) {
        return;
    }

    memcpy(g_zModel_TransformedNormals, di->normals,
           static_cast<size_t>(di->normalCount) * sizeof(zVec3));
    zMath::Vec3ArrayTransformDirection(g_zModel_TransformedNormals, di->normalCount);
    for (int i = 0; i < di->normalCount; ++i) {
        zMath::Vec3Normalize(&g_zModel_TransformedNormals[i]);
    }
}

int CopyEntryVerticesToScratch(zDiPartial *di, zDiEntryPartial *entry, int vertexCount) {
    int *indices = static_cast<int *>(entry->vertexIndices);
    if (indices == 0) {
        return 0;
    }

    for (int i = 0; i < vertexCount; ++i) {
        const int vertexIndex = indices[i];
        if (vertexIndex < 0 || vertexIndex >= di->vertCount) {
            return 0;
        }
        const zVec3 &src = g_zModel_TransformedVerts[vertexIndex];
        g_Clip_PolyVertsScratch[i].x = src.x;
        g_Clip_PolyVertsScratch[i].y = src.y;
        g_Clip_PolyVertsScratch[i].z = src.z;
    }
    return 1;
}

void CopyEntryNormalsToCurrent(zDiPartial *di, zDiEntryPartial *entry, int vertexCount) {
    g_zModel_CurrentPolyNormals = 0;
    if (g_zModel_VertexShadingEnabled == 0 || di->normalCount <= 0 ||
        (entry->flagsAndIndexCount & 0x0200) == 0 || entry->normalIndices == 0) {
        return;
    }

    int *indices = static_cast<int *>(entry->normalIndices);
    for (int i = 0; i < vertexCount; ++i) {
        const int normalIndex = indices[i];
        if (normalIndex < 0 || normalIndex >= di->normalCount) {
            g_zModel_CurrentPolyNormals = 0;
            return;
        }
        g_zModel_CurrentPolyNormalsStorage[i] = g_zModel_TransformedNormals[normalIndex];
    }
    g_zModel_CurrentPolyNormals = g_zModel_CurrentPolyNormalsStorage;
}

void ClearPolyAttributes(int vertexCount) {
    for (int i = 0; i < vertexCount; ++i) {
        g_Clip_PolyAttr0[i] = 0.0f;
        g_Clip_PolyAttr1[i] = 0.0f;
        g_Clip_PolyAttr2[i] = 0.0f;
    }
}

void FillPolyAttributes(float value, int vertexCount) {
    for (int i = 0; i < vertexCount; ++i) {
        g_Clip_PolyAttr0[i] = value;
        g_Clip_PolyAttr1[i] = value;
        g_Clip_PolyAttr2[i] = value;
    }
}

int BuildPolyAttributes(const zVec3 *surfaceNormal, int vertexCount) {
    int attrFlags = 0;
    int lightingMode = 0;

    if (gModel_FogEnabled != 0) {
        attrFlags |= zModel_Light::BuildAttr1Falloff(vertexCount, &lightingMode) != 0 ? 1 : 0;
    }

    if (gModel_HasActiveLights != 0) {
        int lightFlags = 0;
        attrFlags |= zModel_Light::SetActiveLights((zVec3 *)(surfaceNormal), vertexCount,
                                                   &lightFlags, &lightingMode, 0) != 0
                         ? 1
                         : 0;
    }

    if (attrFlags == 0) {
        FillPolyAttributes(1.0f, vertexCount);
    }
    return attrFlags;
}

int ComputeSurfaceNormalAndCull(int vertexCount, zVec3 *outNormal) {
    if (vertexCount < 3) {
        return 0;
    }

    const zClipVert &v0 = g_Clip_PolyVertsScratch[0];
    const zClipVert &v1 = g_Clip_PolyVertsScratch[1];
    const zClipVert &v2 = g_Clip_PolyVertsScratch[2];

    zVec3 edge0 = {v0.x - v1.x, v0.y - v1.y, v0.z - v1.z};
    zVec3 edge2 = {v2.x - v1.x, v2.y - v1.y, v2.z - v1.z};
    outNormal->x = edge0.z * edge2.y - edge0.y * edge2.z;
    outNormal->y = edge0.x * edge2.z - edge0.z * edge2.x;
    outNormal->z = edge0.y * edge2.x - edge0.x * edge2.y;

    const float facing = outNormal->x * v0.x + outNormal->y * v0.y + outNormal->z * v0.z;
    if (facing <= -g_zModel_BFETolerance) {
        return 0;
    }
    return 1;
}

void CopyEntryUvsToScratch(zDiEntryPartial *entry, int vertexCount) {
    if (g_Clip_PolyUvs == 0 || entry->uvPairs == 0) {
        return;
    }
    memcpy(g_Clip_PolyUvs, entry->uvPairs, static_cast<size_t>(vertexCount) * sizeof(zClipUV));
}

void ProjectScratchToClipVerts(int vertexCount) {
    if (g_zVideo_ActiveRendererPath == 0) {
        zMath::ProjectPointBatch((const zVec3 *)g_Clip_PolyVertsScratch,
                                 (zProjectedPoint *)g_Clip_PolyVerts, vertexCount);
    } else {
        zMath_ProjectSphereBatch((const zVec3 *)g_Clip_PolyVertsScratch,
                                 (zProjectedSphere *)g_Clip_PolyVerts, vertexCount);
    }
}

void ApplyDepthBiasToProjectedVerts(unsigned int drawFlags, int vertexCount) {
    const float depthScale =
        static_cast<float>(static_cast<short>(drawFlags & 0xffff)) * g_zRndr_InverseZTolerance +
        1.0f;
    for (int i = 0; i < vertexCount; ++i) {
        g_Clip_PolyVerts[i].z *= depthScale;
    }
}

int ClipAndProjectNoUv(zClipRectPartial *clipRect, int *vertexCount, int hasAttributes) {
    if (hasAttributes != 0) {
        if (zClipRect::ClipPolyZRange_NoUV_WithAttribs(clipRect, vertexCount) == 0) {
            return 0;
        }
    } else if (zClipRect::ClipPolyZRange_NoUV(clipRect, vertexCount) == 0) {
        return 0;
    }

    ProjectScratchToClipVerts(*vertexCount);

    if (hasAttributes != 0) {
        return zClipRect::ClipPoly_NoUV_WithAttr012_Alt(clipRect, vertexCount);
    }
    return zClipRect::ClipPoly_NoUV(clipRect, vertexCount);
}

int ClipAndProjectUv(zClipRectPartial *clipRect, int *vertexCount, int hasAttributes) {
    if (hasAttributes != 0) {
        if (zClipRect::ClipPolyZRange_WithAttr012(clipRect, vertexCount) == 0) {
            return 0;
        }
    } else if (zClipRect::ClipPolyNearZ(clipRect, vertexCount) == 0) {
        return 0;
    }

    for (int i = 0; i < *vertexCount; ++i) {
        g_Clip_PolyUvs[i].u *= g_Clip_PolyVertsScratch[i].z;
        g_Clip_PolyUvs[i].v *= g_Clip_PolyVertsScratch[i].z;
    }

    ProjectScratchToClipVerts(*vertexCount);
    for (int i_79 = 0; i_79 < *vertexCount; ++i_79) {
        if (g_Clip_PolyVerts[i_79].z != 0.0f) {
            g_Clip_PolyUvs[i_79].u /= g_Clip_PolyVerts[i_79].z;
            g_Clip_PolyUvs[i_79].v /= g_Clip_PolyVerts[i_79].z;
        }
    }

    if (hasAttributes != 0) {
        return zClipRect::ClipPoly_WithAttr012(clipRect, vertexCount);
    }
    return zClipRect::ClipPoly(clipRect, vertexCount);
}

int MaterialAlphaInt(const zModel_MaterialPartial *material) {
    const int alpha = static_cast<int>(material->flags & 0xff);
    return static_cast<int>(static_cast<float>(alpha) * gModel_RenderAlphaScaleCurrent);
}

float MaterialAlphaFloat(const zModel_MaterialPartial *material) {
    return static_cast<float>(MaterialAlphaInt(material)) * (1.0f / 255.0f);
}

zVideo_RenderClass *MaterialRenderClass(zModel_MaterialPartial *material) {
    if (material == 0 || material->currentTextureDirectoryEntry == 0) {
        return 0;
    }
    return (zVideo_RenderClass *)(material->currentTextureDirectoryEntry->texture);
}

int AppendDiVertex(zDiPartial *self, const zVec3 *point) {
    const int index = self->vertCount;
    self->verts = static_cast<zVec3 *>(
        realloc(self->verts, static_cast<size_t>(index + 1) * sizeof(zVec3)));
    self->verts[index] = *point;
    self->vertCount = index + 1;
    return index;
}

int AppendDiNormal(zDiPartial *self, const zVec3 *normal) {
    const int index = self->normalCount;
    self->normals = static_cast<zVec3 *>(
        realloc(self->normals, static_cast<size_t>(index + 1) * sizeof(zVec3)));
    self->normals[index] = *normal;
    self->normalCount = index + 1;
    return index;
}

void NormalizeUvTileOrigin(zClipUV *uvPairs, int uvCount) {
    if (uvPairs == 0 || uvCount <= 0) {
        return;
    }

    float minU = uvPairs[0].u;
    float minV = uvPairs[0].v;
    for (int i = 1; i < uvCount; ++i) {
        if (uvPairs[i].u < minU) {
            minU = uvPairs[i].u;
        }
        if (uvPairs[i].v < minV) {
            minV = uvPairs[i].v;
        }
    }

    const float baseU = static_cast<float>(floor(minU));
    const float baseV = static_cast<float>(floor(minV));
    for (int i_107 = 0; i_107 < uvCount; ++i_107) {
        uvPairs[i_107].u -= baseU;
        uvPairs[i_107].v -= baseV;
    }
}
} // namespace

namespace zDi {
// Reimplements 0x483650: zDi::AddPolygonEx
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
AddPolygonEx(zDiPartial *self, int vertexCount, zVec3 *points, zVec3 *entryNormals,
             zClipUV *uvPairsA, zVec3 *normalsA, zVec3 *normalsB, zClipUV *uvPairsB,
             zModel_MaterialPartial *material, unsigned int drawFlags, int flagBit8,
             const int *userTag) {
    if (vertexCount < 3) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 0xae4,
                          "ERROR: You're trying to add a Polygon with only (%d) verts",
                          vertexCount);
        return 1;
    }

    if (vertexCount >= 58) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 0xaed,
                          "Poly vertex count approaching limit (%d / %d)", vertexCount, 0x40);
        return 1;
    }

    const int originalVertexCount = vertexCount;
    if (zModel_Const::RemoveColinearVerticesInPlace(&vertexCount, points, uvPairsA, normalsB,
                                                    uvPairsB) != 0 &&
        vertexCount < 3) {
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 0xb0d,
                          "Discarding Polygon: (%d of %d) verts after 'check_colinearity()'",
                          vertexCount, originalVertexCount);
        return 1;
    }

    if (vertexCount > 3 && zModel_Const::IsPolygonCoplanar(vertexCount, points) == 0) {
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 0xb19,
                          "Attempting to add non-planar polygon (%d verts), triangulating...",
                          vertexCount);
        zModel_Const::SplitPolygonChunkedByVertexLimit(
            self, originalVertexCount, points, entryNormals, uvPairsA, normalsA, normalsB,
            uvPairsB, material, drawFlags, flagBit8, userTag);
        return 2;
    }

    if (vertexCount > g_zModel_MaxPolygonVertexCountBeforeSplit) {
        AddPolygonSplitByVertexLimit(self, originalVertexCount, points, entryNormals, uvPairsA,
                                     normalsA, normalsB, uvPairsB, material, drawFlags, flagBit8,
                                     userTag, g_zModel_MaxPolygonVertexCountBeforeSplit);
        return 2;
    }

    zDiEntryPartial *entries = static_cast<zDiEntryPartial *>(realloc(
        self->entries, static_cast<size_t>(self->entryCount + 1) * sizeof(zDiEntryPartial)));
    self->entries = entries;

    zDiEntryPartial *const entry = &entries[self->entryCount];
    memset(entry, 0, sizeof(zDiEntryPartial));
    entry->flagsAndIndexCount = static_cast<unsigned int>(vertexCount & 0xff) |
                                (static_cast<unsigned int>(flagBit8 & 1) << 8);
    if (entryNormals != 0) {
        entry->flagsAndIndexCount |= 0x200;
    }
    entry->drawFlags = drawFlags;
    entry->vertexIndices =
        malloc(static_cast<size_t>(vertexCount) * sizeof(int));
    if (entryNormals != 0) {
        entry->normalIndices =
            malloc(static_cast<size_t>(vertexCount) * sizeof(int));
    }

    int *vertexIndices = static_cast<int *>(entry->vertexIndices);
    int *normalIndices = static_cast<int *>(entry->normalIndices);
    zVec3 *pointCursor = points;
    zVec3 *normalBCursor = normalsB;
    zVec3 *entryNormalCursor = entryNormals;
    for (int i = 0; i < vertexCount; ++i) {
        if (normalsA != 0) {
            vertexIndices[i] = zModel_Const::AddOrMergeVertexAndNormal(self, pointCursor,
                                                                       normalBCursor);
            ++normalBCursor;
        } else {
            vertexIndices[i] = zModel_Const::AddOrMergeVertex(self, pointCursor);
        }
        if (vertexIndices[i] < 0) {
            return 1;
        }

        if (entryNormals != 0) {
            normalIndices[i] = zModel_Const::FindOrAppendNormalIndex(self, entryNormalCursor);
            ++entryNormalCursor;
        }
        ++pointCursor;
    }

    if ((material->flags & 0x0100) != 0) {
        entry->uvPairs = malloc(static_cast<size_t>(vertexCount) * sizeof(zClipUV));
        memcpy(entry->uvPairs, uvPairsA, static_cast<size_t>(vertexCount) * sizeof(zClipUV));
        NormalizeUvTileOrigin(static_cast<zClipUV *>(entry->uvPairs), vertexCount);
    }

    entry->material = material;
    RebuildGeneratedUvPairsForEntry(self, self->entryCount);
    if ((material->flags & 0x0100) != 0) {
        zModel_Const::QuantizeAndNormalizeUvPairs(vertexCount,
                                                  static_cast<zClipUV *>(entry->uvPairs));
    }
    memcpy(&entry->variantTagInitialized, userTag, sizeof(*userTag));

    ++self->entryCount;
    return 0;
}

// Reimplements 0x483610: zDi::AddPolygon
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
AddPolygon(zDiPartial *self, int pointCount, zVec3 *points, zClipUV *uvPairsA,
           zVec3 *normalsA, zVec3 *normalsB, zClipUV *uvPairsB, zModel_MaterialPartial *material,
           unsigned int drawFlags, int flagBit8, const int *userTag) {
    return AddPolygonEx(self, pointCount, points, 0, uvPairsA, normalsA, normalsB, uvPairsB,
                        material, drawFlags, flagBit8, userTag);
}

// Reimplements 0x483240: zDi::AddPolygonSplitByVertexLimit
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AddPolygonSplitByVertexLimit(zDiPartial *self, int totalVertexCount, zVec3 *points,
                             zVec3 *entryNormals, zClipUV *uvPairsA, zVec3 *normalsA,
                             zVec3 *normalsBInput, zClipUV *uvPairsBInput,
                             zModel_MaterialPartial *material, unsigned int drawFlags,
                             int flagBit8, const int *userTag, int maxChunkVertexCount) {
    zVec3 chunkPoints[4];
    zVec3 chunkEntryNormals[4];
    zClipUV chunkUvPairsA[4];
    zVec3 chunkNormalsB[4];
    zClipUV chunkUvPairsB[4];

    int clampedChunkVertexCount = maxChunkVertexCount;
    if (clampedChunkVertexCount > 4) {
        clampedChunkVertexCount = 4;
    }

    chunkPoints[0] = points[0];
    if (entryNormals != 0) {
        chunkEntryNormals[0] = entryNormals[0];
    }

    const int hasSecondaryUvSet = (material->flags & 0x0100) != 0;
    if (hasSecondaryUvSet) {
        chunkUvPairsA[0] = uvPairsA[0];
    }

    if (normalsA != 0) {
        chunkNormalsB[0] = normalsBInput[0];
        if (hasSecondaryUvSet) {
            chunkUvPairsB[0] = uvPairsBInput[0];
        }
    }

    int chunkStartVertexIndex = 1;
    if (totalVertexCount - 1 <= 1) {
        return;
    }

    do {
        int vertexCount = clampedChunkVertexCount;
        if (chunkStartVertexIndex + vertexCount > totalVertexCount + 1) {
            vertexCount = totalVertexCount - chunkStartVertexIndex + 1;
        }

        if (vertexCount > 1) {
            for (int chunkVertexIndex = 1; chunkVertexIndex < vertexCount; ++chunkVertexIndex) {
                const int sourceIndex = chunkStartVertexIndex + chunkVertexIndex - 1;
                chunkPoints[chunkVertexIndex] = points[sourceIndex];
                if (entryNormals != 0) {
                    chunkEntryNormals[chunkVertexIndex] = entryNormals[sourceIndex];
                }
                if (hasSecondaryUvSet) {
                    chunkUvPairsA[chunkVertexIndex] = uvPairsA[sourceIndex];
                }
                if (normalsA != 0) {
                    chunkNormalsB[chunkVertexIndex] = normalsBInput[sourceIndex];
                    if (hasSecondaryUvSet) {
                        chunkUvPairsB[chunkVertexIndex] = uvPairsBInput[sourceIndex];
                    }
                }
            }
        }

        if (vertexCount < 3) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 0xa16,
                              "Attempting to add polygon with only %d verts", vertexCount);
        }

        AddPolygonEx(self, vertexCount, chunkPoints, entryNormals != 0 ? chunkEntryNormals : 0,
                     chunkUvPairsA, normalsA, chunkNormalsB, chunkUvPairsB, material, drawFlags,
                     flagBit8, userTag);
        chunkStartVertexIndex += vertexCount - 2;
    } while (chunkStartVertexIndex < totalVertexCount - 1);
}

// Reimplements 0x4843b0: zDi::RebuildGeneratedUvPairsForEntry
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RebuildGeneratedUvPairsForEntry(zDiPartial *self,
                                                                     int entryIndex) {
    zDiEntryPartial *const entry = &self->entries[entryIndex];
    const int vertexCount = static_cast<int>(entry->flagsAndIndexCount & 0xff);
    if (entry->material == 0 || (entry->material->flags & 0x0100) == 0 || vertexCount <= 3) {
        return;
    }

    int *const vertexIndices = static_cast<int *>(entry->vertexIndices);
    zClipUV *const uvPairs = static_cast<zClipUV *>(entry->uvPairs);
    const zVec3 *const vertex0 = &self->verts[vertexIndices[0]];
    const zVec3 *const vertex1 = &self->verts[vertexIndices[1]];
    const zVec3 *const vertex2 = &self->verts[vertexIndices[2]];

    zVec3 triangleNormal;
    zMath_Vec3_TriangleNormal(vertex0, vertex1, vertex2, &triangleNormal);
    zMath::Vec3Normalize(&triangleNormal);

    const float absX = static_cast<float>(fabs(triangleNormal.x));
    const float absY = static_cast<float>(fabs(triangleNormal.y));
    const float absZ = static_cast<float>(fabs(triangleNormal.z));

    float vertex0A;
    float vertex0B;
    float vertex1A;
    float vertex1B;
    float vertex2A;
    float vertex2B;

    if (absX >= absY && absX >= absZ) {
        vertex0A = vertex0->y;
        vertex0B = vertex0->z;
        vertex1A = vertex1->y;
        vertex1B = vertex1->z;
        vertex2A = vertex2->y;
        vertex2B = vertex2->z;
    } else if (absY >= absX && absY >= absZ) {
        vertex0A = vertex0->z;
        vertex0B = vertex0->x;
        vertex1A = vertex1->z;
        vertex1B = vertex1->x;
        vertex2A = vertex2->z;
        vertex2B = vertex2->x;
    } else {
        vertex0A = vertex0->x;
        vertex0B = vertex0->y;
        vertex1A = vertex1->x;
        vertex1B = vertex1->y;
        vertex2A = vertex2->x;
        vertex2B = vertex2->y;
    }

    const zClipUV uGradient = zModel_Const::SolveTriScalarGradient2D(
        vertex0A, vertex0B, vertex1A, vertex1B, vertex2A, vertex2B,
        uvPairs[0].u, uvPairs[1].u, uvPairs[2].u);
    const zClipUV vGradient = zModel_Const::SolveTriScalarGradient2D(
        vertex0A, vertex0B, vertex1A, vertex1B, vertex2A, vertex2B,
        uvPairs[0].v, uvPairs[1].v, uvPairs[2].v);

    for (int vertexIndex = 3; vertexIndex < vertexCount; ++vertexIndex) {
        const zVec3 *const vertex = &self->verts[vertexIndices[vertexIndex]];
        float deltaA;
        float deltaB;
        if (absX >= absY && absX >= absZ) {
            deltaA = vertex->y - vertex0->y;
            deltaB = vertex->z - vertex0->z;
        } else if (absY >= absX && absY >= absZ) {
            deltaA = vertex->z - vertex0->z;
            deltaB = vertex->x - vertex0->x;
        } else {
            deltaA = vertex->x - vertex0->x;
            deltaB = vertex->y - vertex0->y;
        }

        uvPairs[vertexIndex].u = uvPairs[0].u + deltaA * uGradient.u + deltaB * uGradient.v;
        uvPairs[vertexIndex].v = uvPairs[0].v + deltaA * vGradient.u + deltaB * vGradient.v;
    }
}
} // namespace zDi

namespace zModel_Const {
// Reimplements 0x482c60: zModel_Const::SetNormalizedCrossFromVertexTriplet
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE zVec3 *RECOIL_FASTCALL
SetNormalizedCrossFromVertexTriplet(zVec3 *vertex0, zVec3 *vertex1,
                                    zVec3 *outNormal, zVec3 *vertex2) {
    const float edge0X = vertex0->x - vertex1->x;
    const float edge0Y = vertex0->y - vertex1->y;
    const float edge0Z = vertex0->z - vertex1->z;
    const float edge2X = vertex2->x - vertex1->x;
    const float edge2Y = vertex2->y - vertex1->y;
    const float edge2Z = vertex2->z - vertex1->z;

    const float normalX = edge0Z * edge2Y - edge0Y * edge2Z;
    const float normalY = edge0X * edge2Z - edge0Z * edge2X;
    const float normalZ = edge0Y * edge2X - edge0X * edge2Y;

    double length = 0.0;
    if (fabs(normalX) > g_zModel_ColinearTolerance ||
        fabs(normalY) > g_zModel_ColinearTolerance ||
        fabs(normalZ) > g_zModel_ColinearTolerance) {
        length = sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ);
    }

    double scale = 0.0;
    if (fabs(length) > g_zModel_ColinearTolerance) {
        scale = 1.0 / length;
    }

    outNormal->x = static_cast<float>(normalX * scale);
    outNormal->y = static_cast<float>(normalY * scale);
    outNormal->z = static_cast<float>(normalZ * scale);
    return outNormal;
}

// Reimplements 0x482b40: zModel_Const::RemoveColinearVerticesInPlace
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
RemoveColinearVerticesInPlace(int *vertexCount, zVec3 *points, zClipUV *,
                              zVec3 *, zClipUV *) {
    int removedAnyVertices = 0;
    int removedVertexThisPass;

    do {
        removedVertexThisPass = 0;

        if (*vertexCount >= 2) {
            int nextIndex = 2;
            int vertexIndex = 1;
            int scannedVertexCount = 2;
            zVec3 *currentVertex = &points[1];

            do {
                zVec3 outNormal;
                zVec3 *const normal = SetNormalizedCrossFromVertexTriplet(
                    currentVertex - 1, currentVertex, &outNormal, &points[nextIndex]);

                if (fabs(normal->x) < g_zModel_ColinearTolerance &&
                    fabs(normal->y) < g_zModel_ColinearTolerance &&
                    fabs(normal->z) < g_zModel_ColinearTolerance) {
                    removedAnyVertices = 1;
                    removedVertexThisPass = 1;

                    if (vertexIndex < *vertexCount - 1) {
                        zVec3 *write = &points[vertexIndex];
                        do {
                            *write = write[1];
                            ++write;
                            ++vertexIndex;
                        } while (vertexIndex < *vertexCount - 1);
                    }

                    --*vertexCount;
                    break;
                }

                ++vertexIndex;
                ++currentVertex;
                ++scannedVertexCount;
                nextIndex = (nextIndex + 1) % *vertexCount;
            } while (scannedVertexCount <= *vertexCount);
        }
    } while (removedVertexThisPass != 0);

    return removedAnyVertices;
}

// Reimplements 0x482e30: zModel_Const::ComputePolygonPlaneEquation
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE zGeometry_PlaneEquationPartial *RECOIL_FASTCALL
ComputePolygonPlaneEquation(int vertexCount, zVec3 *vertices,
                            zGeometry_PlaneEquationPartial *outPlane) {
    float normalX = 0.0f;
    float normalY = 0.0f;
    float normalZ = 0.0f;
    float sumX = 0.0f;
    float sumY = 0.0f;
    float sumZ = 0.0f;

    for (int i = 0; i < vertexCount; ++i) {
        zVec3 *const vertex = &vertices[i];
        zVec3 *const next = &vertices[(i + 1) % vertexCount];

        normalX += (vertex->y - next->y) * (vertex->z + next->z);
        normalY += (vertex->z - next->z) * (vertex->x + next->x);
        normalZ += (vertex->x - next->x) * (vertex->y + next->y);

        sumX += vertex->x;
        sumY += vertex->y;
        sumZ += vertex->z;
    }

    float normalLength = 0.0f;
    if (normalX != 0.0f || normalY != 0.0f || normalZ != 0.0f) {
        normalLength = static_cast<float>(
            sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ));
    }

    float inverseNormalLength = 0.0f;
    if (normalLength != 0.0f) {
        inverseNormalLength = 1.0f / normalLength;
    }

    outPlane->a = normalX * inverseNormalLength;
    outPlane->b = normalY * inverseNormalLength;
    outPlane->c = normalZ * inverseNormalLength;
    outPlane->d = -((sumX * normalX + sumY * normalY + sumZ * normalZ) /
                    (static_cast<float>(vertexCount) * normalLength));
    return outPlane;
}

// Reimplements 0x482db0: zModel_Const::IsPolygonCoplanar
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL IsPolygonCoplanar(int vertexCount, zVec3 *vertices) {
    zGeometry_PlaneEquationPartial plane;
    ComputePolygonPlaneEquation(vertexCount, vertices, &plane);

    if (vertexCount <= 0) {
        return 1;
    }

    for (int i = 0; i < vertexCount; ++i) {
        const zVec3 *const vertex = &vertices[i];
        const double distance =
            vertex->x * plane.a + vertex->y * plane.b + vertex->z * plane.c + plane.d;
        if (fabs(distance) > g_zModel_CoplanarTolerance) {
            return 0;
        }
    }

    return 1;
}

// Reimplements 0x482720: zModel_Const::AddOrMergeVertex
// (D:\Proj\GameZRecoil\zModel\gmod_const.c)
RECOIL_NOINLINE int RECOIL_FASTCALL AddOrMergeVertex(zDiPartial *self, zVec3 *point) {
    for (int vertexIndex = 0; vertexIndex < self->vertCount; ++vertexIndex) {
        const zVec3 *const existingPoint = &self->verts[vertexIndex];
        if (fabs(existingPoint->x - point->x) <= g_zModel_ConstVertexMergeEpsilon &&
            fabs(existingPoint->y - point->y) <= g_zModel_ConstVertexMergeEpsilon &&
            fabs(existingPoint->z - point->z) <= g_zModel_ConstVertexMergeEpsilon) {
            return vertexIndex;
        }
    }

    if (static_cast<double>(self->vertCount) > g_zModel_ConstVertexWarnThreshold) {
        sprintf(g_zError_DebugMsgBuffer,
                "%s: Line %d: WARNING: Model vertex count = %d\n",
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 1783, self->vertCount);
        sprintf(g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                "         Approaching max allowable: %d\n", 1024);
        zError::EmitDebugBuffer(1);
        return -1;
    }

    const int appendedVertexIndex = self->vertCount;
    self->verts = static_cast<zVec3 *>(realloc(
        self->verts, static_cast<size_t>(appendedVertexIndex + 1) * sizeof(zVec3)));
    self->verts[appendedVertexIndex] = *point;
    self->vertCount = appendedVertexIndex + 1;
    return appendedVertexIndex;
}

// Reimplements 0x482860: zModel_Const::AddOrMergeVertexAndNormal
// (D:\Proj\GameZRecoil\zModel\gmod_const.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
AddOrMergeVertexAndNormal(zDiPartial *self, zVec3 *point, zVec3 *normal) {
    zVec3 blendNormalDelta;
    blendNormalDelta.x = normal->x - point->x;
    blendNormalDelta.y = normal->y - point->y;
    blendNormalDelta.z = normal->z - point->z;

    for (int vertexIndex = 0; vertexIndex < self->vertCount; ++vertexIndex) {
        const zVec3 *const existingPoint = &self->verts[vertexIndex];
        const zVec3 *const existingBlend = &self->blendVerts[vertexIndex];
        if (existingPoint->x == point->x && existingPoint->y == point->y &&
            existingPoint->z == point->z && existingBlend->x == blendNormalDelta.x &&
            existingBlend->y == blendNormalDelta.y &&
            existingBlend->z == blendNormalDelta.z) {
            return vertexIndex;
        }
    }

    const int appendedVertexIndex = self->vertCount;
    self->verts = static_cast<zVec3 *>(realloc(
        self->verts, static_cast<size_t>(appendedVertexIndex + 1) * sizeof(zVec3)));
    self->verts[appendedVertexIndex] = *point;

    self->blendVerts = static_cast<zVec3 *>(realloc(
        self->blendVerts, static_cast<size_t>(appendedVertexIndex + 1) * sizeof(zVec3)));
    self->blendVerts[appendedVertexIndex] = blendNormalDelta;

    self->vertCount = appendedVertexIndex + 1;
    self->blendVertCount = self->vertCount;
    if (static_cast<double>(self->vertCount) > g_zModel_ConstVertexWarnThreshold) {
        sprintf(g_zError_DebugMsgBuffer,
                "%s: Line %d: WARNING: Model vertex count = %d\n",
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 1896, self->vertCount);
        sprintf(g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                "         Approaching max allowable: %d\n", 1024);
        zError::EmitDebugBuffer(1);
        return -1;
    }

    return appendedVertexIndex;
}

// Reimplements 0x482a10: zModel_Const::FindOrAppendNormalIndex
// (D:\Proj\GameZRecoil\zModel\gmod_const.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FindOrAppendNormalIndex(zDiPartial *self,
                                                            zVec3 *normal) {
    for (int normalIndex = 0; normalIndex < self->normalCount; ++normalIndex) {
        const zVec3 *const existingNormal = &self->normals[normalIndex];
        if (fabs(existingNormal->x - normal->x) < g_zModel_NormalMergeEpsilon &&
            fabs(existingNormal->y - normal->y) < g_zModel_NormalMergeEpsilon &&
            fabs(existingNormal->z - normal->z) < g_zModel_NormalMergeEpsilon) {
            return normalIndex;
        }
    }

    const int appendedNormalIndex = self->normalCount;
    self->normals = static_cast<zVec3 *>(realloc(
        self->normals, static_cast<size_t>(appendedNormalIndex + 1) * sizeof(zVec3)));
    self->normals[appendedNormalIndex] = *normal;
    self->normalCount = appendedNormalIndex + 1;
    if (static_cast<double>(self->normalCount) > g_zModel_ConstVertexWarnThreshold) {
        sprintf(g_zError_DebugMsgBuffer,
                "%s: Line %d: WARNING: Model normal count = %d\n",
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 1972, self->normalCount);
        sprintf(g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                "         Approaching max allowable: %d\n", 1024);
        zError::EmitDebugBuffer(1);
        return -1;
    }

    return appendedNormalIndex;
}

// Reimplements 0x484860: zModel_Const::SolveTriScalarGradient2D
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE zClipUV RECOIL_STDCALL
SolveTriScalarGradient2D(float vertex0A, float vertex0B, float vertex1A,
                         float vertex1B, float vertex2A, float vertex2B,
                         float value0, float value1, float value2) {
    const float edge20A = vertex2A - vertex1A;
    const float edge20B = vertex2B - vertex1B;
    const float edge10A = vertex0A - vertex1A;
    const float edge10B = vertex0B - vertex1B;
    const float value20 = value2 - value1;
    const float value10 = value0 - value1;
    const float determinant = edge20B * edge10A - edge20A * edge10B;

    zClipUV gradient = {0};
    if (determinant == 0.0f) {
        return gradient;
    }

    const float inverseDeterminant = 1.0f / determinant;
    gradient.u = -((value20 * edge10B - edge20B * value10) * inverseDeterminant);
    gradient.v = -((edge20A * value10 - value20 * edge10A) * inverseDeterminant);
    return gradient;
}

// Reimplements 0x483510: zModel_Const::QuantizeAndNormalizeUvPairs
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL QuantizeAndNormalizeUvPairs(int vertexCount,
                                                                 zClipUV *uvPairs) {
    if (vertexCount > 0) {
        for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            zClipUV *const uv = &uvPairs[vertexIndex];
            const int uFixed = static_cast<int>(
                (uv->u - g_zModel_UvQuantizeBias) * g_zModel_UvQuantizeScale);
            uv->u = static_cast<float>(uFixed) * g_zModel_UvQuantizeInvScale;

            const int vFixed = static_cast<int>(
                (uv->v - g_zModel_UvQuantizeBias) * g_zModel_UvQuantizeScale);
            uv->v = static_cast<float>(vFixed) * g_zModel_UvQuantizeInvScale;
        }
    }

    float minU = uvPairs[0].u;
    float minV = uvPairs[0].v;
    for (int vertexIndex = 1; vertexIndex < vertexCount; ++vertexIndex) {
        if (uvPairs[vertexIndex].u < minU) {
            minU = uvPairs[vertexIndex].u;
        }
        if (uvPairs[vertexIndex].v < minV) {
            minV = uvPairs[vertexIndex].v;
        }
    }

    const float baseU = static_cast<float>(floor(minU));
    const float baseV = static_cast<float>(floor(minV));
    for (int normalizeIndex = 0; normalizeIndex < vertexCount; ++normalizeIndex) {
        uvPairs[normalizeIndex].u -= baseU;
        uvPairs[normalizeIndex].v -= baseV;
    }
}

// Reimplements 0x482fe0: zModel_Const::SplitPolygonChunkedByVertexLimit
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SplitPolygonChunkedByVertexLimit(zDiPartial *self, int totalVertexCount, zVec3 *points,
                                 zVec3 *entryNormals, zClipUV *uvPairsA, zVec3 *normalsA,
                                 zVec3 *normalsBInput, zClipUV *uvPairsBInput,
                                 zModel_MaterialPartial *material, unsigned int drawFlags,
                                 int flagBit8, const int *userTag) {
    zVec3 trianglePoints[3];
    zVec3 triangleEntryNormals[3];
    zClipUV triangleUvPairsA[3];
    zVec3 triangleNormalsB[3];
    zClipUV triangleUvPairsB[3];

    trianglePoints[0] = points[0];
    if (entryNormals != 0) {
        triangleEntryNormals[0] = entryNormals[0];
    }

    const int hasSecondaryUvSet = (material->flags & 0x0100) != 0;
    if (hasSecondaryUvSet) {
        triangleUvPairsA[0] = uvPairsA[0];
    }

    if (normalsA != 0) {
        triangleNormalsB[0] = normalsBInput[0];
        if (hasSecondaryUvSet) {
            triangleUvPairsB[0] = uvPairsBInput[0];
        }
    }

    if (totalVertexCount <= 2) {
        return;
    }

    for (int vertexIndex = 2; vertexIndex < totalVertexCount; ++vertexIndex) {
        for (int triangleIndex = 1; triangleIndex < 3; ++triangleIndex) {
            const int sourceIndex = vertexIndex - 2 + triangleIndex;
            trianglePoints[triangleIndex] = points[sourceIndex];
            if (entryNormals != 0) {
                triangleEntryNormals[triangleIndex] = entryNormals[sourceIndex];
            }
            if (hasSecondaryUvSet) {
                triangleUvPairsA[triangleIndex] = uvPairsA[sourceIndex];
            }
            if (normalsA != 0) {
                triangleNormalsB[triangleIndex] = normalsBInput[sourceIndex];
                if (hasSecondaryUvSet) {
                    triangleUvPairsB[triangleIndex] = uvPairsBInput[sourceIndex];
                }
            }
        }

        zDi::AddPolygonEx(self, 3, trianglePoints,
                          entryNormals != 0 ? triangleEntryNormals : 0,
                          triangleUvPairsA, normalsA, triangleNormalsB, triangleUvPairsB,
                          material, drawFlags, flagBit8, userTag);
    }
}
} // namespace zModel_Const

// Reimplements 0x4791c0: zModel_Instance_UpdateScrollingTextures
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Instance_UpdateScrollingTextures(
    const zModel_TextureScrollInfoPartial *textureInfo, zModel_Uv *uvs, const float *scrollRates,
    int uvCount) {
    if (uvCount <= 0) {
        return;
    }

    const float rateU = scrollRates[0];
    const float rateV = scrollRates[1];
    if (rateU == 0.0f && rateV == 0.0f) {
        return;
    }

    const float deltaU = rateU * g_FrameDeltaTimeSec;
    const float deltaV = rateV * g_FrameDeltaTimeSec;

    float minU = uvs[0].u + deltaU;
    float maxU = minU;
    float minV = uvs[0].v + deltaV;
    float maxV = minV;
    uvs[0].u = minU;
    uvs[0].v = minV;

    for (int i = 1; i < uvCount; ++i) {
        if (rateU != 0.0f) {
            const float u = uvs[i].u + deltaU;
            uvs[i].u = u;
            if (u < minU) {
                minU = u;
            }
            if (u > maxU) {
                maxU = u;
            }
        }

        if (rateV != 0.0f) {
            const float v = uvs[i].v + deltaV;
            uvs[i].v = v;
            if (v < minV) {
                minV = v;
            }
            if (v > maxV) {
                maxV = v;
            }
        }
    }

    int correctionU = 0;
    if (rateU != 0.0f) {
        correctionU = ComputeWrapCorrection(minU, maxU, TextureWrapExtent(textureInfo, true));
    }

    int correctionV = 0;
    if (rateV != 0.0f) {
        correctionV = ComputeWrapCorrection(minV, maxV, TextureWrapExtent(textureInfo, false));
    }

    ApplyUvCorrection(uvs, uvCount, correctionU, correctionV);
}

// Reimplements 0x478fc0: zModel_Instance_UpdateScrollingTexturesIfNeeded
RECOIL_NOINLINE int RECOIL_FASTCALL
zModel_Instance_UpdateScrollingTexturesIfNeeded(zModel_InstancePartial *instance) {
    if (instance == 0) {
        return -1;
    }

    if (instance->scrollingTextureFrameTick == g_zVideo_FrameTick) {
        return 0;
    }

    instance->scrollingTextureFrameTick = g_zVideo_FrameTick;
    for (int i = 0; i < instance->surfaceEntryCount; ++i) {
        zModel_InstanceSurfaceEntryPartial *entry = &instance->surfaceEntries[i];
        zModel_MaterialTextureBindingPartial *material = entry->materialBinding;
        if ((material->flags & 1) == 0) {
            continue;
        }

        zModel_Instance_UpdateScrollingTextures(
            material->textureRef->textureInfo, entry->uvs, &instance->scrollRateU,
            static_cast<int>(entry->vertexCountAndFlags & 0xff));
    }

    return 0;
}

namespace zModel {
// Reimplements 0x475e70: zModel::Init
// (D:\Proj\GameZRecoil\zModel\gmod_init.c)
RECOIL_NOINLINE int RECOIL_CDECL Init() {
    zModel_Matl::InitGlobals();

    if (g_zVideo_ActiveRendererPath != 0) {
        gModel_RenderFn = zModel::RenderNodeHardware;
        g_zModel_SoftwarePathActive = 0;
    } else {
        g_zModel_SoftwarePathActive = 1;
    }

    gModel_ClipMaskStackTop = gModel_ClipMaskStack;

    int capacity = g_zModel_DiPoolCapacity;
    if (capacity == 0) {
        capacity = 1750;
        g_zModel_DiPoolCapacity = capacity;
    }

    const size_t poolBytes = static_cast<size_t>(capacity) * sizeof(zDiPartial);
    g_zModel_DiPoolBase = static_cast<zDiPartial *>(malloc(poolBytes));
    if (g_zModel_DiPoolBase == 0) {
        return 1;
    }

    memset(g_zModel_DiPoolBase, 0, poolBytes);
    g_zModel_DiPoolFreeHeadIndex = 0;
    for (int i = 0; i < capacity - 1; ++i) {
        g_zModel_DiPoolBase[i].nextFreeIndex = i + 1;
    }
    if (capacity > 0) {
        g_zModel_DiPoolBase[capacity - 1].nextFreeIndex = -1;
    }
    g_zModel_DiPoolInUseCount = 0;
    return 0;
}

// Reimplements 0x476030: zModel::SetVertexShadingEnabled
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetVertexShadingEnabled(int enabled) {
    g_zModel_VertexShadingEnabled = enabled;
}

// Reimplements 0x475ff0: zModel::SetDisplayInstancePoolCapacity
// (D:\Proj\GameZRecoil\zModel\gmod_init.c)
RECOIL_NOINLINE void RECOIL_FASTCALL SetDisplayInstancePoolCapacity(int capacity) {
    if (g_zModel_DiPoolCapacity != 0) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_init.c", 0x1be,
                          "Error setting model3d array size; size already set to %d.",
                          g_zModel_DiPoolCapacity);
        return;
    }

    g_zModel_DiPoolCapacity = capacity;
}

// Reimplements 0x476020: zModel::SetSoftwarePathActive
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetSoftwarePathActive(int active) {
    if (g_zVideo_ActiveRendererPath == 0) {
        g_zModel_SoftwarePathActive = active;
    }
}

// Reimplements 0x476090: zModel::SetTextureWorldPerMeter
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_STDCALL SetTextureWorldPerMeter(float worldPerMeterU,
                                                            float worldPerMeterV) {
    g_zModel_TextureWorldPerMeterU = worldPerMeterU;
    g_zModel_TextureWorldPerMeterV = worldPerMeterV;
}

// Reimplements 0x4760b0: zModel::SetTextureWorldBase
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_STDCALL SetTextureWorldBase(float worldBaseU,
                                                        float worldBaseV) {
    g_zModel_TextureWorldBaseU = worldBaseU;
    g_zModel_TextureWorldBaseV = worldBaseV;
}

// Reimplements 0x4760d0: zModel::SetDiTextureWorldPerMeter
// (D:\Proj\GameZRecoil\zModel\gmod_init.c)
RECOIL_NOINLINE int RECOIL_FASTCALL SetDiTextureWorldPerMeter(zDiPartial *di,
                                                              int worldSpaceEnabled,
                                                              float textureWorldPerMeter,
                                                              int textureWorldAxis) {
    if (di == 0) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_init.c", 0x285,
                          "ERROR setting model texture scroll data; Null ptr.");
        return 1;
    }

    di->flags = (di->flags & ~0x20) | ((worldSpaceEnabled & 1) << 5);
    di->textureWorldPerMeter = textureWorldPerMeter;
    di->textureWorldAxis = textureWorldAxis;
    return 0;
}

// Reimplements 0x477b30: zModel::RenderNodeHardware
// (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL RenderNodeHardware(zClass_NodePartial *node, int clipMask) {
    zDiPartial *const di = NodeDisplayInstance(node);
    if (di == 0) {
        return;
    }

    zMat4x3 matrixScratch = {0};
    zMath::MatStackPushPtr((float *)(&matrixScratch));
    if (di->mode == 1) {
        if ((di->flags & 0x10) != 0) {
            zMath_Mat_LoadView();
        } else if (g_zVideo_pActiveViewContext != 0) {
            zMath_Mat_LoadProjection(g_zVideo_pActiveViewContext->frustumYaw);
        }
    } else {
        zMath_Mat_SetupCamera();
    }

    PrepareTransformedVertices(di);
    PrepareTransformedNormals(di);

    if ((di->flags & 8) != 0 && di->pointEntries != 0) {
        for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
            zModel_PointEntryPartial *const pointEntry = &di->pointEntries[pointIndex];
            if (pointEntry->pointCamList != 0 && pointEntry->pointCamCount > 0) {
                zModel_RenderPointQueueEntry(&pointEntry->pointCamList[0],
                                             pointEntry->packedColor16, pointEntry);
            }
        }
    }

    gClipRect_Primary.flags = clipMask;
    for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
        zDiEntryPartial *const entry = &di->entries[entryIndex];
        zModel_MaterialPartial *const material = entry->material;
        int vertexCount = static_cast<int>(entry->flagsAndIndexCount & 0xff);
        if (material == 0 || vertexCount < 3 || vertexCount > 0x40 ||
            CopyEntryVerticesToScratch(di, entry, vertexCount) == 0) {
            continue;
        }

        zVec3 surfaceNormal = {0};
        if (ComputeSurfaceNormalAndCull(vertexCount, &surfaceNormal) == 0) {
            continue;
        }
        if ((entry->flagsAndIndexCount & 0x0100) != 0) {
            const float facing = surfaceNormal.x * g_Clip_PolyVertsScratch[0].x +
                                 surfaceNormal.y * g_Clip_PolyVertsScratch[0].y +
                                 surfaceNormal.z * g_Clip_PolyVertsScratch[0].z;
            if (facing < g_zModel_BFETolerance) {
                surfaceNormal.x = -surfaceNormal.x;
                surfaceNormal.y = -surfaceNormal.y;
                surfaceNormal.z = -surfaceNormal.z;
            }
        }

        CopyEntryNormalsToCurrent(di, entry, vertexCount);
        ClearPolyAttributes(vertexCount);
        const int hasAttributes = BuildPolyAttributes(&surfaceNormal, vertexCount);

        if ((material->flags & 0x0100) == 0) {
            int clippedCount = vertexCount;
            if (ClipAndProjectNoUv(&gClipRect_Primary, &clippedCount, hasAttributes) == 0) {
                continue;
            }

            ApplyDepthBiasToProjectedVerts(entry->drawFlags, clippedCount);
            if (hasAttributes != 0) {
                SubmitPolyColorAttrProc submit = (SubmitPolyColorAttrProc)(
                    static_cast<unsigned int>(g_zVideo_pfnSubmitPolyColorAttr));
                submit((zVideo_XyzVertex *)g_Clip_PolyVerts, material->packedColor,
                       (zVideo_ColorRgbFloat *)(&material->colorRgb), g_Clip_PolyAttr1,
                       g_Clip_PolyAttr0, g_Clip_PolyAttr2, MaterialAlphaInt(material),
                       clippedCount, entry->drawFlags, gModel_RenderVertexAlphaEnabled);
            } else {
                SubmitPolyFlatColor16Proc submit = (SubmitPolyFlatColor16Proc)(
                    static_cast<unsigned int>(g_zVideo_pfnSubmitPolyFlatColor16));
                submit((zVideo_XyzVertex *)g_Clip_PolyVerts, material->packedColor,
                       MaterialAlphaInt(material), entry->drawFlags, clippedCount,
                       gModel_RenderVertexAlphaEnabled);
            }

            if (gAltClipPassEnabled != 0) {
                clippedCount = vertexCount;
                CopyEntryVerticesToScratch(di, entry, clippedCount);
                if (ClipAndProjectNoUv(&gClipRect_Alt, &clippedCount, hasAttributes) != 0) {
                    for (int i = 0; i < clippedCount; ++i) {
                        zClipAlt::RemapPointXYInPlace(&g_Clip_PolyVerts[i].x);
                    }
                }
            }
            continue;
        }

        if ((material->flags & 0x0400) != 0) {
            zModel_Material::UpdateCycleIfNeeded(material);
        }

        CopyEntryUvsToScratch(entry, vertexCount);
        int clippedCount = vertexCount;
        if (ClipAndProjectUv(&gClipRect_Primary, &clippedCount, hasAttributes) == 0) {
            continue;
        }

        ApplyDepthBiasToProjectedVerts(entry->drawFlags, clippedCount);
        zVideo_RenderClass *const renderClass = MaterialRenderClass(material);
        if (hasAttributes != 0 || g_zModel_CurrentPolyNormals != 0) {
            SubmitPolygonProc submit = (SubmitPolygonProc)(
                static_cast<unsigned int>(g_zVideo_pfnSubmitPolygon));
            submit((zVideo_XyzVertex *)g_Clip_PolyVerts, (zVideo_TexCoord *)g_Clip_PolyUvs,
                   g_Clip_PolyAttr1, g_Clip_PolyAttr0, g_Clip_PolyAttr2, clippedCount,
                   renderClass, entry->drawFlags, MaterialAlphaFloat(material),
                   gModel_RenderVertexAlphaEnabled);
        } else {
            SubmitPolyRenderClassProc submit = (SubmitPolyRenderClassProc)(
                static_cast<unsigned int>(g_zVideo_pfnSubmitPolyRenderClass));
            submit((zVideo_XyzVertex *)g_Clip_PolyVerts, (zVideo_TexCoord *)g_Clip_PolyUvs,
                   clippedCount, renderClass, entry->drawFlags, MaterialAlphaFloat(material),
                   gModel_RenderVertexAlphaEnabled);
        }

        if (gAltClipPassEnabled != 0) {
            clippedCount = vertexCount;
            CopyEntryVerticesToScratch(di, entry, clippedCount);
            CopyEntryUvsToScratch(entry, clippedCount);
            if (ClipAndProjectUv(&gClipRect_Alt, &clippedCount, hasAttributes) != 0) {
                for (int i_265 = 0; i_265 < clippedCount; ++i_265) {
                    zClipAlt::RemapPointXYInPlace(&g_Clip_PolyVerts[i_265].x);
                }
            }
        }
    }

    zMath::MatStackPopPtr();
}
} // namespace zModel

// Reimplements 0x479020: zModel_RenderPointQueueEntry
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_RenderPointQueueEntry(
    const zVec3 *pointPos, int packedColor16, zModel_PointEntryPartial *pointEntry) {
    zVec3 transformedPoint = *pointPos;
    if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
        transformedPoint = TransformPointByCurrentMatrix(pointPos);
    }

    if (transformedPoint.z <= gClipRect_Primary.zMin) {
        return;
    }

    zProjectedPoint projectedPoint = {0};
    if (g_zVideo_ActiveRendererPath == 0) {
        zMath::ProjectPointBatch(&transformedPoint, &projectedPoint, 1);
    } else {
        zMath_ProjectSphereBatch(&transformedPoint,
                                 (zProjectedSphere *)(&projectedPoint), 1);
    }

    if (!ProjectedPointInClipBounds(projectedPoint)) {
        return;
    }

    const int color16 = packedColor16 & 0xffff;
    const int source =
        static_cast<int>((int)(&pointEntry->lensFlareSource[0]));
    if (g_zVideo_ActiveRendererPath == 0) {
        zRndr_LensFlare_QueueProjectedSample(&projectedPoint, color16, source);
        return;
    }

    const int depthBias = static_cast<short>(pointEntry->depthBiasWord & 0xffff);
    projectedPoint.reciprocalZ =
        ((static_cast<float>(depthBias) * g_zRndr_InverseZTolerance) + 1.0f) *
        projectedPoint.reciprocalZ;

    const DrawPointColor16Proc drawPoint = (DrawPointColor16Proc)(
        static_cast<unsigned int>(g_zVideo_pfnDrawPointColor16));
    drawPoint(&projectedPoint, static_cast<unsigned int>(color16), 1);
    zRndr_LensFlare_QueueProjectedSample(&projectedPoint, color16, source);
}

namespace zDi {
// Reimplements 0x484140: zDi::SetEntryValueForAllEntries
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetEntryValueForAllEntries(zDiPartial *self,
                                                                unsigned int entryValue) {
    if (self == 0) {
        return;
    }

    for (int i = 0; i < self->entryCount; ++i) {
        self->entries[i].drawFlags = entryValue;
    }
}

// Reimplements 0x484170: zDi::SetShowBackFaceForAllEntries
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetShowBackFaceForAllEntries(zDiPartial *self,
                                                                  int enabled) {
    const unsigned int showBackFaceBit = (enabled & 1) << 8;
    for (int i = 0; i < self->entryCount; ++i) {
        self->entries[i].flagsAndIndexCount =
            (self->entries[i].flagsAndIndexCount & ~0x0100u) | showBackFaceBit;
    }
}

// Reimplements 0x484230: zDi::ResetCurrentVariant
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ResetCurrentVariant(zDiPartial *self) {
    zModel_MaterialPartial *const material = self->entries->material;
    zModel_MaterialCyclePartial *const cycle = material->cycle;
    if (cycle != 0) {
        cycle->currentFrame = 0.0f;
        material->currentTextureDirectoryEntry = cycle->frameTable[0];
    }
}

// Reimplements 0x484250: zDi::SetCurrentVariantCycleTextureCount
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SetCurrentVariantCycleTextureCount(zDiPartial *self,
                                                                       int textureCount) {
    if (self == 0) {
        sprintf(g_zError_DebugMsgBuffer,
                "%s(%d): ERROR setting model cycle texture. Model 3D pointer is NULL.\n",
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c", 0xf3f);
        fprintf(stderr, g_zError_DebugMsgBuffer);
        return -1;
    }

    zModel_MaterialPartial *const material = self->entries->material;
    if (material != 0) {
        zModel_Material::SetCycleTextureCount(material, textureCount);
        return 0;
    }

    // Original code reaches this only for a null material pointer and then
    // dereferences it while clearing the cycle-texture flag.
    material->flags = static_cast<unsigned short>(material->flags & 0xfbff);
    return 0;
}

// Reimplements 0x4842b0: zDi::SetCurrentVariant
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetCurrentVariant(zDiPartial *self,
                                                       int variantIndex) {
    zModel_MaterialPartial *const material = self->entries->material;
    zModel_MaterialCyclePartial *const cycle = material->cycle;
    if (cycle == 0) {
        return;
    }

    const int frameCount = cycle->frameCount;
    if (variantIndex >= frameCount) {
        variantIndex %= frameCount;
    } else if (variantIndex < 0) {
        variantIndex = 0;
    }

    material->currentTextureDirectoryEntry = cycle->frameTable[variantIndex];
    cycle->currentFrame = static_cast<float>(variantIndex);
}

// Reimplements 0x484310: zDi::SetCurrentVariantCycleTextureSpeed
// (D:\Proj\GameZRecoil\zDi\zdi.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SetCurrentVariantCycleTextureSpeed(zDiPartial *self,
                                                                       float cycleSpeed) {
    if (self == 0) {
        return 0;
    }

    return zModel_Material::SetCycleTextureSpeed(self->entries->material, cycleSpeed);
}

// Reimplements 0x483f80: zDi::BuildBlendVertsFromConnectivity
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
BuildBlendVertsFromConnectivity(zDiPartial *self, int *excludedVertexIndices, float blendY,
                                int excludedVertexCount, int minSharedVertexCount) {
    const int vertCount = self->vertCount;
    self->blendVerts = static_cast<zVec3 *>(
        realloc(self->blendVerts, static_cast<size_t>(vertCount) * sizeof(zVec3)));

    int *const blendDisabledMask =
        static_cast<int *>(malloc(static_cast<size_t>(vertCount) * sizeof(int)));
    int *const vertexReferenceCounts =
        static_cast<int *>(malloc(static_cast<size_t>(vertCount) * sizeof(int)));

    for (int vertexIndex = 0; vertexIndex < vertCount; ++vertexIndex) {
        blendDisabledMask[vertexIndex] = 0;
        vertexReferenceCounts[vertexIndex] = 0;
    }

    for (int excludeIndex = 0; excludeIndex < excludedVertexCount; ++excludeIndex) {
        blendDisabledMask[excludedVertexIndices[excludeIndex]] = 1;
    }

    for (int entryIndex = 0; entryIndex < self->entryCount; ++entryIndex) {
        zDiEntryPartial *const entry = &self->entries[entryIndex];
        const unsigned int entryVertexCount = entry->flagsAndIndexCount & 0xff;
        int *const vertexIndices = static_cast<int *>(entry->vertexIndices);
        for (unsigned int entryVertexIndex = 0; entryVertexIndex < entryVertexCount;
             ++entryVertexIndex) {
            ++vertexReferenceCounts[vertexIndices[entryVertexIndex]];
        }
    }

    if (minSharedVertexCount > 0) {
        for (int vertexIndex = 0; vertexIndex < vertCount; ++vertexIndex) {
            if (vertexReferenceCounts[vertexIndex] < minSharedVertexCount) {
                blendDisabledMask[vertexIndex] = 1;
            }
        }
    }

    for (int blendVertexIndex = 0; blendVertexIndex < vertCount; ++blendVertexIndex) {
        int enableBlendY = 1;
        for (int excludeIndex = 0; enableBlendY != 0 && excludeIndex < excludedVertexCount;
             ++excludeIndex) {
            if (excludedVertexIndices[excludeIndex] == blendVertexIndex) {
                enableBlendY = 0;
            }
        }
        if (blendDisabledMask[blendVertexIndex] == 1) {
            enableBlendY = 0;
        }

        self->blendVerts[blendVertexIndex].x = 0.0f;
        self->blendVerts[blendVertexIndex].y = enableBlendY != 0 ? blendY : 0.0f;
        self->blendVerts[blendVertexIndex].z = 0.0f;
    }

    if (blendDisabledMask != 0) {
        free(blendDisabledMask);
    }
    if (vertexReferenceCounts != 0) {
        free(vertexReferenceCounts);
    }

    self->flags |= 0x08;
    self->blendScale = 1.0f;
    self->blendVertCount = self->vertCount;
}

// Reimplements 0x484350: zDi::SetObject3DColorModeForMaterials
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetObject3DColorModeForMaterials(zDiPartial *self,
                                                                      int colorMode) {
    zDiEntryPartial *entry = self->entries;
    for (int i = 0; i < self->entryCount; ++i, ++entry) {
        zModel_MaterialPartial *material = entry->material;
        if ((material->flags & 0x0100) != 0) {
            continue;
        }

        material->colorRgb.red = static_cast<float>(colorMode);
        material->colorRgb.green = 0.0f;
        material->colorRgb.blue = 0.0f;
        material->packedColor = static_cast<unsigned short>(
            (material->packedColor & 0x00ff) |
            ((static_cast<unsigned int>(colorMode) & 0xff) << 8));
        material->colorScalar = 1.0f;
    }
}
} // namespace zDi

namespace zModel_Instance {
// Reimplements 0x4842f0: zModel_Instance::SetCycleTextureLoop
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureLoop(zDiPartial *instance, int loopEnabled) {
    if (instance == 0) {
        return 0;
    }

    return zModel_Material::SetCycleTextureLoop(instance->entries->material, loopEnabled);
}

// Reimplements 0x484330: zModel_Instance::AddCycleTexture
// (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
AddCycleTexture(zDiPartial *instance, zImage_TexDirEntryPartial *textureDirectoryEntry) {
    if (instance == 0) {
        return 0;
    }

    return zModel_Material::AddCycleTexture(instance->entries->material, textureDirectoryEntry);
}
} // namespace zModel_Instance

namespace zDi {

// Reimplements 0x476a50: zDi::EvalBoundingSphereLightingFlags
RECOIL_NOINLINE void RECOIL_FASTCALL EvalBoundingSphereLightingFlags(
    zDiPartial *self, int *outDepthFade, int *outActiveLightState,
    int *outLensFlareVisible) {
    zVec3 mappedPoint = self->bboxCenter;
    if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
        mappedPoint = TransformPointByCurrentMatrix(&self->bboxCenter);
    }

    if (gModel_FogEnabled != 0 && (self->flags & 2) != 0 &&
        zModel_Light::EvalSphereFogFade(&mappedPoint, self->bboxRadius) >
            kVisibleContributionThreshold) {
        *outDepthFade = 1;
    } else {
        *outDepthFade = 0;
    }

    int activeLightContributionCount = 0;
    if (ModelGraphicsFlagBit0Enabled()) {
        if (gModel_HasActiveLights != 0 && (self->flags & 1) != 0) {
            activeLightContributionCount =
                zModel_Light::PointInPolygonTestRadiusXZ(&mappedPoint, self->bboxRadius);
            *outActiveLightState = activeLightContributionCount > 0 ? 1 : 0;
        } else {
            *outActiveLightState = 0;
        }

        if (g_zModel_FogTargetColorOverride.weight > kVisibleContributionThreshold) {
            ++activeLightContributionCount;
            *outActiveLightState = 1;
        }
    } else {
        *outActiveLightState = 0;
    }

    if (activeLightContributionCount <= 1) {
        *outLensFlareVisible = 0;
        return;
    }

    zColorRgb fogColorRgb01 = {0};
    float totalWeight = 0.0f;
    float maxWeight = 0.0f;

    for (int i = 0; i < gModel_ActiveLightCount; ++i) {
        zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[i];
        if (entry.contributesToLighting == 0) {
            continue;
        }

        zClass_LightDataPartial *light = entry.light;
        if (g_zModel_SoftwarePathActive != 0 && light->isPointMode != 0) {
            continue;
        }

        const float weight = g_Clip_PolyAttr0[i];
        fogColorRgb01.red += light->specularColor.red * weight;
        fogColorRgb01.green += light->specularColor.green * weight;
        fogColorRgb01.blue += light->specularColor.blue * weight;
        totalWeight += weight;
        if (maxWeight < weight) {
            maxWeight = weight;
        }
    }

    (void)maxWeight;

    if (g_zModel_FogTargetColorOverride.weight > kVisibleContributionThreshold) {
        fogColorRgb01.red += g_zModel_FogTargetColorOverride.colorRgb01.red;
        fogColorRgb01.green += g_zModel_FogTargetColorOverride.colorRgb01.green;
        fogColorRgb01.blue += g_zModel_FogTargetColorOverride.colorRgb01.blue;
        totalWeight += g_zModel_FogTargetColorOverride.weight;
    }

    const float invTotalWeight = 1.0f / totalWeight;
    fogColorRgb01.red *= invTotalWeight;
    fogColorRgb01.green *= invTotalWeight;
    fogColorRgb01.blue *= invTotalWeight;
    zRndr::SetFogTargetColorRgb01Clamped(&fogColorRgb01);
    *outLensFlareVisible = 1;
}
} // namespace zDi
