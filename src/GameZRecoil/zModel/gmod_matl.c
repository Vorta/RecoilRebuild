#include "GameZRecoil/zModel/gmod.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Reimplements data 0x566a1c: Symbol.
 * Authored zModel material-pool global.
 * Purpose: point at the allocated material-slot pool storage.
 */
zModel_MaterialSlot *g_zModel_MatlPool = 0;
/**
 * Reimplements data 0x566a18: Symbol.
 * Authored zModel material-pool global.
 * Purpose: record the configured material-slot pool capacity.
 */
int g_zModel_MatlPoolCapacity = 0;
/**
 * Reimplements data 0x566a20: Symbol.
 * Authored zModel material-pool global.
 * Purpose: count material-slot pool entries currently allocated.
 */
int g_zModel_MatlPoolInUseCount = 0;
/**
 * Reimplements data 0x4e1160: Symbol.
 * Authored zModel material-pool global.
 * Purpose: hold the head index of the material-slot free list.
 */
int g_zModel_MatlFreeHeadIndex = -1;
/**
 * Reimplements data 0x4e1164: Symbol.
 * Authored zModel material-pool global.
 * Purpose: hold the head index of the active material-slot list.
 */
int g_zModel_MatlActiveHeadIndex = -1;
/**
 * Reimplements data 0x566a24: Symbol.
 * Authored zModel material-pool global.
 * Purpose: cache a reusable material record for display material allocation.
 */
zModel_MaterialPartial *g_zModel_MatlReuseCache = 0;
zModel_MaterialPartial g_zModel_DefaultMaterial = {0};
namespace {
extern char g_ZrdrGlobalString_Default[];
extern char g_ZrdrGlobalString_Water[];
extern char g_ZrdrGlobalString_Seafloor[];
extern char g_ZrdrGlobalString_Quicksand[];
extern char g_ZrdrGlobalString_Lava[];
extern char g_ZrdrGlobalString_Fire[];
} // namespace
/**
 * Reimplements data 0x4e0fc8: Symbol.
 * Authored renderer global-string table data.
 * Purpose: track the active count in the fixed-prefix plus dynamic string table.
 */
int g_zRndr_GlobalStringCount = 6;
/**
 * Reimplements data 0x4e0fd0: Symbol.
 * Authored renderer global-string table data.
 * Purpose: store the six built-in renderer prefixes followed by dynamic entries loaded at runtime.
 */
char *g_zRndr_GlobalStringTable[100] = {
    g_ZrdrGlobalString_Default,
    g_ZrdrGlobalString_Water,
    g_ZrdrGlobalString_Seafloor,
    g_ZrdrGlobalString_Quicksand,
    g_ZrdrGlobalString_Lava,
    g_ZrdrGlobalString_Fire
};

namespace {
/**
 * Reimplements data 0x4e1168: g_ZrdrGlobalString_Fire.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "fire" renderer prefix.
 */
char g_ZrdrGlobalString_Fire[] = "fire";
/**
 * Reimplements data 0x4e1170: g_ZrdrGlobalString_Lava.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "lava" renderer prefix.
 */
char g_ZrdrGlobalString_Lava[] = "lava";
/**
 * Reimplements data 0x4e1178: g_ZrdrGlobalString_Quicksand.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "quicksand" renderer prefix.
 */
char g_ZrdrGlobalString_Quicksand[] = "quicksand";
/**
 * Reimplements data 0x4e1184: g_ZrdrGlobalString_Seafloor.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "seafloor" renderer prefix.
 */
char g_ZrdrGlobalString_Seafloor[] = "seafloor";
/**
 * Reimplements data 0x4e1190: g_ZrdrGlobalString_Water.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "water" renderer prefix.
 */
char g_ZrdrGlobalString_Water[] = "water";
/**
 * Reimplements data 0x4e1198: g_ZrdrGlobalString_Default.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "default" renderer prefix.
 */
char g_ZrdrGlobalString_Default[] = "default";
} // namespace

