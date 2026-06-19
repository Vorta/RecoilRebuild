#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
    /**
     * Original inline helper evidence: no standalone retail function exists;
     * observed fully inlined in 0x483b80 zDi::BuildAabb.
     * Purpose: expand a min/max bounds record to include one point.
     */
    inline void IncludePoint(
        zBoundsMinMaxPartial * bounds,
        const zVec3 *point
    ) {
        if (point->x < bounds->min.x) {
            bounds->min.x = point->x;
        }
        if (bounds->max.x < point->x) {
            bounds->max.x = point->x;
        }
        if (point->y < bounds->min.y) {
            bounds->min.y = point->y;
        }
        if (bounds->max.y < point->y) {
            bounds->max.y = point->y;
        }
        if (point->z < bounds->min.z) {
            bounds->min.z = point->z;
        }
        if (bounds->max.z < point->z) {
            bounds->max.z = point->z;
        }
    }

    /**
     * Original inline helper evidence: no standalone retail function exists;
     * observed fully inlined in 0x483b80 zDi::BuildAabb.
     * Purpose: initialize a min/max bounds record from its first point.
     */
    inline void InitializeBounds(
        zBoundsMinMaxPartial * bounds,
        const zVec3 *point
    ) {
        bounds->min = *point;
        bounds->max = *point;
    }

    struct MaterialClonePair {
        zModel_MaterialPartial *source;
        zModel_MaterialPartial *clone;
    };

    /**
     * Original static helper observed in zModel display-instance clone code
     * (D:\Proj\GameZRecoil\zModel\gdi.c).
     * Purpose: allocate and copy an optional entry-owned byte array.
     */
    void *CopyArrayBytes(
        const void *source,
        size_t byteCount
    ) {
        void *const copy = malloc(byteCount);
        if (copy != 0 && source != 0 && byteCount != 0) {
            memcpy(
                copy,
                source,
                byteCount
            );
        }
        return copy;
    }

    /**
     * Original static helper observed in zModel display-instance clone code
     * (D:\Proj\GameZRecoil\zModel\gdi.c).
     * Purpose: clone an entry-owned array only when the source entry has bytes to copy.
     */
    void CopyEntryArrayIfPresent(
        void **dest,
        void *source,
        size_t byteCount
    ) {
        if (byteCount != 0) {
            *dest = CopyArrayBytes(
                source,
                byteCount
            );
        }
    }
}

namespace zDi {
    /**
     * Reimplements 0x4826f0: zDi::AddRef
     * (GameZRecoil/zDi/zdi.c).
     * Purpose: increment a display-instance reference count.
     */
    int __fastcall AddRef(zDiPartial * self) {
        ++self->refCount;
        return 0;
    }

    /**
     * Reimplements 0x482700: zDi::Release
     * (GameZRecoil/zDi/zdi.c).
     * Purpose: decrement a display-instance reference count.
     */
    int __fastcall Release(zDiPartial * self) {
        --self->refCount;
        return 0;
    }

    /**
     * Reimplements 0x482710: zDi::GetRefCount
     * (GameZRecoil/zModel/gdi.c).
     * Purpose: return a display-instance reference count.
     */
    int __fastcall GetRefCount(zDiPartial * self) {
        return self->refCount;
    }

