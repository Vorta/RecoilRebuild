#include "GameZRecoil/zModel/zModel.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

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
             zClipUV *uvPairsA, zVec3 *normalsA, zVec3 *normalsB, zClipUV *,
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
    entry->unknown_04 = drawFlags;
    entry->vertexIndices =
        malloc(static_cast<size_t>(vertexCount) * sizeof(int));
    if (entryNormals != 0) {
        entry->normalIndices =
            malloc(static_cast<size_t>(vertexCount) * sizeof(int));
    }

    int *vertexIndices = static_cast<int *>(entry->vertexIndices);
    int *normalIndices = static_cast<int *>(entry->normalIndices);
    for (int i = 0; i < vertexCount; ++i) {
        vertexIndices[i] = AppendDiVertex(self, &points[i]);
        if (normalsA != 0 && normalsB != 0) {
            const int vertexIndex = vertexIndices[i];
            self->blendVerts = static_cast<zVec3 *>(realloc(
                self->blendVerts, static_cast<size_t>(vertexIndex + 1) * sizeof(zVec3)));
            self->blendVerts[vertexIndex] = normalsB[i];
            if (self->blendVertCount <= vertexIndex) {
                self->blendVertCount = vertexIndex + 1;
            }
        }
        if (entryNormals != 0) {
            normalIndices[i] = AppendDiNormal(self, &entryNormals[i]);
        }
    }

    if (material != 0 && (material->flags & 1) != 0) {
        entry->uvPairs = malloc(static_cast<size_t>(vertexCount) * sizeof(zClipUV));
        if (uvPairsA != 0) {
            memcpy(entry->uvPairs, uvPairsA,
                        static_cast<size_t>(vertexCount) * sizeof(zClipUV));
        } else {
            memset(entry->uvPairs, 0, static_cast<size_t>(vertexCount) * sizeof(zClipUV));
        }
        NormalizeUvTileOrigin(static_cast<zClipUV *>(entry->uvPairs), vertexCount);
    }

    entry->material = material;
    if (userTag != 0) {
        memcpy(&entry->variantTagInitialized, userTag, sizeof(*userTag));
    }

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
} // namespace zDi

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