/*
 * BN identifies the gmod_matl.c diagnostics as writable .data char arrays in
 * this order, including VC alignment padding between rows.
 */
/**
 * Reimplements data 0x4e11a0: g_zModel_GModMatl_FILE.
 * Purpose: store the writable source-file path passed to material diagnostics.
 */
char g_zModel_GModMatl_FILE[0x27] = "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c";
/**
 * Reimplements data 0x4e11c8: g_zModel_Matl_ErrWriteBuffer.
 * Purpose: store the writable material-buffer write failure diagnostic.
 */
char g_zModel_Matl_ErrWriteBuffer[0x1f] = "Error writing material buffer.";
/**
 * Reimplements data 0x4e11e8: g_zModel_ReadMaterialCycleTextureDataErrorMsg.
 * Purpose: store the writable material-cycle read failure diagnostic.
 */
char g_zModel_ReadMaterialCycleTextureDataErrorMsg[0x31] =
    "Error reading GameZ Material cycle texture data.";
/**
 * Reimplements data 0x4e121c: g_zModel_ReadMaterialBufferDataErrorMsg.
 * Purpose: store the writable material-buffer read failure diagnostic.
 */
char g_zModel_ReadMaterialBufferDataErrorMsg[0x2a] =
    "Error reading GameZ Material buffer data.";
/**
 * Reimplements data 0x4e1248: g_zModel_SetMaterialArraySizeLimitFmt.
 * Purpose: store the writable material-pool size limit diagnostic format.
 */
char g_zModel_SetMaterialArraySizeLimitFmt[0x39] =
    "Error setting material array size to %d; limit is 32767.";
/**
 * Reimplements data 0x4e1284: g_zModel_SetMaterialArraySizeAlreadySetFmt.
 * Purpose: store the writable material-pool already-sized diagnostic format.
 */
char g_zModel_SetMaterialArraySizeAlreadySetFmt[0x3b] =
    "Error setting material array size; size already set to %d.";
/**
 * Reimplements data 0x4e12c0: g_zModel_Matl_ErrCycleNullStr.
 * Purpose: store the writable null material-cycle diagnostic format.
 */
char g_zModel_Matl_ErrCycleNullStr[0x2d] =
    "Material Cycle Pointer is NULL: flag is (%s)";
/**
 * Reimplements data 0x4e12f0: g_zModel_SetCycleTextureLoopTextureNotCycledMsg.
 * Purpose: store the writable SetCycleTextureLoop not-cycled diagnostic.
 */
char g_zModel_SetCycleTextureLoopTextureNotCycledMsg[0x29] =
    "SetCycleTextureLoop:  Texture not cycled";
/**
 * Reimplements data 0x4e131c: g_zModel_SetCycleTextureSpeedTextureNotCycledMsg.
 * Purpose: store the writable SetCycleTextureSpeed not-cycled diagnostic.
 */
char g_zModel_SetCycleTextureSpeedTextureNotCycledMsg[0x2a] =
    "SetCycleTextureSpeed:  Texture not cycled";
/**
 * Reimplements data 0x4e1348: g_zModel_CopyMaterialBufferFullUsingDefaultMsg.
 * Purpose: store the writable material clone fallback diagnostic.
 */
char g_zModel_CopyMaterialBufferFullUsingDefaultMsg[0x3e] =
    "ERROR: Copying material; material buffer full; using default.";
RECOIL_STATIC_ASSERT(sizeof(g_zModel_GModMatl_FILE) == 0x27);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_Matl_ErrWriteBuffer) == 0x1f);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadMaterialCycleTextureDataErrorMsg) == 0x31);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadMaterialBufferDataErrorMsg) == 0x2a);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SetMaterialArraySizeLimitFmt) == 0x39);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SetMaterialArraySizeAlreadySetFmt) == 0x3b);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_Matl_ErrCycleNullStr) == 0x2d);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SetCycleTextureLoopTextureNotCycledMsg) == 0x29);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SetCycleTextureSpeedTextureNotCycledMsg) == 0x2a);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_CopyMaterialBufferFullUsingDefaultMsg) == 0x3e);

