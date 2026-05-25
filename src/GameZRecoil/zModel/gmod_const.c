#include "GameZRecoil/zModel/zModel.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
    const char *kGmodConstSourceFile = "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c";

    const char *kModel3DBufferWriteError = "Error writing model3d buffer.";

    const char *kModel3DBufferReadError = "Error reading GameZ Model3D buffer data.";

    void ReportModel3DBufferReadError(int line, const char *message) {
        zError::ReportOld(0x200, kGmodConstSourceFile, line, message);
    }

    void ReportModel3DBufferWriteError(int line) {
        zError::ReportOld(0x200, kGmodConstSourceFile, line, kModel3DBufferWriteError);
    }
}

float g_zModel_ConstVertexMergeEpsilon = 0.001f;
int g_zModel_MaxPolygonVertexCountBeforeSplit = 48;
double g_zModel_ConstVertexWarnThreshold = 920.0;
double g_zModel_NormalMergeEpsilon = 0.0001;
double g_zModel_CoplanarTolerance = 0.001;
double g_zModel_ColinearTolerance = 0.001;
float g_zModel_UvQuantizeBias = -0.001953125f;
float g_zModel_UvQuantizeScale = 256.0f;
float g_zModel_UvQuantizeInvScale = 0.00390625f;

namespace zModel_Const {
    // Reimplements 0x481530: zModel_Const::GetVertexMergeEpsilon
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE float RECOIL_CDECL GetVertexMergeEpsilon() {
        return g_zModel_ConstVertexMergeEpsilon;
    }

    // Reimplements 0x481540: zModel_Const::SetVertexMergeEpsilon
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE void RECOIL_STDCALL SetVertexMergeEpsilon(float epsilon) {
        unsigned int bits;
        memcpy(&bits, &epsilon, sizeof(bits));
        memcpy(&g_zModel_ConstVertexMergeEpsilon, &bits, sizeof(bits));
    }

    // Reimplements 0x481550: zModel_Const::SetCoplanarTolerance
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE void RECOIL_STDCALL SetCoplanarTolerance(float tolerance) {
        g_zModel_CoplanarTolerance = tolerance;
    }

    // Reimplements 0x481560: zModel_Const::SetColinearTolerance
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE void RECOIL_STDCALL SetColinearTolerance(float tolerance) {
        g_zModel_ColinearTolerance = tolerance;
    }
}

