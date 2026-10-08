#include "recoil/Mfc42Abi.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zTime/time.h"
#include "zdi.h"

#include "Battlesport/player.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-sourcefile-gmodconstc
 * @recoil-artifact defines .data recoil:data:0x4e13a0: g_zModel_SourceFile_GmodConstC.
 * Data owner: geometry_model_assets.zmodel_gmod_const_literals.
 * Purpose: store the writable gmod_const.c source-file path used by model
 * buffer diagnostics.
 *
 * Retail 0x4e13a0: initialized .data char[0x28] literal
 * "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c".
 */
char g_zModel_SourceFile_GmodConstC[0x28] = "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c";
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SourceFile_GmodConstC) == 0x28);

/*
 * BN identifies the gmod_const.c Model3D diagnostics as writable .data char
 * arrays in this order, including VC alignment padding between rows.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-writemodel3dbuffererrormsg
 * @recoil-artifact defines .data recoil:data:0x4e13c8: g_zModel_WriteModel3dBufferErrorMsg.
 * Purpose: store the writable Model3D buffer write failure diagnostic.
 */
char g_zModel_WriteModel3dBufferErrorMsg[0x1e] = "Error writing model3d buffer.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dbufferdataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e13e8: g_zModel_ReadModel3dBufferDataErrorMsg.
 * Purpose: store the writable Model3D buffer read failure diagnostic.
 */
char g_zModel_ReadModel3dBufferDataErrorMsg[0x29] = "Error reading GameZ Model3D buffer data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dbufferheadererrormsg
 * @recoil-artifact defines .data recoil:data:0x4e1414: g_zModel_ReadModel3dBufferHeaderErrorMsg.
 * Purpose: store the writable Model3D buffer header read failure diagnostic.
 */
char g_zModel_ReadModel3dBufferHeaderErrorMsg[0x30] = "Error reading GameZ Model3D buffer header data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dpolytexvertdataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e1444: g_zModel_ReadModel3dPolyTexVertDataErrorMsg.
 * Purpose: store the writable Model3D polygon texture-vertex read diagnostic.
 */
char g_zModel_ReadModel3dPolyTexVertDataErrorMsg[0x39] = "Error reading GameZ Model3D polygon texture vertex data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dpolyvertnormalindexerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e1480: g_zModel_ReadModel3dPolyVertNormalIndexErrorMsg.
 * Purpose: store the writable Model3D polygon normal-index read diagnostic.
 */
char g_zModel_ReadModel3dPolyVertNormalIndexErrorMsg[0x39] = "Error reading GameZ Model3D polygon vertex normal index.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dpolyvertindexerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e14bc: g_zModel_ReadModel3dPolyVertIndexErrorMsg.
 * Purpose: store the writable Model3D polygon vertex-index read diagnostic.
 */
char g_zModel_ReadModel3dPolyVertIndexErrorMsg[0x32] = "Error reading GameZ Model3D polygon vertex index.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dpolygonbuffererrormsg
 * @recoil-artifact defines .data recoil:data:0x4e14f0: g_zModel_ReadModel3dPolygonBufferErrorMsg.
 * Purpose: store the writable Model3D polygon-buffer read diagnostic.
 */
char g_zModel_ReadModel3dPolygonBufferErrorMsg[0x2c] = "Error reading GameZ Model3D polygon buffer.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dpointlightdataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e151c: g_zModel_ReadModel3dPointLightDataErrorMsg.
 * Purpose: store the writable Model3D point-light data read diagnostic.
 */
char g_zModel_ReadModel3dPointLightDataErrorMsg[0x2e] = "Error reading GameZ Model3D point light data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dmorphvertexdataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e154c: g_zModel_ReadModel3dMorphVertexDataErrorMsg.
 * Purpose: store the writable Model3D morph-vertex read diagnostic.
 */
char g_zModel_ReadModel3dMorphVertexDataErrorMsg[0x2f] = "Error reading GameZ Model3D morph vertex data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dvertexnormaldataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e157c: g_zModel_ReadModel3dVertexNormalDataErrorMsg.
 * Purpose: store the writable Model3D vertex-normal read diagnostic.
 */
char g_zModel_ReadModel3dVertexNormalDataErrorMsg[0x30] = "Error reading GameZ Model3D vertex normal data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-readmodel3dvertexdataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e15ac: g_zModel_ReadModel3dVertexDataErrorMsg.
 * Purpose: store the writable Model3D vertex read diagnostic.
 */
char g_zModel_ReadModel3dVertexDataErrorMsg[0x29] = "Error reading GameZ Model3D vertex data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-createmodel3dbufferfullerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e15d8: g_zModel_CreateModel3dBufferFullErrorMsg.
 * Purpose: store the writable Model3D create-buffer-full diagnostic.
 */
char g_zModel_CreateModel3dBufferFullErrorMsg[0x2c] = "ERROR: Creating Model3D; model buffer full.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-createmodel3dapproachinglimitfmt
 * @recoil-artifact defines .data recoil:data:0x4e1604: g_zModel_CreateModel3dApproachingLimitFmt.
 * Purpose: store the writable Model3D creation limit warning format.
 */
char g_zModel_CreateModel3dApproachingLimitFmt[0x28] = "         Approaching max allowable: %d\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-vertexcountwarningfmt
 * @recoil-artifact defines .data recoil:data:0x4e162c: g_zModel_VertexCountWarningFmt.
 * Purpose: store the writable model vertex-count warning format.
 */
char g_zModel_VertexCountWarningFmt[0x2f] = "%s: Line %d: WARNING: Model vertex count = %d\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-normalcountwarningfmt
 * @recoil-artifact defines .data recoil:data:0x4e165c: g_zModel_NormalCountWarningFmt.
 * Purpose: store the writable model normal-count warning format.
 */
char g_zModel_NormalCountWarningFmt[0x2f] = "%s: Line %d: WARNING: Model normal count = %d\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-addpolygontoofewvertsfmt
 * @recoil-artifact defines .data recoil:data:0x4e168c: g_zModel_AddPolygonTooFewVertsFmt.
 * Purpose: store the writable AddPolygon too-few-vertices diagnostic format.
 */
char g_zModel_AddPolygonTooFewVertsFmt[0x2d] = "Attempting to add polygon with only %d verts";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-addnonplanarpolygontriangulatingfmt
 * @recoil-artifact defines .data recoil:data:0x4e16bc: g_zModel_AddNonPlanarPolygonTriangulatingFmt.
 * Purpose: store the writable non-planar AddPolygon triangulation diagnostic.
 */
char g_zModel_AddNonPlanarPolygonTriangulatingFmt[0x42]
    = "Attempting to add non-planar polygon (%d verts), triangulating...";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-discardpolygonaftercheckcolinearityfmt
 * @recoil-artifact defines .data recoil:data:0x4e1700: g_zModel_DiscardPolygonAfterCheckColinearityFmt.
 * Purpose: store the writable AddPolygon colinearity discard diagnostic.
 */
char g_zModel_DiscardPolygonAfterCheckColinearityFmt[0x41]
    = "Discarding Polygon: (%d of %d) verts after 'check_colinearity()'";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-polyvertexcountapproachinglimitfmt
 * @recoil-artifact defines .data recoil:data:0x4e1744: g_zModel_PolyVertexCountApproachingLimitFmt.
 * Purpose: store the writable polygon vertex-count limit warning format.
 */
char g_zModel_PolyVertexCountApproachingLimitFmt[0x2e] = "Poly vertex count approaching limit (%d / %d)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-addpolygononlyvertserrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e1774: g_zModel_AddPolygonOnlyVertsErrorFmt.
 * Purpose: store the writable AddPolygon only-vertices error format.
 */
char g_zModel_AddPolygonOnlyVertsErrorFmt[0x3b] = "ERROR: You're trying to add a Polygon with only (%d) verts";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-setmodelcycletexturenullmodelfmt
 * @recoil-artifact defines .data recoil:data:0x4e17b0: g_zModel_SetModelCycleTextureNullModelFmt.
 * Purpose: store the writable SetModelCycleTexture null-model diagnostic.
 */
char g_zModel_SetModelCycleTextureNullModelFmt[0x46]
    = "%s(%d): ERROR setting model cycle texture. Model 3D pointer is NULL.\n";
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

