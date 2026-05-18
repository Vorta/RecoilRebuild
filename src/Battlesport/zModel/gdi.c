#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zModel/zModel.h"
#include "zDi.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
    template <typename T> const T &min(const T &lhs, const T &rhs) {
        return lhs < rhs ? lhs : rhs;
    }

    template <typename T> const T &max(const T &lhs, const T &rhs) {
        return lhs < rhs ? rhs : lhs;
    }

    void IncludePoint(zBoundsMinMaxPartial * bounds, const zVec3 &point) {
        bounds->min.x = min(bounds->min.x, point.x);
        bounds->max.x = max(bounds->max.x, point.x);
        bounds->min.y = min(bounds->min.y, point.y);
        bounds->max.y = max(bounds->max.y, point.y);
        bounds->min.z = min(bounds->min.z, point.z);
        bounds->max.z = max(bounds->max.z, point.z);
    }

    void InitializeBounds(zBoundsMinMaxPartial * bounds, const zVec3 &point) {
        bounds->min = point;
        bounds->max = point;
    }

    float FastSqrtApprox(float value) {
        int bits = 0;
        memcpy(&bits, &value, sizeof(bits));
        bits = (bits >> 1) + 0x1fc00000;
        float out = 0.0f;
        memcpy(&out, &bits, sizeof(out));
        return out;
    }

    struct MaterialClonePair {
        zModel_MaterialPartial *source;
        zModel_MaterialPartial *clone;
    };

    void *CopyArrayBytes(const void *source, size_t byteCount) {
        void *const copy = malloc(byteCount);
        if (copy != 0 && source != 0 && byteCount != 0) {
            memcpy(copy, source, byteCount);
        }
        return copy;
    }

    void CopyEntryArrayIfPresent(void **dest, void *source, size_t byteCount) {
        if (byteCount != 0) {
            *dest = CopyArrayBytes(source, byteCount);
        }
    }
}