    /**
     * Reimplements 0x482160: zDi::FreeContents
     * (GameZRecoil/zModel/gdi.c).
     * Purpose: release all heap-owned arrays and materials held by a display instance.
     */
    int __fastcall FreeContents(zDiPartial * self) {
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

    /**
     * Reimplements 0x4826d0: zDi::SetFlagBit0
     * (GameZRecoil/zModel/gdi.c).
     * Purpose: update display-instance flag bit 0 while preserving other flags.
     */
    void __fastcall SetFlagBit0(
        zDiPartial * self,
        int enabled
    ) {
        if (self != 0) {
            self->flags = ((enabled ^ self->flags) & 1) ^ self->flags;
        }
    }

    /**
     * Reimplements 0x4826b0: zDi::SetClonedFlag
     * (GameZRecoil/zModel/gdi.c).
     * Purpose: update the display-instance cloned flag bit.
     */
    void __fastcall SetClonedFlag(
        zDiPartial * self,
        int isCloned
    ) {
        if (self != 0) {
            self->flags = (self->flags & ~0x02) | ((isCloned & 1) << 1);
        }
    }

    /**
     * Reimplements 0x482270: zDi::CloneToInstance
     * (GameZRecoil/zModel/gdi.c).
     * Purpose: clone a display instance, optionally cloning or sharing its material references.
     */
    zDiPartial *__fastcall CloneToInstance(
        zDiPartial * self,
        int cloneMaterials,
        int cloneAuxOnly
    ) {
        if (self == 0) {
            return 0;
        }

        zDiPartial *const clone = zModel_DiPool::AllocFromFreeList();
        if (clone == 0) {
            return 0;
        }

        clone->mode = self->mode;
        clone->refCount = 0;
        SetFlagBit0(
            clone,
            self->flags & 1
        );
        SetClonedFlag(
            clone,
            (self->flags >> 1) & 1
        );
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
            clone->pointEntries = (zModel_PointEntryPartial *)(malloc(
                (size_t)(self->pointCount) * sizeof(zModel_PointEntryPartial)
            ));
            for (int i = 0; i < self->pointCount; ++i) {
                clone->pointEntries[i] = self->pointEntries[i];
                if (self->pointEntries[i].pointCamCount > 0) {
                    clone->pointEntries[i].pointCamList = (zVec3 *)(CopyArrayBytes(
                        self->pointEntries[i].pointCamList,
                        (size_t)(self->pointEntries[i].pointCamCount) * sizeof(zVec3)
                    ));
                }
            }
        }

        clone->blendVertCount = self->blendVertCount;
        if (self->blendVertCount > 0) {
            clone->blendVerts = (zVec3 *)(CopyArrayBytes(
                self->blendVerts,
                (size_t)(self->blendVertCount) * sizeof(zVec3)
            ));
        }

        clone->vertCount = self->vertCount;
        if (self->vertCount > 0) {
            clone->verts =
                (zVec3 *)(CopyArrayBytes(
                    self->verts,
                    (size_t)(self->vertCount) * sizeof(zVec3)
                ));
        }

        clone->normalCount = self->normalCount;
        if (self->normalCount > 0) {
            clone->normals = (zVec3 *)(CopyArrayBytes(
                self->normals,
                (size_t)(self->normalCount) * sizeof(zVec3)
            ));
        }

        clone->entryCount = self->entryCount;
        if (self->entryCount > 0) {
            clone->entries =
                (zDiEntryPartial *)(calloc(
                    (size_t)(self->entryCount),
                    sizeof(zDiEntryPartial)
                ));
        }

        MaterialClonePair *materialPairs = 0;
        int materialPairCount = 0;
        for (int i = 0; i < self->entryCount; ++i) {
            const zDiEntryPartial &sourceEntry = self->entries[i];
            zDiEntryPartial &destEntry = clone->entries[i];

            destEntry.drawFlags = sourceEntry.drawFlags;
            destEntry.flagsAndIndexCount = sourceEntry.flagsAndIndexCount & 0x00000300;
            memcpy(
                &destEntry.variantTagInitialized,
                &sourceEntry.variantTagInitialized,
                4
            );

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
                        materialPairs = (MaterialClonePair *)(realloc(
                            materialPairs,
                            (size_t)(materialPairCount + 1) * sizeof(MaterialClonePair)
                        ));
                        materialPairs[materialPairCount].source = sourceEntry.material;
                        materialPairs[materialPairCount].clone = material;
                        ++materialPairCount;
                    }
                }
            }
            destEntry.material = material;

            const unsigned int indexCount = sourceEntry.flagsAndIndexCount & 0xff;
            CopyEntryArrayIfPresent(
                &destEntry.vertexIndices,
                sourceEntry.vertexIndices,
                (size_t)(indexCount) * sizeof(unsigned int)
            );
            if ((sourceEntry.flagsAndIndexCount & 0x00000200) != 0 &&
                sourceEntry.normalIndices != 0) {
                CopyEntryArrayIfPresent(
                    &destEntry.normalIndices,
                    sourceEntry.normalIndices,
                    (size_t)(indexCount) * sizeof(unsigned int)
                );
            }

            destEntry.flagsAndIndexCount = (destEntry.flagsAndIndexCount & ~0xffu) | indexCount;
            if ((destEntry.material->flags & 0x0100) != 0) {
                CopyEntryArrayIfPresent(
                    &destEntry.uvPairs,
                    sourceEntry.uvPairs,
                    (size_t)(indexCount) * 8u
                );
            }
        }

        if (materialPairs != 0) {
            free(materialPairs);
        }

        return clone;
    }

    /**
     * Reimplements 0x483a60: zDi::HasSpecialFlagsOrAuxMaterialData
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: test whether a display instance needs special render/material handling.
     */
    int __fastcall HasSpecialFlagsOrAuxMaterialData(zDiPartial * self) {
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

    /**
     * Reimplements 0x483b80: zDi::BuildAabb
     * (D:\Proj\GameZRecoil\zModel\gdi.c).
     * Purpose: build a display-instance axis-aligned bounds box from vertices and point data.
     */
    void __fastcall BuildAabb(
        zDiPartial * self,
        zBoundsMinMaxPartial * outBoundsMinMax
    ) {
        int i;
        int j;

        if (self->vertCount > 0) {
            InitializeBounds(
                outBoundsMinMax,
                &self->verts[0]
            );
        } else if (self->pointCount > 0) {
            InitializeBounds(
                outBoundsMinMax,
                &self->pointEntries[0].pointCamList[0]
            );
        }

        for (i = 0; i < self->pointCount; ++i) {
            zModel_PointEntryPartial *entry = &self->pointEntries[i];
            for (j = 0; j < entry->pointCamCount; ++j) {
                IncludePoint(
                    outBoundsMinMax,
                    &entry->pointCamList[j]
                );
            }
        }

        for (i = 1; i < self->vertCount; ++i) {
            IncludePoint(
                outBoundsMinMax,
                &self->verts[i]
            );
        }

        if (self->blendVertCount > 0) {
            zMath_Vec3Array_AddScaled(
                g_zModel_SharedVec3ScratchA,
                self->verts,
                self->blendVerts,
                self->blendVertCount,
                1.0f
            );
            for (i = 0; i < self->blendVertCount; ++i) {
                IncludePoint(
                    outBoundsMinMax,
                    &g_zModel_SharedVec3ScratchA[i]
                );
            }
        }
    }

    /**
     * Reimplements 0x483e60: zDi::BuildOriginSymmetricAabb
     * (D:\Proj\GameZRecoil\zModel\gdi.c).
     * Purpose: symmetrize display-instance bounds around the origin according to mode flags.
     */
    void __fastcall BuildOriginSymmetricAabb(
        zDiPartial * self,
        zBoundsMinMaxPartial * outBoundsMinMax
    ) {
        BuildAabb(
            self,
            outBoundsMinMax
        );

        float extentX = (float)fabs(outBoundsMinMax->min.x);
        if (extentX < outBoundsMinMax->max.x) {
            extentX = outBoundsMinMax->max.x;
        }
        float extentY = (float)fabs(outBoundsMinMax->min.y);
        if (extentY < outBoundsMinMax->max.y) {
            extentY = outBoundsMinMax->max.y;
        }
        float extentZ = (float)fabs(outBoundsMinMax->min.z);
        if (extentZ < outBoundsMinMax->max.z) {
            extentZ = outBoundsMinMax->max.z;
        }

        if ((self->flags & 0x10) != 0) {
            float maxExtent = extentX;
            if (maxExtent < extentY) {
                maxExtent = extentY;
            }
            if (maxExtent < extentZ) {
                maxExtent = extentZ;
            }
            outBoundsMinMax->min.x = -maxExtent;
            outBoundsMinMax->min.y = -maxExtent;
            outBoundsMinMax->min.z = -maxExtent;
            outBoundsMinMax->max.x = maxExtent;
            outBoundsMinMax->max.y = maxExtent;
            outBoundsMinMax->max.z = maxExtent;
            return;
        }

        if (extentX < extentZ) {
            extentX = extentZ;
        } else {
            extentZ = extentX;
        }

        outBoundsMinMax->min.x = -extentX;
        outBoundsMinMax->min.y = -extentY;
        outBoundsMinMax->min.z = -extentZ;
        outBoundsMinMax->max.x = extentX;
        outBoundsMinMax->max.y = extentY;
        outBoundsMinMax->max.z = extentZ;
    }

    /**
     * Reimplements 0x483ad0: zDi::RebuildBounds
     * (D:\Proj\GameZRecoil\zModel\gdi.c).
     * Purpose: rebuild display-instance bounds, center, and approximate bounding radius.
     */
    void __fastcall RebuildBounds(
        zDiPartial * self,
        zBoundsMinMaxPartial * outBoundsMinMax
    ) {
        if (self == 0 || outBoundsMinMax == 0) {
            return;
        }

        if (self->mode == 0) {
            BuildAabb(
                self,
                outBoundsMinMax
            );
        } else if (self->mode == 1) {
            BuildOriginSymmetricAabb(
                self,
                outBoundsMinMax
            );
        }

        const float halfX = (outBoundsMinMax->max.x - outBoundsMinMax->min.x) * 0.5f;
        const float halfY = (outBoundsMinMax->max.y - outBoundsMinMax->min.y) * 0.5f;
        const float halfZ = (outBoundsMinMax->max.z - outBoundsMinMax->min.z) * 0.5f;
        self->bboxCenter.x = halfX + outBoundsMinMax->min.x;
        self->bboxCenter.y = halfY + outBoundsMinMax->min.y;
        self->bboxCenter.z = halfZ + outBoundsMinMax->min.z;
        union {
            float radius;
            int bits;
        } radiusEstimate;
        radiusEstimate.radius = halfX * halfX + halfY * halfY + halfZ * halfZ;
        radiusEstimate.bits = (radiusEstimate.bits >> 1) + 0x1fc00000;
        self->bboxRadius = radiusEstimate.radius;
    }
}