namespace zModel_MatlSlot {
/**
     * Reimplements 0x4805b0: zModel_MatlSlot::IndexFromPtrOrMinus1
     * (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp).
     *
     * Purpose: convert a material-slot pointer into its pool index, or return
     * -1 for a null slot pointer.
     */
    int __fastcall IndexFromPtrOrMinus1(zModel_MaterialSlot * slot) {
        if (slot == 0) {
            return -1;
        }

        return (int)(slot - g_zModel_MatlPool);
    }
} // namespace zModel_MatlSlot

namespace zModel_Matl {
/**
 * Reimplements 0x4805e0: zModel_Matl::GetPoolEntry
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: return the material-slot pool entry for a non-negative index.
 */
zModel_MaterialSlot *__fastcall GetPoolEntry(
    int index
) {
    if (index < 0) {
        return 0;
    }

    return &g_zModel_MatlPool[index];
}
} // namespace zModel_Matl

namespace zModel_MatlBuffer {
/**
     * Reimplements 0x480600: zModel_MatlBuffer::WriteGameZ
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     *
     * Purpose: serialize the material-pool header, active material slots, and
     * cycle-frame data while converting live texture pointers to TexDir indices.
     */
    int __fastcall WriteGameZ(void *stream) {
        zImage_TexDirEntryPartial **frameBuffer = 0;
        FILE *const file = (FILE *)(stream);

        if (fwrite(
            &g_zModel_MatlPoolCapacity,
            4,
            1,
            file
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zModel_GModMatl_FILE,
                0x1e9,
                g_zModel_Matl_ErrWriteBuffer
            );
        }
        if (fwrite(
            &g_zModel_MatlPoolInUseCount,
            4,
            1,
            file
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zModel_GModMatl_FILE,
                0x1f6,
                g_zModel_Matl_ErrWriteBuffer
            );
        }
        if (fwrite(
            &g_zModel_MatlFreeHeadIndex,
            4,
            1,
            file
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zModel_GModMatl_FILE,
                0x203,
                g_zModel_Matl_ErrWriteBuffer
            );
        }
        if (fwrite(
            &g_zModel_MatlActiveHeadIndex,
            4,
            1,
            file
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zModel_GModMatl_FILE,
                0x210,
                g_zModel_Matl_ErrWriteBuffer
            );
        }

        int result = g_zModel_MatlPoolCapacity;
        const int poolBytes = g_zModel_MatlPoolCapacity * (int)(sizeof(zModel_MaterialSlot));
        zModel_MaterialSlot *poolCopy = (zModel_MaterialSlot *)(malloc(poolBytes));
        memcpy(
            poolCopy,
            g_zModel_MatlPool,
            poolBytes
        );

        {
            for (int activeIndex = g_zModel_MatlActiveHeadIndex; activeIndex >= 0;) {
                zModel_MaterialSlot *const slot = &poolCopy[activeIndex];
                if ((slot->material.flags & 0x0100) != 0) {
                    slot->material.currentTextureDirectoryEntry =
                        (zImage_TexDirEntryPartial *)((int)(zImage::TexDirEntryToIndex(
                            slot->material.currentTextureDirectoryEntry
                        )));
                }
                activeIndex = slot->nextPoolIndex;
            }
        }

        if (fwrite(
            poolCopy,
            poolBytes,
            1,
            file
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zModel_GModMatl_FILE,
                0x234,
                g_zModel_Matl_ErrWriteBuffer
            );
            result = 0;
        }

        {
            for (int activeIndex = g_zModel_MatlActiveHeadIndex; activeIndex >= 0;) {
                zModel_MaterialSlot *const slot = &poolCopy[activeIndex];
                if ((slot->material.flags & 0x0400) != 0) {
                    zModel_MaterialCyclePartial *const cycle = slot->material.cycle;
                    if (fwrite(
                        cycle,
                        0x1c,
                        1,
                        file
                    ) != 1) {
                        zError::ReportOld(
                            0x200,
                            g_zModel_GModMatl_FILE,
                            0x249,
                            g_zModel_Matl_ErrWriteBuffer
                        );
                        result = 0;
                        break;
                    }

                    const unsigned int frameBytes =
                        (unsigned int)(cycle->frameCount) * sizeof(zImage_TexDirEntryPartial *);
                    frameBuffer = (zImage_TexDirEntryPartial **)(realloc(
                        frameBuffer,
                        frameBytes
                    ));
                    memcpy(
                        frameBuffer,
                        cycle->frameTable,
                        frameBytes
                    );
                    for (int i = 0; i < cycle->frameCount; ++i) {
                        frameBuffer[i] =
                            (zImage_TexDirEntryPartial *)((int)(zImage::TexDirEntryToIndex(
                                frameBuffer[i]
                            )));
                    }

                    if (fwrite(
                        frameBuffer,
                        frameBytes,
                        1,
                        file
                    ) != 1) {
                        zError::ReportOld(
                            0x200,
                            g_zModel_GModMatl_FILE,
                            0x266,
                            g_zModel_Matl_ErrWriteBuffer
                        );
                        result = 0;
                        break;
                    }
                }
                activeIndex = slot->nextPoolIndex;
            }
        }

        if (frameBuffer != 0) {
            free(frameBuffer);
        }
        free(poolCopy);
        return result;
    }


/**
     * Reimplements 0x4808c0: zModel_MatlBuffer::ReadGameZ
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     *
     * Purpose: read a serialized material pool, resize backing storage, restore
     * texture pointers from TexDir indices, and rebuild cycle frame tables.
     */
    int __fastcall ReadGameZ(void *stream) {
        FILE *const file = (FILE *)(stream);
        const int oldCapacity = g_zModel_MatlPoolCapacity;
        const char *errorMessage = g_zModel_ReadMaterialBufferDataErrorMsg;
        int errorLine = 0;
        int poolBytes = 0;

        if (fread(
            &g_zModel_MatlPoolCapacity,
            4,
            1,
            file
        ) != 1) {
            errorLine = 0x29b;
            goto readError;
        }
        if (fread(
            &g_zModel_MatlPoolInUseCount,
            4,
            1,
            file
        ) != 1) {
            errorLine = 0x2a8;
            goto readError;
        }
        if (fread(
            &g_zModel_MatlFreeHeadIndex,
            4,
            1,
            file
        ) != 1) {
            errorLine = 0x2b5;
            goto readError;
        }
        if (fread(
            &g_zModel_MatlActiveHeadIndex,
            4,
            1,
            file
        ) != 1) {
            errorLine = 0x2c2;
            goto readError;
        }

        if (g_zModel_MatlPoolCapacity == 0) {
            return 0;
        }

        poolBytes = g_zModel_MatlPoolCapacity * (int)(sizeof(zModel_MaterialSlot));
        if (g_zModel_MatlPool == 0) {
            g_zModel_MatlPool = (zModel_MaterialSlot *)(malloc(poolBytes));
        } else if (g_zModel_MatlPoolCapacity > oldCapacity) {
            g_zModel_MatlPool = (zModel_MaterialSlot *)(realloc(
                g_zModel_MatlPool,
                poolBytes
            ));
        }

        if (fread(
            g_zModel_MatlPool,
            poolBytes,
            1,
            file
        ) != 1) {
            errorLine = 0x2dd;
            goto readError;
        }

        {
            for (int activeIndex = g_zModel_MatlActiveHeadIndex; activeIndex >= 0;) {
                zModel_MaterialSlot *const slot = &g_zModel_MatlPool[activeIndex];
                zModel_MaterialPartial *const material = &slot->material;

                if ((material->flags & 0x0100) != 0) {
                    material->currentTextureDirectoryEntry = zImage::TexIndexToDirEntry(
                        (int)((int)(material->currentTextureDirectoryEntry))
                    );
                } else {
                    material->packedColor =
                        zVid_PackColorRgbFloats((zVideo_ColorRgbFloat *)(&material->colorRgb));
                }

                if ((material->flags & 0x0400) != 0) {
                    material->cycle = (zModel_MaterialCyclePartial *)(malloc(
                        sizeof(zModel_MaterialCyclePartial)
                    ));
                    if (fread(
                        material->cycle,
                        sizeof(zModel_MaterialCyclePartial),
                        1,
                        file
                    ) != 1) {
                        errorMessage = g_zModel_ReadMaterialCycleTextureDataErrorMsg;
                        errorLine = 0x2fa;
                        goto readError;
                    }

                    const unsigned int frameTableBytes =
                        (unsigned int)(material->cycle->frameCount) *
                        sizeof(zImage_TexDirEntryPartial *);
                    material->cycle->frameTable =
                        (zImage_TexDirEntryPartial **)(malloc(frameTableBytes));
                    if (fread(
                        material->cycle->frameTable,
                        frameTableBytes,
                        1,
                        file
                    ) != 1) {
                        errorMessage = g_zModel_ReadMaterialCycleTextureDataErrorMsg;
                        errorLine = 0x30b;
                        goto readError;
                    }

                    for (int i = 0; i < material->cycle->frameCount; ++i) {
                        material->cycle->frameTable[i] = zImage::TexIndexToDirEntry(
                            (int)((int)(material->cycle->frameTable[i]))
                        );
                    }
                }

                activeIndex = slot->nextPoolIndex;
            }
        }

        return g_zModel_MatlPoolCapacity;

    readError:
        zError::ReportOld(
            0x200,
            g_zModel_GModMatl_FILE,
            errorLine,
            errorMessage
        );
        return -1;
    }
} // namespace zModel_MatlBuffer