namespace
{
    struct MaterialClonePair {
        zModel_MaterialPartial* source;
        zModel_MaterialPartial* clone;
    };
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-constvertexmergeepsilon
 * @recoil-artifact defines .data recoil:data:0x4e1398: g_zModel_ConstVertexMergeEpsilon.
 * Purpose: Stores g zModel ConstVertexMergeEpsilon data used by engine.zmodel.vertex_merge_epsilon_global.
 */
float g_zModel_ConstVertexMergeEpsilon = 0.001f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-maxpolygonvertexcountbeforesplit
 * @recoil-artifact defines .data recoil:data:0x4e139c: g_zModel_MaxPolygonVertexCountBeforeSplit.
 * Purpose: store the AddPolygonEx vertex-count threshold before chunk splitting.
 */
int g_zModel_MaxPolygonVertexCountBeforeSplit = 48;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-coplanartolerance
 * @recoil-artifact defines .data recoil:data:0x4e1388: g_zModel_CoplanarTolerance.
 * Purpose: store the coplanar polygon tolerance.
 */
double g_zModel_CoplanarTolerance = 0.001;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.g-zmodel-colineartolerance
 * @recoil-artifact defines .data recoil:data:0x4e1390: g_zModel_ColinearTolerance.
 * Purpose: store the colinear polygon tolerance.
 */
double g_zModel_ColinearTolerance = 0.001;
float g_zModel_UvQuantizeBias = -0.001953125f;
float g_zModel_UvQuantizeScale = 256.0f;
float g_zModel_UvQuantizeInvScale = 0.00390625f;

namespace zModel_Const { }

namespace zModel_DiPool { }

namespace zModel_Const
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.getvertexmergeepsilon
     * @recoil-artifact defines .text recoil:function:0x481530: zModel_Const::GetVertexMergeEpsilon
     * @recoil-match byte
     *
     * Purpose: return the global vertex-merge epsilon.
     */
    float __cdecl GetVertexMergeEpsilon()
    {
        return g_zModel_ConstVertexMergeEpsilon;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setvertexmergeepsilon
     * @recoil-artifact defines .text recoil:function:0x481540: zModel_Const::SetVertexMergeEpsilon
     * @recoil-match byte
     *
     * Purpose: set the global vertex-merge epsilon using the original bit-preserving copy.
     */
    void __stdcall SetVertexMergeEpsilon(float epsilon)
    {
        unsigned int bits;
        memcpy(&bits, &epsilon, sizeof(bits));
        memcpy(&g_zModel_ConstVertexMergeEpsilon, &bits, sizeof(bits));
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setcoplanartolerance
     * @recoil-artifact defines .text recoil:function:0x481550: zModel_Const::SetCoplanarTolerance
     * @recoil-match byte
     *
     * Purpose: set the global coplanar polygon tolerance.
     */
    void __stdcall SetCoplanarTolerance(float tolerance)
    {
        g_zModel_CoplanarTolerance = tolerance;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setcolineartolerance
     * @recoil-artifact defines .text recoil:function:0x481560: zModel_Const::SetColinearTolerance
     * @recoil-match byte
     *
     * Purpose: set the global colinear polygon tolerance.
     */
    void __stdcall SetColinearTolerance(float tolerance)
    {
        g_zModel_ColinearTolerance = tolerance;
    }
} // namespace zModel_Const

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.ptrtoindexorminus1
     * @recoil-artifact defines .text recoil:function:0x481570: zDi::PtrToIndexOrMinus1
     * @recoil-match byte
     *
     * Purpose: convert a display-instance pool pointer to its pool index, or -1 for null.
     */
    int __fastcall PtrToIndexOrMinus1(zDiPartial * self)
    {
        if (self == 0) {
            return -1;
        }

        return (int)(self - g_zModel_DiPoolBase);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.indextoptrornull
     * @recoil-artifact defines .text recoil:function:0x4815a0: zDi::IndexToPtrOrNull
     * @recoil-match byte
     *
     * Purpose: convert a non-negative display-instance pool index to its entry pointer.
     */
    zDiPartial* __fastcall IndexToPtrOrNull(int index)
    {
        if (index < 0) {
            return 0;
        }

        return &g_zModel_DiPoolBase[index];
    }
} // namespace zDi

namespace zModel_DiPool
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.writetostream
     * @recoil-artifact defines .text recoil:function:0x4815c0: zModel_DiPool::WriteToStream
     *
     *
     * Purpose: serialize the display-instance pool and its dynamic arrays to a stream.
     */
    int __fastcall WriteToStream(void* stream)
    {
        FILE* const file = (FILE*)(stream);

        if (fwrite(&g_zModel_DiPoolCapacity, 4, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x141, g_zModel_WriteModel3dBufferErrorMsg);
        }
        if (fwrite(&g_zModel_DiPoolInUseCount, 4, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x14e, g_zModel_WriteModel3dBufferErrorMsg);
        }
        if (fwrite(&g_zModel_DiPoolFreeHeadIndex, 4, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x15b, g_zModel_WriteModel3dBufferErrorMsg);
        }

        if (g_zModel_DiPoolCapacity == 0) {
            return 0;
        }

        const int capacity = g_zModel_DiPoolCapacity;
        int result = capacity;
        const long tableOffset = ftell(file);
        if (fwrite(g_zModel_DiPoolBase, capacity * sizeof(zDiPartial), 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x172, g_zModel_WriteModel3dBufferErrorMsg);
            result = 0;
        }

        for (int diIndex = 0; diIndex < result; ++diIndex) {
            const long dynamicOffset = ftell(file);
            zDiPartial* const di = &g_zModel_DiPoolBase[diIndex];
            int wroteDynamicData = 0;

            if (di->vertCount > 0) {
                wroteDynamicData = 1;
                if (fwrite(di->verts, 0x0c, di->vertCount, file) != (size_t)(di->vertCount)) {
                    zError::ReportOld(
                        0x200,
                        g_zModel_SourceFile_GmodConstC,
                        0x18c,
                        g_zModel_WriteModel3dBufferErrorMsg
                    );
                    result = 0;
                    break;
                }
            }

            if (di->normalCount > 0) {
                wroteDynamicData = 1;
                if (fwrite(di->normals, 0x0c, di->normalCount, file) != (size_t)(di->normalCount)) {
                    zError::ReportOld(
                        0x200,
                        g_zModel_SourceFile_GmodConstC,
                        0x19f,
                        g_zModel_WriteModel3dBufferErrorMsg
                    );
                    result = 0;
                    break;
                }
            }

            if (di->blendVertCount > 0) {
                wroteDynamicData = 1;
                if (fwrite(di->blendVerts, 0x0c, di->blendVertCount, file) != (size_t)(di->blendVertCount)) {
                    zError::ReportOld(
                        0x200,
                        g_zModel_SourceFile_GmodConstC,
                        0x1b2,
                        g_zModel_WriteModel3dBufferErrorMsg
                    );
                    result = 0;
                    break;
                }
            }

            if (di->pointCount > 0) {
                wroteDynamicData = 1;
                if (fwrite(di->pointEntries, sizeof(zModel_PointEntryPartial), di->pointCount, file)
                    != (size_t)(di->pointCount)) {
                    zError::ReportOld(
                        0x200,
                        g_zModel_SourceFile_GmodConstC,
                        0x1c9,
                        g_zModel_WriteModel3dBufferErrorMsg
                    );
                    result = 0;
                    break;
                }

                zModel_PointEntryPartial* point = di->pointEntries;
                for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex, ++point) {
                    if (point->pointCamCount > 0
                        && fwrite(point->pointCamList, sizeof(zVec3), point->pointCamCount, file)
                            != (size_t)(point->pointCamCount)) {
                        zError::ReportOld(
                            0x200,
                            g_zModel_SourceFile_GmodConstC,
                            0x1dd,
                            g_zModel_WriteModel3dBufferErrorMsg
                        );
                        result = 0;
                        break;
                    }
                }
            }

            const int entryCount = di->entryCount;
            if (entryCount > 0) {
                wroteDynamicData = 1;
                const int entryBytes = entryCount * (int)(sizeof(zDiEntryPartial));
                zDiEntryPartial* serializedEntries = (zDiEntryPartial*)(malloc(entryBytes));
                memcpy(serializedEntries, di->entries, entryBytes);

                zDiEntryPartial* convertedEntry = serializedEntries;
                for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex, ++convertedEntry) {
                    convertedEntry->material = (zModel_MaterialPartial*)(zModel_MatlSlot::IndexFromPtrOrMinus1(
                        (zModel_MaterialSlot*)(convertedEntry->material)
                    ));
                }

                if (fwrite(serializedEntries, entryBytes, 1, file) != 1) {
                    zError::ReportOld(
                        0x200,
                        g_zModel_SourceFile_GmodConstC,
                        0x209,
                        g_zModel_WriteModel3dBufferErrorMsg
                    );
                    result = 0;
                    break;
                }

                zDiEntryPartial* const sourceEntries = di->entries;
                for (int writeIndex = 0; writeIndex < entryCount; ++writeIndex) {
                    zDiEntryPartial* const entry = &serializedEntries[writeIndex];
                    if ((entry->flagsAndIndexCount & 0xff) > 0) {
                        if (fwrite(entry->vertexIndices, 4, entry->flagsAndIndexCount & 0xff, file)
                            != (entry->flagsAndIndexCount & 0xff)) {
                            zError::ReportOld(
                                0x200,
                                g_zModel_SourceFile_GmodConstC,
                                0x21e,
                                g_zModel_WriteModel3dBufferErrorMsg
                            );
                            result = 0;
                            break;
                        }
                        if ((entry->flagsAndIndexCount & 0x0200) != 0 && entry->normalIndices != 0
                            && fwrite(entry->normalIndices, 4, entry->flagsAndIndexCount & 0xff, file)
                                != (entry->flagsAndIndexCount & 0xff)) {
                            zError::ReportOld(
                                0x200,
                                g_zModel_SourceFile_GmodConstC,
                                0x22e,
                                g_zModel_WriteModel3dBufferErrorMsg
                            );
                            result = 0;
                            break;
                        }
                    }

                    if ((sourceEntries[writeIndex].material->flags & 0x0100) != 0
                        && fwrite(entry->uvPairs, 8, entry->flagsAndIndexCount & 0xff, file)
                            != (entry->flagsAndIndexCount & 0xff)) {
                        zError::ReportOld(
                            0x200,
                            g_zModel_SourceFile_GmodConstC,
                            0x240,
                            g_zModel_WriteModel3dBufferErrorMsg
                        );
                        result = 0;
                        break;
                    }
                }

                free(serializedEntries);
            }

            if (wroteDynamicData != 0) {
                g_zModel_DiPoolBase[diIndex].nextFreeIndex = (int)(dynamicOffset);
            }
        }

        const long endOffset = ftell(file);
        fseek(file, tableOffset, SEEK_SET);
        if (fwrite(g_zModel_DiPoolBase, g_zModel_DiPoolCapacity * sizeof(zDiPartial), 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x263, g_zModel_WriteModel3dBufferErrorMsg);
            result = 0;
        }
        fseek(file, endOffset, SEEK_SET);
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.readentrybyindexfromstream
     * @recoil-artifact defines .text recoil:function:0x481aa0: zModel_DiPool::ReadEntryByIndexFromStream
     * @recoil-match byte
     *
     * Purpose: load one serialized display-instance entry by pool index.
     */
    RECOIL_NO_GS zDiPartial* __fastcall ReadEntryByIndexFromStream(void* stream, int index)
    {
        FILE* const file = (FILE*)(stream);

        int serializedCapacity;
        int serializedInUseCount;
        int serializedFreeHeadIndex;
        if (ReadHeaderFromStream(file, &serializedCapacity, &serializedInUseCount, &serializedFreeHeadIndex) != 0) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x401, g_zModel_ReadModel3dBufferHeaderErrorMsg);
            return 0;
        }

        if (serializedCapacity == 0) {
            return 0;
        }

        if (index >= serializedCapacity) {
            return 0;
        }

        fseek(file, index * (int)(sizeof(zDiPartial)), SEEK_CUR);

        zDiPartial serializedEntry;
        if (fread(&serializedEntry, sizeof(zDiPartial), 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x41a, g_zModel_ReadModel3dBufferDataErrorMsg);
            return 0;
        }