namespace zModel_Material {
    /**
     * Reimplements 0x480c40: zModel_Material::ResetDefaults
     * (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp).
     * Purpose: reset a material record to the default texture, color, and flag state.
     */
    void __fastcall ResetDefaults(zModel_MaterialPartial * material) {
        material->cycle = 0;
        material->currentTextureDirectoryEntry = 0;
        material->flags = (unsigned short)((material->flags & 0xf800u) | 0x00ffu);
        material->colorRgb.red = 255.0f;
        material->colorRgb.green = 255.0f;
        material->colorRgb.blue = 255.0f;
        material->colorScalar = 0.5f;
        material->unknown_1c = 0.5f;
        material->unknown_14 = 0.0f;
        material->packedColor = 0x7fff;
        material->userTag = 0;
    }

    /**
     * Reimplements 0x480c80: zModel_Material::HasAuxData
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     * Purpose: test whether a material has auxiliary data or cycle state.
     */
    int __fastcall HasAuxData(zModel_MaterialPartial * material) {
        return (material->flags & 0x0200) != 0 || (material->flags & 0x0400) != 0 ||
                       material->cycle != 0
                   ? 1
                   : 0;
    }

    /**
     * Reimplements 0x480d20: zModel_Material::CompareForReuse
     * (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp).
     * Purpose: compare two material records for reuse, merging missing user tags when possible.
     */
    int __fastcall CompareForReuse(
        zModel_MaterialPartial * lhs,
        zModel_MaterialPartial * rhs
    ) {
        if (lhs->currentTextureDirectoryEntry != rhs->currentTextureDirectoryEntry) {
            return 1;
        }

        const int compare = memcmp(
            lhs,
            rhs,
            offsetof(zModel_MaterialPartial, userTag)
        );
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

    /**
     * Reimplements 0x481420: zModel_Material::FindByTexDirEntry
     * (D:\Proj\GameZRecoil\zModel\zmat.cpp).
     * Purpose: find the active material that references a texture-directory entry.
     */
    zModel_MaterialPartial *__fastcall FindByTexDirEntry(
        zImage_TexDirEntryPartial * texDirEntry
    ) {
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

    /**
     * Reimplements 0x480ca0: zModel_Material::FindOrClone
     * (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp).
     * Purpose: reuse a matching active material or clone the supplied material into the pool.
     */
    zModel_MaterialPartial *__fastcall FindOrClone(
        zModel_MaterialPartial * material
    ) {
        zModel_MaterialPartial *reuseCache = g_zModel_MatlReuseCache;
        if (reuseCache != 0 && CompareForReuse(
            reuseCache,
            material
        ) == 0) {
            return g_zModel_MatlReuseCache;
        }

        int slotIndex = g_zModel_MatlActiveHeadIndex;
        while (slotIndex >= 0) {
            zModel_MaterialSlot *const slot = &g_zModel_MatlPool[slotIndex];
            zModel_MaterialPartial *const candidate = &slot->material;
            if (CompareForReuse(
                candidate,
                material
            ) == 0) {
                return candidate;
            }
            slotIndex = slot->nextPoolIndex;
        }

        g_zModel_MatlReuseCache = Clone(material);
        return g_zModel_MatlReuseCache;
    }

    /**
     * Reimplements 0x481040: zModel_Material::SetUserTag
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     * Purpose: assign a caller-defined material user tag.
     */
    int __fastcall SetUserTag(
        zModel_MaterialPartial * material,
        int userTag
    ) {
        if (material == 0) {
            return 0;
        }

        material->userTag = userTag;
        return 1;
    }

    /**
     * Reimplements 0x481050: zModel_Material::SetCycleTextureCount
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: allocate or grow the material texture-cycle frame table.
     */
    int __fastcall SetCycleTextureCount(
        zModel_MaterialPartial * material,
        int textureCount
    ) {
        if (material == &g_zModel_DefaultMaterial) {
            return 0;
        }

        zModel_MaterialCyclePartial *cycle = material->cycle;
        material->flags = (unsigned short)(material->flags | 0x0500);
        if (cycle != 0 && cycle->frameCount >= textureCount) {
            return 0;
        }

        cycle =
            (zModel_MaterialCyclePartial *)(realloc(
                cycle,
                sizeof(zModel_MaterialCyclePartial)
            ));
        material->cycle = cycle;
        cycle->loopEnabled = 0;
        cycle->currentFrame = 0.0f;
        cycle->framesPerSecond = 15.0f;
        cycle->frameCount = textureCount;
        cycle->frameWriteCount = 0;
        cycle->frameTable = 0;
        cycle->frameTable = (zImage_TexDirEntryPartial **)(realloc(
            cycle->frameTable,
            (size_t)(textureCount) * sizeof(cycle->frameTable[0])
        ));

        for (int i = 0; i < textureCount; ++i) {
            cycle->frameTable[i] = zImage::GetDefaultImageRefPtr();
        }

        return 1;
    }

    /**
     * Reimplements 0x481100: zModel_Material::AddCycleTexture
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: append one texture-directory entry to a material cycle.
     */
    int __fastcall AddCycleTexture(
        zModel_MaterialPartial * material,
        zImage_TexDirEntryPartial * textureDirectoryEntry
    ) {
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

    /**
     * Reimplements 0x481140: zModel_Material::UpdateCycleIfNeeded
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     * Purpose: advance a cycled material texture once per video frame tick.
     */
    void __fastcall UpdateCycleIfNeeded(zModel_MaterialPartial * material) {
        zModel_MaterialCyclePartial *cycle = material->cycle;
        if (cycle == 0) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c",
                0x5ca,
                "Material Cycle Pointer is NULL: flag is (%s)",
                (material->flags & 0x0400) != 0 ? "TRUE" : "FALSE"
            );
            return;
        }

        if (cycle->lastUpdateFrameTick == g_zVideo_FrameTick) {
            return;
        }

        const int frameIndex = (int)(cycle->currentFrame) % cycle->frameCount;
        material->currentTextureDirectoryEntry = cycle->frameTable[frameIndex];
        cycle->currentFrame += cycle->framesPerSecond * g_FrameDeltaTimeSec;

        cycle = material->cycle;
        if (cycle->loopEnabled == 0 && (float)(cycle->frameCount) <= cycle->currentFrame) {
            cycle->currentFrame = (float)(cycle->frameCount - 1);
        }

        cycle = material->cycle;
        if (cycle->currentFrame < 0.0f) {
            cycle->currentFrame += (float)((int)(fabs(cycle->framesPerSecond)) * cycle->frameCount);
        }

        material->cycle->lastUpdateFrameTick = g_zVideo_FrameTick;
    }

    /**
     * Reimplements 0x481220: zModel_Material::SetCycleTextureLoop
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: set whether a cycled material loops after the last frame.
     */
    int __fastcall SetCycleTextureLoop(
        zModel_MaterialPartial * material,
        int loopEnabled
    ) {
        if (material != 0) {
            if ((material->flags & 0x0400) != 0) {
                zModel_MaterialCyclePartial *const cycle = material->cycle;
                if (cycle != 0) {
                    cycle->loopEnabled = loopEnabled;
                    return 1;
                }
            }

            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c",
                0x5fb,
                "SetCycleTextureLoop:  Texture not cycled"
            );
        }

        return 0;
    }