namespace zDi {
    // Reimplements 0x4826f0: zDi::AddRef (GameZRecoil/zDi/zdi.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL AddRef(zDiPartial * self) {
        ++self->refCount;
        return 0;
    }

    // Reimplements 0x482700: zDi::Release (GameZRecoil/zDi/zdi.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL Release(zDiPartial * self) {
        --self->refCount;
        return 0;
    }

    // Reimplements 0x482710: zDi::GetRefCount (Battlesport/zModel/gdi.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetRefCount(zDiPartial * self) {
        return self->refCount;
    }

    // Reimplements 0x482160: zDi::FreeContents (Battlesport/zModel/gdi.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL FreeContents(zDiPartial * self) {
        if (self == 0) {
            return 5;
        }

        for (int i = 0; i < self->entryCount; ++i) {
            zDiEntryPartial &entry = self->entries[i];
            if (entry.vertexIndices != 0) {
                free(entry.vertexIndices);
                entry.vertexIndices = 0;
            }
            if (entry.normalIndices != 0) {
                free(entry.normalIndices);
                entry.normalIndices = 0;
            }
            if (entry.uvPairs != 0) {
                free(entry.uvPairs);
                entry.uvPairs = 0;
            }
        }

        self->entryCount = 0;
        if (self->entries != 0) {
            free(self->entries);
            self->entries = 0;
        }
        if (self->verts != 0) {
            free(self->verts);
            self->verts = 0;
        }
        if (self->normals != 0) {
            free(self->normals);
            self->normals = 0;
        }
        if (self->blendVerts != 0) {
            free(self->blendVerts);
            self->blendVerts = 0;
        }

        if (self->pointEntries != 0) {
            for (int i = 0; i < self->pointCount; ++i) {
                if (self->pointEntries[i].pointCamList != 0) {
                    free(self->pointEntries[i].pointCamList);
                }
            }

            free(self->pointEntries);
            self->pointEntries = 0;
        }

        return 0;
    }

    // Reimplements 0x4826d0: zDi::SetFlagBit0
    RECOIL_NOINLINE void RECOIL_FASTCALL SetFlagBit0(zDiPartial * self, int enabled) {
        if (self != 0) {
            self->flags = ((enabled ^ self->flags) & 1) ^ self->flags;
        }
    }

    // Reimplements 0x4826b0: zDi::SetClonedFlag (Battlesport/zModel/gdi.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL SetClonedFlag(zDiPartial * self, int isCloned) {
        if (self != 0) {
            self->flags = (self->flags & ~0x02) | ((isCloned & 1) << 1);
        }
    }

    // Reimplements 0x482270: zDi::CloneToInstance (Battlesport/zModel/gdi.c)
    RECOIL_NOINLINE zDiPartial *RECOIL_FASTCALL CloneToInstance(
        zDiPartial * self, int cloneMaterials, int cloneAuxOnly) {
        if (self == 0) {
            return 0;
        }

        zDiPartial *const clone = zModel_DiPool::AllocFromFreeList();
        if (clone == 0) {
            return 0;
        }

        clone->mode = self->mode;
        clone->refCount = 0;
        SetFlagBit0(clone, self->flags & 1);
        SetClonedFlag(clone, (self->flags >> 1) & 1);
        clone->flags = (clone->flags & ~0x04) | (self->flags & 0x04);
        clone->flags = (clone->flags & ~0x08) | (self->flags & 0x08);
        clone->flags = (clone->flags & ~0x10) | (self->flags & 0x10);
        clone->blendScale = self->blendScale;
        clone->flags = (clone->flags & ~0x20) | (self->flags & 0x20);
        clone->textureWorldPerMeter = self->textureWorldPerMeter;
        clone->textureWorldAxis = self->textureWorldAxis;
        clone->field2c = self->field2c;

        clone->pointCount = self->pointCount;
        if (self->pointCount > 0) {
            clone->pointEntries = static_cast<zModel_PointEntryPartial *>(malloc(
                static_cast<size_t>(self->pointCount) * sizeof(zModel_PointEntryPartial)));
            for (int i = 0; i < self->pointCount; ++i) {
                clone->pointEntries[i] = self->pointEntries[i];
                if (self->pointEntries[i].pointCamCount > 0) {
                    clone->pointEntries[i].pointCamList = static_cast<zVec3 *>(CopyArrayBytes(
                        self->pointEntries[i].pointCamList,
                        static_cast<size_t>(self->pointEntries[i].pointCamCount) *
                            sizeof(zVec3)));
                }
            }
        }

        clone->blendVertCount = self->blendVertCount;
        if (self->blendVertCount > 0) {
            clone->blendVerts = static_cast<zVec3 *>(CopyArrayBytes(
                self->blendVerts, static_cast<size_t>(self->blendVertCount) * sizeof(zVec3)));
        }

        clone->vertCount = self->vertCount;
        if (self->vertCount > 0) {
            clone->verts = static_cast<zVec3 *>(CopyArrayBytes(
                self->verts, static_cast<size_t>(self->vertCount) * sizeof(zVec3)));
        }

        clone->normalCount = self->normalCount;
        if (self->normalCount > 0) {
            clone->normals = static_cast<zVec3 *>(CopyArrayBytes(
                self->normals, static_cast<size_t>(self->normalCount) * sizeof(zVec3)));
        }

        clone->entryCount = self->entryCount;
        if (self->entryCount > 0) {
            clone->entries = static_cast<zDiEntryPartial *>(
                calloc(static_cast<size_t>(self->entryCount), sizeof(zDiEntryPartial)));
        }

        MaterialClonePair *materialPairs = 0;
        int materialPairCount = 0;
        for (int i = 0; i < self->entryCount; ++i) {
            const zDiEntryPartial &sourceEntry = self->entries[i];
            zDiEntryPartial &destEntry = clone->entries[i];

            destEntry.unknown_04 = sourceEntry.unknown_04;
            destEntry.flagsAndIndexCount = sourceEntry.flagsAndIndexCount & 0x00000300;
            memcpy(&destEntry.variantTagInitialized, &sourceEntry.variantTagInitialized, 4);

            zModel_MaterialPartial *material = sourceEntry.material;
            if (cloneMaterials != 0) {
                if (cloneAuxOnly == 0 || zModel_Material::HasAuxData(sourceEntry.material) != 0) {
                    material = 0;
                    {
                    for (int pairIndex = 0; pairIndex < materialPairCount; ++pairIndex) {
                        if (materialPairs[pairIndex].source == sourceEntry.material) {
                            material = materialPairs[pairIndex].clone;
                            break;
                        }
                    }
                    }

                    if (material == 0) {
                        material = zModel_Material::Clone(sourceEntry.material);
                        materialPairs = static_cast<MaterialClonePair *>(realloc(
                            materialPairs, static_cast<size_t>(materialPairCount + 1) *
                                               sizeof(MaterialClonePair)));
                        materialPairs[materialPairCount].source = sourceEntry.material;
                        materialPairs[materialPairCount].clone = material;
                        ++materialPairCount;
                    }
                }
            }
            destEntry.material = material;

            const unsigned int indexCount = sourceEntry.flagsAndIndexCount & 0xff;
            CopyEntryArrayIfPresent(&destEntry.vertexIndices, sourceEntry.vertexIndices,
                                    static_cast<size_t>(indexCount) * sizeof(unsigned int));
            if ((sourceEntry.flagsAndIndexCount & 0x00000200) != 0 &&
                sourceEntry.normalIndices != 0) {
                CopyEntryArrayIfPresent(&destEntry.normalIndices, sourceEntry.normalIndices,
                                        static_cast<size_t>(indexCount) *
                                            sizeof(unsigned int));
            }

            destEntry.flagsAndIndexCount = (destEntry.flagsAndIndexCount & ~0xffu) | indexCount;
            if ((destEntry.material->flags & 0x0100) != 0) {
                CopyEntryArrayIfPresent(&destEntry.uvPairs, sourceEntry.uvPairs,
                                        static_cast<size_t>(indexCount) * 8u);
            }
        }

        if (materialPairs != 0) {
            free(materialPairs);
        }

        return clone;
    }

    // Reimplements 0x483a60: zDi::HasSpecialFlagsOrAuxMaterialData
    // (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL HasSpecialFlagsOrAuxMaterialData(zDiPartial *
                                                                                  self) {
        if (self == 0) {
            return 0;
        }

        if ((self->flags & 0x04) != 0 || (self->flags & 0x08) != 0 || (self->flags & 0x20) != 0) {
            return 1;
        }

        for (int i = 0; i < self->entryCount; ++i) {
            if (zModel_Material::HasAuxData(self->entries[i].material) != 0) {
                return 1;
            }
        }

        return 0;
    }

    // Reimplements 0x483b80: zDi::BuildAabb
    RECOIL_NOINLINE void RECOIL_FASTCALL BuildAabb(zDiPartial * self,
                                                   zBoundsMinMaxPartial * outBoundsMinMax) {
        bool initialized = false;

        if (self->vertCount > 0 && self->verts != 0) {
            InitializeBounds(outBoundsMinMax, self->verts[0]);
            initialized = true;
        } else if (self->pointCount > 0 && self->pointEntries != 0 &&
                   self->pointEntries[0].pointCamCount > 0 &&
                   self->pointEntries[0].pointCamList != 0) {
            InitializeBounds(outBoundsMinMax, self->pointEntries[0].pointCamList[0]);
            initialized = true;
        }

        if (!initialized) {
            return;
        }

        if (self->pointEntries != 0) {
            for (int i = 0; i < self->pointCount; ++i) {
                const zModel_PointEntryPartial &entry = self->pointEntries[i];
                if (entry.pointCamList == 0) {
                    continue;
                }
                for (int j = 0; j < entry.pointCamCount; ++j) {
                    IncludePoint(outBoundsMinMax, entry.pointCamList[j]);
                }
            }
        }

        for (int i = 1; i < self->vertCount; ++i) {
            IncludePoint(outBoundsMinMax, self->verts[i]);
        }

        if (self->blendVertCount > 0 && self->verts != 0 && self->blendVerts != 0) {
            for (int i = 0; i < self->blendVertCount; ++i) {
                zVec3 blended = {self->verts[i].x + self->blendVerts[i].x,
                              self->verts[i].y + self->blendVerts[i].y,
                              self->verts[i].z + self->blendVerts[i].z};
                IncludePoint(outBoundsMinMax, blended);
            }
        }
    }

    // Reimplements 0x483e60: zDi::BuildOriginSymmetricAabb
    RECOIL_NOINLINE void RECOIL_FASTCALL BuildOriginSymmetricAabb(
        zDiPartial * self, zBoundsMinMaxPartial * outBoundsMinMax) {
        BuildAabb(self, outBoundsMinMax);

        float extentX = max((float)fabs(outBoundsMinMax->min.x), outBoundsMinMax->max.x);
        float extentY = max((float)fabs(outBoundsMinMax->min.y), outBoundsMinMax->max.y);
        float extentZ = max((float)fabs(outBoundsMinMax->min.z), outBoundsMinMax->max.z);

        if ((self->flags & 0x10) != 0) {
            const float maxExtent = max(extentX, max(extentY, extentZ));
            outBoundsMinMax->min.x = -maxExtent;
            outBoundsMinMax->min.y = -maxExtent;
            outBoundsMinMax->min.z = -maxExtent;
            outBoundsMinMax->max.x = maxExtent;
            outBoundsMinMax->max.y = maxExtent;
            outBoundsMinMax->max.z = maxExtent;
            return;
        }

        if ((self->flags & 0x20) == 0) {
            const float xzExtent = max(extentX, extentZ);
            extentX = xzExtent;
            extentZ = xzExtent;
        }

        outBoundsMinMax->min.x = -extentX;
        outBoundsMinMax->min.y = -extentY;
        outBoundsMinMax->min.z = -extentZ;
        outBoundsMinMax->max.x = extentX;
        outBoundsMinMax->max.y = extentY;
        outBoundsMinMax->max.z = extentZ;
    }

    // Reimplements 0x483ad0: zDi::RebuildBounds
    RECOIL_NOINLINE void RECOIL_FASTCALL RebuildBounds(zDiPartial * self,
                                                       zBoundsMinMaxPartial * outBoundsMinMax) {
        if (self == 0 || outBoundsMinMax == 0) {
            return;
        }

        if (self->mode == 0) {
            BuildAabb(self, outBoundsMinMax);
        } else if (self->mode == 1) {
            BuildOriginSymmetricAabb(self, outBoundsMinMax);
        }

        const float halfX = (outBoundsMinMax->max.x - outBoundsMinMax->min.x) * 0.5f;
        const float halfY = (outBoundsMinMax->max.y - outBoundsMinMax->min.y) * 0.5f;
        const float halfZ = (outBoundsMinMax->max.z - outBoundsMinMax->min.z) * 0.5f;
        self->bboxCenter.x = halfX + outBoundsMinMax->min.x;
        self->bboxCenter.y = halfY + outBoundsMinMax->min.y;
        self->bboxCenter.z = halfZ + outBoundsMinMax->min.z;
        self->bboxRadius = FastSqrtApprox(halfX * halfX + halfY * halfY + halfZ * halfZ);
    }
}

