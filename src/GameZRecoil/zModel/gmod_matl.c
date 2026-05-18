#include "GameZRecoil/zModel/zModel.h"

#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
    const char *kGmodMatlSourceFile = "D:\\Proj\\GameZRecoil\\zModel\\gmod_matl.c";

    const char *kMatlWriteError = "Error writing material buffer.";

    const char *kMatlReadError = "Error reading GameZ Material buffer data.";

    const char *kMatlCycleReadError = "Error reading GameZ Material cycle texture data.";
}

namespace zDi {
    // Reimplements 0x476340: zDi::SetVariantTagIfUnset
    RECOIL_NOINLINE void RECOIL_FASTCALL SetVariantTagIfUnset(zDiPartial * self,
                                                              int variantTag) {
        if (self == 0 || self->entryCount <= 0) {
            return;
        }

        for (int i = 0; i < self->entryCount; ++i) {
            zDiEntryPartial *entry = &self->entries[i];
            if (entry->variantTagInitialized == 0) {
                entry->variantTag = static_cast<unsigned char>(variantTag);
                entry->variantTagInitialized = 1;
            }
        }
    }
}

namespace zModel_MatlSlot {
    // Reimplements 0x4805b0: zModel_MatlSlot::IndexFromPtrOrMinus1
    // (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL IndexFromPtrOrMinus1(zModel_MaterialSlot * slot) {
        if (slot == 0) {
            return -1;
        }

        return static_cast<int>(slot - g_zModel_MatlPool);
    }
}

namespace zModel_MatlBuffer {
    // Reimplements 0x480600: zModel_MatlBuffer::WriteGameZ
    // (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL WriteGameZ(void *stream) {
        zImage_TexDirEntryPartial **frameBuffer = 0;
        FILE *const file = static_cast<FILE *>(stream);

        if (fwrite(&g_zModel_MatlPoolCapacity, 4, 1, file) != 1) {
            zError::ReportOld(0x200, kGmodMatlSourceFile, 0x1e9, kMatlWriteError);
        }
        if (fwrite(&g_zModel_MatlPoolInUseCount, 4, 1, file) != 1) {
            zError::ReportOld(0x200, kGmodMatlSourceFile, 0x1f6, kMatlWriteError);
        }
        if (fwrite(&g_zModel_MatlFreeHeadIndex, 4, 1, file) != 1) {
            zError::ReportOld(0x200, kGmodMatlSourceFile, 0x203, kMatlWriteError);
        }
        if (fwrite(&g_zModel_MatlActiveHeadIndex, 4, 1, file) != 1) {
            zError::ReportOld(0x200, kGmodMatlSourceFile, 0x210, kMatlWriteError);
        }

        int result = g_zModel_MatlPoolCapacity;
        const int poolBytes =
            g_zModel_MatlPoolCapacity * static_cast<int>(sizeof(zModel_MaterialSlot));
        zModel_MaterialSlot *poolCopy = static_cast<zModel_MaterialSlot *>(malloc(poolBytes));
        memcpy(poolCopy, g_zModel_MatlPool, poolBytes);