    /**
     * Reimplements 0x481260: zModel_Material::SetCycleTextureSpeed
     * (D:\Proj\GameZRecoil\zModel\zmodel_mat.cpp).
     * Purpose: set the frames-per-second speed for a cycled material.
     */
    int __fastcall SetCycleTextureSpeed(
        zModel_MaterialPartial * material,
        float cycleSpeed
    ) {
        if (material != 0) {
            if ((material->flags & 0x0400) != 0) {
                zModel_MaterialCyclePartial *const cycle = material->cycle;
                if (cycle != 0) {
                    memcpy(
                        &cycle->framesPerSecond,
                        &cycleSpeed,
                        sizeof(cycle->framesPerSecond)
                    );
                    return 1;
                }
            }

            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c",
                0x60f,
                "SetCycleTextureSpeed:  Texture not cycled"
            );
        }

        return 0;
    }
}

namespace zModel_MatlBuffer {
    /**
     * Reimplements 0x4812c0: zModel_MatlBuffer::CloneToActiveSlot
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     * Purpose: clone a material into a free material-buffer slot and link it active.
     */
    zModel_MaterialPartial *__fastcall CloneToActiveSlot(
        zModel_MaterialPartial * material
    ) {
        if (material == 0) {
            return 0;
        }

        const int slotIndex = g_zModel_MatlFreeHeadIndex;
        if (slotIndex < 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c",
                0x626,
                "ERROR: Copying material; material buffer full; using default."
            );
            return &g_zModel_DefaultMaterial;
        }