namespace zModel_Matl {
/**
 * Reimplements 0x480ae0: zModel_Matl::InitGlobals
 * (GameZRecoil/zModel/zModel_Matl.cpp).
 * Purpose: allocate and initialize the global material-slot pool and default material.
 */
int InitGlobals() {
    if (g_zModel_MatlPoolCapacity == 0) {
        g_zModel_MatlPoolCapacity = 2500;
    }

    const size_t poolBytes = (size_t)(g_zModel_MatlPoolCapacity) * sizeof(zModel_MaterialSlot);
    g_zModel_MatlPool = (zModel_MaterialSlot *)(malloc(poolBytes));
    memset(
        g_zModel_MatlPool,
        0,
        poolBytes
    );

    g_zModel_MatlFreeHeadIndex = 0;
    if (g_zModel_MatlPoolCapacity > 0) {
        for (int i = 0; i < g_zModel_MatlPoolCapacity; ++i) {
            g_zModel_MatlPool[i].prevPoolIndex = (short)(i == 0 ? -1 : i - 1);
            g_zModel_MatlPool[i].nextPoolIndex =
                (short)(i == g_zModel_MatlPoolCapacity - 1 ? -1 : i + 1);
        }
    }

    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;
    zModel_Material::ResetDefaults(&g_zModel_DefaultMaterial);
    return 0;
}

} // namespace zModel_Matl