namespace zModel_Material {
    // Reimplements 0x480c40: zModel_Material::ResetDefaults
    // (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL ResetDefaults(zModel_MaterialPartial * material) {
        material->cycle = 0;
        material->currentTextureDirectoryEntry = 0;
        material->flags = static_cast<unsigned short>((material->flags & 0xf800u) | 0x00ffu);
        material->colorRgb.red = 255.0f;
        material->colorRgb.green = 255.0f;
        material->colorRgb.blue = 255.0f;
        material->colorScalar = 0.5f;
        material->unknown_1c = 0.5f;
        material->unknown_14 = 0.0f;
        material->packedColor = 0x7fff;
        material->userTag = 0;
    }

    // Reimplements 0x480c80: zModel_Material::HasAuxData
    // (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL HasAuxData(zModel_MaterialPartial * material) {
        return (material->flags & 0x0200) != 0 || (material->flags & 0x0400) != 0 ||
                       material->cycle != 0
                   ? 1
                   : 0;
    }

    // Reimplements 0x480d20: zModel_Material::CompareForReuse
    // (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL CompareForReuse(zModel_MaterialPartial * lhs,
                                                                 zModel_MaterialPartial * rhs) {
        if (lhs->currentTextureDirectoryEntry != rhs->currentTextureDirectoryEntry) {
            return 1;
        }

        const int compare =
            memcmp(lhs, rhs, offsetof(zModel_MaterialPartial, userTag));
        if (compare != 0) {
            return compare < 0 ? -1 : 1;
        }

        if (lhs->userTag == rhs->userTag) {
            return 0;
        }

        if (lhs->userTag == 0) {
            lhs->userTag = rhs->userTag;
            return 0;
        }

        if (rhs->userTag == 0) {
            rhs->userTag = lhs->userTag;
            return 0;
        }

        return 1;
    }

    // Reimplements 0x481420: zModel_Material::FindByTexDirEntry
    // (D:\Proj\GameZRecoil\zModel\zmat.cpp)
    RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL FindByTexDirEntry(
        zImage_TexDirEntryPartial * texDirEntry) {
        if (texDirEntry == 0) {
            return 0;
        }

        int slotIndex = g_zModel_MatlActiveHeadIndex;
        while (slotIndex >= 0) {
            zModel_MaterialSlot *const slot = &g_zModel_MatlPool[slotIndex];
            if (slot->material.currentTextureDirectoryEntry == texDirEntry) {
                return &slot->material;
            }

            slotIndex = slot->nextPoolIndex;
        }

        return 0;
    }

    // Reimplements 0x480ca0: zModel_Material::FindOrClone
    // (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp)
    RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL FindOrClone(zModel_MaterialPartial *
                                                                        material) {
        zModel_MaterialPartial *reuseCache = g_zModel_MatlReuseCache;
        if (reuseCache != 0 && CompareForReuse(reuseCache, material) == 0) {
            return g_zModel_MatlReuseCache;
        }

        int slotIndex = g_zModel_MatlActiveHeadIndex;
        while (slotIndex >= 0) {
            zModel_MaterialSlot *const slot = &g_zModel_MatlPool[slotIndex];
            zModel_MaterialPartial *const candidate = &slot->material;
            if (CompareForReuse(candidate, material) == 0) {
                return candidate;
            }
            slotIndex = slot->nextPoolIndex;
        }

        g_zModel_MatlReuseCache = Clone(material);
        return g_zModel_MatlReuseCache;
    }

    // Reimplements 0x481040: zModel_Material::SetUserTag
    // (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetUserTag(zModel_MaterialPartial * material,
                                                            int userTag) {
        if (material == 0) {
            return 0;
        }

        material->userTag = userTag;
        return 1;
    }

    // Reimplements 0x481050: zModel_Material::SetCycleTextureCount
    // (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureCount(
        zModel_MaterialPartial * material, int textureCount) {
        if (material == &g_zModel_DefaultMaterial) {
            return 0;
        }

        zModel_MaterialCyclePartial *cycle = material->cycle;
        material->flags = static_cast<unsigned short>(material->flags | 0x0500);
        if (cycle != 0 && cycle->frameCount >= textureCount) {
            return 0;
        }

        cycle = static_cast<zModel_MaterialCyclePartial *>(
            realloc(cycle, sizeof(zModel_MaterialCyclePartial)));
        material->cycle = cycle;
        cycle->loopEnabled = 0;
        cycle->currentFrame = 0.0f;
        cycle->framesPerSecond = 15.0f;
        cycle->frameCount = textureCount;
        cycle->frameWriteCount = 0;
        cycle->frameTable = 0;
        cycle->frameTable = static_cast<zImage_TexDirEntryPartial **>(
            realloc(cycle->frameTable,
                         static_cast<size_t>(textureCount) * sizeof(cycle->frameTable[0])));

        for (int i = 0; i < textureCount; ++i) {
            cycle->frameTable[i] = zImage::GetDefaultImageRefPtr();
        }

        return 1;
    }

    // Reimplements 0x481100: zModel_Material::AddCycleTexture
    // (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL AddCycleTexture(
        zModel_MaterialPartial * material, zImage_TexDirEntryPartial * textureDirectoryEntry) {
        if ((material->flags & 0x0400) == 0) {
            return 0;
        }

        zModel_MaterialCyclePartial *cycle = material->cycle;
        zImage_TexDirEntryPartial **const frameTable = cycle->frameTable;
        if (frameTable == 0) {
            return 0;
        }

        const int frameWriteCount = cycle->frameWriteCount;
        if (frameWriteCount >= cycle->frameCount) {
            return 0;
        }

        frameTable[frameWriteCount] = textureDirectoryEntry;
        cycle = material->cycle;
        ++cycle->frameWriteCount;
        return 1;
    }

    // Reimplements 0x481220: zModel_Material::SetCycleTextureLoop
    // (D:\Proj\GameZRecoil\zModel\zmodel.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureLoop(
        zModel_MaterialPartial * material, int loopEnabled) {
        if (material != 0) {
            if ((material->flags & 0x0400) != 0) {
                zModel_MaterialCyclePartial *const cycle = material->cycle;
                if (cycle != 0) {
                    cycle->loopEnabled = loopEnabled;
                    return 1;
                }
            }

            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c", 0x5fb,
                              "SetCycleTextureLoop:  Texture not cycled");
        }

        return 0;
    }

    // Reimplements 0x481260: zModel_Material::SetCycleTextureSpeed
    // (D:\Proj\GameZRecoil\zModel\zmodel_mat.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureSpeed(
        zModel_MaterialPartial * material, float cycleSpeed) {
        if (material != 0) {
            if ((material->flags & 0x0400) != 0) {
                zModel_MaterialCyclePartial *const cycle = material->cycle;
                if (cycle != 0) {
                    memcpy(&cycle->framesPerSecond, &cycleSpeed,
                                sizeof(cycle->framesPerSecond));
                    return 1;
                }
            }

            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c", 0x60f,
                              "SetCycleTextureSpeed:  Texture not cycled");
        }

        return 0;
    }
}