        zModel_MaterialSlot *const slot = &g_zModel_MatlPool[slotIndex];
        const int nextFreeIndex = slot->nextPoolIndex;
        const int prevFreeIndex = slot->prevPoolIndex;
        if (prevFreeIndex >= 0) {
            g_zModel_MatlPool[prevFreeIndex].nextPoolIndex = (short)(nextFreeIndex);
        }
        if (nextFreeIndex >= 0) {
            g_zModel_MatlPool[nextFreeIndex].prevPoolIndex = (short)(prevFreeIndex);
        }

        g_zModel_MatlFreeHeadIndex = nextFreeIndex;
        slot->prevPoolIndex = -1;
        slot->nextPoolIndex = (short)(g_zModel_MatlActiveHeadIndex);
        if (g_zModel_MatlActiveHeadIndex >= 0) {
            g_zModel_MatlPool[g_zModel_MatlActiveHeadIndex].prevPoolIndex = (short)(slotIndex);
        }
        g_zModel_MatlActiveHeadIndex = slotIndex;
        ++g_zModel_MatlPoolInUseCount;

        memcpy(
            &slot->material,
            material,
            offsetof(zModel_MaterialPartial, cycle)
        );
        if ((material->flags & 0x0400) == 0) {
            slot->material.cycle = 0;
            return &slot->material;
        }