namespace zModel_MatlBuffer {
/**
     * Reimplements 0x480bf0: zModel_MatlBuffer::SetArraySize
     * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
     *
     * Purpose: set the material-pool capacity before allocation, enforcing the
     * original 32767-entry serialized index limit.
     */
    void __fastcall SetArraySize(int count) {
        if (g_zModel_MatlPoolCapacity != 0) {
            zError::ReportOld(
                0x200,
                g_zModel_GModMatl_FILE,
                0x368,
                g_zModel_SetMaterialArraySizeAlreadySetFmt,
                g_zModel_MatlPoolCapacity
            );
            return;
        }

        if (count > 32767) {
            zError::ReportOld(
                0x200,
                g_zModel_GModMatl_FILE,
                0x371,
                g_zModel_SetMaterialArraySizeLimitFmt,
                count
            );
            return;
        }

        g_zModel_MatlPoolCapacity = count;
    }

} // namespace zModel_MatlBuffer

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

} // namespace zModel_Material

namespace zModel_MatlBuffer {
enum {
    kMaterialHasTextureUploadSurface = 0x0100,
    kMaterialTextureSurfacePinned = 0x0200,
    kRendererBackend3dfx = 2,
};

/**
 * Reimplements 0x480d80: zModel_MatlBuffer::ReleaseAllActive
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: release every active material slot and clear the material reuse cache.
 */
int ReleaseAllActive() {
    while (g_zModel_MatlActiveHeadIndex >= 0) {
        if (g_zModel_MatlPool == 0 || g_zModel_MatlActiveHeadIndex >= g_zModel_MatlPoolCapacity) {
            g_zModel_MatlActiveHeadIndex = -1;
            break;
        }
        zModel_MatlSlot::Release(&g_zModel_MatlPool[g_zModel_MatlActiveHeadIndex]);
    }

    g_zModel_MatlReuseCache = 0;
    return 0;
}

} // namespace zModel_MatlBuffer