        zDiPartial* const entry = AllocFromFreeList();
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.readheaderfromstream
     * @recoil-artifact defines .text recoil:function:0x481bc0: zModel_DiPool::ReadHeaderFromStream
     * @recoil-match byte
     *
     * Purpose: read display-instance pool header fields from a stream.
     */
    int __fastcall ReadHeaderFromStream(void* stream, int* outCapacity, int* outInUseCount, int* outFreeHeadIndex)
    {
        FILE* const file = (FILE*)(stream);

        if (fread(outCapacity, 4, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x28b, g_zModel_ReadModel3dBufferDataErrorMsg);
            return -1;
        }
        if (fread(outInUseCount, 4, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x298, g_zModel_ReadModel3dBufferDataErrorMsg);
            return -1;
        }
        if (fread(outFreeHeadIndex, 4, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x2a5, g_zModel_ReadModel3dBufferDataErrorMsg);
            return -1;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.readentrydynamicdatafromstream
     * @recoil-artifact defines .text recoil:function:0x481c50: zModel_DiPool::ReadEntryDynamicDataFromStream
     *
     *
     * Purpose: read one display-instance entry's dynamic arrays and repair material pointers.
     */
    int __fastcall ReadEntryDynamicDataFromStream(void* stream, zDiPartial* entry)
    {
        FILE* const file = (FILE*)(stream);
        int byteCount;

        if (entry->vertCount > 0) {
            byteCount = entry->vertCount * (int)(sizeof(zVec3));
            entry->verts = (zVec3*)(malloc(byteCount));
            if (fread(entry->verts, byteCount, 1, file) != 1) {
                zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x31c, g_zModel_ReadModel3dVertexDataErrorMsg);
                return -1;
            }
        }

        if (entry->normalCount > 0) {
            byteCount = entry->normalCount * (int)(sizeof(zVec3));
            entry->normals = (zVec3*)(malloc(byteCount));
            if (fread(entry->normals, byteCount, 1, file) != 1) {
                zError::ReportOld(
                    0x200,
                    g_zModel_SourceFile_GmodConstC,
                    0x32f,
                    g_zModel_ReadModel3dVertexNormalDataErrorMsg
                );
                return -1;
            }
        }

        if (entry->blendVertCount > 0) {
            byteCount = entry->blendVertCount * (int)(sizeof(zVec3));
            entry->blendVerts = (zVec3*)(malloc(byteCount));
            if (fread(entry->blendVerts, byteCount, 1, file) != 1) {
                zError::ReportOld(
                    0x200,
                    g_zModel_SourceFile_GmodConstC,
                    0x342,
                    g_zModel_ReadModel3dMorphVertexDataErrorMsg
                );
                return -1;
            }
        }

        if (entry->pointCount > 0) {
            byteCount = entry->pointCount * (int)(sizeof(zModel_PointEntryPartial));
            entry->pointEntries = (zModel_PointEntryPartial*)(malloc(byteCount));
            if (fread(entry->pointEntries, byteCount, 1, file) != 1) {
                zError::ReportOld(
                    0x200,
                    g_zModel_SourceFile_GmodConstC,
                    0x358,
                    g_zModel_ReadModel3dPointLightDataErrorMsg
                );
                return -1;
            }

            {
                zModel_PointEntryPartial* point = entry->pointEntries;
                for (int pointIndex = 0; pointIndex < entry->pointCount; ++pointIndex, ++point) {
                    const unsigned short packedColor = (unsigned short)(zVidPackColorRGB(
                        (unsigned char)((int)(point->colorB + 0.5f)),
                        (unsigned char)((int)(point->colorG + 0.5f)),
                        (unsigned char)((int)(point->colorR + 0.5f))
                    ));
                    point->packedColor16 = packedColor;

                    if (point->pointCamCount > 0) {
                        const int pointCamBytes = point->pointCamCount * (int)(sizeof(zVec3));
                        point->pointCamList = (zVec3*)(malloc(pointCamBytes));
                        if (fread(point->pointCamList, pointCamBytes, 1, file) != 1) {
                            zError::ReportOld(
                                0x200,
                                g_zModel_SourceFile_GmodConstC,
                                0x372,
                                g_zModel_ReadModel3dPointLightDataErrorMsg
                            );
                            return -1;
                        }
                    }
                }
            }
        }

        const int entryCount = entry->entryCount;
        if (entryCount <= 0) {
            return 0;
        }

        const int entryBytes = entryCount * (int)(sizeof(zDiEntryPartial));
        zDiEntryPartial* const entries = (zDiEntryPartial*)(malloc(entryBytes));
        entry->entries = entries;
        if (fread(entries, entryBytes, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x38f, g_zModel_ReadModel3dPolygonBufferErrorMsg);
            return -1;
        }

        {
            zDiEntryPartial* diEntry = entries;
            for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex, ++diEntry) {
                diEntry->material
                    = (zModel_MaterialPartial*)(zModel_Matl::GetPoolEntry((int)((int)(diEntry->material))));
            }
        }

        {
            zDiEntryPartial* diEntry = entries;
            for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex, ++diEntry) {
                // Retail tests the loaded word unsigned and reuses it (0x481e7c mov eax,[esi]; test eax,0xff; jbe).
                if ((diEntry->flagsAndIndexCount & 0xff) > 0) {
                    const unsigned int indexBytes = (diEntry->flagsAndIndexCount & 0xff) * 4;
                    diEntry->vertexIndices = malloc(indexBytes);
                    if (fread(diEntry->vertexIndices, indexBytes, 1, file) != 1) {
                        zError::ReportOld(
                            0x200,
                            g_zModel_SourceFile_GmodConstC,
                            0x3ae,
                            g_zModel_ReadModel3dPolyVertIndexErrorMsg
                        );
                        return -1;
                    }

                    if ((diEntry->flagsAndIndexCount & 0x0200) != 0) {
                        const unsigned int normalBytes = (diEntry->flagsAndIndexCount & 0xff) * 4;
                        diEntry->normalIndices = malloc(normalBytes);
                        if (fread(diEntry->normalIndices, normalBytes, 1, file) != 1) {
                            zError::ReportOld(
                                0x200,
                                g_zModel_SourceFile_GmodConstC,
                                0x3c0,
                                g_zModel_ReadModel3dPolyVertNormalIndexErrorMsg
                            );
                            return -1;
                        }
                    }
                }

                if ((entry->entries[entryIndex].material->flags & 0x0100) != 0) {
                    const unsigned int uvBytes
                        = (diEntry->flagsAndIndexCount & 0xff) * (unsigned int)(sizeof(zModel_Uv));
                    diEntry->uvPairs = malloc(uvBytes);
                    if (fread(diEntry->uvPairs, uvBytes, 1, file) != 1) {
                        zError::ReportOld(
                            0x200,
                            g_zModel_SourceFile_GmodConstC,
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.readfromstream
     * @recoil-artifact defines .text recoil:function:0x481fa0: zModel_DiPool::ReadFromStream
     * @recoil-match byte
     *
     * Purpose: read the display-instance pool and all dynamic entry payloads from a stream.
     */
    int __fastcall ReadFromStream(void* stream)
    {
        FILE* const file = (FILE*)(stream);
        const int oldCapacity = g_zModel_DiPoolCapacity;

        if (ReadHeaderFromStream(
                file,
                &g_zModel_DiPoolCapacity,
                &g_zModel_DiPoolInUseCount,
                &g_zModel_DiPoolFreeHeadIndex
            )
            != 0) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x45b, g_zModel_ReadModel3dBufferHeaderErrorMsg);
            return -1;
        }

        if (g_zModel_DiPoolCapacity == 0) {
            return 0;
        }

        const int poolBytes = g_zModel_DiPoolCapacity * (int)(sizeof(zDiPartial));
        if (g_zModel_DiPoolBase == 0) {
            g_zModel_DiPoolBase = (zDiPartial*)(malloc(poolBytes));
        } else if (g_zModel_DiPoolCapacity > oldCapacity) {
            g_zModel_DiPoolBase = (zDiPartial*)(realloc(g_zModel_DiPoolBase, poolBytes));
        }

        if (fread(g_zModel_DiPoolBase, poolBytes, 1, file) != 1) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodConstC, 0x476, g_zModel_ReadModel3dBufferDataErrorMsg);
            return -1;
        }

        {
            for (int poolIndex = 0; poolIndex < g_zModel_DiPoolCapacity; ++poolIndex) {
                ReadEntryDynamicDataFromStream(file, &g_zModel_DiPoolBase[poolIndex]);
            }
        }

        return g_zModel_DiPoolCapacity;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.allocfromfreelist
     * @recoil-artifact defines .text recoil:function:0x482080: zModel_DiPool::AllocFromFreeList
     * @recoil-match byte
     *
     * Purpose: allocate and initialize a display-instance pool entry from the free list.
     */
    zDiPartial* __cdecl AllocFromFreeList()
    {
        const int slotIndex = g_zModel_DiPoolFreeHeadIndex;
        if (slotIndex < 0) {
            zError::ReportOld(0x400, g_zModel_SourceFile_GmodConstC, 0x4a1, g_zModel_CreateModel3dBufferFullErrorMsg);
            return 0;
        }

        zDiPartial* const entry = &g_zModel_DiPoolBase[slotIndex];
        g_zModel_DiPoolFreeHeadIndex = entry->nextFreeIndex;
        g_zModel_DiPoolInUseCount += 1;
        memset(entry, 0, offsetof(zDiPartial, nextFreeIndex));
        entry->flags = (entry->flags & 0xffffffdf) | 0x03;
        return entry;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.freeifunreferenced
     * @recoil-artifact defines .text recoil:function:0x4820f0: zModel_DiPool::FreeIfUnreferenced
     * @recoil-match byte
     *
     * Purpose: release an unreferenced display-instance entry back to the pool free list.
     */
    int __fastcall FreeIfUnreferenced(zDiPartial * di)
    {
        if (di == 0) {
            return 5;
        }

        if (di->refCount != 0) {
            return 1;
        }

        zDi::FreeContents(di);
        memset(di, 0, offsetof(zDiPartial, nextFreeIndex));

        const ptrdiff_t slotIndex = di - g_zModel_DiPoolBase;
        g_zModel_DiPoolBase[slotIndex].nextFreeIndex = g_zModel_DiPoolFreeHeadIndex;
        --g_zModel_DiPoolInUseCount;
        g_zModel_DiPoolFreeHeadIndex = (int)(slotIndex);
        return 0;
    }
} // namespace zModel_DiPool

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.freecontents
     * @recoil-artifact defines .text recoil:function:0x482160: zDi::FreeContents
     * @recoil-match byte
     *
     * Purpose: release all heap-owned arrays and materials held by a display instance.
     */
    int __fastcall FreeContents(zDiPartial * self)
    {
        if (self == 0) {
            return 5;
        }

        zDiEntryPartial* entries = self->entries;
        zDiEntryPartial* entry = entries;
        for (int i = 0; i < self->entryCount; ++i, ++entry) {
            if (entry->vertexIndices != 0) {
                free(entry->vertexIndices);
            }
            entry->vertexIndices = 0;
            if (entry->normalIndices != 0) {
                free(entry->normalIndices);
            }
            entry->normalIndices = 0;
            if (entry->uvPairs != 0) {
                free(entry->uvPairs);
            }
            entry->uvPairs = 0;
        }

        self->entryCount = 0;
        if (self->entries != 0) {
            free(self->entries);
        }
        self->entries = 0;
        if (self->verts != 0) {
            free(self->verts);
        }
        self->verts = 0;
        if (self->normals != 0) {
            free(self->normals);
        }
        self->normals = 0;
        if (self->blendVerts != 0) {
            free(self->blendVerts);
        }
        self->blendVerts = 0;

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
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.clonetoinstance
     * @recoil-artifact defines .text recoil:function:0x482270: zDi::CloneToInstance
     * @recoil-match byte
     *
     * Purpose: clone a display instance, optionally cloning or sharing its material references.
     */
    zDiPartial* __fastcall CloneToInstance(zDiPartial * self, int cloneMaterials, int cloneAuxOnly)
    {
        MaterialClonePair* materialPairs = 0;
        int materialPairCount = 0;
        int i;
        int entryIndex;

        if (self == 0) {
            return 0;
        }

        zDiPartial* const clone = zModel_DiPool::AllocFromFreeList();
        if (clone == 0) {
            return 0;
        }

        clone->mode = self->mode;
        clone->refCount = 0;
        SetFlagBit0(clone, self->flags & 1);
        SetClonedFlag(clone, ((unsigned int)(self->flags) >> 1) & 1);
        clone->flags = (clone->flags & ~0x04) | (self->flags & 0x04);
        clone->flags = (clone->flags & ~0x08) | (self->flags & 0x08);
        clone->flags = (clone->flags & ~0x10) | (self->flags & 0x10);
        clone->blendScale = self->blendScale;
        clone->flags = (clone->flags & ~0x20) | (self->flags & 0x20);
        clone->scrollRateU = self->scrollRateU;
        clone->scrollRateV = self->scrollRateV;
        clone->field2c = self->field2c;

        clone->pointCount = self->pointCount;
        if (clone->pointCount != 0) {
            clone->pointEntries
                = (zModel_PointEntryPartial*)(malloc((size_t)(self->pointCount) * sizeof(zModel_PointEntryPartial)));
            for (i = 0; i < clone->pointCount; ++i) {
                clone->pointEntries[i] = self->pointEntries[i];
                clone->pointEntries[i].pointCamList
                    = (zVec3*)(malloc((size_t)(clone->pointEntries[i].pointCamCount) * sizeof(zVec3)));
                for (int j = 0; j < clone->pointEntries[i].pointCamCount; ++j) {
                    clone->pointEntries[i].pointCamList[j] = self->pointEntries[i].pointCamList[j];
                }
            }
        }

        if (self->blendVertCount > 0) {
            const size_t blendVertBytes = (size_t)(self->blendVertCount) * sizeof(zVec3);
            clone->blendVerts = (zVec3*)(malloc(blendVertBytes));
            memcpy(clone->blendVerts, self->blendVerts, blendVertBytes);
        }
        clone->blendVertCount = self->blendVertCount;

        if (self->vertCount > 0) {
            const size_t vertBytes = (size_t)(self->vertCount) * sizeof(zVec3);
            clone->verts = (zVec3*)(malloc(vertBytes));
            memcpy(clone->verts, self->verts, vertBytes);
        }
        clone->vertCount = self->vertCount;

        if (self->normalCount > 0) {
            const size_t normalBytes = (size_t)(self->normalCount) * sizeof(zVec3);
            clone->normals = (zVec3*)(malloc(normalBytes));
            memcpy(clone->normals, self->normals, normalBytes);
        }
        clone->normalCount = self->normalCount;

        if (self->entryCount > 0) {
            clone->entries = (zDiEntryPartial*)(calloc((size_t)(self->entryCount), sizeof(zDiEntryPartial)));
        }
        clone->entryCount = self->entryCount;

        zDiEntryPartial* destEntry;
        zDiEntryPartial* sourceEntry;
        for (entryIndex = 0, destEntry = clone->entries, sourceEntry = self->entries; entryIndex < clone->entryCount;
            ++entryIndex, ++destEntry, ++sourceEntry) {
            destEntry->drawFlags = sourceEntry->drawFlags;
            destEntry->flagsAndIndexCount
                = (destEntry->flagsAndIndexCount & ~0x100u) | (sourceEntry->flagsAndIndexCount & 0x100u);
            destEntry->flagsAndIndexCount
                = (destEntry->flagsAndIndexCount & ~0x200u) | (sourceEntry->flagsAndIndexCount & 0x200u);
            memcpy(&destEntry->variantTagInitialized, &sourceEntry->variantTagInitialized, 4);

            if (cloneMaterials == 0) {
                destEntry->material = sourceEntry->material;
            } else if (cloneAuxOnly != 0 && zModel_Material::HasAuxData(sourceEntry->material) == 0) {
                destEntry->material = sourceEntry->material;
            } else {
                zModel_MaterialPartial* material = 0;
                for (int pairIndex = 0; pairIndex < materialPairCount; ++pairIndex) {
                    if (materialPairs[pairIndex].source == sourceEntry->material) {
                        material = materialPairs[pairIndex].clone;
                        break;
                    }
                }

                if (material == 0) {
                    material = zModel_Material::Clone(sourceEntry->material);
                    materialPairs = (MaterialClonePair*)(realloc(
                        materialPairs,
                        (size_t)(materialPairCount + 1) * sizeof(MaterialClonePair)
                    ));
                    materialPairs[materialPairCount].source = sourceEntry->material;
                    materialPairs[materialPairCount].clone = material;
                    ++materialPairCount;
                }
                destEntry->material = material;
            }

            if ((sourceEntry->flagsAndIndexCount & 0xff) > 0) {
                size_t indexBytes = (size_t)(sourceEntry->flagsAndIndexCount & 0xff) * sizeof(int);
                destEntry->vertexIndices = malloc(indexBytes);
                memcpy(destEntry->vertexIndices, sourceEntry->vertexIndices, indexBytes);
                if ((sourceEntry->flagsAndIndexCount & 0x200) != 0 && sourceEntry->normalIndices != 0) {
                    indexBytes = (size_t)(sourceEntry->flagsAndIndexCount & 0xff) * sizeof(int);
                    destEntry->normalIndices = malloc(indexBytes);
                    memcpy(destEntry->normalIndices, sourceEntry->normalIndices, indexBytes);
                }
            }

            destEntry->flagsAndIndexCount
                = (destEntry->flagsAndIndexCount & ~0xffu) | (sourceEntry->flagsAndIndexCount & 0xffu);
            if ((destEntry->material->flags & 0x0100) != 0) {
                const size_t uvBytes = (size_t)(destEntry->flagsAndIndexCount & 0xff) * sizeof(zClipUV);
                destEntry->uvPairs = malloc(uvBytes);
                memcpy(destEntry->uvPairs, sourceEntry->uvPairs, uvBytes);
            }
        }

        if (materialPairs != 0) {
            free(materialPairs);
        }

        return clone;
    }
} // namespace zDi

namespace zUtil
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.storeint32
     * @recoil-artifact defines .text recoil:function:0x4826a0: zUtil::StoreInt32.
     * @recoil-match byte
     *
     * Purpose: Stores the supplied 32-bit integer through the destination pointer.
     */
    void __fastcall StoreInt32(int* outValue, int value)
    {
        *outValue = value;
    }
} // namespace zUtil

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setclonedflag
     * @recoil-artifact defines .text recoil:function:0x4826b0: zDi::SetClonedFlag
     * @recoil-match byte
     *
     * Purpose: update the display-instance cloned flag bit.
     */
    void __fastcall SetClonedFlag(zDiPartial * self, int isCloned)
    {
        if (self != 0) {
            self->flags = (self->flags & ~0x02) | ((isCloned & 1) << 1);
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setflagbit0
     * @recoil-artifact defines .text recoil:function:0x4826d0: zDi::SetFlagBit0
     * @recoil-match byte
     *
     * Purpose: update display-instance flag bit 0 while preserving other flags.
     */
    void __fastcall SetFlagBit0(zDiPartial * self, int enabled)
    {
        if (self != 0) {
            self->flags = ((enabled ^ self->flags) & 1) ^ self->flags;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addref
     * @recoil-artifact defines .text recoil:function:0x4826f0: zDi::AddRef
     * @recoil-match byte
     *
     * Purpose: increment a display-instance reference count.
     */
    int __fastcall AddRef(zDiPartial * self)
    {
        ++self->refCount;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.release
     * @recoil-artifact defines .text recoil:function:0x482700: zDi::Release
     * @recoil-match byte
     *
     * Purpose: decrement a display-instance reference count.
     */
    int __fastcall Release(zDiPartial * self)
    {
        --self->refCount;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.getrefcount
     * @recoil-artifact defines .text recoil:function:0x482710: zDi::GetRefCount
     * @recoil-match byte
     *
     * Purpose: return a display-instance reference count.
     */
    int __fastcall GetRefCount(zDiPartial * self)
    {
        return self->refCount;
    }
} // namespace zDi

namespace zModel_Const
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addormergevertex
     * @recoil-artifact defines .text recoil:function:0x482720: zModel_Const::AddOrMergeVertex
     * @recoil-match byte
     *
     * Purpose: find an existing nearby vertex or append a new display-instance vertex.
     */
    int __fastcall AddOrMergeVertex(zDiPartial * self, zVec3 * point)
    {
        int vertexIndex = -1;
        zVec3* existingPoint = self->verts;
        for (int i = 0; i < self->vertCount; ++existingPoint, ++i) {
            if (fabs(existingPoint->x - point->x) <= g_zModel_ConstVertexMergeEpsilon
                && fabs(existingPoint->y - point->y) <= g_zModel_ConstVertexMergeEpsilon
                && fabs(existingPoint->z - point->z) <= g_zModel_ConstVertexMergeEpsilon) {
                vertexIndex = i;
                break;
            }
        }

        if (vertexIndex == -1) {
            vertexIndex = self->vertCount;
            if ((double)(self->vertCount) > 921.6) {
                sprintf(
                    g_zError_DebugMsgBuffer,
                    g_zModel_VertexCountWarningFmt,
                    g_zModel_SourceFile_GmodConstC,
                    1783,
                    self->vertCount
                );
                sprintf(
                    g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                    g_zModel_CreateModel3dApproachingLimitFmt,
                    1024
                );
                zError::EmitDebugBuffer(1);
                return -1;
            }

            self->verts = (zVec3*)(realloc(self->verts, (self->vertCount + 1) * sizeof(zVec3)));
            // Retail copies the appended vertex field by field through this pointer.
            zVec3* const appended = &self->verts[self->vertCount];
            appended->x = point->x;
            appended->y = point->y;
            appended->z = point->z;
            ++self->vertCount;
        }

        return vertexIndex;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addormergevertexandnormal
     * @recoil-artifact defines .text recoil:function:0x482860: zModel_Const::AddOrMergeVertexAndNormal
     * @recoil-match byte
     *
     * Purpose: find or append a vertex plus its blend-normal delta.
     */
    int __fastcall AddOrMergeVertexAndNormal(zDiPartial * self, zVec3 * point, zVec3 * normal)
    {
        zVec3 blendNormalDelta;
        blendNormalDelta.x = normal->x - point->x;
        blendNormalDelta.y = normal->y - point->y;
        blendNormalDelta.z = normal->z - point->z;

        int vertexIndex = -1;
        zVec3* existingPoint = self->verts;
        zVec3* existingBlend = self->blendVerts;
        for (int i = 0; i < self->vertCount; ++existingPoint, ++existingBlend, ++i) {
            if (existingPoint->x == point->x && existingPoint->y == point->y && existingPoint->z == point->z
                && existingBlend->x == blendNormalDelta.x && existingBlend->y == blendNormalDelta.y
                && existingBlend->z == blendNormalDelta.z) {
                vertexIndex = i;
                break;
            }
        }

        if (vertexIndex == -1) {
            vertexIndex = self->vertCount;
            self->verts = (zVec3*)(realloc(self->verts, (self->vertCount + 1) * sizeof(zVec3)));
            // Retail copies the appended vertex field by field through this pointer.
            zVec3* const appendedVert = &self->verts[self->vertCount];
            appendedVert->x = point->x;
            appendedVert->y = point->y;
            appendedVert->z = point->z;

            self->blendVerts = (zVec3*)(realloc(self->blendVerts, (self->vertCount + 1) * sizeof(zVec3)));
            self->blendVerts[self->vertCount] = blendNormalDelta;

            ++self->vertCount;
            self->blendVertCount = self->vertCount;
            if ((double)(self->vertCount) > 921.6) {
                sprintf(
                    g_zError_DebugMsgBuffer,
                    g_zModel_VertexCountWarningFmt,
                    g_zModel_SourceFile_GmodConstC,
                    1896,
                    self->vertCount
                );
                sprintf(
                    g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                    g_zModel_CreateModel3dApproachingLimitFmt,
                    1024
                );
                zError::EmitDebugBuffer(1);
                return -1;
            }
        }

        return vertexIndex;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.findorappendnormalindex
     * @recoil-artifact defines .text recoil:function:0x482a10: zModel_Const::FindOrAppendNormalIndex
     * @recoil-match byte
     *
     * Purpose: find an existing nearby normal or append a new normal.
     */
    int __fastcall FindOrAppendNormalIndex(zDiPartial * self, zVec3 * normal)
    {
        int normalIndex = -1;
        zVec3* existingNormal = self->normals;
        for (int i = 0; i < self->normalCount; ++existingNormal, ++i) {
            if (fabs(existingNormal->x - normal->x) < 0.0001f && fabs(existingNormal->y - normal->y) < 0.0001f
                && fabs(existingNormal->z - normal->z) < 0.0001f) {
                normalIndex = i;
                break;
            }
        }

        if (normalIndex == -1) {
            normalIndex = self->normalCount;
            self->normals = (zVec3*)(realloc(self->normals, (self->normalCount + 1) * sizeof(zVec3)));
            // Retail copies the appended normal field by field through this pointer.
            zVec3* const appended = &self->normals[self->normalCount];
            appended->x = normal->x;
            appended->y = normal->y;
            appended->z = normal->z;
            ++self->normalCount;
            if ((double)(self->normalCount) > 921.6) {
                sprintf(
                    g_zError_DebugMsgBuffer,
                    g_zModel_NormalCountWarningFmt,
                    g_zModel_SourceFile_GmodConstC,
                    1972,
                    self->normalCount
                );
                sprintf(
                    g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                    g_zModel_CreateModel3dApproachingLimitFmt,
                    1024
                );
                zError::EmitDebugBuffer(1);
                return -1;
            }
        }

        return normalIndex;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.removecolinearverticesinplace
     * @recoil-artifact defines .text recoil:function:0x482b40: zModel_Const::check_colinearity
     * @recoil-match byte
     *
     * Purpose: remove colinear vertices from a polygon point array in place.
     */
    int __fastcall check_colinearity(int* vertexCount, zVec3* points, zClipUV*, zVec3*, zClipUV*)
    {
        int removedAnyVertices = 0;
        int removedVertexThisPass;
        int nextIndex;
        int vertexIndex;
        int scannedVertexCount;
        int copyIndex;

        do {
            // Retail sets both walk indices before the for-loop vertex-count guard.
            nextIndex = 2;
            vertexIndex = 1;
            removedVertexThisPass = 0;

            for (scannedVertexCount = 2; scannedVertexCount <= *vertexCount; ++scannedVertexCount) {
                // The helper returns the normal by value: retail pushes the hidden return slot after
                // vertex2 (0x482b88..0x482b90) and copies the returned vector before the tolerance tests.
                const zVec3 normal = SetNormalizedCrossFromVertexTriplet(
                    &points[vertexIndex - 1],
                    &points[vertexIndex],
                    &points[nextIndex]
                );

                if (fabs(normal.x) < g_zModel_ColinearTolerance && fabs(normal.y) < g_zModel_ColinearTolerance
                    && fabs(normal.z) < g_zModel_ColinearTolerance) {
                    removedAnyVertices = 1;
                    removedVertexThisPass = 1;

                    // Shift the remaining vertices down over the removed one (retail loop 0x482c24).
                    for (copyIndex = vertexIndex; copyIndex < *vertexCount - 1; ++copyIndex) {
                        zVec3* const dest = &points[copyIndex];
                        *dest = dest[1];
                    }

                    --*vertexCount;
                    break;
                }

                ++vertexIndex;
                nextIndex = (nextIndex + 1) % *vertexCount;
            }
        } while (removedVertexThisPass != 0);

        return removedAnyVertices;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setnormalizedcrossfromvertextriplet
     * @recoil-artifact defines .text recoil:function:0x482c60: zModel_Const::SetNormalizedCrossFromVertexTriplet
     * @recoil-match byte
     *
     * Evidence: the stack argument at [esp+4] is the hidden by-value return slot
     * (stored through and returned in EAX; ret 8), and the only caller
     * (check_colinearity, 0x482b91) pushes it after vertex2 and copies the result.
     *
     * Purpose: compute and normalize the cross product from three polygon vertices.
     */
    zVec3 __fastcall SetNormalizedCrossFromVertexTriplet(zVec3 * vertex0, zVec3 * vertex1, zVec3 * vertex2)
    {
        zVec3 edge0;
        zVec3 edge2;
        edge2.y = vertex2->y - vertex1->y;
        edge2.z = vertex2->z - vertex1->z;
        edge0.y = vertex0->y - vertex1->y;
        edge0.z = vertex0->z - vertex1->z;
        edge2.x = vertex2->x - vertex1->x;
        edge0.x = vertex0->x - vertex1->x;

        // Retail builds and scales the cross product in place in this local.
        zVec3 normal;
        normal.x = edge0.z * edge2.y - edge0.y * edge2.z;
        normal.y = edge0.x * edge2.z - edge0.z * edge2.x;
        normal.z = edge0.y * edge2.x - edge0.x * edge2.y;

        float length;
        if (fabs(normal.x) > g_zModel_ColinearTolerance || fabs(normal.y) > g_zModel_ColinearTolerance
            || fabs(normal.z) > g_zModel_ColinearTolerance) {
            length = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        } else {
            length = 0.0f;
        }

        float scale;
        if (fabs(length) > g_zModel_ColinearTolerance) {
            scale = 1.0f / length;
        } else {
            scale = 0.0f;
        }

        normal.x = normal.x * scale;
        normal.y = normal.y * scale;
        normal.z = normal.z * scale;
        return normal;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.ispolygoncoplanar
     * @recoil-artifact defines .text recoil:function:0x482db0: zModel_Const::IsPolygonCoplanar
     * @recoil-match source
     *
     * Purpose: test whether every polygon vertex lies within the coplanar tolerance.
     */
    int __fastcall IsPolygonCoplanar(int vertexCount, zVec3* vertices)
    {
        zGeometry_PlaneEquationPartial plane;
        ComputePolygonPlaneEquation(vertexCount, vertices, &plane);

        int coplanar = 1;
        zVec3* vertex = vertices;
        for (int i = 0; i < vertexCount; ++i) {
            const double distance = vertex->x * plane.a + vertex->y * plane.b + vertex->z * plane.c + plane.d;
            if (fabs(distance) > g_zModel_CoplanarTolerance) {
                coplanar = 0;
                break;
            }
            ++vertex;
        }

        return coplanar;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.computepolygonplaneequation
     * @recoil-artifact defines .text recoil:function:0x482e30: zModel_Const::ComputePolygonPlaneEquation
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
     * @recoil-match byte
     *
     * Raw assembly: the reviewed full-XYZ dot island for the plane offset at
     * retail [0x482f92,0x482fb1); the
     * body otherwise mirrors zGeometry_Vec3Array::ComputeNewellPlane (0x46c3a0)
     * with a true square root instead of the fast estimate. gmod_const.c builds
     * /Ob0, so an inline helper cannot expand.
     *
     * Purpose: compute a normalized plane equation for a polygon.
     */
    zGeometry_PlaneEquationPartial* __fastcall ComputePolygonPlaneEquation(
        int vertexCount,
        zVec3* vertices,
        zGeometry_PlaneEquationPartial* outPlane
    )
    {
        zVec3 normal;
        zVec3 vertexSum;
        normal.z = 0.0f;
        normal.y = 0.0f;
        normal.x = 0.0f;
        vertexSum.z = 0.0f;
        vertexSum.y = 0.0f;
        vertexSum.x = 0.0f;

        for (int i = 0; i < vertexCount; ++i) {
            zVec3* const vertex = &vertices[i];
            zVec3* const next = &vertices[(i + 1) % vertexCount];

            normal.x += (vertex->y - next->y) * (vertex->z + next->z);
            normal.y += (vertex->z - next->z) * (vertex->x + next->x);
            normal.z += (vertex->x - next->x) * (vertex->y + next->y);

            vertexSum.x += vertex->x;
            vertexSum.y += vertex->y;
            vertexSum.z += vertex->z;
        }

        // Retail uses qword comparisons to the pooled double-zero object
        // at 0x4d2ae0; 0.0 reproduces those operands in this VC5SP3 build.
        float normalLength;
        if (normal.x == 0.0 && normal.y == 0.0 && normal.z == 0.0) {
            normalLength = 0.0f;
        } else {
            normalLength = (float)sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        }

        float inverseNormalLength;
        if (normalLength != 0.0) {
            inverseNormalLength = 1.0f / normalLength;
        } else {
            inverseNormalLength = 0.0f;
        }

        outPlane->a = normal.x * inverseNormalLength;
        outPlane->b = normal.y * inverseNormalLength;
        outPlane->c = normal.z * inverseNormalLength;

        float planeDot;
        ZMTH_VECTOR_DOT(planeDot, &vertexSum, &normal);
        // As in ComputeNewellPlane, a double-typed numerator temporary reproduces
        // retail's numerator-first x87 evaluation order under VC5.
        const double planeOffset = planeDot;
        outPlane->d = -(planeOffset / ((float)vertexCount * normalLength));
        return outPlane;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.splitpolygonchunkedbyvertexlimit
     * @recoil-artifact defines .text recoil:function:0x482fe0: zModel_Const::SplitPolygonChunkedByVertexLimit
     * @recoil-match byte
     *
     * Purpose: triangulate a polygon into fan triangles for AddPolygonEx.
     */
    void __fastcall SplitPolygonChunkedByVertexLimit(
        zDiPartial * self,
        int totalVertexCount,
        zVec3* points,
        zVec3* entryNormals,
        zClipUV* uvPairsA,
        zVec3* normalsA,
        zVec3* normalsBInput,
        zClipUV* uvPairsBInput,
        zModel_MaterialPartial* material,
        unsigned int drawFlags,
        int flagBit8,
        const int* userTag
    )
    {
        zVec3 trianglePoints[3];
        zVec3 triangleEntryNormals[3];
        zClipUV triangleUvPairsA[3];
        zVec3 triangleNormalsB[3];
        zClipUV triangleUvPairsB[3];
        zVec3* const polyEntryNormals = entryNormals != 0 ? triangleEntryNormals : 0;

        trianglePoints[0] = points[0];
        if (entryNormals != 0) {
            triangleEntryNormals[0] = entryNormals[0];
        }

        if ((material->flags & 0x0100) != 0) {
            triangleUvPairsA[0] = uvPairsA[0];
        }

        if (normalsA != 0) {
            triangleNormalsB[0] = normalsBInput[0];
            if ((material->flags & 0x0100) != 0) {
                triangleUvPairsB[0] = uvPairsBInput[0];
            }
        }

        for (int vertexIndex = 1; vertexIndex < totalVertexCount - 1; ++vertexIndex) {
            for (int triangleIndex = 1; triangleIndex < 3; ++triangleIndex) {
                const int sourceIndex = vertexIndex - 1 + triangleIndex;
                trianglePoints[triangleIndex] = points[sourceIndex];
                if (entryNormals != 0) {
                    triangleEntryNormals[triangleIndex] = entryNormals[sourceIndex];
                }
                if ((material->flags & 0x0100) != 0) {
                    triangleUvPairsA[triangleIndex] = uvPairsA[sourceIndex];
                }
                if (normalsA != 0) {
                    triangleNormalsB[triangleIndex] = normalsBInput[sourceIndex];
                    if ((material->flags & 0x0100) != 0) {
                        triangleUvPairsB[triangleIndex] = uvPairsBInput[sourceIndex];
                    }
                }
            }

            zDi::AddPolygonEx(
                self,
                3,
                trianglePoints,
                polyEntryNormals,
                triangleUvPairsA,
                normalsA,
                triangleNormalsB,
                triangleUvPairsB,
                material,
                drawFlags,
                flagBit8,
                userTag
            );
        }
    }
} // namespace zModel_Const

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addpolygonsplitbyvertexlimit
     * @recoil-artifact defines .text recoil:function:0x483240: zDi::AddPolygonSplitByVertexLimit
     * @recoil-match byte
     *
     * Purpose: split an oversized polygon into overlapping chunks within the vertex limit.
     */
    void __fastcall AddPolygonSplitByVertexLimit(
        zDiPartial * self,
        int totalVertexCount,
        zVec3* points,
        zVec3* entryNormals,
        zClipUV* uvPairsA,
        zVec3* normalsA,
        zVec3* normalsBInput,
        zClipUV* uvPairsBInput,
        zModel_MaterialPartial* material,
        unsigned int drawFlags,
        int flagBit8,
        const int* userTag,
        int maxChunkVertexCount
    )
    {
        zVec3 chunkPoints[4];
        zVec3 chunkEntryNormals[4];
        zClipUV chunkUvPairsA[4];
        zVec3 chunkNormalsB[4];
        zClipUV chunkUvPairsB[4];
        zVec3* const polyEntryNormals = entryNormals != 0 ? chunkEntryNormals : 0;

        const int clampedChunkVertexCount = maxChunkVertexCount > 4 ? 4 : maxChunkVertexCount;

        chunkPoints[0] = points[0];
        if (entryNormals != 0) {
            chunkEntryNormals[0] = entryNormals[0];
        }

        if ((material->flags & 0x0100) != 0) {
            chunkUvPairsA[0] = uvPairsA[0];
        }

        if (normalsA != 0) {
            chunkNormalsB[0] = normalsBInput[0];
            if ((material->flags & 0x0100) != 0) {
                chunkUvPairsB[0] = uvPairsBInput[0];
            }
        }

        int chunkStartVertexIndex = 1;
        // The chunk size persists across chunks; only the final chunk is shortened.
        int vertexCount = clampedChunkVertexCount;
        while (chunkStartVertexIndex < totalVertexCount - 1) {
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
                    if ((material->flags & 0x0100) != 0) {
                        chunkUvPairsA[chunkVertexIndex] = uvPairsA[sourceIndex];
                    }
                    if (normalsA != 0) {
                        chunkNormalsB[chunkVertexIndex] = normalsBInput[sourceIndex];
                        if ((material->flags & 0x0100) != 0) {
                            chunkUvPairsB[chunkVertexIndex] = uvPairsBInput[sourceIndex];
                        }
                    }
                }
            }

            if (vertexCount < 3) {
                zError::ReportOld(
                    0x400,
                    g_zModel_SourceFile_GmodConstC,
                    0xa16,
                    g_zModel_AddPolygonTooFewVertsFmt,
                    vertexCount
                );
            }

            AddPolygonEx(
                self,
                vertexCount,
                chunkPoints,
                polyEntryNormals,
                chunkUvPairsA,
                normalsA,
                chunkNormalsB,
                chunkUvPairsB,
                material,
                drawFlags,
                flagBit8,
                userTag
            );
            chunkStartVertexIndex += vertexCount - 2;
        }
    }
} // namespace zDi

namespace zModel_Const
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.quantizeandnormalizeuvpairs
     * @recoil-artifact defines .text recoil:function:0x483510: zModel_Const::QuantizeAndNormalizeUvPairs
     * @recoil-match byte
     *
     * Purpose: quantize UV pairs and normalize them to a local tile origin.
     */
    void __fastcall QuantizeAndNormalizeUvPairs(int vertexCount, zClipUV* uvPairs)
    {
        if (vertexCount > 0) {
            for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                float* const u = &uvPairs[vertexIndex].u;
                *u = (float)((int)((*u - g_zModel_UvQuantizeBias) * g_zModel_UvQuantizeScale))
                    * g_zModel_UvQuantizeInvScale;

                float* const v = &uvPairs[vertexIndex].v;
                *v = (float)((int)((*v - g_zModel_UvQuantizeBias) * g_zModel_UvQuantizeScale))
                    * g_zModel_UvQuantizeInvScale;
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

        const float baseU = (float)(floor(minU));
        const float baseV = (float)(floor(minV));
        for (int normalizeIndex = 0; normalizeIndex < vertexCount; ++normalizeIndex) {
            uvPairs[normalizeIndex].u -= baseU;
            uvPairs[normalizeIndex].v -= baseV;
        }
    }
} // namespace zModel_Const

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addpolygon
     * @recoil-artifact defines .text recoil:function:0x483610: zDi::AddPolygon
     * @recoil-match byte
     *
     * Purpose: add a polygon entry without explicit per-entry normals.
     */
    int __fastcall AddPolygon(
        zDiPartial * self,
        int pointCount,
        zVec3* points,
        zClipUV* uvPairsA,
        zVec3* normalsA,
        zVec3* normalsB,
        zClipUV* uvPairsB,
        zModel_MaterialPartial* material,
        unsigned int drawFlags,
        int flagBit8,
        const int* userTag
    )
    {
        return AddPolygonEx(
            self,
            pointCount,
            points,
            0,
            uvPairsA,
            normalsA,
            normalsB,
            uvPairsB,
            material,
            drawFlags,
            flagBit8,
            userTag
        );
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addpolygonex
     * @recoil-artifact defines .text recoil:function:0x483650: zDi::AddPolygonEx
     * @recoil-match byte
     *
     * Purpose: add a polygon entry with optional normals, UVs, splitting, and generated UV repair.
     */
    int __fastcall AddPolygonEx(
        zDiPartial * self,
        int vertexCount,
        zVec3* points,
        zVec3* entryNormals,
        zClipUV* uvPairsA,
        zVec3* normalsA,
        zVec3* normalsB,
        zClipUV* uvPairsB,
        zModel_MaterialPartial* material,
        unsigned int drawFlags,
        int flagBit8,
        const int* userTag
    )
    {
        int remainingVertexCount = vertexCount;
        if (vertexCount < 3) {
            zError::ReportOld(
                0x200,
                g_zModel_SourceFile_GmodConstC,
                0xae4,
                g_zModel_AddPolygonOnlyVertsErrorFmt,
                vertexCount
            );
            return 1;
        }

        if (vertexCount > 0x40 * 0.9) {
            zError::ReportOld(
                0x200,
                g_zModel_SourceFile_GmodConstC,
                0xaed,
                g_zModel_PolyVertexCountApproachingLimitFmt,
                vertexCount,
                0x40
            );
            return 1;
        }

        if (zModel_Const::check_colinearity(&remainingVertexCount, points, uvPairsA, normalsB, uvPairsB) != 0
            && remainingVertexCount < 3) {
            zError::ReportOld(
                0x100,
                g_zModel_SourceFile_GmodConstC,
                0xb0d,
                g_zModel_DiscardPolygonAfterCheckColinearityFmt,
                remainingVertexCount,
                vertexCount
            );
            return 1;
        }

        if (remainingVertexCount > 3 && zModel_Const::IsPolygonCoplanar(remainingVertexCount, points) == 0) {
            zError::ReportOld(
                0x100,
                g_zModel_SourceFile_GmodConstC,
                0xb19,
                g_zModel_AddNonPlanarPolygonTriangulatingFmt,
                remainingVertexCount
            );
            zModel_Const::SplitPolygonChunkedByVertexLimit(
                self,
                vertexCount,
                points,
                entryNormals,
                uvPairsA,
                normalsA,
                normalsB,
                uvPairsB,
                material,
                drawFlags,
                flagBit8,
                userTag
            );
            return 2;
        }

        if (remainingVertexCount > g_zModel_MaxPolygonVertexCountBeforeSplit) {
            AddPolygonSplitByVertexLimit(
                self,
                vertexCount,
                points,
                entryNormals,
                uvPairsA,
                normalsA,
                normalsB,
                uvPairsB,
                material,
                drawFlags,
                flagBit8,
                userTag,
                g_zModel_MaxPolygonVertexCountBeforeSplit
            );
            return 2;
        }

        self->entries
            = (zDiEntryPartial*)(realloc(self->entries, (size_t)(self->entryCount + 1) * sizeof(zDiEntryPartial)));
        zDiEntryPartial* const entry = &self->entries[self->entryCount];
        int i;
        memset(entry, 0, sizeof(zDiEntryPartial));
        entry->flagsAndIndexCount
            = (entry->flagsAndIndexCount & ~0xffu) | ((unsigned int)(remainingVertexCount) & 0xff);
        entry->drawFlags = drawFlags;
        entry->flagsAndIndexCount = (entry->flagsAndIndexCount & ~0x100u) | ((unsigned int)(flagBit8 & 1) << 8);
        if (entryNormals != 0) {
            entry->flagsAndIndexCount |= 0x200;
        } else {
            entry->flagsAndIndexCount &= ~0x200u;
        }
        entry->vertexIndices = malloc((size_t)(remainingVertexCount) * sizeof(int));
        if (entryNormals != 0) {
            entry->normalIndices = malloc((size_t)(remainingVertexCount) * sizeof(int));
        }

        zVec3* normalBCursor = normalsB;
        zVec3* entryNormalCursor = entryNormals;
        for (i = 0; i < remainingVertexCount; ++i) {
            if (normalsA != 0) {
                ((int*)(entry->vertexIndices))[i]
                    = zModel_Const::AddOrMergeVertexAndNormal(self, points, normalBCursor);
                if (((int*)(entry->vertexIndices))[i] < 0) {
                    return 1;
                }
                ++normalBCursor;
            } else {
                ((int*)(entry->vertexIndices))[i] = zModel_Const::AddOrMergeVertex(self, points);
                if (((int*)(entry->vertexIndices))[i] < 0) {
                    return 1;
                }
            }

            if (entryNormalCursor != 0) {
                ((int*)(entry->normalIndices))[i] = zModel_Const::FindOrAppendNormalIndex(self, entryNormalCursor);
                ++entryNormalCursor;
            }
            ++points;
        }

        if ((material->flags & 0x0100) != 0) {
            entry->uvPairs = malloc((size_t)(remainingVertexCount) * sizeof(zClipUV));
            for (i = 0; i < remainingVertexCount; ++i) {
                ((zClipUV*)(entry->uvPairs))[i].u = uvPairsA[i].u;
                ((zClipUV*)(entry->uvPairs))[i].v = uvPairsA[i].v;
            }

            zClipUV* const entryUvPairs = (zClipUV*)(entry->uvPairs);
            float minU = entryUvPairs[0].u;
            float minV = entryUvPairs[0].v;
            for (i = 1; i < remainingVertexCount; ++i) {
                if (entryUvPairs[i].u < minU) {
                    minU = entryUvPairs[i].u;
                }
                if (entryUvPairs[i].v < minV) {
                    minV = entryUvPairs[i].v;
                }
            }

            const float baseU = (float)(floor(minU));
            const float baseV = (float)(floor(minV));
            for (i = 0; i < remainingVertexCount; ++i) {
                ((zClipUV*)(entry->uvPairs))[i].u -= baseU;
                ((zClipUV*)(entry->uvPairs))[i].v -= baseV;
            }
        }

        entry->material = material;
        RebuildGeneratedUvPairsForEntry(self, self->entryCount);
        if ((material->flags & 0x0100) != 0) {
            zModel_Const::QuantizeAndNormalizeUvPairs(remainingVertexCount, (zClipUV*)(entry->uvPairs));
        }
        memcpy(&entry->variantTagInitialized, userTag, sizeof(*userTag));

        ++self->entryCount;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.hasspecialflagsorauxmaterialdata
     * @recoil-artifact defines .text recoil:function:0x483a60: zDi::HasSpecialFlagsOrAuxMaterialData
     * @recoil-match byte
     *
     * Purpose: test whether a display instance needs special render/material handling.
     */
    int __fastcall HasSpecialFlagsOrAuxMaterialData(zDiPartial * self)
    {
        if (self == 0) {
            return 0;
        }

        if ((self->flags & 0x04) != 0) {
            return 1;
        }
        if ((self->flags & 0x08) != 0) {
            return 1;
        }
        if ((self->flags & 0x20) != 0) {
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.rebuildbounds
     * @recoil-artifact defines .text recoil:function:0x483ad0: zDi::RebuildBounds
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.rebuild-bounds.fast-sqrt-estimate recoil:function:0x483ad0
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.rebuild-bounds.fast-sqrt-estimate
     * @recoil-match byte
     *
     * Raw assembly: one in-body 13-byte fast-sqrt estimate island at retail
     * [0x483b5b,0x483b68).
     *
     *
     * Purpose: rebuild display-instance bounds, center, and approximate bounding radius.
     */
    void __fastcall RebuildBounds(zDiPartial * self, zBoundsMinMaxPartial * outBoundsMinMax)
    {
        if (self == 0 || outBoundsMinMax == 0) {
            return;
        }

        if (self->mode == 0) {
            BuildAabb(self, outBoundsMinMax);
        } else if (self->mode == 1) {
            BuildOriginSymmetricAabb(self, outBoundsMinMax);
        }

        float halfX;
        float halfY;
        float halfZ;
        self->bboxCenter.x
            = (halfX = (outBoundsMinMax->max.x - outBoundsMinMax->min.x) * 0.5f) + outBoundsMinMax->min.x;
        self->bboxCenter.y
            = (halfY = (outBoundsMinMax->max.y - outBoundsMinMax->min.y) * 0.5f) + outBoundsMinMax->min.y;
        self->bboxCenter.z
            = (halfZ = (outBoundsMinMax->max.z - outBoundsMinMax->min.z) * 0.5f) + outBoundsMinMax->min.z;
        self->bboxRadius = halfX * halfX + halfY * halfY + halfZ * halfZ;
        float lengthSq = self->bboxRadius;
        float radiusEstimate;
        // Raw-assembly fast square-root estimate: retail transforms the named lengthSq
        // bits through EAX ((bits >> 1) + 0x1fc00000) into the named result local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
            mov eax, lengthSq
            sar eax, 1
            add eax, 01fc00000h
            mov radiusEstimate, eax
        }
#else
        {
            int estimateBits;
            memcpy(&estimateBits, &lengthSq, sizeof estimateBits);
            estimateBits = (estimateBits >> 1) + 0x1fc00000;
            memcpy(&radiusEstimate, &estimateBits, sizeof radiusEstimate);
        }
#endif
        self->bboxRadius = radiusEstimate;
    }

/*
 * Grow a [minValue, maxValue] range to include value. Retail compares and copies the
 * parenthesized arguments (fld/fld/fcompp compares, fld/fstp copies).
 */
#define GMOD_EXPAND_BOUNDS(minValue, maxValue, value)                                                                  \
    if ((value) < (minValue)) {                                                                                        \
        (minValue) = (value);                                                                                          \
    }                                                                                                                  \
    if ((value) > (maxValue)) {                                                                                        \
        (maxValue) = (value);                                                                                          \
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildaabb
     * @recoil-artifact defines .text recoil:function:0x483b80: zDi::BuildAabb
     * @recoil-match byte
     *
     * Purpose: build a display-instance axis-aligned bounds box from vertices and point data.
     */
    void __fastcall BuildAabb(zDiPartial * self, zBoundsMinMaxPartial * outBoundsMinMax)
    {
        int i;
        int j;

        if (self->vertCount > 0) {
            outBoundsMinMax->min.x = self->verts[0].x;
            outBoundsMinMax->max.x = self->verts[0].x;
            outBoundsMinMax->min.y = self->verts[0].y;
            outBoundsMinMax->max.y = self->verts[0].y;
            outBoundsMinMax->min.z = self->verts[0].z;
            outBoundsMinMax->max.z = self->verts[0].z;
        } else if (self->pointCount > 0) {
            outBoundsMinMax->min.x = self->pointEntries[0].pointCamList[0].x;
            outBoundsMinMax->max.x = self->pointEntries[0].pointCamList[0].x;
            outBoundsMinMax->min.y = self->pointEntries[0].pointCamList[0].y;
            outBoundsMinMax->max.y = self->pointEntries[0].pointCamList[0].y;
            outBoundsMinMax->min.z = self->pointEntries[0].pointCamList[0].z;
            outBoundsMinMax->max.z = self->pointEntries[0].pointCamList[0].z;
        }

        zModel_PointEntryPartial* entry = self->pointEntries;
        for (i = 0; i < self->pointCount; ++i, ++entry) {
            for (j = 0; j < entry->pointCamCount; ++j) {
                GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.x, outBoundsMinMax->max.x, entry->pointCamList[j].x);
                GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.y, outBoundsMinMax->max.y, entry->pointCamList[j].y);
                GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.z, outBoundsMinMax->max.z, entry->pointCamList[j].z);
            }
        }

        const zVec3* point = self->verts + 1;
        for (i = 1; i < self->vertCount; ++i, ++point) {
            GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.x, outBoundsMinMax->max.x, point->x);
            GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.y, outBoundsMinMax->max.y, point->y);
            GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.z, outBoundsMinMax->max.z, point->z);
        }

        if (self->blendVertCount != 0) {
            zMathVec3ArrayAddScaled(
                g_zModel_SharedVec3ScratchA,
                self->verts,
                self->blendVerts,
                self->blendVertCount,
                1.0f
            );
            point = g_zModel_SharedVec3ScratchA;
            for (i = 0; i < self->blendVertCount; ++i, ++point) {
                GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.x, outBoundsMinMax->max.x, point->x);
                GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.y, outBoundsMinMax->max.y, point->y);
                GMOD_EXPAND_BOUNDS(outBoundsMinMax->min.z, outBoundsMinMax->max.z, point->z);
            }
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildoriginsymmetricaabb
     * @recoil-artifact defines .text recoil:function:0x483e60: zDi::BuildOriginSymmetricAabb
     * @recoil-match byte
     *
     * Purpose: symmetrize display-instance bounds around the origin according to mode flags.
     */
    void __fastcall BuildOriginSymmetricAabb(zDiPartial * self, zBoundsMinMaxPartial * outBoundsMinMax)
    {
        BuildAabb(self, outBoundsMinMax);

        zVec3 extent = outBoundsMinMax->max;
        if (fabs(outBoundsMinMax->min.x) > extent.x) {
            extent.x = (float)fabs(outBoundsMinMax->min.x);
        }
        if (fabs(outBoundsMinMax->min.y) > extent.y) {
            extent.y = (float)fabs(outBoundsMinMax->min.y);
        }
        if (fabs(outBoundsMinMax->min.z) > extent.z) {
            extent.z = (float)fabs(outBoundsMinMax->min.z);
        }

        if ((self->flags & 0x10) != 0) {
            // Retail keeps the running maximum in extent.x on the x87 stack (0x483e7d..0x483f0b);
            // the extent stores are dead after the bounds stores.
            if (extent.y > extent.x) {
                extent.x = extent.y;
            }
            if (extent.z > extent.x) {
                extent.x = extent.z;
            }
            outBoundsMinMax->max.x = extent.x;
            outBoundsMinMax->min.x = -extent.x;
            outBoundsMinMax->max.y = extent.x;
            outBoundsMinMax->min.y = -extent.x;
            outBoundsMinMax->max.z = extent.x;
            outBoundsMinMax->min.z = -extent.x;
            return;
        }

        if (extent.x > extent.z) {
            extent.z = extent.x;
        } else {
            extent.x = extent.z;
        }

        outBoundsMinMax->max.x = extent.x;
        outBoundsMinMax->min.x = -extent.x;
        outBoundsMinMax->max.y = extent.y;
        outBoundsMinMax->min.y = -extent.y;
        outBoundsMinMax->max.z = extent.z;
        outBoundsMinMax->min.z = -extent.z;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.buildblendvertsfromconnectivity
     * @recoil-artifact defines .text recoil:function:0x483f80: zDi::BuildBlendVertsFromConnectivity
     * @recoil-match byte
     *
     * Retail 0x4c4228..0x4c4241 passes self in ECX, exclusions in EDX,
     * and blendY on the stack. VC5 skips floating-point fastcall arguments when
     * assigning registers: the source order is self, blendY, exclusions. This
     * order reproduces the complete caller while leaving this callee unchanged.
     * Purpose: build blend-vertex offsets from connectivity and exclusion rules.
     */
    void __fastcall BuildBlendVertsFromConnectivity(
        zDiPartial * self,
        float blendY,
        int* excludedVertexIndices,
        int excludedVertexCount,
        int minSharedVertexCount
    )
    {
        self->blendVerts = (zVec3*)(realloc(self->blendVerts, (size_t)(self->vertCount) * sizeof(zVec3)));

        int* const blendDisabledMask = (int*)(malloc((size_t)(self->vertCount) * sizeof(int)));
        int* const vertexReferenceCounts = (int*)(malloc((size_t)(self->vertCount) * sizeof(int)));

        for (int vertexIndex = 0; vertexIndex < self->vertCount; ++vertexIndex) {
            blendDisabledMask[vertexIndex] = 0;
            vertexReferenceCounts[vertexIndex] = 0;
        }

        for (int excludeIndex = 0; excludeIndex < excludedVertexCount; ++excludeIndex) {
            blendDisabledMask[excludedVertexIndices[excludeIndex]] = 1;
        }

        zDiEntryPartial* entry = self->entries;
        for (int entryIndex = 0; entryIndex < self->entryCount; ++entryIndex, ++entry) {
            for (unsigned int entryVertexIndex = 0; entryVertexIndex < (entry->flagsAndIndexCount & 0xff);
                ++entryVertexIndex) {
                ++vertexReferenceCounts[((int*)(entry->vertexIndices))[entryVertexIndex]];
            }
        }

        if (minSharedVertexCount > 0) {
            for (int vertexIndex = 0; vertexIndex < self->vertCount; ++vertexIndex) {
                if (vertexReferenceCounts[vertexIndex] < minSharedVertexCount) {
                    blendDisabledMask[vertexIndex] = 1;
                }
            }
        }

        for (int blendVertexIndex = 0; blendVertexIndex < self->vertCount; ++blendVertexIndex) {
            int enableBlendY = 1;
            for (int excludeIndex = 0; excludeIndex < excludedVertexCount && enableBlendY != 0; ++excludeIndex) {
                if (excludedVertexIndices[excludeIndex] == blendVertexIndex) {
                    enableBlendY = 0;
                }
            }
            if (blendDisabledMask[blendVertexIndex] == 1) {
                enableBlendY = 0;
            }

            self->blendVerts[blendVertexIndex].x = 0.0f;
            if (enableBlendY != 0) {
                self->blendVerts[blendVertexIndex].y = blendY;
            } else {
                self->blendVerts[blendVertexIndex].y = 0.0f;
            }
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setentryvalueforallentries
     * @recoil-artifact defines .text recoil:function:0x484140: zDi::SetEntryValueForAllEntries
     * @recoil-match byte
     *
     * Purpose: set the draw-flags value for every display-instance polygon entry.
     */
    void __fastcall SetEntryValueForAllEntries(zDiPartial * self, unsigned int entryValue)
    {
        if (self == 0) {
            return;
        }

        for (int i = 0; i < self->entryCount; ++i) {
            self->entries[i].drawFlags = entryValue;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setshowbackfaceforallentries
     * @recoil-artifact defines .text recoil:function:0x484170: zDi::SetShowBackFaceForAllEntries
     * @recoil-match byte
     *
     * Purpose: update the show-backface bit on every display-instance polygon entry.
     */
    void __fastcall SetShowBackFaceForAllEntries(zDiPartial * self, int enabled)
    {
        for (int i = 0; i < self->entryCount; ++i) {
            self->entries[i].flagsAndIndexCount
                = (self->entries[i].flagsAndIndexCount & ~0x0100u) | ((enabled & 1) << 8);
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setmaterialflagbit9forflagbit0entries
     * @recoil-artifact defines .text recoil:function:0x4841b0: zDi::SetMaterialFlagBit9ForFlagBit0Entries
     * @recoil-match byte
     *
     * Purpose: set material flag bit 9 for display-instance materials whose
     * flag bit 8 (0x0100) is set.
     */
    void __fastcall SetMaterialFlagBit9ForFlagBit0Entries(zDiPartial * self, int enabled)
    {
        for (int i = 0; i < self->entryCount; ++i) {
            zModel_MaterialPartial* material = self->entries[i].material;
            if ((material->flags & 0x0100) != 0) {
                zModel_Material::SetFlagBit9(material, enabled);
            }
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.invalidateimagesforflagbit8materials
     * @recoil-artifact defines .text recoil:function:0x4841f0: zDi::InvalidateImagesForFlagBit8Materials
     * @recoil-match byte
     *
     * Purpose: invalidate eligible images for display-instance materials selected by flag bit 0.
     */
    void __fastcall InvalidateImagesForFlagBit8Materials(zDiPartial * self)
    {
        for (int i = 0; i < self->entryCount; ++i) {
            zModel_MaterialPartial* material = self->entries[i].material;
            if ((material->flags & 0x0100) != 0) {
                zModel_Material::InvalidateImagesIfEligible(material);
            }
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.resetcurrentvariant
     * @recoil-artifact defines .text recoil:function:0x484230: zDi::ResetCurrentVariant
     * @recoil-match byte
     *
     * Purpose: reset the current material cycle frame on the first entry.
     */
    void __fastcall ResetCurrentVariant(zDiPartial * self)
    {
        zModel_MaterialPartial* const material = self->entries->material;
        if (material->cycle != 0) {
            material->cycle->currentFrame = 0.0f;
            material->currentTextureDirectoryEntry = material->cycle->frameTable[0];
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setcurrentvariantcycletexturecount
     * @recoil-artifact defines .text recoil:function:0x484250: zDi::SetCurrentVariantCycleTextureCount
     * @recoil-match byte
     *
     * Purpose: configure the current material cycle texture count.
     */
    int __fastcall SetCurrentVariantCycleTextureCount(zDiPartial * self, int textureCount)
    {
        if (self == 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_zModel_SetModelCycleTextureNullModelFmt,
                g_zModel_SourceFile_GmodConstC,
                0xf3f
            );
            fprintf(stderr, g_zError_DebugMsgBuffer);
            return -1;
        }

        zModel_MaterialPartial* const material = self->entries->material;
        if (material == 0) {
            // Original code dereferences the null material pointer here while
            // clearing the cycle-texture flag.
            material->flags = (unsigned short)(material->flags & 0xfbff);
            return 0;
        }

        zModel_Material::SetCycleTextureCount(material, textureCount);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setcurrentvariant
     * @recoil-artifact defines .text recoil:function:0x4842b0: zDi::SetCurrentVariant
     * @recoil-match byte
     *
     * Purpose: select the current texture-cycle variant frame.
     */
    void __fastcall SetCurrentVariant(zDiPartial * self, int variantIndex)
    {
        zModel_MaterialPartial* const material = self->entries->material;
        zModel_MaterialCyclePartial* const cycle = material->cycle;
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
        cycle->currentFrame = (float)(variantIndex);
    }
} // namespace zDi

namespace zModel_Instance
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setcycletextureloop
     * @recoil-artifact defines .text recoil:function:0x4842f0: zModel_Instance::SetCycleTextureLoop
     * @recoil-match byte
     *
     * Purpose: set the cycle loop flag on an instance's first material entry.
     */
    int __fastcall SetCycleTextureLoop(zDiPartial * instance, int loopEnabled)
    {
        if (instance == 0) {
            return 0;
        }

        return zModel_Material::SetCycleTextureLoop(instance->entries->material, loopEnabled);
    }
} // namespace zModel_Instance

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setcurrentvariantcycletexturespeed
     * @recoil-artifact defines .text recoil:function:0x484310: zDi::SetCurrentVariantCycleTextureSpeed
     * @recoil-match byte
     *
     * Purpose: set the cycle speed for the current material variant.
     */
    int __fastcall SetCurrentVariantCycleTextureSpeed(zDiPartial * self, float cycleSpeed)
    {
        if (self == 0) {
            return 0;
        }

        return zModel_Material::SetCycleTextureSpeed(self->entries->material, cycleSpeed);
    }
} // namespace zDi

namespace zModel_Instance
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.addcycletexture
     * @recoil-artifact defines .text recoil:function:0x484330: zModel_Instance::AddCycleTexture
     * @recoil-match byte
     *
     * Purpose: add a cycle texture to an instance's first material entry.
     */
    int __fastcall AddCycleTexture(zDiPartial * instance, zImage_TexDirEntryPartial * textureDirectoryEntry)
    {
        if (instance == 0) {
            return 0;
        }

        return zModel_Material::AddCycleTexture(instance->entries->material, textureDirectoryEntry);
    }
} // namespace zModel_Instance

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.setobject3dcolormodeformaterials
     * @recoil-artifact defines .text recoil:function:0x484350: zDi::SetObject3DColorModeForMaterials
     * @recoil-match byte
     *
     * Purpose: apply an object3D color mode to untextured materials.
     */
    void __fastcall SetObject3DColorModeForMaterials(zDiPartial * self, int colorMode)
    {
        zDiEntryPartial* entry = self->entries;
        for (int i = 0; i < self->entryCount; ++i, ++entry) {
            if ((entry->material->flags & 0x0100) != 0) {
                continue;
            }

            entry->material->colorRgb.red = (float)(colorMode);
            entry->material->colorRgb.green = 0.0f;
            entry->material->colorRgb.blue = 0.0f;
            // Retail stores only the high byte of packedColor (mov byte ptr [material+3]).
            ((unsigned char*)(&entry->material->packedColor))[1] = (unsigned char)(colorMode);
            entry->material->colorScalar = 1.0f;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.rebuildgenerateduvpairsforentry
     * @recoil-artifact defines .text recoil:function:0x4843b0: zDi::RebuildGeneratedUvPairsForEntry
     *
     *
     * Purpose: rebuild generated UV pairs for polygon vertices beyond the first triangle.
     */
    void __fastcall RebuildGeneratedUvPairsForEntry(zDiPartial * self, int entryIndex)
    {
        zDiEntryPartial* const entry = &self->entries[entryIndex];
        const int vertexCount = (int)(entry->flagsAndIndexCount & 0xff);
        // Retail tests the material flag without a null check.
        if ((entry->material->flags & 0x0100) == 0 || vertexCount <= 3) {
            return;
        }

        // Retail solves from a local copy of the entry UV pairs.
        zClipUV uvPairs[0x40];
        memcpy(uvPairs, entry->uvPairs, vertexCount * sizeof(zClipUV));

        zVec3 normal;
        zMathVec3TriangleNormal(
            &self->verts[((int*)(entry->vertexIndices))[0]],
            &self->verts[((int*)(entry->vertexIndices))[1]],
            &self->verts[((int*)(entry->vertexIndices))[2]],
            &normal
        );
        zMath::Vec3Normalize(&normal);

        const float absX = (float)(fabs(normal.x));
        const float absY = (float)(fabs(normal.y));
        const float absZ = (float)(fabs(normal.z));
        const int* indices;
        float planeA0;
        float planeB0;
        float planeA1;
        float planeB1;
        float planeA2;
        float planeB2;
        zClipUV uGradient;
        zClipUV vGradient;
        float deltaA;
        float deltaB;

        if (absX >= absY && absX >= absZ) {
            indices = (const int*)(entry->vertexIndices);
            planeA0 = self->verts[indices[0]].y;
            planeB0 = self->verts[indices[0]].z;
            planeA1 = self->verts[indices[1]].y;
            planeB1 = self->verts[indices[1]].z;
            planeA2 = self->verts[indices[2]].y;
            planeB2 = self->verts[indices[2]].z;
            uGradient = zModel_Const::SolveTriScalarGradient2D(
                planeA0,
                planeB0,
                planeA1,
                planeB1,
                planeA2,
                planeB2,
                uvPairs[0].u,
                uvPairs[1].u,
                uvPairs[2].u
            );
            vGradient = zModel_Const::SolveTriScalarGradient2D(
                planeA0,
                planeB0,
                planeA1,
                planeB1,
                planeA2,
                planeB2,
                uvPairs[0].v,
                uvPairs[1].v,
                uvPairs[2].v
            );

            for (int vertexIndex = 3; vertexIndex < vertexCount; ++vertexIndex) {
                indices = (const int*)(entry->vertexIndices);
                const zVec3* const vertex = &self->verts[indices[vertexIndex]];
                const zVec3* const vertex0 = &self->verts[indices[0]];
                deltaA = vertex->y - vertex0->y;
                deltaB = vertex->z - vertex0->z;
                ((zClipUV*)(self->entries[entryIndex].uvPairs))[vertexIndex].u
                    = deltaA * uGradient.u + deltaB * uGradient.v + uvPairs[0].u;
                ((zClipUV*)(self->entries[entryIndex].uvPairs))[vertexIndex].v
                    = deltaA * vGradient.u + deltaB * vGradient.v + uvPairs[0].v;
            }
        } else if (absY >= absX && absY >= absZ) {
            indices = (const int*)(entry->vertexIndices);
            planeA0 = self->verts[indices[0]].z;
            planeB0 = self->verts[indices[0]].x;
            planeA1 = self->verts[indices[1]].z;
            planeB1 = self->verts[indices[1]].x;
            planeA2 = self->verts[indices[2]].z;
            planeB2 = self->verts[indices[2]].x;
            uGradient = zModel_Const::SolveTriScalarGradient2D(
                planeA0,
                planeB0,
                planeA1,
                planeB1,
                planeA2,
                planeB2,
                uvPairs[0].u,
                uvPairs[1].u,
                uvPairs[2].u
            );
            vGradient = zModel_Const::SolveTriScalarGradient2D(
                planeA0,
                planeB0,
                planeA1,
                planeB1,
                planeA2,
                planeB2,
                uvPairs[0].v,
                uvPairs[1].v,
                uvPairs[2].v
            );

            for (int vertexIndex = 3; vertexIndex < vertexCount; ++vertexIndex) {
                indices = (const int*)(entry->vertexIndices);
                const zVec3* const vertex = &self->verts[indices[vertexIndex]];
                const zVec3* const vertex0 = &self->verts[indices[0]];
                deltaA = vertex->z - vertex0->z;
                deltaB = vertex->x - vertex0->x;
                ((zClipUV*)(self->entries[entryIndex].uvPairs))[vertexIndex].u
                    = deltaA * uGradient.u + deltaB * uGradient.v + uvPairs[0].u;
                ((zClipUV*)(self->entries[entryIndex].uvPairs))[vertexIndex].v
                    = deltaA * vGradient.u + deltaB * vGradient.v + uvPairs[0].v;
            }
        } else {
            indices = (const int*)(entry->vertexIndices);
            planeA0 = self->verts[indices[0]].x;
            planeB0 = self->verts[indices[0]].y;
            planeA1 = self->verts[indices[1]].x;
            planeB1 = self->verts[indices[1]].y;
            planeA2 = self->verts[indices[2]].x;
            planeB2 = self->verts[indices[2]].y;
            uGradient = zModel_Const::SolveTriScalarGradient2D(
                planeA0,
                planeB0,
                planeA1,
                planeB1,
                planeA2,
                planeB2,
                uvPairs[0].u,
                uvPairs[1].u,
                uvPairs[2].u
            );
            vGradient = zModel_Const::SolveTriScalarGradient2D(
                planeA0,
                planeB0,
                planeA1,
                planeB1,
                planeA2,
                planeB2,
                uvPairs[0].v,
                uvPairs[1].v,
                uvPairs[2].v
            );

            for (int vertexIndex = 3; vertexIndex < vertexCount; ++vertexIndex) {
                indices = (const int*)(entry->vertexIndices);
                const zVec3* const vertex = &self->verts[indices[vertexIndex]];
                const zVec3* const vertex0 = &self->verts[indices[0]];
                deltaA = vertex->x - vertex0->x;
                deltaB = vertex->y - vertex0->y;
                ((zClipUV*)(self->entries[entryIndex].uvPairs))[vertexIndex].u
                    = deltaA * uGradient.u + deltaB * uGradient.v + uvPairs[0].u;
                ((zClipUV*)(self->entries[entryIndex].uvPairs))[vertexIndex].v
                    = deltaA * vGradient.u + deltaB * vGradient.v + uvPairs[0].v;
            }
        }
    }
} // namespace zDi

namespace zModel_Const
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zmodel.gmod-const.solvetriscalargradient2d
     * @recoil-artifact defines .text recoil:function:0x484860: zModel_Const::SolveTriScalarGradient2D
     * @recoil-match byte
     *
     * Purpose: solve the 2D scalar gradient over a triangle.
     */
    zClipUV __stdcall SolveTriScalarGradient2D(
        float vertex0A,
        float vertex0B,
        float vertex1A,
        float vertex1B,
        float vertex2A,
        float vertex2B,
        float value0,
        float value1,
        float value2
    )
    {
        zVec3 edge10;
        zVec3 edge20;
        edge10.x = vertex0A - vertex1A;
        edge10.y = vertex0B - vertex1B;
        edge10.z = value0 - value1;
        edge20.x = vertex2A - vertex1A;
        edge20.y = vertex2B - vertex1B;
        edge20.z = value2 - value1;

        float normalZ = edge20.y * edge10.x - edge20.x * edge10.y;
        const float normalX = edge20.z * edge10.y - edge20.y * edge10.z;
        const float normalY = edge20.x * edge10.z - edge20.z * edge10.x;

        zClipUV gradient;
        if (normalZ != 0.0f) {
            normalZ = 1.0f / normalZ;
            gradient.u = -normalX * normalZ;
            gradient.v = -normalY * normalZ;
            return gradient;
        }

        gradient.u = 0.0f;
        gradient.v = 0.0f;
        return gradient;
    }
} // namespace zModel_Const