namespace zModel_DiPool {
    // Reimplements 0x4815c0: zModel_DiPool::WriteToStream
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL WriteToStream(void *stream) {
        FILE *const file = static_cast<FILE *>(stream);

        if (fwrite(&g_zModel_DiPoolCapacity, 4, 1, file) != 1) {
            ReportModel3DBufferWriteError(0x141);
        }
        if (fwrite(&g_zModel_DiPoolInUseCount, 4, 1, file) != 1) {
            ReportModel3DBufferWriteError(0x14e);
        }
        if (fwrite(&g_zModel_DiPoolFreeHeadIndex, 4, 1, file) != 1) {
            ReportModel3DBufferWriteError(0x15b);
        }

        const int capacity = g_zModel_DiPoolCapacity;
        if (capacity == 0) {
            return 0;
        }

        int result = capacity;
        const long tableOffset = ftell(file);
        const int tableBytes = capacity * static_cast<int>(sizeof(zDiPartial));
        if (fwrite(g_zModel_DiPoolBase, tableBytes, 1, file) != 1) {
            ReportModel3DBufferWriteError(0x172);
            result = 0;
        }

        {
        for (int diIndex = 0; diIndex < result; ++diIndex) {
            const long dynamicOffset = ftell(file);
            zDiPartial *const di = &g_zModel_DiPoolBase[diIndex];
            bool wroteDynamicData = false;

            if (di->vertCount > 0) {
                wroteDynamicData = true;
                if (fwrite(di->verts, 0x0c, di->vertCount, file) !=
                    static_cast<size_t>(di->vertCount)) {
                    ReportModel3DBufferWriteError(0x18c);
                    result = 0;
                    break;
                }
            }

            if (di->normalCount > 0) {
                wroteDynamicData = true;
                if (fwrite(di->normals, 0x0c, di->normalCount, file) !=
                    static_cast<size_t>(di->normalCount)) {
                    ReportModel3DBufferWriteError(0x19f);
                    result = 0;
                    break;
                }
            }

            if (di->blendVertCount > 0) {
                wroteDynamicData = true;
                if (fwrite(di->blendVerts, 0x0c, di->blendVertCount, file) !=
                    static_cast<size_t>(di->blendVertCount)) {
                    ReportModel3DBufferWriteError(0x1b2);
                    result = 0;
                    break;
                }
            }

            if (di->pointCount > 0) {
                wroteDynamicData = true;
                if (fwrite(di->pointEntries, sizeof(zModel_PointEntryPartial), di->pointCount,
                                file) != static_cast<size_t>(di->pointCount)) {
                    ReportModel3DBufferWriteError(0x1c9);
                    result = 0;
                    break;
                }

                {
                for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
                    zModel_PointEntryPartial *const point = &di->pointEntries[pointIndex];
                    if (point->pointCamCount > 0 &&
                        fwrite(point->pointCamList, sizeof(zVec3), point->pointCamCount,
                                    file) != static_cast<size_t>(point->pointCamCount)) {
                        ReportModel3DBufferWriteError(0x1dd);
                        result = 0;
                        break;
                    }
                }
                }
            }

            if (di->entryCount > 0) {
                wroteDynamicData = true;
                const int entryBytes =
                    di->entryCount * static_cast<int>(sizeof(zDiEntryPartial));
                zDiEntryPartial *serializedEntries =
                    static_cast<zDiEntryPartial *>(malloc(entryBytes));
                memcpy(serializedEntries, di->entries, entryBytes);

                {
                for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
                    const int materialIndex = zModel_MatlSlot::IndexFromPtrOrMinus1(
                        (zModel_MaterialSlot *)(
                            serializedEntries[entryIndex].material));
                    serializedEntries[entryIndex].material =
                        (zModel_MaterialPartial *)(
                            static_cast<int>(materialIndex));
                }
                }

                if (fwrite(serializedEntries, entryBytes, 1, file) != 1) {
                    ReportModel3DBufferWriteError(0x209);
                    result = 0;
                    break;
                }

                bool entryWriteFailed = false;
                {
                for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
                    zDiEntryPartial *const entry = &serializedEntries[entryIndex];
                    const unsigned int indexCount = entry->flagsAndIndexCount & 0xff;

                    if (indexCount != 0 &&
                        fwrite(entry->vertexIndices, 4, indexCount, file) != indexCount) {
                        ReportModel3DBufferWriteError(0x21e);
                        result = 0;
                        entryWriteFailed = true;
                        break;
                    }

                    if ((entry->flagsAndIndexCount & 0x0200) != 0 &&
                        entry->normalIndices != 0 &&
                        fwrite(entry->normalIndices, 4, indexCount, file) != indexCount) {
                        ReportModel3DBufferWriteError(0x22e);
                        result = 0;
                        entryWriteFailed = true;
                        break;
                    }

                    const zDiEntryPartial *const liveEntry = &di->entries[entryIndex];
                    if ((liveEntry->material->flags & 0x0100) != 0 &&
                        fwrite(entry->uvPairs, 8, indexCount, file) != indexCount) {
                        ReportModel3DBufferWriteError(0x240);
                        result = 0;
                        entryWriteFailed = true;
                        break;
                    }
                }
                }

                free(serializedEntries);
                if (entryWriteFailed) {
                    di->nextFreeIndex = static_cast<int>(dynamicOffset);
                    break;
                }
            }

