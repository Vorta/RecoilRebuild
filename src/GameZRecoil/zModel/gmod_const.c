#include "GameZRecoil/zModel/zModel.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Reimplements data 0x4e13a0: g_zModel_SourceFile_GmodConstC.
 * Data owner: geometry_model_assets.zmodel_gmod_const_literals.
 * Purpose: store the writable gmod_const.c source-file path used by model
 * buffer diagnostics.
 *
 * Retail 0x4e13a0: initialized .data char[0x28] literal
 * "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c".
 */
char g_zModel_SourceFile_GmodConstC[0x28] =
    "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c";
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SourceFile_GmodConstC) == 0x28);

/*
 * BN identifies the gmod_const.c Model3D diagnostics as writable .data char
 * arrays in this order, including VC alignment padding between rows.
 */
/**
 * Reimplements data 0x4e13c8: g_zModel_WriteModel3dBufferErrorMsg.
 * Purpose: store the writable Model3D buffer write failure diagnostic.
 */
char g_zModel_WriteModel3dBufferErrorMsg[0x1e] =
    "Error writing model3d buffer.";
/**
 * Reimplements data 0x4e13e8: g_zModel_ReadModel3dBufferDataErrorMsg.
 * Purpose: store the writable Model3D buffer read failure diagnostic.
 */
char g_zModel_ReadModel3dBufferDataErrorMsg[0x29] =
    "Error reading GameZ Model3D buffer data.";
/**
 * Reimplements data 0x4e1414: g_zModel_ReadModel3dBufferHeaderErrorMsg.
 * Purpose: store the writable Model3D buffer header read failure diagnostic.
 */
char g_zModel_ReadModel3dBufferHeaderErrorMsg[0x30] =
    "Error reading GameZ Model3D buffer header data.";
/**
 * Reimplements data 0x4e1444: g_zModel_ReadModel3dPolyTexVertDataErrorMsg.
 * Purpose: store the writable Model3D polygon texture-vertex read diagnostic.
 */
char g_zModel_ReadModel3dPolyTexVertDataErrorMsg[0x39] =
    "Error reading GameZ Model3D polygon texture vertex data.";
/**
 * Reimplements data 0x4e1480: g_zModel_ReadModel3dPolyVertNormalIndexErrorMsg.
 * Purpose: store the writable Model3D polygon normal-index read diagnostic.
 */
char g_zModel_ReadModel3dPolyVertNormalIndexErrorMsg[0x39] =
    "Error reading GameZ Model3D polygon vertex normal index.";
/**
 * Reimplements data 0x4e14bc: g_zModel_ReadModel3dPolyVertIndexErrorMsg.
 * Purpose: store the writable Model3D polygon vertex-index read diagnostic.
 */
char g_zModel_ReadModel3dPolyVertIndexErrorMsg[0x32] =
    "Error reading GameZ Model3D polygon vertex index.";
/**
 * Reimplements data 0x4e14f0: g_zModel_ReadModel3dPolygonBufferErrorMsg.
 * Purpose: store the writable Model3D polygon-buffer read diagnostic.
 */
char g_zModel_ReadModel3dPolygonBufferErrorMsg[0x2c] =
    "Error reading GameZ Model3D polygon buffer.";
/**
 * Reimplements data 0x4e151c: g_zModel_ReadModel3dPointLightDataErrorMsg.
 * Purpose: store the writable Model3D point-light data read diagnostic.
 */
char g_zModel_ReadModel3dPointLightDataErrorMsg[0x2e] =
    "Error reading GameZ Model3D point light data.";
/**
 * Reimplements data 0x4e154c: g_zModel_ReadModel3dMorphVertexDataErrorMsg.
 * Purpose: store the writable Model3D morph-vertex read diagnostic.
 */
char g_zModel_ReadModel3dMorphVertexDataErrorMsg[0x2f] =
    "Error reading GameZ Model3D morph vertex data.";
/**
 * Reimplements data 0x4e157c: g_zModel_ReadModel3dVertexNormalDataErrorMsg.
 * Purpose: store the writable Model3D vertex-normal read diagnostic.
 */