namespace zModel_MatlBuffer {
    // Reimplements 0x4812c0: zModel_MatlBuffer::CloneToActiveSlot
    // (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
    RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL CloneToActiveSlot(
        zModel_MaterialPartial * material) {
        if (material == 0) {
            return 0;
        }

        const int slotIndex = g_zModel_MatlFreeHeadIndex;
        if (slotIndex < 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c", 0x626,
                              "ERROR: Copying material; material buffer full; using default.");
            return &g_zModel_DefaultMaterial;
        }

        zModel_MaterialSlot *const slot = &g_zModel_MatlPool[slotIndex];
        const int nextFreeIndex = slot->nextPoolIndex;
        const int prevFreeIndex = slot->prevPoolIndex;
        if (prevFreeIndex >= 0) {
            g_zModel_MatlPool[prevFreeIndex].nextPoolIndex =
                static_cast<short>(nextFreeIndex);
        }
        if (nextFreeIndex >= 0) {
            g_zModel_MatlPool[nextFreeIndex].prevPoolIndex =
                static_cast<short>(prevFreeIndex);
        }

        g_zModel_MatlFreeHeadIndex = nextFreeIndex;
        slot->prevPoolIndex = -1;
        slot->nextPoolIndex = static_cast<short>(g_zModel_MatlActiveHeadIndex);
        if (g_zModel_MatlActiveHeadIndex >= 0) {
            g_zModel_MatlPool[g_zModel_MatlActiveHeadIndex].prevPoolIndex =
                static_cast<short>(slotIndex);
        }
        g_zModel_MatlActiveHeadIndex = slotIndex;
        ++g_zModel_MatlPoolInUseCount;