        {
        for (int activeIndex = g_zModel_MatlActiveHeadIndex; activeIndex >= 0;) {
            zModel_MaterialSlot *const slot = &poolCopy[activeIndex];
            if ((slot->material.flags & 0x0100) != 0) {
                slot->material.currentTextureDirectoryEntry =
                    reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<int>(
                        zImage::TexDirEntryToIndex(slot->material.currentTextureDirectoryEntry)));
            }
            activeIndex = slot->nextPoolIndex;
        }
        }

        if (fwrite(poolCopy, poolBytes, 1, file) != 1) {
            zError::ReportOld(0x200, kGmodMatlSourceFile, 0x234, kMatlWriteError);
            result = 0;
        }

        {
        for (int activeIndex = g_zModel_MatlActiveHeadIndex; activeIndex >= 0;) {
            zModel_MaterialSlot *const slot = &poolCopy[activeIndex];
            if ((slot->material.flags & 0x0400) != 0) {
                zModel_MaterialCyclePartial *const cycle = slot->material.cycle;
                if (fwrite(cycle, 0x1c, 1, file) != 1) {
                    zError::ReportOld(0x200, kGmodMatlSourceFile, 0x249, kMatlWriteError);
                    result = 0;
                    break;
                }

                const unsigned int frameBytes = static_cast<unsigned int>(cycle->frameCount) *
                                                 sizeof(zImage_TexDirEntryPartial *);
                frameBuffer = static_cast<zImage_TexDirEntryPartial **>(
                    realloc(frameBuffer, frameBytes));
                memcpy(frameBuffer, cycle->frameTable, frameBytes);
                for (int i = 0; i < cycle->frameCount; ++i) {
                    frameBuffer[i] = reinterpret_cast<zImage_TexDirEntryPartial *>(
                        static_cast<int>(zImage::TexDirEntryToIndex(frameBuffer[i])));
                }

                if (fwrite(frameBuffer, frameBytes, 1, file) != 1) {
                    zError::ReportOld(0x200, kGmodMatlSourceFile, 0x266, kMatlWriteError);
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

    // Reimplements 0x4808c0: zModel_MatlBuffer::ReadGameZ
    // (D:\Proj\GameZRecoil\zModel\gmod_matl.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL ReadGameZ(void *stream) {
        FILE *const file = static_cast<FILE *>(stream);
        const int oldCapacity = g_zModel_MatlPoolCapacity;
        const char *errorMessage = kMatlReadError;
        int errorLine = 0;
        int poolBytes = 0;

        if (fread(&g_zModel_MatlPoolCapacity, 4, 1, file) != 1) {
            errorLine = 0x29b;
            goto readError;
        }
        if (fread(&g_zModel_MatlPoolInUseCount, 4, 1, file) != 1) {
            errorLine = 0x2a8;
            goto readError;
        }
        if (fread(&g_zModel_MatlFreeHeadIndex, 4, 1, file) != 1) {
            errorLine = 0x2b5;
            goto readError;
        }
        if (fread(&g_zModel_MatlActiveHeadIndex, 4, 1, file) != 1) {
            errorLine = 0x2c2;
            goto readError;
        }

        if (g_zModel_MatlPoolCapacity == 0) {
            return 0;
        }

        poolBytes =
            g_zModel_MatlPoolCapacity * static_cast<int>(sizeof(zModel_MaterialSlot));
        if (g_zModel_MatlPool == 0) {
            g_zModel_MatlPool = static_cast<zModel_MaterialSlot *>(malloc(poolBytes));
        } else if (g_zModel_MatlPoolCapacity > oldCapacity) {
            g_zModel_MatlPool =
                static_cast<zModel_MaterialSlot *>(realloc(g_zModel_MatlPool, poolBytes));
        }

        if (fread(g_zModel_MatlPool, poolBytes, 1, file) != 1) {
            errorLine = 0x2dd;
            goto readError;
        }

        {
        for (int activeIndex = g_zModel_MatlActiveHeadIndex; activeIndex >= 0;) {
            zModel_MaterialSlot *const slot = &g_zModel_MatlPool[activeIndex];
            zModel_MaterialPartial *const material = &slot->material;

            if ((material->flags & 0x0100) != 0) {
                material->currentTextureDirectoryEntry =
                    zImage::TexIndexToDirEntry(static_cast<int>(
                        reinterpret_cast<int>(material->currentTextureDirectoryEntry)));
            } else {
                material->packedColor = zVid_PackColorRgbFloats(
                    reinterpret_cast<zVideo_ColorRgbFloat *>(&material->colorRgb));
            }

            if ((material->flags & 0x0400) != 0) {
                material->cycle = static_cast<zModel_MaterialCyclePartial *>(
                    malloc(sizeof(zModel_MaterialCyclePartial)));
                if (fread(material->cycle, sizeof(zModel_MaterialCyclePartial), 1, file) !=
                    1) {
                    errorMessage = kMatlCycleReadError;
                    errorLine = 0x2fa;
                    goto readError;
                }

                const unsigned int frameTableBytes =
                    static_cast<unsigned int>(material->cycle->frameCount) *
                    sizeof(zImage_TexDirEntryPartial *);
                material->cycle->frameTable =
                    static_cast<zImage_TexDirEntryPartial **>(malloc(frameTableBytes));
                if (fread(material->cycle->frameTable, frameTableBytes, 1, file) != 1) {
                    errorMessage = kMatlCycleReadError;
                    errorLine = 0x30b;
                    goto readError;
                }

                for (int i = 0; i < material->cycle->frameCount; ++i) {
                    material->cycle->frameTable[i] =
                        zImage::TexIndexToDirEntry(static_cast<int>(
                            reinterpret_cast<int>(material->cycle->frameTable[i])));
                }
            }

            activeIndex = slot->nextPoolIndex;
        }
        }

        return g_zModel_MatlPoolCapacity;

    readError:
        zError::ReportOld(0x200, kGmodMatlSourceFile, errorLine, errorMessage);
        return -1;
    }
}