char g_zModel_ReadModel3dVertexNormalDataErrorMsg[0x30] =
    "Error reading GameZ Model3D vertex normal data.";
/**
 * Reimplements data 0x4e15ac: g_zModel_ReadModel3dVertexDataErrorMsg.
 * Purpose: store the writable Model3D vertex read diagnostic.
 */
char g_zModel_ReadModel3dVertexDataErrorMsg[0x29] =
    "Error reading GameZ Model3D vertex data.";
/**
 * Reimplements data 0x4e15d8: g_zModel_CreateModel3dBufferFullErrorMsg.
 * Purpose: store the writable Model3D create-buffer-full diagnostic.
 */
char g_zModel_CreateModel3dBufferFullErrorMsg[0x2c] =
    "ERROR: Creating Model3D; model buffer full.";
/**
 * Reimplements data 0x4e1604: g_zModel_CreateModel3dApproachingLimitFmt.
 * Purpose: store the writable Model3D creation limit warning format.
 */
char g_zModel_CreateModel3dApproachingLimitFmt[0x28] =
    "         Approaching max allowable: %d\n";
/**
 * Reimplements data 0x4e162c: g_zModel_VertexCountWarningFmt.
 * Purpose: store the writable model vertex-count warning format.
 */
char g_zModel_VertexCountWarningFmt[0x2f] =
    "%s: Line %d: WARNING: Model vertex count = %d\n";
/**
 * Reimplements data 0x4e165c: g_zModel_NormalCountWarningFmt.
 * Purpose: store the writable model normal-count warning format.
 */
char g_zModel_NormalCountWarningFmt[0x2f] =
    "%s: Line %d: WARNING: Model normal count = %d\n";
/**
 * Reimplements data 0x4e168c: g_zModel_AddPolygonTooFewVertsFmt.
 * Purpose: store the writable AddPolygon too-few-vertices diagnostic format.
 */
char g_zModel_AddPolygonTooFewVertsFmt[0x2d] =
    "Attempting to add polygon with only %d verts";
/**
 * Reimplements data 0x4e16bc: g_zModel_AddNonPlanarPolygonTriangulatingFmt.
 * Purpose: store the writable non-planar AddPolygon triangulation diagnostic.
 */
char g_zModel_AddNonPlanarPolygonTriangulatingFmt[0x42] =
    "Attempting to add non-planar polygon (%d verts), triangulating...";
/**
 * Reimplements data 0x4e1700: g_zModel_DiscardPolygonAfterCheckColinearityFmt.
 * Purpose: store the writable AddPolygon colinearity discard diagnostic.
 */
char g_zModel_DiscardPolygonAfterCheckColinearityFmt[0x41] =
    "Discarding Polygon: (%d of %d) verts after 'check_colinearity()'";
/**
 * Reimplements data 0x4e1744: g_zModel_PolyVertexCountApproachingLimitFmt.
 * Purpose: store the writable polygon vertex-count limit warning format.
 */
char g_zModel_PolyVertexCountApproachingLimitFmt[0x2e] =
    "Poly vertex count approaching limit (%d / %d)";
/**
 * Reimplements data 0x4e1774: g_zModel_AddPolygonOnlyVertsErrorFmt.
 * Purpose: store the writable AddPolygon only-vertices error format.
 */
char g_zModel_AddPolygonOnlyVertsErrorFmt[0x3b] =
    "ERROR: You're trying to add a Polygon with only (%d) verts";
/**
 * Reimplements data 0x4e17b0: g_zModel_SetModelCycleTextureNullModelFmt.
 * Purpose: store the writable SetModelCycleTexture null-model diagnostic.
 */
char g_zModel_SetModelCycleTextureNullModelFmt[0x46] =
    "%s(%d): ERROR setting model cycle texture. Model 3D pointer is NULL.\n";