namespace zModel_MatlSlot {
/**
 * Reimplements 0x480dc0: zModel_MatlSlot::Release
 * (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp).
 * Purpose: release a material slot, free cycle data, and return the slot to the material free list.
 */
void __fastcall Release(
    zModel_MaterialSlot *slot
) {
    if (slot == 0) {
        return;
    }

    if ((slot->material.flags & 0x0400) != 0 && slot->material.cycle != 0) {
        if (slot->material.cycle->frameTable != 0) {
            free(slot->material.cycle->frameTable);
            slot->material.cycle->frameTable = 0;
        }
        free(slot->material.cycle);
        slot->material.cycle = 0;
    }

    memset(
        &slot->material,
        0,
        sizeof(slot->material)
    );

    const int slotIndex = zModel_MatlSlot::IndexFromPtrOrMinus1(slot);
    const short prevIndex = slot->prevPoolIndex;
    const short nextIndex = slot->nextPoolIndex;

    if (prevIndex >= 0) {
        g_zModel_MatlPool[prevIndex].nextPoolIndex = nextIndex;
    }
    if (nextIndex >= 0) {
        g_zModel_MatlPool[nextIndex].prevPoolIndex = prevIndex;
    }
    if (g_zModel_MatlActiveHeadIndex == slotIndex) {
        g_zModel_MatlActiveHeadIndex = nextIndex;
    }

    slot->prevPoolIndex = -1;
    slot->nextPoolIndex = (short)(g_zModel_MatlFreeHeadIndex);
    if (g_zModel_MatlFreeHeadIndex >= 0) {
        g_zModel_MatlPool[g_zModel_MatlFreeHeadIndex].prevPoolIndex = (short)(slotIndex);
    }

    g_zModel_MatlFreeHeadIndex = slotIndex;
    --g_zModel_MatlPoolInUseCount;
}
} // namespace zModel_MatlSlot

namespace zRndr {
/**
 * Reimplements 0x480ec0: zRndr::GlobalStringTable_ReleaseDynamicEntries
 * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
 * Purpose: release dynamically loaded renderer global-string entries and restore the fixed prefix count.
 */
void GlobalStringTable_ReleaseDynamicEntries() {
    for (int i = 6; i < g_zRndr_GlobalStringCount; ++i) {
        free(g_zRndr_GlobalStringTable[i]);
        g_zRndr_GlobalStringTable[i] = 0;
    }

    g_zRndr_GlobalStringCount = 6;
}
} // namespace zRndr