        memcpy(&slot->material, material, offsetof(zModel_MaterialPartial, cycle));
        if ((material->flags & 0x0400) == 0) {
            slot->material.cycle = 0;
            return &slot->material;
        }

        slot->material.cycle = static_cast<zModel_MaterialCyclePartial *>(
            malloc(sizeof(zModel_MaterialCyclePartial)));
        memcpy(slot->material.cycle, material->cycle, sizeof(zModel_MaterialCyclePartial));
        slot->material.cycle->frameTable = static_cast<zImage_TexDirEntryPartial **>(
            calloc(static_cast<size_t>(slot->material.cycle->frameCount),
                        sizeof(slot->material.cycle->frameTable[0])));
        memcpy(slot->material.cycle->frameTable, material->cycle->frameTable,
                    static_cast<size_t>(slot->material.cycle->frameCount) *
                        sizeof(slot->material.cycle->frameTable[0]));

        return &slot->material;
    }
}

namespace zModel_Material {
    // Reimplements 0x4812b0: zModel_Material::Clone
    // (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
    RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL Clone(zModel_MaterialPartial *
                                                                  material) {
        return zModel_MatlBuffer::CloneToActiveSlot(material);
    }
}

// Reimplements 0x480f60: zModel_Material_SetFlagBit9
RECOIL_NOINLINE int RECOIL_FASTCALL
zModel_Material_SetFlagBit9(zModel_MaterialPartial *material, int enabled) {
    if (material == 0) {
        return 0;
    }

    material->flags = static_cast<unsigned short>((material->flags & 0xfdff) | ((enabled & 1) << 9));
    return 1;
}