RECOIL_STATIC_ASSERT(sizeof(g_zModel_WriteModel3dBufferErrorMsg) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dBufferDataErrorMsg) == 0x29);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dBufferHeaderErrorMsg) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dPolyTexVertDataErrorMsg) == 0x39);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dPolyVertNormalIndexErrorMsg) == 0x39);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dPolyVertIndexErrorMsg) == 0x32);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dPolygonBufferErrorMsg) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dPointLightDataErrorMsg) == 0x2e);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dMorphVertexDataErrorMsg) == 0x2f);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dVertexNormalDataErrorMsg) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_ReadModel3dVertexDataErrorMsg) == 0x29);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_CreateModel3dBufferFullErrorMsg) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_CreateModel3dApproachingLimitFmt) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_VertexCountWarningFmt) == 0x2f);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_NormalCountWarningFmt) == 0x2f);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_AddPolygonTooFewVertsFmt) == 0x2d);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_AddNonPlanarPolygonTriangulatingFmt) == 0x42);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_DiscardPolygonAfterCheckColinearityFmt) == 0x41);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_PolyVertexCountApproachingLimitFmt) == 0x2e);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_AddPolygonOnlyVertsErrorFmt) == 0x3b);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SetModelCycleTextureNullModelFmt) == 0x46);

namespace {
    /**
     * Original static helper observed in zModel_DiPool read paths
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: report a model3d-buffer read failure with the original source-file line.
     */
    void ReportModel3DBufferReadError(
        int line,
        const char *message
    ) {
        zError::ReportOld(
            0x200,
            g_zModel_SourceFile_GmodConstC,
            line,
            message
        );
    }

    /**
     * Original static helper observed in zModel_DiPool write paths
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: report a model3d-buffer write failure with the original source-file line.
     */
    void ReportModel3DBufferWriteError(int line) {
        zError::ReportOld(
            0x200,
            g_zModel_SourceFile_GmodConstC,
            line,
            g_zModel_WriteModel3dBufferErrorMsg
        );
    }
}

/**
 * Reimplements data 0x4e1398: g_zModel_ConstVertexMergeEpsilon.
 * Purpose: Stores g zModel ConstVertexMergeEpsilon data used by engine.zmodel.vertex_merge_epsilon_global.
 */
float g_zModel_ConstVertexMergeEpsilon = 0.001f;
/**
 * Reimplements data 0x4e139c: g_zModel_MaxPolygonVertexCountBeforeSplit.
 * Purpose: store the AddPolygonEx vertex-count threshold before chunk splitting.
 */
int g_zModel_MaxPolygonVertexCountBeforeSplit = 48;
double g_zModel_ConstVertexWarnThreshold = 921.6;
double g_zModel_NormalMergeEpsilon = 0.0001;
double g_zModel_CoplanarTolerance = 0.001;
double g_zModel_ColinearTolerance = 0.001;
float g_zModel_UvQuantizeBias = -0.001953125f;
float g_zModel_UvQuantizeScale = 256.0f;
float g_zModel_UvQuantizeInvScale = 0.00390625f;

namespace zModel_Const {
    /**
     * Reimplements 0x481530: zModel_Const::GetVertexMergeEpsilon
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: return the global vertex-merge epsilon.
     */
    float GetVertexMergeEpsilon() {
        return g_zModel_ConstVertexMergeEpsilon;
    }

    /**
     * Reimplements 0x481540: zModel_Const::SetVertexMergeEpsilon
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: set the global vertex-merge epsilon using the original bit-preserving copy.
     */
    void __stdcall SetVertexMergeEpsilon(float epsilon) {
        unsigned int bits;
        memcpy(
            &bits,
            &epsilon,
            sizeof(bits)
        );
        memcpy(
            &g_zModel_ConstVertexMergeEpsilon,
            &bits,
            sizeof(bits)
        );
    }

    /**
     * Reimplements 0x481550: zModel_Const::SetCoplanarTolerance
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: set the global coplanar polygon tolerance.
     */
    void __stdcall SetCoplanarTolerance(float tolerance) {
        g_zModel_CoplanarTolerance = tolerance;
    }

    /**
     * Reimplements 0x481560: zModel_Const::SetColinearTolerance
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: set the global colinear polygon tolerance.
     */
    void __stdcall SetColinearTolerance(float tolerance) {
        g_zModel_ColinearTolerance = tolerance;
    }
}