namespace zModel_MatlBuffer {
/**
 * Reimplements 0x480f10: zModel_MatlBuffer::Shutdown
 * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
 * Purpose: shut down the material pool and release dynamic renderer string entries.
 */
int Shutdown() {
    ReleaseAllActive();
    if (g_zModel_MatlPool != 0) {
        free(g_zModel_MatlPool);
        g_zModel_MatlPool = 0;
    }

    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    zRndr::GlobalStringTable_ReleaseDynamicEntries();
    g_zModel_MatlReuseCache = 0;
    return 0;
}
} // namespace zModel_MatlBuffer

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
} // namespace zModel_Material

namespace zModel_MatlBuffer {
/**
 * Reimplements 0x480fd0: zModel_MatlBuffer::ReleaseTextureSurfaces
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: release upload-surface references for active, unpinned texture materials.
 */
void ReleaseTextureSurfaces() {
    int slotIndex = g_zModel_MatlActiveHeadIndex;
    while (slotIndex >= 0) {
        zModel_MaterialSlot *const slot = &g_zModel_MatlPool[slotIndex];
        zModel_MaterialPartial *const material = &slot->material;

        if ((material->flags & kMaterialHasTextureUploadSurface) != 0 &&
            (material->flags & kMaterialTextureSurfacePinned) == 0) {
            zImage_TexDirEntryPartial *const texDirEntry = material->currentTextureDirectoryEntry;

            if (texDirEntry != 0 && texDirEntry->texture != 0) {
                if (g_zVideo_ActiveRendererPath == kRendererBackend3dfx) {
                    zVid_Image::ReleaseOwnedBuffers(texDirEntry->image);
                }

                ((
                    zVideo_TextureRecordReleaseUploadSurfaceRefProc
                )g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef)(texDirEntry->texture);
            }
        }

        slotIndex = slot->nextPoolIndex;
    }
}

} // namespace zModel_MatlBuffer

namespace zModel_Material {
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
                g_zModel_GModMatl_FILE,
                0x5ca,
                g_zModel_Matl_ErrCycleNullStr,
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
                g_zModel_GModMatl_FILE,
                0x5fb,
                g_zModel_SetCycleTextureLoopTextureNotCycledMsg
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
                g_zModel_GModMatl_FILE,
                0x60f,
                g_zModel_SetCycleTextureSpeedTextureNotCycledMsg
            );
        }

        return 0;
    }

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
} // namespace zModel_Material

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
                g_zModel_GModMatl_FILE,
                0x626,
                g_zModel_CopyMaterialBufferFullUsingDefaultMsg
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
} // namespace zModel_MatlBuffer

namespace zModel_Material {
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

} // namespace zModel_Material

namespace zRndr_GlobalStringTable {
/**
 * Reimplements 0x481460: zRndr_GlobalStringTable::LoadDynamicEntriesFromPath
 * (D:\Proj\GameZRecoil\zRndr\zRndr_GlobalStringTable.cpp).
 * Purpose: append non-prefix dynamic global-string entries loaded from a zReader node tree.
 */
void __fastcall LoadDynamicEntriesFromPath(
    char *path
) {
    if (path == 0) {
        return;
    }

    zReader::Node *const root = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (root == 0) {
        return;
    }

    zReader::Node *const stringList = root->value.nodes[1].value.nodes;
    if (stringList == 0) {
        return;
    }

    const int stringCount = stringList[0].value.i32;
    for (int index = 1; index < stringCount; ++index) {
        char *const entry = stringList[index].value.str;
        if (zReader::FindGlobalStringPrefixIndex(entry) != -1) {
            continue;
        }

        if (g_zRndr_GlobalStringCount >= 100) {
            break;
        }

        const size_t byteCount = strlen(entry) + 1;
        char *const copy = (char *)(malloc(byteCount));
        g_zRndr_GlobalStringTable[g_zRndr_GlobalStringCount] = copy;
        if (copy != 0) {
            ++g_zRndr_GlobalStringCount;
            memcpy(
                copy,
                entry,
                byteCount
            );
        }
    }

    zReader::FreeLoadedTree(root);
}
} // namespace zRndr_GlobalStringTable