namespace zModel_Material {
    // Reimplements 0x480f80: zModel_Material::InvalidateImagesIfEligible
    RECOIL_NOINLINE void RECOIL_FASTCALL InvalidateImagesIfEligible(zModel_MaterialPartial *
                                                                    material) {
        if (material == 0 || (material->flags & 0x0300) != 0x0300) {
            return;
        }

        zImage::InvalidateLoadedVariantChain(material->currentTextureDirectoryEntry);
        zModel_MaterialCyclePartial *cycle = material->cycle;
        if (cycle == 0) {
            return;
        }

        for (int i = 0; i < cycle->frameCount; ++i) {
            zImage::InvalidateLoadedVariantChain(cycle->frameTable[i]);
        }
    }
}

// Reimplements 0x4841b0: zDi_SetMaterialFlagBit9ForFlagBit0Entries
RECOIL_NOINLINE void RECOIL_FASTCALL
zDi_SetMaterialFlagBit9ForFlagBit0Entries(zDiPartial *self, int enabled) {
    for (int i = 0; i < self->entryCount; ++i) {
        zModel_MaterialPartial *material = self->entries[i].material;
        if ((material->flags & 0x0100) != 0) {
            zModel_Material_SetFlagBit9(material, enabled);
        }
    }
}

namespace zDi {
    // Reimplements 0x4841f0: zDi::InvalidateImagesForFlagBit8Materials
    RECOIL_NOINLINE void RECOIL_FASTCALL InvalidateImagesForFlagBit8Materials(zDiPartial * self) {
        for (int i = 0; i < self->entryCount; ++i) {
            zModel_MaterialPartial *material = self->entries[i].material;
            if ((material->flags & 0x0100) != 0) {
                zModel_Material::InvalidateImagesIfEligible(material);
            }
        }
    }
}