        slot->material.cycle =
            (zModel_MaterialCyclePartial *)(malloc(sizeof(zModel_MaterialCyclePartial)));
        memcpy(
            slot->material.cycle,
            material->cycle,
            sizeof(zModel_MaterialCyclePartial)
        );
        slot->material.cycle->frameTable = (zImage_TexDirEntryPartial **)(calloc(
            (size_t)(slot->material.cycle->frameCount),
            sizeof(slot->material.cycle->frameTable[0])
        ));
        memcpy(
            slot->material.cycle->frameTable,
            material->cycle->frameTable,
            (size_t)(slot->material.cycle->frameCount) * sizeof(slot->material.cycle->frameTable[0])
        );

        return &slot->material;
    }
}

namespace zModel_Material {
    /**
     * Reimplements 0x4812b0: zModel_Material::Clone
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     * Purpose: clone a material through the active material-buffer slot allocator.
     */
    zModel_MaterialPartial *__fastcall Clone(
        zModel_MaterialPartial * material
    ) {
        return zModel_MatlBuffer::CloneToActiveSlot(material);
    }
}

namespace zModel_Material {
    /**
     * Reimplements 0x480f60: zModel_Material::SetFlagBit9
     * Source: D:\Proj\GameZRecoil\zModel\gdi.c
     * Purpose: update material flag bit 9 from a boolean input.
     */
    int __fastcall SetFlagBit9(
        zModel_MaterialPartial *material,
        int enabled
    ) {
        if (material == 0) {
            return 0;
        }

        material->flags = (unsigned short)((material->flags & 0xfdff) | ((enabled & 1) << 9));
        return 1;
    }
}