namespace zModel_DiPool {
    /**
     * Reimplements 0x4815c0: zModel_DiPool::WriteToStream
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: serialize the display-instance pool and its dynamic arrays to a stream.
     */
    int __fastcall WriteToStream(void *stream) {
        FILE *const file = (FILE *)(stream);

        if (fwrite(
            &g_zModel_DiPoolCapacity,
            4,
            1,
            file
        ) != 1) {
            ReportModel3DBufferWriteError(0x141);
        }
        if (fwrite(
            &g_zModel_DiPoolInUseCount,
            4,
            1,
            file
        ) != 1) {
            ReportModel3DBufferWriteError(0x14e);
        }
        if (fwrite(
            &g_zModel_DiPoolFreeHeadIndex,
            4,
            1,
            file
        ) != 1) {
            ReportModel3DBufferWriteError(0x15b);
        }

        const int capacity = g_zModel_DiPoolCapacity;
        if (capacity == 0) {
            return 0;
        }

        int result = capacity;
        const long tableOffset = ftell(file);
        const int tableBytes = capacity * (int)(sizeof(zDiPartial));
        if (fwrite(
            g_zModel_DiPoolBase,
            tableBytes,
            1,
            file
        ) != 1) {
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
                    if (fwrite(
                        di->verts,
                        0x0c,
                        di->vertCount,
                        file
                    ) != (size_t)(di->vertCount)) {
                        ReportModel3DBufferWriteError(0x18c);
                        result = 0;
                        break;
                    }
                }

                if (di->normalCount > 0) {
                    wroteDynamicData = true;
                    if (fwrite(di->normals, 0x0c, di->normalCount, file) !=
                        (size_t)(di->normalCount)) {
                        ReportModel3DBufferWriteError(0x19f);
                        result = 0;
                        break;
                    }
                }

                if (di->blendVertCount > 0) {
                    wroteDynamicData = true;
                    if (fwrite(di->blendVerts, 0x0c, di->blendVertCount, file) !=
                        (size_t)(di->blendVertCount)) {
                        ReportModel3DBufferWriteError(0x1b2);
                        result = 0;
                        break;
                    }
                }

                if (di->pointCount > 0) {
                    wroteDynamicData = true;
                    if (fwrite(
                            di->pointEntries,
                            sizeof(zModel_PointEntryPartial),
                            di->pointCount,
                            file
                        ) != (size_t)(di->pointCount)) {
                        ReportModel3DBufferWriteError(0x1c9);
                        result = 0;
                        break;
                    }

                    {
                        for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
                            zModel_PointEntryPartial *const point = &di->pointEntries[pointIndex];
                            if (point->pointCamCount > 0 && fwrite(
                                                                point->pointCamList,
                                                                sizeof(zVec3),
                                                                point->pointCamCount,
                                                                file
                                                            ) != (size_t)(point->pointCamCount)) {
                                ReportModel3DBufferWriteError(0x1dd);
                                result = 0;
                                break;
                            }
                        }
                    }
                }

                if (di->entryCount > 0) {
                    wroteDynamicData = true;
                    const int entryBytes = di->entryCount * (int)(sizeof(zDiEntryPartial));
                    zDiEntryPartial *serializedEntries = (zDiEntryPartial *)(malloc(entryBytes));
                    memcpy(
                        serializedEntries,
                        di->entries,
                        entryBytes
                    );

                    {
                        for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
                            const int materialIndex = zModel_MatlSlot::IndexFromPtrOrMinus1(
                                (zModel_MaterialSlot *)(serializedEntries[entryIndex].material)
                            );
                            serializedEntries[entryIndex].material =
                                (zModel_MaterialPartial *)((int)(materialIndex));
                        }
                    }

                    if (fwrite(
                        serializedEntries,
                        entryBytes,
                        1,
                        file
                    ) != 1) {
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
                                fwrite(
                                    entry->vertexIndices,
                                    4,
                                    indexCount,
                                    file
                                ) != indexCount) {
                                ReportModel3DBufferWriteError(0x21e);
                                result = 0;
                                entryWriteFailed = true;
                                break;
                            }