            if (wroteDynamicData) {
                di->nextFreeIndex = static_cast<int>(dynamicOffset);
            }
        }
        }

        const long endOffset = ftell(file);
        fseek(file, tableOffset, SEEK_SET);
        if (fwrite(g_zModel_DiPoolBase, tableBytes, 1, file) != 1) {
            ReportModel3DBufferWriteError(0x263);
            result = 0;
        }
        fseek(file, endOffset, SEEK_SET);
        return result;
    }

    // Reimplements 0x481bc0: zModel_DiPool::ReadHeaderFromStream
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL ReadHeaderFromStream(
        void *stream, int *outCapacity, int *outInUseCount,
        int *outFreeHeadIndex) {
        FILE *const file = static_cast<FILE *>(stream);

        if (fread(outCapacity, 4, 1, file) != 1) {
            ReportModel3DBufferReadError(0x28b, kModel3DBufferReadError);
            return -1;
        }
        if (fread(outInUseCount, 4, 1, file) != 1) {
            ReportModel3DBufferReadError(0x298, kModel3DBufferReadError);
            return -1;
        }
        if (fread(outFreeHeadIndex, 4, 1, file) != 1) {
            ReportModel3DBufferReadError(0x2a5, kModel3DBufferReadError);
            return -1;
        }

        return 0;
    }

    // Reimplements 0x481c50: zModel_DiPool::ReadEntryDynamicDataFromStream
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL ReadEntryDynamicDataFromStream(void *stream,
                                                                                zDiPartial *entry) {
        FILE *const file = static_cast<FILE *>(stream);

        if (entry->vertCount > 0) {
            const int byteCount =
                entry->vertCount * static_cast<int>(sizeof(zVec3));
            entry->verts = static_cast<zVec3 *>(malloc(byteCount));
            if (fread(entry->verts, byteCount, 1, file) != 1) {
                ReportModel3DBufferReadError(0x31c, "Error reading GameZ Model3D vertex data.");
                return -1;
            }
        }

        if (entry->normalCount > 0) {
            const int byteCount =
                entry->normalCount * static_cast<int>(sizeof(zVec3));
            entry->normals = static_cast<zVec3 *>(malloc(byteCount));
            if (fread(entry->normals, byteCount, 1, file) != 1) {
                ReportModel3DBufferReadError(0x32f,
                                             "Error reading GameZ Model3D vertex normal data.");
                return -1;
            }
        }

        if (entry->blendVertCount > 0) {
            const int byteCount =
                entry->blendVertCount * static_cast<int>(sizeof(zVec3));
            entry->blendVerts = static_cast<zVec3 *>(malloc(byteCount));
            if (fread(entry->blendVerts, byteCount, 1, file) != 1) {
                ReportModel3DBufferReadError(0x342,
                                             "Error reading GameZ Model3D morph vertex data.");
                return -1;
            }
        }

        if (entry->pointCount > 0) {
            const int byteCount =
                entry->pointCount * static_cast<int>(sizeof(zModel_PointEntryPartial));
            entry->pointEntries = static_cast<zModel_PointEntryPartial *>(malloc(byteCount));
            if (fread(entry->pointEntries, byteCount, 1, file) != 1) {
                ReportModel3DBufferReadError(0x358,
                                             "Error reading GameZ Model3D point light data.");
                return -1;
            }

            {
            for (int pointIndex = 0; pointIndex < entry->pointCount; ++pointIndex) {
                zModel_PointEntryPartial *const point = &entry->pointEntries[pointIndex];
                const unsigned short packedColor = static_cast<unsigned short>(zVid_PackColorRGB(
                    static_cast<unsigned char>(static_cast<int>(point->colorB + 0.5f)),
                    static_cast<unsigned char>(static_cast<int>(point->colorG + 0.5f)),
                    static_cast<unsigned char>(static_cast<int>(point->colorR + 0.5f))));
                point->packedColor16 = (point->packedColor16 & 0xffff0000) | packedColor;

                if (point->pointCamCount > 0) {
                    const int pointCamBytes =
                        point->pointCamCount * static_cast<int>(sizeof(zVec3));
                    point->pointCamList = static_cast<zVec3 *>(malloc(pointCamBytes));
                    if (fread(point->pointCamList, pointCamBytes, 1, file) != 1) {
                        ReportModel3DBufferReadError(
                            0x372, "Error reading GameZ Model3D point light data.");
                        return -1;
                    }
                }
            }
            }
        }

        if (entry->entryCount <= 0) {
            return 0;
        }

        const int entryBytes =
            entry->entryCount * static_cast<int>(sizeof(zDiEntryPartial));
        entry->entries = static_cast<zDiEntryPartial *>(malloc(entryBytes));
        if (fread(entry->entries, entryBytes, 1, file) != 1) {
            ReportModel3DBufferReadError(0x38f, "Error reading GameZ Model3D polygon buffer.");
            return -1;
        }

        {
        for (int entryIndex = 0; entryIndex < entry->entryCount; ++entryIndex) {
            zDiEntryPartial *const diEntry = &entry->entries[entryIndex];
            diEntry->material =
                (zModel_MaterialPartial *)(zModel_Matl::GetPoolEntry(
                    static_cast<int>((int)(diEntry->material))));
        }
        }

        {
        for (int entryIndex = 0; entryIndex < entry->entryCount; ++entryIndex) {
            zDiEntryPartial *const diEntry = &entry->entries[entryIndex];
            const unsigned int indexCount = diEntry->flagsAndIndexCount & 0xff;
            if (indexCount != 0) {
                const unsigned int indexBytes = indexCount * 4;
                diEntry->vertexIndices = malloc(indexBytes);
                if (fread(diEntry->vertexIndices, indexBytes, 1, file) != 1) {
                    ReportModel3DBufferReadError(
                        0x3ae, "Error reading GameZ Model3D polygon vertex index.");
                    return -1;
                }
            }

            if ((diEntry->flagsAndIndexCount & 0x0200) != 0) {
                const unsigned int indexBytes = indexCount * 4;
                diEntry->normalIndices = malloc(indexBytes);
                if (fread(diEntry->normalIndices, indexBytes, 1, file) != 1) {
                    ReportModel3DBufferReadError(
                        0x3c0, "Error reading GameZ Model3D polygon vertex normal index.");
                    return -1;
                }
            }

            if ((diEntry->material->flags & 0x0100) != 0) {
                const unsigned int uvBytes =
                    indexCount * static_cast<unsigned int>(sizeof(zModel_Uv));
                diEntry->uvPairs = malloc(uvBytes);
                if (fread(diEntry->uvPairs, uvBytes, 1, file) != 1) {
                    ReportModel3DBufferReadError(
                        0x3d4, "Error reading GameZ Model3D polygon texture vertex data.");
                    return -1;
                }
            }
        }
        }

        return 0;
    }

    // Reimplements 0x481aa0: zModel_DiPool::ReadEntryByIndexFromStream
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE RECOIL_NO_GS zDiPartial *RECOIL_FASTCALL ReadEntryByIndexFromStream(
        void *stream, int index) {
        FILE *const file = static_cast<FILE *>(stream);

        int serializedCapacity;
        int serializedInUseCount;
        int serializedFreeHeadIndex;
        if (ReadHeaderFromStream(file, &serializedCapacity, &serializedInUseCount,
                                 &serializedFreeHeadIndex) != 0) {
            ReportModel3DBufferReadError(0x401, "Error reading GameZ Model3D buffer header data.");
            return 0;
        }

        if (serializedCapacity == 0) {
            return 0;
        }

        if (index >= serializedCapacity) {
            return 0;
        }

        fseek(file, index * static_cast<int>(sizeof(zDiPartial)), SEEK_CUR);

        zDiPartial serializedEntry;
        if (fread(&serializedEntry, sizeof(zDiPartial), 1, file) != 1) {
            ReportModel3DBufferReadError(0x41a, kModel3DBufferReadError);
            return 0;
        }

        zDiPartial *const entry = AllocFromFreeList();
        if (entry == 0) {
            return 0;
        }

        memcpy(entry, &serializedEntry, offsetof(zDiPartial, nextFreeIndex));
        fseek(file, serializedEntry.nextFreeIndex, SEEK_SET);
        if (ReadEntryDynamicDataFromStream(file, entry) != 0) {
            FreeIfUnreferenced(entry);
            return 0;
        }

        return entry;
    }

    // Reimplements 0x481fa0: zModel_DiPool::ReadFromStream
    // (D:\Proj\GameZRecoil\zModel\gmod_const.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL ReadFromStream(void *stream) {
        FILE *const file = static_cast<FILE *>(stream);
        const int oldCapacity = g_zModel_DiPoolCapacity;

        if (ReadHeaderFromStream(file, &g_zModel_DiPoolCapacity, &g_zModel_DiPoolInUseCount,
                                 &g_zModel_DiPoolFreeHeadIndex) != 0) {
            ReportModel3DBufferReadError(0x45b, "Error reading GameZ Model3D buffer header data.");
            return -1;
        }

        if (g_zModel_DiPoolCapacity == 0) {
            return 0;
        }

        const int poolBytes =
            g_zModel_DiPoolCapacity * static_cast<int>(sizeof(zDiPartial));
        if (g_zModel_DiPoolBase == 0) {
            g_zModel_DiPoolBase = static_cast<zDiPartial *>(malloc(poolBytes));
        } else if (g_zModel_DiPoolCapacity > oldCapacity) {
            g_zModel_DiPoolBase =
                static_cast<zDiPartial *>(realloc(g_zModel_DiPoolBase, poolBytes));
        }

        if (fread(g_zModel_DiPoolBase, poolBytes, 1, file) != 1) {
            ReportModel3DBufferReadError(0x476, kModel3DBufferReadError);
            return -1;
        }

        {
        for (int poolIndex = 0; poolIndex < g_zModel_DiPoolCapacity; ++poolIndex) {
            ReadEntryDynamicDataFromStream(file, &g_zModel_DiPoolBase[poolIndex]);
        }
        }

        return g_zModel_DiPoolCapacity;
    }
}
