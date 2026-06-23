#include "GameZRecoil/zModel/zModel.h"

#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

namespace zDi {
    /**
     * Reimplements 0x476340: zDi::SetVariantTagIfUnset
     * (D:\Proj\GameZRecoil\zModel\zModel_Di.cpp).
     *
     * Purpose: assign the variant tag to each display-instance entry that has
     * not already initialized its variant-tag state.
     */
    void __fastcall SetVariantTagIfUnset(
        zDiPartial * self,
        int variantTag
    ) {
        if (self == 0 || self->entryCount <= 0) {
            return;
        }

        for (int i = 0; i < self->entryCount; ++i) {
            zDiEntryPartial *entry = &self->entries[i];
            if (entry->variantTagInitialized == 0) {
                entry->variantTag = (unsigned char)(variantTag);
                entry->variantTagInitialized = 1;
            }
        }
    }
}

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
}

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
}