                            if ((entry->flagsAndIndexCount & 0x0200) != 0 &&
                                entry->normalIndices != 0 &&
                                fwrite(
                                    entry->normalIndices,
                                    4,
                                    indexCount,
                                    file
                                ) != indexCount) {
                                ReportModel3DBufferWriteError(0x22e);
                                result = 0;
                                entryWriteFailed = true;
                                break;
                            }

                            const zDiEntryPartial *const liveEntry = &di->entries[entryIndex];
                            if ((liveEntry->material->flags & 0x0100) != 0 &&
                                fwrite(
                                    entry->uvPairs,
                                    8,
                                    indexCount,
                                    file
                                ) != indexCount) {
                                ReportModel3DBufferWriteError(0x240);
                                result = 0;
                                entryWriteFailed = true;
                                break;
                            }
                        }
                    }

                    free(serializedEntries);
                    if (entryWriteFailed) {
                        di->nextFreeIndex = (int)(dynamicOffset);
                        break;
                    }
                }

                if (wroteDynamicData) {
                    di->nextFreeIndex = (int)(dynamicOffset);
                }
            }
        }

        const long endOffset = ftell(file);
        fseek(
            file,
            tableOffset,
            SEEK_SET
        );
        if (fwrite(
            g_zModel_DiPoolBase,
            tableBytes,
            1,
            file
        ) != 1) {
            ReportModel3DBufferWriteError(0x263);
            result = 0;
        }
        fseek(
            file,
            endOffset,
            SEEK_SET
        );
        return result;
    }

    /**
     * Reimplements 0x481bc0: zModel_DiPool::ReadHeaderFromStream
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: read display-instance pool header fields from a stream.
     */
    int __fastcall ReadHeaderFromStream(
        void *stream,
        int *outCapacity,
        int *outInUseCount,
        int *outFreeHeadIndex
    ) {
        FILE *const file = (FILE *)(stream);

        if (fread(
            outCapacity,
            4,
            1,
            file
        ) != 1) {
            ReportModel3DBufferReadError(
                0x28b,
                g_zModel_ReadModel3dBufferDataErrorMsg
            );
            return -1;
        }
        if (fread(
            outInUseCount,
            4,
            1,
            file
        ) != 1) {
            ReportModel3DBufferReadError(
                0x298,
                g_zModel_ReadModel3dBufferDataErrorMsg
            );
            return -1;
        }
        if (fread(
            outFreeHeadIndex,
            4,
            1,
            file
        ) != 1) {
            ReportModel3DBufferReadError(
                0x2a5,
                g_zModel_ReadModel3dBufferDataErrorMsg
            );
            return -1;
        }

        return 0;
    }

    /**
     * Reimplements 0x481c50: zModel_DiPool::ReadEntryDynamicDataFromStream
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: read one display-instance entry's dynamic arrays and repair material pointers.
     */
    int __fastcall ReadEntryDynamicDataFromStream(
        void *stream,
        zDiPartial *entry
    ) {
        FILE *const file = (FILE *)(stream);

        if (entry->vertCount > 0) {
            const int byteCount = entry->vertCount * (int)(sizeof(zVec3));
            entry->verts = (zVec3 *)(malloc(byteCount));
            if (fread(
                entry->verts,
                byteCount,
                1,
                file
            ) != 1) {
                ReportModel3DBufferReadError(
                    0x31c,
                    g_zModel_ReadModel3dVertexDataErrorMsg
                );
                return -1;
            }
        }

        if (entry->normalCount > 0) {
            const int byteCount = entry->normalCount * (int)(sizeof(zVec3));
            entry->normals = (zVec3 *)(malloc(byteCount));
            if (fread(
                entry->normals,
                byteCount,
                1,
                file
            ) != 1) {
                ReportModel3DBufferReadError(
                    0x32f,
                    g_zModel_ReadModel3dVertexNormalDataErrorMsg
                );
                return -1;
            }
        }

        if (entry->blendVertCount > 0) {
            const int byteCount = entry->blendVertCount * (int)(sizeof(zVec3));
            entry->blendVerts = (zVec3 *)(malloc(byteCount));
            if (fread(
                entry->blendVerts,
                byteCount,
                1,
                file
            ) != 1) {
                ReportModel3DBufferReadError(
                    0x342,
                    g_zModel_ReadModel3dMorphVertexDataErrorMsg
                );
                return -1;
            }
        }

        if (entry->pointCount > 0) {
            const int byteCount = entry->pointCount * (int)(sizeof(zModel_PointEntryPartial));
            entry->pointEntries = (zModel_PointEntryPartial *)(malloc(byteCount));
            if (fread(
                entry->pointEntries,
                byteCount,
                1,
                file
            ) != 1) {
                ReportModel3DBufferReadError(
                    0x358,
                    g_zModel_ReadModel3dPointLightDataErrorMsg
                );
                return -1;
            }

            {
                for (int pointIndex = 0; pointIndex < entry->pointCount; ++pointIndex) {
                    zModel_PointEntryPartial *const point = &entry->pointEntries[pointIndex];
                    const unsigned short packedColor = (unsigned short)(zVid_PackColorRGB(
                        (unsigned char)((int)(point->colorB + 0.5f)),
                        (unsigned char)((int)(point->colorG + 0.5f)),
                        (unsigned char)((int)(point->colorR + 0.5f))
                    ));
                    point->packedColor16 = (point->packedColor16 & 0xffff0000) | packedColor;

                    if (point->pointCamCount > 0) {
                        const int pointCamBytes = point->pointCamCount * (int)(sizeof(zVec3));
                        point->pointCamList = (zVec3 *)(malloc(pointCamBytes));
                        if (fread(
                            point->pointCamList,
                            pointCamBytes,
                            1,
                            file
                        ) != 1) {
                            ReportModel3DBufferReadError(
                                0x372,
                                g_zModel_ReadModel3dPointLightDataErrorMsg
                            );
                            return -1;
                        }
                    }
                }
            }
        }

        if (entry->entryCount <= 0) {
            return 0;
        }

        const int entryBytes = entry->entryCount * (int)(sizeof(zDiEntryPartial));
        entry->entries = (zDiEntryPartial *)(malloc(entryBytes));
        if (fread(
            entry->entries,
            entryBytes,
            1,
            file
        ) != 1) {
            ReportModel3DBufferReadError(
                0x38f,
                g_zModel_ReadModel3dPolygonBufferErrorMsg
            );
            return -1;
        }

        {
            for (int entryIndex = 0; entryIndex < entry->entryCount; ++entryIndex) {
                zDiEntryPartial *const diEntry = &entry->entries[entryIndex];
                diEntry->material = (zModel_MaterialPartial *)(zModel_Matl::GetPoolEntry(
                    (int)((int)(diEntry->material))
                ));
            }
        }

        {
            for (int entryIndex = 0; entryIndex < entry->entryCount; ++entryIndex) {
                zDiEntryPartial *const diEntry = &entry->entries[entryIndex];
                const unsigned int indexCount = diEntry->flagsAndIndexCount & 0xff;
                if (indexCount != 0) {
                    const unsigned int indexBytes = indexCount * 4;
                    diEntry->vertexIndices = malloc(indexBytes);
                    if (fread(
                        diEntry->vertexIndices,
                        indexBytes,
                        1,
                        file
                    ) != 1) {
                        ReportModel3DBufferReadError(
                            0x3ae,
                            g_zModel_ReadModel3dPolyVertIndexErrorMsg
                        );
                        return -1;
                    }
                }

                if ((diEntry->flagsAndIndexCount & 0x0200) != 0) {
                    const unsigned int indexBytes = indexCount * 4;
                    diEntry->normalIndices = malloc(indexBytes);
                    if (fread(
                        diEntry->normalIndices,
                        indexBytes,
                        1,
                        file
                    ) != 1) {
                        ReportModel3DBufferReadError(
                            0x3c0,
                            g_zModel_ReadModel3dPolyVertNormalIndexErrorMsg
                        );
                        return -1;
                    }
                }

                if ((diEntry->material->flags & 0x0100) != 0) {
                    const unsigned int uvBytes = indexCount * (unsigned int)(sizeof(zModel_Uv));
                    diEntry->uvPairs = malloc(uvBytes);
                    if (fread(
                        diEntry->uvPairs,
                        uvBytes,
                        1,
                        file
                    ) != 1) {
                        ReportModel3DBufferReadError(
                            0x3d4,
                            g_zModel_ReadModel3dPolyTexVertDataErrorMsg
                        );
                        return -1;
                    }
                }
            }
        }

        return 0;
    }

    /**
     * Reimplements 0x481aa0: zModel_DiPool::ReadEntryByIndexFromStream
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: load one serialized display-instance entry by pool index.
     */
    RECOIL_NO_GS zDiPartial *__fastcall ReadEntryByIndexFromStream(
        void *stream,
        int index
    ) {
        FILE *const file = (FILE *)(stream);

        int serializedCapacity;
        int serializedInUseCount;
        int serializedFreeHeadIndex;
        if (ReadHeaderFromStream(
                file,
                &serializedCapacity,
                &serializedInUseCount,
                &serializedFreeHeadIndex
            ) != 0) {
            ReportModel3DBufferReadError(
                0x401,
                g_zModel_ReadModel3dBufferHeaderErrorMsg
            );
            return 0;
        }

        if (serializedCapacity == 0) {
            return 0;
        }

        if (index >= serializedCapacity) {
            return 0;
        }

        fseek(
            file,
            index * (int)(sizeof(zDiPartial)),
            SEEK_CUR
        );

        zDiPartial serializedEntry;
        if (fread(
            &serializedEntry,
            sizeof(zDiPartial),
            1,
            file
        ) != 1) {
            ReportModel3DBufferReadError(
                0x41a,
                g_zModel_ReadModel3dBufferDataErrorMsg
            );
            return 0;
        }

        zDiPartial *const entry = AllocFromFreeList();
        if (entry == 0) {
            return 0;
        }

        memcpy(
            entry,
            &serializedEntry,
            offsetof(zDiPartial, nextFreeIndex)
        );
        fseek(
            file,
            serializedEntry.nextFreeIndex,
            SEEK_SET
        );
        if (ReadEntryDynamicDataFromStream(
            file,
            entry
        ) != 0) {
            FreeIfUnreferenced(entry);
            return 0;
        }

        return entry;
    }

    /**
     * Reimplements 0x481fa0: zModel_DiPool::ReadFromStream
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: read the display-instance pool and all dynamic entry payloads from a stream.
     */
    int __fastcall ReadFromStream(void *stream) {
        FILE *const file = (FILE *)(stream);
        const int oldCapacity = g_zModel_DiPoolCapacity;

        if (ReadHeaderFromStream(
                file,
                &g_zModel_DiPoolCapacity,
                &g_zModel_DiPoolInUseCount,
                &g_zModel_DiPoolFreeHeadIndex
            ) != 0) {
            ReportModel3DBufferReadError(
                0x45b,
                g_zModel_ReadModel3dBufferHeaderErrorMsg
            );
            return -1;
        }

        if (g_zModel_DiPoolCapacity == 0) {
            return 0;
        }

        const int poolBytes = g_zModel_DiPoolCapacity * (int)(sizeof(zDiPartial));
        if (g_zModel_DiPoolBase == 0) {
            g_zModel_DiPoolBase = (zDiPartial *)(malloc(poolBytes));
        } else if (g_zModel_DiPoolCapacity > oldCapacity) {
            g_zModel_DiPoolBase = (zDiPartial *)(realloc(
                g_zModel_DiPoolBase,
                poolBytes
            ));
        }

        if (fread(
            g_zModel_DiPoolBase,
            poolBytes,
            1,
            file
        ) != 1) {
            ReportModel3DBufferReadError(
                0x476,
                g_zModel_ReadModel3dBufferDataErrorMsg
            );
            return -1;
        }

        {
            for (int poolIndex = 0; poolIndex < g_zModel_DiPoolCapacity; ++poolIndex) {
                ReadEntryDynamicDataFromStream(
                    file,
                    &g_zModel_DiPoolBase[poolIndex]
                );
            }
        }

        return g_zModel_DiPoolCapacity;
    }
}