namespace zModel_Material {
    /**
     * Reimplements 0x480f80: zModel_Material::InvalidateImagesIfEligible
     * (D:\Proj\GameZRecoil\zModel\gdi.c).
     * Purpose: invalidate texture variants for materials with loaded texture surfaces.
     */
    void __fastcall InvalidateImagesIfEligible(
        zModel_MaterialPartial * material
    ) {
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

namespace zDi {
    /**
     * Reimplements 0x4841b0: zDi::SetMaterialFlagBit9ForFlagBit0Entries
     * Source: D:\Proj\GameZRecoil\zModel\gdi.c
     * Purpose: set material flag bit 9 for display-instance materials whose
     * flag bit 8 (0x0100) is set.
     */
    void __fastcall SetMaterialFlagBit9ForFlagBit0Entries(
        zDiPartial *self,
        int enabled
    ) {
        for (int i = 0; i < self->entryCount; ++i) {
            zModel_MaterialPartial *material = self->entries[i].material;
            if ((material->flags & 0x0100) != 0) {
                zModel_Material::SetFlagBit9(
                    material,
                    enabled
                );
            }
        }
    }
}

namespace zDi {
    /**
     * Reimplements 0x4841f0: zDi::InvalidateImagesForFlagBit8Materials
     * (D:\Proj\GameZRecoil\zModel\gdi.c).
     * Purpose: invalidate eligible images for display-instance materials selected by flag bit 0.
     */
    void __fastcall InvalidateImagesForFlagBit8Materials(zDiPartial * self) {
        for (int i = 0; i < self->entryCount; ++i) {
            zModel_MaterialPartial *material = self->entries[i].material;
            if ((material->flags & 0x0100) != 0) {
                zModel_Material::InvalidateImagesIfEligible(material);
            }
        }
    }
}
